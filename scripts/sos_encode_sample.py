#!/usr/bin/env python3
import argparse
import struct
import wave
from pathlib import Path

XOR_STEM_MAP = {
    "glock": True,
    "bubble": True,
    "thunel16": False,
}

ARRAY_NAME_MAP = {
    "thunel16": "ThunEl16Data",
    "glock": "GlockData",
    "bubble": "BubbleData",
}

HEADER_NAME = "samples.h"

BYTES_PER_LINE = 16

def read_mono_samples(path: Path):
    with wave.open(str(path), 'rb') as w:
        nch = w.getnchannels()
        sw = w.getsampwidth()
        n = w.getnframes()
        raw = w.readframes(n)

    if nch != 1:
        raise ValueError(f"{path.name}: expected mono, got {nch} channels")

    values = []

    if sw == 2:
        for (s,) in struct.iter_unpack('<h', raw):
            signed8 = round(s / 256.0)
            signed8 = max(-128, min(127, signed8))
            values.append(signed8 & 0xFF)

    elif sw == 1:
        for b in raw:
            signed8 = b - 128
            values.append(signed8 & 0xFF)

    else:
        raise ValueError(f"{path.name}: unsupported sample width {sw * 8}-bit")

    return values

def to_raw_bytes(values, xor: bool) -> bytes:
    if xor:
        return bytes(v ^ 0x80 for v in values)
    return bytes(values)


def format_c_array_body(data: bytes) -> str:
    lines = ["\r\n"]
    for i in range(0, len(data), BYTES_PER_LINE):
        chunk = data[i:i + BYTES_PER_LINE]
        body = ",".join(f"0x{b:02x}" for b in chunk)
        is_last_line = (i + BYTES_PER_LINE) >= len(data)
        line = "\t" + body + ("" if is_last_line else ",")
        lines.append(line + "\r\n")
    return "".join(lines)

def write_header(out_dir: Path, entries: list[tuple[str, str]]) -> Path:
    lines = ["#pragma once", ""]
    for array_name, pcm_name in entries:
        lines.append(f"inline constexpr unsigned char {array_name}[] = {{")
        lines.append(f'    #include "{pcm_name}"')
        lines.append("};")
    header_path = out_dir / HEADER_NAME
    header_path.write_text("\n".join(lines) + "\n")
    return header_path


def resolve_xor(stem: str, override: bool | None) -> bool:
    if override is not None:
        return override
    key = stem.lower()
    if key not in XOR_STEM_MAP:
        raise ValueError(
            f"Unknown sample '{stem}' pass --xor/--no-xor explicitly "
            f"(known: {', '.join(XOR_STEM_MAP)})"
        )
    return XOR_STEM_MAP[key]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("wavs", nargs="*", help="explicit .wav files to encode")
    ap.add_argument("--wav-dir")
    ap.add_argument("--out-dir", default="samples_out")
    xg = ap.add_mutually_exclusive_group()
    xg.add_argument("--xor", dest="xor", action="store_true", default=None,
                     help="force xor=True for all inputs given")
    xg.add_argument("--no-xor", dest="xor", action="store_false",
                     help="force xor=False for all inputs given")
    args = ap.parse_args()

    wav_paths = [Path(p) for p in args.wavs]
    if args.wav_dir:
        wav_dir = Path(args.wav_dir)
        wav_paths += [wav_dir / f"{stem.upper() if stem != 'thunel16' else 'THUNEL16'}.wav"
                      for stem in XOR_STEM_MAP]
        seen = set()
        wav_paths = [p for p in wav_paths if not (p in seen or seen.add(p))]

    if not wav_paths:
        ap.error("provide .wav files or --wav-dir")

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    generated = {}

    for wav_path in wav_paths:
        if not wav_path.exists():
            print(f"skip: {wav_path} not found")
            continue

        stem = wav_path.stem
        xor = resolve_xor(stem, args.xor)

        values = read_mono_samples(wav_path)
        raw = to_raw_bytes(values, xor)

        out_name = stem.upper() + ".pcm" if stem.lower() != "thunel16" else "THUNEL16.pcm"
        out_path = out_dir / out_name
        out_path.write_text(format_c_array_body(raw), newline="")

        generated[stem.lower()] = out_name

    header_entries = [
        (ARRAY_NAME_MAP[stem], generated[stem])
        for stem in ARRAY_NAME_MAP
        if stem in generated
    ]
    if len(header_entries) == len(ARRAY_NAME_MAP):
        header_path = write_header(out_dir, header_entries)
    elif generated:
        print("skip: samples.h not (re)generated, missing "
              f"{sorted(set(ARRAY_NAME_MAP) - set(generated))}")

if __name__ == "__main__":
    main()