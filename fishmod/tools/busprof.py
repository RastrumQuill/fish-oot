#!/usr/bin/env python3
"""The bytes moved to and from RDRAM, by who asked, from the file ares writes with ARES_BUSPROF=<file>
(fishmod/tools/ares-prof.patch): the CPU's cache fills and write-backs (the audio driver's own apart, by its
code's addresses), the RSP's DMAs (the audio microcode's apart), the cart, the audio and video interfaces, the RDP's
command fetch. Per game frame at 20 fps (50 ms), timed by the sound itself: the audio interface plays 4 bytes a
sample at 32,006 Hz. (The RDP's drawing isn't in it: ares draws on the GPU.)

    fishmod/tools/busprof.py range ELF            the audio driver's code range, as the two variables to run with
    ARES_BUSPROF=bus.txt ARES_BUSPROF_LO=... ARES_BUSPROF_HI=... ARES_RSPPROF_KEY=00400008 \\
        fishmod/tools/ares_run.sh 0 ROM 300
    fishmod/tools/busprof.py show bus.txt [other.txt ...]

Use a bench build that runs in real time (--script hwbench_play), not the audio check's (three updates a game frame
whatever the clock says).
"""
import re
import subprocess
import sys

AUDIO = re.compile(r"^(AudioThread_|AudioSeq_|AudioHeap_|AudioLoad_|AudioSynth_|AudioEffects_|Audio_|func_800D|"
                   r"func_800E|Fishmod_SampleStore)")
NAMES = [("cpu_code_audio", "CPU: the audio driver's code, filled"),
         ("cpu_data_audio", "CPU: the audio driver's data, filled / written back"),
         ("sp_audio", "RSP: the audio tasks' DMAs, in / out"),
         ("ai", "audio interface: the sound played"),
         ("cpu_code", "CPU: the rest's code"),
         ("cpu_data", "CPU: the rest's data"),
         ("cpu_uncached", "CPU: uncached"),
         ("sp_other", "RSP: the graphics tasks' DMAs"),
         ("dp_lists", "RDP: its commands fetched"),
         ("vi", "video interface: the picture read out"),
         ("pi", "cart"),
         ("si", "controllers")]


def code_range(elf):
    out = subprocess.run(["mips-linux-gnu-nm", "-n", "-S", elf], capture_output=True, text=True).stdout
    lo, hi = None, 0
    for line in out.splitlines():
        p = line.split()
        # (the driver proper: src/audio/internal, up to the sequence player's last function)
        if len(p) == 4 and p[2] in "tT" and AUDIO.match(p[3]):
            a = int(p[0], 16) & 0xFFFFFFFF
            if re.match(r"^(AudioThread_|AudioSeq_|AudioHeap_|AudioLoad_|AudioSynth_|AudioEffects_)", p[3]):
                lo = a if lo is None else min(lo, a)
                hi = max(hi, a + int(p[1], 16))
    return lo, hi


def load(path):
    d = {}
    for line in open(path):
        p = line.split()
        if len(p) == 5 and not line.startswith("#"):
            d[p[0]] = [int(x) for x in p[1:]]
    return d


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    if sys.argv[1] == "range":
        lo, hi = code_range(sys.argv[2])
        print("ARES_BUSPROF_LO=%x ARES_BUSPROF_HI=%x" % (lo, hi))
        return
    runs = [(p, load(p)) for p in sys.argv[2:]]
    print("KB per game frame (50 ms): read / written (transfers)")
    print("%-52s" % "" + "".join("%-34s" % p[-30:] for p, _ in runs))
    for key, label in NAMES:
        row = "%-52s" % label
        for _, d in runs:
            frames = d["ai"][0] / (32006 * 4) * 20 or 1
            r, rc, w, wc = d.get(key, [0, 0, 0, 0])
            n = d.get(key + "_dmas")  # (the RSP's rows: its DMAs; the CPU's: its cache lines)
            if n is not None:
                rc, wc = n[0], n[2]
            row += "%-34s" % ("%7.1f / %6.1f  (%5.0f / %5.0f)" % (r / frames / 1024, w / frames / 1024, rc / frames, wc / frames))
        print(row)
    row = "%-52s" % "the audio's (the first three rows)"
    for _, d in runs:
        frames = d["ai"][0] / (32006 * 4) * 20 or 1
        t = sum(d[k][0] + d[k][2] for k in ("cpu_code_audio", "cpu_data_audio", "sp_audio"))
        row += "%-34s" % ("%7.1f KB, %.0f s of sound" % (t / frames / 1024, d["ai"][0] / (32006 * 4)))
    print(row)


if __name__ == "__main__":
    main()
