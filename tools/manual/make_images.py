#!/usr/bin/env python3
"""Makes cover.jpg and the numbered overview.png used by build.py.
usage: python3 tools/manual/make_images.py tools/manual .
(run from the repo root after refreshing docs/skin_*.png with the StretchKitPreview tool)"""
import sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

out, repo = sys.argv[1], sys.argv[2]

# cover background (A4 @150dpi): blue-violet gradient, light streak, chrome category icons
W, H = 1240, 1754
y, x = np.mgrid[0:H, 0:W].astype(float)
t = x / W * 0.4 + y / H * 0.6
top, mid, bot = np.array([0x3a, 0x3d, 0xc2]), np.array([0x5b, 0x50, 0xd8]), np.array([0x24, 0x25, 0x88])
col = np.where(t[..., None] < 0.5, top + (mid - top) * (t[..., None] / 0.5), mid + (bot - mid) * ((t[..., None] - 0.5) / 0.5))
d = (x - W * 0.25) * 0.62 + (y - H * 0.15) * -0.78
streak = np.exp(-(d / (W * 0.09)) ** 2) * 0.25 + np.exp(-((d - W * 0.07) / (W * 0.015)) ** 2) * 0.32
col = col + (255 - col) * streak[..., None]
bg = Image.fromarray(np.clip(col, 0, 255).astype(np.uint8)).convert("RGBA")
for i, n in enumerate(["kick", "snare", "clap", "hat", "cymbal", "tom", "perc", "other"]):
    ico = Image.open(f"{repo}/Resources/icons/icon_{n}.png").convert("RGBA").resize((118, 118), Image.LANCZOS)
    cx = 120 + (i % 4) * 270 + (135 if i >= 4 else 0) - 60
    cy = 1080 + (i // 4) * 190
    sh = Image.new("RGBA", ico.size, (16, 14, 70, 0))
    sh.putalpha(ico.getchannel("A").point(lambda a: int(a * 0.45)))
    bg.alpha_composite(sh.filter(ImageFilter.GaussianBlur(7)), (cx + 5, cy + 10))
    bg.alpha_composite(ico, (cx, cy))
bg.convert("RGB").save(f"{out}/cover.jpg", quality=92)

# numbered overview of the Liquid Chrome screenshot (positions match the 1180x900 editor)
shot = Image.open(f"{repo}/docs/skin_E_liquid_chrome.png").convert("RGBA")
dr = ImageDraw.Draw(shot)
f = ImageFont.truetype(f"{out}/fonts/Lato-Black.ttf", 15)
marks = [(1, (200, 14)), (2, (560, 14)), (3, (1000, 64)), (4, (416, 86)), (5, (130, 380)), (6, (96, 428)), (7, (1004, 428)),
         (8, (20, 480)), (9, (612, 480)), (10, (1060, 480)), (11, (150, 532)), (12, (214, 532)), (13, (266, 532)),
         (14, (400, 532)), (15, (510, 532))]
for n, (cx, cy) in marks:
    r = 13
    dr.ellipse((cx - r - 2, cy - r - 2, cx + r + 2, cy + r + 2), fill=(20, 18, 80, 255))
    dr.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(255, 79, 216, 255))
    tw = dr.textlength(str(n), font=f)
    dr.text((cx - tw / 2, cy - 9), str(n), font=f, fill=(255, 255, 255, 255))
shot.convert("RGB").save(f"{out}/overview.png")
print("ok")
