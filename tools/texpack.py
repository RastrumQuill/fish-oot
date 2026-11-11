#!/usr/bin/env python3
"""fish-oot: a lossless texture pass over the extracted assets, for the RDP (build.sh --texpack).

Every 16-bit RGBA texture with no more than 256 colours becomes an 8-bit colour-indexed one (CI8) with a palette of
exactly its colours: the RDP looks each texel up before filtering, so every pixel drawn is the same, while a load
moves about half the bytes (the palette comes on top: as many colours as it has). The display lists that load it are
rewritten to match:

- the load's format becomes CI 8-bit, and TMEM addresses halve with the sizes (everything then fits in TMEM's lower
  half, where CI textures must be: the palette sits in the upper half);
- before the material's loads: the palette's load, and the texture-lookup mode set to RGBA16; after the material
  (when the next one isn't converted, or at the list's end), the mode back to none, so nothing after it is looked up;
- textures drawn together in one material share one palette (TMEM holds one 256-colour palette): together they must
  stay within 256 colours, else they stay as they are;
- only where it gains: the textures at 8 bits a texel plus the palette and its load must come to fewer bytes than the
  16-bit textures (a small texture with many colours stays);
- a texture converts only if every use of its name anywhere is a load this pass rewrites (not one used by code, an
  animated-material table or a load it doesn't know), and a list only if it calls no other list and nothing skips its
  end (a cull before its first load is fine);
- a pipe sync with no drawing or loading since the previous one in the same list goes (the pipeline is already empty).

The original tree isn't touched: the output is a copy (hard links for every file the pass doesn't change), which the
build then reads (EXTRACTED_DIR).

Usage: tools/texpack.py SRC_TREE OUT_TREE [--only SUBDIR] [--report]
"""
import collections
import os
import re
import shutil
import struct
import sys
import zlib

from PIL import Image

DRAW = re.compile(r"gsSP(1Triangle|2Triangles|1Quadrangle|Tri|TextureRectangle|ScisTextureRectangle|Line3D)|"
                  r"gsDP(FillRectangle|LoadBlock|LoadTile|LoadTLUT)")
LOAD = ("gsDPLoadTextureBlock", "gsDPLoadMultiBlock")
CALLS = ("gsSPDisplayList", "gsSPBranchList", "gsSPBranchLessZ", "gsSPBranchLessZraw", "gsSPLoadUcode",
         "gsSPLoadUcodeL", "gsSPBranchLessZrg")
TEXDEF = re.compile(r"u64 (\w+)\[TEX_LEN\(u64, \1_WIDTH, \1_HEIGHT, 16\)\] = \{\n#include \"([^\"]+)\.rgba16\.inc\.c\"\n\};")
DLDEF = re.compile(r"Gfx (\w+)\[(\d+)\] = \{\n#include \"([^\"]+\.inc\.c)\"\n\};")
WORD = re.compile(r"\b\w+\b")
PALETTE_COST = 64  # bytes: the palette load's commands (texture image, tiles, the load, the lookup mode on and off)


def parse_commands(text):
    """[(name, args)] for a display list body: gsXxx(args), ... (args with nested parentheses)."""
    out = []
    i = 0
    n = len(text)
    while i < n:
        m = re.compile(r"(gs\w+)\(").search(text, i)
        if not m:
            break
        depth = 0
        j = m.end() - 1
        while j < n:
            if text[j] == "(":
                depth += 1
            elif text[j] == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        out.append((m.group(1), " ".join(text[m.end():j].split())))
        i = j + 1
    return out


def split_args(args):
    out, depth, cur = [], 0, ""
    for ch in args:
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
            continue
        depth += ch == "("
        depth -= ch == ")"
        cur += ch
    out.append(cur.strip())
    return out


def write_ci8_png(path, w, h, indices, palette):
    """A paletted PNG with exactly len(palette) PLTE and tRNS entries (build_from_png requires them to match)."""
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    raw = b"".join(b"\0" + bytes(indices[y * w:(y + 1) * w]) for y in range(h))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 3, 0, 0, 0))
    png += chunk(b"PLTE", b"".join(bytes(p[:3]) for p in palette))
    png += chunk(b"tRNS", bytes(p[3] for p in palette))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    tmp = path + ".tmp"
    open(tmp, "wb").write(png)
    os.replace(tmp, path)


def replace_file(path, text):
    """Write without touching the hard-linked original."""
    tmp = path + ".tmp"
    open(tmp, "w").write(text)
    os.replace(tmp, path)


class Load:
    def __init__(self, dl, index, name, args):
        self.dl, self.index, self.kind = dl, index, name
        self.args = split_args(args)
        self.tex = self.args[0]
        if name == "gsDPLoadTextureBlock":
            self.fmt, self.siz, self.tmem = self.args[1], self.args[2], "0"
        else:
            self.fmt, self.siz, self.tmem = self.args[3], self.args[4], self.args[1]

    def rgba16(self):
        return self.fmt == "G_IM_FMT_RGBA" and self.siz == "G_IM_SIZ_16b"


def main():
    args = sys.argv[1:]
    report = "--report" in args
    only = None
    if "--only" in args:
        only = args[args.index("--only") + 1]
        args = [a for a in args if a not in ("--only", only)]
    args = [a for a in args if a != "--report"]
    if len(args) != 2:
        sys.exit(__doc__)
    src, out = args
    assets = os.path.join(src, "assets")

    # --- every text file of the tree: names used, texture and list definitions ---
    texts = {}
    for root, _, files in os.walk(assets):
        for f in files:
            if f.endswith((".c", ".h")):
                p = os.path.join(root, f)
                texts[p] = open(p, errors="replace").read()
    uses = collections.Counter()
    for t in texts.values():
        uses.update(WORD.findall(t))
    for extra in ("src", "include", "assets", "fishmod"):  # the repository's own code and assets
        for root, _, files in os.walk(extra):
            for f in files:
                if f.endswith((".c", ".h")):
                    uses.update(WORD.findall(open(os.path.join(root, f), errors="replace").read()))
    texdefs = {}  # name -> (defining .c, include stem)
    dldefs = {}   # include path of a list body -> (list name, defining .c)
    for p, t in texts.items():
        if not p.endswith(".c"):
            continue
        for m in TEXDEF.finditer(t):
            texdefs[m.group(1)] = (p, m.group(2))
        for m in DLDEF.finditer(t):
            dldefs[m.group(3)] = (m.group(1), p)

    # --- the display lists and their loads ---
    lists = {}  # include path -> commands
    for inc in dldefs:
        p = os.path.join(src, inc)
        if p in texts and (only is None or any(inc.startswith(os.path.join("assets", o)) for o in only.split(","))):
            lists[inc] = parse_commands(texts[p])
    loads = []
    for inc, cmds in lists.items():
        for k, (name, a) in enumerate(cmds):
            if name in LOAD:
                loads.append(Load(inc, k, name, a))
    by_tex = collections.defaultdict(list)
    for ld in loads:
        by_tex[ld.tex].append(ld)

    # a texture can convert when it's defined as rgba16, has <= 256 colours and a width that's a multiple of 8, and
    # every use of its name is its definition, its extern, or a load (rgba16) in a list this pass reads
    colours = {}
    texels = {}
    eligible = set()
    for name, (cfile, stem) in texdefs.items():
        lds = by_tex.get(name)
        if not lds or not all(ld.rgba16() for ld in lds):
            continue
        if uses[name] != 3 + len(lds):  # (its definition, its data's #include, its extern, the loads)
            continue
        png = os.path.join(src, stem + ".rgba16.png")
        if not os.path.exists(png):
            continue
        im = Image.open(png).convert("RGBA")
        if im.width % 8:
            continue
        cs = set(im.getdata())
        if len(cs) <= 256:
            colours[name] = cs
            texels[name] = im.width * im.height
            eligible.add(name)

    # material groups: the loads between two draws, per list; a list qualifies if it calls nothing, and nothing
    # after its first load could end it early
    def groups(inc):
        cmds = lists[inc]
        out, cur = [], []
        for k, (name, a) in enumerate(cmds):
            if name in LOAD:
                cur.append(k)
            elif DRAW.match(name) and cur:
                out.append(cur)
                cur = []
        if cur:
            out.append(cur)
        return out

    def list_ok(inc):
        cmds = lists[inc]
        first = next((k for k, (n, _) in enumerate(cmds) if n in LOAD), None)
        for k, (name, _) in enumerate(cmds):
            if name in CALLS or name.startswith("gsSPBranch") or name in ("gsDPSetTile", "gsDPSetTextureImage"):
                return False
            if name == "gsSPCullDisplayList" and first is not None and k > first:
                return False
        return True

    ok_lists = {inc for inc in lists if list_ok(inc)}
    group_of = {}
    for inc in lists:
        for g in groups(inc):
            for k in g:
                group_of[(inc, k)] = g
    # settle: a texture stays only if every group it's loaded in converts whole (all its textures convert, the list
    # qualifies, its TMEM addresses halve exactly), and each shared palette fits in 256 colours
    while True:
        changed = False
        for name in sorted(eligible):
            for ld in by_tex[name]:
                g = group_of[(ld.dl, ld.index)]
                g_loads = [Load(ld.dl, k, *lists[ld.dl][k]) for k in g]
                bad = ld.dl not in ok_lists or any(x.tex not in eligible for x in g_loads)
                for x in g_loads:
                    try:
                        bad |= int(x.tmem, 0) % 2 != 0
                    except ValueError:
                        bad = True
                if bad:
                    eligible.discard(name)
                    changed = True
                    break
        # palettes: textures loaded together share one
        parent = {t: t for t in eligible}

        def find(t):
            while parent[t] != t:
                parent[t] = parent[parent[t]]
                t = parent[t]
            return t
        for name in eligible:
            for ld in by_tex[name]:
                for k in group_of[(ld.dl, ld.index)]:
                    other = Load(ld.dl, k, *lists[ld.dl][k]).tex
                    if other in parent:
                        parent[find(other)] = find(name)
        comps = collections.defaultdict(list)
        for t in eligible:
            comps[find(t)].append(t)
        for members in comps.values():
            n_colours = len(set().union(*(colours[t] for t in members)))
            # only where it gains: the textures at 8 bits a texel, plus the palette (2 bytes a colour) and its load's
            # commands (about 64 bytes), against 16 bits a texel now; a small texture with many colours stays
            gains = sum(texels[t] for t in members) + 2 * n_colours + PALETTE_COST < sum(2 * texels[t] for t in members)
            if n_colours > 256 or not gains:
                for t in members:
                    eligible.discard(t)
                changed = True
        if not changed:
            break

    if os.environ.get("TEXPACK_CI") == "0":  # (for bisecting: the display-list pass alone)
        eligible = set()
    for skip in filter(None, os.environ.get("TEXPACK_SKIP", "").split(",")):  # (for bisecting: textures kept)
        eligible.discard(skip)
    parent = {t: t for t in eligible}

    def find(t):
        while parent[t] != t:
            parent[t] = parent[parent[t]]
            t = parent[t]
        return t
    for name in eligible:
        for ld in by_tex[name]:
            for k in group_of[(ld.dl, ld.index)]:
                other = Load(ld.dl, k, *lists[ld.dl][k]).tex
                parent[find(other)] = find(name)
    members_of = collections.defaultdict(list)
    for t in sorted(eligible):
        members_of[find(t)].append(t)
    palette_of = {}  # texture -> (palette array name, palette)
    for members in members_of.values():
        pal = sorted(set().union(*(colours[t] for t in members)))
        by_file = collections.defaultdict(list)
        for t in sorted(members):
            by_file[texdefs[t][0]].append(t)
        for in_file in by_file.values():  # (a copy in each file: a static texture's palette can't be shared)
            for t in in_file:
                palette_of[t] = (in_file[0] + "_TPal", pal)

    total_rgba = sum(1 for n in texdefs if by_tex.get(n))
    print(f"texpack: {len(eligible)} of {len(texdefs)} rgba16 textures convert ({total_rgba} loaded by lists read), "
          f"{len(members_of)} palettes")
    if report:
        return

    # --- the output tree: hard links, then the changed files replaced ---
    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(src, out, copy_function=os.link)

    changed_text = {}  # source path -> new text

    def text_of(p):
        return changed_text.get(p, texts[p])

    # textures: the PNG, the definition (and its extern), and the palette array next to the first member
    pal_defined = set()
    for name in sorted(eligible):
        cfile, stem = texdefs[name]
        png = os.path.join(src, stem + ".rgba16.png")
        im = Image.open(png).convert("RGBA")
        pal_name, pal = palette_of[name]
        index = {c: i for i, c in enumerate(pal)}
        write_ci8_png(os.path.join(out, stem + ".ci8.png"), im.width, im.height,
                      [index[c] for c in im.getdata()], pal)
        os.unlink(os.path.join(out, stem + ".rgba16.png"))
        old = (f"u64 {name}[TEX_LEN(u64, {name}_WIDTH, {name}_HEIGHT, 16)] = {{\n"
               f"#include \"{stem}.rgba16.inc.c\"\n}};")
        new = (f"u64 {name}[TEX_LEN(u64, {name}_WIDTH, {name}_HEIGHT, 8)] = {{\n"
               f"#include \"{stem}.ci8.inc.c\"\n}};")
        t = text_of(cfile)
        assert old in t, (name, cfile)
        storage = "static " if re.search(rf"static u64 {name}\[", t) else ""
        pal_words = (len(pal) + 3) // 4
        if pal_name not in pal_defined:
            pal_defined.add(pal_name)
            # (the match starts after any "static ": the palette takes it, and the texture gets its own back)
            new = (f"u64 {pal_name}[{pal_words}] = {{\n#include \"{stem}.tlut.rgba16.inc.c\"\n}};\n\n{storage}") + new
        changed_text[cfile] = t.replace(old, new, 1)
        hfile = cfile[:-2] + ".h"
        ht = text_of(hfile) if (hfile in texts or hfile in changed_text) else ""
        decl = re.search(rf"(static |extern )?u64 {name}\[TEX_LEN\(u64, {name}_WIDTH, {name}_HEIGHT, 16\)\];", ht)
        if decl is not None:
            prefix = decl.group(1) or ""
            new_h = f"{prefix}u64 {name}[TEX_LEN(u64, {name}_WIDTH, {name}_HEIGHT, 8)];"
            pal_decl = f"{prefix}u64 {pal_name}[{pal_words}];"
            if pal_decl not in ht:
                new_h += "\n" + pal_decl
            changed_text[hfile] = ht.replace(decl.group(0), new_h, 1)

    # display lists
    syncs_removed = 0
    lists_changed = 0
    for inc, cmds in lists.items():
        conv_groups = []
        for g in groups(inc):
            texs = [Load(inc, k, *cmds[k]).tex for k in g]
            if all(t in eligible for t in texs):
                conv_groups.append(g)
            else:
                assert not any(t in eligible for t in texs), (inc, texs)
        all_groups = groups(inc)
        conv = {g[0] for g in conv_groups}
        # each command belongs to a material: the next group's, if no draw comes between them, else the last group's
        owner = [None] * len(cmds)
        nxt = None
        for k in range(len(cmds) - 1, -1, -1):
            name = cmds[k][0]
            starts = [g for g in all_groups if g[0] == k]
            if starts:
                nxt = starts[0][0]
            if DRAW.match(name):
                nxt = None
            owner[k] = nxt
        last = None
        for k in range(len(cmds)):
            if owner[k] is None:
                owner[k] = last
            else:
                last = owner[k]
        own_lut_before_load = set()  # groups whose own setup sets the lookup mode before their first load
        for k, (name, _) in enumerate(cmds):
            if name == "gsDPSetTextureLUT" and owner[k] is not None and k < owner[k]:
                own_lut_before_load.add(owner[k])
        new = []
        drawn = True
        ours = False  # a converted material set RGBA16 here and it isn't put back yet

        def emit(name, a):
            nonlocal drawn
            if name == "gsDPPipeSync":
                drawn = False
            elif DRAW.match(name) or name in LOAD:
                drawn = True
            new.append((name, a))

        def restore():
            nonlocal ours
            if drawn:
                emit("gsDPPipeSync", "")
            emit("gsDPSetTextureLUT", "G_TT_NONE")
            ours = False

        for k, (name, a) in enumerate(cmds):
            g0 = owner[k]
            if name == "gsSPEndDisplayList" and ours:
                restore()
            if k == g0:  # a material's first load
                if g0 in conv:
                    if g0 not in own_lut_before_load:
                        if drawn:
                            emit("gsDPPipeSync", "")
                        emit("gsDPSetTextureLUT", "G_TT_RGBA16")
                    pal_name, pal = palette_of[Load(inc, k, name, a).tex]
                    emit("gsDPLoadTLUT", f"{len(pal)}, 256, {pal_name}")
                    ours = True
                elif ours and g0 not in own_lut_before_load:
                    restore()
            if name == "gsDPSetTextureLUT" and g0 in conv:
                emit(name, "G_TT_RGBA16")
                ours = True
                continue
            if name == "gsDPSetTextureLUT":
                ours = False
            if name in LOAD and g0 in conv:
                ld = Load(inc, k, name, a)
                parts = list(ld.args)
                if name == "gsDPLoadTextureBlock":
                    parts[1], parts[2] = "G_IM_FMT_CI", "G_IM_SIZ_8b"
                else:
                    parts[1] = hex(int(ld.tmem, 0) // 2)
                    parts[3], parts[4] = "G_IM_FMT_CI", "G_IM_SIZ_8b"
                emit(name, ", ".join(parts))
                continue
            emit(name, a)
        # the pipe syncs with nothing drawn or loaded since the last one (not the first: drawing came before)
        final = []
        busy = True
        for name, a in new:
            if name == "gsDPPipeSync":
                if not busy and os.environ.get("TEXPACK_SYNC") != "0":
                    syncs_removed += 1
                    continue
                busy = False
            elif DRAW.match(name) or name in LOAD:
                busy = True
            final.append((name, a))
        if final == [tuple(c) for c in cmds]:
            continue
        lists_changed += 1
        body = "".join(f"    {n}({a}),\n" for n, a in final)
        replace_file(os.path.join(out, inc), body)
        dl_name, cfile = dldefs[inc]
        t = text_of(cfile)
        t2 = re.sub(rf"Gfx {dl_name}\[\d+\] = \{{", f"Gfx {dl_name}[] = {{", t, count=1)
        changed_text[cfile] = t2
        hfile = cfile[:-2] + ".h"
        if hfile in texts:
            changed_text[hfile] = re.sub(rf"extern Gfx {dl_name}\[\d+\];", f"extern Gfx {dl_name}[];",
                                         text_of(hfile), count=1)
    for p, t in changed_text.items():
        replace_file(os.path.join(out, os.path.relpath(p, src)), t)
    print(f"texpack: {lists_changed} display lists rewritten, {syncs_removed} empty pipe syncs removed, "
          f"{len(changed_text)} C files changed")


if __name__ == "__main__":
    main()
