#!/usr/bin/env python3
"""Pop-in check: from a RAM dump of a debug build (framedump.sh), every prop that should be on screen by the last set's
own view (inside the picture, within its kind's draw distance, not on a dry chunk, not right against the camera)
against the props that set draws (fishmod_scatter.c, gFishmodScatterDbg). A prop that should be there and isn't is
pop-in that isn't by distance: listed with where on the screen it is and how far.

Usage: fishmod/tools/popin_check.py build.elf ram.bin
"""
import math
import struct
import subprocess
import sys

KIND_MAX = 32
PROP_BUDGET = 900
BEDCARD_DIST = 1300.0
FADE_BAND = 0.15


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    elf, ram = sys.argv[1], sys.argv[2]
    nm = {}
    for l in subprocess.run(["mips-linux-gnu-nm", elf], capture_output=True, text=True).stdout.splitlines():
        p = l.split()
        if len(p) == 3:
            nm[p[2]] = int(p[0], 16)
    d = open(ram, "rb").read()

    def u32(a):
        return struct.unpack_from(">I", d, a & 0xFFFFFF)[0]

    dbg = [u32(nm["gFishmodScatterDbg"] + 4 * i) for i in range(8)]
    if dbg[0] == 0:
        sys.exit("no set made yet")
    hdr, props, sset, kmax, view, dry, cards, kinds = dbg
    x0, z0, chunk, width, depth, count = struct.unpack_from(">hhhhhH", d, (hdr & 0xFFFFFF) + 4)
    v = struct.unpack_from(">12f", d, view & 0xFFFFFF)
    eye, look, up, fovy, under = v[0:3], v[3:6], v[6:9], v[9], v[10] > 0.5
    kindMax = struct.unpack_from(">%df" % KIND_MAX, d, kmax & 0xFFFFFF)
    info = []
    for k in range(KIND_MAX):
        a = (kinds & 0xFFFFFF) + 0x40 * k
        modelImp = struct.unpack_from(">I", d, a + 0x20)[0]
        floating, plant, xlu, sway, swayRate, card = struct.unpack_from(">6B", d, a + 0x34)
        height, extent = struct.unpack_from(">HH", d, a + 0x3C)
        info.append(dict(imp=modelImp != 0, floating=floating, plant=plant, card=card, height=height, extent=extent))
    base = sset & 0xFFFFFF
    off_prop = base + PROP_BUDGET * 64 + PROP_BUDGET * 4
    off_hidden = off_prop + PROP_BUDGET * 2 + PROP_BUDGET
    off_count = (off_hidden + PROP_BUDGET + 3) & ~3
    n = struct.unpack_from(">i", d, off_count)[0]
    drawn = set()
    hidden = set()
    for i in range(n):
        p = struct.unpack_from(">H", d, off_prop + 2 * i)[0]
        (hidden if d[off_hidden + i] else drawn).add(p)
    # the camera's axes
    rx = look[1] * up[2] - look[2] * up[1]
    ry = look[2] * up[0] - look[0] * up[2]
    rz = look[0] * up[1] - look[1] * up[0]
    rl = math.sqrt(rx * rx + ry * ry + rz * rz)
    rx, ry, rz = rx / rl, ry / rl, rz / rl
    ux = ry * look[2] - rz * look[1]
    uy = rz * look[0] - rx * look[2]
    uz = rx * look[1] - ry * look[0]
    tv = math.tan(math.radians(fovy / 2))
    th = tv * 4 / 3
    missing = []
    should = 0
    for i in range(count):
        x, y, z, kind, yaw, scale = struct.unpack_from(">hhhBBB", d, (props & 0xFFFFFF) + 12 * i)
        if kind >= KIND_MAX:
            continue
        inf = info[kind]
        k = scale / 64.0
        dx, dz = x - eye[0], z - eye[2]
        dist = math.sqrt(dx * dx + dz * dz)
        if dist > kindMax[kind] * 0.9:
            continue
        cx, cz = int((x - x0) // chunk), int((z - z0) // chunk)
        if 0 <= cx < width and 0 <= cz < depth and d[(dry & 0xFFFFFF) + cz * width + cx] and (inf["floating"] or inf["plant"]):
            continue
        if (not under) and inf["imp"] and dist > BEDCARD_DIST * 0.95:
            continue  # (may be in its chunk's bed card)
        h = inf["height"] * k
        mid = y + inf["extent"] * k * 0.5
        dy = mid - eye[1]
        if inf["card"] and dist < 30.0 * k + 10.0 and y - 10 < eye[1] < y + h + 10:
            continue  # (the camera inside it: fishmod_scatter.c NEAR_SKIP_R)
        zc = dx * look[0] + dy * look[1] + dz * look[2]
        if zc < 30:
            continue
        sx = (dx * rx + dy * ry + dz * rz) / (th * zc)
        sy = (dx * ux + dy * uy + dz * uz) / (tv * zc)
        if abs(sx) > 0.9 or abs(sy) > 0.9:
            continue  # (its middle well inside the picture only)
        should += 1
        if i not in drawn:
            missing.append((dist, sx, sy, kind, i, i in hidden))
    print("eye (%.0f, %.0f, %.0f) %s; set: %d props (%d hidden); should be on screen: %d; missing: %d%s" % (
        eye[0], eye[1], eye[2], "under water" if under else "above water", n, len(hidden), should, len(missing),
        " (the budget full)" if n >= PROP_BUDGET else ""))
    for dist, sx, sy, kind, i, hid in sorted(missing)[:40]:
        print("  prop %5d kind %2d  %5.0f away  screen (%+.2f, %+.2f)%s" % (i, kind, dist, sx, sy,
                                                                       "  [marked hidden]" if hid else ""))
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
