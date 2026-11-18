"""Whole-block edits on TimeShift Cyrillic font textures (512x512, 4x4 blocks of 16 bytes).
Only whole compressed blocks are copied: the in-block pixel order of this engine's DXT3 variant is
not standard, so mirroring/re-encoding breaks glyphs, but moving blocks between glyph cells is lossless.

Ukrainian letters (the game's 8-bit text table has no і ї є ґ):
  * The menu font draws Cyrillic Е rounded, i.e. it already looks like Є.
    -> the original Е glyph is used for Є, and a new "closed" E (Е + right stroke of О between the
       top and middle bars, like a big 'e') is built in the unused Э cell for the normal Е.
  * Lowercase є = с + middle bar of е, built in the unused э cell.
  Text mapping (see build_lang.py): Е->Э, є->э, Є->Е, е stays е.
"""

W = 512
EMPTY = bytes(16)


def blk(data, x, y):
    o = ((y // 4) * (W // 4) + x // 4) * 16
    return bytes(data[o:o + 16])


def put(data, x, y, b):
    o = ((y // 4) * (W // 4) + x // 4) * 16
    data[o:o + 16] = b


def copy_region(data, sx, sy, dx, dy, w, h):
    src = {(i, j): blk(data, sx + i, sy + j) for i in range(0, w, 4) for j in range(0, h, 4)}
    for (i, j), b in src.items():
        put(data, dx + i, dy + j, b)


def clear_region(data, x0, y0, x1, y1):
    for y in range(y0, y1, 4):
        for x in range(x0, x1, 4):
            put(data, x, y, EMPTY)


def paste_rows(data, sx, sy, dx, dy, cols, rows):
    """Copy blocks at relative columns `cols` and relative rows `rows` from glyph (sx,sy) to glyph (dx,dy)."""
    for r in rows:
        for c in cols:
            put(data, dx + c, dy + r, blk(data, sx + c, sy + r))


def make_glyph(data, cell, src, w, h, extra=()):
    """cell/src = (x, y_top, x1). Clears the target cell, copies src glyph (w x h), then applies extra
    pastes: list of (src_x, src_y, cols, rows)."""
    cx, cy, cx1 = cell
    clear_region(data, cx, cy, cx1, cy + h)
    copy_region(data, src[0], src[1], cx, cy, min(w, cx1 - cx), h)
    for sx, sy, cols, rows in extra:
        paste_rows(data, sx, sy, cx, cy, cols, rows)


def patch_main_cyr(raw):
    d = bytearray(raw)
    # Э cell -> closed E (big 'e'): Е with its upper-right part taken from Р (closed bowl)
    make_glyph(d, (316, 172, 352), (208, 16), 36, 56,
               extra=[(216, 96, range(16, 36, 4), range(0, 32, 4))])
    # э cell -> є: с + middle bar of е
    make_glyph(d, (320, 420, 356), (256, 344), 36, 44,
               extra=[(208, 264, range(12, 36, 4), range(16, 28, 4))])
    return bytes(d)


def patch_regular_cyr(raw):
    d = bytearray(raw)
    # Э cell -> plain copy of Е (this font's Е is square; Є shows as the same Е)
    make_glyph(d, (232, 232, 276), (264, 16), 40, 52)
    # э cell -> є: с + middle bar of е
    make_glyph(d, (320, 460, 360), (280, 388), 40, 44,
               extra=[(208, 316, range(12, 40, 4), range(16, 28, 4))])
    return bytes(d)
