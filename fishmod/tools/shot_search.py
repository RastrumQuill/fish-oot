#!/usr/bin/env python3
"""Console frame-time estimate for any camera at Lake Hylia, and a search for under-water shots that fit a budget.

The model (fitted to hardware benches 4 and 5h: each stop's near and far prop cost against what each placement put
on screen there, 88 points): the scanlines each prop's cards cover, summed over its cards, plus a fixed cost per prop
drawn,
  near (within 500): 0.00265 ms per card-scanline + 0.107 ms per prop
  far:               0.00173 ms per card-scanline + 0.072 ms per prop    (rms 2.3-3.0 ms per stop on each bench)
Scanlines alone fit bench 4 (rms 3.7) but missed 5h by 14 ms on average: 5h draws many more, smaller props (moss
mats), and each prop costs about as much to issue as a few hundred scanlines.
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

KN, KF = 0.00265, 0.00173  # ms per card-scanline, near / far
PN, PF = 0.107, 0.072      # ms per prop drawn, near / far (its fixed cost: commands, state, setup)
BASE_UNDER = 30.0          # no-prop RDP time under water (hwbench-4: most stops 25-31 ms)
F = 207.8                  # focal length in pixels: 240 lines, 60 degrees
SURFACE = -1313
# per kind: (cards drawn, overall width at scale 1); height is gen_scatter's HEIGHT (rdp_estimate.HEIGHT)
CARDS = {"grass_tuft": (4, 80), "ribbon_weed": (3, 60), "bush_weed": (4, 100), "tall_weed": (2, 86),
         "broadleaf": (2, 40), "big_pondweed": (2, 44), "kelp_frond": (1, 30), "vine_strand": (2, 30),
         "fan_weed": (2, 44), "rock": (1, 120), "boulder": (1, 120), "moss_patch": (1, 100), "lily_pad": (1, 110),
         "pond_reed": (1, 30), "reed_clump": (3, 40), "cattails": (3, 44), "spatterdock": (1, 60),
         "float_leaves": (1, 60)}
FLAT = {"moss_patch", "pebble_patch", "leaf_litter", "lily_pad", "float_leaves", "spatterdock"}
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
        self.FLAT = np.array([k in FLAT for k in self.kind])

    def contrib(self, eye, at, under=True):
        """Per prop: its share of the view's RDP time in ms, and whether it's drawn and on screen (its cover)."""
        eye = np.array(eye, float)
        fw = np.array(at, float) - eye
        fw /= np.linalg.norm(fw)
        rt = np.cross(fw, [0, 1, 0])
        rt /= np.linalg.norm(rt)
        up = np.cross(rt, fw)
        d = self.P[:, :3] - eye
        dsq = d[:, 0] ** 2 + d[:, 2] ** 2
        ok = dsq < (self.CUT if under else self.CUT_ABOVE) ** 2
        # upright plants: a card of their height and width, facing the camera
        c = d.copy()
        c[:, 1] += np.where(self.FLAT, 0, self.H / 2)
        z, x, y = c @ fw, c @ rt, c @ up
        ok &= z > -self.W
        zc = np.maximum(z, 15.0)
        x0, x1 = (x - self.W / 2) * F / zc + 160, (x + self.W / 2) * F / zc + 160
        y0, y1 = (y - self.H / 2) * F / zc + 120, (y + self.H / 2) * F / zc + 120
        # flat ones (moss, pads, decals): a level disc, its near and far edges along the look across the ground
        fh = np.array([fw[0], 0.0, fw[2]])
        fh /= max(np.linalg.norm(fh), 1e-6)
        r = (self.W / 2)[:, None]
        near_pt = c - r * fh
        far_pt = c + r * fh
        zn, zf = np.maximum(near_pt @ fw, 15.0), np.maximum(far_pt @ fw, 15.0)
        yn, yf = (near_pt @ up) * F / zn + 120, (far_pt @ up) * F / zf + 120
        y0 = np.where(self.FLAT, np.minimum(yn, yf), y0)
        y1 = np.where(self.FLAT, np.maximum(yn, yf), y1)
        cx = np.clip(x1, 0, 320) - np.clip(x0, 0, 320)
        cy = np.clip(y1, 0, 240) - np.clip(y0, 0, 240)
        ok &= (cx > 0) & (cy > 0)
        near = dsq < 500 ** 2
        ms = np.where(ok, cy * self.NC * np.where(near, KN, KF) + np.where(near, PN, PF), 0)
        return ms, ok, cx * cy

    def cost(self, eye, at, under=True, base=BASE_UNDER):
        """(estimated RDP ms, plant screen cover, props drawn); base: the view's no-prop RDP time."""
        ms, ok, area = self.contrib(eye, at, under)
        cover = np.where(ok & self.PLANT, np.minimum(area, 4000), 0).sum()
        return float(base + ms.sum()), float(cover), int(ok.sum())


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
