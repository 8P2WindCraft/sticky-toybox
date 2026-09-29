// Abenteuer: a small top-down adventure, one room per screen, one step per
// turn -- the shape of an old handheld dungeon crawl, cut to what e-paper can
// do. Nothing moves unless the player does: every step is one partial
// refresh, and the slimes take their turn inside it.
//
// Four rooms in a two-by-two world. Find the key in the forest, open the
// door in the castle, take the crystal. Walking into a slime strikes it;
// a slime next to you strikes back. Falling over sends you back to the
// meadow with full hearts and everything you found still in your pocket.
//
// Pure logic, no drawing, no Arduino: the host tests walk it.
#pragma once
#include <stdint.h>
#include <string.h>

namespace quest {

constexpr int W = 10, H = 13;  // tiles per room; 48 px each on the panel
constexpr int RW = 2, RH = 2, ROOMS = RW * RH;
constexpr int MAX_EN = 6;
constexpr int ITEMS_PER_ROOM = 4;
constexpr int START_HP = 3, MAX_HP = 5;
constexpr int CHASE = 4;  // a slime this close (steps) comes for you
constexpr uint8_t VERSION = 1;

// '#' stone, 'T' tree, '~' water, 'D' locked door, 'K' key, 'H' heart,
// 'C' the crystal, 'S' a slime's spawn, '@' where the game starts.
inline const char* const MAPS[ROOMS][H] = {
    {"TTTTTTTTTT", "T........T", "T..T.....T", "T........T", "T....T...T", "T.@......T",
     "T.........", "T...T....T", "T........T", "T.T....T.T", "T........T", "T........T",
     "TTTT.TTTTT"},
    {"TTTTTTTTTT", "T....T...T", "T.S.....KT", "T..TT....T", "T........T", "T.T...S..T",
     ".........T", "T....TT..T", "T........T", "T.T..S...T", "T........T", "T....T...T",
     "TTTTTTTTTT"},
    {"TTTT.TTTTT", "T........T", "T.~~~~...T", "T.~~~~~..T", "T..~~~...T", "T.....S..T",
     "T.........", "T..T.....T", "T.....~~~T", "T.H..~~~~T", "T.....~~~T", "T..S.....T",
     "TTTTTTTTTT"},
    {"##########", "#........#", "#.S....S.#", "#........#", "#.######.#", "#.#....#.#",
     "..#..C.#.#", "#.#....#.#", "#.###D##.#", "#........#", "#..S.....#", "#........#",
     "##########"},
};
inline const char* const ROOM_NAMES[ROOMS] = {"Wiese", "Wald", "See", "Burg"};

struct Enemy {
  int8_t x, y;
};

struct State {
  uint8_t version;
  uint8_t room;
  int8_t x, y;
  uint8_t hp, keys;
  uint8_t doorOpen, won;
  uint16_t taken;  // bit room * ITEMS_PER_ROOM + n: the n-th item of a room is gone
  uint16_t steps;
  uint32_t rng;
  uint8_t nEn;
  Enemy en[MAX_EN];
};

enum Event : uint16_t {
  EV_NONE = 0,
  EV_MOVED = 1 << 0,
  EV_BLOCKED = 1 << 1,
  EV_KILLED = 1 << 2,
  EV_HURT = 1 << 3,
  EV_KEY = 1 << 4,
  EV_HEART = 1 << 5,
  EV_DOOR = 1 << 6,
  EV_LOCKED = 1 << 7,
  EV_ROOM = 1 << 8,
  EV_WON = 1 << 9,
  EV_DIED = 1 << 10,
};

inline bool isItem(char t) { return t == 'K' || t == 'H' || t == 'C'; }

// Which item of its room the one at (x, y) is, counting in reading order.
inline int itemIndex(int room, int x, int y) {
  int n = 0;
  for (int j = 0; j < H; j++)
    for (int i = 0; i < W; i++) {
      if (i == x && j == y) return n;
      if (isItem(MAPS[room][j][i])) n++;
    }
  return -1;
}

inline bool isTaken(const State& s, int room, int x, int y) {
  const int n = itemIndex(room, x, y);
  return n >= 0 && n < ITEMS_PER_ROOM && (s.taken & (1u << (room * ITEMS_PER_ROOM + n)));
}

// The tile as it is now: taken items and an opened door are floor, and the
// spawn and start marks were only ever floor with a note on it.
inline char tileAt(const State& s, int room, int x, int y) {
  if (x < 0 || y < 0 || x >= W || y >= H) return '#';
  const char t = MAPS[room][y][x];
  if (t == 'S' || t == '@') return '.';
  if (t == 'D' && s.doorOpen) return '.';
  if (isItem(t) && isTaken(s, room, x, y)) return '.';
  return t;
}

inline int enemyAt(const State& s, int x, int y) {
  for (int i = 0; i < s.nEn; i++)
    if (s.en[i].x == x && s.en[i].y == y) return i;
  return -1;
}

inline void enterRoom(State& s, int room, int x, int y) {
  s.room = (uint8_t)room;
  s.x = (int8_t)x;
  s.y = (int8_t)y;
  // Slimes come back whenever a room is entered, the way the old games did:
  // a room is never permanently safe, which is most of the tension there is.
  s.nEn = 0;
  for (int j = 0; j < H; j++)
    for (int i = 0; i < W; i++)
      if (MAPS[room][j][i] == 'S' && s.nEn < MAX_EN) s.en[s.nEn++] = Enemy{(int8_t)i, (int8_t)j};
}

inline void startPos(int& x, int& y) {
  for (int j = 0; j < H; j++)
    for (int i = 0; i < W; i++)
      if (MAPS[0][j][i] == '@') {
        x = i;
        y = j;
        return;
      }
  x = y = 1;
}

inline void newGame(State& s, uint32_t seed = 12345) {
  memset(&s, 0, sizeof(s));
  s.version = VERSION;
  s.hp = START_HP;
  s.rng = seed ? seed : 1;
  int x, y;
  startPos(x, y);
  enterRoom(s, 0, x, y);
}

// Back on the meadow with full hearts; keys, the open door and taken items stay.
inline void respawn(State& s) {
  s.hp = START_HP;
  int x, y;
  startPos(x, y);
  enterRoom(s, 0, x, y);
}

inline uint32_t nextRand(State& s) {
  s.rng = s.rng * 1103515245u + 12345u;
  return s.rng >> 16;
}

inline bool enemyCanStand(const State& s, int x, int y) {
  return tileAt(s, s.room, x, y) == '.' && enemyAt(s, x, y) < 0 && !(x == s.x && y == s.y);
}

inline int iabs(int v) { return v < 0 ? -v : v; }

inline uint16_t enemiesTurn(State& s) {
  uint16_t ev = EV_NONE;
  for (int i = 0; i < s.nEn; i++) {
    Enemy& e = s.en[i];
    const int dx = s.x - e.x, dy = s.y - e.y;
    const int dist = iabs(dx) + iabs(dy);
    if (dist == 1) {
      if (s.hp > 0) s.hp--;
      ev |= EV_HURT;
      continue;
    }
    int mx = 0, my = 0;
    if (dist <= CHASE) {
      // Toward the hero along the longer gap first, the other if that is blocked.
      const int sx = dx > 0 ? 1 : dx < 0 ? -1 : 0, sy = dy > 0 ? 1 : dy < 0 ? -1 : 0;
      if (iabs(dx) >= iabs(dy)) {
        if (sx && enemyCanStand(s, e.x + sx, e.y)) mx = sx;
        else if (sy && enemyCanStand(s, e.x, e.y + sy)) my = sy;
      } else {
        if (sy && enemyCanStand(s, e.x, e.y + sy)) my = sy;
        else if (sx && enemyCanStand(s, e.x + sx, e.y)) mx = sx;
      }
    } else {
      static const int8_t D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
      const uint32_t r = nextRand(s) % 6;  // four ways, or stay put
      if (r < 4 && enemyCanStand(s, e.x + D[r][0], e.y + D[r][1])) {
        mx = D[r][0];
        my = D[r][1];
      }
    }
    e.x = (int8_t)(e.x + mx);
    e.y = (int8_t)(e.y + my);
  }
  if (s.hp == 0) ev |= EV_DIED;
  return ev;
}

// Standing still is a move too: the slimes still take their turn.
inline uint16_t wait(State& s) {
  if (s.won) return EV_NONE;
  s.steps++;
  return enemiesTurn(s);
}

// One step of the hero. Bumping a wall costs nothing -- no turn passes --
// so a mistaken tap is never punished.
inline uint16_t step(State& s, int dx, int dy) {
  if (s.won || s.hp == 0) return EV_NONE;
  const int nx = s.x + dx, ny = s.y + dy;

  if (nx < 0 || ny < 0 || nx >= W || ny >= H) {
    int rx = s.room % RW + (nx < 0 ? -1 : nx >= W ? 1 : 0);
    int ry = s.room / RW + (ny < 0 ? -1 : ny >= H ? 1 : 0);
    if (rx < 0 || ry < 0 || rx >= RW || ry >= RH) return EV_BLOCKED;
    const int room = ry * RW + rx;
    const int ex = (nx + W) % W, ey = (ny + H) % H;
    if (tileAt(s, room, ex, ey) != '.') return EV_BLOCKED;
    s.steps++;
    enterRoom(s, room, ex, ey);
    return EV_ROOM;
  }

  const int e = enemyAt(s, nx, ny);
  if (e >= 0) {
    s.en[e] = s.en[--s.nEn];  // one hit is enough for a slime
    s.steps++;
    return EV_KILLED | enemiesTurn(s);
  }

  const char t = tileAt(s, s.room, nx, ny);
  if (t == '#' || t == 'T' || t == '~') return EV_BLOCKED;
  if (t == 'D') {
    if (s.keys == 0) return EV_LOCKED;
    s.keys--;
    s.doorOpen = 1;
    s.steps++;
    return EV_DOOR | enemiesTurn(s);
  }

  uint16_t ev = EV_MOVED;
  s.x = (int8_t)nx;
  s.y = (int8_t)ny;
  s.steps++;
  if (isItem(t)) {
    const int n = itemIndex(s.room, nx, ny);
    if (n >= 0 && n < ITEMS_PER_ROOM) s.taken |= (uint16_t)(1u << (s.room * ITEMS_PER_ROOM + n));
    if (t == 'K') {
      s.keys++;
      ev |= EV_KEY;
    } else if (t == 'H') {
      s.hp = s.hp < MAX_HP ? s.hp + 1 : MAX_HP;
      ev |= EV_HEART;
    } else {
      s.won = 1;
      return ev | EV_WON;
    }
  }
  return ev | enemiesTurn(s);
}

// Is a saved state one this build can carry on from?
inline bool valid(const State& s) {
  if (s.version != VERSION || s.room >= ROOMS || s.nEn > MAX_EN || s.hp > MAX_HP) return false;
  if (s.x < 0 || s.y < 0 || s.x >= W || s.y >= H) return false;
  for (int i = 0; i < s.nEn; i++)
    if (s.en[i].x < 0 || s.en[i].y < 0 || s.en[i].x >= W || s.en[i].y >= H) return false;
  return true;
}

}  // namespace quest
