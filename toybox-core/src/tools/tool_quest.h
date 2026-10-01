// Abenteuer: the screen and the controls for quest_rules.h.
//
// Tap beside the hero to walk that way -- anywhere on that side of the room
// will do, so a finger does not have to find a 48 px tile. The side buttons
// walk up and down, and the power button waits a turn.
//
// Drawn to the e-paper rules in docs/SCHULE.md: outlines, not filled
// sprites, so a slime moving across the floor leaves no shadow behind in a
// partial refresh. The only fills are the hearts, which change rarely.
#pragma once
#include <esp_random.h>

#include "decor.h"
#include "quest_rules.h"
#include "tools_draw.h"
#include "tools_ui.h"

namespace qui {
inline constexpr int T = 48;  // tile
inline constexpr int MAP_Y = 86;
inline constexpr int HUD_Y = 44;
inline constexpr int MSG_Y = MAP_Y + quest::H * T + 18;
inline constexpr TRect BIG_BTN{60, 600, 360, 80};
}  // namespace qui

class QuestTool : public ToolApp {
 public:
  ~QuestTool() override { save(); }

  const char* title() const override { return "ABENTEUER"; }
  bool cleanTransitions() const override { return true; }

  void enter(ToolsHost& h) override {
    ToolApp::enter(h);
    const size_t got = prefs().getBytes("qs_state", &_s, sizeof(_s));
    if (got != sizeof(_s) || !quest::valid(_s)) quest::newGame(_s, esp_random());
    _msg = _s.steps == 0 ? "Tippe neben den Helden, um zu laufen." : quest::ROOM_NAMES[_s.room];
    _loaded = true;
  }

  void render(ToolsCanvas& c) override {
    host().topBar(title());
    if (_s.won) return renderEnd(c, "Du hast den Kristall gefunden!", "NEUES SPIEL");
    if (_s.hp == 0) return renderEnd(c, "Du bist ohnm\xc3\xa4" "chtig geworden.", "WEITER");
    renderHud(c);
    for (int j = 0; j < quest::H; j++)
      for (int i = 0; i < quest::W; i++)
        drawTile(c, quest::tileAt(_s, _s.room, i, j), i * qui::T, qui::MAP_Y + j * qui::T);
    for (int k = 0; k < _s.nEn; k++)
      slime(c, _s.en[k].x * qui::T, qui::MAP_Y + _s.en[k].y * qui::T);
    hero(c, _s.x * qui::T, qui::MAP_Y + _s.y * qui::T);
    c.fillRect(0, qui::MAP_Y + quest::H * qui::T, c.width(), 1, true);
    c.textCentered(c.width() / 2, qui::MSG_Y, _msg, TS_MED, true);
  }

  void onTap(int x, int y) override {
    if (host().isBackTap(x, y)) {
      save();
      host().beep(1);
      return host().goHub();
    }
    if (_s.won || _s.hp == 0) {
      if (!qui::BIG_BTN.hit(x, y)) return;
      if (_s.won)
        quest::newGame(_s, esp_random());
      else
        quest::respawn(_s);
      _msg = quest::ROOM_NAMES[_s.room];
      save();
      host().beep(1);
      return host().refresh(true);
    }
    if (y < qui::MAP_Y || y >= qui::MAP_Y + quest::H * qui::T) return;
    const int dx = x / qui::T - _s.x, dy = (y - qui::MAP_Y) / qui::T - _s.y;
    if (dx == 0 && dy == 0) {
      // On the hero himself. At the edge of the room there is nothing beyond
      // to tap, so this is the way through an exit; anywhere else, a wait.
      const int ox = _s.x == quest::W - 1 ? 1 : _s.x == 0 ? -1 : 0;
      const int oy = ox ? 0 : _s.y == quest::H - 1 ? 1 : _s.y == 0 ? -1 : 0;
      if (ox || oy) {
        const uint16_t ev = quest::step(_s, ox, oy);  // a blocked step changes nothing
        if (ev != quest::EV_BLOCKED) return act(ev);
      }
      return act(quest::wait(_s));
    }
    if (quest::iabs(dx) >= quest::iabs(dy)) return move(dx > 0 ? 1 : -1, 0);
    move(0, dy > 0 ? 1 : -1);
  }

  bool onButton(SideBtn b) override {
    if (_s.won || _s.hp == 0) return false;
    if (b == SideBtn::Ok)
      act(quest::wait(_s));
    else
      move(0, b == SideBtn::Up ? -1 : 1);
    return true;
  }

#ifdef TOYBOX_HOST
  quest::State& hostState() { return _s; }
#endif

 private:
  void move(int dx, int dy) {
    _face = dy > 0 ? 0 : dy < 0 ? 1 : dx < 0 ? 2 : 3;
    act(quest::step(_s, dx, dy));
  }

  void act(uint16_t ev) {
    using namespace quest;
    if (ev == EV_BLOCKED) return host().beep(2);  // a wall: nothing happened, nothing to paint
    if (ev & EV_WON) _msg = "Der Kristall!";
    else if (ev & EV_DIED) _msg = "";
    else if (ev & EV_ROOM) _msg = ROOM_NAMES[_s.room];
    else if (ev & EV_LOCKED) _msg = "Verschlossen. Du brauchst einen Schl\xc3\xbc" "ssel.";
    else if (ev & EV_DOOR) _msg = "Die T\xc3\xbc" "r ist offen.";
    else if (ev & EV_KEY) _msg = "Ein Schl\xc3\xbc" "ssel!";
    else if (ev & EV_HEART) _msg = "Ein Herz. Du f\xc3\xbc" "hlst dich besser.";
    else if (ev & EV_HURT) _msg = "Autsch!";
    else if (ev & EV_KILLED) _msg = "Getroffen!";
    else _msg = ROOM_NAMES[_s.room];
    host().beep((ev & (EV_HURT | EV_DIED)) ? 2 : (ev & (EV_KEY | EV_HEART | EV_DOOR | EV_WON)) ? 3 : 0);
    // A new room or an end screen replaces the whole panel: full. A step
    // changes a few tiles: partial, with the host's periodic clean.
    if (ev & (EV_ROOM | EV_WON | EV_DIED)) {
      save();
      return host().refresh(true);
    }
    host().refreshUi();
  }

  void save() {
    if (_loaded && _host) prefs().putBytes("qs_state", &_s, sizeof(_s));
  }

  // --- drawing -------------------------------------------------------------
  void renderHud(ToolsCanvas& c) {
    for (int i = 0; i < quest::MAX_HP; i++) {
      const int cx = 26 + i * 34, cy = qui::HUD_Y + 20;
      if (i < _s.hp)
        tdraw::heart(c, cx, cy, 26, true);
      else if (i < quest::START_HP)
        c.drawCircle(cx, cy, 3, 1, true);  // an empty place where a heart was
    }
    c.textCentered(c.width() / 2, qui::HUD_Y + 8, quest::ROOM_NAMES[_s.room], TS_MED, true, true);
    if (_s.keys) {
      keyIcon(c, c.width() - 96, qui::HUD_Y + 2);
      char n[8];
      snprintf(n, sizeof(n), "x%d", _s.keys);
      c.text(c.width() - 44, qui::HUD_Y + 8, n, TS_MED, true);
    }
  }

  void renderEnd(ToolsCanvas& c, const char* line, const char* btn) {
    c.textCentered(c.width() / 2, 260, line, TS_LARGE, true, true);
    if (_s.won) {
      char b[40];
      snprintf(b, sizeof(b), "in %u Schritten", (unsigned)_s.steps);
      c.textCentered(c.width() / 2, 320, b, TS_MED, true);
      crystal(c, c.width() / 2 - qui::T / 2, 400);
    } else {
      c.textCentered(c.width() / 2, 320, "Alles Gefundene bleibt bei dir.", TS_MED, true);
    }
    c.button(qui::BIG_BTN.x, qui::BIG_BTN.y, qui::BIG_BTN.w, qui::BIG_BTN.h, btn, true, TS_LARGE);
  }

  static void drawTile(ToolsCanvas& c, char t, int x, int y) {
    switch (t) {
      case '#':  // stone: a course of bricks
        c.drawRect(x, y, qui::T, qui::T, 1, true);
        c.fillRect(x, y + 16, qui::T, 1, true);
        c.fillRect(x, y + 32, qui::T, 1, true);
        c.fillRect(x + 24, y, 1, 16, true);
        c.fillRect(x + 12, y + 16, 1, 16, true);
        c.fillRect(x + 36, y + 16, 1, 16, true);
        c.fillRect(x + 24, y + 32, 1, 16, true);
        break;
      case 'T':  // a tree: crown and trunk
        c.drawCircle(x + 24, y + 19, 15, 2, true);
        c.drawCircle(x + 20, y + 16, 5, 1, true);
        c.fillRect(x + 22, y + 34, 4, 11, true);
        break;
      case '~':  // water: three rows of ripples
        for (int k = 0; k < 3; k++)
          for (int i = 0; i < 3; i++) {
            const int wx = x + 4 + i * 15 + (k % 2) * 6, wy = y + 10 + k * 13;
            c.drawLine(wx, wy + 3, wx + 5, wy, 1, true);
            c.drawLine(wx + 5, wy, wx + 10, wy + 3, 1, true);
          }
        break;
      case 'D':  // a locked door with a keyhole
        c.drawRect(x + 6, y + 3, 36, 43, 3, true);
        c.fillCircle(x + 24, y + 21, 4, true);
        c.fillRect(x + 22, y + 23, 4, 10, true);
        break;
      case 'K': keyIcon(c, x, y); break;
      case 'H': tdraw::heart(c, x + 24, y + 26, 26, true); break;
      case 'C': crystal(c, x, y); break;
      default: break;  // floor stays white: nothing on it to ghost
    }
  }

  static void keyIcon(ToolsCanvas& c, int x, int y) {
    c.drawCircle(x + 14, y + 22, 7, 3, true);
    c.fillRect(x + 21, y + 21, 20, 3, true);
    c.fillRect(x + 33, y + 24, 3, 7, true);
    c.fillRect(x + 39, y + 24, 3, 5, true);
  }

  static void crystal(ToolsCanvas& c, int x, int y) {
    const int cx = x + 24;
    c.drawLine(cx, y + 4, x + 38, y + 18, 2, true);
    c.drawLine(x + 38, y + 18, cx, y + 44, 2, true);
    c.drawLine(cx, y + 44, x + 10, y + 18, 2, true);
    c.drawLine(x + 10, y + 18, cx, y + 4, 2, true);
    c.drawLine(x + 10, y + 18, x + 38, y + 18, 1, true);
    c.drawLine(cx, y + 4, cx, y + 44, 1, true);
  }

  static void slime(ToolsCanvas& c, int x, int y) {
    // A dome: the top half of a circle on a flat base, and two eyes.
    const int cx = x + 24, cy = y + 32, r = 16;
    c.drawCircle(cx, cy, r, 3, true);
    c.fillRect(cx - r - 2, cy + 1, 2 * r + 5, r - 2, false);  // stays inside the tile
    c.fillRect(cx - r, cy, 2 * r + 1, 3, true);
    c.fillCircle(cx - 6, cy - 7, 3, true);
    c.fillCircle(cx + 6, cy - 7, 3, true);
  }

  void hero(ToolsCanvas& c, int x, int y) {
    // Head, a tunic as a triangle outline, and the sword held the way he faces.
    const int cx = x + 24;
    c.drawCircle(cx, y + 11, 7, 2, true);
    c.drawLine(cx, y + 18, cx - 11, y + 42, 2, true);
    c.drawLine(cx - 11, y + 42, cx + 11, y + 42, 2, true);
    c.drawLine(cx + 11, y + 42, cx, y + 18, 2, true);
    switch (_face) {
      case 0: c.fillRect(cx + 13, y + 24, 3, 20, true); break;   // down: blade along the side
      case 1: c.fillRect(cx + 13, y + 2, 3, 22, true); break;    // up: raised
      case 2: c.fillRect(x + 1, y + 28, 18, 3, true); break;     // left
      default: c.fillRect(x + 29, y + 28, 18, 3, true); break;   // right
    }
  }

  quest::State _s{};
  bool _loaded = false;
  uint8_t _face = 0;
  const char* _msg = "";
};
