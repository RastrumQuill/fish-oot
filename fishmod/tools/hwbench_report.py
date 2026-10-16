#!/usr/bin/env python3
"""Reads the hardware bench's results (fishmod/scripts/hwbench.inc.c, SRAM 0x0000 of the save file) and prints a
report: per tour stop and mode, the frame rate, where the frame's time goes (CPU, RSP, RDP), the RDP's own counters
(command buffer busy / pipeline busy / TMEM loads, as shares of its cycles), and what each mode changes against the
build as shipped; then the memory tests (latency and bandwidth, with the RDP idle and busy).

Usage: fishmod/tools/hwbench_report.py save.sav [--md out.md]
"""
import argparse
import struct

MODES_V = {
    1: ["base", "tex4", "cold", "noprops", "nofish"],
    2: ["base", "altedge", "nozupd", "1cycle", "nonear", "nofar", "noprops", "nosky", "noxlu"],
    3: ["base", "altedge", "trimfar", "nonear", "nofar", "noprops", "nosky", "noxlu"],
    4: ["base", "altedge", "trimfar", "notri", "nostrict", "nonear", "nofar", "noprops", "nosky"],
}
MODE_DOC = {
    "base": "as shipped",
    "tex4": "props' textures in 4 bits (half the memory per texture load)",
    "cold": "caches filled with other data before each update (CPU work from RDRAM, not the cache)",
    "noprops": "props not drawn (what the props cost)",
    "nofish": "fish not drawn (what the fish cost to draw)",
    "notri": "the small plants' cut outlines instead of one triangle per card",
    "nostrict": "the nearest props by band and kind, not strictly nearest first",
    "trimfar": "far props cut to their outlines too, as before (what the uncut far models save)",
    "altedge": "props with the other edge mode (anti-aliased in the RDP, reading the colour buffer, or by the VI only)",
    "nozupd": "props without depth writes",
    "1cycle": "props in 1-cycle mode, no fog or tint (is the RDP's pipeline or its memory the limit?)",
    "nonear": "only the props past 500 units",
    "nofar": "only the props within 500 units",
    "nosky": "no props, no skybox",
    "noxlu": "no props, no room translucency (the water surface)",
}
METRICS = ["period", "cpu_update", "cpu_draw", "rsp_gfx", "rdp", "rsp_audio", "dp_clock", "dp_buf", "dp_pipe",
           "dp_tmem", "scatter", "fish_upd", "fish_draw", "props"]
TESTS = [("chase", "load latency, cached miss", "ns/load"), ("chase_uncached", "load latency, uncached", "ns/load"),
         ("read", "read 64 KB cached", "MB/s"), ("read_uncached", "read 16 KB uncached", "MB/s"),
         ("write", "write 64 KB + writeback", "MB/s"), ("copy", "bcopy 64 KB", "MB/s")]
COUNT_HZ = 46_875_000  # the CPU's count register
STOP_NAMES = [
    "Across the west flat (under)", "Straight down (under)", "North beach", "Beach shallows (under)", "Pillar ruins",
    "Pillar ruins (under)", "Lab shore and dock", "Gerudo river inflow", "Rock garden (under)", "West weed flat",
    "West weed flat (under)", "The drop-off (under)", "Deep basin (under)", "Water Temple gate", "South bridge",
    "Island shallows (under)", "Island shallows", "East weed flats", "East weed flats (under)", "South reed bank",
    "Under the lilies", "East shore"]


def read(path):
    d = open(path, "rb").read()
    magic, version, stops, modes, metrics, tests, hw = struct.unpack(">IBBBBBB", d[:10])
    if len(d) >= 0x7010 and d[0x7000:0x7004] == b"FBCR":
        laps, = struct.unpack(">I", d[0x7004:0x7008])
        cstop, cmode, cblock = d[0x7008], d[0x7009], d[0x700A]
        count, = struct.unpack(">I", d[0x700C:0x7010])
        names = MODES_V.get(version, MODES_V[max(MODES_V)])
        print(f"(last breadcrumb: lap {laps + 1}, stop {cstop + 1}, mode {names[cmode] if cmode < len(names) else cmode}, "
              f"block {cblock}; {count} written)")
    if magic != 0x4642454E:
        raise SystemExit(f"no bench results in {path}")
    laps, visits = struct.unpack(">II", d[12:20])
    o = 20
    cells = []
    for s in range(stops):
        row = []
        for m in range(modes):
            frames = struct.unpack(">I", d[o:o + 4])[0]
            sums = struct.unpack(f">{metrics}I", d[o + 4:o + 4 + 4 * metrics])
            maxs = struct.unpack(f">{metrics}I", d[o + 4 + 4 * metrics:o + 4 + 8 * metrics])
            row.append((frames, sums, maxs))
            o += 4 + 8 * metrics
        cells.append(row)
    micro = []
    for t in range(tests):
        ctx = []
        for b in range(2):
            ctx.append(struct.unpack(">III", d[o:o + 12]))
            o += 12
        micro.append(ctx)
    return dict(version=version, hw=hw, laps=laps, visits=visits, cells=cells, micro=micro)


def avg(cell, name):
    frames, sums, _ = cell
    return sums[METRICS.index(name)] / frames if frames else 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("save")
    ap.add_argument("--md")
    args = ap.parse_args()
    r = read(args.save)
    out = []
    p = out.append
    p(f"# Hardware bench ({'original-hardware build' if r['hw'] else 'emulator build'})\n")
    MODES = MODES_V[r["version"]]
    p(f"{r['laps']} full laps, {r['visits']} stop visits.\n")
    p("Modes: " + "; ".join(f"**{m}**: {MODE_DOC[m]}" for m in MODES) + ".\n")
    p("RDP columns are shares of its cycles: **buf** = command buffer busy (fetching from RDRAM, or held up), "
      "**pipe** = drawing, **tmem** = loading textures.\n")

    totals = {m: [0.0, 0, 0.0] for m in MODES}
    p("| stop | mode | fps | CPU upd | CPU draw | RSP | RDP ms | buf | pipe | tmem | props | worst frame |")
    p("|---|---|---|---|---|---|---|---|---|---|---|---|")
    for s, row in enumerate(r["cells"]):
        for m, cell in enumerate(row):
            if not cell[0]:
                continue
            period = avg(cell, "period")
            clock = avg(cell, "dp_clock")
            pct = lambda n: f"{100 * avg(cell, n) / clock:.0f}%" if clock else "-"
            totals[MODES[m]][0] += period
            totals[MODES[m]][1] += 1
            totals[MODES[m]][2] += avg(cell, "rdp")
            worst = cell[2][METRICS.index("period")] / 1000
            p(f"| {s + 1} {STOP_NAMES[s] if m == 0 else ''} | {MODES[m]} | {1e6 / period:.1f} | "
              f"{avg(cell, 'cpu_update') / 1000:.1f} | {avg(cell, 'cpu_draw') / 1000:.1f} | "
              f"{avg(cell, 'rsp_gfx') / 1000:.1f} | {avg(cell, 'rdp') / 1000:.1f} | {pct('dp_buf')} | "
              f"{pct('dp_pipe')} | {pct('dp_tmem')} | {avg(cell, 'props'):.0f} | {worst:.0f} ms |")

    p("\n## Each mode against the build as shipped (all stops)\n")
    p("Frame times stop at the 20 fps cap (50 ms), so the RDP's own time shows the modes' cost better.\n")
    p("| mode | mean frame | change | mean RDP | change |")
    p("|---|---|---|---|---|")
    base = totals["base"][0] / max(totals["base"][1], 1)
    base_rdp = totals["base"][2] / max(totals["base"][1], 1)
    rdp = {}
    for m in MODES:
        t, n, tr = totals[m]
        if n:
            mean = t / n
            rdp[m] = tr / n
            p(f"| {m} | {mean / 1000:.1f} ms ({1e6 / mean:.1f} fps) | {100 * (mean - base) / base:+.1f}% | "
              f"{rdp[m] / 1000:.1f} ms | {100 * (rdp[m] - base_rdp) / base_rdp:+.1f}% |")
    if "nosky" in rdp and "noprops" in rdp and "noxlu" in rdp:
        p("\n## Where the RDP's time goes (mean over the stops)\n")
        p(f"- props: {(rdp['base'] - rdp['noprops']) / 1000:.1f} ms; those within 500 units "
          f"{(rdp['base'] - rdp['nonear']) / 1000:.1f} ms, those past it {(rdp['base'] - rdp['nofar']) / 1000:.1f} ms")
        p(f"- skybox: {(rdp['noprops'] - rdp['nosky']) / 1000:.1f} ms")
        p(f"- room translucency (the water surface): {(rdp['noprops'] - rdp['noxlu']) / 1000:.1f} ms")
        p(f"- the rest (terrain, actors, fish, the HUD): {rdp['nosky'] / 1000:.1f} ms less the water's share")

    p("\n## Memory (CPU count cycles: 46.875 MHz)\n")
    p("| test | RDP idle | RDP busy | samples (idle/busy) |")
    p("|---|---|---|---|")
    for (key, doc, unit), ctx in zip(TESTS, r["micro"]):
        vals = []
        for count, total, mn in ctx:
            if not count:
                vals.append("-")
                continue
            mean = total / count
            if unit == "ns/load":
                vals.append(f"{mean * 1e9 / COUNT_HZ:.0f} ns (best {mn * 1e9 / COUNT_HZ:.0f})")
            else:
                # cycles per KB -> MB/s
                vals.append(f"{1024 * COUNT_HZ / mean / 1e6:.0f} MB/s (best {1024 * COUNT_HZ / mn / 1e6:.0f})")
        p(f"| {doc} | {vals[0]} | {vals[1]} | {ctx[0][0]}/{ctx[1][0]} |")
    text = "\n".join(out) + "\n"
    print(text)
    if args.md:
        open(args.md, "w").write(text)


if __name__ == "__main__":
    main()
