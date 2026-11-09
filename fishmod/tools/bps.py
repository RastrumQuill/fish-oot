#!/usr/bin/env python3
"""BPS patches (the format Flips makes), with no outside tool: create one from a source and a target, or apply one.

The patch copies the source wherever the target matches it at the same place (SourceRead), or anywhere else in the
source (SourceCopy: the ROM's files move when one before them grows), and the target's own bytes elsewhere, also
borrowing from earlier in the target where a stretch repeats (TargetCopy). Any BPS patcher applies it (Flips, the
online patcher).

Usage:
  fishmod/tools/bps.py --create --bps SOURCE TARGET PATCH   (Flips' own argument order)
  fishmod/tools/bps.py --apply PATCH SOURCE OUT
"""
import sys
import zlib

SOURCE_READ, TARGET_READ, SOURCE_COPY, TARGET_COPY = 0, 1, 2, 3
MIN_SAME = 16   # a run of equal bytes shorter than this goes as target bytes (each action costs a few bytes)
BLOCK = 32      # TargetCopy: the stretch length its search indexes
SRC_KEY = 16    # SourceCopy: the source is indexed by the SRC_KEY bytes at every SRC_KEY-th offset, so a match at any
                # shift is found within SRC_KEY bytes of its start (then extended back over them)
MIN_COPY = 32   # a SourceCopy shorter than this isn't worth its offset


def varint(n):
    out = bytearray()
    while True:
        x = n & 0x7F
        n >>= 7
        if n == 0:
            out.append(0x80 | x)
            return out
        out.append(x)
        n -= 1


def read_varint(data, pos):
    n = 0
    shift = 1
    while True:
        x = data[pos]
        pos += 1
        n += (x & 0x7F) * shift
        if x & 0x80:
            return n, pos
        shift <<= 7
        n += shift


def signed(n):
    return varint((abs(n) << 1) | (1 if n < 0 else 0))


def match_len(a, ai, b, bi, limit):
    """How many bytes a[ai:] and b[bi:] share, up to limit (compared in chunks: the patch has long matches)."""
    n = 0
    step = 4096
    while step:
        while n + step <= limit and a[ai + n:ai + n + step] == b[bi + n:bi + n + step]:
            n += step
        step //= 8
    while n < limit and a[ai + n] == b[bi + n]:
        n += 1
    return n


def create(source, target):
    out = bytearray(b"BPS1")
    out += varint(len(source)) + varint(len(target)) + varint(0)
    source = bytes(source)
    target = bytes(target)
    src_index = {}  # the source's SRC_KEY bytes at each SRC_KEY-th offset -> the first offset holding them
    for k in range(0, len(source) - SRC_KEY + 1, SRC_KEY):
        src_index.setdefault(source[k:k + SRC_KEY], k)
    seen = {}  # a target block's bytes -> where it first started (for TargetCopy)
    rel = {"source": 0, "target": 0}
    lit = bytearray()

    def flush_literal():
        if lit:
            out.extend(varint(((len(lit) - 1) << 2) | TARGET_READ))
            out.extend(lit)
            lit.clear()

    i = 0
    n = len(target)
    while i < n:
        # the source at the same place
        if i < len(source) and source[i] == target[i]:
            j = match_len(source, i, target, i, min(n, len(source)) - i)
            if j >= MIN_SAME:
                flush_literal()
                out.extend(varint(((j - 1) << 2) | SOURCE_READ))
                i += j
                continue
        # the same bytes elsewhere in the source: try each of the SRC_KEY alignments the index can see
        best = None
        for d in range(SRC_KEY):
            at = src_index.get(target[i + d:i + d + SRC_KEY])
            if at is None or at < d:
                continue
            s0 = at - d  # where target[i] would sit in the source
            j = match_len(source, s0, target, i, min(n - i, len(source) - s0))
            if j >= MIN_COPY and (best is None or j > best[1]):
                best = (s0, j)
            if best is not None:
                break
        if best is not None:
            s0, j = best
            # take back literal bytes the match also covers
            back = 0
            while back < len(lit) and s0 - back - 1 >= 0 and source[s0 - back - 1] == lit[-1 - back]:
                back += 1
            if back:
                del lit[len(lit) - back:]
            flush_literal()
            out.extend(varint(((j + back - 1) << 2) | SOURCE_COPY))
            out.extend(signed(s0 - back - rel["source"]))
            rel["source"] = s0 + j
            i += j
            continue
        # an earlier stretch of the target
        key = target[i:i + BLOCK]
        at = seen.get(key) if len(key) == BLOCK else None
        if at is not None and at < i:
            j = match_len(target, at, target, i, n - i)
            flush_literal()
            out.extend(varint(((j - 1) << 2) | TARGET_COPY))
            out.extend(signed(at - rel["target"]))
            rel["target"] = at + j
            for k in range(i, i + j, BLOCK):
                if k + BLOCK <= n:
                    seen.setdefault(target[k:k + BLOCK], k)
            i += j
            continue
        if len(key) == BLOCK and i % 4 == 0:
            seen.setdefault(key, i)
        lit.append(target[i])
        i += 1
    flush_literal()
    out += zlib.crc32(source).to_bytes(4, "little")
    out += zlib.crc32(target).to_bytes(4, "little")
    out += zlib.crc32(out).to_bytes(4, "little")
    return bytes(out)


def apply(patch, source):
    if patch[:4] != b"BPS1":
        raise ValueError("not a BPS patch")
    if zlib.crc32(patch[:-4]) != int.from_bytes(patch[-4:], "little"):
        raise ValueError("the patch is damaged (its checksum doesn't match)")
    pos = 4
    src_size, pos = read_varint(patch, pos)
    tgt_size, pos = read_varint(patch, pos)
    meta, pos = read_varint(patch, pos)
    pos += meta
    if len(source) != src_size or zlib.crc32(source) != int.from_bytes(patch[-12:-8], "little"):
        raise ValueError("this patch is for a different source file")
    out = bytearray()
    src_rel = tgt_rel = 0
    end = len(patch) - 12
    while pos < end:
        data, pos = read_varint(patch, pos)
        kind, length = data & 3, (data >> 2) + 1
        if kind == SOURCE_READ:
            out += source[len(out):len(out) + length]
        elif kind == TARGET_READ:
            out += patch[pos:pos + length]
            pos += length
        else:
            d, pos = read_varint(patch, pos)
            d = -(d >> 1) if d & 1 else d >> 1
            if kind == SOURCE_COPY:
                src_rel += d
                out += source[src_rel:src_rel + length]
                src_rel += length
            else:
                tgt_rel += d
                for _ in range(length):
                    out.append(out[tgt_rel])
                    tgt_rel += 1
    if len(out) != tgt_size or zlib.crc32(out) != int.from_bytes(patch[-8:-4], "little"):
        raise ValueError("the result doesn't match the patch's target")
    return bytes(out)


def main():
    a = sys.argv[1:]
    if a[:2] == ["--create", "--bps"] and len(a) == 5:
        source, target = open(a[2], "rb").read(), open(a[3], "rb").read()
        patch = create(source, target)
        if apply(patch, source) != target:
            sys.exit("bps.py: the patch made doesn't rebuild the target (a bug)")
        open(a[4], "wb").write(patch)
        print("The patch was created successfully!")
    elif a[:1] == ["--apply"] and len(a) == 4:
        open(a[3], "wb").write(apply(open(a[1], "rb").read(), open(a[2], "rb").read()))
        print("The patch was applied successfully!")
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
