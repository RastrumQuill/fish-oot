#!/usr/bin/env python3
"""The bed-skin test (fishmod/build.sh --bedskin): Lake Hylia's bed under the full lake, cut finer and coloured per
vertex, to make the bed itself look lush instead of scattering moss mats on it.

The room's ground is lit by the scene's lights, so its vertices hold normals, not colours (gen_bedshade.py). This
makes a second, finer copy of the bed under the water instead, lying on it, with its own vertex colours: greener
where the weed beds grow (from the placement before any thinning), in soft patches, darker in deep water, fading to
the plain bed toward the surface. It lies on the bed and is drawn over it as a decal (the depth test allows for
coplanar surfaces). It uses the bed's own sand texture; the game draws it near the camera when the camera
is under water, unlit, times the scene's light (as the props are). Without FISHMOD_BEDSKIN=1 it writes an empty file.

Made from the scene's own geometry (the extracted assets), so written to src/fishmod_data/ (not committed).

Usage: fishmod/tools/gen_bedskin.py [--edge 180] [--png preview.png]
"""
import argparse
import glob
import math
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
SCENE = os.path.join(ROOT, "extracted", "ntsc-1.2", "assets", "scenes", "overworld", "spot06")
OUT = os.path.join(ROOT, "src", "fishmod_data", "fishmod_bed_spot06.c")
SURFACE = -1313  # the full lake
LIFT = 0         # on the vanilla bed: drawn over it as a decal (the depth test allows for coplanar)
CELL = 512       # chunks the game picks by distance

# Colour (multiplies the sand texture; 1 = the bed as it is)
GREEN = (0.86, 1.00, 0.68)  # weed-bed silt
OLIVE = (0.90, 0.86, 0.66)  # browner patches
SIGMA = 150.0               # how far a plant greens the bed around it
DENSE = 5.0                 # this much plant nearby is a full bed
DEEP0, DEEP1 = 250.0, 1100.0
FADE = 60.0                 # up to this far under the surface, less of it (no seam at the waterline)
# plant weights (how much bed each greens); the same kinds as gen_bedshade's shade
PLANTS = {"bush_weed": 1.0, "broadleaf": 1.0, "big_pondweed": 1.2, "kelp_frond": 1.0, "tall_weed": 1.0,
          "ribbon_weed": 0.8, "grass_tuft": 0.6, "fan_weed": 0.8, "vine_strand": 0.6, "lily_pad": 0.5,
          "spatterdock": 0.5, "pond_reed": 0.4, "reed_clump": 0.5, "cattails": 0.5}

VTX = re.compile(r"VTX\(\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\w+),\s*(-?\w+),")
LOAD = re.compile(r"gsDPLoadTextureBlock\((\w+), G_IM_FMT_(\w+), G_IM_SIZ_(\w+), \w+,\s*\w+, 0, ([^,]+), ([^,]+), "
                  r"(\w+), (\w+), (\w+), (\w+)\)")
# the second texture (the bed's materials blend it over the first by the environment's alpha: on the lake bed it's the
# sand, the first the bright ripples): the copy uses the second where there is one
LOAD1 = re.compile(r"gsDPLoadMultiBlock\((\w+), \w+, 1, G_IM_FMT_(\w+), G_IM_SIZ_(\w+), \w+,\s*\w+, 0, ([^,]+), "
                   r"([^,]+), (\w+), (\w+), (\w+), (\w+)\)")


def tx_flags(s):
    v = 0
    for part in s.split("|"):
        part = part.strip()
        v |= {"G_TX_NOMIRROR": 0, "G_TX_WRAP": 0, "G_TX_MIRROR": 1, "G_TX_CLAMP": 2}[part]
    return v


def tx_num(s):
    return 0 if s == "G_TX_NOLOD" else int(s, 0)


def tex_size(name):
    h = open(os.path.join(SCENE, "spot06_scene.h")).read()
    w = int(re.search(rf"{name}_WIDTH (\d+)", h).group(1))
    ht = int(re.search(rf"{name}_HEIGHT (\d+)", h).group(1))
    return w, ht


def room_triangles():
    """Every triangle of the room's display lists: (texture load, three (x, y, z, s, t))."""
    arrays = {}

    def vtx(name):
        if name not in arrays:
            path = glob.glob(os.path.join(SCENE, f"*{name}*.inc.c"))[0]
            arrays[name] = [tuple(int(m.group(i), 0) for i in range(1, 6)) for m in VTX.finditer(open(path).read())]
        return arrays[name]

    tris = []
    for dl in sorted(glob.glob(os.path.join(SCENE, "gSpot06DL_*_DL.inc.c"))):
        text = re.sub(r"\n\s+", " ", open(dl).read())
        buf = [None] * 64
        load = None
        for line in text.split("),"):
            line = line.strip() + ")"
            m = LOAD.search(line) or LOAD1.search(line)
            if m:
                load = (m.group(1), m.group(2), m.group(3), tx_flags(m.group(4)), tx_flags(m.group(5)),
                        tx_num(m.group(6)), tx_num(m.group(7)), tx_num(m.group(8)), tx_num(m.group(9)))
            m = re.search(r"gsSPVertex\(&(\w+)\[(\d+)\], (\d+), (\d+)\)", line)
            if m:
                arr = vtx(m.group(1))
                s, n, d = int(m.group(2)), int(m.group(3)), int(m.group(4))
                for i in range(n):
                    buf[d + i] = arr[s + i]
            m = re.search(r"gsSP(2Triangles|1Triangle)\(([^)]*)\)", line)
            if m:
                a = [int(x) for x in m.group(2).split(",")]
                for k in (0, 4) if m.group(1) == "2Triangles" else (0,):
                    tris.append((load, buf[a[k]], buf[a[k + 1]], buf[a[k + 2]]))
    return tris


def hash01(ix, iz, salt):
    h = (ix * 73856093) ^ (iz * 19349663) ^ (salt * 83492791)
    h = (h ^ (h >> 13)) * 1274126177
    return ((h ^ (h >> 16)) & 0xFFFF) / 65535.0


def noise(x, z, scale, salt):
    """Smooth value noise, 0..1."""
    fx, fz = x / scale, z / scale
    ix, iz = math.floor(fx), math.floor(fz)
    tx, tz = fx - ix, fz - iz
    tx, tz = tx * tx * (3 - 2 * tx), tz * tz * (3 - 2 * tz)
    a = hash01(ix, iz, salt) * (1 - tx) + hash01(ix + 1, iz, salt) * tx
    b = hash01(ix, iz + 1, salt) * (1 - tx) + hash01(ix + 1, iz + 1, salt) * tx
    return a * (1 - tz) + b * tz


def colour(x, y, z, ny, plants):
    """(r, g, b) of the bed here, and how green it is (0 sand, 1 full weed-bed green)."""
    depth = SURFACE - y
    near = sum(w * math.exp(-((x - px) ** 2 + (z - pz) ** 2) / (2 * SIGMA ** 2)) for px, pz, w in plants
               if abs(x - px) < 3 * SIGMA and abs(z - pz) < 3 * SIGMA)
    dense = 1.0 - math.exp(-near / DENSE)
    patch = noise(x, z, 520.0, 1) * 0.5 + noise(x, z, 190.0, 2) * 0.3 + noise(x, z, 90.0, 4) * 0.2
    g = min(max(-0.3 + 0.8 * dense + 1.2 * (patch - 0.5), 0.0), 1.0)
    olive = noise(x, z, 340.0, 3)
    tint = [GREEN[i] * (1 - olive) + OLIVE[i] * olive for i in range(3)]
    deep = min(max((depth - DEEP0) / (DEEP1 - DEEP0), 0.0), 1.0)
    bright = (1.0 - 0.15 * dense) * (1.0 - 0.25 * deep) * (0.85 + 0.15 * max(ny, 0.0))
    fade = min(max(depth / FADE, 0.0), 1.0)
    out = []
    for i in range(3):
        c = (1.0 - g) + g * tint[i]
        c = 1.0 - (1.0 - c * bright) * fade
        out.append(max(0, min(255, round(255 * c))))
    return out, g * fade


def subdivide(tri, edge):
    """The triangle cut into n x n: (vertices keyed by (i, j), triangles as key triples)."""
    _, a, b, c = tri
    pts = [a, b, c]
    n = max(1, math.ceil(max(math.dist(pts[i][:3], pts[(i + 1) % 3][:3]) for i in range(3)) / edge))
    verts = {}
    for i in range(n + 1):
        for j in range(n + 1 - i):
            u, v = i / n, j / n
            w = 1 - u - v
            verts[(i, j)] = tuple(w * a[k] + u * b[k] + v * c[k] for k in range(5))
    tris = []
    for i in range(n):
        for j in range(n - i):
            tris.append(((i, j), (i + 1, j), (i, j + 1)))
            if j < n - i - 1:
                tris.append(((i + 1, j), (i + 1, j + 1), (i, j + 1)))
    return verts, tris


def write_empty():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as f:
        f.write("// Generated by fishmod/tools/gen_bedskin.py (the bed-skin test is off). Do not edit.\n")
        f.write('#include "fishmod_bedskin.h"\n\n')
        f.write("FishmodBedHeader gFishmodBed_spot06 = { FISHMOD_BED_MAGIC, 0, 0, 0, 0, 0, 0, { 0, 0, 0 } };\n")
    print(f"bed skin off -> {os.path.relpath(OUT, ROOT)}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--edge", type=float, default=180.0, help="longest edge after cutting")
    ap.add_argument("--png")
    ap.add_argument("--map", help="a top-down map of where the bed is green and where it's sand, with the shares")
    ap.add_argument("--spots", type=int, default=0,
                    help="write fishmod/scripts/look_bedskin.inc.c: this many views down onto the bed, on sand, on "
                         "weed-bed green and on the edges between (for the A/B capture, coloured bed on and off)")
    args = ap.parse_args()
    if os.environ.get("FISHMOD_BEDSKIN") != "1":
        write_empty()
        return
    import importlib.util

    spec = importlib.util.spec_from_file_location("gen_scatter",
                                                  os.path.join(ROOT, "fishmod", "tools", "gen_scatter.py"))
    gs = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gs)
    plants = [(p["x"], p["z"], PLANTS[p["kind"]] * p["scale"]) for p in gs.accents(gs.place("SCENE_LAKE_HYLIA"))
              if p["kind"] in PLANTS]

    under = [t for t in room_triangles() if t[0] is not None and min(v[1] for v in t[1:]) < SURFACE - 5]
    for t in under:
        if t[0][1] != "RGBA" or t[0][2] != "16b":
            raise SystemExit(f"bed texture {t[0][0]} isn't RGBA 16-bit")
    texs = sorted({t[0] for t in under})
    tex_index = {t: i for i, t in enumerate(texs)}

    # cut, colour, and file each small triangle under the chunk its middle is in
    vcache = {}
    vlist = []
    chunks = {}
    for ti, tri in enumerate(under):
        verts, tris = subdivide(tri, args.edge)
        p = [tri[k][:3] for k in (1, 2, 3)]
        e1 = [p[1][k] - p[0][k] for k in range(3)]
        e2 = [p[2][k] - p[0][k] for k in range(3)]
        nrm = [e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]]
        ln = math.sqrt(sum(c * c for c in nrm)) or 1.0
        ny = abs(nrm[1]) / ln
        for key, (x, y, z, s, t) in verts.items():
            vcache[(ti, key)] = len(vlist)
            rgb, green = colour(x, y, z, ny, plants)
            vlist.append((round(x), round(y) + LIFT, round(z), round(s), round(t), *rgb, green))
        for a, b, c in tris:
            ia, ib, ic = vcache[(ti, a)], vcache[(ti, b)], vcache[(ti, c)]
            cx = (vlist[ia][0] + vlist[ib][0] + vlist[ic][0]) / 3
            cz = (vlist[ia][2] + vlist[ib][2] + vlist[ic][2]) / 3
            chunks.setdefault((int(cx // CELL), int(cz // CELL)), []).append((tex_index[tri[0]], ia, ib, ic))

    # per chunk and texture, batches of up to 32 vertices
    out_v, out_t, batches, chunk_rows = [], [], [], []
    for key in sorted(chunks):
        first = len(batches)
        ys, xs, zs = [], [], []
        for tex in sorted({c[0] for c in chunks[key]}):
            pending = [c[1:] for c in chunks[key] if c[0] == tex]
            while pending:
                local = {}
                tris = []
                rest = []
                for tri in pending:
                    new = [v for v in tri if v not in local]
                    if len(local) + len(new) <= 32 and len(tris) < 255:
                        for v in new:
                            local[v] = len(local)
                        tris.append(tuple(local[v] for v in tri))
                    else:
                        rest.append(tri)
                pending = rest
                vstart = len(out_v)
                for v, _ in sorted(local.items(), key=lambda kv: kv[1]):
                    out_v.append(vlist[v])
                    xs.append(vlist[v][0])
                    ys.append(vlist[v][1])
                    zs.append(vlist[v][2])
                batches.append((vstart, len(out_t), len(local), len(tris), tex))
                out_t += tris
        cx, cz = (min(xs) + max(xs)) / 2, (min(zs) + max(zs)) / 2
        r = max(math.hypot(x - cx, z - cz) for x, z in zip(xs, zs))
        chunk_rows.append((round(cx), round(cz), math.ceil(r), first, len(batches) - first, min(ys), max(ys)))
    if len(out_v) > 0xFFFF or len(out_t) > 0xFFFF:
        raise SystemExit(f"too big for 16-bit indices: {len(out_v)} vertices, {len(out_t)} triangles")

    tri_bytes = [b for t in out_t for b in t]
    tri_bytes += [0] * (-len(tri_bytes) % 8)
    with open(OUT, "w") as f:
        f.write("// Generated by fishmod/tools/gen_bedskin.py from Lake Hylia's room (the bed-skin test). "
                "Do not edit.\n")
        f.write('#include "fishmod_bedskin.h"\n#include "gfx.h"\n\n')
        f.write("typedef struct {\n    FishmodBedHeader header;\n")
        f.write(f"    FishmodBedTex tex[{len(texs)}];\n    FishmodBedChunk chunks[{len(chunk_rows)}];\n")
        f.write(f"    FishmodBedBatch batches[{len(batches)}];\n    Vtx vtx[{len(out_v)}];\n")
        f.write(f"    u8 tris[{len(tri_bytes)}];\n}} FishmodBed_spot06;\n\n")
        f.write("FishmodBed_spot06 gFishmodBed_spot06 = {\n")
        f.write(f"    {{ FISHMOD_BED_MAGIC, {len(texs)}, {len(chunk_rows)}, {len(batches)}, 0, {len(out_v)}, "
                f"{len(out_t)}, {{ 0, 0, 0 }} }},\n    {{\n")
        for name, _, _, cms, cmt, masks, maskt, shifts, shiftt in texs:
            w, h = tex_size(name)
            off = int(re.search(r"_([0-9A-F]{8})_Tex", name).group(1), 16)
            f.write(f"        {{ 0x{0x02000000 | off:08X}, {w}, {h}, {cms}, {cmt}, {masks}, {maskt}, {shifts}, "
                    f"{shiftt}, 0 }},\n")
        f.write("    },\n    {\n")
        for row in chunk_rows:
            f.write("        { %d, %d, %d, %d, %d, %d, %d, 0 },\n" % row)
        f.write("    },\n    {\n")
        for row in batches:
            f.write("        { %d, %d, %d, %d, %d, 0 },\n" % row)
        f.write("    },\n    {\n")
        for x, y, z, s, t, r, g, b, _ in out_v:
            f.write(f"        VTX({x}, {y}, {z}, {s}, {t}, {r}, {g}, {b}, 255),\n")
        f.write("    },\n    {\n")
        for i in range(0, len(tri_bytes), 24):
            f.write("        " + ", ".join(str(b) for b in tri_bytes[i:i + 24]) + ",\n")
        f.write("    },\n};\n")
    size = 32 + 16 * len(texs) + 16 * len(chunk_rows) + 8 * len(batches) + 16 * len(out_v) + len(tri_bytes)
    print(f"bed skin: {len(under)} bed triangles -> {len(out_t)} ({len(out_v)} vertices) in {len(chunk_rows)} "
          f"chunks, {len(batches)} batches, {size / 1024:.1f} KB -> {os.path.relpath(OUT, ROOT)}")
    if args.png:
        from PIL import Image, ImageDraw

        img = Image.new("RGB", (1400, 1280), (20, 30, 40))
        d = ImageDraw.Draw(img)
        for tris in chunks.values():
            for _, ia, ib, ic in tris:
                vs = [vlist[ia], vlist[ib], vlist[ic]]
                col = tuple(sum(v[5 + k] for v in vs) // 3 for k in range(3))
                d.polygon([((v[0] + 4800) / 5, (v[2] - 3300) / 5) for v in vs], fill=col)
        img.save(args.png)

    if args.map:
        bed_map(chunks, vlist, args.map)
    if args.spots:
        write_spots(vlist, args.spots)


def bed_map(chunks, vlist, path):
    """Top-down: sand light, green as drawn; the share of the bed's area that is sand, part green, green."""
    from PIL import Image, ImageDraw

    img = Image.new("RGB", (1400, 1300), (20, 30, 40))
    d = ImageDraw.Draw(img)
    share = [0.0, 0.0, 0.0]
    for tris in chunks.values():
        for _, ia, ib, ic in tris:
            vs = [vlist[ia], vlist[ib], vlist[ic]]
            g = sum(v[8] for v in vs) / 3
            a = abs((vs[1][0] - vs[0][0]) * (vs[2][2] - vs[0][2]) - (vs[2][0] - vs[0][0]) * (vs[1][2] - vs[0][2])) / 2
            share[0 if g < 0.2 else 1 if g < 0.5 else 2] += a
            sand, green = (222, 206, 160), (70, 120, 50)
            col = tuple(round(sand[k] * (1 - g) + green[k] * g) for k in range(3))
            d.polygon([((v[0] + 4800) / 5, (v[2] - 3300) / 5) for v in vs], fill=col)
    total = sum(share) or 1.0
    text = (f"sand {100 * share[0] / total:.0f}%   part green {100 * share[1] / total:.0f}%   "
            f"green {100 * share[2] / total:.0f}%   (of the bed under the lake)")
    d.text((20, 1270), text, fill=(255, 255, 255))
    img.save(path)
    print(text)


def write_spots(vlist, count):
    """Views from a diver's height down onto the bed: a third on sand, a third on green, a third on the edges between
    (where the green varies most nearby), each well apart."""
    import random

    pts = [(v[0], v[1], v[2], v[8]) for v in vlist if SURFACE - v[1] > 260]
    grid = {}
    for p in pts:
        grid.setdefault((p[0] // 300, p[2] // 300), []).append(p)

    def around(p):
        out = []
        for gx in range(p[0] // 300 - 1, p[0] // 300 + 2):
            for gz in range(p[2] // 300 - 1, p[2] // 300 + 2):
                out += [q for q in grid.get((gx, gz), []) if (q[0] - p[0]) ** 2 + (q[2] - p[2]) ** 2 < 250 ** 2]
        return out

    rng = random.Random(5)
    cands = rng.sample(pts, min(len(pts), 1500))
    scored = []
    for p in cands:
        near = around(p)
        gs = [q[3] for q in near]
        mean = sum(gs) / len(gs)
        spread = (sum((g - mean) ** 2 for g in gs) / len(gs)) ** 0.5
        scored.append((p, mean, spread))
    picks = []

    def take(key, n, label):
        for p, mean, spread in sorted(scored, key=key):
            if sum(1 for q in picks if q[1] == label) >= n:
                break
            if all((p[0] - q[0][0]) ** 2 + (p[2] - q[0][2]) ** 2 > 900 ** 2 for q in picks):
                picks.append((p, label))

    n = max(1, count // 3)
    take(lambda r: r[1], n, "sand")
    take(lambda r: -r[1], n, "green")
    take(lambda r: -r[2], count - 2 * n, "edge")
    out = os.path.join(ROOT, "fishmod", "scripts", "look_bedskin.inc.c")
    with open(out, "w") as f:
        f.write("// The bed-skin test, looked at: views from a diver's height down onto the bed (on sand, on\n"
                "// weed-bed green and on the edges between; picked by fishmod/tools/gen_bedskin.py --spots),\n"
                "// each frozen and shown with the coloured bed and without it.\n"
                "// Build: fishmod/build.sh --script look_bedskin --opt 5 --hw --bedskin; capture:\n"
                "// fishmod/tools/edge_ab_capture.py ROM OUT --stops N (\"new\" has it, \"old\" is the plain bed).\n")
        f.write("#define FISHMOD_EDGE_AB 1\n#define FISHMOD_TOUR_AUTO 400\n")
        f.write("#define FISHMOD_EDGE_AB_FLAGS FISHMOD_BENCH_NOBEDSKIN\n")
        f.write("#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0\n#define FISHMOD_SCRIPT_DAY 1        // a clear day\n")
        f.write("#define FISHMOD_SCRIPT_TIME 0x8000  // midday\n#define FISHMOD_TOUR\n")
        f.write("static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };\n")
        f.write("static s16 sScriptStartYaw = (s16)0x8000;\n")
        f.write("static FishmodAutoplayStep sScriptSteps[] = {\n    { 1, 0, 0, 0, true },\n};\n\n")
        f.write("static FishmodTourStop sTourStops[] = {\n")
        for i, (p, label) in enumerate(picks):
            ang = rng.uniform(0, 2 * math.pi)
            ex, ez = p[0] - math.cos(ang) * 230, p[2] - math.sin(ang) * 230
            ey = min(p[1] + 150, SURFACE - 25)
            f.write(f'    {{ {{ {round(ex)}, {round(ey)}, {round(ez)} }}, {{ {p[0]}, {p[1]}, {p[2]} }}, '
                    f'"{i + 1}. {label}", "", "", "" }},\n')
        f.write("};\n")
    print(f"{len(picks)} views -> {os.path.relpath(out, ROOT)}: " + ", ".join(lb for _, lb in picks))


if __name__ == "__main__":
    main()
