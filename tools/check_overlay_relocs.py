#!/usr/bin/env python3
"""Checks that an object's relocations come out right through the game's overlay relocation (Overlay_Relocate,
src/libu64/relocation_gc.c; fado copies the relocations in the object's order): the loader pairs each R_MIPS_LO16 with
the last R_MIPS_HI16 *in the same register* (the lui's rt, the lo instruction's rs), in the relocations' order. So a
HI16 must be followed, before any other HI16 in its register, by the LO16s that belong to it, and every HI16 must get
at least one LO16 (else its lui is never relocated). Pairs are checked by symbol: a LO16 paired with a HI16 for another
symbol is wrong.

IDO complies; GCC needs -mno-explicit-relocs -mno-split-addresses; clang is checked here.

Usage: tools/check_overlay_relocs.py OBJECT...   (all of one overlay's objects; exit status 1 if any breaks the rule)
"""
import struct
import sys

R_MIPS_HI16, R_MIPS_LO16 = 5, 6
SHT_SYMTAB, SHT_REL, SHT_RELA = 2, 9, 4


def elf(path):
    d = open(path, "rb").read()
    shoff, = struct.unpack_from(">I", d, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from(">HHH", d, 0x2E)
    secs = [struct.unpack_from(">10I", d, shoff + i * shentsize) for i in range(shnum)]
    return d, secs, shstrndx


def symbols(d, secs):
    """[(name, defined)] by index, from the object's symbol table."""
    out = []
    for sec in secs:
        if sec[1] != SHT_SYMTAB:
            continue
        strtab = secs[sec[6]]
        for i in range(sec[5] // 16):
            st_name, _, _, _, _, shndx = struct.unpack_from(">IIIBBH", d, sec[4] + i * 16)
            s = strtab[4] + st_name
            out.append((d[s:d.index(b"\0", s)].decode(), shndx != 0))
    return out


def defined_names(paths):
    names = set()
    for p in paths:
        d, secs, _ = elf(p)
        names.update(n for n, defined in symbols(d, secs) if defined and n)
    return names


def check(path, overlay_names):
    d = open(path, "rb").read()
    if d[:4] != b"\x7fELF" or d[4] != 1 or d[5] != 2:
        return [f"{path}: not a 32-bit big-endian ELF"]
    shoff, = struct.unpack_from(">I", d, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from(">HHH", d, 0x2E)
    secs = [struct.unpack_from(">10I", d, shoff + i * shentsize) for i in range(shnum)]
    names = secs[shstrndx]
    syms = symbols(d, secs)

    def internal(sym):
        """Relocated at run time (fado keeps it): defined in this object or in another file of the overlay. A symbol in
        the game's code is resolved when the overlay is linked and never reaches the loader."""
        n, defined = syms[sym]
        return defined or n in overlay_names

    def name(off):
        s = names[4] + off
        return d[s:d.index(b"\0", s)].decode()

    problems = []
    for sec in secs:
        if sec[1] == SHT_RELA:
            problems.append(f"{path}: {name(sec[0])}: RELA relocations (the loader takes REL only)")
            continue
        if sec[1] != SHT_REL:
            continue
        tsec = secs[sec[7]]
        target = name(tsec[0])

        def insn(off):
            return struct.unpack_from(">I", d, tsec[4] + off)[0]

        pending = {}  # register -> [symbol, offset, used]: the last HI16 in it, as the loader keeps it
        for k in range(sec[5] // 8):
            off, info = struct.unpack_from(">II", d, sec[4] + k * 8)
            typ, sym = info & 0xFF, info >> 8
            if typ in (R_MIPS_HI16, R_MIPS_LO16) and not internal(sym):
                continue
            if typ == R_MIPS_HI16:
                rt = (insn(off) >> 16) & 0x1F
                old = pending.get(rt)
                if old is not None and not old[2]:
                    problems.append(f"{path}: {target}+{old[1]:#x}: HI16 in ${rt} replaced before any LO16 used it")
                pending[rt] = [sym, off, False]
            elif typ == R_MIPS_LO16:
                rs = (insn(off) >> 21) & 0x1F
                hi = pending.get(rs)
                if hi is None:
                    problems.append(f"{path}: {target}+{off:#x}: LO16 with no HI16 in ${rs} before it")
                elif hi[0] != sym:
                    problems.append(f"{path}: {target}+{off:#x}: LO16 paired with the HI16 at {hi[1]:#x} in ${rs}, "
                                    "for another symbol")
                else:
                    hi[2] = True
        for rt, hi in pending.items():
            if not hi[2]:
                problems.append(f"{path}: {target}+{hi[1]:#x}: HI16 in ${rt} that no LO16 uses")
    return problems


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    problems = []
    overlay_names = defined_names(sys.argv[1:])  # (give all of one overlay's objects together)
    for p in sys.argv[1:]:
        problems += check(p, overlay_names)
    for p in problems:
        print(p)
    print(f"{len(sys.argv) - 1} objects, {len(problems)} problems")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
