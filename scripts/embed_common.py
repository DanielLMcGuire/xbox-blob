#!/usr/bin/env python3
import argparse
from pathlib import Path

EMBED_PRAGMA_BEGIN = """\
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
"""

EMBED_PRAGMA_END = """\
#pragma GCC diagnostic pop
#ifdef __clang__
#pragma clang diagnostic pop
#endif
"""

EMBED_PRELUDE = EMBED_PRAGMA_BEGIN + """
#include <bit>
#include <cstddef>
#include <cstdint>

#ifndef XBB_EMBED_VIEW
#define XBB_EMBED_VIEW
static_assert(std::endian::native == std::endian::little,
              "embedded .bin assets are little-endian");

template <class T, std::size_t N>
inline const T (&xbb_embed_view(const signed char (&bytes)[N]))[N / sizeof(T)]
{
    static_assert(N % sizeof(T) == 0, "embedded size is not a multiple of the element size");
    return reinterpret_cast<const T (&)[N / sizeof(T)]>(bytes);
}
#endif
"""

EMBED_EPILOGUE = EMBED_PRAGMA_END

def add_embed_argument(ap: argparse.ArgumentParser) -> None:
    ap.add_argument(
        "--embed", action="store_true",
        help="write raw .bin files and have the generated header #embed them "
             "instead of writing numbers")

def embed_bytes_lines(bytes_name: str, bin_name: str) -> list[str]:
    return [
        f"alignas(8) inline constexpr signed char {bytes_name}[] = {{",
        f'    #embed "{bin_name}"',
        "};",
    ]

def embed_view_lines(decl_prefix: str, name: str, elem_type: str,
                     bin_name: str) -> list[str]:
    bytes_name = f"{name}_bytes"
    lines = embed_bytes_lines(bytes_name, bin_name)
    lines.append(f"{decl_prefix} auto& {name} = xbb_embed_view<{elem_type}>({bytes_name});")
    return lines

def write_bin(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
