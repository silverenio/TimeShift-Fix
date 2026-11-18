"""Generates the guide/installer logo (original artwork). Output: logo.png (512x512) and logo.ico."""
import math, random, os
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops

S = 512
OUT = os.path.dirname(os.path.abspath(__file__))
F = lambda name, size: ImageFont.truetype(f"C:/Windows/Fonts/{name}", size)

def radial(size, inner, outer, center=None, radius=None):
    w, h = size; cx, cy = center or (w / 2, h / 2); r = radius or max(w, h) * 0.75
    img = Image.new("RGB", size); px = img.load()
    for y in range(h):
        for x in range(w):
            t = min(1.0, math.hypot(x - cx, y - cy) / r)
            px[x, y] = tuple(int(inner[i] + (outer[i] - inner[i]) * t) for i in range(3))
    return img

def glow(layer, radius, strength=2):
    g = layer.filter(ImageFilter.GaussianBlur(radius))
    out = g
    for _ in range(strength - 1):
        out = ImageChops.add(out, g)
    return out

random.seed(7)
# --- background: deep blue-steel radial + stormy noise
bg = radial((S, S), (40, 62, 88), (5, 8, 14), center=(S * 0.5, S * 0.42), radius=S * 0.8)
noise = Image.effect_noise((S // 4, S // 4), 60).resize((S, S), Image.BICUBIC).filter(ImageFilter.GaussianBlur(6))
bg = Image.blend(bg, Image.merge("RGB", (noise,) * 3).point(lambda v: v * 0.35), 0.25)
d = ImageDraw.Draw(bg)

# --- clock rings / time ripples behind the visor
rings = Image.new("RGB", (S, S)); rd = ImageDraw.Draw(rings)
cx, cy = S // 2, int(S * 0.43)
for r, w, c in [(205, 3, (60, 150, 220)), (172, 2, (50, 120, 190)), (140, 1, (40, 100, 160))]:
    rd.ellipse([cx - r, cy - r, cx + r, cy + r], outline=c, width=w)
for i in range(60):
    a = math.radians(i * 6); L = 16 if i % 5 == 0 else 7; r = 205
    rd.line([(cx + (r - L) * math.sin(a), cy - (r - L) * math.cos(a)), (cx + r * math.sin(a), cy - r * math.cos(a))],
            fill=(90, 190, 255) if i % 5 == 0 else (50, 120, 180), width=3 if i % 5 == 0 else 1)
# clock hands frozen at "time stop"
rd.line([(cx, cy), (cx + 120 * math.sin(math.radians(60)), cy - 120 * math.cos(math.radians(60)))], fill=(80, 170, 240), width=4)
rd.line([(cx, cy), (cx + 85 * math.sin(math.radians(300)), cy - 85 * math.cos(math.radians(300)))], fill=(80, 170, 240), width=6)
bg = ImageChops.add(bg, glow(rings, 6, 2)); bg = ImageChops.add(bg, rings.point(lambda v: int(v * 0.8)))

# --- glowing visor (stylised suit helmet "V" chevron)
# Y-shaped emblem: two swept wings + a central blade, drawn as glowing outlines with a dark core
vis = Image.new("RGB", (S, S)); vd = ImageDraw.Draw(vis)
core = Image.new("L", (S, S)); cd = ImageDraw.Draw(core)
k = 115
wingL = [(-1.05, -0.62), (-0.12, -0.10), (-0.10, 0.12), (-0.62, -0.05)]
wingR = [(-x, y) for x, y in wingL]
blade = [(0, -0.02), (0.13, 0.12), (0.07, 0.95), (0, 1.08), (-0.07, 0.95), (-0.13, 0.12)]
shapes = [wingL, wingR, blade]
P = lambda pts: [(cx + x * k, cy - 20 + y * k) for x, y in pts]
for sh in shapes:
    cd.polygon(P(sh), fill=255)
    vd.line(P(sh) + [P(sh)[0]], fill=(255, 140, 10), width=12, joint="curve")
for sh in shapes:
    vd.line(P(sh) + [P(sh)[0]], fill=(255, 235, 150), width=4, joint="curve")
bg = ImageChops.add(bg, glow(vis, 18, 2))
bg.paste((30, 18, 6), (0, 0), core)            # dark metal core
# subtle metallic sheen inside the core
sheen = Image.new("RGB", (S, S)); sd = ImageDraw.Draw(sheen)
for yy in range(S):
    v = max(0, 70 - abs(yy - (cy - 30)) // 2); sd.line([(0, yy), (S, yy)], fill=(v, int(v * 0.7), int(v * 0.3)))
bg.paste(ImageChops.add(bg, sheen), (0, 0), core)
bg = ImageChops.add(bg, vis)

# --- title "TIMESHIFT" (metallic) at top
def metal_text(img, text, font, y, top=(235, 242, 250), bot=(120, 135, 155), outline=(10, 15, 25), ow=4):
    dd = ImageDraw.Draw(img); w = dd.textlength(text, font=font); x = (S - w) / 2
    mask = Image.new("L", (S, S)); ImageDraw.Draw(mask).text((x, y), text, font=font, fill=255)
    bbox = mask.getbbox(); grad = Image.new("RGB", (S, S)); gd = ImageDraw.Draw(grad)
    for yy in range(bbox[1], bbox[3] + 1):
        t = (yy - bbox[1]) / max(1, bbox[3] - bbox[1]); t = t * t
        gd.line([(0, yy), (S, yy)], fill=tuple(int(top[i] + (bot[i] - top[i]) * t) for i in range(3)))
    om = mask.filter(ImageFilter.MaxFilter(ow * 2 + 1))
    img.paste(outline, (0, 0), om); img.paste(grad, (0, 0), mask)
metal_text(bg, "TIMESHIFT", F("bahnschrift.ttf", 70), 22)

# --- big "FIX" banner at bottom (Impact, yellow-orange, heavy outline + glow)
fx = F("impact.ttf", 150); dd = ImageDraw.Draw(bg); w = dd.textlength("FIX", font=fx); x = (S - w) / 2; y = S - 190
mask = Image.new("L", (S, S)); ImageDraw.Draw(mask).text((x, y), "FIX", font=fx, fill=255)
gl = Image.new("RGB", (S, S)); gl.paste((255, 120, 0), (0, 0), mask.filter(ImageFilter.MaxFilter(9)))
bg = ImageChops.add(bg, glow(gl, 14, 2))
bg.paste((0, 0, 0), (0, 0), mask.filter(ImageFilter.MaxFilter(15)))
grad = Image.new("RGB", (S, S)); gd = ImageDraw.Draw(grad); bb = mask.getbbox()
for yy in range(bb[1], bb[3] + 1):
    t = (yy - bb[1]) / max(1, bb[3] - bb[1])
    gd.line([(0, yy), (S, yy)], fill=(255, int(240 - 110 * t), int(120 - 110 * t)))
bg.paste(grad, (0, 0), mask)

# --- small tag line
tag = "CPU + DRIVER  •  WIN 10/11"
metal_text(bg, tag, F("bahnschrift.ttf", 26), S - 44, top=(170, 215, 255), bot=(110, 160, 220), ow=2)

# vignette + thin frame
vig = radial((S, S), (255, 255, 255), (110, 110, 110), radius=S * 0.78)
bg = ImageChops.multiply(bg, vig)
ImageDraw.Draw(bg).rectangle([0, 0, S - 1, S - 1], outline=(70, 140, 200), width=3)

bg.save(os.path.join(OUT, "logo.png"))
bg.save(os.path.join(OUT, "logo.ico"), sizes=[(256, 256), (64, 64), (48, 48), (32, 32), (16, 16)])
print("ok")
