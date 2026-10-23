#!/usr/bin/env python3
"""List every car's nodes and parts, and every wheel, in a workbook.

    python car_models.py BUILD_OUT -o car_models.xlsx

Sheets:
- Cars: the 45 car blocks: code, name, node files and body XPR2s of sets
  a, b and c, how many parts each set has, the stock wheel.
- Wheels: the 100 wheels (rim, brake, XPR2 ids and what to type in the
  Blender import's Wheels field), the two tire models and the other
  wheel-area parts.
- One sheet per car: for each set, the node tree (indented by depth,
  with positions in the node frame and in car space) and the parts, slot
  by slot (part id, what it attaches to, its shapes and markers).

Needs openpyxl. The ids, names and slot groups come from
sbx_formats/cars.py; docs/game_data.md, "Car models".
"""

import argparse
import os
import sys

import numpy as np
from openpyxl import Workbook
from openpyxl.styles import Alignment, Font, PatternFill
from openpyxl.utils import get_column_letter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from sbx_formats import cars, read_NODE_nodes, read_PART  # noqa: E402

BOLD = Font(bold=True)
HEAD = PatternFill('solid', fgColor='DDE4EE')
TITLE = Font(bold=True, size=13)

# What the positioned nodes are (docs/game_data.md, "Node files").
NODE_ROLES = [('tire_', 'wheel center'), ('seat_', 'seat'), ('acce_null', 'accessory (charm)'),
              ('dl', 'door hinge'), ('dr', 'door hinge'), ('bn_', 'bonnet hinge'), ('sm_', 'mirror')]


def role(name):
    for prefix, what in NODE_ROLES:
        if name.startswith(prefix):
            return what
    return ''


def header(ws, row, names, widths=None):
    for c, n in enumerate(names, 1):
        cell = ws.cell(row=row, column=c, value=n)
        cell.font = BOLD
        cell.fill = HEAD
    return row + 1


def part_summary(path):
    p = read_PART(open(path, 'rb').read())
    attach = next((n.name for n in p.nodes if n.is_shape_node), '')
    shapes = ', '.join(s.name for s in p.shapes)
    markers = ', '.join(n.name for n in p.nodes if not n.is_shape_node)
    return attach, shapes, markers, p


def car_sheet(wb, build_out, k):
    code, name = cars.CAR_BLOCKS[k]
    ws = wb.create_sheet('%02d %s' % (k, code)[:31])
    ws.cell(row=1, column=1, value='%s: %s' % (code, name)).font = TITLE
    ws.cell(row=2, column=1, value='Car block %d; its node files\' root node is %s_<set>_tr' % (k, code.lower()))
    if k < 20:
        rim, brake = cars.wheel_parts(cars.WHEELS[k])
        ws.cell(row=3, column=1, value='Stock wheel: %s (rim %d, brake %d); Blender Wheels field: %s'
                % (cars.WHEELS[k], rim, brake, cars.WHEELS[k]))
    row = 5
    for set_name, (first, stride, _) in cars.CAR_SETS.items():
        node_id = first + stride * k + 2
        node_path = os.path.join(build_out, '%08d.node' % node_id)
        ws.cell(row=row, column=1, value='Set %s: node file %08d, body XPR2 %08d' % (
            set_name, node_id, node_id - 2)).font = TITLE
        row += 1
        if not os.path.exists(node_path):
            ws.cell(row=row, column=1, value='The node file isn\'t in the dump (BUILD_OUT); its parts are listed '
                                             'without the node tree.')
            row += 2
            nodes = []
        else:
            nodes = read_NODE_nodes(open(node_path, 'rb').read())
            ws.cell(row=row, column=1, value='Node tree').font = BOLD
            row += 1
            row = header(ws, row, ['#', 'Node', 'Parent', 'x (dm)', 'y (dm)', 'z (dm)',
                                   'Car x (m)', 'Car y (m)', 'Car z (m)', 'What'])
            for i, n in enumerate(nodes):
                car = np.asarray(n.world[3, :3]) @ cars.NODE_TO_CAR
                values = [i, '  ' * n.depth + n.name, nodes[n.parent].name if 0 <= n.parent < len(nodes) else '',
                          *[round(float(x), 4) for x in n.translation], *[round(float(x), 4) for x in car],
                          role(n.name) if np.abs(n.translation).max() > 1e-6 else '']
                for c, v in enumerate(values, 1):
                    ws.cell(row=row, column=c, value=v)
                row += 1
            row += 1
        ws.cell(row=row, column=1, value='Parts').font = BOLD
        row += 1
        row = header(ws, row, ['Slot group', 'Slot', 'Part', 'Attaches to', 'Shapes', 'Markers'])
        for group in cars.CAR_SLOT_GROUPS[set_name]:
            gname, lo, hi = group[:3]
            for slot in range(lo, hi + 1):
                pid = node_id + slot
                path = os.path.join(build_out, '%08d.part' % pid)
                if not os.path.exists(path) or os.path.getsize(path) == 0:
                    continue
                attach, shapes, markers, _ = part_summary(path)
                for c, v in enumerate([gname + (' (optional)' if len(group) > 3 else ''), '+%d' % slot,
                                       '%08d' % pid, attach, shapes, markers], 1):
                    ws.cell(row=row, column=c, value=v)
                row += 1
        row += 2
    widths = [16, 26, 14, 14, 60, 40, 10, 10, 10, 18]
    for c, w in enumerate(widths, 1):
        ws.column_dimensions[get_column_letter(c)].width = w
    for r in ws.iter_rows(min_row=1, max_row=ws.max_row):
        for cell in r:
            cell.alignment = Alignment(vertical='top', wrap_text=cell.column == 5)


def cars_sheet(wb, build_out):
    ws = wb.active
    ws.title = 'Cars'
    ws.cell(row=1, column=1, value='Car blocks').font = TITLE
    ws.cell(row=2, column=1, value='One sheet per car follows. Node file = the file to pick in Blender\'s '
                                   'File > Import > SBX Car. "missing": not in the dump.')
    row = header(ws, 4, ['Block', 'Code', 'Car', 'Set a node', 'Set b node', 'Set c node',
                         'Parts a', 'Parts b', 'Parts c', 'Stock wheel'])
    for k, (code, name) in enumerate(cars.CAR_BLOCKS):
        values = [k, code, name]
        counts = []
        for set_name, (first, stride, _) in cars.CAR_SETS.items():
            node_id = first + stride * k + 2
            exists = os.path.exists(os.path.join(build_out, '%08d.node' % node_id))
            values.append('%08d' % node_id + ('' if exists else ' (missing)'))
            counts.append(len(cars.car_part_ids(build_out, node_id, set_name)))
        values += counts + [cars.WHEELS[k] if k < 20 else '']
        for c, v in enumerate(values, 1):
            ws.cell(row=row, column=c, value=v)
        row += 1
    for c, w in enumerate([7, 10, 32, 18, 18, 18, 8, 8, 8, 12], 1):
        ws.column_dimensions[get_column_letter(c)].width = w
    ws.freeze_panes = 'A5'


def wheels_sheet(wb, build_out):
    ws = wb.create_sheet('Wheels')
    ws.cell(row=1, column=1, value='Wheels').font = TITLE
    ws.cell(row=2, column=1, value='Each wheel is three ids: its XPR2 (textures), its rim and wheel face, its '
                                   'brake and hub. In Blender\'s SBX Car import, type the name in Wheels '
                                   '(e.g. "AD03"), optionally with a tire ("AD03 583"); 581 is used without one.')
    row = header(ws, 4, ['#', 'Name', 'Kind', 'XPR2', 'Rim', 'Brake', 'Wheels field', 'Rim shapes', 'Brake shapes'])
    for i, name in enumerate(cars.WHEELS):
        rim, brake = cars.wheel_parts(name)
        kind = ('stock wheel of %s' % cars.CAR_BLOCKS[i][1] if i < 20 else
                'traffic' if i < 36 else 'aftermarket (table 73)')
        _, rim_shapes, _, _ = part_summary(os.path.join(build_out, '%08d.part' % rim))
        _, brake_shapes, _, _ = part_summary(os.path.join(build_out, '%08d.part' % brake))
        for c, v in enumerate([i, name, kind, '%08d' % (rim - 1), '%08d' % rim, '%08d' % brake, name,
                               rim_shapes, brake_shapes], 1):
            ws.cell(row=row, column=c, value=v)
        row += 1
    row += 1
    ws.cell(row=row, column=1, value='Tires and other wheel-area parts').font = TITLE
    row = header(ws, row + 1, ['', 'Name', 'What', '', 'Part', '', 'Wheels field', 'Shapes'])
    others = [(581, 'tire_00', 'tire model, with 5 morph targets (shape keys); 0.61 m across, 0.255 m wide', '581'),
              (583, 'tire_01', 'tire model, the same size as 581', '583'),
              (886, 'brake', 'brake calipers (c_normal, c_tune; _09 and _18)', ''),
              (888, 'wheel', 'a 6 m wheel (not a car part)', '')]
    for pid, name, what, field in others:
        _, shapes, _, _ = part_summary(os.path.join(build_out, '%08d.part' % pid))
        for c, v in enumerate(['', name, what, '', '%08d' % pid, '', field, shapes], 1):
            ws.cell(row=row, column=c, value=v)
        row += 1
    for c, w in enumerate([5, 10, 44, 11, 11, 11, 13, 60, 60], 1):
        ws.column_dimensions[get_column_letter(c)].width = w
    ws.freeze_panes = 'A5'


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('build_out', help='the dumped BUILD folder (BUILD_OUT)')
    ap.add_argument('-o', '--output', default='car_models.xlsx')
    args = ap.parse_args(argv)
    wb = Workbook()
    cars_sheet(wb, args.build_out)
    wheels_sheet(wb, args.build_out)
    for k in range(len(cars.CAR_BLOCKS)):
        car_sheet(wb, args.build_out, k)
    wb.save(args.output)
    print('%s: %d sheets' % (args.output, len(wb.sheetnames)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
