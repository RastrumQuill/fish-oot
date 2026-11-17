#!/usr/bin/env python3
"""Writes fishmod/data/audio_text_order.txt (the order tools/mkspecrules.c links the audio driver's functions in)
from the audio sweep's CPU profiles: every function under src/audio/ that ran, the most-run first.

The instruction cache is 16 KB, direct-mapped: two functions 16 KB apart push each other out every time one calls
the other. Placed by how much they run, the functions that make up nearly all of an update sit in one stretch
shorter than the cache, so none of them can push another out, whatever the scene or the sound setting. (The order
this replaces was the order one update ran them in at Lake Hylia, in stereo: in headphones the note mixer's
helpers for the headphone effect and for streamed samples fell 16 KB from the mixer.)

    audio_text_order.py ELF cpu.bin [cpu.bin...] > fishmod/data/audio_text_order.txt

ELF and the profiles are one build's (audio_sweep.sh's OUT/rom.elf and OUT/*/cpu.bin).
"""
import bisect, collections, re, struct, subprocess, sys


def main():
    elf, profs = sys.argv[1], sys.argv[2:]
    syms = []
    for line in subprocess.run(["mips-linux-gnu-nm", "-n", "--defined-only", elf], capture_output=True, text=True).stdout.splitlines():
        p = line.split()
        if len(p) == 3 and p[1] in "tT" and not p[2].startswith(("$", ".L", "L8")):
            syms.append((int(p[0], 16) & 0xFFFFFFFF, p[2]))
    addrs = [a for a, _ in syms]
    files = subprocess.run(["mips-linux-gnu-addr2line", "-e", elf] + ["0x%x" % a for a in addrs], capture_output=True,
                           text=True).stdout.splitlines()
    count = collections.Counter()
    size = {}
    for pf in profs:
        d = open(pf, "rb").read()
        for i in range(0, len(d) - 27, 28):
            pc, n = struct.unpack_from("<II", d, i)
            k = bisect.bisect_right(addrs, pc) - 1
            if k < 0 or "/src/audio/" not in files[k]:
                continue
            name = re.sub(r"\.(part|isra|constprop|cold)\.\d+$", "", syms[k][1])
            count[name] += n
            size[syms[k][1]] = (addrs[k + 1] - addrs[k]) if k + 1 < len(addrs) else 0
    total = sum(count.values())
    # Who calls whom, from the code itself (jal and j between the driver's functions).
    lo, hi = min(a for a, f in zip(addrs, files) if "/src/audio/" in f), max(a for a, f in zip(addrs, files) if "/src/audio/" in f) + 0x4000
    dis = subprocess.run(["mips-linux-gnu-objdump", "-d", "--no-show-raw-insn", "--start-address=0x%x" % lo,
                          "--stop-address=0x%x" % hi, elf], capture_output=True, text=True).stdout
    base = lambda n: re.sub(r"\.(part|isra|constprop|cold)\.\d+$", "", n)
    calls = collections.defaultdict(set)
    for line in dis.splitlines():
        m = re.match(r"^([0-9a-f]{8}):\s+(jal|j)\s+([0-9a-f]{8})", line)
        if m:
            k = bisect.bisect_right(addrs, int(m.group(1), 16)) - 1
            t = bisect.bisect_right(addrs, int(m.group(3), 16)) - 1
            if k >= 0 and t >= 0 and addrs[t] == int(m.group(3), 16):
                calls[base(syms[k][1])].add(base(syms[t][1]))
    # The parts of an update, in the order one runs them: each part's functions together, the most-run first. A
    # part runs to its end before the next starts (three times an update), so only functions of the same part can
    # push each other out again and again; the mixing and the notes' upkeep, which loop over every note, come to
    # under 16 KB together and are placed first.
    parts = [("the mixing (the command list written)", "AudioSynth_DoOneAudioUpdate"),
             ("the notes' upkeep", "Audio_ProcessNotes"),
             ("the sequences", "AudioSeq_ProcessSequences")]
    placed = set()
    out = []
    hot = lambda n: count[n] * 20000 >= total  # (under 0.005% of the driver's instructions: left where it is)
    for title, root in parts:
        seen, todo = set(), [root]
        while todo:
            f = todo.pop()
            if f in seen:
                continue
            seen.add(f)
            todo.extend(calls.get(f, ()))
        names = sorted((n for n in seen if n in count and hot(n) and n not in placed), key=lambda n: -count[n])
        out.append(("# " + title, names))
        placed.update(names)
    out.append(("# the rest of what runs", [n for n, c in count.most_common() if hot(n) and n not in placed]))
    print("# The audio driver's functions in the order they are linked (tools/mkspecrules.c): the parts of an update")
    print("# one after another, each part's functions together and the most-run first, so that functions that call")
    print("# each other don't share lines of the 16 KB instruction cache. Written by fishmod/tools/audio_text_order.py")
    print("# from the audio sweep's profiles (%d of them) and the code's own calls: don't edit by hand." % len(profs))
    print("# A name ending in * also takes the compiler's own pieces of the function (.part.0, .isra.0 ...).")
    run = 0
    for title, names in out:
        print(title)
        for n in names:
            run += count[n]
            print(n + "*")
    sys.stderr.write("%d functions, %.2f%% of the driver's instructions\n" % (len(placed) + len(out[-1][1]), 100.0 * run / total))


if __name__ == "__main__":
    main()
