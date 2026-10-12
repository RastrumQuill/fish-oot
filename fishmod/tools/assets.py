#!/usr/bin/env python3
"""
Turns the mod's source art into C the build compiles, from one manifest: fishmod/assets/assets.json.

Textures (PNG) in every N64 texture format:
  rgba32, rgba16, ia16, ia8, ia4, i8, i4   direct colour / intensity
  ci8, ci4                                 palette textures: the palette (RGBA16) is written as <name>TLUT.
                                           Colours are quantized to 256 / 16 if the image has more.
Models (Wavefront OBJ) as a vertex list and a display list (F3DEX2-compatible: loads of up to 32 vertices).

The manifest:
{
  "textures": [
    { "name": "gFishmodBottleLarva3Tex", "src": "fishmod/art/bottle_larva_3.png", "format": "rgba32",
      "out": "src/code/fishmod_bait_icons.inc.c" }
  ],
  "models": [
    { "name": "gFishmodRockDL", "src": "fishmod/assets/rock.obj", "out": "src/.../rock.inc.c",
      "scale": 10.0,                       // OBJ units -> world units (default 1)
      "texture": "gFishmodRockTex",        // optional: a texture from "textures" to load and map with the OBJ's
                                           // UVs (its size and format come from that entry)
      "color": [255, 255, 255],            // vertex colour when the OBJ has none (default white)
      "lighting": false,                   // true: write normals instead of colours (for lit materials)
      "separateMaterial": false }          // true: the texture load in its own <name>Material display list
  ]
}
Paths are from the repo root. Entries with the same "out" go into the same file, in manifest order. A file is only
rewritten when its contents change, so make only rebuilds what changed. Textures are checked against TMEM (4 KB,
2 KB for palette textures) and other N64 rules; problems are errors, not silent.

Textures are declared as u64 arrays (the RDP needs 8-byte alignment). The generated .inc.c files go wherever they
are #included; declare the symbols you use from other files with `extern u64 name[];`.

Usage: fishmod/tools/assets.py            regenerate everything that changed
       fishmod/tools/assets.py --check    only report problems / out-of-date files (exit 1 if any)
       fishmod/tools/assets.py --manifest other.json   use another manifest (for experiments)
Needs Pillow.
"""

import json
import os
import sys

from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
MANIFEST = os.path.join(ROOT, "fishmod", "assets", "assets.json")

# bits per texel, and the gDP texel format/size names
FORMATS = {
    "rgba32": (32, "G_IM_FMT_RGBA", "G_IM_SIZ_32b"),
    "rgba16": (16, "G_IM_FMT_RGBA", "G_IM_SIZ_16b"),
    "ia16": (16, "G_IM_FMT_IA", "G_IM_SIZ_16b"),
    "ia8": (8, "G_IM_FMT_IA", "G_IM_SIZ_8b"),
    "ia4": (4, "G_IM_FMT_IA", "G_IM_SIZ_4b"),
    "i8": (8, "G_IM_FMT_I", "G_IM_SIZ_8b"),
    "i4": (4, "G_IM_FMT_I", "G_IM_SIZ_4b"),
    "ci8": (8, "G_IM_FMT_CI", "G_IM_SIZ_8b"),
    "ci4": (4, "G_IM_FMT_CI", "G_IM_SIZ_4b"),
}


class AssetError(Exception):
    pass


def rel(path):
    return os.path.join(ROOT, path)


def rgba5551(r, g, b, a):
    return ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | (1 if a >= 128 else 0)


def gray(r, g, b):
    return (r * 299 + g * 587 + b * 114) // 1000


def pixels(img):
    return list(img.get_flattened_data()) if hasattr(img, "get_flattened_data") else list(img.getdata())


def pack(values, bits):
    """Packs texel values (each `bits` wide) big-endian into bytes."""
    out = bytearray()
    if bits >= 8:
        n = bits // 8
        for v in values:
            out += v.to_bytes(n, "big")
    else:
        per = 8 // bits
        for i in range(0, len(values), per):
            byte = 0
            for v in values[i:i + per]:
                byte = (byte << bits) | v
            out.append(byte)
    return bytes(out)


def palette_of(img, colors):
    """Indices and an RGBA palette (at most `colors` entries) for an RGBA image, quantizing if needed."""
    px = pixels(img)
    distinct = list(dict.fromkeys(px))
    if len(distinct) <= colors:
        index = {c: i for i, c in enumerate(distinct)}
        return [index[c] for c in px], distinct
    # more colours than fit: quantize (transparent texels stay transparent)
    q = img.quantize(colors=colors, method=Image.Quantize.FASTOCTREE)
    pal = q.getpalette("RGBA")[:colors * 4]
    palette = [tuple(pal[i:i + 4]) for i in range(0, len(pal), 4)]
    return pixels(q), palette


def convert_texture(entry):
    """Bytes of the texture (and of its palette, or None) in the entry's format."""
    fmt = entry["format"]
    if fmt not in FORMATS:
        raise AssetError(f"{entry['name']}: unknown format {fmt!r} (one of {', '.join(FORMATS)})")
    bits = FORMATS[fmt][0]
    img = Image.open(rel(entry["src"])).convert("RGBA")
    w, h = img.size
    if w * bits % 64:
        raise AssetError(f"{entry['name']}: a row of {w} texels at {bits} bits isn't a multiple of 64 bits")
    for side in (w, h):
        if side & (side - 1):
            # allowed by the RDP, but only without wrapping/mirroring; usually a mistake
            print(f"warning: {entry['name']}: {w}x{h} is not a power of two (no wrap or mirror on that axis)")
    tmem = 2048 if fmt.startswith("ci") else 4096
    if w * h * bits // 8 > tmem:
        raise AssetError(f"{entry['name']}: {w}x{h} {fmt} is {w * h * bits // 8} bytes, more than TMEM ({tmem})")

    px = pixels(img)
    tlut = None
    if fmt == "rgba32":
        data = bytes(b for p in px for b in p)
    elif fmt == "rgba16":
        data = pack([rgba5551(*p) for p in px], 16)
    elif fmt == "ia16":
        data = pack([(gray(*p[:3]) << 8) | p[3] for p in px], 16)
    elif fmt == "ia8":
        data = pack([((gray(*p[:3]) >> 4) << 4) | (p[3] >> 4) for p in px], 8)
    elif fmt == "ia4":
        data = pack([((gray(*p[:3]) >> 5) << 1) | (1 if p[3] >= 128 else 0) for p in px], 4)
    elif fmt == "i8":
        data = pack([gray(*p[:3]) for p in px], 8)
    elif fmt == "i4":
        data = pack([gray(*p[:3]) >> 4 for p in px], 4)
    else:
        colors = 256 if fmt == "ci8" else 16
        indices, palette = palette_of(img, colors)
        data = pack(indices, bits)
        tlut = pack([rgba5551(*c) for c in palette], 16)
    return data, tlut, (w, h)


def u64_array(name, data, comment):
    data = data + bytes(-len(data) % 8)
    words = ["0x" + data[i:i + 8].hex().upper() for i in range(0, len(data), 8)]
    lines = [f"// {comment}", f"u64 {name}[{len(words)}] = {{"]
    for i in range(0, len(words), 4):
        lines.append("    " + ", ".join(words[i:i + 4]) + ",")
    lines.append("};")
    return "\n".join(lines)


def load_obj(path):
    """Positions, UVs, normals, colours and triangles (each corner a (v, vt, vn) index triple) of an OBJ file."""
    pos, uv, nrm, col, tris = [], [], [], [], []
    with open(path) as f:
        for line in f:
            p = line.split()
            if not p:
                continue
            if p[0] == "v":
                pos.append(tuple(float(x) for x in p[1:4]))
                # the common "v x y z r g b" extension for vertex colours (0..1)
                col.append(tuple(round(float(x) * 255) for x in p[4:7]) if len(p) >= 7 else None)
            elif p[0] == "vt":
                uv.append((float(p[1]), float(p[2]) if len(p) > 2 else 0.0))
            elif p[0] == "vn":
                nrm.append(tuple(float(x) for x in p[1:4]))
            elif p[0] == "f":
                corners = []
                for c in p[1:]:
                    idx = (c.split("/") + ["", ""])[:3]
                    corners.append(tuple(int(i) - 1 if i else -1 for i in idx))
                for i in range(1, len(corners) - 1):  # fan-triangulate polygons
                    tris.append((corners[0], corners[i], corners[i + 1]))
    return pos, uv, nrm, col, tris


def convert_model(entry, textures):
    name = entry["name"]
    pos, uv, nrm, col, tris = load_obj(rel(entry["src"]))
    if not tris:
        raise AssetError(f"{name}: no faces in {entry['src']}")
    scale = entry.get("scale", 1.0)
    lighting = entry.get("lighting", False)
    default_color = tuple(entry.get("color", [255, 255, 255]))
    tex = None
    if "texture" in entry:
        tex = textures.get(entry["texture"])
        if tex is None:
            raise AssetError(f"{name}: texture {entry['texture']!r} is not in the manifest's textures")
    tw, th = tex["size"] if tex else (32, 32)

    # unique vertices (position + uv + normal/colour combination)
    verts, vindex, faces = [], {}, []
    for tri in tris:
        face = []
        for v, t, n in tri:
            x, y, z = (round(c * scale) for c in pos[v])
            if max(abs(x), abs(y), abs(z)) > 32767:
                raise AssetError(f"{name}: a vertex is outside the s16 range after scaling")
            s, tt = (uv[t] if t >= 0 else (0.0, 0.0))
            s, tt = round(s * tw * 32), round((1.0 - tt) * th * 32)  # s10.5, OBJ's v points up
            if lighting:
                nx, ny, nz = nrm[n] if n >= 0 else (0.0, 1.0, 0.0)
                c = tuple(max(-128, min(127, round(k * 127))) & 0xFF for k in (nx, ny, nz))
            else:
                c = col[v] or default_color
            key = (x, y, z, s, tt) + c
            if key not in vindex:
                vindex[key] = len(verts)
                verts.append(key)
            face.append(vindex[key])
        faces.append(face)

    # split into loads of up to 32 vertices (the F3DEX2 vertex cache)
    batches = []  # (list of global vertex indices, faces in local indices)
    cur_verts, cur_map, cur_faces = [], {}, []
    for face in faces:
        new = [v for v in dict.fromkeys(face) if v not in cur_map]
        if len(cur_verts) + len(new) > 32:
            batches.append((cur_verts, cur_faces))
            cur_verts, cur_map, cur_faces = [], {}, []
            new = list(dict.fromkeys(face))
        for v in new:
            cur_map[v] = len(cur_verts)
            cur_verts.append(v)
        cur_faces.append([cur_map[v] for v in face])
    batches.append((cur_verts, cur_faces))

    vtx_lines, dl = [], []
    order = []
    for bverts, _ in batches:
        order.extend(bverts)  # vertices are written in load order; shared ones are repeated per load
    vtx_lines.append(f"Vtx {name}Vtx[{len(order)}] = {{")
    for v in order:
        x, y, z, s, t, r, g, b = verts[v]
        vtx_lines.append(f"    VTX({x}, {y}, {z}, {s}, {t}, 0x{r:02X}, 0x{g:02X}, 0x{b:02X}, 0xFF),")
    vtx_lines.append("};")

    # "separateMaterial": the texture load goes in its own <name>Material display list, drawn once before many
    # copies of the model (props scattered by the hundred); the model itself is then geometry only
    separate = entry.get("separateMaterial", False)
    mat = []
    if separate:
        dl.append(f"Gfx {name}Material[] = {{")
    else:
        dl.append(f"Gfx {name}[] = {{")
    dl.append("    gsDPPipeSync(),")
    if tex:
        bits, gfmt, gsiz = FORMATS[tex["format"]]
        load = "gsDPLoadTextureBlock_4b" if bits == 4 else "gsDPLoadTextureBlock"
        siz = "" if bits == 4 else f"{gsiz}, "
        if tex["format"].startswith("ci"):
            dl.append(f"    gsDPSetTextureLUT(G_TT_RGBA16),")
            dl.append(f"    gsDPLoadTLUT_pal{'256' if bits == 8 else '16'}"
                      f"({'' if bits == 8 else '0, '}{tex['name']}TLUT),")
        dl.append(f"    {load}({tex['name']}, {gfmt}, {siz}{tw}, {th}, 0, G_TX_WRAP, G_TX_WRAP, "
                  f"{(tw - 1).bit_length()}, {(th - 1).bit_length()}, G_TX_NOLOD, G_TX_NOLOD),")
        dl.append("    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),")
    if separate:
        dl.append("    gsSPEndDisplayList(),")
        dl.append("};")
        dl.append("")
        dl.append(f"Gfx {name}[] = {{")
    start = 0
    for bverts, bfaces in batches:
        dl.append(f"    gsSPVertex(&{name}Vtx[{start}], {len(bverts)}, 0),")
        for i in range(0, len(bfaces), 2):
            if i + 1 < len(bfaces):
                a, b = bfaces[i], bfaces[i + 1]
                dl.append(f"    gsSP2Triangles({a[0]}, {a[1]}, {a[2]}, 0, {b[0]}, {b[1]}, {b[2]}, 0),")
            else:
                a = bfaces[i]
                dl.append(f"    gsSP1Triangle({a[0]}, {a[1]}, {a[2]}, 0),")
        start += len(bverts)
    if tex and not separate:
        dl.append("    gsSPTexture(0, 0, 0, G_TX_RENDERTILE, G_OFF),")
    dl.append("    gsSPEndDisplayList(),")
    dl.append("};")
    comment = (f"// {entry['src']}: {len(faces)} triangles, {len(order)} vertices in {len(batches)} loads"
               f"{', lit' if lighting else ''}")
    return comment + "\n" + "\n".join(vtx_lines) + "\n\n" + "\n".join(dl)


def build(manifest, check):
    outputs = {}  # out path -> list of C chunks
    textures = {}
    problems = 0

    for entry in manifest.get("textures", []):
        try:
            data, tlut, size = convert_texture(entry)
        except (AssetError, OSError) as e:
            print(f"error: {e}")
            problems += 1
            continue
        textures[entry["name"]] = dict(entry, size=size)
        w, h = size
        chunks = outputs.setdefault(entry["out"], [])
        if tlut is not None:
            chunks.append(u64_array(entry["name"] + "TLUT", tlut, f"palette of {entry['name']}: "
                                    f"{len(tlut) // 2} colours, RGBA16"))
        chunks.append(u64_array(entry["name"], data, f"{entry['src']}: {w}x{h} {entry['format']}"))

    for entry in manifest.get("models", []):
        try:
            outputs.setdefault(entry["out"], []).append(convert_model(entry, textures))
        except (AssetError, OSError) as e:
            print(f"error: {e}")
            problems += 1

    for out, chunks in outputs.items():
        text = ("// Generated by fishmod/tools/assets.py from fishmod/assets/assets.json. Edit the sources, not this "
                "file.\n\n" + "\n\n".join(chunks) + "\n")
        path = rel(out)
        old = open(path).read() if os.path.exists(path) else None
        if old == text:
            continue
        if check:
            print(f"out of date: {out}")
            problems += 1
        else:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w") as f:
                f.write(text)
            print(f"wrote {out}")
    return problems


def main():
    args = sys.argv[1:]
    check = "--check" in args
    manifest = MANIFEST
    if "--manifest" in args:
        manifest = args[args.index("--manifest") + 1]
    with open(manifest) as f:
        manifest = json.load(f)
    sys.exit(1 if build(manifest, check) else 0)


if __name__ == "__main__":
    main()
