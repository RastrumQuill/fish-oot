#!/usr/bin/env bash
# Compiles just the given source files with the exact commands a build used (from its log), to check a change in
# seconds instead of a whole build (compile, link, compress, patch). The objects are written as the build would.
# Usage: fishmod/tools/recheck.sh BUILD.log FILE.c...
set -uo pipefail
LOG=$1; shift
rc=0
for f in "$@"; do
    cmds=$(grep -E "^(gcc |tools/fishmod_gcc_cc.sh |\./tools/preprocess.sh )" "$LOG" | grep -E "[ =]$f( |$)")
    if [ -z "$cmds" ]; then
        echo "$f: not in $LOG"; rc=1; continue
    fi
    while IFS= read -r c; do
        out=$(eval "$c" 2>&1) || { echo "$out" | grep -E "error" | head -20; echo "$f: FAILED"; rc=1; }
    done <<< "$cmds"
done
[ $rc = 0 ] && echo "ok: $*"
exit $rc
