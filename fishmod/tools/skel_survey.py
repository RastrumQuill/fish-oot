#!/usr/bin/env python3
"""Every skeleton in the game and how much of it ever moves: for each object's skeletons, its limbs (with geometry or
without: a limb with no display list is a joint only, still computed every frame), and, over every animation of the
same object made for that skeleton (the same joint count), which limbs' rotations never change (a fixed value in
every frame of every one of those animations). A limb that never turns relative to its parent could be drawn under
its parent's matrix (its geometry moved into the parent's frame once, at build time): one matrix fewer a frame for
the CPU to compute and convert and for the RSP to load. (An actor's own code may still turn such a limb at run time,
a head looking at Link: the candidates are checked against the actor's limb callbacks before anything is merged.)

Usage: fishmod/tools/skel_survey.py [--version ntsc-1.2] [--top N] [--object NAME]
"""
import argparse
import collections
import glob
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))


def read(p):
    try:
        return open(p).read()
    except OSError:
        return ""


def nums(text):
    return [int(x, 0) for x in re.findall(r"(-?0x[0-9A-Fa-f]+|-?\d+)\s*,", text)]


def parse_object(d):
    name = os.path.basename(d)
    main = read(os.path.join(d, name + ".c"))
    if not main:
        return None
    def inc(n):
        """a definition's body: its own file (#included in the object's .c) or written in the .c itself"""
        m = re.search(r"^[\w ]+\b" + re.escape(n) + r"\b(\[\w*\])? = \{(.*?)^\};", main, re.M | re.S)
        if not m:
            return read(os.path.join(d, n + ".inc.c"))
        body = m.group(2)
        i = re.search(r'#include "([^"]+\.inc\.c)"', body)
        if i:
            return read(os.path.join(ROOT, "extracted", os.path.basename(os.path.dirname(os.path.dirname(os.path.dirname(d)))),
                                     i.group(1))) or read(os.path.join(d, os.path.basename(i.group(1))))
        return body
    # limbs: type, display lists, children
    limbs = {}
    for m in re.finditer(r"^(StandardLimb|LodLimb|SkinLimb|LegacyLimb) (\w+) = \{", main, re.M):
        typ, ln = m.group(1), m.group(2)
        t = inc(ln)
        dls = re.findall(r"^\s*(NULL|\w+|0x[0-9A-Fa-f]+),\s*// (?:dList|dLists\[\d\]|segment|dlist)", t, re.M)
        if typ == "LodLimb":
            dl = re.search(r"\{\s*(\w+),\s*(\w+)\s*,?\s*\}", t)
            dls = [dl.group(1), dl.group(2)] if dl else dls
        has_dl = any(x not in ("NULL", "0", "0x00000000") for x in dls) if dls else ("NULL" not in t)
        limbs[ln] = {"type": typ, "dl": has_dl}
    skels = []
    for m in re.finditer(r"^(SkeletonHeader|FlexSkeletonHeader) (\w+) = \{", main, re.M):
        sn = m.group(2)
        t = inc(sn)
        lst = re.search(r"(\w+),\s*// segment", t)
        if not lst:
            continue
        names = re.findall(r"&(\w+),", inc(lst.group(1)))
        skels.append({"name": sn, "flex": m.group(1) == "FlexSkeletonHeader", "limbs": names})
    anims = []
    for m in re.finditer(r"^AnimationHeader (\w+) = \{", main, re.M):
        t = inc(m.group(1))
        fc = re.search(r"(-?\d+),\s*// frameCount", t)
        fd = re.search(r"(\w+),\s*// frameData", t)
        ji = re.search(r"(\w+),\s*// jointIndices", t)
        sm = re.search(r"(\d+),\s*// staticIndexMax", t)
        if not (fc and fd and ji and sm):
            continue
        jv = nums(inc(ji.group(1)))
        joints = [tuple(jv[i:i + 3]) for i in range(0, len(jv) - 2, 3)]
        anims.append({"name": m.group(1), "frames": int(fc.group(1)), "data": nums(inc(fd.group(1))),
                      "joints": joints, "static": int(sm.group(1))})
    return {"name": name, "limbs": limbs, "skels": skels, "anims": anims}


def survey(obj):
    out = []
    for sk in obj["skels"]:
        n = len(sk["limbs"])
        anims = [a for a in obj["anims"] if len(a["joints"]) == n + 1]
        with_dl = [obj["limbs"].get(ln, {}).get("dl", True) for ln in sk["limbs"]]
        fixed = []  # per limb: its rotation the same fixed value in every animation (None: no animations)
        for li in range(n):
            vals = set()
            ok = bool(anims)
            for a in anims:
                j = a["joints"][li + 1]  # (joint 0: the root's translation)
                if any(ix >= a["static"] for ix in j):
                    ok = False
                    break
                try:
                    vals.add(tuple(a["data"][ix] for ix in j))
                except IndexError:
                    ok = False
                    break
            fixed.append(ok and len(vals) == 1)
        zero = [f and all(v == 0 for v in vals) for f in fixed] if False else None
        out.append({"skel": sk["name"], "flex": sk["flex"], "limbs": n, "anims": len(anims),
                    "with_dl": sum(with_dl), "no_dl": n - sum(with_dl),
                    "fixed": sum(fixed), "fixed_with_dl": sum(1 for f, d in zip(fixed, with_dl) if f and d),
                    "fixed_no_dl": sum(1 for f, d in zip(fixed, with_dl) if f and not d),
                    "moving": n - sum(fixed)})
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--version", default="ntsc-1.2")
    ap.add_argument("--top", type=int, default=30)
    ap.add_argument("--object")
    args = ap.parse_args()
    base = os.path.join(ROOT, "extracted", args.version, "assets", "objects")
    rows = []
    for d in sorted(glob.glob(os.path.join(base, "*"))):
        if args.object and os.path.basename(d) != args.object:
            continue
        obj = parse_object(d)
        if not obj:
            continue
        for s in survey(obj):
            rows.append((obj["name"], s))
    animated = [(o, s) for o, s in rows if s["anims"]]
    tl = sum(s["limbs"] for _, s in animated)
    print(f"{len(rows)} skeletons in {len({o for o, _ in rows})} objects; {len(animated)} with animations in the same "
          f"object ({tl} limbs)")
    print(f"  limbs without geometry (joints only): {sum(s['no_dl'] for _, s in animated)}")
    print(f"  limbs whose rotation never changes in any of their animations: {sum(s['fixed'] for _, s in animated)} "
          f"({sum(s['fixed_with_dl'] for _, s in animated)} with geometry, {sum(s['fixed_no_dl'] for _, s in animated)} "
          f"without)")
    print(f"  limbs that move: {sum(s['moving'] for _, s in animated)}")
    print("\n| object | skeleton | limbs | animations | without geometry | never turn | of which with geometry | move |")
    print("|---|---|---|---|---|---|---|---|")
    for o, s in sorted(animated, key=lambda r: -r[1]["fixed"])[:args.top]:
        print(f"| {o} | {s['skel']} | {s['limbs']} | {s['anims']} | {s['no_dl']} | {s['fixed']} | {s['fixed_with_dl']} "
              f"| {s['moving']} |")


if __name__ == "__main__":
    main()
