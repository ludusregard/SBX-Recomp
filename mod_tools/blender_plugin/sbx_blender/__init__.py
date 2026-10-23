"""SBX Part Tools: import Shutoko Battle X part files into Blender.

Registration and menus only; the work is in the other modules:

    import_part.py   game files -> Blender objects
    export_part.py   Blender objects -> part files
    materials.py     display materials and XPR2 textures
    properties.py    the sbx_* tags that point back into the sidecar
    frames.py        game spaces to Blender's
    log.py           the log file (path in the add-on preferences)
    sbx_formats/     the format code (copied in by build.py)

Plan and status: mod_tools/blender_plugin/PLAN.md in the SBX repo.
"""

import os

import bpy
from bpy.props import BoolProperty, CollectionProperty, EnumProperty, StringProperty
from bpy_extras.io_utils import ImportHelper

from . import log
from .log import get_logger

_log = get_logger(__name__)


class SBX_OT_import_part(bpy.types.Operator, ImportHelper):
    """Import Shutoko Battle X part files (.part)"""
    bl_idname = 'import_scene.sbx_part'
    bl_label = 'Import SBX Part'
    bl_options = {'REGISTER', 'UNDO'}

    filename_ext = '.part'
    filter_glob: StringProperty(default='*.part', options={'HIDDEN'})
    files: CollectionProperty(type=bpy.types.OperatorFileListElement, options={'HIDDEN', 'SKIP_SAVE'})
    directory: StringProperty(subtype='DIR_PATH', options={'HIDDEN', 'SKIP_SAVE'})
    xpr_path: StringProperty(
        name='XPR2', subtype='FILE_PATH',
        description='XPR2 the texture indices point into. Empty: found from the part id '
                    '(the car block\'s body XPR2, or a wheel set\'s)')

    def execute(self, context):
        from . import import_part
        paths = [os.path.join(self.directory, f.name) for f in self.files if f.name] or [self.filepath]
        try:
            c = import_part.import_parts(context, paths, bpy.path.abspath(self.xpr_path) or None)
        except Exception as e:
            _log.exception('part import failed')
            self.report({'ERROR'}, 'SBX import failed: %s (see the log)' % e)
            return {'CANCELLED'}
        self.report({'INFO'}, 'Imported %(parts)d parts, %(shapes)d shapes, %(triangles)d triangles' % c)
        return {'FINISHED'}


class SBX_OT_import_car(bpy.types.Operator, ImportHelper):
    """Import a Shutoko Battle X car block from its node file (.node)"""
    bl_idname = 'import_scene.sbx_car'
    bl_label = 'Import SBX Car'
    bl_options = {'REGISTER', 'UNDO'}

    filename_ext = '.node'
    filter_glob: StringProperty(default='*.node', options={'HIDDEN'})
    car_set: EnumProperty(
        name='Set', default='AUTO',
        items=[('AUTO', 'From the id', 'Work the set out from the node file id'),
               ('a', 'a: full car', 'Opening doors and bonnet, engine bay'),
               ('b', 'b: outside model', 'Merged doors, no engine bay, shadow'),
               ('c', 'c: cockpit view', 'Front half and interior')])
    stock: BoolProperty(name='Stock', default=True,
                        description='Only the first part of each slot group, no optional groups')
    wheels: StringProperty(name='Wheels', default='',
                           description='A wheel by name (its stock wheel, e.g. "SKY35", or an aftermarket one, '
                                       'e.g. "AD03"), optionally with a tire ("AD03 583"), or part ids '
                                       '("587 588 581"). Names: car_models.xlsx, sheet Wheels')

    def execute(self, context):
        from . import import_part
        try:
            wheels = import_part.parse_wheels(self.wheels)
        except ValueError as e:
            self.report({'ERROR'}, 'Wheels: %s' % e)
            return {'CANCELLED'}
        try:
            c = import_part.import_car(context, self.filepath, None if self.car_set == 'AUTO' else self.car_set,
                                       self.stock, wheels)
        except Exception as e:
            _log.exception('car import failed')
            self.report({'ERROR'}, 'SBX import failed: %s (see the log)' % e)
            return {'CANCELLED'}
        self.report({'INFO'}, 'Imported %(parts)d parts, %(shapes)d shapes, %(triangles)d triangles' % c)
        return {'FINISHED'}


class SBX_OT_export_parts(bpy.types.Operator):
    """Write every visible SBX part to a folder, one .part file each"""
    bl_idname = 'export_scene.sbx_parts'
    bl_label = 'Export SBX Parts'
    bl_options = {'REGISTER'}

    directory: StringProperty(name='Folder', subtype='DIR_PATH',
                              description='Where the .part files go, e.g. assets/PATCH_DATA/BUILD/TXR/BUILD')
    filter_folder: BoolProperty(default=True, options={'HIDDEN'})
    skip_identical: BoolProperty(name='Skip unchanged parts', default=False,
                                 description='Don\'t write parts that come out identical to the game\'s file')
    apply_modifiers: BoolProperty(name='Apply modifiers', default=False,
                                  description='Export the meshes with their modifiers applied '
                                              '(otherwise modifiers are ignored)')

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}

    def execute(self, context):
        from . import export_part
        folder = bpy.path.abspath(self.directory)
        if not folder:
            self.report({'ERROR'}, 'Pick a folder')
            return {'CANCELLED'}
        try:
            c = export_part.export_parts(context, folder, self.skip_identical, self.apply_modifiers)
        except export_part.ExportError as e:
            _log.error('%s', e)
            self.report({'ERROR'}, str(e))
            return {'CANCELLED'}
        except Exception as e:
            _log.exception('export failed')
            self.report({'ERROR'}, 'SBX export failed: %s (see the log)' % e)
            return {'CANCELLED'}
        self.report({'INFO'}, 'Wrote %(written)d parts (%(edited)d edited, %(identical)d identical to the '
                              'game\'s, %(skipped)d skipped)' % c)
        return {'FINISHED'}


class SBX_FH_part(bpy.types.FileHandler):
    """Drag and drop .part files into the viewport."""
    bl_idname = 'SBX_FH_part'
    bl_label = 'SBX Part'
    bl_import_operator = 'import_scene.sbx_part'
    bl_file_extensions = '.part'

    @classmethod
    def poll_drop(cls, context):
        return context.area is not None and context.area.type == 'VIEW_3D'


class SBX_preferences(bpy.types.AddonPreferences):
    bl_idname = __package__

    def draw(self, context):
        self.layout.label(text='Log file: ' + log.log_path())


def _menu(self, context):
    self.layout.operator(SBX_OT_import_part.bl_idname, text='SBX Part (.part)')
    self.layout.operator(SBX_OT_import_car.bl_idname, text='SBX Car (.node)')


def _export_menu(self, context):
    self.layout.operator(SBX_OT_export_parts.bl_idname, text='SBX Parts (visible, .part)')


_classes = (SBX_OT_import_part, SBX_OT_import_car, SBX_OT_export_parts, SBX_FH_part, SBX_preferences)


def register():
    log.setup()
    for c in _classes:
        bpy.utils.register_class(c)
    bpy.types.TOPBAR_MT_file_import.append(_menu)
    bpy.types.TOPBAR_MT_file_export.append(_export_menu)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(_export_menu)
    bpy.types.TOPBAR_MT_file_import.remove(_menu)
    for c in reversed(_classes):
        bpy.utils.unregister_class(c)
    log.teardown()
