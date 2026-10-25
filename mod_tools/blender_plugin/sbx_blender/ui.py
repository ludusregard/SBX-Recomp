"""Panels: the SBX tab in the 3D view's sidebar, and SBX in the material
properties."""

import os

import bpy

from . import edit_part, part_tree, properties
from .sources import library_path


class SBX_PT_part(bpy.types.Panel):
    bl_label = 'SBX Part'
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'SBX'

    def draw(self, context):
        lay = self.layout
        obj = context.active_object
        root = part_tree.root_of(obj) if obj else None
        box = lay.box()
        if root is None:
            box.label(text='Active object isn\'t in an SBX part', icon='INFO')
        else:
            box.label(text='Part %s (%s)' % (root['sbx_part'], root.get('sbx_attach', '') or '-'), icon='MESH_CUBE')
            if properties.shape_ref(obj):
                box.label(text='Shape %s' % obj.get('sbx_shape_name', ''))
            elif properties.new_shape_ref(obj):
                box.label(text='New shape', icon='ADD')
                box.prop(obj, '["sbx_new_shape"]', text='Name in part')
            elif obj.get('sbx_marker'):
                box.label(text='Marker %s' % obj['sbx_marker'])
            elif properties.new_marker_ref(obj):
                box.label(text='New marker', icon='ADD')
                box.prop(obj, '["sbx_new_marker"]', text='Name in part')
            elif obj is root:
                box.label(text='Part root')
        col = lay.column(align=True)
        col.operator('sbx.add_to_part', icon='ADD')
        col.operator('sbx.remove_from_part', icon='REMOVE')
        col.operator('sbx.make_material', icon='MATERIAL')
        col.operator('sbx.sync_sidecar', icon='FILE_REFRESH')
        p = library_path()
        lay.label(text='Shader library: %s' % ('found' if os.path.exists(p) else 'not built (preferences)'),
                  icon='CHECKMARK' if os.path.exists(p) else 'ERROR')


class SBX_PT_material(bpy.types.Panel):
    bl_label = 'SBX'
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'material'

    @classmethod
    def poll(cls, context):
        return context.material is not None

    def draw(self, context):
        lay = self.layout
        mat = context.material
        if not properties.is_sbx_material(mat):
            lay.label(text='Not an SBX material', icon='INFO')
            lay.operator('sbx.make_material', icon='MATERIAL')
            return
        key = mat.get('sbx_shader') or ''
        lay.label(text='Shader %s' % (edit_part.pair_ids(key) if key else '?'))
        names = [n for n in mat.get('sbx_texture_names', '').split(',') if n]
        lay.label(text='Textures: %s' % (', '.join(names) or 'none'))
        if 'sbx_new' not in mat:
            lay.label(text='From %s; values: in the part\'s sidecar, per draw' % mat.get('sbx_record', '?'))
            lay.operator('sbx.make_material', text='Make a New SBX Material from It', icon='MATERIAL')
            return
        lay.label(text='New, from %s' % mat.get('sbx_template', '?'))
        lay.label(text='Each draw\'s values (colors, textures by index or name, shader, flags ...)')
        lay.label(text='are edited in its part\'s sidecar text (<part>.sbx.json, Text Editor).')
        lay.operator('sbx.make_material', text='Change Shader or Textures', icon='MATERIAL')


classes = (SBX_PT_part, SBX_PT_material)
