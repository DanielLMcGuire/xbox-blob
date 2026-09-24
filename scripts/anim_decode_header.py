#!/usr/bin/env python3
import argparse
import math
import re
from pathlib import Path

from anim_common import (PrimConstants, decompress_indices, QUAT_SCALE,
                         POS_OBJ, ROT_OBJ, POS_SEQ_PREFIX, ROT_SEQ_PREFIX)

INT_RE = re.compile(r'-?\d+')
HEX_RE = re.compile(r'0[xX]([0-9A-Fa-f]+)')

def array_body(text: str, name: str) -> str:
    m = re.search(rf'\b{name}\s*\[\s*\]\s*=\s*\{{(.*?)\n\}};', text, re.DOTALL)
    if not m:
        raise ValueError(f"couldn't find array '{name}'")
    return m.group(1)

def parse_ints(body: str) -> list[int]:
    return [int(v) for v in INT_RE.findall(body)]

def parse_seqs(body: str, bytes_per_seq: int) -> list[bytes]:
    seqs = []
    for group in re.findall(r'\{([^{}]*)\}', body):
        vals = parse_ints(group)
        if len(vals) > bytes_per_seq:
            raise ValueError(f"animation entry has {len(vals)} bytes, limit is {bytes_per_seq}")
        seqs.append(bytes(v & 0xFF for v in vals) + bytes(bytes_per_seq - len(vals)))
    return seqs

def format_obj(header_comment: list[str], verts: list[str], seq_prefix: str,
               seqs: list[list[int]]) -> str:
    lines = ["# auto-generated"] + [f"# {c}" for c in header_comment]
    lines += verts
    for n, seq in enumerate(seqs):
        lines.append(f"o {seq_prefix}{n:02d}")
        lines.append("l " + " ".join(str(i + 1) for i in seq))
    return "\n".join(lines) + "\n"

def decode(header: Path, prim_types: Path, out_dir: Path) -> None:
    const = PrimConstants(prim_types)
    text = header.read_text()

    pos_raw = parse_ints(array_body(text, "thePos"))
    quat_raw = parse_ints(array_body(text, "theQuats"))
    sign_words = [int(h, 16) for h in HEX_RE.findall(array_body(text, "theQuatSigns"))]
    pos_seqs = parse_seqs(array_body(text, "thePosAnimSeq"), const.max_pos_samples * 3)
    rot_seqs = parse_seqs(array_body(text, "theRotAnimSeq"), const.max_rot_samples * 3)

    if len(pos_raw) % 3 or len(quat_raw) % 3:
        raise ValueError("palette arrays must hold xyz triples")
    num_pos, num_quats = len(pos_raw) // 3, len(quat_raw) // 3
    if len(sign_words) * 32 < num_quats:
        raise ValueError("theQuatSigns is too short for theQuats")

    pos_verts = []
    for i in range(num_pos):
        p = [pos_raw[i * 3 + a] * const.pos_scale[a] + const.pos_delta[a] for a in range(3)]
        pos_verts.append(f"v {p[0]:.6f} {p[1]:.6f} {p[2]:.6f}")

    rot_verts = []
    for i in range(num_quats):
        x, y, z = (quat_raw[i * 3 + a] / QUAT_SCALE for a in range(3))
        w = math.sqrt(max(0.0, 1.0 - x * x - y * y - z * z))
        if not (sign_words[i >> 5] >> (i & 31)) & 1:
            w = -w
        rot_verts.append(f"v {x:.8f} {y:.8f} {z:.8f} {w:.8f}")

    pos_idx = [decompress_indices(s, const.max_pos_samples) for s in pos_seqs]
    rot_idx = [decompress_indices(s, const.max_rot_samples) for s in rot_seqs]

    for seqs, count, what in ((pos_idx, num_pos, "position"), (rot_idx, num_quats, "rotation")):
        for n, seq in enumerate(seqs):
            if any(not (0 <= i < count) for i in seq):
                raise ValueError(f"{what} animation {n} references a palette index out of range")

    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / POS_OBJ).write_text(format_obj(
        [f"extracted from {header.name}: position palette and position animations",
         "v x y z     palette point",
         "o / l       one path per animation, 30 palette points in time order"],
        pos_verts, POS_SEQ_PREFIX, pos_idx))
    (out_dir / ROT_OBJ).write_text(format_obj(
        [f"extracted from {header.name}: rotation palette and rotation animations",
         "v x y z w   quaternion xyz as stored, the sign of w is stored, |w| is derived",
         "o / l       one path per animation, 30 palette entries in time order"],
        rot_verts, ROT_SEQ_PREFIX, rot_idx))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--header", default="build/animation/anim_data.h")
    ap.add_argument("--prim-types", default="src/scene/scene_prim_types.h")
    ap.add_argument("--out-dir", default="assets/animation")
    args = ap.parse_args()

    decode(Path(args.header), Path(args.prim_types), Path(args.out_dir))

if __name__ == "__main__":
    main()
