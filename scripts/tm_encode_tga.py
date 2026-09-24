#!/usr/bin/env python3
import argparse
import struct
from pathlib import Path

from embed_common import add_embed_argument, embed_view_lines, write_bin, EMBED_PRELUDE, EMBED_EPILOGUE

WIDTH = 16
HEIGHT = 16
HEADER_NAME = "tm_pixels.h"
BIN_NAME = "tm_pixels.bin"
VALUES_PER_LINE = 16


def read_tga(path: Path) -> list[int]:
    data = path.read_bytes()
    if len(data) < 18:
        raise ValueError(f"{path.name}: too small to be a TGA")

    id_len, cmap_type, img_type = data[0], data[1], data[2]
    cmap_len = struct.unpack_from('<H', data, 5)[0]
    cmap_bits = data[7]
    width, height = struct.unpack_from('<HH', data, 12)
    bpp, descriptor = data[16], data[17]

    if img_type not in (2, 10):
        raise ValueError(f"{path.name}: unsupported TGA type {img_type} "
                         "(need uncompressed or RLE truecolor)")
    if bpp not in (24, 32):
        raise ValueError(f"{path.name}: unsupported depth {bpp}bpp (need 24 or 32)")
    if (width, height) != (WIDTH, HEIGHT):
        raise ValueError(f"{path.name}: expected {WIDTH}x{HEIGHT}, got {width}x{height}")

    bytes_pp = bpp // 8
    offset = 18 + id_len
    if cmap_type:
        offset += cmap_len * ((cmap_bits + 7) // 8)

    count = width * height
    pixels: list[int] = []

    def pack(chunk: bytes) -> int:
        b, g, r = chunk[0], chunk[1], chunk[2]
        a = chunk[3] if bytes_pp == 4 else 0xFF
        return (a << 24) | (r << 16) | (g << 8) | b

    if img_type == 2:
        need = count * bytes_pp
        if len(data) < offset + need:
            raise ValueError(f"{path.name}: truncated pixel data")
        for i in range(count):
            pixels.append(pack(data[offset + i * bytes_pp: offset + (i + 1) * bytes_pp]))
    else:
        pos = offset
        while len(pixels) < count:
            if pos >= len(data):
                raise ValueError(f"{path.name}: truncated RLE data")
            packet = data[pos]
            pos += 1
            run = (packet & 0x7F) + 1
            if packet & 0x80:
                value = pack(data[pos:pos + bytes_pp])
                pos += bytes_pp
                pixels.extend([value] * run)
            else:
                for _ in range(run):
                    pixels.append(pack(data[pos:pos + bytes_pp]))
                    pos += bytes_pp
        pixels = pixels[:count]

    bottom_up = not (descriptor & 0x20)
    if bottom_up:
        rows = [pixels[y * width:(y + 1) * width] for y in range(height)]
        pixels = [p for row in reversed(rows) for p in row]

    right_to_left = bool(descriptor & 0x10)
    if right_to_left:
        rows = [pixels[y * width:(y + 1) * width] for y in range(height)]
        pixels = [p for row in rows for p in reversed(row)]

    return pixels


def format_header(argb: list[int]) -> str:
    lines = ["#pragma once", "", "#include <cstdint>", "",
             f"uint32_t tm_pixels[{WIDTH * HEIGHT}] =", "{"]
    for i in range(0, len(argb), VALUES_PER_LINE):
        chunk = argb[i:i + VALUES_PER_LINE]
        last = (i + VALUES_PER_LINE) >= len(argb)
        lines.append("\t" + ",".join(f"0x{v:x}" for v in chunk) + ("" if last else ","))
    lines.append("};")
    return "\n".join(lines) + "\n"


def format_embed_header(bin_name: str) -> str:
    lines = ["#pragma once", "", EMBED_PRELUDE]
    lines += embed_view_lines("inline const", "tm_pixels", "uint32_t", bin_name)
    lines.append(f"static_assert(sizeof(tm_pixels) == {WIDTH * HEIGHT} * sizeof(uint32_t));")
    lines.append(EMBED_EPILOGUE)
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tga", default="assets/textures/tm_pixels.tga")
    ap.add_argument("--out-dir", default="texture_gen_out")
    add_embed_argument(ap)
    args = ap.parse_args()

    argb = read_tga(Path(args.tga))
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    if args.embed:
        write_bin(out_dir / BIN_NAME, b"".join(struct.pack('<I', v) for v in argb))
        (out_dir / HEADER_NAME).write_text(format_embed_header(BIN_NAME))
    else:
        (out_dir / HEADER_NAME).write_text(format_header(argb))


if __name__ == "__main__":
    main()
