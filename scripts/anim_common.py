#!/usr/bin/env python3
import re
import struct
from pathlib import Path

QUAT_SCALE = 32750.0
ESCAPE = 127

POS_OBJ = "pos_palette.obj"
ROT_OBJ = "rot_palette.obj"
POS_SEQ_PREFIX = "pos_anim_"
ROT_SEQ_PREFIX = "rot_anim_"

_FLOAT = r'-?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?'

class PrimConstants:
    def __init__(self, path: Path):
        text = path.read_text()

        def define(name: str) -> int:
            m = re.search(rf'#define\s+{name}\s+(\d+)', text)
            if not m:
                raise ValueError(f"{path}: missing #define {name}")
            return int(m.group(1))

        def const_float(name: str) -> float:
            m = re.search(rf'const\s+float\s+{name}\s*=\s*({_FLOAT})f?\s*;', text)
            if not m:
                raise ValueError(f"{path}: missing const float {name}")
            return float(m.group(1))

        self.max_pos_samples = define("MAX_POS_SAMPLES")
        self.max_rot_samples = define("MAX_ROT_SAMPLES")
        self.num_pos_seq = define("NUM_POS_SEQ")
        self.num_rot_seq = define("NUM_ROT_SEQ")
        self.pos_scale = tuple(const_float(f"OO_POS_ANIM_SCALE_{a}") for a in "XYZ")
        self.pos_delta = tuple(const_float(f"POS_ANIM_DELTA_{a}") for a in "XYZ")

def compress_indices(indices: list[int]) -> bytes:
    out = bytearray()
    prev = 0
    for idx in indices:
        delta = idx - prev
        if -128 <= delta <= 127 and delta != ESCAPE:
            out.append(delta & 0xFF)
        else:
            if not (-32768 <= delta <= 32767):
                raise ValueError(f"index delta {delta} doesn't fit in 16 bits")
            out.append(ESCAPE)
            out += struct.pack('>h', delta)
        prev = idx
    return bytes(out)

def decompress_indices(raw: bytes, count: int) -> list[int]:
    def s8(b: int) -> int:
        return b - 256 if b >= 128 else b

    out: list[int] = []
    pos = 0
    prev = 0
    for _ in range(count):
        b = raw[pos]
        if b == ESCAPE:
            delta = struct.unpack('>h', bytes([raw[pos + 1], raw[pos + 2]]))[0]
            pos += 3
        else:
            delta = s8(b)
            pos += 1
        prev = ((prev + delta + 32768) & 0xFFFF) - 32768
        out.append(prev)
    return out
