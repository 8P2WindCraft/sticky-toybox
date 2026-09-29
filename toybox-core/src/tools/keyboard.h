// A German on-screen keyboard: QWERTZ with the umlauts, lower and upper case,
// and a second page for digits and punctuation.
//
// The picker's own keyboard is capitals-only QWERTY, which is fine for a name
// on a list and useless for "Raum 204" or "Übungen S. 42". This one is shared
// by the school apps. It draws and hit-tests itself and hands back what the
// tap meant; beeping and refreshing stay with the app, like everywhere else.
#pragma once
#include <string.h>

#include "tools_ui.h"
#include "unicode.h"

namespace kbd {

inline constexpr int MAX_CHARS = 32;
inline constexpr int MAX_BYTES = MAX_CHARS * 3;

inline constexpr int KEY_W = 40, KEY_H = 56, KEY_GAP = 3, ROW_X = 5;
inline constexpr int ROW1_Y = 330, ROW2_Y = 394, ROW3_Y = 458, ROW4_Y = 522;
inline constexpr TRect SHIFT{5, ROW3_Y, 61, KEY_H};
inline constexpr int ROW3_X = 69;  // eight keys between SHIFT and DEL
inline constexpr TRect DEL{413, ROW3_Y, 62, KEY_H};
inline constexpr TRect PAGE{5, ROW4_Y, 100, KEY_H};
inline constexpr TRect SPACE{110, ROW4_Y, 260, KEY_H};
inline constexpr TRect COMMA{375, ROW4_Y, 48, KEY_H};
inline constexpr TRect DOT{427, ROW4_Y, 48, KEY_H};
inline constexpr TRect CANCEL{20, 606, 215, 64};
inline constexpr TRect OK{245, 606, 215, 64};

// Lower case, upper case, symbols: eleven, eleven and eight keys a row.
inline const char* const KEYS_LO[3][11] = {
    {"q", "w", "e", "r", "t", "z", "u", "i", "o", "p", "\xc3\xbc"},
    {"a", "s", "d", "f", "g", "h", "j", "k", "l", "\xc3\xb6", "\xc3\xa4"},
    {"y", "x", "c", "v", "b", "n", "m", "\xc3\x9f"}};
inline const char* const KEYS_UP[3][11] = {
    {"Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P", "\xc3\x9c"},
    {"A", "S", "D", "F", "G", "H", "J", "K", "L", "\xc3\x96", "\xc3\x84"},
    {"Y", "X", "C", "V", "B", "N", "M", "\xc3\x9f"}};
inline const char* const KEYS_SYM[3][11] = {
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-"},
    {"/", ":", ";", "(", ")", "&", "@", "?", "!", "+", "="},
    {"'", "\"", "#", "%", "*", "_", "<", ">"}};
inline constexpr int ROW_N[3] = {11, 11, 8};

class Keyboard {
 public:
  enum class Key : uint8_t { None, Typed, Ok, Cancel };

  void begin(const char* prompt, const char* initial, int maxChars = MAX_CHARS) {
    _prompt = prompt;
    _max = maxChars > MAX_CHARS ? MAX_CHARS : maxChars;
    _len = 0;
    _buf[0] = 0;
    _sym = false;
    if (initial) {
      for (const char* p = initial; *p && uni::count(_buf) < _max;) {
        const char* q = p;
        uni::next(q);
        if (_len + (int)(q - p) > MAX_BYTES) break;
        while (p < q) _buf[_len++] = *p++;
        _buf[_len] = 0;
      }
    }
    _shift = _len == 0;  // a new word starts with a capital; most here are nouns
  }

  const char* text() const { return _buf; }
  bool empty() const { return _len == 0; }

  void render(ToolsCanvas& c) {
    c.textCentered(c.width() / 2, 54, _prompt, TS_LARGE, true, true);
    c.drawRect(20, 110, 440, 64, 3, true);
    // The end of a long entry is the part being typed, so it is what shows.
    const char* shown = _buf;
    while (*shown && c.textWidth(shown, TS_LARGE) > 420) uni::next(shown);
    c.textInBox(20, 110, 440, 64, _len ? shown : "_", TS_LARGE, true, true);
    char count[16];
    snprintf(count, sizeof(count), "%d / %d", uni::count(_buf), _max);
    c.textCentered(c.width() / 2, 186, count, TS_SMALL, true);

    const char* const(*rows)[11] = _sym ? KEYS_SYM : (_shift ? KEYS_UP : KEYS_LO);
    for (int r = 0; r < 3; r++)
      for (int i = 0; i < ROW_N[r]; i++) {
        const TRect k = keyRect(r, i);
        c.button(k.x, k.y, k.w, k.h, rows[r][i], false, TS_MED);
      }
    if (!_sym) c.option(SHIFT.x, SHIFT.y, SHIFT.w, SHIFT.h, "Aa", _shift, TS_MED);
    c.button(DEL.x, DEL.y, DEL.w, DEL.h, "DEL", false, TS_SMALL);
    c.button(PAGE.x, PAGE.y, PAGE.w, PAGE.h, _sym ? "ABC" : "123", false, TS_MED);
    c.button(SPACE.x, SPACE.y, SPACE.w, SPACE.h, "Leertaste", false, TS_MED);
    c.button(COMMA.x, COMMA.y, COMMA.w, COMMA.h, ",", false, TS_MED);
    c.button(DOT.x, DOT.y, DOT.w, DOT.h, ".", false, TS_MED);
    c.button(CANCEL.x, CANCEL.y, CANCEL.w, CANCEL.h, "ABBRECHEN", false, TS_MED);
    c.button(OK.x, OK.y, OK.w, OK.h, "OK", true, TS_LARGE);
  }

  Key tap(int x, int y) {
    if (OK.hit(x, y)) return Key::Ok;
    if (CANCEL.hit(x, y)) return Key::Cancel;
    if (DEL.hit(x, y)) {
      if (_len == 0) return Key::None;
      do _len--;
      while (_len > 0 && ((uint8_t)_buf[_len] & 0xC0) == 0x80);
      _buf[_len] = 0;
      return Key::Typed;
    }
    if (PAGE.hit(x, y)) {
      _sym = !_sym;
      return Key::Typed;
    }
    if (!_sym && SHIFT.hit(x, y)) {
      _shift = !_shift;
      return Key::Typed;
    }
    if (SPACE.hit(x, y)) return type(" ");
    if (COMMA.hit(x, y)) return type(",");
    if (DOT.hit(x, y)) return type(".");
    const char* const(*rows)[11] = _sym ? KEYS_SYM : (_shift ? KEYS_UP : KEYS_LO);
    for (int r = 0; r < 3; r++)
      for (int i = 0; i < ROW_N[r]; i++)
        if (keyRect(r, i).hit(x, y)) {
          const Key k = type(rows[r][i]);
          if (k == Key::Typed) _shift = false;  // one capital, then lower case
          return k;
        }
    return Key::None;
  }

 private:
  static TRect keyRect(int row, int i) {
    const int x0 = row == 2 ? ROW3_X : ROW_X;
    const int y = row == 0 ? ROW1_Y : row == 1 ? ROW2_Y : ROW3_Y;
    return TRect{x0 + i * (KEY_W + KEY_GAP), y, KEY_W, KEY_H};
  }

  Key type(const char* s) {
    const int n = (int)strlen(s);
    if (uni::count(_buf) >= _max || _len + n > MAX_BYTES) return Key::None;
    memcpy(_buf + _len, s, (size_t)n + 1);
    _len += n;
    return Key::Typed;
  }

  const char* _prompt = "";
  char _buf[MAX_BYTES + 1] = {};
  int _len = 0, _max = MAX_CHARS;
  bool _shift = true, _sym = false;
};

}  // namespace kbd
