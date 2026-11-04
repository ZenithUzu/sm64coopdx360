"""Checks the pack tool's PNG codec, scaling and texture rewriting on small hand-built data."""
import pathlib
import struct
import sys
import zlib

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import port_dynos_pack as tool

# RGBA and opaque images survive the round trip.
w, h = 5, 3
rgba = bytes((x * 40, y * 80, x * y * 20, 255 if x else 128)[c] for y in range(h) for x in range(w) for c in range(4))
assert tool.png_decode(tool.png_encode(w, h, rgba)) == (w, h, rgba)
opaque = bytes(255 if c == 3 else (x + y) * 30 for y in range(h) for x in range(w) for c in range(4))
assert tool.png_decode(tool.png_encode(w, h, opaque)) == (w, h, opaque)

# A palette with transparency.
palette = (tool.PNG_SIGNATURE + tool.png_chunk(b"IHDR", struct.pack(">IIBBBBB", 2, 1, 8, 3, 0, 0, 0))
           + tool.png_chunk(b"PLTE", bytes((1, 2, 3, 4, 5, 6))) + tool.png_chunk(b"tRNS", bytes((9,)))
           + tool.png_chunk(b"IDAT", zlib.compress(bytes((0, 0, 1)))) + tool.png_chunk(b"IEND", b""))
assert tool.png_decode(palette) == (2, 1, bytes((1, 2, 3, 9, 4, 5, 6, 255)))

# Transparent texels don't darken the average.
assert tool.rgba_halve(2, 2, bytes((100, 0, 0, 255, 0, 0, 0, 0, 100, 0, 0, 255, 0, 0, 0, 0))) == (1, 1, bytes((100, 0, 0, 128)))

# A texture node is found, and only it is rewritten with its new length.
noise = bytes((x * 7 + y * 13) & 255 for y in range(64) for x in range(64) for _ in range(4))
big = tool.png_encode(64, 64, noise)
name = b"mario_texture"
node = bytes((tool.DATA_TYPE_TEXTURE, len(name))) + name + struct.pack("<i", len(big)) + big
data = b"head" + node + b"tail"
nodes = tool.texture_nodes(data)
assert len(nodes) == 1
assert data[nodes[0][1]:nodes[0][2]] == big
small = tool.png_fit(big, 16)
assert tool.png_size(small) == (16, 16)
assert tool.png_fit(big, 64) is big

# A display list with one of each triangle command, and a pointer code carrying a name.
words = struct.pack("<II", tool.G_TRI1 << 24, 0) + struct.pack("<II", tool.G_TRI2 << 24, 0)
pointer = struct.pack("<II", 0xFD << 24, tool.POINTER_CODE) + bytes((3,)) + b"tex" + struct.pack("<I", 0)
display_list = bytes((tool.DATA_TYPE_DISPLAY_LIST, 4)) + b"list" + struct.pack("<I", 3) + words + pointer
assert tool.count_display_lists(display_list) == (3, 1)
