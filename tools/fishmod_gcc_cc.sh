#!/usr/bin/env bash
# fish-oot: GCC for the game's own code in an otherwise IDO build (GCC_CODE=1, see the Makefile). Drops the IDO-only
# options that per-file rules add (-trapuv and the like), then runs mips-linux-gnu-gcc with what's left.
args=()
skip=0
for a in "$@"; do
    if [ "$skip" = 1 ]; then skip=0; continue; fi
    case "$a" in
        # (IDO's -signed: plain char is signed in that file; GCC's default here, like IDO's, is unsigned)
        -signed) args+=(-fsigned-char) ;;
        -trapuv|-use_readwrite_const|-fullwarn|-verbose|-non_shared|-Xcpluscomm|-Wab,*|-o32) ;;
        -woff) skip=1 ;;
        *) args+=("$a") ;;
    esac
done
# (GCC 14 makes errors of what the game's old C does in places, as calling a function before its declaration: they
# stay the warnings they were, so one source builds with GCC 10 and 14 alike)
# FISHMOD_GCC_AS_DIR: a folder holding the assembler to use as `as` (a bare-metal GCC's own writes IRIX-flavoured
# objects the build's linker won't take; the build's mips-linux-gnu-as, linked there, writes the usual ones)
extra=()
[ -n "${FISHMOD_GCC_AS_DIR:-}" ] && extra+=("-B${FISHMOD_GCC_AS_DIR%/}/")
exec "${FISHMOD_GCC:-mips-linux-gnu-gcc}" "${extra[@]}" -Wno-error=implicit-function-declaration -Wno-error=int-conversion \
    -Wno-error=incompatible-pointer-types -Wno-error=implicit-int "${args[@]}"
