"""Where material records, shape layouts and shader pairs come from.

    src = Sources()
    rec, pair_key, template_ref = src.material_record(mat)
    shape = src.template_shape(template_ref)
    pair = src.pair(pair_key)

First from the sidecars of the parts imported into this .blend, then
from the shader library (library_path(): the add-on's preference, or
shader_library.json in the add-on's folder; built from BUILD_OUT with
the button in the add-on's preferences or python -m
sbx_formats.library). References:

- 'part:shape:draw': a record of an imported part (a material's
  sbx_record, or a new material's sbx_template);
- 'library:<pair key>:<example>': an example in the library.

A new material's record is its template's with the material's own
values (sbx_diffuse ...) and textures written in; everything else (the
fields not worked out) stays the template's.
"""

import json
import os

import bpy

from . import properties
from .log import get_logger
from .sbx_formats import library, shaders, sidecar
from .sbx_formats.part_model import Draw

log = get_logger(__name__)

LIBRARY_NAME = 'shader_library.json'


class SourceError(Exception):
    pass


def default_library_path():
    from .log import log_folder
    return os.path.join(os.path.dirname(log_folder()), LIBRARY_NAME)


def library_path():
    """The shader library's file: the preference if set, else the default."""
    try:
        prefs = bpy.context.preferences.addons[__package__].preferences
        p = bpy.path.abspath(prefs.library_path) if prefs.library_path else ''
    except (KeyError, AttributeError):
        p = ''
    return p or default_library_path()


def sidecar_names():
    """Part names with a sidecar in this .blend."""
    out = []
    for t in bpy.data.texts:
        if t.name.endswith(properties.SIDECAR_SUFFIX) and not t.name.startswith('.'):
            out.append(t.name[:-len(properties.SIDECAR_SUFFIX)])
    return sorted(out)


def load_sidecar(part_name):
    text = bpy.data.texts.get(properties.sidecar_text_name(part_name))
    raw = properties.sidecar_raw(bpy.data.texts, part_name)
    if text is None or raw is None:
        raise SourceError('%s: its sidecar text blocks are missing (import the part again)' % part_name)
    return sidecar.join(json.loads(text.as_string()), raw)


def set_record_values(rec, mat):
    """A new material's values and textures -> its record (in place)."""
    for name in properties.NEW_MATERIAL_FIELDS:
        key = 'sbx_' + name
        if key not in mat:
            continue
        v = mat[key]
        if hasattr(v, 'to_list'):
            v = v.to_list()
        if isinstance(v, (list, tuple)):
            rec.set(name, [float(x) for x in v])
        elif name in ('power',):
            rec.set(name, float(v))
        else:
            rec.set(name, int(v))
    tex = properties.parse_textures(mat.get('sbx_textures', ''))
    count = rec.get('texture_count')
    xprs, texs = list(rec.get('texture_xprs')), list(rec.get('texture_indices'))
    slots = [j for j in range(min(count, 8)) if texs[j] >= 0]
    if tex and len(tex) != len(slots):
        raise SourceError('material %s binds %d textures, its template %d (the count can\'t change yet)'
                          % (mat.name, len(tex), len(slots)))
    for j, (x, t) in zip(slots, tex):
        xprs[j], texs[j] = int(x), int(t)
    rec.set('texture_xprs', xprs)
    rec.set('texture_indices', texs)


class Sources:
    def __init__(self):
        self.sidecars = {}
        self._library = None
        self._library_tried = False

    # ------------------------------------------------------------ pieces

    def sidecar(self, part_name):
        if part_name not in self.sidecars:
            self.sidecars[part_name] = load_sidecar(part_name)
        return self.sidecars[part_name]

    def library(self):
        if not self._library_tried:
            self._library_tried = True
            path = library_path()
            if os.path.exists(path):
                try:
                    self._library = library.load_library(path)
                    log.info('shader library %s: %d pairs', path, len(self._library['pairs']))
                except Exception as e:
                    log.warning('shader library %s not read: %s', path, e)
        return self._library

    def _split(self, ref):
        if not ref:
            raise SourceError('no template record')
        if ref.startswith('library:'):
            body = ref[len('library:'):]
            key, index = body.rsplit(':', 1)
            return 'library', key, int(index)
        part_name, shape, draw = ref.rsplit(':', 2)
        return 'part', part_name, (int(shape), int(draw))

    def record(self, ref):
        """(Draw, pair key) of a reference."""
        kind, a, b = self._split(ref)
        if kind == 'library':
            lib = self.library()
            if lib is None or a not in lib['pairs']:
                raise SourceError('%s: not in the shader library (%s)' % (ref, library_path()))
            raw, _, _ = library.library_example(lib, a, b)
            return Draw(raw), a
        sc = self.sidecar(a)
        rec = Draw(sidecar.unb64(sc['raw']['shapes'][b[0]]['draws'][b[1]]['record']))
        _, pairs, _ = shaders.split_section(sidecar.unb64(sc['raw']['shaders']))
        sh = rec.get('shader')
        if sh >= len(pairs):
            raise SourceError('%s: its shader slot %d isn\'t in its part' % (ref, sh))
        return rec, pairs[sh].vs_hash + ':' + pairs[sh].ps_hash

    def template_shape(self, ref):
        """The shape a reference's record belongs to, as a layout template."""
        kind, a, b = self._split(ref)
        if kind == 'library':
            lib = self.library()
            if lib is None or a not in lib['pairs']:
                raise SourceError('%s: not in the shader library' % ref)
            return library.library_example(lib, a, b)[1]
        return sidecar.shape_template(self.sidecar(a), b[0])

    def pair(self, key):
        """The ShaderPair with key 'vs:ps': from an imported part, else the library."""
        for name in sidecar_names():
            try:
                p = sidecar.shader_pair(self.sidecar(name), key)
            except SourceError:
                continue
            if p is not None:
                return p
        lib = self.library()
        p = library.library_pair(lib, key) if lib is not None else None
        if p is None:
            raise SourceError('shader pair %s isn\'t in any imported part or in the shader library (%s); '
                              'build the library in the add-on\'s preferences' % (key, library_path()))
        return p

    # ------------------------------------------------------------ materials

    def material_record(self, mat):
        """An SBX material -> (Draw, pair key, template reference).

        An imported material gives the record it was made from; a new
        one its template's record with its own values and textures."""
        if mat is None or not properties.is_sbx_material(mat):
            raise SourceError('%s isn\'t an SBX material (use Make SBX Material)'
                              % (mat.name if mat else 'an empty slot'))
        if 'sbx_new' in mat:
            ref = mat.get('sbx_template', '')
            rec, key = self.record(ref)
            rec = Draw(rec.raw)
            set_record_values(rec, mat)
            if mat.get('sbx_shader') and mat['sbx_shader'] != key:
                raise SourceError('material %s: its shader %s isn\'t its template\'s (%s)'
                                  % (mat.name, mat['sbx_shader'], key))
            return rec, key, ref
        ref = mat['sbx_record']
        rec, key = self.record(ref)
        return Draw(rec.raw), key, ref

    # ------------------------------------------------------------ choices

    def pair_choices(self):
        """[(key, ids, what, source)] for every pair known: imported parts' first."""
        out, seen = [], set()
        for name in sidecar_names():
            try:
                sc = self.sidecar(name)
            except SourceError:
                continue
            _, pairs, _ = shaders.split_section(sidecar.unb64(sc['raw']['shaders']))
            for p in pairs:
                key = p.vs_hash + ':' + p.ps_hash
                if key not in seen:
                    seen.add(key)
                    out.append((key, p.ids, library.describe(p), 'imported'))
        lib = self.library()
        if lib is not None:
            for key, e in lib['pairs'].items():
                if key not in seen:
                    seen.add(key)
                    out.append((key, e['ids'], e['what'], 'library'))
        return sorted(out, key=lambda t: t[1])

    def template_choices(self, key):
        """[(ref, label)] of records using pair key: imported first, then the library's."""
        out, seen = [], set()
        for name in sidecar_names():
            try:
                sc = self.sidecar(name)
            except SourceError:
                continue
            _, pairs, _ = shaders.split_section(sidecar.unb64(sc['raw']['shaders']))
            slots = [k for k, p in enumerate(pairs) if p.vs_hash + ':' + p.ps_hash == key]
            if not slots:
                continue
            for shape, rshape in zip(sc['shapes'], sc['raw']['shapes']):
                for d, rd in zip(shape['draws'], rshape['draws']):
                    rec = Draw(sidecar.unb64(rd['record']))
                    if rec.get('shader') not in slots:
                        continue
                    sig = (rec.get('shared_shader'), rec.get('texture_count'), rec.get('blend'),
                           rec.get('flags'), d.get('material'))
                    if sig in seen:
                        continue
                    seen.add(sig)
                    out.append(('%s:%d:%d' % (name, shape['shape'], d['draw']),
                                '%s (%s %s)' % (d.get('material') or 'draw', name, shape['name'])))
        lib = self.library()
        if lib is not None and key in lib['pairs']:
            for k, ex in enumerate(lib['pairs'][key]['examples']):
                shared, count, blend, flags = ex['key']
                out.append(('library:%s:%d' % (key, k),
                            'library: %d texture%s%s%s (from %s)' % (
                                count, '' if count == 1 else 's',
                                ', shared %d' % shared if shared else '', ', blend' if blend else '',
                                ex['part'])))
        return out
