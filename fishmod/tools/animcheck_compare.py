#!/usr/bin/env python3
"""Compares two animation check logs (fishmod/scripts/animcheck.inc.c, saved by fishmod/tools/collog.sh --anim): case
by case, the pose (the joint table and the limb matrices: level 1) and the opaque and translucent draw streams (every
triangle with its vertices and the state it's drawn with: level 2). Prints the first differing cases (which
animation, which frame, which hash) and a count per phase, or that every case is identical.

Usage: fishmod/tools/animcheck_compare.py A.bin B.bin [--all]
Exit status 0 only when both logs hold the same cases and every one matches.
"""
import collections
import os
import re
import struct
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
PHASES = ["frames", "blends", "model groups", "faces", "tunics/boots", "far detail"]


def anim_names():
    p = os.path.join(ROOT, "src", "fishmod_data", "fishmod_animcheck_cases.h")
    if not os.path.exists(p):
        return []
    return re.findall(r"&(gPlayerAnim_\w+),", open(p).read())


def load(path):
    d = open(path, "rb").read()
    out = collections.OrderedDict()
    for i in range(len(d) // 32):
        r = struct.unpack_from(">8I", d, 32 * i)
        out[r[0]] = r
    return out


def describe(case, names):
    phase, a, f = case >> 28, (case >> 16) & 0xFFF, case & 0xFFFF
    name = PHASES[phase] if phase < len(PHASES) else "phase %d" % phase
    if phase == 0 or phase == 5:
        an = names[a if phase == 0 else f] if names else "#%d" % a
        return "%s: %s frame %d" % (name, an, f if phase == 0 else 0)
    if phase == 1:
        return "%s: %s with %s at %.2f" % (name, names[a] if names else a, names[a + 1] if names else a + 1,
                                         0.25 * (f + 1))
    return "%s: variant %d, %s" % (name, a, names[f] if names and f < len(names) else "#%d" % f)


def main():
    a, b = load(sys.argv[1]), load(sys.argv[2])
    names = anim_names()
    show_all = "--all" in sys.argv
    common = [c for c in a if c in b]
    only_a = [c for c in a if c not in b]
    only_b = [c for c in b if c not in a]
    diff = collections.Counter()
    per_phase = collections.Counter()
    shown = 0
    for c in common:
        ra, rb = a[c], b[c]
        per_phase[c >> 28] += 1
        what = [n for n, i in (("pose", 1), ("opaque stream", 2), ("translucent stream", 3)) if ra[i] != rb[i]]
        if ra[4] != rb[4]:
            what.append("triangle counts %d/%d -> %d/%d" % (ra[4] >> 16, ra[4] & 0xFFFF, rb[4] >> 16, rb[4] & 0xFFFF))
        if what:
            diff[c >> 28] += 1
            if shown < 20 or show_all:
                print("DIFFERS  %-60s %s" % (describe(c, names), ", ".join(what)))
                shown += 1
    flagged = sum(1 for r in list(a.values()) + list(b.values()) if r[6])
    print("\n%d cases in both (%s)" % (len(common), ", ".join("%s %d" % (PHASES[p], n) for p, n in sorted(per_phase.items()))))
    if only_a or only_b:
        print("cases in only one log: %d in A, %d in B (a run cut short?)" % (len(only_a), len(only_b)))
    if flagged:
        print("WARNING: %d records flagged (a walk went astray, or no limb matrices found)" % flagged)
    if diff:
        print("DIFFERENT: %d cases (%s)" % (sum(diff.values()), ", ".join("%s %d" % (PHASES[p], n) for p, n in sorted(diff.items()))))
        sys.exit(1)
    print("IDENTICAL: every case's pose and draw streams the same")
    sys.exit(0 if not (only_a or only_b) else 2)


if __name__ == "__main__":
    main()
