#!/usr/bin/env python3
"""The props' share of the RDP over several frames (rdp_cost.py --json outputs): by kind, and by kind and tier (the
model list's name: ...DL near (cut cards), ...DLFar whole cards, ...DLFarH half texture, ...DLImp single card).

Usage: rdp_cost_props.py A.json B.json ...
"""
import collections
import json
import re
import sys

TIER = {"": "near", "Far": "far", "FarH": "half", "Imp": "single card"}


def main():
    files = sys.argv[1:]
    kind = collections.defaultdict(lambda: [0.0, 0, 0.0, 0])
    tier = collections.defaultdict(lambda: [0.0, 0, 0.0, 0])
    total = props = 0.0
    for f in files:
        d = json.load(open(f))
        for k, e in d.items():
            total += e["ns"]
            m = re.match(r"gFishmodProp_(\w+?)DL(Far|FarH|Imp)?(Material)?$", k)
            if not m:
                continue
            props += e["ns"]
            name = m.group(1)
            t = TIER[m.group(2) or ""] + (" material" if m.group(3) else "")
            for agg, key in ((kind, name), (tier, (name, t))):
                agg[key][0] += e["ns"]
                agg[key][1] += e["tris"]
                agg[key][2] += e["px"]
                agg[key][3] += e["loads"]
    n = len(files)
    print("%d frames: the whole frame %.2f ms (estimated), the props %.2f ms (%.0f%%)" % (
        n, total / n / 1e6, props / n / 1e6, 100 * props / max(total, 1)))
    print("\n| kind | ms a frame | share of props | triangles | pixels | px/tri |")
    print("|---|---|---|---|---|---|")
    for k, e in sorted(kind.items(), key=lambda kv: -kv[1][0]):
        print("| %s | %.2f | %.0f%% | %.0f | %.0f | %.0f |" % (k, e[0] / n / 1e6, 100 * e[0] / props, e[1] / n,
                                                         e[2] / n, e[2] / max(e[1], 1)))
    print("\n| kind | tier | ms a frame | triangles | pixels | ns/px | us/tri |")
    print("|---|---|---|---|---|---|---|")
    for (k, t), e in sorted(tier.items(), key=lambda kv: -kv[1][0])[:30]:
        print("| %s | %s | %.2f | %.0f | %.0f | %.0f | %.1f |" % (k, t, e[0] / n / 1e6, e[1] / n, e[2] / n,
                                                               e[0] / max(e[2], 1), e[0] / max(e[1], 1) / 1e3))


if __name__ == "__main__":
    main()
