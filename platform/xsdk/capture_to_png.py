#!/usr/bin/env python3
"""Turns capture.raw, a frame the console saved as Xenos tiled texels, into a PNG.

    python capture_to_png.py capture.raw frame.png [--order rgba|bgra|argb|abgr]
"""
import argparse
import pathlib
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from port_dynos_pack import png_encode  # noqa: E402


# Xenos's 2D tiled layout for 32-bit texels (xenia:src/xenia/gpu/texture_address.cc, Tiled2D), as a byte offset.
def tiled_offset(x, y, pitch):
    bpp_log2 = 2
    macro = ((y >> 5) * (pitch >> 5)) << (bpp_log2 + 7)
    micro = ((y & 6) << 2) << bpp_log2
    row = macro + ((micro & ~0xF) << 1) + (micro & 0xF) + ((y & 8) << (3 + bpp_log2)) + ((y & 1) << 4)
    macro = (x >> 5) << (bpp_log2 + 7)
    micro = (x & 7) << bpp_log2
    offset = row + macro + ((micro & ~0xF) << 1) + (micro & 0xF)
    return ((offset & ~0x1FF) << 3) + ((offset & 0x1C0) << 2) + (offset & 0x3F) + ((y & 16) << 7) + \
        (((((y & 8) >> 2) + (x >> 3)) & 3) << 6)


def convert(data, order):
    width, height, pitch = struct.unpack_from('>III', data)
    texels = data[12:]
    picks = {'rgba': (0, 1, 2), 'bgra': (2, 1, 0), 'argb': (1, 2, 3), 'abgr': (3, 2, 1)}[order]
    out = bytearray()
    for y in range(height):
        for x in range(width):
            o = tiled_offset(x, y, pitch)
            px = texels[o:o + 4]
            out += bytes((px[picks[0]], px[picks[1]], px[picks[2]], 255))
    return width, height, bytes(out)


def main():
    a = argparse.ArgumentParser()
    a.add_argument('raw'); a.add_argument('png')
    a.add_argument('--order', default='argb', choices=('rgba', 'bgra', 'argb', 'abgr'))
    args = a.parse_args()
    width, height, rgba = convert(pathlib.Path(args.raw).read_bytes(), args.order)
    pathlib.Path(args.png).write_bytes(png_encode(width, height, rgba))
    print(f'{width}x{height} -> {args.png}')


if __name__ == '__main__':
    main()
