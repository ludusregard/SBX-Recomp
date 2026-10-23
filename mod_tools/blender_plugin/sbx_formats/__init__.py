"""SBX file formats: the Blender add-on's format code (build.py copies it in).

Plain Python and numpy, no Blender. One module per job:

    binary.py      big-endian helpers, the Record base
    layout.py      field tables of the PART records
    node.py        NODE files (read; raw write)
    streams.py     vertex streams
    strips.py      triangle strips
    part_model.py  the in-memory PART model
    part_read.py   PART reader
    part_write.py  PART writer
    decode.py      the decoded view: positions, normals, UVs, triangles
    cars.py        car blocks: part slots, the node frame, textures
    sidecar.py     what Blender can't hold, as JSON
    shaders.py     the shader section and the shader library
    diff.py        field-by-field comparison of two PART files
    encode.py      encoding edited shapes
    xpr_textures.py  XPR2 textures: list and decode

Format docs: docs/game_data.md ("Models", "Node files", "Part files").
"""

from .node import NODEFile, Node, node_by_name, read_NODE, read_NODE_nodes
from .part_model import PARTFile
from .part_read import is_PART, read_PART

__all__ = ['NODEFile', 'Node', 'node_by_name', 'read_NODE', 'read_NODE_nodes',
           'PARTFile', 'is_PART', 'read_PART']
