#!/usr/bin/env python3
"""The input recorder's PC side (src/code/fishmod_debug_inputrec.inc.c; fishmod/docs/input-replay.md).

  inputrec.py join DIR [DIR...] -o REC.finp     the packets a recording build sent (`sc64deployer debug`'s or the
                                                patched ares's binaryout-*.bin files) joined into one recording
  inputrec.py info REC.finp [-v]                what's in a recording: its length, each game state and scene
  inputrec.py attach ROM.z64 REC.finp OUT.z64   a replay build's ROM with the recording attached (64 MB)
  inputrec.py cut REC.finp OUT.finp --states N  the recording's first N game states (a short one for a test)

A recording: 'FINP', 1, the save memory's bytes, the stream's words, the save context's words, three spare words; the
save memory as the game found it at power-on; the stream (32-bit big-endian words, described in the .inc.c).
"""
import argparse
import glob
import os
import re
import struct
import sys

ROM_AT = 0x03000000
ROM_SIZE = 0x04000000
TAG, REPEAT = 0xFFFE0000, 0xFFFF0000
SEG, SYNC, END = 1, 2, 0xFF
STATES = ["setup", "map select", "logo", "play", "title setup", "file select"]


def read_packets(folders):
    """The 'FINP' packets' words in order and the 'FSRM' save memory, from folders of binaryout-*.bin."""
    words, sram, lost, seq_next, gaps, save_words = [], {}, 0, 0, 0, 0
    for folder in folders:
        names = sorted(glob.glob(os.path.join(folder, "binaryout-*.bin")),
                       key=lambda n: int(re.search(r"(\d+)\.bin$", n).group(1)))
        if not names:
            sys.exit(f"{folder}: no binaryout-*.bin files")
        for name in names:
            data = open(name, "rb").read()
            if len(data) < 16:
                continue
            magic, a, b, c = struct.unpack(">4I", data[:16])
            if magic == 0x4653524D:  # 'FSRM'
                sram[a] = data[16:16 + 0x1000]
                save_words = c
            elif magic == 0x46494E50:  # 'FINP'
                if a != seq_next:
                    gaps += 1
                    print(f"warning: packet {seq_next} missing before {os.path.basename(name)} (got {a})")
                seq_next = a + 1
                lost = c
                words += struct.unpack(f">{b}I", data[16:16 + 4 * b])
    if len(sram) != 8:
        sys.exit(f"the save memory is incomplete ({len(sram)} of 8 packets): was the capture running before power-on?")
    return words, b"".join(sram[i] for i in range(8)), lost, gaps, save_words


def walk(words):
    """Yields ('input', word, frames) | ('seg', state, number, rng, runs) | ('sync', frame, rng, pos, where) | ('end',)."""
    i, n = 0, len(words)
    while i < n:
        w = words[i]
        if w & 0xFFFF0000 == TAG:
            if i + 1 >= n:
                return
            kind, count = w & 0xFF, words[i + 1]
            body = words[i + 2:i + 2 + count]
            if len(body) < count:
                return  # (cut off mid-block)
            i += 2 + count
            if kind == SEG:
                yield ("seg", body[0], body[1], body[2], body[3:])
            elif kind == SYNC:
                yield ("sync", body[0], body[1], struct.unpack(">3f", struct.pack(">3I", *body[2:5])), body[5])
            elif kind == END:
                yield ("end",)
                return
        elif w & 0xFFFF0000 == REPEAT:
            yield ("repeat", w & 0xFFFF)
            i += 1
        else:
            yield ("input", w)
            i += 1


def complete(words):
    """The stream up to its last whole item, without an end block."""
    i, n, good = 0, len(words), 0
    while i < n:
        w = words[i]
        if w & 0xFFFF0000 == TAG:
            if i + 1 >= n or i + 2 + words[i + 1] > n or (w & 0xFF) == END:
                break
            i += 2 + words[i + 1]
        else:
            i += 1
        good = i
    return list(words[:good])


def load(path):
    data = open(path, "rb").read()
    magic, version, sram_bytes, nwords, save_words = struct.unpack(">5I", data[:20])
    if magic != 0x46494E50 or version != 1:
        sys.exit(f"{path}: not a recording")
    sram = data[32:32 + sram_bytes]
    words = struct.unpack(f">{nwords}I", data[32 + sram_bytes:32 + sram_bytes + 4 * nwords])
    return sram, list(words), save_words


def store(path, sram, words, save_words):
    with open(path, "wb") as f:
        f.write(struct.pack(">8I", 0x46494E50, 1, len(sram), len(words), save_words, 0, 0, 0))
        f.write(sram)
        f.write(struct.pack(f">{len(words)}I", *words))


def summary(words, verbose=False, save_offsets=None):
    frames = seg_frames = 0
    segs = []
    save = {}
    for item in walk(words):
        if item[0] == "input":
            frames += 1
            seg_frames += 1
        elif item[0] == "repeat":
            frames += item[1]
            seg_frames += item[1]
        elif item[0] == "seg":
            if segs:
                segs[-1]["frames"] = seg_frames
            runs = item[4]
            k = 0
            while k < len(runs):
                at, n = runs[k] >> 16, runs[k] & 0xFFFF
                for j in range(n):
                    save[at + j] = runs[k + 1 + j]
                k += 1 + n
            segs.append({"state": item[1], "at": frames, "frames": 0, "entrance": save.get(0, 0), "scene": None})
            seg_frames = 0
        elif item[0] == "sync" and segs and segs[-1]["scene"] is None:
            segs[-1]["scene"] = (item[4] >> 8) & 0xFF
    if segs:
        segs[-1]["frames"] = seg_frames
    return frames, segs


def scene_names():
    here = os.path.dirname(os.path.abspath(__file__))
    try:
        text = open(os.path.join(here, "..", "..", "include", "tables", "scene_table.h")).read()
    except OSError:
        return []
    return re.findall(r"DEFINE_SCENE\(\s*\w+,\s*\w+,\s*(SCENE_\w+)", text)


def cmd_join(args):
    words, sram, lost, gaps, save_words = read_packets(args.dirs)
    words = complete(words)
    words += [TAG | END, 0]
    store(args.out, sram, words, save_words)
    frames, segs = summary(words)
    print(f"{args.out}: {frames} frames ({frames / 20 / 60:.1f} minutes at 20 a second), {len(segs)} game states, "
          f"{len(words) * 4 / 1024:.0f} KB")
    if lost or gaps:
        print(f"WARNING: {lost} words dropped by the game (USB not keeping up) and {gaps} packets missing: the replay "
              f"will go wrong where they were")


def cmd_info(args):
    sram, words, save_words = load(args.rec)
    frames, segs = summary(words)
    names = scene_names()
    print(f"{args.rec}: {frames} frames ({frames / 20 / 60:.1f} minutes), {len(segs)} game states, "
          f"save context {save_words} words")
    plays = [s for s in segs if s["state"] == 3]
    scenes = sorted({s["scene"] for s in plays if s["scene"] is not None})
    print(f"{len(plays)} scenes loaded, {len(scenes)} different ones"
          + (f" of {len(names)}" if names else ""))
    if args.verbose:
        for n, s in enumerate(segs):
            state = STATES[s["state"]] if 0 <= s["state"] < len(STATES) else str(s["state"])
            scene = ""
            if s["scene"] is not None:
                scene = names[s["scene"]] if s["scene"] < len(names) else f"scene {s['scene']}"
            print(f"{n:5d}  {s['at'] / 1200:7.1f} min  {s['frames']:6d} frames  {state:12s} {scene}")
    elif names:
        missing = [names[i] for i in range(len(names)) if i not in scenes]
        print("not visited: " + (", ".join(missing) if missing else "none"))


def cmd_attach(args):
    rom = bytearray(open(args.rom, "rb").read())
    rec = open(args.rec, "rb").read()
    if len(rom) > ROM_AT:
        sys.exit(f"{args.rom} is bigger than {ROM_AT:#x}: already has a recording?")
    if ROM_AT + len(rec) > ROM_SIZE:
        sys.exit("the recording doesn't fit in a 64 MB ROM")
    rom += b"\xFF" * (ROM_AT - len(rom))
    rom += rec
    rom += b"\xFF" * (ROM_SIZE - len(rom))
    open(args.out, "wb").write(rom)
    print(f"{args.out}: {len(rec) / 1024:.0f} KB of recording at {ROM_AT:#x}")


def cmd_cut(args):
    sram, words, save_words = load(args.rec)
    out, i, segs = [], 0, 0
    words = complete(words)
    while i < len(words):
        w = words[i]
        if w & 0xFFFF0000 == TAG:
            if (w & 0xFF) == SEG:
                segs += 1
                if segs > args.states:
                    break
            n = 2 + words[i + 1]
        else:
            n = 1
        out += words[i:i + n]
        i += n
    out += [TAG | END, 0]
    store(args.out, sram, out, save_words)
    frames, s = summary(out)
    print(f"{args.out}: {frames} frames, {len(s)} game states")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("join")
    p.add_argument("dirs", nargs="+")
    p.add_argument("-o", "--out", required=True)
    p.set_defaults(fn=cmd_join)
    p = sub.add_parser("info")
    p.add_argument("rec")
    p.add_argument("-v", "--verbose", action="store_true")
    p.set_defaults(fn=cmd_info)
    p = sub.add_parser("attach")
    p.add_argument("rom")
    p.add_argument("rec")
    p.add_argument("out")
    p.set_defaults(fn=cmd_attach)
    p = sub.add_parser("cut")
    p.add_argument("rec")
    p.add_argument("out")
    p.add_argument("--states", type=int, required=True)
    p.set_defaults(fn=cmd_cut)
    args = ap.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
