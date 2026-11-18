#!/usr/bin/env python3
"""Generate 4-bit indexed RGB565 pixel-art C headers for Blues Box (M-VAVE FM-1)."""
from collections import deque
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "firmware" / "gen" / "pixelart_assets.h"

BRAIN = Path("/Users/brianprovost/.gemini/antigravity/brain/b9f6b7ef-5901-4d94-b1af-6091ac0f0a16")
SPLASH_SRC = BRAIN / "srv_splash_raw_1795023664115.jpg"
STAGE_SRC = BRAIN / "srv_stage_raw_1795023666975.jpg"
DRUM_SRC = BRAIN / "drummer_chunky_raw_1795028709248.jpg"
KEYS_SRC = BRAIN / "keys_girl_raw_final_1795028704953.jpg"


def rgb888_to_565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def remove_white_bg(im: Image.Image) -> Image.Image:
    px = im.load()
    w, h = im.size
    vis = [[False] * w for _ in range(h)]
    q = deque([(x, 0) for x in range(w)] + [(x, h - 1) for x in range(w)] +
              [(0, y) for y in range(h)] + [(w - 1, y) for y in range(h)])
    while q:
        x, y = q.popleft()
        if x < 0 or x >= w or y < 0 or y >= h or vis[y][x]:
            continue
        r, g, b = px[x, y]
        if r > 200 and g > 200 and b > 200:
            vis[y][x] = True
            px[x, y] = (0, 0, 0)
            q.extend([(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)])
    return im


def crop_nonblack(im: Image.Image, thr: int = 26) -> Image.Image:
    px = im.load()
    w, h = im.size
    x0, y0, x1, y1 = w, h, 0, 0
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            if r > thr or g > thr or b > thr:
                x0 = min(x0, x); y0 = min(y0, y)
                x1 = max(x1, x); y1 = max(y1, y)
    return im.crop((x0, y0, x1 + 1, y1 + 1))


def quantize_sprite(im: Image.Image, w: int, h: int, colors: int = 16) -> tuple[list[int], list[int]]:
    sm = im.resize((w, h), Image.NEAREST)
    q = sm.quantize(colors=colors, method=Image.MEDIANCUT, dither=0)
    raw_pal = q.getpalette()[: colors * 3]
    pal565 = []
    for i in range(colors):
        r, g, b = raw_pal[i * 3 : i * 3 + 3]
        if r < 24 and g < 24 and b < 24:
            r, g, b = 0, 0, 0
        pal565.append(rgb888_to_565(r, g, b))
    pixels = list(q.getdata())
    return pal565, pixels


def find_black_idx(pal565: list[int]) -> int:
    best_i, best_luma = 0, 999999
    for i, c in enumerate(pal565):
        r = (c >> 11) & 0x1F
        g = (c >> 5) & 0x3F
        b = c & 0x1F
        luma = r * 2 + g + b * 2
        if luma < best_luma:
            best_luma = luma
            best_i = i
    return best_i


def make_bounce_frame(px_up: list[int], w: int, h: int, y_start: int, y_end: int, x_start: int, x_end: int, bg_idx: int) -> list[int]:
    px_down = list(px_up)
    for y in range(y_end, y_start, -1):
        for x in range(x_start, x_end):
            px_down[y * w + x] = px_up[(y - 1) * w + x]
    for x in range(x_start, x_end):
        px_down[y_start * w + x] = bg_idx
    return px_down


def make_drum_hit_frame(px_up: list[int], w: int, h: int, bg_idx: int) -> list[int]:
    """Swing both drummer arms + sticks (x=1..9 and x=18..25) down 3 sprite rows (6 screen px)."""
    px_down = list(px_up)
    shift = 3
    for x0, x1 in [(1, 10), (18, 26)]:
        for y in range(15, 0, -1):
            src_y = y - shift
            for x in range(x0, x1):
                px_down[y * w + x] = px_up[src_y * w + x] if src_y >= 1 else bg_idx
    return px_down


def pack_4bit(pixels: list[int]) -> list[int]:
    out = []
    for i in range(0, len(pixels), 2):
        hi = pixels[i] & 0x0F
        lo = pixels[i + 1] & 0x0F if i + 1 < len(pixels) else 0
        out.append((hi << 4) | lo)
    return out


def format_array(name: str, ctype: str, data: list[int], fmt: str = "0x{:02X}") -> str:
    lines = [f"static const {ctype} {name}[{len(data)}] = {{"]
    for i in range(0, len(data), 16):
        chunk = ", ".join(fmt.format(v) for v in data[i : i + 16])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def main() -> None:
    # 1. Boot Splash: 120x110 (upscales 2x to 240x220)
    splash_im = Image.open(SPLASH_SRC).convert("RGB")
    splash_pal, splash_px = quantize_sprite(splash_im, 120, 110, 16)

    # 2. Guitarist/Bassist: 24x30 (upscales 2x to 48x60)
    stage_full = Image.open(STAGE_SRC).convert("RGB").resize((120, 120), Image.NEAREST)
    guy_crop = crop_nonblack(stage_full.crop((24, 27, 75, 93)))
    guy_pal, guy_up = quantize_sprite(guy_crop, 24, 30, 16)
    guy_down = make_bounce_frame(guy_up, 24, 30, 12, 23, 4, 22, find_black_idx(guy_pal))

    # 3. Drummer: 27x30 (upscales 2x to 54x60)
    drum_crop = crop_nonblack(Image.open(DRUM_SRC).convert("RGB"))
    drum_pal, drum_up = quantize_sprite(drum_crop, 27, 30, 16)
    drum_down = make_drum_hit_frame(drum_up, 27, 30, find_black_idx(drum_pal))

    # 4. Cool Girl Keyboardist: 27x30 (upscales 2x to 54x60)
    keys_crop = crop_nonblack(remove_white_bg(Image.open(KEYS_SRC).convert("RGB")))
    keys_pal, keys_up = quantize_sprite(keys_crop, 27, 30, 16)
    keys_down = make_bounce_frame(keys_up, 27, 30, 6, 18, 2, 24, find_black_idx(keys_pal))

    OUT.parent.mkdir(parents=True, exist_ok=True)
    parts = [
        "#ifndef PIXELART_ASSETS_H",
        "#define PIXELART_ASSETS_H",
        "#include <stdint.h>",
        "",
        "#define SPLASH_ART_W 120",
        "#define SPLASH_ART_H 110",
        format_array("SPLASH_ART_PAL", "uint16_t", splash_pal, "0x{:04X}u"),
        format_array("SPLASH_ART_DATA", "uint8_t", pack_4bit(splash_px)),
        "",
        "#define SRV_SPRITE_W 24",
        "#define SRV_SPRITE_H 30",
        format_array("SRV_SPRITE_PAL", "uint16_t", guy_pal, "0x{:04X}u"),
        format_array("SRV_SPRITE_UP", "uint8_t", pack_4bit(guy_up)),
        format_array("SRV_SPRITE_DOWN", "uint8_t", pack_4bit(guy_down)),
        "",
        "#define DRUM_SPRITE_W 27",
        "#define DRUM_SPRITE_H 30",
        format_array("DRUM_SPRITE_PAL", "uint16_t", drum_pal, "0x{:04X}u"),
        format_array("DRUM_SPRITE_UP", "uint8_t", pack_4bit(drum_up)),
        format_array("DRUM_SPRITE_DOWN", "uint8_t", pack_4bit(drum_down)),
        "",
        "#define KEYS_SPRITE_W 27",
        "#define KEYS_SPRITE_H 30",
        format_array("KEYS_SPRITE_PAL", "uint16_t", keys_pal, "0x{:04X}u"),
        format_array("KEYS_SPRITE_UP", "uint8_t", pack_4bit(keys_up)),
        format_array("KEYS_SPRITE_DOWN", "uint8_t", pack_4bit(keys_down)),
        "",
        "#endif // PIXELART_ASSETS_H",
        "",
    ]
    OUT.write_text("\n".join(parts))
    print(f"Wrote {OUT} ({OUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
