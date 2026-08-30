// ---------------------------------------------------------------------------
// worker.js — one core's worth of picture work.
//
// The main thread hands over the compressed bytes of one image; this decodes,
// trims, turns, fits, dithers and packs it, and hands back 48,008 bytes. The
// decode is the browser's own, which is why this is worth doing off-thread:
// it is the slowest step and it releases the lock while it runs.
// ---------------------------------------------------------------------------

import { prepareArt } from "./tbi.js";

async function decode(bytes, type) {
  const bmp = await createImageBitmap(new Blob([bytes], { type: type || "" }));
  const { width: w, height: h } = bmp;
  const canvas = new OffscreenCanvas(w, h);
  // willReadFrequently keeps this on the CPU backend, which is faster here
  // and — more to the point — avoids a GPU readback that some drivers round.
  const ctx = canvas.getContext("2d", { willReadFrequently: true, alpha: true });
  ctx.drawImage(bmp, 0, 0);
  const rgba = ctx.getImageData(0, 0, w, h).data;
  bmp.close();
  return { rgba, w, h };
}

self.onmessage = async (ev) => {
  const { id, bytes, type, opts, alsoFS } = ev.data;
  try {
    const { rgba, w, h } = await decode(bytes, type);
    const { tbi, trimmed, turned } = prepareArt(rgba, w, h, opts);

    // THE SIDECAR IS NOT "PAGE ONE AGAIN". It is cover_block()'s recipe:
    // fitted whole on white, no trim, NO TURN, Floyd-Steinberg — the .tbk
    // cover spec names that algorithm, and cover_block() takes no dither
    // argument and never rotates. Page one turns a wide picture sideways;
    // a shelf cover must not, or the shelf shows a book lying on its side.
    //
    // Only when page one happened to use exactly those settings are the two
    // the same bytes, and then the second render is skipped.
    let extra = null;
    if (alsoFS && (opts.dither !== "fs" || turned)) {
      extra = prepareArt(rgba, w, h,
                         { ...opts, trim: false, turn: false, dither: "fs" }).tbi;
    }
    const move = extra ? [tbi.buffer, extra.buffer] : [tbi.buffer];
    self.postMessage({ id, ok: true, tbi, fsTbi: extra, w, h, trimmed, turned }, move);
  } catch (e) {
    self.postMessage({ id, ok: false, error: String(e && e.message || e) });
  }
};
