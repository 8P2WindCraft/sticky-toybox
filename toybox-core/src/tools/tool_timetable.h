// Stundenplan: the school week on the fridge.
//
// Opens on the day that matters right now -- today until school is out, then
// the next school day, so the bag gets packed for the right one. The side
// buttons and a swipe step through the week; WOCHE shows all five days at
// once. Tapping a lesson picks its subject from the ones already in the plan,
// or types a new one on the German keyboard.
//
// The plan lives in NVS as the text form in timetable_data.h, which is also
// what the server will send once the device syncs.
#pragma once
#include "keyboard.h"
#include "timetable_data.h"
#include "tools_ui.h"

namespace ttui {
inline constexpr int HEAD_Y = 52;
inline constexpr int ROWS_Y = 110, ROW_H = 54;
inline constexpr int MIN_ROWS = 6;
inline constexpr TRect PREV{20, 690, 100, 70};
inline constexpr TRect MID{130, 690, 220, 70};
inline constexpr TRect NEXT{360, 690, 100, 70};

inline constexpr int WK_X = 40, WK_COL = 88, WK_HEAD_Y = 50, WK_Y = 88, WK_ROW = 58;

inline constexpr int MAX_SUBJ = 16;
inline constexpr int SUBJ_Y = 110, SUBJ_STEP = 60;
inline constexpr TRect NEW_BTN{20, 610, 215, 64};
inline constexpr TRect EMPTY_BTN{245, 610, 215, 64};
inline constexpr TRect BACK_BTN{20, 690, 440, 64};

inline TRect subjRect(int i) {
  return TRect{i % 2 ? 245 : 20, SUBJ_Y + (i / 2) * SUBJ_STEP, 215, 52};
}
}  // namespace ttui

class TimetableTool : public ToolApp {
 public:
  const char* title() const override { return "STUNDENPLAN"; }

  void enter(ToolsHost& h) override {
    ToolApp::enter(h);
    load();
    int y, m, d, hh, mm;
    _haveClock = h.clockDate(y, m, d) && h.clockHHMM(hh, mm);
    _wday = _haveClock ? ttdata::weekday(y, m, d) : 0;
    _minutes = _haveClock ? hh * 60 + mm : 0;
    _shown = ttdata::dayToShow(_plan, _wday, _minutes);
    _day = _shown.day;
    _screen = Screen::Day;
  }

  void render(ToolsCanvas& c) override {
    switch (_screen) {
      case Screen::Day: renderDay(c); break;
      case Screen::Week: renderWeek(c); break;
      case Screen::Pick: renderPick(c); break;
      case Screen::Typing: _kbd.render(c); break;
    }
  }

  void onTap(int x, int y) override {
    if (_screen == Screen::Typing) return tapTyping(x, y);
    if (host().isBackTap(x, y)) {
      host().beep(1);
      if (_screen == Screen::Day) return host().goHub();
      return show(Screen::Day);
    }
    switch (_screen) {
      case Screen::Day: return tapDay(x, y);
      case Screen::Week: return tapWeek(x, y);
      default: return tapPick(x, y);
    }
  }

  void onSwipe(int dx, int dy) override {
    if (_screen != Screen::Day || abs(dx) < abs(dy)) return;
    step(dx < 0 ? 1 : -1);
  }

  bool onButton(SideBtn b) override {
    if (_screen != Screen::Day || b == SideBtn::Ok) return false;
    step(b == SideBtn::Down ? 1 : -1);
    return true;
  }

#ifdef TOYBOX_HOST
  ttdata::Plan& hostPlan() { return _plan; }
#endif

 private:
  enum class Screen : uint8_t { Day, Week, Pick, Typing };

  int rows() const {
    const int n = ttdata::periodsUsed(_plan);
    return n < ttui::MIN_ROWS ? ttui::MIN_ROWS : n;
  }

  void show(Screen s) {
    _screen = s;
    host().refreshUi();
  }

  void step(int by) {
    _day = (_day + by + ttdata::DAYS) % ttdata::DAYS;
    host().beep(0);
    host().refreshUi();
  }

  // --- day ---------------------------------------------------------------
  void renderDay(ToolsCanvas& c) {
    using namespace ttui;
    host().topBar(title());
    char head[40];
    const char* rel = nullptr;
    if (_haveClock && _day == _shown.day)
      rel = _shown.when == ttdata::When::Today      ? "Heute"
            : _shown.when == ttdata::When::Tomorrow ? "Morgen"
                                                     : nullptr;
    if (rel)
      snprintf(head, sizeof(head), "%s, %s", rel, ttdata::DAY_LONG[_day]);
    else
      snprintf(head, sizeof(head), "%s", ttdata::DAY_LONG[_day]);
    c.textCentered(c.width() / 2, HEAD_Y, head, TS_LARGE, true, true);

    const int now = (_haveClock && _day == _wday) ? ttdata::lessonAt(_plan, _minutes) : -1;
    const int n = rows();
    for (int i = 0; i < n; i++) {
      const int y = ROWS_Y + i * ROW_H;
      char num[8];
      snprintf(num, sizeof(num), "%d.", i + 1);
      c.text(24, y + (ROW_H - c.textHeight(TS_MED)) / 2, num, TS_MED, true, true);
      if (_plan.start[i]) {
        char t[8];
        snprintf(t, sizeof(t), "%d:%02d", _plan.start[i] / 60, _plan.start[i] % 60);
        c.text(72, y + (ROW_H - c.textHeight(TS_SMALL)) / 2, t, TS_SMALL, true);
      }
      const char* s = _plan.cell[_day][i];
      c.textClipped(150, y + (ROW_H - c.textHeight(TS_LARGE)) / 2, 310, s[0] ? s : "-", TS_LARGE,
                    true, i == now);
      if (i == now) c.drawRect(12, y + 2, 456, ROW_H - 4, 3, true);
      if (i + 1 < n) c.fillRect(20, y + ROW_H - 1, 440, 1, true);
    }
    if (ttdata::periodsUsed(_plan) == 0)
      c.textCentered(c.width() / 2, ROWS_Y + n * ROW_H + 8, "Tippe auf eine Stunde", TS_SMALL, true);

    c.button(PREV.x, PREV.y, PREV.w, PREV.h, "<", false, TS_LARGE);
    c.button(MID.x, MID.y, MID.w, MID.h, "WOCHE", false, TS_LARGE);
    c.button(NEXT.x, NEXT.y, NEXT.w, NEXT.h, ">", false, TS_LARGE);
  }

  void tapDay(int x, int y) {
    using namespace ttui;
    if (PREV.hit(x, y)) return step(-1);
    if (NEXT.hit(x, y)) return step(1);
    if (MID.hit(x, y)) {
      host().beep(1);
      return show(Screen::Week);
    }
    const int i = (y - ROWS_Y) / ROW_H;
    if (y >= ROWS_Y && i < rows()) {
      _lesson = i;
      host().beep(1);
      show(Screen::Pick);
    }
  }

  // --- week --------------------------------------------------------------
  void renderWeek(ToolsCanvas& c) {
    using namespace ttui;
    host().topBar("WOCHE", false, "TAG");
    for (int d = 0; d < ttdata::DAYS; d++) {
      const int x = WK_X + d * WK_COL;
      c.textInBox(x, WK_HEAD_Y, WK_COL, 30, ttdata::DAY_SHORT[d], TS_MED, true,
                  _haveClock && d == _wday);
      c.fillRect(x, WK_Y, 1, rows() * WK_ROW, true);
    }
    for (int i = 0; i < rows(); i++) {
      const int y = WK_Y + i * WK_ROW;
      char num[4];
      snprintf(num, sizeof(num), "%d", i + 1);
      c.textInBox(0, y, WK_X, WK_ROW, num, TS_SMALL, true);
      for (int d = 0; d < ttdata::DAYS; d++)
        c.textClipped(WK_X + d * WK_COL + 5, y + (WK_ROW - c.textHeight(TS_SMALL)) / 2, WK_COL - 8,
                      _plan.cell[d][i], TS_SMALL, true);
      c.fillRect(WK_X, y + WK_ROW - 1, ttdata::DAYS * WK_COL, 1, true);
    }
    c.button(MID.x, MID.y, MID.w, MID.h, "TAG", false, TS_LARGE);
  }

  void tapWeek(int x, int y) {
    using namespace ttui;
    if (MID.hit(x, y)) {
      host().beep(1);
      return show(Screen::Day);
    }
    // A tap anywhere in a day's column opens that day.
    if (x >= WK_X && y >= WK_HEAD_Y && y < WK_Y + rows() * WK_ROW) {
      const int d = (x - WK_X) / WK_COL;
      if (d < ttdata::DAYS) {
        _day = d;
        host().beep(1);
        show(Screen::Day);
      }
    }
  }

  // --- choosing a subject ------------------------------------------------
  void renderPick(ToolsCanvas& c) {
    using namespace ttui;
    host().topBar(title(), false, "TAG");
    char head[40];
    snprintf(head, sizeof(head), "%s, %d. Stunde", ttdata::DAY_LONG[_day], _lesson + 1);
    c.textCentered(c.width() / 2, HEAD_Y, head, TS_LARGE, true, true);
    const char* subj[MAX_SUBJ];
    const int n = ttdata::subjects(_plan, subj, MAX_SUBJ);
    const char* cur = _plan.cell[_day][_lesson];
    for (int i = 0; i < n; i++) {
      const TRect r = subjRect(i);
      c.button(r.x, r.y, r.w, r.h, subj[i], strcmp(subj[i], cur) == 0, TS_MED);
    }
    if (n == 0)
      c.textCentered(c.width() / 2, 300, "Noch keine F\xc3\xa4" "cher", TS_MED, true);
    c.button(NEW_BTN.x, NEW_BTN.y, NEW_BTN.w, NEW_BTN.h, "NEUES FACH", false, TS_MED);
    c.button(EMPTY_BTN.x, EMPTY_BTN.y, EMPTY_BTN.w, EMPTY_BTN.h, "FREI", false, TS_MED);
    c.button(BACK_BTN.x, BACK_BTN.y, BACK_BTN.w, BACK_BTN.h, "ZUR\xc3\x9c" "CK", false, TS_MED);
  }

  void tapPick(int x, int y) {
    using namespace ttui;
    if (BACK_BTN.hit(x, y)) {
      host().beep(1);
      return show(Screen::Day);
    }
    if (EMPTY_BTN.hit(x, y)) return setLesson("");
    if (NEW_BTN.hit(x, y)) {
      _kbd.begin("NEUES FACH", nullptr, ttdata::NAME_CHARS);
      host().beep(1);
      return show(Screen::Typing);
    }
    const char* subj[MAX_SUBJ];
    const int n = ttdata::subjects(_plan, subj, MAX_SUBJ);
    for (int i = 0; i < n; i++)
      if (subjRect(i).hit(x, y)) {
        // Copied before the write: subj points into the plan itself.
        ttdata::Name pick;
        strcpy(pick, subj[i]);
        return setLesson(pick);
      }
  }

  void tapTyping(int x, int y) {
    switch (_kbd.tap(x, y)) {
      case kbd::Keyboard::Key::Typed:
        host().beep(0);
        host().refresh(false);
        return;
      case kbd::Keyboard::Key::Ok:
        if (_kbd.empty()) return host().beep(2);
        return setLesson(_kbd.text());
      case kbd::Keyboard::Key::Cancel:
        host().beep(1);
        return show(Screen::Pick);
      default: return;
    }
  }

  void setLesson(const char* name) {
    ttdata::copyName(name, name + strlen(name), _plan.cell[_day][_lesson]);
    save();
    host().beep(1);
    show(Screen::Day);
  }

  // --- persistence -------------------------------------------------------
  void load() {
    _text[0] = 0;
    prefs().getString("tt_plan", _text, sizeof(_text));
    ttdata::fromText(_text, _plan);
  }

  void save() {
    if (ttdata::toText(_plan, _text, sizeof(_text)) > 0) prefs().putString("tt_plan", _text);
  }

  ttdata::Plan _plan{};
  char _text[3000] = {};  // a member, not a local: the loop task's stack is small
  kbd::Keyboard _kbd;
  Screen _screen = Screen::Day;
  int _day = 0, _lesson = 0;
  bool _haveClock = false;
  int _wday = 0, _minutes = 0;
  ttdata::Shown _shown{0, ttdata::When::Later};
};
