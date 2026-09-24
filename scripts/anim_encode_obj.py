#!/usr/bin/env python3
import argparse
import math
import struct
from pathlib import Path

from anim_common import (PrimConstants, compress_indices, QUAT_SCALE,
                         POS_OBJ, ROT_OBJ)
from embed_common import add_embed_argument, embed_view_lines, write_bin, EMBED_PRELUDE, EMBED_EPILOGUE

HEADER_NAME = "anim_data.h"

def parse_anim_obj(path: Path, min_components: int, max_components: int):
    verts: list[tuple[float, ...]] = []
    seqs: list[list[int]] = []
    open_object = False

    for lineno, raw in enumerate(path.read_text().splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        tag, *parts = line.split()
        where = f"{path.name}:{lineno}"
        if tag == "v":
            if not (min_components <= len(parts) <= max_components):
                raise ValueError(f"{where}: 'v' needs {min_components}"
                                 f"{'' if min_components == max_components else '-' + str(max_components)}"
                                 f" components, got {len(parts)}")
            verts.append(tuple(float(p) for p in parts))
        elif tag == "o":
            if open_object:
                raise ValueError(f"{where}: previous 'o' object has no 'l' line")
            open_object = True
        elif tag == "l":
            if not open_object:
                raise ValueError(f"{where}: 'l' line outside an 'o' object")
            seq = []
            for token in parts:
                i = int(token.split("/")[0])
                i = i - 1 if i > 0 else len(verts) + i
                if not (0 <= i < len(verts)):
                    raise ValueError(f"{where}: index {token} is out of range")
                seq.append(i)
            seqs.append(seq)
            open_object = False

    if open_object:
        raise ValueError(f"{path.name}: last object has no 'l' line")
    return verts, seqs


def encode_pos(verts, const: PrimConstants) -> list[int]:
    out = []
    for n, v in enumerate(verts):
        for a in range(3):
            raw = round((v[a] - const.pos_delta[a]) / const.pos_scale[a])
            if not (-32768 <= raw <= 32767):
                raise ValueError(f"{POS_OBJ}: point {n + 1} axis {'xyz'[a]} = {v[a]} "
                                 "is outside the range scene_prim_types.h can represent")
            out.append(raw)
    return out


def encode_quats(verts) -> tuple[list[int], list[int]]:
    raw_out: list[int] = []
    signs = [0] * ((len(verts) + 31) // 32)
    for n, v in enumerate(verts):
        raw = [round(c * QUAT_SCALE) for c in v[:3]]
        if any(abs(r) > 32750 for r in raw) or sum(r * r for r in raw) > 32750 * 32750 + 1:
            raise ValueError(f"{ROT_OBJ}: quaternion {n + 1} has |xyz| > 1")
        raw_out += raw
        w = v[3] if len(v) > 3 else 1.0
        if math.copysign(1.0, w) > 0:
            signs[n >> 5] |= 1 << (n & 31)
    return raw_out, signs


def check_seqs(seqs, samples: int, what: str, expected: int):
    for n, seq in enumerate(seqs):
        if len(seq) != samples:
            raise ValueError(f"{what} animation {n} has {len(seq)} samples, "
                             f"scene_prim_types.h says {samples}")
    if len(seqs) != expected:
        raise ValueError(f"found {len(seqs)} {what} animations, scene_prim_types.h "
                         f"NUM_*_SEQ says {expected}")


def short_lines(name: str, values: list[int]) -> list[str]:
    lines = [f"static const short {name}[]=", "{"]
    for i in range(0, len(values), 3):
        lines.append("\t" + ",".join(str(v) for v in values[i:i + 3]) + ",")
    lines.append("};")
    return lines


def seq_lines(type_name: str, name: str, streams: list[bytes]) -> list[str]:
    lines = [f"static const {type_name} {name}[] =", "{"]
    for n, s in enumerate(streams):
        body = ",".join(str(b - 256 if b >= 128 else b) for b in s)
        lines.append(("{" if n == 0 else ",{") + body + "}")
    lines.append("};")
    return lines


def compress_all(seqs, max_bytes: int, what: str) -> list[bytes]:
    streams = [compress_indices(s) for s in seqs]
    for n, stream in enumerate(streams):
        if len(stream) > max_bytes:
            raise ValueError(f"{what} animation {n} compresses to {len(stream)} bytes, "
                             f"the sequence struct only holds {max_bytes}")
    return streams


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--assets-dir", default="assets/animation")
    ap.add_argument("--prim-types", default="src/scene/scene_prim_types.h")
    ap.add_argument("--out-dir", default="anim_gen_out")
    add_embed_argument(ap)
    args = ap.parse_args()

    assets = Path(args.assets_dir)
    const = PrimConstants(Path(args.prim_types))

    pos_verts, pos_seqs = parse_anim_obj(assets / POS_OBJ, 3, 3)
    rot_verts, rot_seqs = parse_anim_obj(assets / ROT_OBJ, 3, 4)
    check_seqs(pos_seqs, const.max_pos_samples, "position", const.num_pos_seq)
    check_seqs(rot_seqs, const.max_rot_samples, "rotation", const.num_rot_seq)

    pos_raw = encode_pos(pos_verts, const)
    quat_raw, quat_signs = encode_quats(rot_verts)

    pos_streams = compress_all(pos_seqs, const.max_pos_samples * 3, "position")
    rot_streams = compress_all(rot_seqs, const.max_rot_samples * 3, "rotation")

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    num_pos, num_quats = len(pos_raw) // 3, len(quat_raw) // 3

    lines = ["#pragma once", "", '#include "scene_prim_types.h"', ""]

    if args.embed:
        def padded(streams, size):
            return b"".join(s + bytes(size - len(s)) for s in streams)

        write_bin(out_dir / "rot_anim_seq.bin", padded(rot_streams, const.max_rot_samples * 3))
        write_bin(out_dir / "pos_anim_seq.bin", padded(pos_streams, const.max_pos_samples * 3))
        write_bin(out_dir / "pos.bin", struct.pack(f"<{len(pos_raw)}h", *pos_raw))
        write_bin(out_dir / "quats.bin", struct.pack(f"<{len(quat_raw)}h", *quat_raw))
        write_bin(out_dir / "quat_signs.bin", struct.pack(f"<{len(quat_signs)}I", *quat_signs))

        lines += [EMBED_PRELUDE,
                  "static_assert(sizeof(RotAnimSeq) == MAX_ROT_SAMPLES * 3);",
                  "static_assert(sizeof(PosAnimSeq) == MAX_POS_SAMPLES * 3);", ""]
        lines += embed_view_lines("static const", "theRotAnimSeq", "RotAnimSeq", "rot_anim_seq.bin")
        lines += embed_view_lines("static const", "thePosAnimSeq", "PosAnimSeq", "pos_anim_seq.bin")
        lines.append(f"static const short numPos = {num_pos};")
        lines += embed_view_lines("static const", "thePos", "short", "pos.bin")
        lines.append(f"static const short numQuats = {num_quats};")
        lines += embed_view_lines("static const", "theQuats", "short", "quats.bin")
        lines += embed_view_lines("static const", "theQuatSigns", "uint32_t", "quat_signs.bin")
        lines.append(EMBED_EPILOGUE)
    else:
        lines += ["#include <cstdint>", ""]
        lines += seq_lines("RotAnimSeq", "theRotAnimSeq", rot_streams)
        lines += seq_lines("PosAnimSeq", "thePosAnimSeq", pos_streams)
        lines.append(f"static const short numPos = {num_pos};")
        lines += short_lines("thePos", pos_raw)
        lines.append(f"static const short numQuats = {num_quats};")
        lines += short_lines("theQuats", quat_raw)
        lines.append("static const uint32_t theQuatSigns[]=")
        lines.append("{")
        lines += [f"\t0X{w:X}," for w in quat_signs]
        lines.append("};")

    (out_dir / HEADER_NAME).write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
