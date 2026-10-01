# School apps: e-paper design rules

The school apps (Stundenplan, Aufgaben, Vokabeln, E-Mail), the Abenteuer
game and the web app that feeds them follow the same rules. Most are what the
Toybox already does; they are written down here so every new screen is
checked against them. Sources are at the end.

The one idea behind all of them: **treat the panel like print, not like a
slow monitor.** A screen is a page that gets reprinted, not a surface that
moves.

## Ink

- **Black and white only.** No grey on UI screens: grey is a dither on this
  panel, it needs a full refresh and it ghosts. Pictures, if any, are
  dithered to black and white before they reach the panel.
- **Little black.** Large solid black areas are what ghosts worst and what
  flashes loudest. Mostly white page, hairline rules (1 px), 1 px button
  frames.
- **Heavy frames mark one thing** (3 px): the current lesson, the chosen
  option.
- **Fills are rare.** A filled button is THE action of a screen (OK), and it
  does not move. A selection is shown with `ToolsCanvas::option()` -- heavy
  frame and bold label -- never a fill, because a black slab that changes
  place on a partial refresh flashes and leaves a shadow.
- **Sprites are outlines.** Anything that moves between partial refreshes
  (the hero and slimes in Abenteuer) is drawn as an outline, so what it
  leaves behind is a thin line, not a block.
- **No motion.** No spinners, progress bars that tick, blinking cursors or
  animated transitions. Hierarchy and state come from layout, size, weight
  and frames -- never from movement.

## Type

- **Hierarchy by size and weight**, not by colour: content in `TS_LARGE`
  (subject names, due tasks), labels in `TS_MED`, `TS_SMALL` only for
  secondary facts (times, counts). Bold for "now" and "chosen".
- **As large as the space allows.** Where a value may or may not fit (a
  subject in the week grid), measure it and step down a size only when it
  must -- the device is read from across a kitchen.
- Every screen must pass the preview harness's overflow check: text is
  clipped with `textClipped`, never allowed to run off the panel.
- German UI text, UTF-8, only glyphs the UI font carries (Latin-1 and
  Latin Extended-A; no arrows or emoji -- `<` `>` instead).

## Refresh

- **Partial** (`refreshUi`, 0.3 s) for small changes on the same screen:
  stepping a day, ticking a task, a step in the game, a key press
  (`refresh(false)`).
- **Full** (`refresh(true)`, 1.7 s) when most of the panel changes shape:
  into or out of the keyboard, a new room, an end screen, after the SD card
  was used, QR codes, and anything that will stay on the panel while the
  device sleeps.
- **Bounded ghosting.** Five to ten partials between full cleans is the
  usual advice. The host promotes every eighth `refreshUi` to a full clean
  on its own; apps do not count. A run of raw `refresh(false)` (typing) must
  end in a full refresh when the screen is left.
- **Paginate, never scroll.** A scroll is dozens of partials in a row; a page
  turn is one. Lists get pages and the side buttons turn them.
- **Paint once, when the work is done.** A sync or a long computation shows
  one static line ("Lädt …"), then the finished screen -- never a screen
  that fills in piece by piece.

## Feedback

- **Sound first, ink second.** A touch should be answered within about
  70 ms to feel connected to the finger; the panel needs 300 ms. So every
  tap beeps *before* the refresh is asked for (`beep()` then `refresh…()`),
  and a tap that does nothing (a wall in Abenteuer) beeps low and paints
  nothing.
- **Wrong taps cost nothing.** A blocked move, an empty key, a tap between
  buttons: no state change, no refresh.

## The sleeping screen

- Whatever is left on the panel while the device sleeps (the pinned note,
  later the Heute page) is drawn with a full refresh, so it carries no
  shadows for hours.
- It is redrawn at least once a day: the date stays right, and the same
  image held for days on end can leave a faint imprint on the panel.

## Layout

- Portrait 480 × 800, top bar 40 px, back button top left.
- **Fixed zones.** What does not change (bar, buttons, labels) stays in the
  same place on every screen of an app; only the content zone changes, so a
  partial has the fewest pixels to move.
- Touch targets at least 48 px high; primary buttons at the bottom
  (y ≥ 690) where a thumb reaches while the device hangs on the fridge.
- The side buttons step through whatever the screen lists (days, tasks,
  cards, pages), so an app is usable with touch unreliable.

## Checking a screen

Open it in the web emulator (`emu/`, https://sticky.beseler.org) with
**Ghosting zeigen** on and use it for a minute. A grey residue that builds
up is a screen that moves too much black on partials; the counter shows how
many partials ran since the last full clean.

## Web app

Same look on phone and PC: black on white, one sans face, hairline rules,
no colour except for errors, no animation. A parent should recognise the
device's screens in it.

## Sources

- E-ink display integration best practices (refresh cadence, full clean every
  5–10 partials, image retention):
  https://sempervent.github.io/best-practices/esp32/e-ink-display-best-practices/
- Ask HN, conventions for e-ink UI development (print not screen, no
  animation, pagination over scrolling, paint finished results):
  https://daily.dev/posts/ask-hn-in-your-experience-what-are-sound-conventions-for-e-ink-ui-development--magn6xjre
- A dedicated e-paper design system for mobile phones, CHI 2026 (minimise
  large black areas, hierarchy by icon, spacing and type):
  https://doi.org/10.1145/3772318.3791459
- TRMNL developer platform (1-bit components, dithering, no-scroll layouts,
  values sized to fit): https://trmnl.com/developers
- Latency guidelines for touchscreen button feedback (audio 20–70 ms, visual
  30–85 ms, 300 ms rated clearly worse):
  https://www.researchgate.net/publication/341354128_Latency_Guidelines_for_Touchscreen_Virtual_Button_Feedback
