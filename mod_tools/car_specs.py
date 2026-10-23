#!/usr/bin/env python3
"""Export the car specs (BUILD 72, 00000072.bin) to an Excel workbook and
build them back.

    python car_specs.py export 00000072.bin car_specs.xlsx
    python car_specs.py build car_specs.xlsx 00000072.bin

The workbook has an Index sheet (a summary of every car; for reading only)
and one sheet per car, "NN ID" (NN = its index in the file), with a row per
field: offset in the car's record, block, field, type, value, bytes and
notes. Bytes is a formula, so it follows the value as it's edited, and each
engine stage has a torque and power chart next to it (both use hidden helper
columns from Z). Only the Value column is read back, by the row's Offset and Type, so
rows can be sorted or annotated without breaking the build. Fields whose
meaning isn't known yet are named UNLABELED; their type (f32, i32 or u32) is
a guess from their values across all cars.

The layout is in docs/game_data.md ("72: car specs"). Needs openpyxl
(pip install openpyxl).
"""

import argparse
import struct
import sys

from openpyxl.utils import get_column_letter

CAR_SIZE = 8432
CAR_COUNT = 72

# Rival and traffic names, for the Index sheet (docs/game_data.md).
NICKNAMES = {
    'CON_X': "Skull Bullet (the player's)",
    'BOSS00': 'Devil Z', 'BOSS01': 'Snake Eyes', 'BOSS02': 'Emperor',
    'BOSS03': 'Crimson Devil', 'BOSS04': 'White Charisma',
    'BOSS05': 'Melancholy Angel', 'BOSS06': 'The Rook', 'BOSS07': 'The Bishop',
    'BOSS08': 'The Knight', 'BOSS09': 'Blood Hound',
    'BOSS10': 'Midnight Cinderella', 'BOSS11': 'Silvery Nobleman',
    'BOSS20': 'Skull Bullet', 'LEAD01': 'Rolling Guy No. 1',
    'LEAD02': 'Rolling Gal No. 1', 'LEAD04': 'White Fairy',
    'LEAD06': "Naniwa's Warship", 'WON01': 'Death Driver', 'WON02': 'Matterhorn',
    'WON03': 'Rolling Master', 'WON04': 'Gold License', 'WON06': 'Stalking Hammer',
}

DRIVE_TYPES = {0: 'FF', 1: 'FR', 4: '4WD'}
ENGINE_TYPES = {1: 'L4', 2: 'L6', 3: 'H4', 5: 'V6', 6: 'V8', 7: 'Rotary'}
ASPIRATION = {0: 'NA', 1: 'Turbo'}

SIZES = {'u8': 1, 'u16': 2, 'u32': 4, 'i32': 4, 'f32': 4}
FORMATS = {'u8': '<B', 'u16': '<H', 'u32': '<I', 'i32': '<i', 'f32': '<f'}


class Field:
    def __init__(self, offset, size, type_, block, name, note=''):
        self.offset, self.size, self.type = offset, size, type_
        self.block, self.name, self.note = block, name, note


def tire_block(i):
    """Block name of tire i (0-41): 21 front, then 21 rear, each the stock
    tire and then 4 types x 5 grades (CarSpec::GetTire)."""
    side, slot = ('Front' if i < 21 else 'Rear'), i % 21
    if slot == 0:
        return f'{side} tire stock'
    return f'{side} tire type {(slot - 1) // 5 + 1} grade {(slot - 1) % 5 + 1}'


def color(f, offset, block, name, note=''):
    """A paint-style color: u16 hue, u8 saturation, u8 value."""
    f.append(Field(offset, 2, 'u16', block, f'{name} hue', note or '0-359'))
    f.append(Field(offset + 2, 1, 'u8', block, f'{name} saturation', '0-255'))
    f.append(Field(offset + 3, 1, 'u8', block, f'{name} value', '0-255 (brightness)'))


def known_fields():
    """The labeled fields, by offset in a car's record (docs/game_data.md)."""
    f = []

    def car_id(offset, block):
        f.append(Field(offset, 8, 'text', block, 'Car id',
                       'Repeated at the start of every block'))

    # Block 1: the physics values (the vehicle setup reads these).
    car_id(0, 'Block 1')
    for offset, name in ((0x20, 'Length (m)'), (0x24, 'Height (m)'), (0x28, 'Width (m)')):
        f.append(Field(offset, 4, 'f32', 'Block 1', name))
    for offset, name in ((0x40, 'Box min x (m)'), (0x44, 'Box min y (m)'), (0x48, 'Box min z (m)')):
        f.append(Field(offset, 4, 'f32', 'Block 1', name,
                       'Probably the collision box: centered, 0 to the height, the '
                       "car's width, 0.10-0.16 m shorter than its length"))
    for offset, name in ((0x50, 'Box max x (m)'), (0x54, 'Box max y (m)'), (0x58, 'Box max z (m)')):
        f.append(Field(offset, 4, 'f32', 'Block 1', name))
    f.append(Field(0x174, 4, 'f32', 'Block 1', 'Final drive'))
    f.append(Field(0x180, 4, 'f32', 'Block 1', 'Reverse gear ratio', 'Negative'))
    for i in range(6):
        f.append(Field(0x184 + 4 * i, 4, 'f32', 'Block 1', f'Gear {i + 1} ratio',
                       'Forward gears; 0 after the last' if i == 0 else ''))
    f.append(Field(532, 4, 'f32', 'Block 1', 'Mass (kg)',
                   'What the physics uses; 7-10% below the spec sheet\'s'))
    f.append(Field(536, 4, 'f32', 'Block 1', 'Wheelbase (m)'))
    f.append(Field(540, 4, 'f32', 'Block 1', 'Front track (m)'))
    f.append(Field(544, 4, 'f32', 'Block 1', 'Rear track (m)'))

    # Block 2: the spec sheet the menus show, and the car's lists.
    b2 = 640
    car_id(b2, 'Block 2')
    f.append(Field(b2 + 8, 4, 'u32', 'Block 2', 'Displacement (cc)'))
    f.append(Field(b2 + 12, 4, 'f32', 'Block 2', 'Max power (PS)'))
    f.append(Field(b2 + 16, 4, 'f32', 'Block 2', 'Max power rpm'))
    f.append(Field(b2 + 20, 4, 'f32', 'Block 2', 'Max torque (kgm)'))
    f.append(Field(b2 + 24, 4, 'f32', 'Block 2', 'Max torque rpm'))
    f.append(Field(b2 + 68, 1, 'u8', 'Block 2', 'Engine type',
                   '1 L4, 2 L6, 3 H4, 5 V6, 6 V8, 7 rotary; the tuning uses it as the '
                   'cylinder count (2 for the rotary)'))
    f.append(Field(b2 + 69, 1, 'u8', 'Block 2', 'Aspiration', '0 NA, 1 turbo'))
    for i in (70, 71):
        f.append(Field(b2 + i, 1, 'u8', 'Block 2', 'UNLABELED'))
    f.append(Field(b2 + 72, 4, 'u32', 'Block 2', 'Mass (kg)', 'Shown in menus'))
    f.append(Field(b2 + 76, 4, 'u32', 'Block 2', 'Length (mm)', 'Shown in menus'))
    f.append(Field(b2 + 80, 4, 'u32', 'Block 2', 'Height (mm)', 'Shown in menus'))
    f.append(Field(b2 + 84, 4, 'u32', 'Block 2', 'Width (mm)', 'Shown in menus'))
    f.append(Field(b2 + 88, 4, 'u32', 'Block 2', 'Model year'))
    f.append(Field(b2 + 92, 4, 'u32', 'Block 2', 'Doors'))
    f.append(Field(b2 + 96, 4, 'u32', 'Block 2', 'UNLABELED',
                   '0 or 1 for road cars and rivals, 0-3 for traffic'))
    f.append(Field(b2 + 100, 4, 'u32', 'Block 2', 'Gears',
                   'Matches the ratios in block 1 in 68 of 72 cars'))
    f.append(Field(b2 + 104, 4, 'u32', 'Block 2', 'Price', '999999 = not sold'))
    f.append(Field(b2 + 128, 1, 'u8', 'Block 2', 'UNLABELED'))
    f.append(Field(b2 + 129, 1, 'u8', 'Block 2', 'Maker',
                   '3 Mazda, 4 Mitsubishi, 5 Nissan, 6 Subaru, 8 Toyota, '
                   '0 rivals and traffic'))
    f.append(Field(b2 + 130, 1, 'u8', 'Block 2', 'Drive type', '0 FF, 1 FR, 4 4WD'))
    f.append(Field(b2 + 131, 1, 'u8', 'Block 2', 'Car list',
                   '0 road cars, 1 CON_X and rivals (the two lists the starter '
                   'cars and shop show), 2 traffic'))
    f.append(Field(b2 + 316, 24, 'text', 'Block 2', 'Model name',
                   'Only some traffic cars have one; the rest "-" or spaces'))

    # Engine stages: the 10 engine options, then turbo kits (real only on
    # some NA cars), then a stage that's the same in every car.
    for i in range(16):
        stage = 980 + 204 * i
        block = f'Engine {i + 1:02}'
        car_id(stage, block)
        f.append(Field(stage + 12, 4, 'f32', block, 'Curve end (rpm)', '32 points x 400 rpm'))
        f.append(Field(stage + 20, 4, 'f32', block, 'Rev limit (rpm)'))
        f.append(Field(stage + 24, 4, 'f32', block, 'Torque',
                       'kgm x 10; scales the torque curve'))
        for k in range(32):
            f.append(Field(stage + 28 + 4 * k, 4, 'f32', block, f'Torque curve {k + 1:02}',
                           f'{(k + 1) * 400} rpm; relative to Torque, peaks at 1'))
        f.append(Field(stage + 168, 4, 'f32', block, 'UNLABELED',
                       'A fixed fraction of Torque within each car'))
        f.append(Field(stage + 172, 4, 'u32', block, 'Turbo', '1 turbo, 0 NA'))
        f.append(Field(stage + 176, 4, 'f32', block, 'UNLABELED',
                       'Turbo setting; on turbo stages this x the next = 3.75'))
        f.append(Field(stage + 180, 4, 'f32', block, 'UNLABELED', 'Turbo setting'))
        f.append(Field(stage + 184, 4, 'f32', block, 'UNLABELED', 'Turbo setting'))
        f.append(Field(stage + 200, 1, 'u8', block, 'UNLABELED', '1 on NA stages, 2 on turbo'))
        f.append(Field(stage + 201, 1, 'u8', block, 'UNLABELED',
                       'Changes where the rev limit does; probably the engine sound'))
        for k in (202, 203):
            f.append(Field(stage + k, 1, 'u8', block, 'UNLABELED'))

    for i in range(42):
        tire = 4244 + 92 * i
        block = tire_block(i)
        car_id(tire, block)
        f.append(Field(tire + 44, 4, 'f32', block, 'Radius (m)',
                       'Rim diameter / 2 + width x aspect ratio'))
        f.append(Field(tire + 48, 4, 'f32', block, 'Width (m)'))
        f.append(Field(tire + 64, 4, 'f32', block, 'Grip',
                       'Stock x a factor per type and grade; road cars\' stock is '
                       '0.525 x (1 + 3.67 x sqrt(radius x width))'))
        f.append(Field(tire + 68, 4, 'f32', block, 'Grip (again)', 'Same as Grip in every tire'))
        f.append(Field(tire + 80, 4, 'f32', block, 'Rim diameter (in)'))
        f.append(Field(tire + 84, 4, 'f32', block, 'Aspect ratio'))
        f.append(Field(tire + 88, 4, 'f32', block, 'Rim width (in)'))

    paint = 8108
    car_id(paint, 'Paint')
    for i in range(13):
        slot = paint + 8 + 20 * i
        block = f'Paint {i + 1:02}'
        f.append(Field(slot, 2, 'u16', block, 'Type',
                       '0 solid, 1 metallic, 2 rival paint (keeps its own shade)'))
        f.append(Field(slot + 2, 2, 'u16', block, 'UNLABELED', '0 in all cars'))
        color(f, slot + 4, block, 'Color')
        color(f, slot + 8, block, 'Second color', 'Only the SUPRA70 and a WON03 slot use it')
        color(f, slot + 12, block, 'Shade',
              'The game sets it for solid and metallic paints; type 2 uses this one')
        color(f, slot + 16, block, 'Fourth color', '0 in all cars')
    f.append(Field(paint + 8 + 20 * 13, 4, 'u32', 'Paint', 'Paint color count'))

    # The stock wheel, in table 73's format (the aftermarket wheels).
    wheel = 8380
    car_id(wheel, 'Stock wheel')
    for part in (8, 20):
        n = 'A' if part == 8 else 'B'
        color(f, wheel + part, 'Stock wheel', f'UNLABELED {n}1', 'Swapped like a paint color')
        color(f, wheel + part + 4, 'Stock wheel', f'UNLABELED {n}2', 'Swapped like a paint color')
    f.append(Field(wheel + 48, 4, 'text', 'Stock wheel', 'UNLABELED', '"0000" in all cars'))
    return sorted(f, key=lambda x: x.offset)


BLOCKS = [(0, 'Block 1'), (640, 'Block 2')] + \
    [(980 + 204 * i, f'Engine {i + 1:02}') for i in range(16)] + \
    [(4244 + 92 * i, tire_block(i)) for i in range(42)] + \
    [(8108, 'Paint')] + [(8116 + 20 * i, f'Paint {i + 1:02}') for i in range(13)] + \
    [(8376, 'Paint'), (8380, 'Stock wheel')]


def block_at(offset):
    name = BLOCKS[0][1]
    for start, block in BLOCKS:
        if start <= offset:
            name = block
    return name


def guess_type(data, offset):
    """f32, i32 or u32 for an unlabeled word, from its values in every car."""
    words = [struct.unpack_from('<I', data, c * CAR_SIZE + offset)[0]
             for c in range(CAR_COUNT)]
    if any(w & 0x7FFFFFFF for w in words):
        floats = [struct.unpack('<f', struct.pack('<I', w))[0] for w in words]
        if all(w & 0x7FFFFFFF == 0 or 1e-5 <= abs(x) <= 1e7
               for w, x in zip(words, floats)):
            return 'f32'
    if any(w & 0x80000000 for w in words) and \
            all(w < 0x10000 or w >= 0xFFFF0000 for w in words):
        return 'i32'
    return 'u32'


def layout(data):
    """Every field of a car's record, labeled or not, in offset order."""
    fields, pos = [], 0
    for k in known_fields() + [Field(CAR_SIZE, 0, '', '', '')]:
        assert k.offset >= pos, (k.offset, pos)
        assert pos == k.offset or (pos % 4 == 0 and k.offset % 4 == 0), (k.offset, pos)
        while pos < k.offset:  # Unlabeled words in the gap.
            fields.append(Field(pos, 4, guess_type(data, pos), block_at(pos), 'UNLABELED'))
            pos += 4
        if k.size:
            fields.append(k)
            pos = k.offset + k.size
    assert pos == CAR_SIZE
    return fields


def short_float(raw):
    """The shortest decimal that packs back to the same f32 bits."""
    value = struct.unpack('<f', raw)[0]
    for digits in range(1, 10):
        text = f'{value:.{digits}g}'
        if struct.pack('<f', float(text)) == raw:
            return float(text)
    return value


def decode(field, raw):
    if field.type == 'text':
        text = raw.split(b'\0', 1)[0]
        if text + b'\0' * (len(raw) - len(text)) != raw or \
                any(c < 0x20 or c > 0x7E for c in text):
            raise ValueError(f'+{field.offset:#x}: not NUL-padded ASCII: {raw!r}')
        return text.decode('ascii')
    if field.type == 'f32':
        if raw == b'\0\0\0\x80':
            return '-0'  # Spreadsheets store -0.0 as 0; text keeps the sign.
        return short_float(raw)
    return struct.unpack(FORMATS[field.type], raw)[0]


def encode(field, value, where):
    if field.type == 'text':
        text = ('' if value is None else str(value)).encode('ascii')
        if len(text) > field.size:
            raise ValueError(f'{where}: "{value}" is longer than {field.size} characters')
        return text + b'\0' * (field.size - len(text))
    if value is None or value == '':
        raise ValueError(f'{where}: no value')
    if field.type == 'f32':
        return struct.pack('<f', float(value))
    number = float(value)
    if number != int(number):
        raise ValueError(f'{where}: {value} isn\'t a whole number')
    try:
        return struct.pack(FORMATS[field.type], int(number))
    except struct.error:
        raise ValueError(f'{where}: {value} doesn\'t fit in {field.type}') from None


def le_bytes(number, size):
    """Formula: `number` (a formula for an unsigned integer) as `size` hex bytes,
    little-endian, e.g. "D5 07 00 00"."""
    return '&" "&'.join(f'DEC2HEX(MOD(INT(({number})/{256 ** k}),256),2)' for k in range(size))


# Hidden helper columns, right of the charts (column H on): for the f32
# formulas, the exponent (rough, then exact), rounded mantissa and the 32 bits
# as an unsigned integer; for the torque curve charts, each point's rpm,
# torque (kgm) and power (PS).
HELPER_COLUMN = 26  # Z
HELPERS = ['_exp0', '_exp', '_mant', '_bits', '_rpm', '_kgm', '_ps']
EXP0, EXP, MANT, BITS, RPM, KGM, PS = (
    get_column_letter(HELPER_COLUMN + i) for i in range(len(HELPERS)))


def bytes_formulas(field, row):
    """The Bytes cell's formula for a row, and its helper formulas (f32 only),
    so the Bytes column follows the Value column as it's edited."""
    v = f'E{row}'
    if field.type == 'text':
        parts = [f'IF(LEN({v})>={i},DEC2HEX(CODE(MID({v},{i},1)),2),"00")'
                 for i in range(1, field.size + 1)]
        return '=' + '&" "&'.join(parts), None
    if field.type != 'f32':
        # MOD makes a negative i32 its two's complement.
        return '=' + le_bytes(f'MOD({v},{256 ** field.size})', field.size), None
    exp0, exp, mant, bits = (f'{c}{row}' for c in (EXP0, EXP, MANT, BITS))
    helpers = [
        f'=IF({v}="-0","",IF({v}=0,"",INT(LOG(ABS({v}),2))))',
        f'=IF({exp0}="","",{exp0}+IF(ABS({v})/2^{exp0}>=2,1,0)-IF(ABS({v})/2^{exp0}<1,1,0))',
        f'=IF({exp}="","",ROUND((ABS({v})/2^{exp}-1)*8388608,0))',
        f'=IF({v}="-0",2147483648,IF({v}=0,0,IF({v}<0,2147483648,0)'
        f'+({exp}+127+IF({mant}=8388608,1,0))*8388608+MOD({mant},8388608)))',
    ]
    return '=' + le_bytes(bits, 4), helpers


def add_torque_chart(sheet, rows, block):
    """Torque (kgm, red) and power (PS, blue) of an engine stage against rpm,
    like the game's graph, next to its rows. Built from formulas, so it
    follows edits; points above the rev limit are left out."""
    from openpyxl.chart import LineChart, Reference, Series
    from openpyxl.chart.layout import Layout, ManualLayout

    rev, torque = rows[block, 'Rev limit (rpm)'], rows[block, 'Torque']
    first = rows[block, 'Torque curve 01']
    last = rows[block, 'Torque curve 32']
    for k, row in enumerate(range(first, last + 1)):
        sheet[f'{RPM}{row}'] = (k + 1) * 400
        sheet[f'{KGM}{row}'] = f'=IF({RPM}{row}>$E${rev},NA(),$E${torque}/10*E{row})'
        sheet[f'{PS}{row}'] = f'=IF({RPM}{row}>$E${rev},NA(),{KGM}{row}*{RPM}{row}/716.2)'

    def series(column, title, color):
        col = HELPER_COLUMN + HELPERS.index(column)
        line = Series(Reference(sheet, min_col=col, min_row=first, max_row=last), title=title)
        line.graphicalProperties.line.solidFill = color
        line.graphicalProperties.line.width = 22000
        line.marker.symbol = 'none'
        line.smooth = False
        return line

    chart = LineChart()
    chart.title = f'{block}: torque and power'
    chart.series.append(series('_kgm', 'Torque (kgm)', 'C00000'))
    chart.set_categories(Reference(sheet, min_col=HELPER_COLUMN + HELPERS.index('_rpm'),
                                   min_row=first, max_row=last))
    chart.x_axis.title = 'rpm'
    chart.y_axis.title = 'Torque (kgm)'
    power = LineChart()
    power.series.append(series('_ps', 'Power (PS)', '0070C0'))
    power.y_axis.axId = 200
    power.y_axis.title = 'Power (PS)'
    power.y_axis.crosses = 'max'
    power.y_axis.majorGridlines = None  # The torque axis's gridlines are enough.
    for axis in (chart.x_axis, chart.y_axis, power.y_axis):
        axis.delete = False
    chart.x_axis.tickLblSkip = 4
    chart += power
    chart.visible_cells_only = False  # The helper columns are hidden.
    chart.legend.position = 'b'
    chart.width, chart.height = 30, 20
    chart.layout = Layout(
        ManualLayout(
        x=0.125, y=0.125,
        h=0.75, w=0.75,
        xMode="edge",
        yMode="edge",
        )
    )
    # The plot area inside the chart, as fractions of it: left room for the
    # title, legend and axis titles, which Excel otherwise draws over it.
    sheet.add_chart(chart, f'H{rows[block, "Car id"]}')

def export(bin_path, xlsx_path):
    from openpyxl import Workbook
    from openpyxl.styles import Alignment, Font, PatternFill

    data = open(bin_path, 'rb').read()
    if len(data) != CAR_SIZE * CAR_COUNT:
        sys.exit(f'{bin_path}: {len(data)} bytes, expected {CAR_SIZE * CAR_COUNT}')
    fields = layout(data)

    wb = Workbook()
    index = wb.active
    index.title = 'Index'
    bold = Font(bold=True)
    gray = Font(color='808080')
    left = Alignment(horizontal='left')
    # Alternating shades, one per block, so the blocks are easy to tell apart.
    shades = [PatternFill('solid', fgColor='DDEBF7'), PatternFill('solid', fgColor='FFFFFF')]
    summary = ['Car', 'Sheet', 'Name', 'Model year', 'Mass (kg)', 'Physics mass (kg)',
               'Wheelbase (m)', 'Displacement (cc)',
               'Max power (PS)', 'Max torque (kgm)', 'Drive type', 'Engine type',
               'Aspiration', 'Doors', 'Gears', 'Price', 'Car list',
               'Paint color count']
    index.append(summary)
    for cell in index[1]:
        cell.font = bold
    index.append(['For reading only: edit a car on its own sheet.'])
    index['A2'].font = gray

    for car in range(CAR_COUNT):
        record = data[car * CAR_SIZE:(car + 1) * CAR_SIZE]
        car_id = record[:8].decode('ascii').strip()
        sheet = wb.create_sheet(f'{car:02} {car_id}')
        gap = [None] * (HELPER_COLUMN - 8)
        sheet.append(['Offset', 'Block', 'Field', 'Type', 'Value', 'Bytes', 'Notes']
                     + gap + HELPERS)
        for cell in sheet[1]:
            cell.font = bold
            cell.alignment = left
        values = {}
        rows = {}  # (block, field) -> row, for the charts
        shade, last_block = 1, None
        row = 1
        for field in fields:
            raw = record[field.offset:field.offset + field.size]
            value = decode(field, raw)
            values[field.name, field.block] = value
            row += 1
            rows[field.block, field.name] = row
            formula, helpers = bytes_formulas(field, row)
            sheet.append([f'0x{field.offset:04X}', field.block, field.name, field.type,
                          value, formula, field.note] + gap + (helpers or [None] * 4))
            if field.block != last_block:
                shade, last_block = 1 - shade, field.block
            for column in range(1, 8):
                cell = sheet.cell(row, column)
                cell.fill = shades[shade]
                cell.alignment = left
                if field.name.startswith('UNLABELED'):
                    cell.font = gray
        sheet.freeze_panes = 'A2'
        for column, width in zip('ABCDEFG', (9, 15, 20, 6, 26, 14, 50)):
            sheet.column_dimensions[column].width = width
        for i in range(len(HELPERS)):
            sheet.column_dimensions[get_column_letter(HELPER_COLUMN + i)].hidden = True
        for stage in range(1, 17):
            add_torque_chart(sheet, rows, f'Engine {stage:02}')

        b2 = {name: value for (name, block), value in values.items() if block == 'Block 2'}
        index.append([car_id, sheet.title, NICKNAMES.get(car_id, ''), b2['Model year'],
                      b2['Mass (kg)'], values['Mass (kg)', 'Block 1'],
                      values['Wheelbase (m)', 'Block 1'], b2['Displacement (cc)'],
                      b2['Max power (PS)'],
                      b2['Max torque (kgm)'],
                      DRIVE_TYPES.get(b2['Drive type'], b2['Drive type']),
                      ENGINE_TYPES.get(b2['Engine type'], b2['Engine type']),
                      ASPIRATION.get(b2['Aspiration'], b2['Aspiration']),
                      b2['Doors'], b2['Gears'], b2['Price'], b2['Car list'],
                      values['Paint color count', 'Paint']])
    for row in index.iter_rows():
        for cell in row:
            cell.alignment = left
    index.freeze_panes = 'A2'
    for column, width in zip('ABCDEFGHIJKLMNOPQR',
                             (10, 12, 26, 11, 10, 17, 14, 17, 15, 17, 11, 12, 11, 7, 15, 9, 9,
                              17)):
        index.column_dimensions[column].width = width
    wb.save(xlsx_path)
    print(f'{xlsx_path}: {CAR_COUNT} cars, {len(fields)} fields each')


def build(xlsx_path, bin_path):
    from openpyxl import load_workbook

    wb = load_workbook(xlsx_path, data_only=True)
    sheets = {}
    for sheet in wb.worksheets:
        number = sheet.title.split(' ', 1)[0]
        if number.isdigit():
            sheets[int(number)] = sheet
    missing = [c for c in range(CAR_COUNT) if c not in sheets]
    if missing:
        sys.exit(f'{xlsx_path}: no sheet for car(s) {missing}')

    out = bytearray(CAR_SIZE * CAR_COUNT)
    for car in range(CAR_COUNT):
        sheet = sheets[car]
        rows = sheet.iter_rows(values_only=True)
        header = [str(h).strip() if h is not None else '' for h in next(rows)]
        col = {name: header.index(name) for name in ('Offset', 'Type', 'Value')}
        covered = bytearray(CAR_SIZE)
        for number, row in enumerate(rows, 2):
            if row[col['Offset']] in (None, ''):
                continue
            where = f'{sheet.title}, row {number}'
            offset = int(str(row[col['Offset']]), 0)
            type_ = str(row[col['Type']]).strip()
            value = row[col['Value']]
            if type_ == 'text':
                # The text field's size is up to the next row's offset; find it
                # from the known layout instead.
                size = next(k.size for k in known_fields() if k.offset == offset)
            elif type_ in SIZES:
                size = SIZES[type_]
            else:
                sys.exit(f'{where}: unknown type "{type_}"')
            if offset + size > CAR_SIZE or any(covered[offset:offset + size]):
                sys.exit(f'{where}: offset 0x{offset:04X} overlaps another row')
            try:
                raw = encode(Field(offset, size, type_, '', ''), value, where)
            except ValueError as e:
                sys.exit(str(e))
            start = car * CAR_SIZE + offset
            out[start:start + size] = raw
            covered[offset:offset + size] = b'\1' * size
        if not all(covered):
            gap = covered.index(0)
            sys.exit(f'{sheet.title}: no row covers offset 0x{gap:04X}')
    open(bin_path, 'wb').write(out)
    print(f'{bin_path}: {len(out)} bytes')


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    sub = parser.add_subparsers(dest='command', required=True)
    p = sub.add_parser('export', help='00000072.bin -> .xlsx')
    p.add_argument('bin')
    p.add_argument('xlsx')
    p = sub.add_parser('build', help='.xlsx -> 00000072.bin')
    p.add_argument('xlsx')
    p.add_argument('bin')
    args = parser.parse_args()
    if args.command == 'export':
        export(args.bin, args.xlsx)
    else:
        build(args.xlsx, args.bin)


if __name__ == '__main__':
    main()
