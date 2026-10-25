"""Operators for changing what a part holds (the SBX panel's buttons).

The work is in edit_part.py; these only collect the user's choices.
"""

import os

import bpy
from bpy.props import EnumProperty, StringProperty

from . import edit_part, part_tree, properties
from .log import get_logger
from .sources import SourceError, Sources, library_path

log = get_logger(__name__)

# Blender keeps only references to enum item strings: they're kept here.
_items = {}


def _keep(key, items):
    _items[key] = items
    return items


class SBX_OT_add_to_part(bpy.types.Operator):
    """Join the selected meshes (as new shapes) and empties (as new markers) to the active object's part"""
    bl_idname = 'sbx.add_to_part'
    bl_label = 'Add to Part'
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context):
        act = context.active_object
        return act is not None and part_tree.root_of(act) is not None and len(context.selected_objects) > 1

    def execute(self, context):
        act = context.active_object
        objs = [o for o in context.selected_objects if o is not act]
        context.view_layer.update()
        try:
            done = edit_part.add_to_part(objs, act)
        except (edit_part.EditError, SourceError) as e:
            self.report({'ERROR'}, str(e))
            return {'CANCELLED'}
        if not done:
            self.report({'WARNING'}, 'Nothing joined (only meshes and empties not already in the part can be)')
            return {'CANCELLED'}
        self.report({'INFO'}, 'Joined to %s: %s' % (part_tree.root_of(act)['sbx_part'],
                                                   ', '.join(w for _, w in done)))
        return {'FINISHED'}


class SBX_OT_remove_from_part(bpy.types.Operator):
    """The selected new shapes and markers stop being part of their part (they stay in the scene)"""
    bl_idname = 'sbx.remove_from_part'
    bl_label = 'Remove from Part'
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context):
        return any(properties.new_shape_ref(o) or properties.new_marker_ref(o) for o in context.selected_objects)

    def execute(self, context):
        edit_part.remove_from_part(context.selected_objects)
        return {'FINISHED'}


def _pair_items(self, context):
    try:
        choices = Sources().pair_choices()
    except Exception as e:
        log.warning('shader pairs not listed: %s', e)
        choices = []
    items = [(key, '%s  (%s)' % (ids, what), 'From %s' % src) for key, ids, what, src in choices]
    return _keep('pairs', items or [('NONE', 'No shader pairs: import a part or build the library', '')])


def _template_items(self, context):
    if self.pair in ('', 'NONE'):
        return _keep('templates', [('NONE', 'Pick a shader first', '')])
    try:
        choices = Sources().template_choices(self.pair)
    except Exception as e:
        log.warning('templates not listed: %s', e)
        choices = []
    items = [(ref, label, ref) for ref, label in choices]
    return _keep('templates', items or [('NONE', 'No record uses this shader', '')])


def _texture_items(self, context):
    root = part_tree.root_of(context.active_object) if context.active_object else None
    names = properties.xpr_names(root)
    items = [('TEMPLATE', 'As the template', 'Keep the template\'s texture')]
    if names:
        items += [(str(i), '%d %s' % (i, n), n) for i, n in enumerate(names)]
        items.append((str(len(names)), '%d (paint / vinyl sheet)' % len(names),
                      'One past the XPR2\'s last texture: bound at run time (the car\'s paint or vinyl)'))
    else:
        items += [(str(i), 'texture %d' % i, '') for i in range(64)]
    return _keep('textures', items)


def _pair_changed(self, context):
    items = _template_items(self, context)
    if items:
        self.template = items[0][0]


class SBX_OT_make_material(bpy.types.Operator):
    """Make the active material an SBX material: pick its shader pair, the record it starts from, and its textures"""
    bl_idname = 'sbx.make_material'
    bl_label = 'Make SBX Material'
    bl_options = {'REGISTER', 'UNDO'}

    material: StringProperty(name='Material', description='Blender material to make (empty: a new one)')
    pair: EnumProperty(name='Shader', items=_pair_items, update=_pair_changed,
                       description='Vertex and pixel shader pair (ids as in docs/game_data.md)')
    template: EnumProperty(name='Start from', items=_template_items,
                           description='The game record whose fields not worked out the material keeps')
    tex0: EnumProperty(name='Texture 1', items=_texture_items)
    tex1: EnumProperty(name='Texture 2', items=_texture_items)
    tex2: EnumProperty(name='Texture 3', items=_texture_items)

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def invoke(self, context, event):
        obj = context.active_object
        mat = obj.active_material if obj else None
        self.material = mat.name if mat else ''
        if mat is not None and mat.get('sbx_shader'):
            try:
                self.pair = mat['sbx_shader']
            except TypeError:
                pass
        return context.window_manager.invoke_props_dialog(self, width=460)

    def _bound(self):
        if self.template in ('', 'NONE'):
            return 0
        try:
            rec, _ = Sources().record(self.template)
        except SourceError:
            return 0
        return len(rec.textures)

    def draw(self, context):
        lay = self.layout
        lay.prop(self, 'material')
        lay.prop(self, 'pair')
        lay.prop(self, 'template')
        for k in range(min(self._bound(), 3)):
            lay.prop(self, 'tex%d' % k)
        root = part_tree.root_of(context.active_object)
        if root is not None and not properties.xpr_names(root):
            lay.label(text='Texture names not known (import the part again to list them)', icon='INFO')

    def execute(self, context):
        if self.pair in ('', 'NONE') or self.template in ('', 'NONE'):
            self.report({'ERROR'}, 'Pick a shader and a record to start from')
            return {'CANCELLED'}
        obj = context.active_object
        mat = bpy.data.materials.get(self.material) if self.material else None
        if mat is None:
            mat = bpy.data.materials.new(self.material or 'SBX material')
            if obj is not None and obj.type == 'MESH':
                obj.data.materials.append(mat)
        elif 'sbx_record' in mat:
            # An imported material stays as it is (other shapes use it):
            # the new material is a copy, put in the active slot.
            mat = mat.copy()
            mat.name = properties.game_name(self.material) + ' new'
            if obj is not None and obj.type == 'MESH' and obj.active_material is not None:
                obj.material_slots[obj.active_material_index].material = mat
        try:
            src = Sources()
            rec, _ = src.record(self.template)
            bound = rec.textures
            textures = []
            for k, (x, t) in enumerate(bound):
                choice = getattr(self, 'tex%d' % k) if k < 3 else 'TEMPLATE'
                textures.append((x, t) if choice == 'TEMPLATE' else (0, int(choice)))
            edit_part.make_material(mat, self.pair, self.template, textures, part_tree.root_of(obj), src)
        except (edit_part.EditError, SourceError) as e:
            self.report({'ERROR'}, str(e))
            return {'CANCELLED'}
        self.report({'INFO'}, '%s: SBX material, %s' % (mat.name, edit_part.pair_ids(self.pair)))
        return {'FINISHED'}


class SBX_OT_sync_sidecar(bpy.types.Operator):
    """Update the active part's readable sidecar from the scene (export does this too)"""
    bl_idname = 'sbx.sync_sidecar'
    bl_label = 'Refresh Sidecar'
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context):
        return context.active_object is not None and part_tree.root_of(context.active_object) is not None

    def execute(self, context):
        name = part_tree.root_of(context.active_object)['sbx_part']
        edit_part.sync_sidecar(name)
        self.report({'INFO'}, 'Sidecar of %s refreshed (text %s)' % (name, properties.sidecar_text_name(name)))
        return {'FINISHED'}


class SBX_OT_build_library(bpy.types.Operator):
    """Build the shader library from the dumped part files (BUILD_OUT): every shader pair, with examples"""
    bl_idname = 'sbx.build_library'
    bl_label = 'Build Shader Library'

    def execute(self, context):
        from .sbx_formats import library, read_PART
        prefs = context.preferences.addons[__package__].preferences
        folder = bpy.path.abspath(prefs.build_out)
        if not folder or not os.path.isdir(folder):
            self.report({'ERROR'}, 'Set the BUILD_OUT folder first')
            return {'CANCELLED'}
        wm = context.window_manager
        wm.progress_begin(0, 100)
        try:
            lib = library.build_library(folder, read_PART,
                                        lambda done, total: wm.progress_update(int(100 * done / max(total, 1))))
        finally:
            wm.progress_end()
        if not lib['pairs']:
            self.report({'ERROR'}, 'No part files in %s' % folder)
            return {'CANCELLED'}
        path = library_path()
        os.makedirs(os.path.dirname(path), exist_ok=True)
        library.write_library(lib, path)
        log.info('shader library built from %s: %d pairs -> %s', folder, len(lib['pairs']), path)
        self.report({'INFO'}, 'Shader library: %d pairs, written to %s' % (len(lib['pairs']), path))
        return {'FINISHED'}


classes = (SBX_OT_add_to_part, SBX_OT_remove_from_part, SBX_OT_make_material, SBX_OT_sync_sidecar,
           SBX_OT_build_library)
