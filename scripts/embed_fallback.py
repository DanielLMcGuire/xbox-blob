#!/usr/bin/env python3
import argparse
from pathlib import Path

RAW_STRING_DELIM = "GLSL"

def generate_inl(source_text: str) -> str:
    if f'){RAW_STRING_DELIM}"' in source_text:
        raise ValueError(f"source contains the raw string terminator )'{RAW_STRING_DELIM}\"")
    return f'R"{RAW_STRING_DELIM}({source_text}){RAW_STRING_DELIM}"'

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input", help="shader source file (.vert/.frag)")
    ap.add_argument("output", help="generated .inl file to write")
    args = ap.parse_args()

    in_path = Path(args.input)
    out_path = Path(args.output)

    source_text = in_path.read_text()
    inl_text = generate_inl(source_text)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(inl_text)

if __name__ == "__main__":
    main()