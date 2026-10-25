"""Compiles Intel's CMAA2 (cmaa2/CMAA2.hlsl, unmodified from
https://github.com/GameTechDev/CMAA2, Apache 2.0) into SPIR-V for the native
renderer's --cmaa2 (passes.cpp, ApplyCmaa2), written to
../generated_cmaa2.h. Checked in, so the build needs no shader compiler; run
from this directory after changing anything here:

  python3 compile_cmaa2.py

Needs glslangValidator (and spirv-val, if present, to check the result).

The HLSL is patched on the way, as text, for glslang and Vulkan:
- its #error lines (reminders to set the defines below) are dropped;
- each resource gets an explicit binding in set 0: 0 the color read
  (sampled image), 1 the color written (storage image, rgba16f), 2 edges
  (r32ui), 3 shape candidates, 4 blend locations, 5 blend items, 6 item list
  heads (r32ui), 7 the control buffer, 8 the indirect dispatch arguments;
- Load(position, offset) on the color becomes Load(position + offset):
  glslang emits it as OpImageFetch with an Offset, which Vulkan allows only
  on gathers;
- the control buffer's InterlockedAdd(..., 1, ...) add 1u: an int there
  makes OpAtomicIAdd with mismatched types;
- the edge threshold is specialization constant 0 (the renderer's
  --cmaa2_threshold; the preset's 0.07 by default in the SPIR-V);
- the written pixels keep the input's alpha (CMAA2 writes 0; the game's
  post-processing may read the main view's alpha).
glslang ignores [[vk::image_format]], so the written color's storage image
is declared rgba32f; it's changed to rgba16f in the SPIR-V (the main view's
format).
"""
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.join(HERE, 'cmaa2', 'CMAA2.hlsl')
OUT = os.path.join(HERE, '..', 'generated_cmaa2.h')

# Name in the header, HLSL entry point.
KERNELS = [
    ('edges', 'EdgesColor2x2CS'),
    ('dispatch_args', 'ComputeDispatchArgsCS'),
    ('process_candidates', 'ProcessCandidatesCS'),
    ('deferred_apply', 'DeferredColorApply2x2CS'),
]

DEFINES = [
    'CMAA2_UAV_STORE_TYPED=1',              # the output is a typed storage image
    'CMAA2_UAV_STORE_CONVERT_TO_SRGB=0',
    'CMAA2_UAV_STORE_TYPED_UNORM_FLOAT=0',  # a float format (rgba16f)
    'CMAA2_SUPPORT_HDR_COLOR_RANGE=0',      # its input is tonemapped into 0..1 (aa_in.frag)
    'CMAA2_EDGE_DETECTION_LUMA_PATH=1',     # luma worked out from the color
    'CMAA2_STATIC_QUALITY_PRESET=2',        # high
]

BINDINGS = {
    'g_inoutColorReadonly': '[[vk::binding(0, 0)]]',
    'g_inoutColorWriteonly': '[[vk::binding(1, 0)]]',
    'g_workingEdges': '[[vk::binding(2, 0)]]',
    'g_workingShapeCandidates': '[[vk::binding(3, 0)]]',
    'g_workingDeferredBlendLocationList': '[[vk::binding(4, 0)]]',
    'g_workingDeferredBlendItemList': '[[vk::binding(5, 0)]]',
    'g_workingDeferredBlendItemListHeads': '[[vk::binding(6, 0)]]',
    'g_workingControlBuffer': '[[vk::binding(7, 0)]]',
    'g_workingExecuteIndirectBuffer': '[[vk::binding(8, 0)]]',
}


def patch(text):
    lines = []
    for line in text.split('\n'):
        if line.lstrip().startswith('#error'):
            line = '// (#error dropped by compile_cmaa2.py)'
        m = re.match(r'^\w[\w<> ]*?\s+(g_\w+)\s+: register', line)
        if m and m.group(1) in BINDINGS:
            line = BINDINGS[m.group(1)] + ' ' + line
        lines.append(line)
    text = '\n'.join(lines)
    old = 'g_inoutColorReadonly.Load( int3( pixelPos, 0 ), offset )'
    assert old in text
    text = text.replace(old, 'g_inoutColorReadonly.Load( int3( int2( pixelPos ) + offset, 0 ) )')
    text, n = re.subn(r'InterlockedAdd\( (4\*\d+), 1, ', r'InterlockedAdd( \1, 1u, ', text)
    assert n == 3, n
    old = '    lpfloat retVal = g_CMAA2_EdgeThreshold;'
    assert text.count(old) == 1
    text = text.replace('lpfloat GetActualEdgeThreshold( )',
                        '[[vk::constant_id(0)]] const float c_edgeThreshold = 0.07;\n'
                        'lpfloat GetActualEdgeThreshold( )')
    text = text.replace(old, '    lpfloat retVal = c_edgeThreshold;')
    old = 'g_inoutColorWriteonly[ pixelPos ] = lpfloat4( color.rgb, 0 );'
    assert old in text
    text = text.replace(old, 'g_inoutColorWriteonly[ pixelPos ] = lpfloat4( color.rgb, '
                        'g_inoutColorReadonly.Load( int3( pixelPos, 0 ) ).a );')
    return text


# SPIR-V: OpTypeImage (25) words: result, sampled type, dim, depth, arrayed,
# ms, sampled, format. Storage (sampled 2) float images declared Rgba32f (1)
# become Rgba16f (2).
def fix_formats(words):
    float_types = set()
    i = 5
    while i < len(words):
        count, op = words[i] >> 16, words[i] & 0xFFFF
        if op == 22 and words[i + 2] == 32:  # OpTypeFloat 32
            float_types.add(words[i + 1])
        if op == 25 and words[i + 2] in float_types and words[i + 7] == 2 and words[i + 8] == 1:
            words[i + 8] = 2
        i += count
    return words


def main():
    if not shutil.which('glslangValidator'):
        sys.exit('glslangValidator not found')
    with open(SOURCE, encoding='utf-8') as f:
        hlsl = patch(f.read())
    out = ['// Generated by src/native/glsl/compile_cmaa2.py from cmaa2/CMAA2.hlsl',
           '// (Intel\'s CMAA2, Apache 2.0: Copyright (c) 2018, Intel Corporation);',
           '// don\'t edit.',
           '#pragma once',
           '#include <cstdint>',
           '']
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, 'cmaa2.hlsl')
        with open(src, 'w', encoding='utf-8') as f:
            f.write(hlsl)
        for name, entry in KERNELS:
            spv = os.path.join(tmp, name + '.spv')
            cmd = ['glslangValidator', '-V', '-D', '-S', 'comp', '-e', 'main',
                   '--source-entrypoint', entry, '--target-env', 'vulkan1.1']
            cmd += ['-D' + d for d in DEFINES] + [src, '-o', spv]
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode:
                sys.exit(result.stdout + result.stderr)
            data = open(spv, 'rb').read()
            words = fix_formats(list(struct.unpack(f'<{len(data) // 4}I', data)))
            with open(spv, 'wb') as f:
                f.write(struct.pack(f'<{len(words)}I', *words))
            if shutil.which('spirv-val'):
                result = subprocess.run(['spirv-val', '--target-env', 'vulkan1.1', spv],
                                        capture_output=True, text=True)
                if result.returncode:
                    sys.exit(f'{entry}: ' + result.stdout + result.stderr)
            out.append(f'inline constexpr uint32_t k_cmaa2_{name}[] = {{')
            for i in range(0, len(words), 8):
                out.append('    ' + ', '.join(f'0x{w:08x}' for w in words[i:i + 8]) + ',')
            out.append('};')
            out.append('')
    with open(OUT, 'w', newline='\r\n') as f:
        f.write('\n'.join(out))
    print('written', os.path.normpath(OUT))


main()
