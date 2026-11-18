"""Guide/installer logo from the TimeShift cover art (soldier) + FIX banner. Usage: python make_logo_cover.py <cover.jpg>"""
import sys, os
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops, ImageEnhance

S = 512
OUT = os.path.dirname(os.path.abspath(__file__))
src = Image.open(sys.argv[1]).convert("RGB")
box = tuple(int(v) for v in sys.argv[2].split(",")) if len(sys.argv) > 2 else (88, 52, 256, 220)
im = src.crop(box).resize((S, S), Image.LANCZOS)
im = im.filter(ImageFilter.UnsharpMask(radius=3, percent=120, threshold=2))
im = ImageEnhance.Contrast(im).enhance(1.12)

def F(name, size): return ImageFont.truetype(f"C:/Windows/Fonts/{name}", size)

def outlined(img, text, font, xy, fill_top, fill_bot, outline=(0, 0, 0), ow=8, glow_col=None):
    mask = Image.new("L", (S, S)); ImageDraw.Draw(mask).text(xy, text, font=font, fill=255)
    if glow_col:
        g = Image.new("RGB", (S, S)); g.paste(glow_col, (0, 0), mask.filter(ImageFilter.MaxFilter(9)))
        g = g.filter(ImageFilter.GaussianBlur(14)); img = ImageChops.add(img, ImageChops.add(g, g))
    img.paste(outline, (0, 0), mask.filter(ImageFilter.MaxFilter(ow * 2 + 1)))
    grad = Image.new("RGB", (S, S)); gd = ImageDraw.Draw(grad); bb = mask.getbbox()
    for y in range(bb[1], bb[3] + 1):
        t = (y - bb[1]) / max(1, bb[3] - bb[1])
        gd.line([(0, y), (S, y)], fill=tuple(int(fill_top[i] + (fill_bot[i] - fill_top[i]) * t) for i in range(3)))
    img.paste(grad, (0, 0), mask)
    return img

# darken bottom for text readability
shade = Image.new("L", (S, S)); sd = ImageDraw.Draw(shade)
for y in range(S):
    sd.line([(0, y), (S, y)], fill=int(max(0, (y - S * 0.55) / (S * 0.45)) * 170))
im.paste((0, 0, 0), (0, 0), shade)

fx = F("impact.ttf", 170)
w = ImageDraw.Draw(im).textlength("FIX", font=fx)
im = outlined(im, "FIX", fx, ((S - w) / 2, S - 215), (255, 240, 120), (255, 130, 10), ow=9, glow_col=(255, 120, 0))
tag = "CPU + DRIVER  •  WIN 10/11"; tf = F("bahnschrift.ttf", 30)
w = ImageDraw.Draw(im).textlength(tag, font=tf)
im = outlined(im, tag, tf, ((S - w) / 2, S - 46), (235, 245, 255), (170, 205, 240), ow=3)
ImageDraw.Draw(im).rectangle([0, 0, S - 1, S - 1], outline=(255, 170, 40), width=4)

im.save(os.path.join(OUT, "logo.png"))
im.save(os.path.join(OUT, "logo.ico"), sizes=[(256, 256), (64, 64), (48, 48), (32, 32), (16, 16)])
print("ok")
