#!/usr/bin/env python3
"""
Renders StretchKit's liquid-chrome icon set.

A tiny signed-distance-field ray marcher: every icon is a 3D shape (tubes along 2D
curves, puffy extruded silhouettes, or blobs of spheres) shaded only by reflections of
a procedural "studio" environment - bright sky, a dark horizon band and pink / teal
iridescent tints - which is what gives liquid chrome its look.

usage: python3 chrome_icons.py <out-dir> [size]
"""
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

FIELD = 900          # resolution of 2D distance fields
EXT = 1.3            # 2D fields cover [-EXT, EXT]^2
CAM = 1.18           # orthographic camera half-width


# ----------------------------------------------------------------------------- 2D fields
def _px(x, y):
    return ((x + EXT) / (2 * EXT) * FIELD, (EXT - y) / (2 * EXT) * FIELD)


def curve_field(curves):
    """Unsigned distance (world units) to a set of polylines."""
    img = Image.new("L", (FIELD, FIELD), 0)
    d = ImageDraw.Draw(img)
    for pts in curves:
        d.line([_px(x, y) for x, y in pts], fill=255, width=2)
    mask = np.array(img) > 0
    return ndimage.distance_transform_edt(~mask) * (2 * EXT / FIELD)


def shape_field(add, sub=()):
    """Signed distance (negative inside) to filled polygons (add minus sub)."""
    img = Image.new("L", (FIELD, FIELD), 0)
    d = ImageDraw.Draw(img)
    for poly in add:
        d.polygon([_px(x, y) for x, y in poly], fill=255)
    for poly in sub:
        d.polygon([_px(x, y) for x, y in poly], fill=0)
    mask = np.array(img) > 0
    out = ndimage.distance_transform_edt(~mask)
    inside = ndimage.distance_transform_edt(mask)
    signed = (out - inside) * (2 * EXT / FIELD)
    # soften the creases along the medial axis so pillows look inflated, not faceted
    return ndimage.gaussian_filter(signed, FIELD * 0.018)


def sample(field, x, y):
    col = (x + EXT) / (2 * EXT) * FIELD
    row = (EXT - y) / (2 * EXT) * FIELD
    return ndimage.map_coordinates(field, [row, col], order=1, mode="nearest")


def circle(cx, cy, r, n=200, a0=0.0, a1=2 * math.pi):
    return [(cx + r * math.cos(a0 + (a1 - a0) * i / n), cy + r * math.sin(a0 + (a1 - a0) * i / n)) for i in range(n + 1)]


def chaikin(pts, rounds=4, closed=True):
    for _ in range(rounds):
        out = []
        n = len(pts)
        for i in range(n if closed else n - 1):
            p, q = np.array(pts[i]), np.array(pts[(i + 1) % n])
            out += [tuple(0.75 * p + 0.25 * q), tuple(0.25 * p + 0.75 * q)]
        if closed:
            out.append(out[0])
        pts = out
    return pts


# ----------------------------------------------------------------------------- 3D SDF helpers
def tube(field, radius):
    return lambda p: np.sqrt(sample(field, p[:, 0], p[:, 1]) ** 2 + p[:, 2] ** 2) - radius


def puffy(field, half_thickness, width=0.45):
    """A pillow: the 2D silhouette inflated with a rounded (elliptic) profile."""
    h, w = half_thickness, width
    def f(p):
        d2 = sample(field, p[:, 0], p[:, 1])
        s = np.clip(-d2 / w, 0.0, 1.0)
        height = h * np.sqrt(np.clip(1.0 - (1.0 - s) ** 2, 0.0, 1.0))
        return np.maximum(d2, (np.abs(p[:, 2]) - height) * 0.6)
    return f


def sphere(c, r):
    c = np.array(c, dtype=float)
    return lambda p: np.linalg.norm(p - c, axis=1) - r


def ellipsoid(c, radii):
    c, radii = np.array(c, dtype=float), np.array(radii, dtype=float)
    def f(p):
        q = (p - c) / radii
        k0 = np.linalg.norm(q, axis=1)
        k1 = np.linalg.norm(q / radii, axis=1)
        return k0 * (k0 - 1.0) / np.maximum(k1, 1e-6)
    return f


def capsule(a, b, r):
    a, b = np.array(a, dtype=float), np.array(b, dtype=float)
    ba = b - a
    def f(p):
        pa = p - a
        h = np.clip((pa @ ba) / (ba @ ba), 0.0, 1.0)
        return np.linalg.norm(pa - h[:, None] * ba, axis=1) - r
    return f


def rbox(half, rad):
    half = np.array(half, dtype=float)
    def f(p):
        q = np.abs(p) - half + rad
        return np.linalg.norm(np.maximum(q, 0), axis=1) + np.minimum(q.max(axis=1), 0) - rad
    return f


def smin(fs, k):
    def f(p):
        d = fs[0](p)
        for g in fs[1:]:
            e = g(p)
            h = np.clip(0.5 + 0.5 * (e - d) / k, 0.0, 1.0)
            d = e * (1 - h) + d * h - k * h * (1 - h)
        return d
    return f


def subtract(a, bs):
    def f(p):
        d = a(p)
        for b in bs:
            d = np.maximum(d, -b(p))
        return d
    return f


def union(fs):
    return lambda p: np.min(np.stack([g(p) for g in fs]), axis=0)


# ----------------------------------------------------------------------------- icons
def icon_kick():      # eight-pointed sun star
    pts = []
    for i in range(801):
        t = 2 * math.pi * i / 800
        c = math.cos(8 * t)
        r = 0.64 + 0.26 * math.copysign(abs(c) ** 0.55, c)
        pts.append((r * math.cos(t + math.pi / 8), r * math.sin(t + math.pi / 8)))
    return tube(curve_field([pts]), 0.15)


def icon_snare():     # four-loop knot ("X")
    pts = []
    for i in range(1201):
        t = 2 * math.pi * i / 1200
        r = 0.92 * math.cos(2 * t)
        pts.append((r * math.cos(t + math.pi / 4), r * math.sin(t + math.pi / 4)))
    return tube(curve_field([pts]), 0.13)


def icon_clap():      # five-pointed star outline
    pts = []
    for i in range(10):
        a = math.pi / 2 + i * math.pi / 5
        r = 0.9 if i % 2 == 0 else 0.42
        pts.append((r * math.cos(a), r * math.sin(a) - 0.04))
    return tube(curve_field([chaikin(pts, 3)]), 0.15)


def icon_hat():       # soft diamond
    pts = []
    for i in range(801):
        t = 2 * math.pi * i / 800
        c, s = math.cos(t), math.sin(t)
        k = (abs(c) ** 0.85 + abs(s) ** 0.85) ** (-1 / 0.85)
        pts.append((0.82 * k * c, 0.82 * k * s))
    return tube(curve_field([chaikin(pts, 1)]), 0.15)


def icon_cymbal():    # ring
    return tube(curve_field([circle(0, 0, 0.6, 400)]), 0.21)


def icon_tom():       # puffy crescent moon
    outer = circle(0, 0, 0.78, 300)
    inner = circle(0.36, 0.22, 0.64, 300)
    return puffy(shape_field([outer], [inner]), 0.17)


def icon_perc():      # heart outline
    pts = []
    for i in range(801):
        t = 2 * math.pi * i / 800
        x = 16 * math.sin(t) ** 3
        y = 13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t)
        pts.append((x * 0.046, y * 0.046 + 0.1))
    return tube(curve_field([pts]), 0.14)


def icon_other():     # teddy bear
    return smin([
        sphere((0, 0.42, 0), 0.32),
        sphere((-0.27, 0.68, -0.02), 0.13), sphere((0.27, 0.68, -0.02), 0.13),
        sphere((0, 0.33, 0.27), 0.12),
        ellipsoid((0, -0.17, 0), (0.33, 0.38, 0.28)),
        capsule((-0.25, 0.02, 0.05), (-0.52, -0.2, 0.12), 0.11),
        capsule((0.25, 0.02, 0.05), (0.52, -0.2, 0.12), 0.11),
        sphere((-0.21, -0.58, 0.08), 0.16), sphere((0.21, -0.58, 0.08), 0.16),
    ], 0.07)


def icon_play():
    tri = [(-0.42, 0.58), (-0.42, -0.58), (0.62, 0.0)]
    field = shape_field([tri]) - 0.08
    return puffy(field, 0.2)


def icon_stop():
    field = shape_field([[(-0.5, -0.5), (0.5, -0.5), (0.5, 0.5), (-0.5, 0.5)]]) - 0.1
    return puffy(field, 0.2)


def icon_add():
    return tube(curve_field([[(-0.62, 0), (0.62, 0)], [(0, -0.62), (0, 0.62)]]), 0.19)


def icon_folder():
    poly = [(-0.78, -0.5), (0.78, -0.5), (0.78, 0.36), (0.06, 0.36), (-0.12, 0.54), (-0.78, 0.54)]
    return puffy(shape_field([poly]) - 0.04, 0.15)


def icon_rescan():    # open ring with a ball on the end
    arc = circle(0, 0, 0.58, 300, math.radians(40), math.radians(330))
    end = arc[0]
    return smin([tube(curve_field([arc]), 0.17), sphere((end[0], end[1], 0), 0.25)], 0.06)


def icon_dice():
    box = rbox((0.5, 0.5, 0.5), 0.16)
    pips = []
    for x, y in [(-0.24, -0.24), (0.24, 0.24), (0, 0), (-0.24, 0.24), (0.24, -0.24)]:
        pips.append(sphere((x, y, 0.62), 0.15))
    for x, z in [(-0.2, -0.2), (0.2, 0.2)]:
        pips.append(sphere((x, 0.62, z), 0.15))
    for y, z in [(0.25, 0.25), (0, 0), (-0.25, -0.25)]:
        pips.append(sphere((0.62, y, z), 0.15))
    return subtract(box, pips)


def icon_midi():      # eighth note
    flag = chaikin([(0.08, 0.66), (0.3, 0.52), (0.48, 0.3), (0.42, 0.02)], 3, closed=False)
    return smin([
        ellipsoid((-0.22, -0.5, 0), (0.29, 0.21, 0.19)),
        capsule((0.03, -0.45, 0), (0.03, 0.66, 0), 0.1),
        tube(curve_field([flag]), 0.1),
    ], 0.06)


ICONS = {
    "kick": (icon_kick, 0.42, -0.30), "snare": (icon_snare, 0.42, 0.25), "clap": (icon_clap, 0.45, -0.25),
    "hat": (icon_hat, 0.42, 0.3), "cymbal": (icon_cymbal, 0.55, -0.35), "tom": (icon_tom, 0.3, -0.45),
    "perc": (icon_perc, 0.4, 0.3), "other": (icon_other, 0.25, -0.3), "play": (icon_play, 0.3, -0.35),
    "stop": (icon_stop, 0.35, -0.4), "add": (icon_add, 0.45, -0.4), "folder": (icon_folder, 0.4, -0.35),
    "rescan": (icon_rescan, 0.45, 0.3), "dice": (icon_dice, 0.55, -0.62), "midi": (icon_midi, 0.25, -0.3),
}


# ----------------------------------------------------------------------------- shading
def hexc(h):
    h = h.lstrip("#")
    return np.array([int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)])


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def environment(r):
    x, y, z = r[:, 0], r[:, 1], r[:, 2]
    sky = hexc("#ffffff") + (hexc("#e4e2ff") - hexc("#ffffff")) * smoothstep(0.0, 1.0, y)[:, None]
    ground = hexc("#e6e6ff") + (hexc("#6d6ee0") - hexc("#e6e6ff")) * smoothstep(-0.12, -0.9, y)[:, None]
    col = np.where((y > -0.12)[:, None], sky, ground)

    for centre, width, amount, tone in ((-0.14, 0.085, 0.97, "#07081f"), (0.5, 0.055, 0.75, "#1d1e52")):
        band = amount * np.exp(-((y - centre) / width) ** 2)
        col = col + (hexc(tone) - col) * band[:, None]

    ang = np.arctan2(x, z)
    hue = 0.5 + 0.5 * np.sin(ang * 2.4 + y * 4.0)
    irid = hexc("#ff9ade") + (hexc("#8af4ff") - hexc("#ff9ade")) * hue[:, None]
    amount = (0.42 * np.exp(-((y + 0.0) / 0.09) ** 2) + 0.35 * np.exp(-((y - 0.62) / 0.12) ** 2)
              + 0.28 * np.exp(-((y + 0.36) / 0.15) ** 2))
    col = col + (irid - col) * amount[:, None]

    box1 = smoothstep(0.17, 0.05, np.abs(x - 0.42)) * smoothstep(-0.05, 0.25, y)
    box2 = smoothstep(0.10, 0.02, np.abs(x + 0.58)) * smoothstep(0.15, 0.45, y)
    col = col + (0.85 * box1 + 0.55 * box2)[:, None]
    return col


def rotation(pitch, yaw):
    cx, sx, cy, sy = math.cos(pitch), math.sin(pitch), math.cos(yaw), math.sin(yaw)
    rx = np.array([[1, 0, 0], [0, cx, -sx], [0, sx, cx]])
    ry = np.array([[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]])
    return ry @ rx


def render(sdf_obj, pitch, yaw, size, ss=3):
    res = size * ss
    rot = rotation(pitch, yaw)
    sdf = lambda p: sdf_obj(p @ rot)          # world -> object space

    xs = np.linspace(-CAM, CAM, res)
    gx, gy = np.meshgrid(xs, -xs)
    origin = np.stack([gx.ravel(), gy.ravel(), np.full(gx.size, 3.0)], axis=1)
    t = np.zeros(gx.size)
    active = np.ones(gx.size, dtype=bool)
    hit = np.zeros(gx.size, dtype=bool)

    for _ in range(160):
        idx = np.nonzero(active)[0]
        if idx.size == 0:
            break
        p = origin[idx].copy()
        p[:, 2] -= t[idx]
        d = sdf(p)
        t[idx] += 0.8 * d
        done = d < 1.5e-3
        hit[idx[done]] = True
        active[idx[done | (t[idx] > 6.0)]] = False

    rgb = np.zeros((gx.size, 3))
    idx = np.nonzero(hit)[0]
    p = origin[idx].copy()
    p[:, 2] -= t[idx]

    e = 2e-3
    n = np.stack([sdf(p + [e, 0, 0]) - sdf(p - [e, 0, 0]),
                  sdf(p + [0, e, 0]) - sdf(p - [0, e, 0]),
                  sdf(p + [0, 0, e]) - sdf(p - [0, 0, e])], axis=1)
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-9)

    view = np.array([0.0, 0.0, -1.0])
    refl = view - 2 * (n @ view)[:, None] * n
    ndv = np.clip(-(n @ view), 0, 1)

    col = environment(refl)
    col *= (0.78 + 0.22 * (1 - ndv) ** 2)[:, None]

    k = 0.07                                   # cheap ambient occlusion
    ao = np.clip(sdf(p + n * k) / k, 0, 1)
    col *= (0.55 + 0.45 * ao)[:, None]

    light = np.array([-0.45, 0.6, 0.66]); light /= np.linalg.norm(light)
    spec = np.clip(refl @ light, 0, 1) ** 90
    col += (1.5 * spec)[:, None]
    col = np.clip(col, 0, 1)
    col = 0.55 * col + 0.45 * (col * col * (3 - 2 * col))     # a touch more contrast
    rgb[idx] = col

    alpha = hit.astype(float).reshape(res, res)
    rgba = np.dstack([rgb.reshape(res, res, 3) * alpha[..., None], alpha])   # premultiplied
    small = rgba.reshape(size, ss, size, ss, 4).mean(axis=(1, 3))
    a = small[..., 3:4]
    small[..., :3] = np.where(a > 0, small[..., :3] / np.maximum(a, 1e-6), 0)
    return Image.fromarray((np.clip(small, 0, 1) * 255).astype(np.uint8), "RGBA")


def main():
    out = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    size = int(sys.argv[2]) if len(sys.argv) > 2 else 128
    out.mkdir(parents=True, exist_ok=True)
    only = set(sys.argv[3:])
    for name, (make, pitch, yaw) in ICONS.items():
        if only and name not in only:
            continue
        img = render(make(), pitch, yaw, size)
        img.save(out / f"icon_{name}.png", optimize=True)
        print("rendered", name, flush=True)


if __name__ == "__main__":
    main()
