#!/usr/bin/env bash
# fish-oot: clang (LLVM) for the mod's own code (CLANG_MOD=1, see the Makefile; fishmod/build.sh --clang), the rest
# staying as it was (the game's code on GCC, boot, libultra, libc and audio on IDO). Takes the GCC command line the
# Makefile builds and turns it into clang's: the same o32 ABI, VR4300 (as MIPS II, with LLVM's own VR4300
# multiply-bug fix), no PIC, no small data. Drops what's GCC- or IDO-only. FISHMOD_CLANG names the clang (default
# clang); CLANG_EXTRA adds options (for example -fprofile-sample-use=...).
# -mno-odd-spreg: the VR4300 in the game's FPU mode (FR=0) does single-precision arithmetic in the even registers only;
# an odd one raises "unimplemented operation" (GCC never uses them; LLVM does unless told).
args=(--target=mips-linux-gnu -mfp32 -mno-odd-spreg -fno-pic -mno-abicalls -std=gnu89
      # what GCC's MIPS port predefines and the game's headers test (clang has the _MIPS_SZ*, _ABIO32 and MIPSEB ones)
      -D_LANGUAGE_C -DLANGUAGE_C -D__LANGUAGE_C -D__LANGUAGE_C__ -U_MIPS_ISA -D_MIPS_ISA=2
      -D_MIPS_ISA_MIPS1=1 -D_MIPS_ISA_MIPS2=2 -D_MIPS_ISA_MIPS3=3 -D_MIPS_ISA_MIPS4=4)
skip=0
has_g=0
for a in "$@"; do
    if [ "$skip" = 1 ]; then skip=0; continue; fi
    case "$a" in
        # IDO's own
        # (IDO's -signed: plain char is signed in that file; IDO's default, like GCC's -funsigned-char, is unsigned)
        -signed) args+=(-fsigned-char) ;;
        -trapuv|-use_readwrite_const|-fullwarn|-verbose|-non_shared|-Xcpluscomm|-Wab,*|-o32) ;;
        -woff) skip=1 ;;
        # GCC's own: the CPU (LLVM knows the VR4300 as MIPS III), the divide check (LLVM's traps the same way),
        # the character set (the mod's strings are ASCII), the overlay relocation options (LLVM pairs every
        # %hi with its %lo already: tools/check_overlay_relocs.py checks)
        # (as MIPS II: LLVM 18 at MIPS III with o32 can merge two 32-bit stores into a 64-bit one it then can't emit,
        # "Cannot select: store i64"; o32 code uses no 64-bit instructions anyway, as GCC's doesn't)
        -march=vr4300) args+=(-march=mips2) ;;
        -mdivide-breaks|-fexec-charset=*|-mno-explicit-relocs|-mno-split-addresses|-mgp32|-mips2|-mips3) ;;
        -g) args+=(-gdwarf-4); has_g=1 ;;
        *) args+=("$a") ;;
    esac
done
# CLANG_OPT (build.sh --clang-opt): the optimisation level, after the Makefile's. Not for files marked
# -DFISHMOD_CLANG_KEEP_OPT: libultra hangs at boot built -Os (the VI manager is never started; -O2 runs) - not yet
# understood, so it stays at -O2.
case " ${args[*]} " in
    *" -DFISHMOD_CLANG_KEEP_OPT "*) ;;
    *) [ -n "${CLANG_OPT:-}" ] && args+=("-$CLANG_OPT") ;;
esac
# Sample profiles: the profile is by source line, so a build that reads one needs line tables (-gline-tables-only
# when it has no -g: else the profile is silently ignored), and every build with them gets discriminators for each
# block (-fdebug-info-for-profiling). Neither changes the code (checked: z_actor.c's .text is identical).
case " ${CLANG_EXTRA:-} " in
    *" -fprofile-sample-use="*) [ "${has_g:-0}" = 1 ] || { args+=(-gline-tables-only); has_g=1; } ;;
esac
[ "${has_g:-0}" = 1 ] && args+=(-fdebug-info-for-profiling)
# shellcheck disable=SC2086
exec "${FISHMOD_CLANG:-clang}" "${args[@]}" -mfix4300 -Wno-everything $CLANG_EXTRA
