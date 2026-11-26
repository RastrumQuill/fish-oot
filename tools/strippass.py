#!/usr/bin/env python3
"""fish-oot: the scenes' triangles as F3DEX3's triangle snakes (build.sh --strips), for a console A/B of whether strips
pay off where the frame waits on the RDP (Lake Hylia: the RSP stalls on the RDP's full FIFO there, so its extra work
for a snake may hide in that wait while the RDRAM a snake saves goes to the RDP).

Every run of triangle commands in the game's display lists (the scenes' rooms, every object's models, the actors'
own; the objects the mod reads triangles from at run time left alone: SKIP_DIRS) (between vertex loads and state changes) is drawn as snakes: a
chain of triangles each joined to the one before by an edge, 1 byte a triangle after the first three (8 to 16 bytes
for 5 triangles, against 8 bytes for every 2 in gsSP2Triangles). A chain of 1 or 2 triangles stays as it was (a snake
of 2 is no smaller). Each triangle keeps its own winding; the order within a run changes, so lists with a
translucent render mode are left alone (their draw order matters). Everything else in the lists is as it was. The
lists the mesh pass writes into the rooms' .c files (the lake's areas) are done too.

Usage: tools/strippass.py SRC_TREE OUT_TREE [--report]
"""
import os
import re
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import meshpack as mp  # noqa: E402

TRI_CMDS = ("gsSP1Triangle", "gsSP2Triangles", "gsSP1Quadrangle")
# models whose triangles the mod reads at run time, which only knows the plain triangle commands: the fish rig
# (fishmod_rig.c: object_fish's fish, gameplay_keep's), the plants that bend (fishmod_scatter.c: object_fish's reed)
SKIP_DIRS = {"object_fish", "gameplay_keep"}
UNSIZED = []  # (directory, list name): lists whose length changed
STATS = {"tris": 0, "snaked": 0, "snakes": 0, "bytes_before": 0, "bytes_after": 0, "lists": 0}


def tris_of(c):
    n = mp.name_of(c)
    a = [int(x, 0) for x in mp.args_of(c)]
    if n == "gsSP1Triangle":
        return [tuple(a[0:3])]
    if n == "gsSP2Triangles":
        return [tuple(a[0:3]), tuple(a[4:7])]
    return [(a[0], a[1], a[2]), (a[0], a[2], a[3])]


def rotations(t):
    return [t, (t[1], t[2], t[0]), (t[2], t[0], t[1])]


def same_tri(t, u):
    return u in rotations(t)


def grow(start, unused):
    """the longest greedy chain from start (A, B, C as the snake holds them): [(index, direction)] after the first"""
    a, b, c = start
    seq = []
    used = set()
    while True:
        nxt = None
        for k in unused:
            if k in used:
                continue
            t = unused[k]
            for direction in (0, 1):  # 0 right: (n, A, C); 1 left: (n, B, A)
                e1, e2 = (a, c) if direction == 0 else (b, a)
                for r in rotations(t):
                    if r[1] == e1 and r[2] == e2 and r[0] not in (e1, e2):
                        nxt = (k, r[0], direction)
                        break
                if nxt:
                    break
            if nxt:
                break
        if nxt is None:
            return seq, used
        k, n, direction = nxt
        used.add(k)
        seq.append((n, direction, k))
        if direction == 0:
            b = a
        else:
            c = a
        a = n


def snakes(tris):
    """the run's triangles as chains: [(first triangle as (i1, i2, i3), [(index, direction)])]"""
    unused = {k: t for k, t in enumerate(tris)}
    out = []
    while unused:
        best = None
        for k0, t in list(unused.items()):
            rest = {k: u for k, u in unused.items() if k != k0}
            for r in rotations(t):
                # the snake draws A-B-C = i3-i1-i2 first: A, B, C a rotation of the triangle
                seq, used = grow(r, rest)
                if best is None or len(seq) > len(best[2]):
                    best = (k0, r, seq)
            if best and len(best[2]) >= 8:
                break
        k0, (A, B, C), seq = best
        del unused[k0]
        for _, _, k in seq:
            del unused[k]
        out.append(((B, C, A), [(n, d) for n, d, _ in seq]))
    return out


def encode(first, seq):
    """gsSPTriSnake (+ gsSPContinueSnake) commands for one chain"""
    i1, i2, i3 = first
    idx = [(i3, None)] + list(seq)  # (i3's direction is the macro's own)
    # the last valid index carries G_SNAKE_LAST
    last = len(idx) - 1
    vals = []
    for j, (i, d) in enumerate(idx):
        vals.append(("%d | G_SNAKE_LAST" % i) if j == last else str(i))
    dirs = [None] + ["G_SNAKE_LEFT" if d else "G_SNAKE_RIGHT" for _, d in seq]
    head = [vals[0]]
    rest = list(zip(vals[1:], dirs[1:]))
    cmds = []
    first4 = rest[:4] + [("0", "0")] * (4 - len(rest[:4]))
    cmds.append("gsSPTriSnake(%d, %d, %s, %s)" % (i1, i2, head[0], ", ".join("%s, %s" % p for p in first4)))
    rest = rest[4:]
    while rest:
        chunk = rest[:8] + [("0", "0")] * (8 - len(rest[:8]))
        cmds.append("gsSPContinueSnake(%s)" % ", ".join("%s, %s" % p for p in chunk))
        rest = rest[8:]
    return cmds


def convert(cmds):
    """the list's commands with its triangle runs as snakes; None if it's left alone"""
    for c in cmds:
        if "(" in c and mp.name_of(c) in ("gsDPSetRenderMode",) and "XLU" in c:
            return None
        if "(" in c and mp.name_of(c) in ("gsSPTriSnake", "gsSPContinueSnake"):
            return None
    out, run = [], []

    def flush():
        if not run:
            return
        tris = [t for c in run for t in tris_of(c)]
        chains = snakes(tris)
        small = []
        for first, seq in chains:
            STATS["tris"] += 1 + len(seq)
            if len(seq) >= 2:
                out.extend(encode(first, seq))
                STATS["snaked"] += 1 + len(seq)
                STATS["snakes"] += 1
            else:
                i1, i2, i3 = first
                small.append((i3, i1, i2))
                if seq:
                    n, d = seq[0]
                    # (the second triangle as the snake would have drawn it: right (n, i3, i2), left (n, i1, i3))
                    small.append((n, i3, i2) if d == 0 else (n, i1, i3))
        for k in range(0, len(small) - 1, 2):
            out.append("gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0)" % (small[k] + small[k + 1]))
        if len(small) % 2:
            out.append("gsSP1Triangle(%d, %d, %d, 0)" % small[-1])
        STATS["bytes_before"] += 8 * len(run)
        run.clear()

    for c in cmds:
        if "(" in c and mp.name_of(c) in TRI_CMDS:
            run.append(c)
            continue
        flush()
        out.append(c)
    flush()
    STATS["bytes_after"] += 8 * sum(1 for c in out if "Triangle" in c or "Snake" in c)
    return out


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    src, out = sys.argv[1], sys.argv[2]
    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(src, out, copy_function=os.link)
    base = os.path.join(out, "assets")
    for root, dirs, files in os.walk(base):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for f in files:
            p = os.path.join(root, f)
            if f.endswith(".inc.c") and "Vtx" not in f:
                text = open(p).read()
                if ("gsSP2Triangles" not in text and "gsSP1Triangle" not in text) or "#include" in text:
                    continue
                items = mp.split_items(text)
                if not all(("(" in c) and c.lstrip().startswith("gs") for c in items):
                    continue  # (not a display list on its own)
                new = convert(items)
                if new is None or new == items:
                    continue
                os.remove(p)
                open(p, "w").write("".join("    %s,\n" % c for c in new))
                STATS["lists"] += 1
                UNSIZED.append((root, f[:-len(".inc.c")]))
            elif re.search(r"_room_\d+\.c$", f):
                text = open(p).read()
                if "gsSP2Triangles" not in text:
                    continue

                def sub(m):
                    if "Triangle" not in m.group(2) or "#include" in m.group(2):
                        return m.group(0)
                    new = convert(mp.split_items(m.group(2)))
                    if new is None:
                        return m.group(0)
                    STATS["lists"] += 1
                    return m.group(1) + "\n" + "".join("    %s,\n" % c for c in new) + "};"
                new_text = re.sub(r"(Gfx \w+\[\w*\] = \{)(.*?)\};", sub, text, flags=re.S)
                if new_text != text:
                    os.remove(p)
                    open(p, "w").write(new_text)
    # the changed lists' declared lengths (the .c and .h beside them give each its original length) left to their contents
    by_dir = {}
    for d, name in UNSIZED:
        by_dir.setdefault(d, set()).add(name)
    for d, names in by_dir.items():
        pat = re.compile(r"\b(%s)\[\d+\]" % "|".join(re.escape(n) for n in sorted(names, key=len, reverse=True)))
        for f in os.listdir(d):
            if f.endswith((".c", ".h")) and not f.endswith(".inc.c"):
                p = os.path.join(d, f)
                text = open(p).read()
                new = pat.sub(lambda m: m.group(1) + "[]", text)
                if new != text:
                    os.remove(p)
                    open(p, "w").write(new)
    s = STATS
    print("strippass: %d lists; %d of %d triangles in %d snakes (%.1f a snake); triangle commands %d -> %d bytes" % (
        s["lists"], s["snaked"], s["tris"], s["snakes"], s["snaked"] / max(1, s["snakes"]),
        s["bytes_before"], s["bytes_after"]))


if __name__ == "__main__":
    main()
