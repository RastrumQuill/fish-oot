#!/usr/bin/env python3
"""A small assembler and disassembler for the N64's RSP (its MIPS subset and the vector unit), for the audio
microcode: `dis` turns a microcode's text into source this assembles back to the same bytes; `asm` assembles a
source (the game's own, taken apart and changed) into text for the build.

    fishmod/tools/rspasm.py dis TEXT.bin [--base 0x1000] > out.s
    fishmod/tools/rspasm.py asm in.s -o TEXT.bin [--size N] [--sym OUT.txt]
    fishmod/tools/rspasm.py check TEXT.bin      (dis, asm, compare: the round trip)

Source: one instruction a line; `name:` labels; `;` or `#` comments; `.org ADDR`, `.word VALUE`, `.equ NAME, VALUE`,
`.fill ADDR` (zero words up to ADDR), `.include "file"`. Registers are $0-$31 and v0-v31; a vector operand's element
is v5[e3] (the instruction's 4-bit element field) and a load, store or move's v5[6] (its byte index). Branch and jump
targets are labels or addresses in IMEM (0x1000-0x1FFF); immediates take simple expressions (+, -, |, <<, labels,
.equ names).
"""
import argparse
import re
import struct
import sys

SPECIAL = {0: "sll", 2: "srl", 3: "sra", 4: "sllv", 6: "srlv", 7: "srav", 8: "jr", 9: "jalr", 13: "break", 32: "add",
           33: "addu", 34: "sub", 35: "subu", 36: "and", 37: "or", 38: "xor", 39: "nor", 42: "slt", 43: "sltu"}
IMM = {8: "addi", 9: "addiu", 10: "slti", 11: "sltiu", 12: "andi", 13: "ori", 14: "xori"}
MEM = {32: "lb", 33: "lh", 35: "lw", 36: "lbu", 37: "lhu", 40: "sb", 41: "sh", 43: "sw"}
BRANCH = {4: "beq", 5: "bne", 6: "blez", 7: "bgtz"}
VOPS = {0: "vmulf", 1: "vmulu", 2: "vrndp", 3: "vmulq", 4: "vmudl", 5: "vmudm", 6: "vmudn", 7: "vmudh", 8: "vmacf",
        9: "vmacu", 10: "vrndn", 11: "vmacq", 12: "vmadl", 13: "vmadm", 14: "vmadn", 15: "vmadh", 16: "vadd",
        17: "vsub", 19: "vabs", 20: "vaddc", 21: "vsubc", 29: "vsar", 32: "vlt", 33: "veq", 34: "vne", 35: "vge",
        36: "vcl", 37: "vch", 38: "vcr", 39: "vmrg", 40: "vand", 41: "vnand", 42: "vor", 43: "vnor", 44: "vxor",
        45: "vnxor", 48: "vrcp", 49: "vrcpl", 50: "vrcph", 51: "vmov", 52: "vrsq", 53: "vrsql", 54: "vrsqh",
        55: "vnop"}
VLS = ["bv", "sv", "lv", "dv", "qv", "rv", "pv", "uv", "hv", "fv", "wv", "tv"]
VSHIFT = [0, 1, 2, 3, 4, 4, 3, 3, 4, 4, 4, 4]
C2MOVE = {0: "mfc2", 2: "cfc2", 4: "mtc2", 6: "ctc2"}
R_SPECIAL = {v: k for k, v in SPECIAL.items()}
R_IMM = {v: k for k, v in IMM.items()}
R_MEM = {v: k for k, v in MEM.items()}
R_BRANCH = {v: k for k, v in BRANCH.items()}
R_VOPS = {v: k for k, v in VOPS.items()}
R_C2MOVE = {v: k for k, v in C2MOVE.items()}


def dis_one(w, pc, labels=None):
    """One instruction as text. pc is its IMEM address (0x1000-based); labels maps target addresses to names."""
    def tgt(a):
        return labels.get(a, "0x%04X" % a) if labels is not None else "0x%04X" % a
    op, rs, rt, rd, sa, fn = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31, w & 63
    imm = w & 0xFFFF
    simm = imm - 0x10000 if imm & 0x8000 else imm
    R = lambda n: "$%d" % n
    if w == 0:
        return "nop"
    if op == 0 and fn in SPECIAL and not (fn in (0, 2, 3) and rs) and not (fn in (4, 6, 7, 32, 33, 34, 35, 36, 37, 38, 39, 42, 43) and sa):
        n = SPECIAL[fn]
        if fn in (0, 2, 3):
            return "%s %s, %s, %d" % (n, R(rd), R(rt), sa)
        if fn in (4, 6, 7):
            return "%s %s, %s, %s" % (n, R(rd), R(rt), R(rs))
        if fn == 8 and not (rt or rd or sa):
            return "jr %s" % R(rs)
        if fn == 9 and not (rt or sa):
            return "jalr %s, %s" % (R(rd), R(rs))
        if fn == 13:
            return "break" if (w >> 6) == 0 else ".word 0x%08X" % w
        if fn >= 32:
            return "%s %s, %s, %s" % (n, R(rd), R(rs), R(rt))
    if op == 1 and rt in (0, 1, 16, 17):
        return "%s %s, %s" % ({0: "bltz", 1: "bgez", 16: "bltzal", 17: "bgezal"}[rt], R(rs), tgt(0x1000 | ((pc + 4 + simm * 4) & 0xFFF)))
    if op in (2, 3):
        a = (w & 0x3FFFFFF) << 2
        if 0x1000 <= a < 0x2000 or True:
            return "%s %s" % ("j" if op == 2 else "jal", tgt(a) if 0x1000 <= a < 0x2000 else "0x%X" % a)
    if op in BRANCH and not (op in (6, 7) and rt):
        t = tgt(0x1000 | ((pc + 4 + simm * 4) & 0xFFF))
        if op in (6, 7):
            return "%s %s, %s" % (BRANCH[op], R(rs), t)
        return "%s %s, %s, %s" % (BRANCH[op], R(rs), R(rt), t)
    if op in IMM:
        return "%s %s, %s, %s" % (IMM[op], R(rt), R(rs), simm if op < 12 else "0x%X" % imm)
    if op == 15 and rs == 0:
        return "lui %s, 0x%X" % (R(rt), imm)
    if op == 16 and rs in (0, 4) and not (w & 0x7FF):
        return "%s %s, c%d" % ("mfc0" if rs == 0 else "mtc0", R(rt), rd)
    if op in MEM:
        return "%s %s, %d(%s)" % (MEM[op], R(rt), simm, R(rs))
    if op == 18:
        if rs & 16:
            if fn in VOPS:
                return "%s v%d, v%d, v%d[e%d]" % (VOPS[fn], sa, rd, rt, rs & 15)
        elif rs in C2MOVE and not (w & 0x7F):
            return "%s %s, v%d[%d]" % (C2MOVE[rs], R(rt), rd, (w >> 7) & 15)
    if op in (50, 58) and rd < 12:
        off = w & 0x7F
        off = off - 128 if off & 0x40 else off
        return "%s%s v%d[%d], %d(%s)" % ("l" if op == 50 else "s", VLS[rd], rt, (w >> 7) & 15, off, R(rs))
    return ".word 0x%08X" % w


def disassemble(data, base=0x1000):
    words = struct.unpack(">%dI" % (len(data) // 4), data[:len(data) & ~3])
    labels = {}
    for i, w in enumerate(words):
        pc = base + i * 4
        op, rt = w >> 26, (w >> 16) & 31
        if op in (2, 3):
            a = (w & 0x3FFFFFF) << 2
            if base <= a < base + len(data):
                labels.setdefault(a, "L_%04X" % a)
        elif op in BRANCH or (op == 1 and rt in (0, 1, 16, 17)):
            simm = (w & 0xFFFF) - 0x10000 if w & 0x8000 else w & 0xFFFF
            a = 0x1000 | ((pc + 4 + simm * 4) & 0xFFF)
            if not dis_one(w, pc).startswith(".word") and base <= a < base + len(data):
                labels.setdefault(a, "L_%04X" % a)
    out = [".org 0x%04X" % base]
    for i, w in enumerate(words):
        pc = base + i * 4
        if pc in labels:
            out.append("%s:" % labels[pc])
        out.append("    %-40s ; %04X" % (dis_one(w, pc, labels), pc))
    return "\n".join(out) + "\n"


class AsmError(Exception):
    pass


def assemble(text, incdir="."):
    lines = []

    def load(src, name):
        for n, raw in enumerate(src.split("\n"), 1):
            line = re.split(r"[;#]", raw, maxsplit=1)[0].strip()
            m = re.match(r'\.include\s+"([^"]+)"', line)
            if m:
                import os
                path = os.path.join(incdir, m.group(1))
                load(open(path).read(), path)
            elif line:
                lines.append((name, n, line))

    load(text, "<source>")
    syms, equs = {}, {}

    def val(expr, pc=None, final=True):
        e = expr.strip()
        def rep(m):
            k = m.group(0)
            if k in equs:
                return str(equs[k])
            if k in syms:
                return str(syms[k])
            if not final:
                return "0"
            raise AsmError("unknown name %s" % k)
        e2 = re.sub(r"\b[A-Za-z_.][A-Za-z_0-9.]*\b", lambda m: m.group(0) if re.match(r"0[xX]", m.group(0)) else rep(m), e)
        if not re.match(r"^[0-9a-fA-FxX+\-|&<>*/()\s~]+$", e2):
            raise AsmError("bad expression %s" % expr)
        return int(eval(e2, {"__builtins__": {}}, {}))

    def reg(s):
        m = re.match(r"^\$(\d+)$", s.strip())
        if not m or int(m.group(1)) > 31:
            raise AsmError("bad register %s" % s)
        return int(m.group(1))

    def vreg(s, kind=None):
        m = re.match(r"^v(\d+)(?:\[(e?)(\d+)\])?$", s.strip())
        if not m or int(m.group(1)) > 31:
            raise AsmError("bad vector register %s" % s)
        return int(m.group(1)), int(m.group(3) or 0)

    def mem(s):
        m = re.match(r"^(.*)\((\$\d+)\)$", s.strip())
        if not m:
            raise AsmError("bad address %s" % s)
        return m.group(1) or "0", reg(m.group(2))

    def encode(mn, ops, pc, final):
        def branch(t):
            d = (val(t, pc, final) - (pc + 4)) >> 2 if final else 0
            if final and not -0x8000 <= d < 0x8000:
                raise AsmError("branch out of range")
            return d & 0xFFFF
        if mn == "nop":
            return 0
        if mn == "break":
            return 13
        if mn == ".word":
            return val(ops[0], pc, final) & 0xFFFFFFFF
        if mn in R_SPECIAL:
            fn = R_SPECIAL[mn]
            if fn in (0, 2, 3):
                return reg(ops[1]) << 16 | reg(ops[0]) << 11 | (val(ops[2], pc, final) & 31) << 6 | fn
            if fn in (4, 6, 7):
                return reg(ops[2]) << 21 | reg(ops[1]) << 16 | reg(ops[0]) << 11 | fn
            if fn == 8:
                return reg(ops[0]) << 21 | fn
            if fn == 9:
                return reg(ops[1]) << 21 | reg(ops[0]) << 11 | fn
            return reg(ops[1]) << 21 | reg(ops[2]) << 16 | reg(ops[0]) << 11 | fn
        if mn in ("bltz", "bgez", "bltzal", "bgezal"):
            return 1 << 26 | reg(ops[0]) << 21 | {"bltz": 0, "bgez": 1, "bltzal": 16, "bgezal": 17}[mn] << 16 | branch(ops[1])
        if mn in ("j", "jal"):
            return (2 if mn == "j" else 3) << 26 | ((val(ops[0], pc, final) >> 2) & 0x3FFFFFF)
        if mn in R_BRANCH:
            op = R_BRANCH[mn]
            if op in (6, 7):
                return op << 26 | reg(ops[0]) << 21 | branch(ops[1])
            return op << 26 | reg(ops[0]) << 21 | reg(ops[1]) << 16 | branch(ops[2])
        if mn in R_IMM:
            return R_IMM[mn] << 26 | reg(ops[1]) << 21 | reg(ops[0]) << 16 | (val(ops[2], pc, final) & 0xFFFF)
        if mn == "lui":
            return 15 << 26 | reg(ops[0]) << 16 | (val(ops[1], pc, final) & 0xFFFF)
        if mn in ("mfc0", "mtc0"):
            m = re.match(r"^c(\d+)$", ops[1].strip())
            return 16 << 26 | (0 if mn == "mfc0" else 4) << 21 | reg(ops[0]) << 16 | int(m.group(1)) << 11
        if mn in R_MEM:
            off, base = mem(ops[1])
            return R_MEM[mn] << 26 | base << 21 | reg(ops[0]) << 16 | (val(off, pc, final) & 0xFFFF)
        if mn in R_VOPS:
            vd, _ = vreg(ops[0])
            vs, _ = vreg(ops[1])
            vt, e = vreg(ops[2])
            return 18 << 26 | (16 | e) << 21 | vt << 16 | vs << 11 | vd << 6 | R_VOPS[mn]
        if mn in R_C2MOVE:
            vr, e = vreg(ops[1])
            return 18 << 26 | R_C2MOVE[mn] << 21 | reg(ops[0]) << 16 | vr << 11 | e << 7
        if len(mn) == 3 and mn[0] in "ls" and mn[1:] in VLS:
            k = VLS.index(mn[1:])
            vt, e = vreg(ops[0])
            off, base = mem(ops[1])
            o = val(off, pc, final)
            if final and not -64 <= o < 64:
                raise AsmError("vector offset out of range")
            return (50 if mn[0] == "l" else 58) << 26 | base << 21 | vt << 16 | k << 11 | e << 7 | (o & 0x7F)
        raise AsmError("unknown instruction %s" % mn)

    out = {}
    for final in (False, True):
        pc = 0x1000
        for name, n, line in lines:
            try:
                m = re.match(r"^([A-Za-z_.][A-Za-z_0-9.]*):\s*(.*)$", line)
                while m:
                    if not final:
                        if m.group(1) in syms:
                            raise AsmError("label %s twice" % m.group(1))
                        syms[m.group(1)] = pc
                    line = m.group(2)
                    m = re.match(r"^([A-Za-z_.][A-Za-z_0-9.]*):\s*(.*)$", line)
                if not line:
                    continue
                parts = line.split(None, 1)
                mn = parts[0].lower()
                ops = [o.strip() for o in parts[1].split(",")] if len(parts) > 1 else []
                if mn == ".org":
                    pc = val(ops[0], pc, True)
                    continue
                if mn == ".equ":
                    if not final:
                        equs[ops[0]] = val(ops[1], pc, True)
                    continue
                if mn == ".fill":
                    end = val(ops[0], pc, True)
                    if end < pc:
                        raise AsmError(".fill to 0x%X, already at 0x%X" % (end, pc))
                    while pc < end:
                        if final:
                            out[pc] = 0
                        pc += 4
                    continue
                w = encode(mn, ops, pc, final)
                if final:
                    out[pc] = w
                pc += 4
            except AsmError as e:
                raise AsmError("%s:%d: %s (%s)" % (name, n, e, line))
    lo, hi = min(out), max(out) + 4
    data = b"".join(struct.pack(">I", out.get(a, 0)) for a in range(lo, hi, 4))
    return lo, data, syms


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=("dis", "asm", "check"))
    ap.add_argument("file")
    ap.add_argument("-o", "--output")
    ap.add_argument("--base", type=lambda s: int(s, 0), default=0x1000)
    ap.add_argument("--size", type=lambda s: int(s, 0), default=0)
    ap.add_argument("--sym")
    a = ap.parse_args()
    if a.cmd == "dis":
        sys.stdout.write(disassemble(open(a.file, "rb").read(), a.base))
    elif a.cmd == "check":
        d = open(a.file, "rb").read()
        lo, back, _ = assemble(disassemble(d, a.base))
        print("round trip: %s (%d bytes)" % ("the same" if back == d[:len(d) & ~3] else "DIFFERENT", len(d)))
        if back != d[:len(d) & ~3]:
            for i in range(0, min(len(d), len(back)), 4):
                if d[i:i + 4] != back[i:i + 4]:
                    print("  first at 0x%04X: %s -> %s" % (a.base + i, d[i:i + 4].hex(), back[i:i + 4].hex()))
                    break
            sys.exit(1)
    else:
        import os
        lo, data, syms = assemble(open(a.file).read(), os.path.dirname(a.file) or ".")
        if a.size:
            if len(data) > a.size:
                sys.exit("%d bytes of text: over the %d allowed" % (len(data), a.size))
            data += bytes(a.size - len(data))
        open(a.output, "wb").write(data)
        if a.sym:
            open(a.sym, "w").write("".join("%04X %s\n" % (v, k) for k, v in sorted(syms.items(), key=lambda kv: kv[1])))
        print("%s: %d bytes from 0x%04X" % (a.output, len(data), lo))


if __name__ == "__main__":
    try:
        main()
    except AsmError as e:
        sys.exit("error: %s" % e)
