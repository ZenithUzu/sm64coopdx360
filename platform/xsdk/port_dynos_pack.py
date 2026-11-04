#!/usr/bin/env python3
"""Ports a DynOS model pack or mod to the Xbox 360: smaller textures, optional removal of heavy models, and a report.

Originals are never changed. A removed actor falls back to the game's own model.

    python port_dynos_pack.py <pack folder> <output folder> [--max-texture 256] [--max-triangles N] [--skip GLOB]
    python port_dynos_pack.py <pack folder> --report
"""
import argparse
import concurrent.futures
import fnmatch
import pathlib
import re
import shutil
import struct
import sys
import zlib

BIN_COMPRESS_MAGIC = 0x4E4942534F4E5944
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
DATA_TYPE_TEXTURE = 2
DATA_TYPE_DISPLAY_LIST = 4
# DynOS's pointer codes in a display list word (data/dynos.cpp.h), each followed by extra data.
FUNCTION_CODE, POINTER_CODE, LUA_VAR_CODE = 0x434E5546, 0x52544E50, 0x5641554C
G_TRI1, G_TRI2 = 0x05, 0x06
FATX_NAME_LIMIT = 42
NAME = re.compile(rb"[A-Za-z0-9_.\-]+")


def png_size(png):
    if png[12:16] != b"IHDR":
        return None
    return struct.unpack_from(">II", png, 16)


def png_decode(png):
    """Gives (width, height, RGBA bytes), or None for a form this doesn't read: such a texture stays as it was."""
    pos, palette, alpha, idat = 8, None, None, []
    width = height = depth = color = 0
    while pos + 8 <= len(png):
        length, kind = struct.unpack_from(">I4s", png, pos)
        body = png[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, color, _, _, interlace = struct.unpack(">IIBBBBB", body)
            if interlace:
                return None
        elif kind == b"PLTE":
            palette = body
        elif kind == b"tRNS":
            alpha = body
        elif kind == b"IDAT":
            idat.append(body)
        elif kind == b"IEND":
            break
        pos += 12 + length
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(color)
    if not channels or depth not in (1, 2, 4, 8, 16):
        return None
    raw = zlib.decompress(b"".join(idat))
    stride = (width * channels * depth + 7) // 8
    bpp = max(1, channels * depth // 8)
    previous = bytearray(stride)
    out = bytearray(width * height * 4)
    src = 0
    for y in range(height):
        kind = raw[src]
        row = bytearray(raw[src + 1:src + 1 + stride])
        src += 1 + stride
        if kind == 1:
            for i in range(bpp, stride):
                row[i] = (row[i] + row[i - bpp]) & 255
        elif kind == 2:
            row = bytearray((a + b) & 255 for a, b in zip(row, previous))
        elif kind == 3:
            for i in range(stride):
                row[i] = (row[i] + ((row[i - bpp] if i >= bpp else 0) + previous[i]) // 2) & 255
        elif kind == 4:
            for i in range(stride):
                a = row[i - bpp] if i >= bpp else 0
                b = previous[i]
                c = previous[i - bpp] if i >= bpp else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                row[i] = (row[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        previous = row
        o = y * width * 4
        if depth == 8 and color == 6:
            out[o:o + width * 4] = row
            continue
        if depth == 16:
            samples = row[::2]
        elif depth == 8:
            samples = row
        else:
            mask = (1 << depth) - 1
            samples = [(row[i * depth // 8] >> (8 - depth - (i * depth % 8))) & mask for i in range(width)]
        for x in range(width):
            if color == 3:
                index = samples[x]
                r, g, b = palette[index * 3:index * 3 + 3]
                a = alpha[index] if alpha and index < len(alpha) else 255
            elif color == 0:
                r = g = b = samples[x] * 255 // ((1 << min(depth, 8)) - 1)
                a = 255
            elif color == 2:
                r, g, b = samples[x * 3:x * 3 + 3]
                a = 255
            elif color == 4:
                r = g = b = samples[x * 2]
                a = samples[x * 2 + 1]
            else:
                r, g, b, a = samples[x * 4:x * 4 + 4]
            out[o + x * 4:o + x * 4 + 4] = bytes((r, g, b, a))
    return width, height, bytes(out)


def rgba_halve(width, height, rgba):
    """Averages each 2x2 block, weighting color by alpha so cutout edges don't darken."""
    new_w, new_h = max(1, width // 2), max(1, height // 2)
    out = bytearray(new_w * new_h * 4)
    step_x = 2 if width > 1 else 1
    step_y = 2 if height > 1 else 1
    for y in range(new_h):
        o = y * new_w * 4
        top = y * step_y * width * 4
        bottom = (y * step_y + step_y - 1) * width * 4
        for x in range(new_w):
            left = x * step_x * 4
            right = (x * step_x + step_x - 1) * 4
            r = g = b = a = 0
            for p in (top + left, top + right, bottom + left, bottom + right):
                pa = rgba[p + 3]
                r += rgba[p] * pa
                g += rgba[p + 1] * pa
                b += rgba[p + 2] * pa
                a += pa
            if a:
                out[o] = (r + a // 2) // a
                out[o + 1] = (g + a // 2) // a
                out[o + 2] = (b + a // 2) // a
                out[o + 3] = (a + 2) // 4
            o += 4
    return new_w, new_h, bytes(out)


def png_chunk(kind, body):
    return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))


def png_encode(width, height, rgba):
    opaque = all(a == 255 for a in rgba[3::4])
    channels = 3 if opaque else 4
    pixels = rgba
    if opaque:
        pixels = bytearray(width * height * 3)
        pixels[0::3], pixels[1::3], pixels[2::3] = rgba[0::4], rgba[1::4], rgba[2::4]
    stride = width * channels
    rows = bytearray()
    previous = bytes(stride)
    for y in range(height):
        row = bytes(pixels[y * stride:(y + 1) * stride])
        rows.append(2)  # the Up filter compresses better than none, at little cost here
        rows += bytes((a - b) & 255 for a, b in zip(row, previous))
        previous = row
    header = struct.pack(">IIBBBBB", width, height, 8, 2 if opaque else 6, 0, 0, 0)
    return PNG_SIGNATURE + png_chunk(b"IHDR", header) + png_chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + png_chunk(b"IEND", b"")


def png_fit(png, max_side):
    """Gives the PNG with both sides within max_side, or itself when it already fits or can't be read."""
    size = png_size(png)
    if not size or max(size) <= max_side:
        return png
    decoded = png_decode(png)
    if not decoded:
        return png
    width, height, rgba = decoded
    while max(width, height) > max_side:
        width, height, rgba = rgba_halve(width, height, rgba)
    return png_encode(width, height, rgba)


def read_container(path):
    data = path.read_bytes()
    if len(data) >= 16 and struct.unpack_from("<Q", data)[0] == BIN_COMPRESS_MAGIC:
        return zlib.decompress(data[16:])
    return data


def texture_nodes(data):
    """Finds each texture node: its type byte, name, 32-bit length and PNG. Anything else matching stays untouched."""
    found = []
    for match in re.finditer(re.escape(PNG_SIGNATURE), data):
        start = match.start()
        if start < 7:
            continue
        length = struct.unpack_from("<i", data, start - 4)[0]
        if length < 16 or start + length > len(data) or data[start + length - 8:start + length - 4] != b"IEND":
            continue
        for name_length in range(1, 256):
            at = start - 4 - name_length - 1
            if at < 1:
                break
            if data[at] == name_length and data[at - 1] == DATA_TYPE_TEXTURE and NAME.fullmatch(data[at + 1:start - 4]):
                found.append((start - 4, start, start + length))
                break
    return found


def count_display_lists(data):
    """Counts triangles the file's display lists draw, each list once, and returns (triangles, lists)."""
    triangles = lists = 0
    for match in re.finditer(bytes([DATA_TYPE_DISPLAY_LIST]), data):
        at = match.start() + 1
        if at >= len(data):
            continue
        name_length = data[at]
        name = data[at + 1:at + 1 + name_length]
        if name_length < 2 or not NAME.fullmatch(name) or at + 5 + name_length > len(data):
            continue
        pos = at + 1 + name_length
        count = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        if not 0 < count < 2000000:
            continue
        here, valid = 0, True
        for _ in range(count):
            if pos + 8 > len(data):
                valid = False
                break
            w0, w1 = struct.unpack_from("<II", data, pos)
            pos += 8
            op = w0 >> 24
            if op == G_TRI1:
                here += 1
            elif op == G_TRI2:
                here += 2
            if w1 == FUNCTION_CODE:
                pos += 4
            elif w1 == POINTER_CODE:
                pos += 1 + data[pos] + 4 if pos < len(data) else 0
            elif w1 == LUA_VAR_CODE:
                pos += 1 + data[pos] if pos < len(data) else 0
        if valid:
            triangles += here
            lists += 1
    return triangles, lists


def rgba_bytes(pngs):
    total = 0
    for png in pngs:
        size = png_size(png)
        if size:
            total += size[0] * size[1] * 4
    return total


def process(job):
    path, relative, output, max_side = job
    if path.suffix == ".tex" and path.read_bytes()[:8] == PNG_SIGNATURE:
        data = path.read_bytes()
        pngs = [data]
        result = png_fit(data, max_side) if output else data
        new = [result]
        triangles = 0
    else:
        data = read_container(path)
        nodes = texture_nodes(data)
        pngs = [data[start:end] for _, start, end in nodes]
        triangles = count_display_lists(data)[0]
        new = []
        pieces = []
        last = 0
        for (length_at, start, end), png in zip(nodes, pngs):
            fitted = png_fit(png, max_side) if output else png
            new.append(fitted)
            pieces += [data[last:length_at], struct.pack("<i", len(fitted)), fitted]
            last = end
        pieces.append(data[last:])
        result = b"".join(pieces)
    if output:
        target = output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(result)
    return {
        "name": str(relative), "disk": path.stat().st_size, "ram": len(data), "textures": len(pngs),
        "rgba": rgba_bytes(pngs), "new_ram": len(result), "new_rgba": rgba_bytes(new), "triangles": triangles,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("pack", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path, nargs="?")
    parser.add_argument("--report", action="store_true", help="only measure the pack")
    parser.add_argument("--max-texture", type=int, default=256, help="longest texture side in pixels (default 256)")
    parser.add_argument("--max-triangles", type=int, default=0, help="leave out actors above this many triangles")
    parser.add_argument("--skip", action="append", default=[], help="leave out actors matching this pattern")
    args = parser.parse_args()
    if not args.pack.is_dir():
        parser.error(f"{args.pack} is not a folder")
    if not args.report and not args.output:
        parser.error("give an output folder, or --report")
    output = None if args.report else args.output
    if output and (output.resolve() == args.pack.resolve() or args.pack.resolve() in output.resolve().parents):
        parser.error("the output folder must be outside the pack")

    jobs, others = [], []
    for path in sorted(args.pack.rglob("*")):
        if not path.is_file():
            continue
        relative = path.relative_to(args.pack)
        if path.suffix in (".bin", ".lvl", ".tex"):
            jobs.append((path, relative, output, args.max_texture))
        else:
            others.append((path, relative))

    with concurrent.futures.ProcessPoolExecutor() as pool:
        rows = list(pool.map(process, jobs))

    removed = []
    if output:
        for row in rows:
            stem = pathlib.Path(row["name"]).stem
            heavy = args.max_triangles and row["triangles"] > args.max_triangles
            if heavy or any(fnmatch.fnmatch(stem, pattern) for pattern in args.skip):
                (output / row["name"]).unlink()
                removed.append(row)
        for path, relative in others:
            target = output / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)

    kept = [row for row in rows if row not in removed]
    megabytes = 1024 * 1024
    print(f"archivos {len(rows)}, otros {len(others)}, quitados {len(removed)}")
    print(f"antes:   RAM {sum(r['ram'] for r in rows) / megabytes:.1f} MB, texturas decodificadas {sum(r['rgba'] for r in rows) / megabytes:.1f} MB, triangulos {sum(r['triangles'] for r in rows)}")
    if output:
        print(f"despues: RAM {sum(r['new_ram'] for r in kept) / megabytes:.1f} MB, texturas decodificadas {sum(r['new_rgba'] for r in kept) / megabytes:.1f} MB, triangulos {sum(r['triangles'] for r in kept)}")
    print("modelos con mas triangulos:")
    for row in sorted(rows, key=lambda r: -r["triangles"])[:15]:
        print(f"  {row['triangles']:7}  {row['ram'] / megabytes:6.1f} MB  {row['name']}")
    long_names = [str(rel) for _, rel in others if len(rel.name) > FATX_NAME_LIMIT] + [r["name"] for r in rows if len(pathlib.Path(r["name"]).name) > FATX_NAME_LIMIT]
    if long_names:
        print(f"nombres de mas de {FATX_NAME_LIMIT} caracteres, que el disco de la Xbox no admite:")
        for name in long_names:
            print(f"  {name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
