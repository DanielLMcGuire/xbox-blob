#!/usr/bin/env python3
import argparse
import re
import struct
from pathlib import Path

LOGO_MESHES = [
    "xboxlogointerior",
    "xboxlogolip",
    "xboxlogosurfacetop",
    "xboxlogosurface",
    "tm_slash",
    "tm_wordmark",
]
TEXT_MESHES = ["text"]

INT_RE = re.compile(r'-?\d+')
FLOAT_RE = re.compile(r'-?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?')

def extract_block(text: str, start_marker: str, end_marker: str = "}") -> str:
    idx = text.index(start_marker)
    brace_start = text.index("{", idx)
    brace_end = text.index(end_marker, brace_start)
    return text[brace_start + 1:brace_end]

def parse_ints(block: str) -> list[int]:
    return [int(m) for m in INT_RE.findall(block)]

def parse_macro_float(text: str, name: str) -> float:
    m = re.search(rf'{re.escape(name)}\s*=?\s*({FLOAT_RE.pattern})f?', text)
    if not m:
        raise ValueError(f"couldn't find macro/const {name}")
    return float(m.group(1))

def decompress_indices(raw: bytes, count: int) -> list[int]:
    out = [0] * count
    out[0] = raw[0] - 256 if raw[0] >= 128 else raw[0]
    out[0] &= 0xFFFF
    pos = 0
    for i in range(1, count):
        b = raw[pos + i]
        if b == 126:
            hi = raw[pos + i + 1]
            lo = raw[pos + i + 2]
            delta = struct.unpack('>h', bytes([hi, lo]))[0]
            out[i] = (out[i - 1] + delta) & 0xFFFF
            pos += 2
        else:
            signed = b - 256 if b >= 128 else b
            out[i] = (out[i - 1] + signed) & 0xFFFF
    return out

def decode_mesh(rawVerts: list[int], vertCount: int, rawIndices: list[int], indexCount: int,
                 ooPosScale: float, posDelta: float,
                 ooTexScale: float | None, texDelta: float | None):
    stride = 5 if ooTexScale is not None else 3
    verts = []
    for i in range(vertCount):
        rv = rawVerts[i * stride:i * stride + stride]
        x = rv[0] * ooPosScale + posDelta
        y = rv[1] * ooPosScale + posDelta
        z = rv[2] * ooPosScale + posDelta
        if ooTexScale is not None:
            u = rv[3] * ooTexScale + texDelta
            v = rv[4] * ooTexScale + texDelta
            verts.append((x, y, z, u, v))
        else:
            verts.append((x, y, z))
    raw_bytes = bytes((b & 0xFF) for b in rawIndices)
    indices = decompress_indices(raw_bytes, indexCount)
    return verts, indices

def write_obj(path: Path, verts, indices, has_uv: bool, comment: str):
    lines = [f"# {comment}", "# auto-generated"]
    for v in verts:
        lines.append(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}")
    if has_uv:
        for v in verts:
            lines.append(f"vt {v[3]:.6f} {v[4]:.6f}")
    assert len(indices) % 3 == 0, "expected a flat triangle list"
    for i in range(0, len(indices), 3):
        a, b, c = (idx + 1 for idx in indices[i:i + 3])
        if has_uv:
            lines.append(f"f {a}/{a} {b}/{b} {c}/{c}")
        else:
            lines.append(f"f {a} {b} {c}")
    path.write_text("\n".join(lines) + "\n")

def decode_group(header_path: Path, stems: list[str], prefix: str, has_uv: bool, out_dir: Path):
    text = header_path.read_text()
    ooPosScale = parse_macro_float(text, f"{prefix}_OO_POS_SCALE")
    posDelta = parse_macro_float(text, f"{prefix}_POS_DELTA")
    ooTexScale = parse_macro_float(text, f"{prefix}_OO_TEX_SCALE") if has_uv else None
    texDelta = parse_macro_float(text, f"{prefix}_TEX_DELTA") if has_uv else None

    out_dir.mkdir(parents=True, exist_ok=True)
    for stem in stems:
        vertCount = int(re.search(rf'vertex_count_{re.escape(stem)}_0\s*=\s*(\d+)', text).group(1))
        indexCount = int(re.search(rf'index_count_{re.escape(stem)}_0\s*=\s*(\d+)', text).group(1))
        rawVerts = parse_ints(extract_block(text, f"verts_{stem}_0C[] ="))
        rawIndices = parse_ints(extract_block(text, f"indices_{stem}_0C[] ="))

        verts, indices = decode_mesh(rawVerts, vertCount, rawIndices, indexCount,
                                      ooPosScale, posDelta, ooTexScale, texDelta)
        out_path = out_dir / f"{stem}.obj"
        write_obj(out_path, verts, indices, has_uv, f"extracted from {header_path.name}: {stem}")

    decode_point_arrays(text, header_path, out_dir)

POINT_ARRAY_RE = re.compile(
    r'xbt_vertex\s+(\w+)\s*\[\s*\d*\s*\]\s*=\s*\{(.*?)\};', re.DOTALL)

def decode_point_arrays(text: str, header_path: Path, out_dir: Path):
    for m in POINT_ARRAY_RE.finditer(text):
        name, body = m.group(1), m.group(2)
        triples = re.findall(r'\{([^{}]*)\}', body)
        points = []
        for triple in triples:
            nums = [float(x) for x in FLOAT_RE.findall(triple)]
            if len(nums) >= 3:
                points.append(nums[:3])
        if not points:
            continue
        out_path = out_dir / f"{name}.obj"
        lines = [f"# extracted from {header_path.name}: {name}",
                 "# auto-generated"]
        lines += [f"v {p[0]:.6f} {p[1]:.6f} {p[2]:.6f}" for p in points]
        out_path.write_text("\n".join(lines) + "\n")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--logo-header", default="build/meshes/logo_data.h")
    ap.add_argument("--text-header", default="build/meshes/text_data.h")
    ap.add_argument("--out-dir", default="assets/meshes")
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    decode_group(Path(args.logo_header), LOGO_MESHES, "xbl", True, out_dir / "logo")
    decode_group(Path(args.text_header), TEXT_MESHES, "xbt", False, out_dir / "text")

if __name__ == "__main__":
    main()