#!/usr/bin/env python3
"""Lists the textures in an XPR2 file and decodes them to PNG (top mip only).

    python -m sbx_formats.xpr_textures FILE.xpr            list the textures
    python -m sbx_formats.xpr_textures FILE.xpr -o DIR     write DIR/<name>.png

Handles the formats the car textures use: DXT1 (0x12), DXT2/3 (0x13),
DXT4/5 (0x14) and A8R8G8B8 (0x06), tiled or linear. The layout is in
docs/game_data.md ("Other formats", XPR2).
"""

import argparse
import os
import struct
import sys

import numpy as np

FMT_DXT1, FMT_DXT3, FMT_DXT5, FMT_ARGB8 = 0x12, 0x13, 0x14, 0x06
BLOCK_BYTES = {FMT_DXT1: 8, FMT_DXT3: 16, FMT_DXT5: 16}


def read_XPR(d):
    """XPR2 -> list of dicts (name, width, height, fmt, tiled, data offset)."""
    magic, header_size, data_size, count = struct.unpack_from('>4sIII', d, 0)
    if magic != b'XPR2':
        raise ValueError('not an XPR2 file')
    data_start = 12 + header_size
    out = []
    for i in range(count):
        rtype, off, size, name_off = struct.unpack_from('>4sIII', d, 16 + 16 * i)
        name = d[12 + name_off:12 + name_off + 128].split(b'\0')[0]
        name = name.decode('ascii', 'replace')
        if rtype != b'TX2D':
            out.append(dict(name=name, type=rtype.decode('ascii', 'replace')))
            continue
        # D3D texture header: 7 dwords, then the GPU fetch constant (6 dwords).
        fetch = struct.unpack_from('>6I', d, 12 + off + 0x1C)
        out.append(dict(
            name=name, type='TX2D',
            fmt=fetch[1] & 0x3F,
            tiled=bool(fetch[0] & (1 << 31)),
            pitch=((fetch[0] >> 22) & 0x1FF) * 32,
            width=(fetch[2] & 0x1FFF) + 1,
            height=((fetch[2] >> 13) & 0x1FFF) + 1,
            data=data_start + (fetch[1] & ~0xFFF),
        ))
    return out


def tiled_offset(x, y, width, log_bpp):
    """Xenos 2D tiled address of element (x, y), in elements.

    width is the row width in elements (blocks for DXT), log_bpp the log2
    of the element size in bytes.
    """
    aligned_width = (width + 31) & ~31
    macro = ((x >> 5) + (y >> 5) * (aligned_width >> 5)) << (log_bpp + 7)
    micro = ((x & 7) + ((y & 6) << 2)) << log_bpp
    offset = macro + ((micro & ~15) << 1) + (micro & 15) + ((y & 8) << (3 + log_bpp)) + ((y & 1) << 4)
    return (((offset & ~511) << 3) + ((offset & 448) << 2) + (offset & 63)
            + ((y & 16) << 7) + (((((y & 8) >> 2) + (x >> 3)) & 3) << 6)) >> log_bpp


def untile(raw, bw, bh, elem_bytes, tiled, pitch):
    """Raw element data -> (bh, bw, elem_bytes) array in linear order."""
    log_bpp = {4: 2, 8: 3, 16: 4}[elem_bytes]
    width = max(pitch, (bw + 31) & ~31)
    out = np.zeros((bh, bw, elem_bytes), np.uint8)
    src = np.frombuffer(raw, np.uint8)
    for y in range(bh):
        for x in range(bw):
            o = (tiled_offset(x, y, width, log_bpp) if tiled else y * width + x) * elem_bytes
            if o + elem_bytes <= len(src):
                out[y, x] = src[o:o + elem_bytes]
    return out


def rgb565(c):
    r = ((c >> 11) & 31) * 255 // 31
    g = ((c >> 5) & 63) * 255 // 63
    b = (c & 31) * 255 // 31
    return np.stack([r, g, b], -1).astype(np.int32)


def decode_color_block(b):
    """b: (..., 8) little-endian DXT color blocks -> (..., 16, 4) RGBA."""
    c0 = b[..., 0].astype(np.int32) | (b[..., 1].astype(np.int32) << 8)
    c1 = b[..., 2].astype(np.int32) | (b[..., 3].astype(np.int32) << 8)
    bits = (b[..., 4].astype(np.uint32) | (b[..., 5].astype(np.uint32) << 8)
            | (b[..., 6].astype(np.uint32) << 16) | (b[..., 7].astype(np.uint32) << 24))
    p0, p1 = rgb565(c0), rgb565(c1)
    four = (c0 > c1)[..., None]
    p2 = np.where(four, (2 * p0 + p1) // 3, (p0 + p1) // 2)
    p3 = np.where(four, (p0 + 2 * p1) // 3, 0)
    pal = np.stack([p0, p1, p2, p3], -2)  # (..., 4, 3)
    alpha = np.where(four, 255, np.array([255, 255, 255, 0]))  # (..., 4)
    alpha = np.broadcast_to(alpha, pal.shape[:-1])
    idx = (bits[..., None] >> (2 * np.arange(16))) & 3
    rgb = np.take_along_axis(pal, idx[..., None].astype(np.int64).repeat(3, -1), -2)
    a = np.take_along_axis(alpha, idx.astype(np.int64), -1)
    return np.concatenate([rgb, a[..., None]], -1)


def decode_dxt5_alpha(b):
    a0 = b[..., 0].astype(np.int32)
    a1 = b[..., 1].astype(np.int32)
    bits = np.zeros(b.shape[:-1], np.uint64)
    for k in range(6):
        bits |= b[..., 2 + k].astype(np.uint64) << np.uint64(8 * k)
    idx = ((bits[..., None] >> (np.uint64(3) * np.arange(16, dtype=np.uint64))) & np.uint64(7)).astype(np.int64)
    eight = (a0 > a1)[..., None]
    pal8 = np.stack([a0, a1] + [((7 - k) * a0 + k * a1) // 7 for k in range(1, 7)], -1)
    pal6 = np.stack([a0, a1] + [((5 - k) * a0 + k * a1) // 5 for k in range(1, 5)]
                    + [np.zeros_like(a0), np.full_like(a0, 255)], -1)
    pal = np.where(eight, pal8, pal6)
    return np.take_along_axis(pal, idx, -1)


def decode(d, tex):
    w, h, fmt = tex['width'], tex['height'], tex['fmt']
    if fmt == FMT_ARGB8:
        px = untile(d[tex['data']:tex['data'] + max(tex['pitch'], (w + 31) & ~31) * ((h + 31) & ~31) * 4],
                    w, h, 4, tex['tiled'], tex['pitch'])
        return px[..., [1, 2, 3, 0]]  # stored A, R, G, B
    if fmt not in BLOCK_BYTES:
        raise ValueError('format 0x%02X not handled' % fmt)
    nb = BLOCK_BYTES[fmt]
    bw, bh = (w + 3) // 4, (h + 3) // 4
    pitch_blocks = max(tex['pitch'] // 4, (bw + 31) & ~31)
    size = pitch_blocks * ((bh + 31) & ~31) * nb
    blocks = untile(d[tex['data']:tex['data'] + size], bw, bh, nb, tex['tiled'], pitch_blocks)
    blocks = blocks.reshape(bh, bw, nb // 2, 2)[..., ::-1].reshape(bh, bw, nb)  # 8in16 swap
    if fmt == FMT_DXT1:
        px = decode_color_block(blocks)
    else:
        px = decode_color_block(blocks[..., 8:16])
        if fmt == FMT_DXT3:
            a = blocks[..., 0:8]
            nib = np.stack([a & 15, a >> 4], -1).reshape(bh, bw, 16).astype(np.int32)
            px[..., 3] = nib * 17
        else:
            px[..., 3] = decode_dxt5_alpha(blocks[..., 0:8])
    img = px.reshape(bh, bw, 4, 4, 4).transpose(0, 2, 1, 3, 4).reshape(bh * 4, bw * 4, 4)
    return img[:h, :w].astype(np.uint8)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('xpr')
    ap.add_argument('-o', '--out-dir', help='write <name>.png files here')
    args = ap.parse_args()
    with open(args.xpr, 'rb') as f:
        d = f.read()
    texs = read_XPR(d)
    if args.out_dir:
        from PIL import Image
        os.makedirs(args.out_dir, exist_ok=True)
    for i, t in enumerate(texs):
        if t['type'] != 'TX2D':
            print('%2d %s (%s)' % (i, t['name'], t['type']))
            continue
        print('%2d %-20s %4dx%-4d fmt 0x%02X %s' % (i, t['name'], t['width'], t['height'], t['fmt'],
                                                    'tiled' if t['tiled'] else 'linear'))
        if args.out_dir:
            try:
                img = decode(d, t)
            except ValueError as e:
                print('   skipped: %s' % e)
                continue
            name = ''.join(c if c.isalnum() or c in '_-' else '_' for c in t['name'])
            Image.fromarray(img, 'RGBA').save(os.path.join(args.out_dir, name + '.png'))


if __name__ == '__main__':
    sys.exit(main())
