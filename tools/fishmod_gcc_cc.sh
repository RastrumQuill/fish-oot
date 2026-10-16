#!/usr/bin/env bash
# fish-oot: GCC for the game's own code in an otherwise IDO build (GCC_CODE=1, see the Makefile). Drops the IDO-only
# options that per-file rules add (-trapuv and the like), then runs mips-linux-gnu-gcc with what's left.
args=()
skip=0
for a in "$@"; do
    if [ "$skip" = 1 ]; then skip=0; continue; fi
    case "$a" in
        -trapuv|-signed|-use_readwrite_const|-fullwarn|-verbose|-non_shared|-Xcpluscomm|-Wab,*|-o32) ;;
        -woff) skip=1 ;;
        *) args+=("$a") ;;
    esac
done
exec "${FISHMOD_GCC:-mips-linux-gnu-gcc}" "${args[@]}"
