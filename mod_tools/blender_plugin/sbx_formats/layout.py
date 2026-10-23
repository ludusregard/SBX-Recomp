"""Field layouts of the PART file's fixed-size records.

One table per record: (name, offset, struct format), big-endian, covering
every byte. The reader decodes with these, the diff names differences with
them, and the writer patches through them. Fields named unknown_* aren't
worked out; they're carried through untouched. See docs/game_data.md,
"Part files".
"""

from .binary import check_layout

HEADER_SIZE = 0x50
PART_NODE_SIZE = 192
SHAPE_SIZE = 0x220
MATERIAL_SIZE = 0x200

HEADER_FIELDS = (
    ('magic', 0x00, 'I'),
    ('unknown_04', 0x04, 'I'),          # 0
    ('node_count', 0x08, 'I'),
    ('shape_count', 0x0C, 'I'),
    ('unknown_10', 0x10, 'I'),          # 0
    ('nodes_offset', 0x14, 'I'),
    ('nodes_offset_2', 0x18, 'I'),      # same as nodes_offset in every part
    ('shapes_offset', 0x1C, 'I'),
    ('materials_offset', 0x20, 'I'),
    ('embedded_offset', 0x24, 'I'),
    ('small_offsets', 0x28, '6I'),      # small sections, not worked out
    ('vertex_data_offset', 0x40, 'I'),
    ('vertex_data_size', 0x44, 'I'),    # streams + index buffers, in bytes
    ('shaders_offset', 0x48, 'I'),
    ('unknown_4c', 0x4C, 'I'),          # 1 (0 in one part)
)

PART_NODE_FIELDS = (
    ('name', 0x00, '32s'),
    ('kind', 0x20, 'I'),                # 1 = a shape's attach node, 0 = marker
    ('node_index', 0x24, 'i'),          # index in the embedded node file
    ('shape_index', 0x28, 'i'),         # -1 for a marker
    ('unknown_2c', 0x2C, '148s'),
)

SHAPE_FIELDS = (
    ('name', 0x000, '32s'),
    ('matrix', 0x020, '16f'),
    ('bounds_box', 0x060, '32f'),       # 8 corners, float4 each
    ('bounds_sphere', 0x0E0, '4f'),     # center xyz, radius
    ('stream_0', 0x0F0, '4I'),          # count, type, stride, size
    ('stream_1', 0x100, '4I'),
    ('stream_2', 0x110, '4I'),
    ('fetch_0', 0x120, '8I'),           # 1, 1, 0, 0, 0, FFFF0000, offset | 3, size | 2
    ('fetch_1', 0x140, '8I'),
    ('fetch_2', 0x160, '8I'),
    ('stream_offsets', 0x180, '3I'),
    ('material_offset', 0x18C, 'I'),
    ('skin_position_offset', 0x190, 'I'),
    ('skin_normal_offset', 0x194, 'I'),
    ('skin_offset', 0x198, 'I'),
    ('unknown_19c', 0x19C, '32s'),
    ('unknown_1bc', 0x1BC, 'I'),        # 4
    ('unknown_1c0', 0x1C0, '96s'),
)

MATERIAL_FIELDS = (
    ('unknown_000', 0x000, 'I'),        # 0-5
    ('blend', 0x004, 'I'),              # 1 on blended draws (inferred)
    ('diffuse', 0x008, '4f'),           # inferred
    ('ambient', 0x018, '4f'),           # inferred
    ('specular', 0x028, '4f'),          # inferred
    ('unknown_038', 0x038, '32s'),
    ('power', 0x058, 'f'),              # specular power (inferred)
    ('params', 0x05C, '2f'),
    ('unknown_064', 0x064, 'I'),
    ('shader', 0x068, 'I'),             # index of one of the part's shader pairs
    ('unknown_06c', 0x06C, 'I'),
    ('flags', 0x070, 'I'),              # 0, 1 or 3
    ('unknown_074', 0x074, '20s'),
    ('texture_count', 0x088, 'I'),
    ('unknown_08c', 0x08C, '12s'),
    ('texture_xprs', 0x098, '8i'),      # XPR2 index in the model's list
    ('texture_indices', 0x0B8, '8i'),   # texture index in that XPR2, -1 = none
    ('unknown_0d8', 0x0D8, '128s'),
    ('index_count', 0x158, 'I'),
    ('primitive_count', 0x15C, 'I'),
    ('primitive_type', 0x160, 'I'),     # 6 = triangle strip
    ('unknown_164', 0x164, '28s'),      # 6, 0x20000002, 1, 0, 0, 0, FFFF0000
    ('index_offset', 0x180, 'I'),       # from the vertex data
    ('index_size', 0x184, 'I'),         # rounded up to 4 bytes
    ('index_file_offset', 0x188, 'I'),
    ('unknown_18c', 0x18C, 'I'),
    ('shared_shader', 0x190, 'I'),      # shared material shader pair (1000-1005) or 0
    ('unknown_194', 0x194, '108s'),
)

# Shared material shader pairs (material record +0x190) by role, from
# the RX7 (docs/game_data.md, "Part files").
SHARED_SHADER_ROLES = {1000: 'paint', 1001: 'bonnet', 1002: 'cover', 1003: 'trim'}

check_layout(HEADER_FIELDS, HEADER_SIZE)
check_layout(PART_NODE_FIELDS, PART_NODE_SIZE)
check_layout(SHAPE_FIELDS, SHAPE_SIZE)
check_layout(MATERIAL_FIELDS, MATERIAL_SIZE)
