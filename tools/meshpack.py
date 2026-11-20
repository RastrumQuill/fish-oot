#!/usr/bin/env python3
"""fish-oot: a mesh pass over the extracted scenes (build.sh --meshpack): the RSP's culling made finer, the same
picture.

A room's mesh is drawn in pieces (RoomShapeCullable entries), each culled whole by its bounding sphere on the CPU and
by its bounding box on the RSP (gsSPCullDisplayList, its first eight vertices). The lake's widest pieces span most of
the lake (a sphere radius of 3,400 to 5,500 units), so they're drawn wherever the camera looks: every vertex
transformed and lit, the triangles off the screen thrown away only after. Here each listed piece's display list is cut
into areas on a grid (by each triangle's centre), each area a display list of its own with its own bounding box and
cull command first, called in turn from the piece's list. An area out of view now costs its eight box corners.

Exact: every triangle is drawn once, with the same three vertices (the same bytes), after the same state commands in
the same order as before (an area's list replays every state command of the original that comes before its
triangles, in order, so whatever a triangle saw, it still sees). Only the order the areas' triangles are drawn in
changes, and these pieces are opaque, z-buffered and without decals (a piece with a decal or translucent render mode,
or a command not known here, is left as it is). Identical vertices within a load are loaded once.
The tool checks all of that before it writes (every triangle, its vertices and its state count) and stops otherwise.

The original tree isn't touched: the output is a copy (hard links for every file the pass doesn't change), which the
build then reads (EXTRACTED_DIR), like tools/colpack.py.

Usage: tools/meshpack.py SRC_TREE OUT_TREE [--report]
"""
import collections
import os
import random
import re
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import meshpack_merge  # noqa: E402

# the pieces to cut: (scene directory, room file stem, display lists); the area size
TARGETS = [
    ("assets/scenes/overworld/spot06", "spot06_room_0",
     ["gSpot06DL_00A400_03003270_DL", "gSpot06DL_00A400_03001240_DL", "gSpot06DL_00A400_03003EF8_DL",
      "gSpot06DL_00A400_03004AF0_DL",
      # (the pieces with triangles under the water, the pit by the lab's among them: cut into areas too)
      "gSpot06DL_00A400_03008858_DL", "gSpot06DL_00A400_03008ED0_DL", "gSpot06DL_00A400_030059C0_DL",
      "gSpot06DL_00A400_030098C8_DL"]),
]
# the lake bed's caustics (combiners that blend the caustic texture into the sand by the environment alpha) faded out
# with depth: a triangle deep under the surface drawn sand only (the caustics come from the sun through the surface;
# deep down, where the surface can't be seen, they don't belong). By each triangle's centre depth below WATER_Y, in
# CAUSTIC_STEPS steps from CAUSTIC_FROM (as before) to CAUSTIC_TO (sand only, 255, as the scene draws its drained lake):
# the steps between call segment 0x0A's lists, an environment colour each (Scene_DrawConfigLakeHylia makes them each
# frame from the scene's own colour: the drained lake's is 255, so every step is too)
# (the lists drawing the caustic texture, 000170B8. The shore's lists blend two grass textures the same way, 00018CB8
# into 000184B8: not caustics, left alone)
CAUSTICS = [("assets/scenes/overworld/spot06", "spot06_room_0",
             ["gSpot06DL_00A400_03003270_DL", "gSpot06DL_00A400_03008858_DL", "gSpot06DL_00A400_03008ED0_DL"])]
WATER_Y = -1313
CAUSTIC_FROM, CAUSTIC_TO = 150, 550   # depths (units) where the fade starts and where it's sand only
CAUSTIC_STEPS = 4                      # steps of the fade (the lists in segment 0x0A: CAUSTIC_STEPS - 1 of them)
# MESHPACK_NO_CAUSTIC_FADE=1: no fade (vanilla's one blend everywhere). The grey caustics below stay either way.
CAUSTIC_FADE = not os.environ.get("MESHPACK_NO_CAUSTIC_FADE")
# the steps' edges (where a depth's step changes: between f = 1/8, 3/8, 5/8, 7/8 of the way): the triangles cut along
# them (as the fog's bands), so a step changes along a depth line, not a triangle's edge (with MESHPACK_DEPTH_MERGE)
CAUSTIC_EDGES = [CAUSTIC_FROM + (CAUSTIC_TO - CAUSTIC_FROM) * (2 * i + 1) / (2.0 * CAUSTIC_STEPS)
                 for i in range(CAUSTIC_STEPS)]
CAUSTIC_CUT = bool(os.environ.get("MESHPACK_DEPTH_MERGE"))
# ... and the caustic texture itself made light alone: their brightness (i8), no colour (the water's colour is
# the fog's; the rocks' caustics are the same light: fishmod_scatter.c). Half the size to load.
GREY_CAUSTICS = [("assets/scenes/overworld/spot06", "spot06_scene", ["spot06_scene_000170B8_Tex"])]
if os.environ.get("MESHPACK_NO_GREY_CAUSTICS"):
    GREY_CAUSTICS = []
# Water fog (fishmod.c Fishmod_WaterFogBands): the camera-under-water fog seen from above, done by the RSP's own fog.
# The cut pieces' triangles under the surface are split at these depths into bands (the first: the surface itself;
# the parts above it stay as they were), each band's lists called after a list of segment 0x0A's (after the caustic
# steps) that sets the fog for that band: the fog over the share of the view's ray inside the water at the band's
# depth, as the RSP's own distance fog with its two numbers scaled (and the water's colour); then a list setting the
# scene's fog back. Scene_DrawConfigLakeHylia makes the lists each frame (empty with the camera under water, the lake
# drained, or the bench's NOWATERFOG).
WATER_FOG = not os.environ.get("MESHPACK_NO_WATER_FOG")
WATER_BANDS = [0, 12, 30, 60, 100, 160, 250, 400]   # each band's top, by depth under the surface
# (the game's band lists: these 8, by their middles' depths, fishmod.c sWaterBandMid)
GAME_BAND_MID = [6, 21, 45, 80, 130, 205, 325, 520]
# tests: MESHPACK_FOG_BANDS="0,30,100,250" cuts at fewer depths (each band's fog the game's list nearest its middle);
# uncut (the default): each whole triangle in the band of its middle's depth
if os.environ.get("MESHPACK_FOG_BANDS"):
    WATER_BANDS = [int(x) for x in os.environ["MESHPACK_FOG_BANDS"].split(",")]
# (the default since rc1: the cuts made long thin slivers, each scanline of each a triangle setup for the RDP: the
# console's frames28 set, 8 cut bands 54% hitches and 17.2 fps, uncut 32% and 18.2 fps, no bed fog at all 30-31%.
# MESHPACK_FOG_CUT=1 cuts again)
# MESHPACK_DEPTH_MERGE=1 (a test, 20 Nov): the bed cut along every depth line (the fog's bands and the caustics'
# steps) and the pieces merged back where the surface allows (tools/meshpack_merge.py). It merges little: the bed
# isn't flat and its texture coordinates don't run on from one triangle to the next (2,475 triangles drawn for the
# bed's main piece against 649). The default stays uncut (each whole triangle by its middle, as rc1);
# MESHPACK_FOG_CUT=1: cut, not merged (frames28's).
DEPTH_MERGE = bool(os.environ.get("MESHPACK_DEPTH_MERGE"))
FOG_NOCUT = not (os.environ.get("MESHPACK_FOG_CUT") or DEPTH_MERGE)
# ... but a triangle crossing the surface is cut at it (MESHPACK_NO_SURFACE_CUT=1: not): a few, along the shore
FOG_CUT_SURFACE = not os.environ.get("MESHPACK_NO_SURFACE_CUT")
SURFACE_CUTS = [0]
MERGED = [0]  # (triangles the merge took away)


def game_band(k):
    """the game's band list for this band (by the depth of its middle)"""
    top = WATER_BANDS[k]
    bottom = WATER_BANDS[k + 1] if k + 1 < len(WATER_BANDS) else top + 250
    mid = (top + bottom) / 2.0
    return min(range(len(GAME_BAND_MID)), key=lambda i: abs(GAME_BAND_MID[i] - mid))
FOG_SEG_BASE = 0x0A000000 + 3 * 16                    # (after the three caustic steps, two commands each)


def fog_band_call(k):
    return "gsSPDisplayList(0x%08X)" % (FOG_SEG_BASE + game_band(k) * 32)


FOG_OFF_CALL = "gsSPDisplayList(0x%08X)" % (FOG_SEG_BASE + len(GAME_BAND_MID) * 32)
FOG_CALLS = {fog_band_call(k) for k in range(len(WATER_BANDS))} | {FOG_OFF_CALL}


def _sb(v):
    v = int(v, 0) & 0xFF
    return v - 256 if v > 127 else v


def lerp_vtx(a, b, f, lit):
    """the vertex f of the way from a to b (a and b in a fixed order, so an edge two triangles share is cut at the
    same point in both: no crack); a lit vertex's normal interpolated and kept its length, else its colour"""
    if a > b:
        a, b, f = b, a, 1.0 - f
    ia = [int(x, 0) for x in a]
    ib = [int(x, 0) for x in b]
    out = [round(ia[i] + (ib[i] - ia[i]) * f) for i in range(5)]
    if lit:
        na = [_sb(x) for x in a[5:8]]
        nb = [_sb(x) for x in b[5:8]]
        n = [na[i] + (nb[i] - na[i]) * f for i in range(3)]
        ln = max(1e-6, sum(x * x for x in n) ** 0.5)
        want = max(sum(x * x for x in na) ** 0.5, sum(x * x for x in nb) ** 0.5)
        out += [round(x * want / ln) & 0xFF for x in n]
    else:
        out += [round(ia[i] + (ib[i] - ia[i]) * f) for i in range(5, 8)]
    out.append(round(ia[8] + (ib[8] - ia[8]) * f))
    v = tuple(str(x) for x in out)
    ORIGIN.setdefault(v, ORIGIN.get(a if f < 0.5 else b))
    return v


def clip_poly(poly, y0, lit):
    """the polygon's parts at or over height y0 and under it"""
    above, below = [], []
    n = len(poly)
    for i in range(n):
        p, q = poly[i], poly[(i + 1) % n]
        yp, yq = int(p[1], 0), int(q[1], 0)
        (above if yp >= y0 else below).append(p)
        if (yp >= y0) != (yq >= y0):
            m = lerp_vtx(p, q, (y0 - yp) / float(yq - yp), lit)
            above.append(m)
            below.append(m)
    return above, below


def band_split(tri, lit):
    """the triangle's parts by band: [(band (-1: over the surface), triangle)]"""
    if FOG_NOCUT:
        def band_of(t):
            d = WATER_Y - sum(int(v[1], 0) for v in t) / 3.0
            b = -1
            for k, top in enumerate(WATER_BANDS):
                if d > top:
                    b = k
            return b

        ys = [int(v[1], 0) for v in tri]
        if FOG_CUT_SURFACE and min(ys) < WATER_Y < max(ys):
            # crossing the surface: cut there only (else the whole triangle takes one side's fog, and the shore shows
            # a straight edge where it changes), the part over it unfogged, the part under it by its own middle
            above, below = clip_poly(list(tri), WATER_Y, lit)
            out = [(-1, (above[0], above[i], above[i + 1])) for i in range(1, len(above) - 1)]
            for i in range(1, len(below) - 1):
                t = (below[0], below[i], below[i + 1])
                out.append((max(band_of(t), 0), t))
            SURFACE_CUTS[0] += 1
            return out
        return [(band_of(tri), tuple(tri))]
    out = []
    poly = list(tri)
    for k, d in enumerate(WATER_BANDS):
        above, below = clip_poly(poly, WATER_Y - d, lit)
        if len(above) >= 3:
            out += [(k - 1, (above[0], above[i], above[i + 1])) for i in range(1, len(above) - 1)]
        poly = below
        if len(poly) < 3:
            return out
    out += [(len(WATER_BANDS) - 1, (poly[0], poly[i], poly[i + 1])) for i in range(1, len(poly) - 1)]
    return out


def tri_area(t):
    p = [[int(v[i], 0) for i in range(3)] for v in t]
    u = [p[1][i] - p[0][i] for i in range(3)]
    w = [p[2][i] - p[0][i] for i in range(3)]
    c = (u[1] * w[2] - u[2] * w[1], u[2] * w[0] - u[0] * w[2], u[0] * w[1] - u[1] * w[0])
    return 0.5 * sum(x * x for x in c) ** 0.5


RELIGHT = {"spot06_room_0"}  # rooms whose lit pieces read their vertices lit by the CPU (segment RELIT_SEG)
if os.environ.get("MESHPACK_NO_RELIGHT"):  # (a test: the areas without the CPU's lighting)
    RELIGHT = set()
RELIT_SEG = 0x0C
CELL = 1500        # area size (units, x and z)
# MESHPACK_BY_AREA: the first cut (each area its own list, replaying the piece's state before its triangles: frames14)
BY_AREA = bool(os.environ.get("MESHPACK_BY_AREA"))
MIN_TRIS = 12      # areas with fewer triangles are joined to their nearest neighbour
VTX_SLOTS = 32     # vertices loaded at a time (the original lists' own limit)
SAFE_STATE = {"gsDPPipeSync", "gsSPClearGeometryMode", "gsSPSetGeometryMode", "gsDPSetTextureLUT", "gsSPTexture",
              "gsDPLoadTextureBlock", "gsDPLoadMultiBlock", "gsDPLoadTLUT", "gsDPSetCombineLERP",
              "gsDPSetRenderMode", "gsDPSetPrimColor", "gsDPSetEnvColor", "gsSPDisplayList",
              "gsDPLoadTextureBlock_4b", "gsDPLoadMultiBlock_4b", "gsDPSetTile", "gsDPSetTileSize",
              "gsDPLoadSync", "gsDPTileSync", "gsDPSetTextureImage", "gsDPLoadBlock", "gsDPSetFogColor",
              "gsDPLoadTLUT_pal16", "gsDPLoadTLUT_pal256", "gsDPLoadTextureBlock_4b", "gsDPLoadTLUT",
              "gsDPLoadMultiBlock_4b"}  # (the palette loads and 4-bit textures of tools/texpack.py's output)


def split_items(text):
    """the top-level macro calls of a display list file, as strings"""
    items, depth, cur = [], 0, []
    for ch in text:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            s = "".join(cur).strip()
            if s:
                items.append(s)
            cur = []
            continue
        cur.append(ch)
    s = "".join(cur).strip()
    if s:
        items.append(s)
    return items


def args_of(item):
    inner = item[item.index("(") + 1:item.rindex(")")]
    out, depth, cur = [], 0, []
    for ch in inner:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
            continue
        cur.append(ch)
    if "".join(cur).strip():
        out.append("".join(cur).strip())
    return out


def name_of(item):
    return item[:item.index("(")].strip()


VTX_REF = re.compile(r"&?\s*(\w+)\s*(?:\[\s*(\d+)\s*\])?")


ORIGIN = {}  # a vertex (its fields) -> the first room vertex it was read from ("array[index]"): its copies' source


def load_vtx(dirpath, arr):
    """the array's vertices as their argument strings (each remembered as the copies' source, ORIGIN)"""
    f = os.path.join(dirpath, arr + ".inc.c")
    vs = [tuple(a.strip() for a in m.split(",")) for m in re.findall(r"VTX\(([^)]*)\)", open(f).read())]
    for i, v in enumerate(vs):
        ORIGIN.setdefault(v, "%s[%d]" % (arr, i))
    return vs


def state_key(c):
    """what a state command sets, for the entry check (None: not state, as a pipe sync)"""
    n = name_of(c)
    if n in ("gsDPPipeSync", "gsDPLoadSync", "gsDPTileSync"):
        return None
    if n in ("gsSPSetGeometryMode", "gsSPClearGeometryMode"):
        return "geometry"
    if n in ("gsDPLoadMultiBlock", "gsDPLoadMultiBlock_4b", "gsDPSetTile", "gsDPSetTileSize"):
        return (n, args_of(c)[1] if len(args_of(c)) > 1 else "")  # (by its texture memory address or tile)
    if n == "gsSPDisplayList":
        if args_of(c)[0].startswith("0x0A"):
            return "gsDPSetEnvColor"  # (caustic_fade's middle tier: an environment colour)
        return (n, args_of(c)[0])
    return n


def cut(dirpath, dl):
    """the piece's list cut into areas: (area lists, new vertex arrays, the piece's new list) or (None, reason)"""
    text = open(os.path.join(dirpath, dl + ".inc.c")).read()
    items = split_items(text)
    arrays = {}
    stream = []        # the state commands in order (each a string)
    tris = []          # (state count before it, (vtx, vtx, vtx), centre x, centre z)
    slots = [None] * 64
    seen_cull = False
    for it in items:
        n = name_of(it)
        if n == "gsSPVertex":
            a = args_of(it)
            m = VTX_REF.fullmatch(a[0].strip())
            if not m:
                return None, "vertex reference %r" % a[0]
            arr, base = m.group(1), int(m.group(2) or 0)
            if arr not in arrays:
                arrays[arr] = load_vtx(dirpath, arr)
            cnt, v0 = int(a[1], 0), int(a[2], 0)
            for k in range(cnt):
                slots[v0 + k] = arrays[arr][base + k]
        elif n == "gsSPCullDisplayList":
            if seen_cull or tris:
                return None, "a cull command after the first"
            seen_cull = True
        elif n in ("gsSP1Triangle", "gsSP2Triangles"):
            a = [int(x, 0) for x in args_of(it)]
            groups = [a[0:3]] if n == "gsSP1Triangle" else [a[0:3], a[4:7]]
            for g in groups:
                vs = tuple(slots[i] for i in g)
                if None in vs:
                    return None, "a triangle on an unloaded vertex"
                cx = sum(int(v[0]) for v in vs) / 3.0
                cz = sum(int(v[2]) for v in vs) / 3.0
                tris.append((len(stream), vs, cx, cz))
        elif n == "gsSPEndDisplayList":
            break
        elif n in SAFE_STATE:
            if n == "gsDPSetRenderMode" and ("DEC" in it or "XLU" in it):
                return None, "a decal or translucent render mode"
            stream.append(it)
        else:
            return None, "a command not known here: %s" % n
    if not tris:
        return None, "no triangles"
    # every kind of state the list changes must be set before its first triangle: then an area replaying the list's
    # state up to its own triangles sets every one of them to what the original had there, whatever an area before it
    # left (one changed only later could still hold a later value when an earlier triangle is drawn)
    # (the material cut, the default, keeps every state command where it was and only regroups the triangles between
    # two of them: this holds for the per-area cut alone)
    first = tris[0][0]
    early = {state_key(c) for c in stream[:first]}
    for c in stream[first:]:
        k = state_key(c)
        if BY_AREA and k is not None and k not in early:
            return None, "state first set after a triangle (%s)" % name_of(c)
    # the original's bounding box load and cull come first in its stream (state only around them): the cull itself is
    # dropped here, the box vertices aren't in `stream` (vertex loads aren't state)
    cells = collections.defaultdict(list)
    for t in tris:
        cells[(int(t[2] // CELL), int(t[3] // CELL))].append(t)
    # small areas joined to the nearest bigger one
    while True:
        small = [k for k, v in cells.items() if len(v) < MIN_TRIS]
        if not small or len(cells) == 1:
            break
        k = min(small, key=lambda c: len(cells[c]))
        others = [c for c in cells if c != k]
        tgt = min(others, key=lambda c: (c[0] - k[0]) ** 2 + (c[1] - k[1]) ** 2)
        cells[tgt].extend(cells.pop(k))
    order = sorted(cells)
    if not BY_AREA:
        return cut_by_material(dl, stream, tris, cells, order)
    area_dls = []
    area_vtx = []
    for ai, key in enumerate(order):
        ts = sorted(cells[key], key=lambda t: tris.index(t))  # (original order)
        name = "%s_mp%d" % (dl, ai)
        vname = name + "_Vtx"
        vtx = []
        out = []
        # the area's box: its triangles' vertices' extent
        xs = [int(v[0]) for t in ts for v in t[1]]
        ys = [int(v[1]) for t in ts for v in t[1]]
        zs = [int(v[2]) for t in ts for v in t[1]]
        box_base = 0
        for x in (min(xs), max(xs)):
            for y in (min(ys), max(ys)):
                for z in (min(zs), max(zs)):
                    vtx.append(("%d" % x, "%d" % y, "%d" % z, "0", "0", "0", "0", "0", "0"))
        out.append("gsSPClearGeometryMode(G_FOG | G_LIGHTING)")
        out.append("gsSPVertex(&%s[%d], 8, 0)" % (vname, box_base))
        out.append("gsSPCullDisplayList(0, 7)")
        emitted = 0  # state commands of the stream emitted so far
        batch = []   # vertex tuples in this load
        pending = []  # triangles (slot triples) for this load

        def flush():
            if not batch:
                return
            start = len(vtx)
            vtx.extend(batch)
            out.append("gsSPVertex(&%s[%d], %d, 0)" % (vname, start, len(batch)))
            k = 0
            while k < len(pending):
                if k + 1 < len(pending):
                    a, b = pending[k], pending[k + 1]
                    out.append("gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0)" % (a + b))
                    k += 2
                else:
                    a = pending[k]
                    out.append("gsSP1Triangle(%d, %d, %d, 0)" % a)
                    k += 1
            batch.clear()
            pending.clear()

        for t in ts:
            if t[0] > emitted:
                flush()
                out.extend(stream[emitted:t[0]])
                emitted = t[0]
            need = [v for v in dict.fromkeys(t[1]) if v not in batch]
            if len(batch) + len(need) > VTX_SLOTS:
                flush()
            for v in t[1]:
                if v not in batch:
                    batch.append(v)
            pending.append(tuple(batch.index(v) for v in t[1]))
        flush()
        out.append("gsSPEndDisplayList()")
        area_dls.append((name, out))
        last_emitted = emitted
        area_vtx.append((vname, vtx))
    # the piece's own list: the areas in turn (state set inside each area)
    # ... then the rest of the list's state commands, so the piece hands the state on as the original did (the
    # next piece drawn may inherit some, as the lake's do the environment colour)
    new_dl = ["gsSPDisplayList(%s)" % n for n, _ in area_dls] + list(stream[last_emitted:]) + ["gsSPEndDisplayList()"]
    # check: every triangle once, same vertices, after the same state commands in order
    got = collections.Counter()
    for (name, out), (vname, vtx) in zip(area_dls, area_vtx):
        n_state = 0
        sl = [None] * 64
        culled = False
        for it in out:
            nm = name_of(it)
            if nm == "gsSPVertex":
                a = args_of(it)
                m = VTX_REF.fullmatch(a[0])
                base, cnt = int(m.group(2)), int(a[1])
                for k in range(cnt):
                    sl[k] = vtx[base + k]
            elif nm in ("gsSP1Triangle", "gsSP2Triangles"):
                a = [int(x, 0) for x in args_of(it)]
                for g in ([a[0:3]] if nm == "gsSP1Triangle" else [a[0:3], a[4:7]]):
                    got[(n_state, tuple(sl[i] for i in g))] += 1
            elif nm == "gsSPCullDisplayList":
                culled = True
            elif nm != "gsSPEndDisplayList" and culled:
                # the stream's commands come in order: count how far along it this area is
                if n_state < len(stream) and it == stream[n_state]:
                    n_state += 1
                else:
                    return None, "check: state out of order in %s" % name
    want = collections.Counter((t[0], t[1]) for t in tris)
    if got != want:
        return None, "check: the triangles differ"
    return (area_dls, area_vtx, new_dl, len(tris), len(order), {}), None


def emit_tris(ts, vname, vtx, out):
    """the triangles' vertex loads (identical vertices in a load loaded once) and triangles, appended to out"""
    batch = []
    pending = []

    def flush():
        if not batch:
            return
        start = len(vtx)
        vtx.extend(batch)
        out.append("gsSPVertex(&%s[%d], %d, 0)" % (vname, start, len(batch)))
        k = 0
        while k < len(pending):
            if k + 1 < len(pending):
                out.append("gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0)" % (pending[k] + pending[k + 1]))
                k += 2
            else:
                out.append("gsSP1Triangle(%d, %d, %d, 0)" % pending[k])
                k += 1
        batch.clear()
        pending.clear()

    for t in ts:
        need = [v for v in dict.fromkeys(t[1]) if v not in batch]
        if len(batch) + len(need) > VTX_SLOTS:
            flush()
        for v in t[1]:
            if v not in batch:
                batch.append(v)
        pending.append(tuple(batch.index(v) for v in t[1]))
    flush()


def cut_by_material(dl, stream, tris, cells, order):
    """The piece drawn material by material, as the original: its state commands (texture loads among them) run once
    each, in the piece's own list, in their order; after each run of state, every area with triangles drawn under it
    is called in turn: a small list of its own (that area's triangles under that state: their box, the cull, their
    vertices and triangles; no state), so an area out of view costs its box and the textures stay loaded. (The areas
    first cut replayed the state before each of their triangles, every texture load before them included: more
    loads for the RDP than the piece had.)"""
    area_of = {}
    for ai, key in enumerate(order):
        for t in cells[key]:
            area_of[id(t)] = ai
    by_state = collections.OrderedDict()
    for t in tris:  # (the original order: the state points ascending)
        by_state.setdefault(t[0], collections.defaultdict(list))[area_of[id(t)]].append(t)
    # each state point's lighting (G_LIGHTING as the piece's state has it there): a split vertex's normal or colour
    lit_at, lit = {}, None
    for j, c in enumerate(stream + [None]):
        lit_at[j] = lit
        if c is not None and name_of(c) in ("gsSPSetGeometryMode", "gsSPClearGeometryMode") and \
                "G_LIGHTING" in [x.strip() for x in args_of(c)[0].split("|")]:
            lit = name_of(c) == "gsSPSetGeometryMode"
    sub_dls, sub_vtx, new_dl = [], [], []
    sub_state = []  # (each area list, the state point it's drawn at)
    # each state point's areas' triangles by band (-1: dry, or no water fog), first for all of them
    parts_of = collections.OrderedDict()
    for mi, (k, areas) in enumerate(by_state.items()):
        for ai in sorted(areas):
            if WATER_FOG and any(min(int(v[1], 0) for v in t[1]) < WATER_Y for t in areas[ai]):
                by_band = collections.defaultdict(list)
                for t in areas[ai]:
                    for b, piece in band_split(t[1], lit_at[t[0]]):
                        by_band[b].append((t[0], piece, t[2], t[3]))
            else:
                by_band = {-1: list(areas[ai])}
            parts_of[(mi, k, ai)] = by_band
    # ... merged back into as few triangles as the surface allows (meshpack_merge.py), each band of each area and
    # state point a group of its own: the depth lines and the areas' edges stay where they are
    if DEPTH_MERGE and WATER_FOG and not FOG_NOCUT:
        items = []
        for (mi, k, ai), by_band in parts_of.items():
            for b, ts in by_band.items():
                for t in ts:
                    items.append(((mi, k, ai, b), t[1], (t[0], t[2], t[3])))
        merged, gone = meshpack_merge.merge(items, lambda g: bool(lit_at[g[1]]))
        MERGED[0] += gone
        for key in parts_of:
            parts_of[key] = collections.defaultdict(list)
        for (mi, k, ai, b), t, pay in merged:
            parts_of[(mi, k, ai)][b].append((pay[0], t, pay[1], pay[2]))
    emitted = 0
    last_mi = None
    for (mi, k, ai), by_band in parts_of.items():
        if mi != last_mi:
            new_dl.extend(stream[emitted:k])
            emitted = k
            last_mi = mi
        if True:
            parts = [(by_band[b], b, "" if b < 0 else "b%d" % b) for b in sorted(by_band) if by_band[b]]
            wet = False
            for ts, band, suffix in parts:
                name = "%s_mp%d_%d%s" % (dl, ai, mi, suffix)
                vname = name + "_Vtx"
                vtx = []
                out = []
                xs = [int(v[0]) for t in ts for v in t[1]]
                ys = [int(v[1]) for t in ts for v in t[1]]
                zs = [int(v[2]) for t in ts for v in t[1]]
                for x in (min(xs), max(xs)):
                    for y in (min(ys), max(ys)):
                        for z in (min(zs), max(zs)):
                            vtx.append(("%d" % x, "%d" % y, "%d" % z, "0", "0", "0", "0", "0", "0"))
                out.append("gsSPVertex(&%s[0], 8, 0)" % vname)
                out.append("gsSPCullDisplayList(0, 7)")
                emit_tris(ts, vname, vtx, out)
                out.append("gsSPEndDisplayList()")
                sub_dls.append((name, out))
                sub_vtx.append((vname, vtx))
                sub_state.append((name, k))
                if band >= 0:
                    new_dl.append(fog_band_call(band))  # (that band's fog)
                    wet = True
                new_dl.append("gsSPDisplayList(%s)" % name)
            if wet:
                new_dl.append(FOG_OFF_CALL)  # (the scene's fog again)
    new_dl.extend(stream[emitted:])
    new_dl.append("gsSPEndDisplayList()")
    # check: walking the piece's list, every state command of the original once and in order, and every triangle
    # once with the same vertices after the same state commands
    subs = dict(sub_dls)
    vtxs = dict(sub_vtx)
    got = collections.Counter()
    n_state = 0
    for it in new_dl[:-1]:
        nm = name_of(it)
        if it in FOG_CALLS:
            continue  # (the water fog's lists: no state of the piece's)
        if nm == "gsSPDisplayList" and args_of(it)[0] in subs:
            name = args_of(it)[0]
            sl = [None] * 64
            for c in subs[name]:
                cn = name_of(c)
                if cn == "gsSPVertex":
                    a = args_of(c)
                    m = VTX_REF.fullmatch(a[0])
                    base, cnt = int(m.group(2)), int(a[1])
                    for j in range(cnt):
                        sl[j] = vtxs[name + "_Vtx"][base + j]
                elif cn in ("gsSP1Triangle", "gsSP2Triangles"):
                    a = [int(x, 0) for x in args_of(c)]
                    for g in ([a[0:3]] if cn == "gsSP1Triangle" else [a[0:3], a[4:7]]):
                        got[(n_state, tuple(sl[i] for i in g))] += 1
                elif cn not in ("gsSPCullDisplayList", "gsSPEndDisplayList"):
                    return None, "check: state in an area's list (%s)" % c
        elif n_state < len(stream) and it == stream[n_state]:
            n_state += 1
        else:
            return None, "check: state out of order (%s)" % it
    if n_state != len(stream):
        return None, "check: %d of %d state commands" % (n_state, len(stream))
    want = collections.Counter((t[0], t[1]) for t in tris)
    if got != want:
        # (split at the water's bands: the same surface after each state point, in its pieces)
        ga, wa = collections.defaultdict(float), collections.defaultdict(float)
        for (k, t), c in got.items():
            ga[k] += tri_area(t) * c
        for (k, t), c in want.items():
            wa[k] += tri_area(t) * c
        if not WATER_FOG or any(abs(ga[k] - wa[k]) > 0.002 * wa[k] + 4 for k in set(ga) | set(wa)):
            return None, "check: the triangles differ"
    entry_lit = {}
    for name, k in sub_state:
        entry_lit[name] = lit_at[k]
    return (sub_dls, sub_vtx, new_dl, len(tris), len(order), entry_lit), None


def relight(lists, arrays_of, entry_lit=None):
    """The lit pieces' vertex loads moved to segment RELIT_SEG, lighting off: lists is {name: [commands]} (each list
    changed in place); arrays_of(name) gives an array's vertices. Returns the vertices in the table's order (the CPU
    lights them, code/fishmod_relight.c) or None and why. A load is lit when G_LIGHTING is on where it is (the lists
    are walked from their start, where each piece sets its geometry mode); the vertices are the same bytes, the
    colour the RSP would have computed from their normals is the CPU's now."""
    table = []
    for name, cmds in lists.items():
        lit = (entry_lit or {}).get(name)  # (an area's list called with the piece's state: known; else unknown)
        out = []
        for i, c in enumerate(cmds):
            n = name_of(c)
            if n in ("gsSPSetGeometryMode", "gsSPClearGeometryMode"):
                terms = [t.strip() for t in args_of(c)[0].split("|")]
                if "G_LIGHTING" in terms:
                    lit = (n == "gsSPSetGeometryMode")
                    if n == "gsSPSetGeometryMode":
                        terms = [t for t in terms if t != "G_LIGHTING"]
                        c = "%s(%s)" % (n, " | ".join(terms)) if terms else "gsSPNoOp()"
            elif n == "gsSPVertex" and (i + 1 < len(cmds)) and name_of(cmds[i + 1]) == "gsSPCullDisplayList":
                pass  # (a bounding box: positions only, for the cull)
            elif n == "gsSPVertex" and lit:
                a = args_of(c)
                m = VTX_REF.fullmatch(a[0].strip())
                if not m:
                    return None, "vertex reference %r in %s" % (a[0], name)
                vs = arrays_of(m.group(1))
                base, cnt = int(m.group(2) or 0), int(a[1], 0)
                off = len(table)
                table.extend(vs[base:base + cnt])
                c = "gsSPVertex((Vtx*)0x%08X, %s, %s)" % ((RELIT_SEG << 24) + off * 16, a[1], a[2])
            elif n == "gsSPVertex" and lit is None:
                return None, "a vertex load before the lighting is known in %s" % name
            elif n == "gsSPDisplayList" and lit:
                pass  # (the scene's material lists: colours and tiles, not the geometry mode)
            out.append(c)
        cmds[:] = out
    return table, None


ADDED = "\x00"  # (caustic_fade: a command it added, marked for its check)


def caustic_fade(dirpath, dl):
    """The piece's caustic triangles regrouped by depth (see CAUSTICS): each run of vertex loads and triangles under a
    caustic combiner redrawn as its shallow triangles, then (if any) its middle ones after the middle tier's colour,
    then its deep ones after 255, then the colour the run started with set again. Returns ({vertex array: vertices},
    triangles moved) and rewrites the piece's list; ({}, 0) when nothing is deep enough to change."""
    path = os.path.join(dirpath, dl + ".inc.c")
    items = split_items(open(path).read())
    arrays = {}
    slots = [None] * 64
    caustic = False
    env_src = None      # the command that set the environment colour last (a literal or a segment's list)
    out, run, new_arrays, moved = [], [], {}, 0
    lit_now = [None]   # (G_LIGHTING where the list is: a cut vertex's normal or colour)

    def tier(vs):
        depth = WATER_Y - sum(int(v[1]) for v in vs) / 3.0
        f = min(max((depth - CAUSTIC_FROM) / float(CAUSTIC_TO - CAUSTIC_FROM), 0.0), 1.0)
        return int(round(f * CAUSTIC_STEPS))

    def cut_tiers(tris):
        """each triangle cut where its step changes (the step edges' depths), into its pieces"""
        out_ = []
        for t in tris:
            ys = [int(v[1]) for v in t]
            poly = list(t)
            pieces = []
            for d in CAUSTIC_EDGES:
                y0 = WATER_Y - d
                if min(int(v[1]) for v in poly) < y0 <= max(int(v[1]) for v in poly):
                    above, below = clip_poly(poly, y0, lit_now[0])
                    if len(above) >= 3:
                        pieces.append(above)
                    poly = below
                    if len(poly) < 3:
                        break
            if len(poly) >= 3:
                pieces.append(poly)
            if len(pieces) == 1:
                out_.append(t)
                continue
            for pc in pieces:
                out_ += [(pc[0], pc[i], pc[i + 1]) for i in range(1, len(pc) - 1)]
        return out_

    def end_run():
        nonlocal moved
        if not run:
            return
        tris = [t for kind, t in run if kind == "tri"]
        if CAUSTIC_CUT and caustic:
            tris = cut_tiers(tris)
        tiers = [tier(t) for t in tris]
        if not caustic or not any(tiers) or env_src is None:
            out.extend(c for kind, c in run if kind == "cmd")
            run.clear()
            return
        vname = "%s_cf%d_Vtx" % (dl, len(new_arrays))
        vtx = []
        for t_ in range(CAUSTIC_STEPS + 1):
            ts = [(None, t) for t, k in zip(tris, tiers) if k == t_]
            if not ts:
                continue
            if t_ == CAUSTIC_STEPS:
                step = "gsDPSetEnvColor(0, 0, 0, 255)"
            elif t_ > 0 and name_of(env_src) == "gsDPSetEnvColor":
                a = args_of(env_src)  # (a colour of the list's own: the step's own colour)
                a0 = int(a[3], 0)
                step = "gsDPSetEnvColor(%s, %s, %s, %d)" % (a[0], a[1], a[2], a0 + (255 - a0) * t_ // CAUSTIC_STEPS)
            elif t_ > 0:
                step = "gsSPDisplayList(0x%08X)" % (0x0A000000 + (t_ - 1) * 16)
            if t_ > 0:
                out.extend([ADDED + "gsDPPipeSync()", ADDED + step])
            emit_tris(ts, vname, vtx, out)
        out.extend([ADDED + "gsDPPipeSync()", ADDED + env_src])
        new_arrays[vname] = vtx
        moved += sum(1 for k in tiers if k)
        run.clear()

    for it in items:
        n = name_of(it)
        if n == "gsSPVertex":
            a = args_of(it)
            m = VTX_REF.fullmatch(a[0].strip())
            arr, base = m.group(1), int(m.group(2) or 0)
            if arr not in arrays:
                arrays[arr] = load_vtx(dirpath, arr)
            cnt, v0 = int(a[1], 0), int(a[2], 0)
            for k in range(cnt):
                slots[v0 + k] = arrays[arr][base + k]
            run.append(("cmd", it))
            continue
        if n in ("gsSP1Triangle", "gsSP2Triangles"):
            a = [int(x, 0) for x in args_of(it)]
            for g in ([a[0:3]] if n == "gsSP1Triangle" else [a[0:3], a[4:7]]):
                run.append(("tri", tuple(slots[i] for i in g)))
            run.append(("cmd", it))
            continue
        if n == "gsSPCullDisplayList":
            # (the box load before it is in the run: no triangles yet, so the run is passed through as it was)
            run.append(("cmd", it))
            end_run()
            continue
        end_run()
        if n in ("gsSPSetGeometryMode", "gsSPClearGeometryMode") and \
                "G_LIGHTING" in [x.strip() for x in args_of(it)[0].split("|")]:
            lit_now[0] = n == "gsSPSetGeometryMode"
        if n == "gsDPSetCombineLERP":
            caustic = "ENV_ALPHA" in args_of(it)[:4]
        elif n == "gsDPSetEnvColor" or (n == "gsSPDisplayList" and args_of(it)[0] in ("0x08000000", "0x09000000")):
            env_src = it
        out.append(it)
    end_run()
    if not moved:
        return {}, 0
    # check: every triangle once, with the same vertices, after the same original state commands
    def tris_of(cmds, extra):
        got = collections.Counter()
        sl = [None] * 64
        k = 0
        for c in cmds:
            if c.startswith(ADDED):
                continue
            nm = name_of(c)
            if nm == "gsSPVertex":
                a = args_of(c)
                m = VTX_REF.fullmatch(a[0].strip())
                src = extra.get(m.group(1)) or arrays[m.group(1)]
                for j in range(int(a[1], 0)):
                    sl[int(a[2], 0) + j] = src[int(m.group(2) or 0) + j]
            elif nm in ("gsSP1Triangle", "gsSP2Triangles"):
                a = [int(x, 0) for x in args_of(c)]
                for g in ([a[0:3]] if nm == "gsSP1Triangle" else [a[0:3], a[4:7]]):
                    got[(k, tuple(sl[i] for i in g))] += 1
            elif nm != "gsSPCullDisplayList":
                k += 1
        return got
    got_, want_ = tris_of(out, new_arrays), tris_of(items, {})
    if got_ != want_:
        # (cut at the steps' edges: the same surface after each state point, in its pieces)
        ga, wa = collections.defaultdict(float), collections.defaultdict(float)
        for (k, t), c in got_.items():
            ga[k] += tri_area(tuple(tuple(str(x) for x in v) for v in t)) * c
        for (k, t), c in want_.items():
            wa[k] += tri_area(tuple(tuple(str(x) for x in v) for v in t)) * c
        if not CAUSTIC_CUT or any(abs(ga[k] - wa[k]) > 0.002 * wa[k] + 4 for k in set(ga) | set(wa)):
            sys.exit("meshpack: caustic_fade check failed for %s" % dl)
    out = [c[len(ADDED):] if c.startswith(ADDED) else c for c in out]
    os.remove(path)
    open(path, "w").write("".join("    %s,\n" % c for c in out))
    for vname, vtx in new_arrays.items():
        open(os.path.join(dirpath, vname + ".inc.c"), "w").write("".join("VTX(%s),\n" % ", ".join(v) for v in vtx))
    return new_arrays, moved


def grey_caustics(dirpath, scene, texs, dls):
    """each caustic texture made i8 (its brightness) in place: its picture, its array's size, and its loads in the
    caustic lists (dls). Returns how many loads changed."""
    from PIL import Image
    for t in texs:
        src = os.path.join(dirpath, t + ".rgba16.png")
        im = Image.open(src).convert("RGB")
        grey = Image.new("L", im.size)
        grey.putdata([int(round((r * 299 + g * 587 + b * 114) / 1000.0)) for r, g, b in im.getdata()])
        os.remove(src)
        grey.save(os.path.join(dirpath, t + ".i8.png"))
    for ext in (".c", ".h"):
        p = os.path.join(dirpath, scene + ext)
        text = open(p).read()
        for t in texs:
            text = text.replace("%s_WIDTH, %s_HEIGHT, 16)]" % (t, t), "%s_WIDTH, %s_HEIGHT, 8)]" % (t, t))
            text = text.replace("%s.rgba16.inc.c" % t, "%s.i8.inc.c" % t)
        os.remove(p)
        open(p, "w").write(text)
    n = 0
    for dl in dls:
        p = os.path.join(dirpath, dl + ".inc.c")
        text = open(p).read()
        for t in texs:
            old = "gsDPLoadTextureBlock(%s, G_IM_FMT_RGBA, G_IM_SIZ_16b," % t
            n += text.count(old)
            text = text.replace(old, "gsDPLoadTextureBlock(%s, G_IM_FMT_I, G_IM_SIZ_8b," % t)
            if re.search(r"gsDPLoad\w*\(%s,[^)]*G_IM_FMT_RGBA" % t, text):
                sys.exit("meshpack: %s loaded some other way in %s" % (t, dl))
        os.remove(p)
        open(p, "w").write(text)
    return n


def unsize(dirpath, room, dl):
    """the list's declared length (the room's .c and .h give each list its original length) left to its contents"""
    for ext in (".c", ".h"):
        p = os.path.join(dirpath, room + ext)
        text = open(p).read()
        new = re.sub(r"\b%s\[\d+\]" % re.escape(dl), dl + "[]", text)
        if new != text:
            os.remove(p)  # (a hard link: written fresh)
            open(p, "w").write(new)


# The lake's water surface (object_spot06_objects: gLakeHyliaHighWaterDL, gLakeHyliaLowWaterDL), its texture and
# material made again from vanilla's: its 32x32 colour texture (2 KB) as the template, its brightness upscaled to a 64x64
# intensity texture (I4: the same 2 KB, 4 times the texels), its two layers at vanilla's sizes on the water (a ripple
# every 77 units, the second layer every 154, mirrored) with twice the texels, each scrolled its own way
# (z_bg_spot06_objects.c). The colour is the frame's: the waves move it from vanilla's darkest colour (the
# environment's) to its brightest (the primitive's), in the scene's light (fishmod_water_colours.h, written here); the
# alpha is vanilla's, one value over the whole surface (the environment's). MESHPACK_NO_WATER_SURFACE=1: vanilla's.
WATER_SURFACE = not os.environ.get("MESHPACK_NO_WATER_SURFACE")
WATER_DIR = "assets/objects/object_spot06_objects"
WATER_TEX = "gLakeHyliaWaterTex"
WATER_W = 64


def water_texture(src_png, path):
    """the waves: vanilla's own texture as the template, its brightness (its colour is the frame's now: z_bg_spot06_
    objects.c) upscaled to 64x64 (tiled 3 x 3 first, so the edges join as they did), 4-bit grey; and the base and
    highlight colours it's drawn between, fitted to vanilla's (below)"""
    import numpy as np
    from PIL import Image
    im = np.asarray(Image.open(src_png).convert("RGB"), float)
    lum = im @ np.array([0.299, 0.587, 0.114])
    n = im.shape[0]
    big = Image.fromarray(np.tile(lum, (3, 3)).astype(np.float32), "F").resize((WATER_W * 3, WATER_W * 3),
                                                                                 Image.BICUBIC)
    h = np.asarray(big)[WATER_W:2 * WATER_W, WATER_W:2 * WATER_W]
    h = np.clip((h - h.min()) / max(h.max() - h.min(), 1e-6), 0, 1)
    v = np.round(h * 15).astype(np.uint8) * 17
    Image.fromarray(v, "L").save(path)
    # the base and highlight: vanilla's colour against its brightness, a line fitted per channel through its texels as
    # the game shows them (its two layers averaged, at many offsets), taken at the darkest and brightest brightness
    # (the texture's 0 and 1): the waves' colours then go as vanilla's do (within about 4 of 255)
    rng = np.random.default_rng(1)
    cols = []
    lums = []
    for _ in range(64):
        dy, dx = rng.integers(0, n, 2)
        cols.append(((im + np.roll(im, (dy, dx), (0, 1))) / 2).reshape(-1, 3))
        lums.append(((lum + np.roll(lum, (dy, dx), (0, 1))) / 2).ravel())
    cols = np.concatenate(cols)
    lums = np.concatenate(lums)
    coef = np.linalg.lstsq(np.stack([np.ones_like(lums), lums], 1), cols, rcond=None)[0]
    low = np.clip(coef[0] + coef[1] * lum.min(), 0, 255)
    high = np.clip(coef[0] + coef[1] * lum.max(), 0, 255)
    mean = im.reshape(-1, 3).mean(0)
    return ([int(round(c)) for c in low], [int(round(c)) for c in high], [int(round(c)) for c in mean])


WATER_MATERIAL_OLD = re.compile(r"gsDPLoadTextureBlock\(gLakeHyliaWaterTex,.*?gsSPSetGeometryMode\(G_FOG \| G_LIGHTING\),",
                                re.S)
WATER_MATERIAL = (
    "gsDPLoadTextureBlock_4b(gLakeHyliaWaterTex, G_IM_FMT_I, 64, 64, 0, G_TX_NOMIRROR | G_TX_WRAP, "
    "G_TX_NOMIRROR | G_TX_WRAP, 6, 6, 13, 13),\n"
    "    gsDPTileSync(),\n"
    "    gsDPSetTile(G_IM_FMT_I, G_IM_SIZ_4b, 4, 0x0000, 1, 0, G_TX_MIRROR | G_TX_WRAP, 6, 14, "
    "G_TX_MIRROR | G_TX_WRAP, 6, 14),\n"
    "    gsDPSetTileSize(1, 0, 0, 0x00FC, 0x00FC),\n"
    "    gsDPSetCombineLERP(TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0, TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0, PRIMITIVE, "
    "ENVIRONMENT, COMBINED, ENVIRONMENT, 0, 0, 0, ENVIRONMENT),\n"
    "    gsDPSetRenderMode(G_RM_FOG_SHADE_A, G_RM_AA_ZB_XLU_SURF2),\n"
    "    gsSPClearGeometryMode(G_CULL_BACK | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LIGHTING),\n"
    "    gsSPSetGeometryMode(G_FOG),")


def water_surface(out):
    d = os.path.join(out, WATER_DIR)
    n = 0
    for dl in ("gLakeHyliaHighWaterDL", "gLakeHyliaLowWaterDL"):
        p = os.path.join(d, dl + ".inc.c")
        t = open(p).read()
        t2, k = WATER_MATERIAL_OLD.subn(WATER_MATERIAL, t)
        if k != 1:
            sys.exit("meshpack: %s's material not as expected" % dl)
        # (the colours and alphas are the frame's, in the scroll lists of segments 8 and 9: the list's own go)
        t2 = re.sub(r"\s*gsDPSetPrimColor\(0, 0, 255, 255, 255, \d+\),", "", t2)
        os.remove(p)
        open(p, "w").write(t2)
        n += 1
    template = os.path.join(d, WATER_TEX + ".rgba16.png")
    out_png = os.path.join(d, WATER_TEX + ".i4.png")
    tmp = out_png + ".tmp.png"
    low, high, mean = water_texture(template, tmp)
    for f in os.listdir(d):
        if f.startswith(WATER_TEX + ".") and not f.endswith(".tmp.png"):
            os.remove(os.path.join(d, f))
    os.rename(tmp, out_png)
    # (the base and highlight colours for the game: vanilla's own, by its darkest and brightest texels)
    hp = os.path.join(d, "fishmod_water_colours.h")
    open(hp, "w").write("// tools/meshpack.py: vanilla's water texture's darkest, brightest and average colours\n"
                        "#define FISHMOD_WATER_BASE { %d, %d, %d }\n#define FISHMOD_WATER_HIGH { %d, %d, %d }\n"
                        "#define FISHMOD_WATER_MEAN { %d, %d, %d }\n" % tuple(low + high + mean))
    for f, a, b in (("object_spot06_objects.c", WATER_TEX + ".rgba16.inc.c", WATER_TEX + ".i4.inc.c"),
                    ("object_spot06_objects.c", "gLakeHyliaWaterTex_HEIGHT, 16)]", "gLakeHyliaWaterTex_HEIGHT, 4)]"),
                    ("object_spot06_objects.h", "gLakeHyliaWaterTex_WIDTH 32", "gLakeHyliaWaterTex_WIDTH 64"),
                    ("object_spot06_objects.h", "gLakeHyliaWaterTex_HEIGHT 32", "gLakeHyliaWaterTex_HEIGHT 64"),
                    ("object_spot06_objects.h", "gLakeHyliaWaterTex_HEIGHT, 16)]", "gLakeHyliaWaterTex_HEIGHT, 4)]")):
        p = os.path.join(d, f)
        t = open(p).read()
        if a not in t:
            sys.exit("meshpack: %s: %r not found" % (f, a))
        os.remove(p)
        open(p, "w").write(t.replace(a, b))
    return n


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    src, out = sys.argv[1], sys.argv[2]
    report = "--report" in sys.argv
    if os.path.exists(out):
        shutil.rmtree(out)
    # a copy of the tree of hard links (the files changed below are written fresh, breaking their link)
    shutil.copytree(src, out, copy_function=os.link)
    if WATER_SURFACE:
        n = water_surface(out)
        if report:
            print("meshpack: the lake's water surface: %d lists, a 64x64 intensity texture of waves" % n)
    for scene_dir, scene, texs in GREY_CAUSTICS:
        dls = [dl for sd, _, ds in CAUSTICS if sd == scene_dir for dl in ds]
        n = grey_caustics(os.path.join(out, scene_dir), scene, texs, dls)
        if report:
            print("meshpack: %s: %d caustic textures made light alone (i8), %d loads" % (scene, len(texs), n))
    fade_arrays = collections.defaultdict(dict)  # room -> {vertex array: vertices} (caustic_fade's)
    for scene_dir, room, dls in (CAUSTICS if CAUSTIC_FADE else []):
        d = os.path.join(out, scene_dir)
        for dl in dls:
            arrs, moved = caustic_fade(d, dl)
            fade_arrays[room].update(arrs)
            if moved:
                unsize(d, room, dl)
            if report:
                print("meshpack: %s: %d caustic triangles faded with depth" % (dl, moved))
    for scene_dir, room, dls in TARGETS:
        d = os.path.join(out, scene_dir)
        areas = {}      # area list name -> commands
        area_arrays = {}  # area vertex array name -> vertices
        pieces = {}     # piece list name -> commands (as the build will have them)
        entry_lit = {}  # area list name -> the lighting where it's called (the material cut's lists set no state)
        for dl in dls:
            res, why = cut(d, dl)
            if res is None:
                print("meshpack: %s left as it is (%s)" % (dl, why))
                continue
            area_dls, area_vtx, new_dl, ntri, nareas, lits = res
            entry_lit.update(lits)
            pieces[dl] = new_dl
            for (name, cmds), (vname, vtx) in zip(area_dls, area_vtx):
                areas[name] = cmds
                area_arrays[vname] = vtx
            if report:
                nout = sum(sum(1 for c in cmds if "Triangle" in c and "gsSP2" in c) * 2 +
                           sum(1 for c in cmds if "gsSP1Triangle" in c) for _, cmds in area_dls)
                print("meshpack: %s: %d triangles in %d areas, %d area lists; %d drawn (%d merged away so far)" %
                      (dl, ntri, nareas, len(area_dls), nout, MERGED[0]))
        table = None
        if room in RELIGHT:
            # every piece of the room (cut or not) relit
            ent = [f for f in os.listdir(d) if f.startswith(room) and f.endswith("_CullableEntries.inc.c")]
            names = re.findall(r"(\w+), // opa", open(os.path.join(d, ent[0])).read()) if ent else []
            lists = dict(areas)
            lists.update(pieces)  # (the cut pieces' own lists: the material cut's hold the piece's state)
            for n in names:
                if n != "NULL" and n not in pieces:
                    p = os.path.join(d, n + ".inc.c")
                    lists[n] = [c for c in split_items(open(p).read())]
                    pieces[n] = lists[n]
            room_arrays = {}

            def arrays_of(a):
                if a in area_arrays:
                    return area_arrays[a]
                if a not in room_arrays:
                    room_arrays[a] = load_vtx(d, a)
                return room_arrays[a]
            table, why = relight(lists, arrays_of, entry_lit)
            if table is None:
                sys.exit("meshpack: relighting %s failed: %s" % (room, why))
            for n in lists:
                if n in areas:
                    areas[n] = lists[n]
            if report:
                print("meshpack: %s: %d vertices lit by the CPU (segment 0x%02X)" % (room, len(table), RELIT_SEG))
        for dl, cmds in pieces.items():
            p = os.path.join(d, dl + ".inc.c")
            os.remove(p)  # (a hard link: written fresh)
            open(p, "w").write("".join("    %s,\n" % c for c in cmds))
        defs = []
        externs = []
        for vname, vtx in fade_arrays.pop(room, {}).items():
            externs.append("extern Vtx %s[];\n" % vname)
            defs.append("Vtx %s[] = {\n%s};\n\n" % (vname, "".join("    VTX(%s),\n" % ", ".join(v) for v in vtx)))
        for name, cmds in areas.items():
            vname = name + "_Vtx"
            externs.append("extern Vtx %s[];\nextern Gfx %s[];\n" % (vname, name))
            defs.append("Vtx %s[] = {\n%s};\n\n" % (vname, "".join(
                "    VTX(%s),\n" % ", ".join(v) for v in area_arrays[vname])))
            defs.append("Gfx %s[] = {\n%s};\n\n" % (name, "".join("    %s,\n" % c for c in cmds)))
        if table is not None:
            # the table the CPU lights from: its count, then its vertices (fishmod_relight.c finds it by this name)
            defs.append("Vtx %s_relitVtx[] = {\n%s};\n\n" % (room, "".join(
                "    VTX(%s),\n" % ", ".join(v) for v in table)))
            # each vertex's source in the room (identical copies share one: the same normal, the same shade): the CPU
            # reads the normal from there, as the game may have shaded it (fishmod_scatter.c's lake bed shading
            # shortens the normals of the room's own vertices)
            missing = [v for v in table if v not in ORIGIN]
            if missing:
                sys.exit("meshpack: %d relit vertices with no source in the room" % len(missing))
            defs.append("Vtx* %s_relitSrc[] = {\n%s};\n\n" % (room, "".join(
                "    &%s,\n" % ORIGIN[v] for v in table)))
            defs.append("void* %s_fishmodRelit[3] = { (void*)%d, %s_relitVtx, %s_relitSrc };\n" % (
                room, len(table), room, room))
        if not defs:
            continue
        rp = os.path.join(d, room + ".c")
        text = open(rp).read()
        os.remove(rp)
        # the declarations after the includes (the pieces' lists use the areas' names), the definitions at the end
        # (the data files are included inside each array's definition: the declarations go before the first one)
        first_def = re.search(r"^[A-Za-z_][\w ]*\*?\s+\w+\[\w*\]\s*=", text, re.M).start()
        text = text[:first_def] + "// fish-oot: tools/meshpack.py's areas\n" + "".join(externs) + "\n" + text[first_def:]
        text += "\n// fish-oot: tools/meshpack.py's areas\n" + "".join(defs)
        open(rp, "w").write(text)


if __name__ == "__main__":
    main()
