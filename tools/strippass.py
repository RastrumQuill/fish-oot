#!/usr/bin/env python3
"""fish-oot: the scenes' triangles as F3DEX3's triangle snakes (build.sh --strips), for a console A/B of whether strips
pay off where the frame waits on the RDP (Lake Hylia: the RSP stalls on the RDP's full FIFO there, so its extra work
for a snake may hide in that wait while the RDRAM a snake saves goes to the RDP).

Every run of triangle commands in the game's display lists (the scenes' rooms, every object's models, the actors'
own; the objects the mod reads triangles from at run time left alone: SKIP_DIRS) (between vertex loads and state changes) is drawn as snakes: a
chain of triangles each joined to the one before by an edge, 1 byte a triangle after the first three (8 to 16 bytes
for 5 triangles, against 8 bytes for every 2 in gsSP2Triangles). A chain of 1 or 2 triangles stays as it was (a snake
of 2 is no smaller). Each triangle is drawn exactly as the game drew it, its corners in the same order (a triangle the near plane cuts
is drawn as a fan from its first corner, and its shading follows where the fan starts: a strip that turned them
shaded the floor at the camera's feet differently). Where the depth buffer settles what's seen (an opaque render
mode that compares and writes depth) the order within a run changes freely; where the draw order shows (a render mode
that blends with what's drawn already, or one that doesn't write depth: translucent surfaces, decals, effects; or a
render mode the pass can't tell, see order_class) a run keeps its triangles in the game's order, and a snake joins
only triangles that follow one another in it. Everything else in the lists is as it was. The lists the mesh pass
writes into the rooms' .c files (the lake's areas) are done too.

tools/strippass.py --selftest: random runs through both orders (triangle set, winding and first corner kept; the
order kept where it has to be).

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
# STRIPPASS_TURN_ALL=1: every triangle may be turned to join a strip (the most strips; the floor at the camera's feet
# shaded a little differently where the near plane cuts it)
TURN_ALL = bool(os.environ.get("STRIPPASS_TURN_ALL"))
# How alike a triangle's corners must shade (colour or normal, alpha: the most any one channel differs, of 255) for it
# to be turned: the fan's start then shifts a cut triangle's blend by about this much at most
SHADE_TOL = int(os.environ.get("STRIPPASS_SHADE_TOL", "8"))
# A triangle with no edge longer than this (units) may be turned whatever its shading: the near plane hardly ever cuts
# one so small, and if it does, only a sliver of it
SMALL_EDGE = int(os.environ.get("STRIPPASS_SMALL_EDGE", "64"))
# Each model's own scale (its units to the world's), for the size test: an object's or an actor's models are drawn at
# the scale its actors give them (Actor_SetScale or an init chain's scale, the largest of them: the cautious one); a
# room's are the world's own. Not found: 1 (no triangle counts as small that isn't small in its own units).
KNOWN_SCALE = {"object_link_boy": 0.01, "object_link_child": 0.01}  # (Player's, set in its code, not in a profile)


def model_scales(root):
    """{asset directory name: scale} from the actors' sources"""
    import glob

    out = dict(KNOWN_SCALE)
    for f in glob.glob(os.path.join(root, "src", "overlays", "actors", "*", "*.c")):
        t = open(f, errors="ignore").read()
        sc = [float(x.rstrip("f")) for x in re.findall(r"Actor_SetScale\([^,]+,\s*([0-9.]+)f?\s*\)", t)]
        sc += [int(x) / 1000 for x in re.findall(r"ICHAIN_VEC3F_DIV1000\(scale,\s*(\d+)", t)]
        sc = [x for x in sc if x > 0]  # (a scale of 0: an actor growing in from nothing)
        if not sc:
            continue
        m = re.search(r"ActorProfile\s+\w+\s*=\s*\{([^}]*)\}", t, re.S)
        names = [o.lower() for o in re.findall(r"OBJECT_[A-Z0-9_]+", m.group(1))] if m else []
        names.append(os.path.basename(os.path.dirname(f)))  # (the actor's own models: assets/overlays/ovl_X)
        for n in names:
            if n not in KNOWN_SCALE:
                out[n] = max(out.get(n, 0.0), max(sc))
    return out


SCALES = {}

# ---- which runs' draw order shows ----
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FREE, ORDERED = "free", "ordered"


class _Gbi(dict):
    """the render mode macros of gbi.h, worked out on demand"""

    def __init__(self):
        super().__init__()
        text = open(os.path.join(ROOT, "include", "ultra64", "gbi.f3dex3.h")).read().replace("\\\n", " ")
        self.obj, self.fn = {}, {}
        for m in re.finditer(r"^#define[ \t]+(\w+)(\(clk\))?[ \t]+(.+)$", text, re.M):
            (self.fn if m.group(2) else self.obj).setdefault(m.group(1), re.sub(r"/\*.*?\*/", "", m.group(3)).strip())
        self["GBL_c1"] = lambda a, b, c, d: a << 30 | b << 26 | c << 22 | d << 18
        self["GBL_c2"] = lambda a, b, c, d: a << 28 | b << 24 | c << 20 | d << 16

    def __missing__(self, k):
        if k in self.fn:
            body = self.fn[k]
            v = lambda clk: eval(body.replace("GBL_c##clk", "GBL_c%d" % clk), {}, self)  # noqa: E731
        elif k in self.obj:
            v = eval(self.obj[k], {}, self)
        else:
            raise KeyError(k)
        self[k] = v
        return v


_GBI = None
_MODE_CACHE = {}


def mode_class(expr):
    """a render mode (the two cycles' words, or'ed) -> FREE (the depth buffer settles what's seen: it compares and
    writes depth, and doesn't blend with what's drawn already) or ORDERED (anything else, or not worked out)"""
    global _GBI
    if expr not in _MODE_CACHE:
        if _GBI is None:
            _GBI = _Gbi()
        try:
            v = eval(expr.replace("\n", " "), {}, _GBI)
        except Exception:
            v = None
        if v is None:
            _MODE_CACHE[expr] = ORDERED
        else:
            zb = (v & 0x30) == 0x30  # Z_CMP | Z_UPD
            # blends with the frame buffer: forced blending with the memory colour an input (either cycle's setting:
            # which one acts is the cycle type's, not known here)
            mem = any(((v >> sh) & 3) == 1 for sh in (30, 28, 22, 20))
            blend = bool(v & 0x4000) and mem
            _MODE_CACHE[expr] = FREE if (zb and not blend) else ORDERED
    return _MODE_CACHE[expr]


def cmd_mode(c):
    """the render mode class a command sets (None: it sets none)"""
    n = name_of(c)
    if n == "gsDPSetRenderMode":
        return mode_class(" | ".join("(%s)" % a for a in mp.args_of(c)))
    if n in ("gsDPSetOtherMode", "gsSPSetOtherMode", "gsSPSetOtherModeLo", "gsDPSetOtherModeL"):
        return mode_class("(%s)" % mp.args_of(c)[-1]) if n == "gsDPSetOtherMode" else ORDERED
    return None


def name_of(c):
    return mp.name_of(c) if "(" in c else ""


DUPLICATE = set()  # names more than one list has (two versions' or scenes' own): never trusted
LISTS = {}      # list name -> its commands (every display list of the tree, before the pass)
_ENTRY = {}     # list name -> the class it's entered in
_EXIT = {}      # list name -> the class it leaves (None: as entered)
SRC_REFS = {}   # list name -> set of FREE/ORDERED/None: how the game's code draws it
_CALLERS = None


# ---- what a segment's list does to the render mode (the game's code builds it: gSPSegment) ----
SRC_FILES = {}   # list name -> the source files that name it
_SRC = {}        # source file -> its text
_FUNCS = None    # function name -> its body
_SEG_CACHE = {}
_OBJ_FILES = {}  # an object's name (OBJECT_X) -> the source files that name it
SETS_MODE = re.compile(r"RenderMode|OtherMode|SetupDL|gSPDisplayList|gSPBranchList|func_8009|ACTOR_SETUP|gActorSetup")
NOT_A_LIST = re.compile(r"^(SEGMENTED_TO_VIRTUAL|MATRIX_FINALIZE|NULL)$|(Vtx|Mtx|mtx|mtx2|Tex|tex|[mM]atrix|Texture)$")
PLAIN_LISTS = {"gEmptyDL", "sEmptyDL", "gCullBackDList", "gCullFrontDList", "cullDList"}  # (the code's own; no render mode)
SEGMENT_SET = re.compile(r"gSPSegment\(\s*[^,]+,\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*\(?\s*(?:\([^()]*\)\s*)?&?([A-Za-z_]\w*)")


def _functions():
    global _FUNCS
    if _FUNCS is None:
        _FUNCS = {}
        for f, text in _SRC.items():
            spans = _SPANS[id(text)] = []
            for m in re.finditer(r"^[A-Za-z_][\w \t\*]*?\b(\w+)\([^;{}]*\)\s*\{\n(.*?)^\}", text, re.S | re.M):
                # (a name two files define: both bodies)
                _FUNCS[m.group(1)] = _FUNCS.get(m.group(1), "") + m.group(2)
                spans.append((m.start(), m.end(), m.group(2)))
    return _FUNCS


_SPANS = {}
LOCAL_SETS_MODE = re.compile(r"RenderMode|OtherMode|SetupDL|func_8009|ACTOR_SETUP|gActorSetup")


def local_neutral(text, at, what):
    """a list the function at `at` builds in a local of its own (Gfx* what): True if neither the function nor the
    functions it hands the local to set a render mode anywhere"""
    _functions()
    for a, b, body in _SPANS.get(id(text), ((0, len(text), text),)):  # (a function's body on its own: all of it)
        if a <= at < b:
            if not re.search(r"\bGfx\s*\*\s*%s\b|\bGfx\s+%s\s*\[" % (what, what), body):
                return False
            if LOCAL_SETS_MODE.search(body):
                return False
            for m in re.finditer(r"\b(\w+)\(([^;]*?\b%s\b[^;]*?)\)\s*;" % what, body):
                g = m.group(1)
                if g in _FUNCS and LOCAL_SETS_MODE.search(_FUNCS[g]):
                    return False
            return True
    return False


def _scene_config(scene):
    """the body of the draw config function of a scene (by its directory's name), '' if not found"""
    try:
        table = open(os.path.join(ROOT, "include", "tables", "scene_table.h")).read()
    except OSError:
        return ""
    m = re.search(r"DEFINE_SCENE\(\s*%s_scene\s*,[^,]*,[^,]*,\s*(SDC_\w+)" % re.escape(scene), table)
    text = _SRC.get(os.path.join(ROOT, "src", "code", "z_scene_table.c"), "")
    if not m:
        return ""
    fn = re.search(r"^\s*(\w+),\s*// %s\s*$" % m.group(1), text, re.M)
    return _functions().get(fn.group(1), "") if fn else ""


def segment_neutral(dirpath, seg):
    """True if a call of segment `seg`'s list from a list of this asset directory is known to leave the render mode as
    it is: every list the code that draws the directory's models puts in the segment is one that sets none (a texture
    scroll, a colour, an empty list). Not known: False."""
    key = (dirpath, seg)
    if key in _SEG_CACHE:
        return _SEG_CACHE[key]
    res = False
    if dirpath is not None:
        parts = dirpath.replace("\\", "/").split("/")
        texts = []
        if "scenes" in parts:
            # a room: its scene's draw config (and the mod's own, for the scenes it draws into)
            scene = parts[-1]
            texts.append(_scene_config(scene))
            texts += [t for f, t in _SRC.items() if os.path.basename(f).startswith("fishmod")]
        else:
            # an object's or an actor's models: every source file that names one of the directory's lists, skeletons
            # or the object itself
            names = {f.split(".")[0] for f in os.listdir(dirpath)} if os.path.isdir(dirpath) else set()
            files = set()
            for n in names:
                files |= SRC_FILES.get(n, set())
            if not _OBJ_FILES:
                for f, t in _SRC.items():
                    for w in set(re.findall(r"\b(?:OBJECT|GAMEPLAY)_[A-Z0-9_]+\b", t)):
                        _OBJ_FILES.setdefault(w, set()).add(f)
            files |= _OBJ_FILES.get(parts[-1].upper(), set())
            texts += [_SRC[f] for f in sorted(files)]
        found, ok = 0, True
        for t in texts:
            for m in SEGMENT_SET.finditer(t):
                if int(m.group(1), 0) != seg:
                    continue
                found += 1
                what = m.group(2)
                if NOT_A_LIST.search(what) or what in PLAIN_LISTS:
                    continue
                if what in LISTS and what not in DUPLICATE:
                    ok = ok and exit_class(what) is None
                elif what in _functions():
                    ok = ok and not SETS_MODE.search(_functions()[what])
                else:
                    ok = ok and local_neutral(t, m.start(), what)
        res = bool(found) and ok
    _SEG_CACHE[key] = res
    return res


LIST_DIR = {}  # list name -> its directory
MID_ENTRY = {}  # list name -> the furthest command the game's code enters it at (&list[n]: a count of Gfx; the
                # list's commands as written here are no more than that many, so the first n of them cover it)


def after_call(c, st, dirpath):
    """the render mode class after a call command (st before it); None: c is no call"""
    t = call_target(c)
    if t is None:
        return None
    if t == "":
        a = mp.args_of(c)[0].strip()
        try:
            seg = int(a, 0) >> 24
        except ValueError:
            return ORDERED
        return st if (seg and segment_neutral(dirpath, seg)) else ORDERED
    e = exit_class(t)
    if e == "?":
        return ORDERED
    return st if e is None else e


def call_target(c):
    """the list a command calls or branches to by name ('' a segment address or the like; None: no call)"""
    if name_of(c) not in ("gsSPDisplayList", "gsSPBranchList", "gsSPBranchLessZraw"):
        return None
    a = mp.args_of(c)[0].strip()
    return a if (a in LISTS and a not in DUPLICATE) else ""


def exit_class(name, seen=()):
    """the render mode class a list leaves behind: FREE/ORDERED, None as it was entered, '?' not known"""
    if name in _EXIT:
        return _EXIT[name]
    if name in seen:
        return "?"
    st = None
    for c in LISTS[name]:
        m = cmd_mode(c)
        if m is not None:
            st = m
            continue
        t = call_target(c)
        if t is None:
            continue
        if t == "":
            a = mp.args_of(c)[0].strip()
            try:
                seg = int(a, 0) >> 24
            except ValueError:
                seg = 0
            if not (seg and segment_neutral(LIST_DIR.get(name), seg)):
                st = "?"  # (a segment's list, built by the game's code: may set a render mode)
        else:
            e = exit_class(t, seen + (name,))
            if e is not None:
                st = e
    _EXIT[name] = st
    return st


def walk_states(name, entry):
    """(command, class before it) over a list entered in class `entry` (FREE or ORDERED)"""
    st = entry
    for c in LISTS[name]:
        yield c, st
        m = cmd_mode(c)
        if m is not None:
            st = m
            continue
        a = after_call(c, st, LIST_DIR.get(name))
        if a is not None:
            st = a


def entry_class(name, seen=()):
    """the render mode class a list is entered in: FREE only if every way it's drawn is known and is: every list that
    calls it (as that one stands at the call), every use in the game's code (the opaque buffer). Not used by anything
    known (a skeleton's limb, a table of lists): ORDERED."""
    global _CALLERS
    if name in _ENTRY:
        return _ENTRY[name]
    if name in seen or name in DUPLICATE or name not in LISTS:
        return ORDERED
    if _CALLERS is None:
        _CALLERS = {}
        for n, cmds in LISTS.items():
            for c in cmds:
                t = call_target(c)
                if t:
                    _CALLERS.setdefault(t, set()).add(n)
    ways = set(SRC_REFS.get(name, ()))
    for caller in _CALLERS.get(name, ()):
        for c, st in walk_states(caller, entry_class(caller, seen + (name,))):
            if call_target(c) == name:
                ways.add(st)
    res = FREE if ways == {FREE} else ORDERED
    if not seen:
        _ENTRY[name] = res
    return res


def scan_sources(names):
    """how the game's code draws each list: on the translucent buffer (ORDERED), the opaque one (FREE), or some way
    not told (None: a table, a skeleton, an argument)"""
    import glob

    pat = re.compile(r"\b[A-Za-z_]\w*\b")
    for f in glob.glob(os.path.join(ROOT, "src", "**", "*.c"), recursive=True):
        text = open(f, errors="ignore").read()
        _SRC[f] = text
        # by statement (a call may span lines)
        for stmt in re.split(r"[;{}]", text):
            hit = [w for w in pat.findall(stmt) if w in names]
            if not hit:
                continue
            if re.search(r"XLU|Xlu|OVERLAY_DISP|WORK_DISP", stmt):
                cls = ORDERED
            elif re.search(r"POLY_OPA_DISP|DrawDListOpa", stmt):
                cls = FREE
            else:
                cls = None
            for w in hit:
                for m in re.finditer(r"(?<!Gfx )\b%s\s*(?:\[\s*(\w+)\s*\]|\+\s*(\w+))" % re.escape(w), stmt):
                    try:
                        at = int(m.group(1) or m.group(2), 0)
                    except ValueError:
                        at = 1 << 30  # (an offset not worked out: the list left alone if it has triangles at all)
                    MID_ENTRY[w] = max(MID_ENTRY.get(w, 0), at)
                SRC_REFS.setdefault(w, set()).add(cls)
                SRC_FILES.setdefault(w, set()).add(f)


def load_lists(base):
    """every display list of the tree: the .inc.c ones and those in the rooms' .c files"""
    for root, dirs, files in os.walk(base):
        for f in files:
            p = os.path.join(root, f)
            if f.endswith(".inc.c") and "Vtx" not in f:
                text = open(p).read()
                if "gs" not in text or "#include" in text:
                    continue
                items = mp.split_items(text)
                if items and all(("(" in c) and c.lstrip().startswith("gs") for c in items):
                    name = f[:-len(".inc.c")]
                    if name in LISTS and LISTS[name] != items:
                        DUPLICATE.add(name)
                    LISTS.setdefault(name, items)
                    LIST_DIR.setdefault(name, root)
            elif re.search(r"_room_\d+\.c$", f):
                for m in re.finditer(r"Gfx (\w+)\[\w*\] = \{(.*?)\};", open(p).read(), re.S):
                    if "#include" not in m.group(2):
                        items = mp.split_items(m.group(2))
                        if m.group(1) in LISTS and LISTS[m.group(1)] != items:
                            DUPLICATE.add(m.group(1))
                        LISTS.setdefault(m.group(1), items)
                        LIST_DIR.setdefault(m.group(1), root)


def Strip_Alike(sh):
    """a triangle's corners shade alike (within SHADE_TOL in every channel)"""
    for ch in range(4):
        vals = [int(v[ch], 0) for v in sh]
        if max(vals) - min(vals) > SHADE_TOL:
            return False
    return True
UNSIZED = []  # (directory, list name): lists whose length changed
STATS = {"tris": 0, "snaked": 0, "snakes": 0, "bytes_before": 0, "bytes_after": 0, "lists": 0,
         "ordered_tris": 0, "ordered_snaked": 0, "ordered_lists": 0, "ordered_before": 0, "ordered_after": 0}


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


def grow(start, unused, free=None):
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
                # (the triangle as the game drew it, its own first corner first: a triangle the near plane cuts is
                # drawn as a fan from its first corner, and its shading follows where the fan starts; one whose
                # corners all shade alike draws the same from any corner, and may be turned)
                for r in (rotations(t) if (free and free.get(k)) else (t,)):
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


def snakes(tris, free=None):
    """the run's triangles as chains: [(first triangle as (i1, i2, i3), [(index, direction)])]"""
    unused = {k: t for k, t in enumerate(tris)}
    out = []
    while unused:
        best = None
        for k0, t in list(unused.items()):
            rest = {k: u for k, u in unused.items() if k != k0}
            for r in (rotations(t) if (free and free.get(k0)) else (t,)):
                # the snake draws A-B-C = i3-i1-i2 first: A, B, C the triangle as the game drew it (or turned, if
                # its corners shade alike)
                seq, used = grow(r, rest, free)
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


def ordered_snakes(tris, free=None):
    """the run's triangles as chains in the run's own order: each chain a stretch of triangles that follow one
    another (so the triangles are drawn in the order the game drew them). Same form as snakes()."""
    n = len(tris)

    def rots(k):
        return rotations(tris[k]) if (free and free.get(k)) else (tris[k],)

    def extend(a, b, c, k, depth=0):
        # the longest chain on from triangle k, the snake holding (A, B, C)
        best = []
        if k >= n:
            return best
        for r in rots(k):
            for direction in (0, 1):
                e1, e2 = (a, c) if direction == 0 else (b, a)
                if r[1] == e1 and r[2] == e2 and r[0] not in (e1, e2):
                    # (grow(): right, B = A; left, C = A; then A = the new corner)
                    if direction == 0:
                        rest = extend(r[0], a, c, k + 1, depth + 1) if depth < 64 else []
                    else:
                        rest = extend(r[0], b, a, k + 1, depth + 1) if depth < 64 else []
                    if 1 + len(rest) > len(best):
                        best = [(r[0], direction)] + rest
        return best

    out, k = [], 0
    while k < n:
        best = None
        for r in rots(k):
            seq = extend(r[0], r[1], r[2], k + 1)
            if best is None or len(seq) > len(best[1]):
                best = (r, seq)
        (A, B, C), seq = best
        out.append(((B, C, A), seq))
        k += 1 + len(seq)
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


def convert(cmds, dirpath=None, scale=1.0, entry=ORDERED, keep=0):
    """the list's commands with its triangle runs as snakes; None if it's left alone. dirpath: where its vertex arrays
    are (to know which triangles' corners shade alike). entry: the render mode class the list is entered in (FREE:
    its runs' triangles may change order; ORDERED: they keep it), until it sets a render mode of its own. keep: the
    game's code enters the list at its keep'th command as well (MID_ENTRY): the commands before it stay as they are (a
    triangle among them: the list left alone), and every run keeps its order (the render mode is the caller's there)."""
    buf = [None] * 64
    arrays = {}

    def shade_of(i):
        v = buf[i] if 0 <= i < 64 else None
        return None if v is None else tuple(v[5:9])

    def small(t):
        # a triangle the near plane hardly ever cuts: no edge longer than SMALL_EDGE (in the model's own units)
        vs = [buf[i] if 0 <= i < 64 else None for i in t]
        if None in vs:
            return False
        ps = [tuple(int(x, 0) for x in v[0:3]) for v in vs]
        lim = (SMALL_EDGE / scale) ** 2
        return all(sum((ps[k][j] - ps[(k + 1) % 3][j]) ** 2 for j in range(3)) <= lim for k in range(3))
    for c in cmds:
        if "(" in c and mp.name_of(c) in ("gsSPTriSnake", "gsSPContinueSnake"):
            return None
    if keep and any(name_of(c) in TRI_CMDS for c in cmds[:keep]):
        return None
    out, run = [], []
    run_free = []
    state = [entry, False]  # the render mode class now; whether any run was an ordered one

    def flush_small(small):
        for k in range(0, len(small) - 1, 2):
            out.append("gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0)" % (small[k] + small[k + 1]))
        if len(small) % 2:
            out.append("gsSP1Triangle(%d, %d, %d, 0)" % small[-1])
        small.clear()

    def flush():
        if not run:
            return
        tris = [t for c in run for t in tris_of(c)]
        free = {k: f for k, f in enumerate(run_free)}
        ordered = (state[0] != FREE) or bool(keep)
        chains = ordered_snakes(tris, free) if ordered else snakes(tris, free)
        small = []
        mark = len(out)
        if ordered:
            state[1] = True
            STATS["ordered_tris"] += len(tris)
            STATS["ordered_before"] += 8 * len(run)
        for first, seq in chains:
            STATS["tris"] += 1 + len(seq)
            if len(seq) >= 2:
                if ordered:
                    flush_small(small)  # (the triangles before the snake, before it)
                    STATS["ordered_snaked"] += 1 + len(seq)
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
                # (each as the game drew it: the first corner first)
        flush_small(small)
        if ordered:
            if len(out) - mark >= len(run):
                # (no smaller than it was: the run as the game had it)
                del out[mark:]
                out.extend(run)
            STATS["ordered_after"] += 8 * (len(out) - mark)
        STATS["bytes_before"] += 8 * len(run)
        run.clear()
        run_free.clear()

    for c in cmds:
        if "(" in c and mp.name_of(c) == "gsSPVertex" and dirpath is not None:
            a = mp.args_of(c)
            m = mp.VTX_REF.match(a[0])
            try:
                cnt, v0 = int(a[1], 0), int(a[2], 0)
            except ValueError:
                cnt, v0 = 0, 0
            arr = m.group(1) if m else None
            if arr is not None and arr not in arrays:
                pth = os.path.join(dirpath, arr + ".inc.c")
                arrays[arr] = mp.load_vtx(dirpath, arr) if os.path.exists(pth) else None
            vs = arrays.get(arr)
            idx = int(m.group(2) or 0) if m else 0
            for k in range(cnt):
                buf[v0 + k] = vs[idx + k] if (vs is not None and idx + k < len(vs) and v0 + k < 64) else None
        if "(" in c and mp.name_of(c) in TRI_CMDS:
            run.append(c)
            for t in tris_of(c):
                sh = [shade_of(i) for i in t]
                run_free.append(TURN_ALL or ((None not in sh) and Strip_Alike(sh)) or small(t))
            continue
        flush()
        out.append(c)
        if "(" in c:
            m = cmd_mode(c)
            if m is not None:
                state[0] = m
            else:
                a = after_call(c, state[0], dirpath)
                if a is not None:
                    state[0] = a
    flush()
    if state[1]:
        STATS["ordered_lists"] += 1
    STATS["bytes_after"] += 8 * sum(1 for c in out if "Triangle" in c or "Snake" in c)
    return out


def expand(cmds):
    """the triangles a converted run draws, in order, each as (first corner, second, third)"""
    out = []
    a = b = c = None
    for cmd in cmds:
        n = mp.name_of(cmd)
        if n in TRI_CMDS:
            out += tris_of(cmd)
            continue
        args = [x.strip() for x in mp.args_of(cmd)]
        if n == "gsSPTriSnake":
            i1, i2 = int(args[0]), int(args[1])
            i3 = int(args[2].split("|")[0])
            # (the snake draws A-B-C = i3-i1-i2)
            a, b, c = i3, i1, i2
            out.append((a, b, c))
            done = "G_SNAKE_LAST" in args[2]
            pairs = list(zip(args[3::2], args[4::2]))
        else:
            done = False
            pairs = list(zip(args[0::2], args[1::2]))
        for v, d in pairs:
            if done:
                break
            nv = int(v.split("|")[0])
            if d == "G_SNAKE_RIGHT":
                out.append((nv, a, c))
                b = a
            else:
                out.append((nv, b, a))
                c = a
            a = nv
            done = "G_SNAKE_LAST" in v
    return out


def selftest(rounds=3000):
    import random

    rnd = random.Random(1)
    snaked = 0
    for it in range(rounds):
        nv = rnd.randint(3, 20)
        tris = []
        # (runs with real strips in them: a walk over a grid's cells now and then, else random triangles)
        while len(tris) < rnd.randint(1, 40):
            if rnd.random() < 0.5:
                x = rnd.randrange(0, 24, 2)
                for k in range(rnd.randint(1, 6)):
                    q = [x + k * 2, x + k * 2 + 1, x + k * 2 + 2, x + k * 2 + 3]
                    tris.append((q[0], q[1], q[2]))
                    tris.append((q[2], q[1], q[3]))
            else:
                t = rnd.sample(range(nv), 3)
                tris.append(tuple(t))
        run = []
        k = 0
        while k < len(tris):
            if k + 1 < len(tris) and rnd.random() < 0.8:
                run.append("gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0)" % (tris[k] + tris[k + 1]))
                k += 2
            else:
                run.append("gsSP1Triangle(%d, %d, %d, 0)" % tris[k])
                k += 1
        turn = rnd.random() < 0.5
        global TURN_ALL
        TURN_ALL = turn
        for entry in (FREE, ORDERED):
            got = expand(convert(list(run) + ["gsSPEndDisplayList()"], None, 1.0, entry)[:-1])
            if turn:
                norm = lambda t: min(rotations(t))  # noqa: E731
            else:
                norm = lambda t: t  # noqa: E731  (the first corner kept too)
            want = [norm(t) for t in tris]
            have = [norm(t) for t in got]
            assert sorted(want) == sorted(have), (it, entry, "triangle set or winding")
            if entry == ORDERED:
                assert want == have, (it, "order")
        snaked += 1
    # the render mode classes
    for mode, cls in (("(G_RM_FOG_SHADE_A) | (G_RM_AA_ZB_OPA_SURF2)", FREE), ("(G_RM_PASS) | (G_RM_AA_ZB_TEX_EDGE2)", FREE),
                      ("(G_RM_FOG_SHADE_A) | (G_RM_AA_ZB_XLU_SURF2)", ORDERED), ("(G_RM_PASS) | (G_RM_ZB_CLD_SURF2)", ORDERED),
                      ("(G_RM_FOG_SHADE_A) | (G_RM_ZB_OVL_SURF2)", ORDERED), ("(G_RM_PASS) | (G_RM_AA_ZB_XLU_DECAL2)", ORDERED),
                      ("(G_RM_FOG_SHADE_A) | (G_RM_AA_ZB_OPA_DECAL2)", ORDERED), ("(G_RM_AA_OPA_SURF) | (G_RM_AA_OPA_SURF2)", ORDERED),
                      ("(G_RM_AA_ZB_OPA_SURF) | (G_RM_AA_ZB_OPA_SURF2)", FREE), ("(G_RM_PASS) | (G_RM_CLD_SURF2)", ORDERED),
                      ("(G_RM_ZB_OPA_SURF) | (G_RM_ZB_OPA_SURF2)", FREE), ("(nonsense)", ORDERED)):
        assert mode_class(mode) == cls, (mode, mode_class(mode))
    print("strippass selftest: %d runs, both orders: triangle sets, winding and first corners kept; the order kept "
          "where it has to be; render mode classes as expected" % rounds)


def main():
    if len(sys.argv) == 2 and sys.argv[1] == "--selftest":
        selftest()
        return
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    src, out = sys.argv[1], sys.argv[2]
    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(src, out, copy_function=os.link)
    SCALES.update(model_scales(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

    def scale_of(d):
        # a room: the world's units; an object's or an actor's models: their actors' scale (else 1)
        parts = os.path.relpath(d, base).split(os.sep)
        if parts[0] == "scenes":
            return 1.0
        for p_ in parts:
            if p_ in SCALES:
                return SCALES[p_]
        return 1.0

    base = os.path.join(out, "assets")
    load_lists(base)
    names = set(LISTS)
    for root, dirs, files in os.walk(base):
        if "scenes" not in root.split(os.sep):
            names.update(f.split(".")[0] for f in files if f.endswith(".inc.c"))  # (skeletons, animations: who draws the directory)
    scan_sources(names)
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
                new = convert(items, root, scale_of(root), entry_class(f[:-len(".inc.c")]),
                              MID_ENTRY.get(f[:-len(".inc.c")], 0))
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
                    if "Triangle" not in m.group(3) or "#include" in m.group(3):
                        return m.group(0)
                    new = convert(mp.split_items(m.group(3)), root, 1.0, entry_class(m.group(2)),
                                  MID_ENTRY.get(m.group(2), 0))
                    if new is None:
                        return m.group(0)
                    STATS["lists"] += 1
                    return m.group(1) + "\n" + "".join("    %s,\n" % c for c in new) + "};"
                new_text = re.sub(r"(Gfx (\w+)\[\w*\] = \{)(.*?)\};", sub, text, flags=re.S)
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
    print("strippass: %d lists with runs kept in the game's order (a render mode that blends or doesn't write depth, "
          "or one not known): %d of their %d triangles in snakes, %d -> %d bytes" % (
              s["ordered_lists"], s["ordered_snaked"], s["ordered_tris"], s["ordered_before"], s["ordered_after"]))
    print("strippass: %d lists; %d of %d triangles in %d snakes (%.1f a snake); triangle commands %d -> %d bytes" % (
        s["lists"], s["snaked"], s["tris"], s["snakes"], s["snaked"] / max(1, s["snakes"]),
        s["bytes_before"], s["bytes_after"]))


if __name__ == "__main__":
    main()
