#!/usr/bin/env python3
import argparse
import struct
from pathlib import Path

GROUPS = {
    "logo": {
        "subdir": "logo",
        "header_name": "logo_data.h",
        "prefix": "xbl",
        "struct_name": "xbl_vertex",
        "define_style": "define",
        "has_uv": True,
        "meshes": [
            "xboxlogointerior",
            "xboxlogolip",
            "xboxlogosurfacetop",
            "xboxlogosurface",
            "tm_slash",
            "tm_wordmark",
        ],
        "point_arrays": [],
    },
    "text": {
        "subdir": "text",
        "header_name": "text_data.h",
        "prefix": "xbt",
        "struct_name": "xbt_vertex",
        "define_style": "const",
        "has_uv": False,
        "meshes": ["text"],
        "point_arrays": ["pos_anim_text"],
    },
}

BYTES_PER_LINE = 16
SENTINEL = 126

def parse_obj(path: Path):
    positions, uvs, faces = [], [], []
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        tag = parts[0]
        if tag == "v":
            positions.append(tuple(float(x) for x in parts[1:4]))
        elif tag == "vt":
            uvs.append(tuple(float(x) for x in parts[1:3]))
        elif tag == "f":
            corners = []
            for token in parts[1:]:
                fields = token.split("/")
                vi = int(fields[0])
                vi = vi - 1 if vi > 0 else len(positions) + vi
                ti = None
                if len(fields) > 1 and fields[1] != "":
                    ti = int(fields[1])
                    ti = ti - 1 if ti > 0 else len(uvs) + ti
                corners.append((vi, ti))

            for i in range(1, len(corners) - 1):
                faces.append([corners[0], corners[i], corners[i + 1]])
    return positions, (uvs or None), faces

def weld_mesh(positions, uvs, faces):
    already_welded = (uvs is None) or all(pos_idx == uv_idx for tri in faces for pos_idx, uv_idx in tri)
    if already_welded:
        if uvs is not None:
            verts = [positions[i] + uvs[i] for i in range(len(positions))]
        else:
            verts = list(positions)
        indices = [pos_idx for tri in faces for pos_idx, _ in tri]
        return verts, indices

    cache: dict[tuple, int] = {}
    verts: list[tuple] = []
    indices: list[int] = []
    for tri in faces:
        for pos_idx, uv_idx in tri:
            key = (pos_idx, uv_idx)
            if key not in cache:
                if uv_idx is None:
                    raise ValueError("mesh has uv-less face but group expects UVs")
                vert = positions[pos_idx] + uvs[uv_idx]
                cache[key] = len(verts)
                verts.append(vert)
            indices.append(cache[key])
    return verts, indices

def compute_quant(values: list[float]) -> tuple[float, float]:
    lo, hi = min(values), max(values)
    if hi == lo:
        return 1.0, lo
    scale = (hi - lo) / 65535.0
    delta = lo + 32768.0 * scale
    return scale, delta

def quantize(val: float, scale: float, delta: float) -> int:
    raw = round((val - delta) / scale) if scale else 0
    return max(-32768, min(32767, raw))

def compress_indices(indices: list[int]) -> bytes:
    if not (0 <= indices[0] <= 127):
        raise ValueError(
            f"first index in the flat triangle list is {indices[0]}, but the wire "
            "format stores it as a single non-negative signed byte (0..127) with "
            "no escape, reorder the mesh's face winding/vertex order so the "
            "first referenced vertex has a small index"
        )
    out = bytearray([indices[0]])
    prev = indices[0]
    for idx in indices[1:]:
        delta = idx - prev
        if -128 <= delta <= 127 and delta != SENTINEL:
            out.append(delta & 0xFF)
        else:
            if not (-32768 <= delta <= 32767):
                raise ValueError(f"index delta {delta} doesn't fit in 16 bits")
            out.append(SENTINEL)
            out += struct.pack('>h', delta)
        prev = idx
    return bytes(out)

def parse_point_obj(path: Path) -> list[tuple]:
    points = []
    for line in path.read_text().splitlines():
        line = line.strip()
        if line.startswith("v "):
            points.append(tuple(float(x) for x in line.split()[1:4]))
    return points

def emit_point_array(header_lines: list[str], struct_name: str, name: str, points: list[tuple]):
    header_lines.append(f"{struct_name} {name}[{len(points)}] =")
    header_lines.append("{")
    for i, p in enumerate(points):
        comma = "," if i + 1 < len(points) else ""
        header_lines.append(f"\t{{{p[0]:.6f}f,{p[1]:.6f}f,{p[2]:.6f}f}}{comma}")
    header_lines.append("};")

def format_decimal_array_body(values, width=6) -> str:
    lines = ["\n"]
    for i in range(0, len(values), width):
        chunk = values[i:i + width]
        body = ",".join(str(v) for v in chunk)
        is_last = (i + width) >= len(values)
        lines.append("\t" + body + ("" if is_last else ",") + "\n")
    return "".join(lines)

def as_signed_bytes(raw: bytes) -> list[int]:
    return [b - 256 if b >= 128 else b for b in raw]

def encode_group(group_key: str, cfg: dict, assets_dir: Path, out_dir: Path):
    mesh_dir = assets_dir / cfg["subdir"]
    meshes = {}
    for stem in cfg["meshes"]:
        obj_path = mesh_dir / f"{stem}.obj"
        positions, uvs, faces = parse_obj(obj_path)
        if cfg["has_uv"] and uvs is None:
            raise ValueError(f"{obj_path} has no vt data but group '{group_key}' expects UVs")
        verts, indices = weld_mesh(positions, uvs if cfg["has_uv"] else None, faces)
        meshes[stem] = (verts, indices)

    all_pos = [c for verts, _ in meshes.values() for v in verts for c in v[0:3]]
    pos_scale, pos_delta = compute_quant(all_pos)
    if cfg["has_uv"]:
        all_uv = [c for verts, _ in meshes.values() for v in verts for c in v[3:5]]
        tex_scale, tex_delta = compute_quant(all_uv)
    else:
        tex_scale, tex_delta = 1.0, 0.0

    out_dir.mkdir(parents=True, exist_ok=True)
    payload_lines = []

    header_lines = ["#pragma once"]
    header_lines.append(f"struct {cfg['struct_name']}")
    header_lines.append("{")
    if cfg["has_uv"]:
        header_lines.append("\tfloat x,y,z;")
        header_lines.append("\tfloat u0;")
        header_lines.append("\tfloat v0;")
    else:
        header_lines.append("\tfloat x,y,z;")
    header_lines.append("};")

    def emit_const(name, value):
        if cfg["define_style"] == "define":
            header_lines.append(f"#define {name} {value:.6f}f")
        else:
            header_lines.append(f"const float {name} = {value:.6f}f;")

    emit_const(f"{cfg['prefix']}_OO_POS_SCALE", pos_scale)
    emit_const(f"{cfg['prefix']}_POS_DELTA", pos_delta)
    emit_const(f"{cfg['prefix']}_OO_TEX_SCALE", tex_scale)
    emit_const(f"{cfg['prefix']}_TEX_DELTA", tex_delta)

    for stem in cfg["meshes"]:
        verts, indices = meshes[stem]
        stride = 5 if cfg["has_uv"] else 3
        raw_verts = []
        for v in verts:
            raw_verts.append(quantize(v[0], pos_scale, pos_delta))
            raw_verts.append(quantize(v[1], pos_scale, pos_delta))
            raw_verts.append(quantize(v[2], pos_scale, pos_delta))
            if cfg["has_uv"]:
                raw_verts.append(quantize(v[3], tex_scale, tex_delta))
                raw_verts.append(quantize(v[4], tex_scale, tex_delta))
        assert len(raw_verts) == len(verts) * stride

        raw_indices = compress_indices(indices)

        verts_inc = f"{stem}_verts.inc"
        indices_inc = f"{stem}_indices.inc"
        (out_dir / verts_inc).write_text(format_decimal_array_body(raw_verts))
        (out_dir / indices_inc).write_text(format_decimal_array_body(as_signed_bytes(raw_indices)))

        header_lines.append(f"const int vertex_count_{stem}_0 = {len(verts)};")
        header_lines.append(f"short verts_{stem}_0C[] =")
        header_lines.append("{")
        header_lines.append(f'\t#include "{verts_inc}"')
        header_lines.append("};")
        header_lines.append(f"const int index_count_{stem}_0 = {len(indices)};")
        header_lines.append(f"char indices_{stem}_0C[] =")
        header_lines.append("{")
        header_lines.append(f'\t#include "{indices_inc}"')
        header_lines.append("};")

    for name in cfg.get("point_arrays", []):
        obj_path = mesh_dir / f"{name}.obj"
        points = parse_point_obj(obj_path)
        if not points:
            raise ValueError(f"{obj_path}: expected at least one 'v' line")
        emit_point_array(header_lines, cfg["struct_name"], name, points)

    header_path = out_dir / cfg["header_name"]
    header_path.write_text("\n".join(header_lines) + "\n")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--assets-dir", default="assets/meshes")
    ap.add_argument("--out-dir", default="mesh_gen_out")
    ap.add_argument("--group", choices=list(GROUPS), help="encode only this group")
    args = ap.parse_args()

    assets_dir = Path(args.assets_dir)
    out_dir = Path(args.out_dir)

    keys = [args.group] if args.group else list(GROUPS)
    for key in keys:
        encode_group(key, GROUPS[key], assets_dir, out_dir)

if __name__ == "__main__":
    main()
