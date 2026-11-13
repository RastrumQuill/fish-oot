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
import re
import shutil
import sys

# the pieces to cut: (scene directory, room file stem, display lists); the area size
TARGETS = [
    ("assets/scenes/overworld/spot06", "spot06_room_0",
     ["gSpot06DL_00A400_03003270_DL", "gSpot06DL_00A400_03001240_DL", "gSpot06DL_00A400_03003EF8_DL",
      "gSpot06DL_00A400_03004AF0_DL"]),
]
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
              "gsDPLoadTLUT_pal16", "gsDPLoadTLUT_pal256", "gsDPLoadTextureBlock_4b"}


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
    first = tris[0][0]
    early = {state_key(c) for c in stream[:first]}
    for c in stream[first:]:
        k = state_key(c)
        if k is not None and k not in early:
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
    sub_dls, sub_vtx, new_dl = [], [], []
    emitted = 0
    for mi, (k, areas) in enumerate(by_state.items()):
        new_dl.extend(stream[emitted:k])
        emitted = k
        for ai in sorted(areas):
            ts = areas[ai]
            name = "%s_mp%d_%d" % (dl, ai, mi)
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
            new_dl.append("gsSPDisplayList(%s)" % name)
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
        return None, "check: the triangles differ"
    # each area list's lighting where it's called (G_LIGHTING as the piece's state has it there)
    lit_at, lit = {}, None
    for j, c in enumerate(stream + [None]):
        lit_at[j] = lit
        if c is not None and name_of(c) in ("gsSPSetGeometryMode", "gsSPClearGeometryMode") and \
                "G_LIGHTING" in [x.strip() for x in args_of(c)[0].split("|")]:
            lit = name_of(c) == "gsSPSetGeometryMode"
    entry_lit = {}
    for mi, (k, areas) in enumerate(by_state.items()):
        for ai in areas:
            entry_lit["%s_mp%d_%d" % (dl, ai, mi)] = lit_at[k]
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


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    src, out = sys.argv[1], sys.argv[2]
    report = "--report" in sys.argv
    if os.path.exists(out):
        shutil.rmtree(out)
    # a copy of the tree of hard links (the files changed below are written fresh, breaking their link)
    shutil.copytree(src, out, copy_function=os.link)
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
                print("meshpack: %s: %d triangles in %d areas, %d area lists" % (dl, ntri, nareas, len(area_dls)))
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
