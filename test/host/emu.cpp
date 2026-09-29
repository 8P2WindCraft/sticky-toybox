// The web emulator: the whole firmware, built against the preview harness's
// fake board, driven from a browser page instead of from a script.
//
// The harness already stands in for every part the firmware talks to -- the
// panel, touch, buzzer, clock, SD card and NVS -- so its file is taken whole,
// with its own main() renamed out of the way. What is added here is the
// handful of calls the page makes: taps, swipes, the side buttons, the clock,
// and a copy of NVS to keep in the browser between visits.
//
// Built with Emscripten; see emu/README.md.
#define main preview_main
#include "host_preview.cpp"
#undef main

#include <emscripten/emscripten.h>

namespace {
uint32_t g_seq = 0;      // bumps on every refresh the firmware asks for
bool g_lastFull = false;
int g_fulls = 0, g_partials = 0;
std::string g_prefsOut;

void onFrame(const uint8_t* fb, bool full) {
  (void)fb;  // the page reads epd's buffer directly; it is the same memory
  g_seq++;
  g_lastFull = full;
  (full ? g_fulls : g_partials)++;
}

void emuLockInfo(lock::Info& i) {
  sensors::Clock ck;
  i.haveClock = sensors::readClock(ck);
  i.hour = ck.hour;
  i.minute = ck.minute;
  i.day = ck.day;
  i.month = ck.month;
  i.year = ck.year;
  i.haveTemp = true;
  i.tempDeciC = 214;
  i.haveBattery = true;
  i.batteryPct = 84;
}
}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE void emu_init(const char* savedPrefs) {
  g_dumpEnabled = false;
  g_frameHook = onFrame;
  epd.begin();
  prefs.begin("toybox", false);
  if (savedPrefs && *savedPrefs) prefs.deserialize(savedPrefs);
  sensors::hostSetClock(true);
  sensors::hostSetBattery(84, false);
  toybox.begin(stickyHost);
  lock::apply(prefs);
  lock::setInfoHook(emuLockInfo);
  toybox.goHub();
}

// Milliseconds as the page counts them, so timers and holds run in real time.
EMSCRIPTEN_KEEPALIVE void emu_set_millis(double ms) { g_millis = 100000 + (unsigned long)ms; }

EMSCRIPTEN_KEEPALIVE void emu_tap(int x, int y) { toybox.onTap(x, y); }
EMSCRIPTEN_KEEPALIVE void emu_swipe(int dx, int dy) { toybox.onSwipe(dx, dy); }

// A short press: 0 up, 1 down, 2 the power button.
EMSCRIPTEN_KEEPALIVE int emu_button(int b) {
  return toybox.onButton(b == 0 ? SideBtn::Up : b == 1 ? SideBtn::Down : SideBtn::Ok);
}

// The holds on the home page, as the firmware's loop does them: UP opens
// settings, DOWN carries on with the last book or app.
EMSCRIPTEN_KEEPALIVE int emu_hold(int b) {
  if (!toybox.atHubHome()) return 0;
  if (b == 0) {
    toybox.openSettings();
    return 1;
  }
  if (b == 1 && toybox.canCarryOn()) {
    toybox.carryOnReading();
    return 1;
  }
  return 0;
}

EMSCRIPTEN_KEEPALIVE int emu_at_home() { return toybox.atHubHome(); }
EMSCRIPTEN_KEEPALIVE int emu_wants_tick() { return toybox.wantsTick(); }
EMSCRIPTEN_KEEPALIVE void emu_tick() { toybox.tick(); }

EMSCRIPTEN_KEEPALIVE void emu_set_clock(int y, int mo, int d, int h, int mi) {
  sensors::Clock c{};
  c.year = (uint16_t)y;
  c.month = (uint8_t)mo;
  c.day = (uint8_t)d;
  c.hour = (uint8_t)h;
  c.minute = (uint8_t)mi;
  sensors::hostSetClockTo(c);
}

// The panel: PANEL_W x PANEL_H, one bit a pixel, EPD_WB bytes a row, set = white.
EMSCRIPTEN_KEEPALIVE const uint8_t* emu_frame() { return epd.fb(); }
EMSCRIPTEN_KEEPALIVE int emu_panel_w() { return PANEL_W; }
EMSCRIPTEN_KEEPALIVE int emu_panel_h() { return PANEL_H; }
EMSCRIPTEN_KEEPALIVE int emu_row_bytes() { return EPD_WB; }
EMSCRIPTEN_KEEPALIVE uint32_t emu_seq() { return g_seq; }
EMSCRIPTEN_KEEPALIVE int emu_last_full() { return g_lastFull; }
EMSCRIPTEN_KEEPALIVE int emu_fulls() { return g_fulls; }
EMSCRIPTEN_KEEPALIVE int emu_partials() { return g_partials; }

EMSCRIPTEN_KEEPALIVE const char* emu_prefs() {
  g_prefsOut = prefs.serialize();
  return g_prefsOut.c_str();
}

}  // extern "C"
