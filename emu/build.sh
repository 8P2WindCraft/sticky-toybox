#!/bin/sh
# Builds the web emulator (emu/dist/) from the firmware sources with Emscripten.
# Needs emcc on the PATH -- the emscripten/emsdk Docker image has it.
set -e
cd "$(dirname "$0")/../test/host"
OUT=../../emu/dist
OBJ=$(mktemp -d)
mkdir -p "$OUT"
INC="-I . -I mock -I ../../src -I ../../toybox-core/src -I ../../lib/QRCode/src -I ../../lib/miniz/src -I ../../lib/tjpgd/src"
# The three C libraries compile as C; em++ would take them for C++.
emcc -O2 -w -DTOYBOX_HOST $INC -c ../../lib/QRCode/src/qrcode.c -o "$OBJ/qrcode.o"
emcc -O2 -w -DTOYBOX_HOST $INC -c ../../lib/tjpgd/src/tjpgd.c -o "$OBJ/tjpgd.o"
emcc -O2 -w -DTOYBOX_HOST $INC -c ../../lib/miniz/src/toybox_miniz_impl.c -o "$OBJ/miniz.o"
em++ -std=gnu++17 -O2 -w -DTOYBOX_HOST $INC \
  emu.cpp ../../src/gfx.cpp ../../src/cardfonts.cpp \
  ../../src/fonts_intl.cpp ../../src/sensors.cpp ../../src/sticky_host.cpp ../../src/sdcard.cpp \
  ../../toybox-core/src/toybox.cpp ../../toybox-core/src/hub.cpp ../../toybox-core/src/epubcore.cpp \
  ../../toybox-core/src/epubcover.cpp ../../toybox-core/src/settings.cpp ../../toybox-core/src/wordle.cpp \
  ../../toybox-core/src/nonogram.cpp ../../toybox-core/src/game2048.cpp ../../toybox-core/src/xo.cpp \
  "$OBJ"/*.o \
  -sMODULARIZE=1 -sEXPORT_NAME=StickyEmu -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=1048576 \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,HEAPU8,UTF8ToString \
  -o "$OUT/emu.js"
rm -rf "$OBJ"
cp ../../emu/index.html "$OUT/"
ls -l "$OUT"
