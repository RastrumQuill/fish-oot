#!/usr/bin/env bash
# Builds a bare-metal GCC for the N64 (target mips64-elf, the VR4300's instructions and timings by default) from the
# GNU sources: the compiler proper and binutils, no C library (the game has its own). fishmod/build.sh uses it when
# FISHMOD_GCC names it (FISHMOD_GCC=/opt/n64gcc/bin/mips64-elf-gcc); otherwise the build uses the system's
# mips-linux-gnu-gcc, a compiler made for Linux programs.
#
# Usage: fishmod/tools/build_n64_gcc.sh [prefix] [work dir]     (default /opt/n64gcc, /opt/src/n64gcc)
# Needs: build-essential, libgmp-dev libmpfr-dev libmpc-dev (or the work dir's gcc source gets them:
#        contrib/download_prerequisites), texinfo not needed (MAKEINFO=true). About 40 minutes on 4 cores.
set -euo pipefail
PREFIX="${1:-/opt/n64gcc}"
WORK="${2:-/opt/src/n64gcc}"
BINUTILS=binutils-2.43.1
GCC=gcc-14.2.0
JOBS="${JOBS:-$(nproc)}"
mkdir -p "$WORK/src" "$PREFIX"
cd "$WORK/src"
[ -f $BINUTILS.tar.xz ] || curl -sS -O https://ftp.gnu.org/gnu/binutils/$BINUTILS.tar.xz
[ -f $GCC.tar.xz ] || curl -sS -O https://ftp.gnu.org/gnu/gcc/$GCC/$GCC.tar.xz
[ -d $BINUTILS ] || tar xf $BINUTILS.tar.xz
[ -d $GCC ] || tar xf $GCC.tar.xz
if ! dpkg -s libgmp-dev libmpfr-dev libmpc-dev >/dev/null 2>&1; then
    (cd $GCC && ./contrib/download_prerequisites)
fi
mkdir -p "$WORK/build-binutils" "$WORK/build-gcc"
cd "$WORK/build-binutils"
"$WORK/src/$BINUTILS/configure" --target=mips64-elf --prefix="$PREFIX" --with-cpu=vr4300 --disable-nls --disable-werror \
    --disable-gdb --disable-sim --disable-gprofng MAKEINFO=true
make -j"$JOBS" MAKEINFO=true
make install MAKEINFO=true
cd "$WORK/build-gcc"
"$WORK/src/$GCC/configure" --target=mips64-elf --prefix="$PREFIX" --with-arch=vr4300 --with-tune=vr4300 \
    --enable-languages=c --without-headers --with-newlib --disable-libssp --disable-threads --disable-shared \
    --disable-multilib --disable-nls --disable-werror --disable-libquadmath --disable-libgomp --disable-libatomic \
    --with-system-zlib MAKEINFO=true
make -j"$JOBS" all-gcc MAKEINFO=true
make install-gcc MAKEINFO=true
"$PREFIX/bin/mips64-elf-gcc" --version | head -1
