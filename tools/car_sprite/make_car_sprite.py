#!/usr/bin/env python3
"""Replace the cycling player's overworld sprite with a car.

Usage: tools/car_sprite/make_car_sprite.py SHEET.png [--scale N]

SHEET.png is a 4x4 sprite sheet (rows: facing down, left, right, up; four
animation frames each), e.g. "Gary Oak HGSS car" by Dionyus1 on DeviantArt
(512x512, drawn at 2x, so --scale 2). Each frame is shrunk to the 32x32,
16-colour format of the cycling sprite and written into
files/data/mmodel/mmodel/mmodel_00000071.NSBTX (MMODEL_CYCLEHERO) in place,
replacing all 24 textures and the palette. Requires Pillow.
"""

import argparse
import os
import struct

from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
NSBTX = os.path.join(ROOT, 'files', 'data', 'mmodel', 'mmodel', 'mmodel_00000071.NSBTX')
FRAME_SIZE = 32
GROUND_Y = 31  # bottom edge of the sprite, like the bike's wheels

# Sheet rows
DOWN, LEFT, RIGHT, UP = range(4)
# cyclehero.N texture -> (sheet row, sheet column)
TEXTURE_FRAMES = {
    1: (UP, 0), 2: (UP, 1), 3: (UP, 2), 4: (UP, 3),
    5: (DOWN, 0), 6: (DOWN, 1), 7: (DOWN, 2), 8: (DOWN, 3),
    9: (LEFT, 0), 10: (LEFT, 1), 11: (LEFT, 2), 12: (LEFT, 3),
    13: (RIGHT, 0), 14: (RIGHT, 1), 15: (RIGHT, 2), 16: (RIGHT, 3),
    17: (LEFT, 0), 18: (LEFT, 1), 19: (RIGHT, 0), 20: (RIGHT, 1),  # bunny hop
    21: (UP, 0), 22: (DOWN, 0), 23: (LEFT, 0), 24: (RIGHT, 0),     # standing
}


def read_dict(d, off):
    count = d[off + 1]
    p = off + 8 + 4 * (count + 1)
    unit = struct.unpack_from('<H', d, p)[0]
    p += 4
    entries = [d[p + i * unit:p + (i + 1) * unit] for i in range(count)]
    p += unit * count
    names = [d[p + i * 16:p + (i + 1) * 16].rstrip(b'\0').decode('ascii') for i in range(count)]
    return names, entries


def load_textures(d):
    assert d[:4] == b'BTX0'
    t = struct.unpack_from('<I', d, 0x10)[0]
    assert d[t:t + 4] == b'TEX0'
    texinfo = struct.unpack_from('<H', d, t + 0xE)[0]
    texdata = struct.unpack_from('<I', d, t + 0x14)[0]
    palinfo = struct.unpack_from('<I', d, t + 0x34)[0]
    paldata = struct.unpack_from('<I', d, t + 0x38)[0]
    textures = {}
    for name, entry in zip(*read_dict(d, t + texinfo)):
        param = struct.unpack_from('<I', entry, 0)[0]
        w, h, fmt = 8 << ((param >> 20) & 7), 8 << ((param >> 23) & 7), (param >> 26) & 7
        assert (w, h, fmt) == (FRAME_SIZE, FRAME_SIZE, 3), (name, w, h, fmt)  # 32x32, 16 colours
        textures[name] = t + texdata + ((param & 0xFFFF) << 3)
    pal_names, pal_entries = read_dict(d, t + palinfo)
    assert len(pal_names) == 1
    palette = t + paldata + (struct.unpack_from('<H', pal_entries[0], 0)[0] << 3)
    return textures, palette


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('sheet')
    parser.add_argument('--scale', type=int, default=2, help='upscale factor of the sheet (default 2)')
    args = parser.parse_args()

    sheet = Image.open(args.sheet).convert('RGBA')
    sheet = sheet.resize((sheet.width // args.scale, sheet.height // args.scale), Image.NEAREST)
    cell = sheet.width // 4
    frames = {}
    for row in range(4):
        for col in range(4):
            frame = sheet.crop((col * cell, row * cell, (col + 1) * cell, (row + 1) * cell))
            frame = frame.resize((FRAME_SIZE, FRAME_SIZE), Image.NEAREST)  # keeps the pixel art crisp
            frames[row, col] = frame

    # Rest every frame on the ground like the bike's wheels: the side views
    # are drawn high in their cell and would look like the car is floating.
    for key, frame in frames.items():
        bottom = frame.getbbox()[3]
        shifted = Image.new('RGBA', frame.size, (0, 0, 0, 0))
        shifted.paste(frame, (0, GROUND_Y - bottom))
        frames[key] = shifted

    # One shared palette: index 0 is transparent, then a few small but
    # important colours (outline, windshield, headlights, highlights) that
    # median cut would drop, then the 11 most representative colours.
    forced = [(0, 0, 0), (139, 196, 249), (255, 255, 167), (234, 240, 252)]
    opaque = [p[:3] for f in frames.values() for p in f.getdata() if p[3] >= 128]
    sample = Image.new('RGB', (len(opaque), 1))
    sample.putdata(opaque)
    colors = sample.quantize(colors=15 - len(forced), method=Image.Quantize.MEDIANCUT).getpalette()[:(15 - len(forced)) * 3]
    palette = [(0, 0, 0)] + forced + [tuple(colors[i:i + 3]) for i in range(0, len(colors), 3)]
    assert len(palette) == 16

    def nearest(rgb):
        return 1 + min(range(15), key=lambda i: sum((a - b) ** 2 for a, b in zip(rgb, palette[1 + i])))

    data = bytearray(open(NSBTX, 'rb').read())
    textures, palette_off = load_textures(data)
    for index, (row, col) in TEXTURE_FRAMES.items():
        frame = frames[row, col]
        pixels = [0 if p[3] < 128 else nearest(p[:3]) for p in frame.getdata()]
        packed = bytes(pixels[i] | (pixels[i + 1] << 4) for i in range(0, len(pixels), 2))
        off = textures['cyclehero.%d' % index]
        data[off:off + len(packed)] = packed
    for i, (r, g, b) in enumerate(palette):
        struct.pack_into('<H', data, palette_off + 2 * i, (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10))
    open(NSBTX, 'wb').write(bytes(data))
    print('Wrote', os.path.relpath(NSBTX, ROOT))


if __name__ == '__main__':
    main()
