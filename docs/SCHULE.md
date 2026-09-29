# School apps: e-paper design rules

The school apps (Stundenplan, Aufgaben, Vokabeln, E-Mail) and the web app
that feeds them follow the same rules. Most are what the Toybox already does;
they are written down here so every new screen is checked against them.

## Ink

- **Black and white only.** No grey on UI screens: grey is a dither on this
  panel, it needs a full refresh and it ghosts.
- **Hairlines, not boxes.** Dividers between rows (1 px), a 1 px frame on a
  button. Heavy frames (3 px) mark one thing: the current lesson, the chosen
  option.
- **Fills are rare.** A filled button is THE action of a screen (OK), and it
  does not move. A selection is shown with `ToolsCanvas::option()` -- heavy
  frame and bold label -- never a fill, because a black slab that changes
  place on a partial refresh flashes and leaves a shadow.
- **No motion.** No spinners, progress bars that tick, blinking cursors or
  animated transitions. A long wait shows one static line of text.

## Type

- Content in `TS_LARGE` (subject names, due tasks); labels in `TS_MED`;
  `TS_SMALL` only for secondary facts (times, counts).
- Every screen must pass the preview harness's overflow check: text is
  clipped with `textClipped`, never allowed to run off the panel.
- German UI text, UTF-8, only glyphs the UI font carries (Latin-1 and
  Latin Extended-A; no arrows or emoji -- `<` `>` instead).

## Refresh

- **Partial** (`refreshUi`, 0.3 s) for small changes on the same screen:
  stepping a day, ticking a task, a key press (`refresh(false)`).
- **Full** (`refresh(true)`, 1.7 s) when most of the panel changes shape:
  into or out of the keyboard, after the SD card was used, QR codes, and
  anything that will stay on the panel while the device sleeps.
- The host promotes every eighth `refreshUi` to a full clean on its own;
  apps do not count.

## Layout

- Portrait 480 × 800, top bar 40 px, back button top left.
- Touch targets at least 48 px high; primary buttons at the bottom
  (y ≥ 690) where a thumb reaches while the device hangs on the fridge.
- The side buttons step through whatever the screen lists (days, tasks,
  cards), so an app is usable with touch unreliable.

## Web app

Same look on phone and PC: black on white, one sans face, hairline rules,
no colour except for errors, no animation. A parent should recognise the
device's screens in it.
