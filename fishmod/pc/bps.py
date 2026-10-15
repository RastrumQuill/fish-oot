"""Applies a .bps patch (the format the mod's patches come in), with the checksum checks, in plain Python."""
import zlib


def to_z64(rom):
    """The ROM in big-endian (.z64) byte order, whatever it came in (.z64, .v64 byte-swapped, .n64 little-endian)."""
    head = rom[:4]
    if head == b"\x80\x37\x12\x40":
        return rom
    if head == b"\x37\x80\x40\x12":  # .v64
        b = bytearray(rom)
        b[0::2], b[1::2] = rom[1::2], rom[0::2]
        return bytes(b)
    if head == b"\x40\x12\x37\x80":  # .n64
        b = bytearray(len(rom))
        for k in range(4):
            b[k::4] = rom[3 - k::4]
        return bytes(b)
    raise ValueError("not an N64 ROM (unknown header)")


def apply(source, patch):
    if patch[:4] != b"BPS1":
        raise ValueError("not a BPS patch")
    i = 4

    def vint():
        nonlocal i
        data, shift = 0, 1
        while True:
            x = patch[i]
            i += 1
            data += (x & 0x7F) * shift
            if x & 0x80:
                return data
            shift <<= 7
            data += shift

    src_size, dst_size, meta_size = vint(), vint(), vint()
    i += meta_size
    src_crc, dst_crc, patch_crc = (int.from_bytes(patch[len(patch) - 12 + 4 * k:len(patch) - 8 + 4 * k], "little")
                                   for k in range(3))
    if zlib.crc32(patch[:-4]) != patch_crc:
        raise ValueError("the patch file is damaged")
    if len(source) != src_size or zlib.crc32(source) != src_crc:
        raise ValueError("this patch is for a different ROM: it needs Ocarina of Time NTSC 1.2 (US)")
    out = bytearray(dst_size)
    o = src_rel = dst_rel = 0
    end = len(patch) - 12
    while i < end:
        data = vint()
        cmd, length = data & 3, (data >> 2) + 1
        if cmd == 0:  # source read
            out[o:o + length] = source[o:o + length]
            o += length
        elif cmd == 1:  # target read
            out[o:o + length] = patch[i:i + length]
            i += length
            o += length
        else:
            d = vint()
            off = -(d >> 1) if d & 1 else d >> 1
            if cmd == 2:  # source copy
                src_rel += off
                out[o:o + length] = source[src_rel:src_rel + length]
                src_rel += length
                o += length
            else:  # target copy (may overlap itself: byte by byte)
                dst_rel += off
                for _ in range(length):
                    out[o] = out[dst_rel]
                    o += 1
                    dst_rel += 1
    if zlib.crc32(out) != dst_crc:
        raise ValueError("patching went wrong (target checksum)")
    return bytes(out)
