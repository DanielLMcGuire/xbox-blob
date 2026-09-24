#!/usr/bin/env python3
import argparse
import re
import struct
from pathlib import Path

WIDTH = 16
HEIGHT = 16

HEX_RE = re.compile(r'0[xX]([0-9A-Fa-f]+)')

def parse_header(path: Path) -> list[int]:
    text = path.read_text()
    m = re.search(r'tm_pixels\s*\[[^\]]*\]\s*=\s*\{(.*?)\}\s*;', text, re.DOTALL)
    if not m:
        raise ValueError(f"{path}: couldn't find the tm_pixels array")
    values = [int(h, 16) for h in HEX_RE.findall(m.group(1))]
    if len(values) != WIDTH * HEIGHT:
        raise ValueError(f"{path}: expected {WIDTH * HEIGHT} pixels, found {len(values)}")
    return values

def write_tga(path: Path, argb: list[int]) -> None:
    header = struct.pack('<BBB5xHHHHBB', 0, 0, 2, 0, 0, WIDTH, HEIGHT, 32, 0x28)
    body = b''.join(struct.pack('<BBBB', p & 0xFF, (p >> 8) & 0xFF,
                                (p >> 16) & 0xFF, (p >> 24) & 0xFF)
                    for p in argb)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + body)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--header", default="build/textures/tm_pixels.h")
    ap.add_argument("--out", default="assets/textures/tm_pixels.tga")
    args = ap.parse_args()

    write_tga(Path(args.out), parse_header(Path(args.header)))

if __name__ == "__main__":
    main()
