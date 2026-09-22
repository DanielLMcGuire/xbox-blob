#!/usr/bin/env python3
import argparse
import re
import struct
import wave
from pathlib import Path

SAMPLES = [
    ("GLOCK.pcm", True),
    ("BUBBLE.pcm", True),
    ("THUNEL16.pcm", False),
]

HEX_RE = re.compile(r'0x([0-9A-Fa-f]{1,2})')

def parse_bytes(path: Path) -> bytes:
    text = path.read_text()
    return bytes(int(h, 16) for h in HEX_RE.findall(text))

def to_pcm8(raw: bytes, xor: bool) -> bytes:
    out = bytearray()

    for b in raw:
        v = (b ^ 0x80) if xor else b
        signed8 = v - 256 if v >= 128 else v
        wav8 = (signed8 + 128) & 0xFF
        out.append(wav8)

    return bytes(out)

def write_wav(path: Path, pcm8: bytes, rate: int):
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(1)
        w.setframerate(rate)
        w.writeframes(pcm8)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src-dir", default="src/sos/samples")
    ap.add_argument("--out-dir", default="wav_out")
    ap.add_argument("--rate", type=int, default=48000)
    args = ap.parse_args()

    src_dir = Path(args.src_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    for name, xor in SAMPLES:
        raw = parse_bytes(src_dir / name)
        pcm8 = to_pcm8(raw, xor)
        out_path = out_dir / (Path(name).stem + ".wav")
        write_wav(out_path, pcm8, args.rate)

if __name__ == "__main__":
    main()