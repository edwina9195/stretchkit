#!/usr/bin/env python3
"""Lays rendered icons out on the Liquid Chrome background, like a console menu.
usage: python3 icon_sheet.py <icon-dir> <font.ttf> <out.png>"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ORDER = [("kick", "KICK"), ("snare", "SNARE"), ("clap", "CLAP"), ("hat", "HAT"),
         ("cymbal", "CYMBAL"), ("tom", "TOM"), ("perc", "PERC"), ("other", "OTHER"),
         ("play", "PLAY"), ("stop", "STOP"), ("add", "ADD FOLDER"), ("folder", "FOLDERS"),
         ("rescan", "RESCAN"), ("dice", "RANDOM"), ("midi", "DRAG MIDI")]


def background(w, h):
    y, x = np.mgrid[0:h, 0:w].astype(float)
    t = (x / w * 0.45 + y / h * 0.55)
    top, mid, bot = np.array([0x3a, 0x3d, 0xc2]), np.array([0x5b, 0x50, 0xd8]), np.array([0x26, 0x27, 0x8e])
    col = np.where(t[..., None] < 0.5, top + (mid - top) * (t[..., None] / 0.5), mid + (bot - mid) * ((t[..., None] - 0.5) / 0.5))
    # diagonal light streak
    d = (x - w * 0.3) * 0.62 + (y - h * 0.1) * -0.78
    streak = np.exp(-(d / (w * 0.06)) ** 2) * 0.28 + np.exp(-((d - w * 0.05) / (w * 0.012)) ** 2) * 0.35
    col = col + (255 - col) * streak[..., None]
    return Image.fromarray(np.clip(col, 0, 255).astype(np.uint8), "RGB").convert("RGBA")


def main():
    icons, font_path, out = Path(sys.argv[1]), sys.argv[2], sys.argv[3]
    cols, cell_w, cell_h, icon = 5, 210, 210, 128
    rows = (len(ORDER) + cols - 1) // cols
    img = background(cols * cell_w, rows * cell_h + 20)
    font = ImageFont.truetype(font_path, 22)
    try:
        font.set_variation_by_axes([14, 600])
    except Exception:
        pass

    for i, (name, label) in enumerate(ORDER):
        cx = (i % cols) * cell_w + cell_w // 2
        top = (i // cols) * cell_h + 22
        ico = Image.open(icons / f"icon_{name}.png").convert("RGBA").resize((icon, icon), Image.LANCZOS)
        shadow = Image.new("RGBA", ico.size, (16, 14, 70, 0))
        shadow.putalpha(ico.getchannel("A").point(lambda a: int(a * 0.45)))
        shadow = shadow.filter(ImageFilter.GaussianBlur(6))
        img.alpha_composite(shadow, (cx - icon // 2 + 4, top + 8))
        img.alpha_composite(ico, (cx - icon // 2, top))
        d = ImageDraw.Draw(img)
        tw = d.textlength(label, font=font)
        d.text((cx - tw / 2, top + icon + 14), label, font=font, fill=(255, 255, 255, 255))

    img.convert("RGB").save(out)


if __name__ == "__main__":
    main()
