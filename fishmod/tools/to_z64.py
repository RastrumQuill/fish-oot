#!/usr/bin/env python3
"""Converts an N64 ROM in any byte order (.z64, .v64, .n64) to big-endian .z64."""

import sys

Z64_MAGIC = bytes([0x80, 0x37, 0x12, 0x40])
V64_MAGIC = bytes([0x37, 0x80, 0x40, 0x12])
N64_MAGIC = bytes([0x40, 0x12, 0x37, 0x80])


def to_z64(data: bytes) -> bytes:
    magic = data[:4]
    if magic == Z64_MAGIC:
        return data
    out = bytearray(len(data))
    if magic == V64_MAGIC:
        out[0::2] = data[1::2]
        out[1::2] = data[0::2]
    elif magic == N64_MAGIC:
        out[0::4] = data[3::4]
        out[1::4] = data[2::4]
        out[2::4] = data[1::4]
        out[3::4] = data[0::4]
    else:
        raise SystemExit("error: not an N64 ROM (unknown header)")
    return bytes(out)


def main():
    if len(sys.argv) != 3:
        raise SystemExit(f"usage: {sys.argv[0]} <in-rom> <out.z64>")
    with open(sys.argv[1], "rb") as f:
        data = f.read()
    with open(sys.argv[2], "wb") as f:
        f.write(to_z64(data))


if __name__ == "__main__":
    main()
