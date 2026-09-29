// The school timetable: five days, up to ten lessons a day, a subject name in
// each, and optionally the time each lesson starts.
//
// Stored as plain text, the same text a phone page or the server will send:
//
//   Zeiten: 7:45 8:35 9:40 10:30 11:35 12:25
//   Mo: Mathe | Deutsch | | Sport | Sport
//   Di: Englisch | Mathe | Kunst
//
// One parser for every path, and a format a parent can read and fix by hand.
// No Arduino types in here, so the host tests can pin the parser down.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "unicode.h"

namespace ttdata {

constexpr int DAYS = 5;
constexpr int PERIODS = 10;
constexpr int NAME_CHARS = 16;  // "Religion/Ethik" fits; a tile row does not want more
constexpr int NAME_BYTES = NAME_CHARS * 3;
constexpr int LESSON_MIN = 45;
// Without start times the day view turns to tomorrow at three in the
// afternoon: late enough that nobody packs their bag for the wrong day.
constexpr int NO_TIMES_SWITCH = 15 * 60;

using Name = char[NAME_BYTES + 1];

struct Plan {
  Name cell[DAYS][PERIODS];
  uint16_t start[PERIODS];  // minutes after midnight, 0 = not set
};

inline const char* const DAY_SHORT[DAYS] = {"Mo", "Di", "Mi", "Do", "Fr"};
inline const char* const DAY_LONG[DAYS] = {"Montag", "Dienstag", "Mittwoch", "Donnerstag",
                                           "Freitag"};

inline void clear(Plan& p) { memset(&p, 0, sizeof(p)); }

// Highest lesson with anything in it on any day, plus one. 0 for an empty plan.
inline int periodsUsed(const Plan& p) {
  int n = 0;
  for (int d = 0; d < DAYS; d++)
    for (int i = 0; i < PERIODS; i++)
      if (p.cell[d][i][0] && i + 1 > n) n = i + 1;
  return n;
}

inline int lastLesson(const Plan& p, int day) {
  for (int i = PERIODS - 1; i >= 0; i--)
    if (p.cell[day][i][0]) return i;
  return -1;
}

// Copies [b, e) into out, trimmed, cut at NAME_CHARS on a codepoint boundary.
inline void copyName(const char* b, const char* e, Name& out) {
  while (b < e && (*b == ' ' || *b == '\t')) b++;
  while (e > b && (e[-1] == ' ' || e[-1] == '\t')) e--;
  int len = 0, chars = 0;
  for (const char* q = b; q < e && chars < NAME_CHARS;) {
    const char* nx = q;
    uni::next(nx);
    if (nx > e || len + (int)(nx - q) > NAME_BYTES) break;
    if ((uint8_t)*q >= 32) {
      memcpy(out + len, q, (size_t)(nx - q));
      len += (int)(nx - q);
      chars++;
    }
    q = nx;
  }
  out[len] = 0;
}

// "7:45" or "07.45" -> 465; anything else -> 0.
inline uint16_t parseTime(const char* b, const char* e) {
  int h = 0, m = 0, digits = 0;
  const char* p = b;
  for (; p < e && *p >= '0' && *p <= '9'; p++, digits++) h = h * 10 + (*p - '0');
  if (digits == 0 || digits > 2 || p >= e || (*p != ':' && *p != '.')) return 0;
  p++;
  digits = 0;
  for (; p < e && *p >= '0' && *p <= '9'; p++, digits++) m = m * 10 + (*p - '0');
  if (digits != 2 || p != e || h > 23 || m > 59) return 0;
  return (uint16_t)(h * 60 + m);
}

inline bool startsWithCI(const char* s, const char* e, const char* word) {
  for (; *word; word++, s++) {
    if (s >= e) return false;
    char c = *s;
    if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    char w = *word;
    if (w >= 'A' && w <= 'Z') w = (char)(w + 32);
    if (c != w) return false;
  }
  return true;
}

// Lines it does not recognise are skipped, so a stray heading or a blank line
// from a phone's text box costs nothing. Returns how many day lines it read.
inline int fromText(const char* text, Plan& p) {
  clear(p);
  int days = 0;
  for (const char* line = text; *line;) {
    const char* eol = line;
    while (*eol && *eol != '\n' && *eol != '\r') eol++;
    const char* colon = line;
    while (colon < eol && *colon != ':') colon++;
    if (colon < eol) {
      const char* body = colon + 1;
      if (startsWithCI(line, colon, "Zeiten")) {
        int i = 0;
        for (const char* q = body; q < eol && i < PERIODS;) {
          while (q < eol && (*q == ' ' || *q == ',' || *q == ';')) q++;
          const char* t = q;
          while (q < eol && *q != ' ' && *q != ',' && *q != ';') q++;
          if (q > t) p.start[i++] = parseTime(t, q);
        }
      } else {
        for (int d = 0; d < DAYS; d++) {
          if (colon - line < 2 || !startsWithCI(line, colon, DAY_SHORT[d])) continue;
          int i = 0;
          for (const char* q = body; i < PERIODS;) {
            const char* bar = q;
            while (bar < eol && *bar != '|') bar++;
            copyName(q, bar, p.cell[d][i++]);
            if (bar >= eol) break;
            q = bar + 1;
          }
          days++;
          break;
        }
      }
    }
    line = *eol ? eol + 1 : eol;
    if (*line == '\n') line++;
  }
  return days;
}

// Writes the text form into out; returns its length, or 0 if cap was too small.
inline size_t toText(const Plan& p, char* out, size_t cap) {
  size_t n = 0;
  auto put = [&](const char* s) {
    const size_t l = strlen(s);
    if (n + l + 1 > cap) return false;
    memcpy(out + n, s, l + 1);
    n += l;
    return true;
  };
  int lastTime = -1;
  for (int i = 0; i < PERIODS; i++)
    if (p.start[i]) lastTime = i;
  if (lastTime >= 0) {
    if (!put("Zeiten:")) return 0;
    for (int i = 0; i <= lastTime; i++) {
      char t[8];
      if (p.start[i])
        snprintf(t, sizeof(t), " %d:%02d", p.start[i] / 60, p.start[i] % 60);
      else
        snprintf(t, sizeof(t), " -");
      if (!put(t)) return 0;
    }
    if (!put("\n")) return 0;
  }
  for (int d = 0; d < DAYS; d++) {
    if (!put(DAY_SHORT[d]) || !put(":")) return 0;
    const int last = lastLesson(p, d);
    for (int i = 0; i <= last; i++) {
      if (i > 0 && !put(" |")) return 0;
      if (p.cell[d][i][0] && (!put(" ") || !put(p.cell[d][i]))) return 0;
    }
    if (!put("\n")) return 0;
  }
  return n;
}

// 0 = Monday .. 6 = Sunday, Gregorian calendar (Sakamoto's method).
inline int weekday(int y, int m, int d) {
  static const int t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  const int sun0 = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
  return (sun0 + 6) % 7;
}

enum class When : uint8_t { Today, Tomorrow, Later };
struct Shown {
  int day;
  When when;
};

// Which day a glance at the device should answer for: today until school is
// out, then the next school day -- the one the bag is being packed for.
inline Shown dayToShow(const Plan& p, int wday, int minutes) {
  if (wday < DAYS) {
    const int last = lastLesson(p, wday);
    int over = NO_TIMES_SWITCH;
    if (last >= 0 && p.start[last]) over = p.start[last] + LESSON_MIN;
    if (minutes < over) return Shown{wday, When::Today};
  }
  if (wday < DAYS - 1) return Shown{wday + 1, When::Tomorrow};
  return Shown{0, wday == 6 ? When::Tomorrow : When::Later};  // Friday evening, Saturday
}

// The lesson running at `minutes` on a day with start times, or -1.
inline int lessonAt(const Plan& p, int minutes) {
  for (int i = 0; i < PERIODS; i++)
    if (p.start[i] && minutes >= p.start[i] && minutes < p.start[i] + LESSON_MIN) return i;
  return -1;
}

// The distinct subjects in the plan, in first-seen order, for the chooser.
inline int subjects(const Plan& p, const char* out[], int max) {
  int n = 0;
  for (int d = 0; d < DAYS; d++)
    for (int i = 0; i < PERIODS; i++) {
      const char* s = p.cell[d][i];
      if (!s[0]) continue;
      bool seen = false;
      for (int k = 0; k < n && !seen; k++) seen = strcmp(out[k], s) == 0;
      if (!seen && n < max) out[n++] = s;
    }
  return n;
}

}  // namespace ttdata
