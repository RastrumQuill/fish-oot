#!/usr/bin/env bash
# fish-oot: clang (LLVM) for the mod's own code (CLANG_MOD=1, see the Makefile; fishmod/build.sh --clang), the rest
# staying as it was (the game's code on GCC, boot, libultra, libc and audio on IDO). Takes the GCC command line the
# Makefile builds and turns it into clang's: the same o32 ABI, VR4300 (as MIPS II, with LLVM's own VR4300
# multiply-bug fix), no PIC, no small data. Drops what's GCC- or IDO-only. FISHMOD_CLANG names the clang (default
# clang); CLANG_EXTRA adds options (for example -fprofile-sample-use=...).
args=(--target=mips-linux-gnu -mfp32 -fno-pic -mno-abicalls -std=gnu89
      # what GCC's MIPS port predefines and the game's headers test (clang has the _MIPS_SZ*, _ABIO32 and MIPSEB ones)
      -D_LANGUAGE_C -DLANGUAGE_C -D__LANGUAGE_C -D__LANGUAGE_C__ -U_MIPS_ISA -D_MIPS_ISA=2)
skip=0
for a in "$@"; do
    if [ "$skip" = 1 ]; then skip=0; continue; fi
    case "$a" in
        # IDO's own
        -trapuv|-signed|-use_readwrite_const|-fullwarn|-verbose|-non_shared|-Xcpluscomm|-Wab,*|-o32) ;;
        -woff) skip=1 ;;
        # GCC's own: the CPU (LLVM knows the VR4300 as MIPS III), the divide check (LLVM's traps the same way),
        # the character set (the mod's strings are ASCII), the overlay relocation options (LLVM pairs every
        # %hi with its %lo already: tools/check_overlay_relocs.py checks)
        # (as MIPS II: LLVM 18 at MIPS III with o32 can merge two 32-bit stores into a 64-bit one it then can't emit,
        # "Cannot select: store i64"; o32 code uses no 64-bit instructions anyway, as GCC's doesn't)
        -march=vr4300) args+=(-march=mips2) ;;
        -mdivide-breaks|-fexec-charset=*|-mno-explicit-relocs|-mno-split-addresses|-mgp32|-mips2|-mips3) ;;
        -g) args+=(-gdwarf-4) ;;
        *) args+=("$a") ;;
    esac
done
# shellcheck disable=SC2086
exec "${FISHMOD_CLANG:-clang}" "${args[@]}" -mfix4300 -Wno-everything $CLANG_EXTRA
