# Web emulator

The firmware, built to WebAssembly against the preview harness's fake board
(`test/host/emu.cpp`), with a page that stands in for the device: touch and
swipe on the panel, the three side buttons, the clock, and NVS kept in the
browser's localStorage.

Live at https://sticky.beseler.org (Coolify builds `emu/Dockerfile` on every
push to `schule`).

Build by hand, anywhere Docker runs:

    docker run --rm -v "$PWD":/src -w /src emscripten/emsdk:4.0.10 sh emu/build.sh

then serve `emu/dist/` with any static server.

What it cannot do: WiFi (phone pairing, the web editors), the real SD card
(it has the harness's invented books), sound, and true e-paper ghosting --
the E-Ink delay only mimics the refresh timing.
