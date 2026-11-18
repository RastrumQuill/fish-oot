#!/usr/bin/env python3
"""Two builds' runs of one scripted route, laid side by side: every set of props Scatter_Build made (the frame, the
camera, what's in it) and Link's place every 8th frame, from memory dumps of gFishmodSetTrace and gFishmodLinkTrace
taken after each run ends (the bench halts itself: fishmod/docs/hw-suite.md).

  settrace.py A.sets A.link B.sets B.link

In ares: ares_run.sh SLOT ROM SECONDS -- "dump binary memory A.sets 0x<gFishmodSetTrace> 0x<...>+20480"
                                         "dump binary memory A.link 0x<gFishmodLinkTrace> 0x<...>+8192"
"""
import struct
import sys


def sets(path):
    d = open(path, "rb").read()
    out = []
    for i in range(len(d) // 20):
        fr, cnt, ex, ey, ez, look, props = struct.unpack_from(">IH3hh2xI", d, 20 * i)
        if fr == 0 and cnt == 0 and props == 0:
            break
        out.append([fr, cnt, (ex, ey, ez), look, props])
    return out


def link(path):
    d = open(path, "rb").read()
    return [struct.unpack_from(">4h", d, 8 * i) for i in range(len(d) // 8)]


def main():
    a, la, b, lb = sets(sys.argv[1]), link(sys.argv[2]), sets(sys.argv[3]), link(sys.argv[4])
    print("sets made: %d and %d (first on frame %d and %d)" % (len(a), len(b), a[0][0] if a else -1, b[0][0] if b else -1))
    fa = {s[0] - a[0][0]: s for s in a}
    fb = {s[0] - b[0][0]: s for s in b}
    both = sorted(set(fa) & set(fb))
    same = [f for f in both if fa[f][1:] == fb[f][1:]]
    print("made on the same frame: %d; of those identical (camera, props, tiers): %d" % (len(both), len(same)))
    diff = sorted(set(fa) ^ set(fb)) + [f for f in both if fa[f][1:] != fb[f][1:]]
    if not diff:
        print("every set is the same in both runs")
    else:
        first = min(diff)
        print("identical up to frame %d of the run; from there:" % first)
        for f in sorted(set(fa) | set(fb)):
            if first - 6 <= f <= first + 8:
                print("  frame %5d  %-48s %s" % (f, tuple(fa[f][1:]) if f in fa else "-",
                                                tuple(fb[f][1:]) if f in fb else "-"))
    pairs = [(i * 8, p, q) for i, (p, q) in enumerate(zip(la, lb)) if p != (0, 0, 0, 0) or q != (0, 0, 0, 0)]
    apart = [(f, max(abs(p[k] - q[k]) for k in range(3))) for f, p, q in pairs]
    print("Link, every 8th frame: %d samples, the same place in %d" % (len(pairs), sum(1 for _, d in apart if d == 0)))
    for thr in (1, 20, 100):
        f = next((f for f, d in apart if d >= thr), None)
        print("  first %d or more units apart: %s" % (thr, "never" if f is None else "frame %d" % f))


if __name__ == "__main__":
    main()
