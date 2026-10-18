#!/usr/bin/env python3
"""Console frame-time estimate for any camera at Lake Hylia, and a search for under-water shots that fit a budget.

The model (fitted to hardware bench 4: each stop's near and far prop cost against what bench 4's placement put on
screen there, 44 points): the RDP's prop time is the scanlines each prop's cards cover, summed over its cards,
  near (within 500): 0.00326 ms per card-scanline, far: 0.00242 ms   (rms error 3.2 ms per stop)
which fits far better than pixels (rms 6.9) or counts: the cost is per span (each scanline's run starts with a
memory fetch), so tall, many-card plants cost most and low flat ones (moss, decals) almost nothing from a low camera.
Far props' cheaper tiers since bench 4 (single card, half texture) aren't modelled: far is an upper bound.
CPU and RSP time per prop isn't modelled either (props past ~900 a frame, or a big jump in count, need a bench).

Usage: shot_search.py [--budget 47] [--tries 6000] [--out views.json]
       shot_search.py --cost EX EY EZ AX AY AZ
"""
import argparse
import json
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rdp_estimate as R  # noqa: E402

KN, KF = 0.00326, 0.00242  # ms per card-scanline, near / far
BASE_UNDER = 30.0          # no-prop RDP time under water (hwbench-4: most stops 25-31 ms)
F = 207.8                  # focal length in pixels: 240 lines, 60 degrees
SURFACE = -1313
# per kind: (cards drawn, overall width at scale 1); height is gen_scatter's HEIGHT (rdp_estimate.HEIGHT)
CARDS = {"grass_tuft": (8, 80), "ribbon_weed": (6, 60), "bush_weed": (6, 100), "tall_weed": (4, 86),
         "broadleaf": (3, 40), "big_pondweed": (4, 44), "kelp_frond": (3, 30), "vine_strand": (2, 30),
         "fan_weed": (2, 44), "rock": (1, 120), "boulder": (1, 120), "moss_patch": (1, 100), "lily_pad": (1, 110),
         "pond_reed": (1, 30), "reed_clump": (3, 40), "cattails": (3, 44), "spatterdock": (1, 60),
         "float_leaves": (1, 60)}
NOT_PLANT = {"rock", "boulder", "moss_patch", "pebble_patch", "lily_pad", "float_leaves", "spatterdock"}


class Scene:
    def __init__(self, placement=None):
        inst = R.read_placement(placement or os.path.join(R.ROOT, "src/fishmod_data/fishmod_sc_spot06.c"))
        dist = R.sb.distances()
        self.kind = [p["kind"] for p in inst]
        self.P = np.array([[p["x"], p["y"], p["z"], p["scale"]] for p in inst], float)
        self.H = np.array([R.HEIGHT.get(k, 40) for k in self.kind]) * self.P[:, 3]
        self.NC = np.array([CARDS.get(k, (2, 0))[0] for k in self.kind])
        self.W = np.array([CARDS[k][1] if k in CARDS else R.HEIGHT.get(k, 40) for k in self.kind]) * self.P[:, 3]
        self.CUT = np.array([dist.get(k, (1600, 900))[1] for k in self.kind])
        self.CUT_ABOVE = np.array([dist.get(k, (1600, 900))[0] for k in self.kind])
        self.PLANT = np.array([k not in NOT_PLANT for k in self.kind])

    def cost(self, eye, at, under=True, base=BASE_UNDER):
        """(estimated RDP ms, plant screen cover, props drawn); base: the view's no-prop RDP time."""
        eye = np.array(eye, float)
        fw = np.array(at, float) - eye
        fw /= np.linalg.norm(fw)
        rt = np.cross(fw, [0, 1, 0])
        rt /= np.linalg.norm(rt)
        up = np.cross(rt, fw)
        d = self.P[:, :3] - eye
        dsq = d[:, 0] ** 2 + d[:, 2] ** 2
        ok = dsq < (self.CUT if under else self.CUT_ABOVE) ** 2
        c = d.copy()
        c[:, 1] += self.H / 2
        z, x, y = c @ fw, c @ rt, c @ up
        ok &= z > -self.W
        z = np.maximum(z, 15.0)
        x0, x1 = (x - self.W / 2) * F / z + 160, (x + self.W / 2) * F / z + 160
        y0, y1 = (y - self.H / 2) * F / z + 120, (y + self.H / 2) * F / z + 120
        cx = np.clip(x1, 0, 320) - np.clip(x0, 0, 320)
        cy = np.clip(y1, 0, 240) - np.clip(y0, 0, 240)
        ok &= (cx > 0) & (cy > 0)
        spans = np.where(ok, cy * self.NC, 0)
        near = dsq < 500 ** 2
        ms = base + KN * spans[near].sum() + KF * spans[~near].sum()
        cover = np.where(ok & self.PLANT, np.minimum(cx * cy, 4000), 0).sum()
        return float(ms), float(cover), int(ok.sum())


def search(scene, tries, seed=1):
    gs, gw = R.gs, R.gs.gw
    d = os.path.join(gw.SCENES, gs.SCENES["SCENE_LAKE_HYLIA"])
    vtx = gw.parse_vtx(gw.one(d, "*_VtxList.inc.c"))
    finder = gw.FloorFinder(gw.parse_floors(gw.one(d, "*_PolyList.inc.c"), vtx))
    rng = np.random.default_rng(seed)
    plants = scene.P[scene.PLANT]
    out = []
    for _ in range(tries):
        p = plants[rng.integers(len(plants))]
        ang = rng.uniform(0, 2 * math.pi)
        back = rng.uniform(150, 450)
        ex, ez = p[0] - math.cos(ang) * back, p[2] - math.sin(ang) * back
        fl = finder.floor(int(ex), int(ez), SURFACE)
        if fl is None or not (70 <= SURFACE - fl <= 320):
            continue
        ey = min(SURFACE - 25, fl + rng.uniform(45, 130))
        if ey < fl + 30:
            continue
        at = (ex + math.cos(ang) * 500, ey - rng.uniform(40, 160), ez + math.sin(ang) * 500)
        ms, cover, n = scene.cost((ex, ey, ez), at)
        out.append({"eye": [round(ex), round(ey), round(ez)], "at": [round(v) for v in at], "ms": round(ms, 1),
                    "cover": cover, "n": n})
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--budget", type=float, default=47.0)
    ap.add_argument("--tries", type=int, default=6000)
    ap.add_argument("--pick", type=int, default=12, help="shots to keep, at least 700 units apart")
    ap.add_argument("--out")
    ap.add_argument("--cost", type=float, nargs=6)
    a = ap.parse_args()
    scene = Scene()
    if a.cost:
        print("%.1f ms, cover %.0f, %d props" % scene.cost(a.cost[:3], a.cost[3:]))
        return
    views = [v for v in search(scene, a.tries) if v["ms"] <= a.budget]
    views.sort(key=lambda v: -v["cover"])
    pick = []
    for v in views:
        if all(math.hypot(v["eye"][0] - q["eye"][0], v["eye"][2] - q["eye"][2]) > 700 for q in pick):
            pick.append(v)
        if len(pick) == a.pick:
            break
    for v in pick:
        print(v["eye"], v["at"], v["ms"], "ms")
    if a.out:
        json.dump(pick, open(a.out, "w"), indent=1)


if __name__ == "__main__":
    main()
