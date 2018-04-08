import os
import bpy
from io_scene_cs.utilities import rnaType, GetPreferences, GetExportPath, GetDefaultPath


class csReferencesPanel():
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "render_layer"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        rd = context.scene.render
        return (rd.engine in cls.COMPAT_ENGINES)


@rnaType
class RENDER_PT_csReferenceMaterialPanel(csReferencesPanel, bpy.types.Panel):
    bl_label = "Crystal Space Material References"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        layout = self.layout

        row = layout.row(align=True)
        row.operator("io_scene_cs.add_material", text="Add material reference")

        for mat in GetPreferences().MaterialRefs:
            col = layout.column()
            col.context_pointer_set("current_material", mat)
            box = col.box()

            row = box.row()
            row.prop(mat, "name", text="Material name")
            row.operator(
                "io_scene_cs.rmv_material", text="", icon='X', emboss=False)

            row = box.row()
            row.prop(mat, "vfs", text="VFS path")


@rnaType
class RENDER_PT_csReferenceFactoryPanel(csReferencesPanel, bpy.types.Panel):
    bl_label = "Crystal Space Factory References"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        layout = self.layout

        row = layout.row(align=True)
        row.operator("io_scene_cs.add_factory", text="Add factory reference")

        for fact in GetPreferences().FactoryRefs:
            col = layout.column()
            col.context_pointer_set("current_factory", fact)
            box = col.box()

            row = box.row()
            row.prop(fact, "name", text="Factory name")
            row.operator(
                "io_scene_cs.rmv_factory", text="", icon='X', emboss=False)

            row = box.row()
            row.prop(fact, "vfs", text="VFS path")


#=============== Crystal Space Factory Reference panel ======================

class csFactoryRef (bpy.types.PropertyGroup):
    name = bpy.props.StringProperty(name="Name Prop", default="Unknown",
                                    description="Name of an existing Crystal Space factory")
    vfs = bpy.props.StringProperty(name="VFS Prop", default=GetDefaultPath(),
                                   description="VFS path of an existing Crystal Space library file")

bpy.utils.register_class(csFactoryRef)


class B2CS_OT_csAddFactory(bpy.types.Operator):
    bl_idname = "io_scene_cs.add_factory"
    bl_label = "Add factory reference"
    bl_description = "Define a reference to existing Crystal Space factory"

    def execute(self, context):
        new_fact = GetPreferences().FactoryRefs.add()

        return {'FINISHED'}


class B2CS_OT_csRemoveFactory(bpy.types.Operator):
    bl_idname = "io_scene_cs.rmv_factory"
    bl_label = "Remove factory reference"
    bl_description = "Remove a reference to existing CS factory"

    def execute(self, context):
        for i, fact in enumerate(GetPreferences().FactoryRefs):
            if fact.name == context.current_factory.name:
                GetPreferences().FactoryRefs.remove(i)
                break

        return {'FINISHED'}


#============= Crystal Space Material Reference panel ====================
class csMaterialRef (bpy.types.PropertyGroup):
    name = bpy.props.StringProperty(name="Name Prop", default="Unknown",
                                    description="Name of an existing Crystal Space material")
    vfs = bpy.props.StringProperty(name="VFS Prop", default=GetDefaultPath(),
                                   description="VFS path of an existing Crystal Space library file")

bpy.utils.register_class(csMaterialRef)


class B2CS_OT_csAddMaterial(bpy.types.Operator):
    bl_idname = "io_scene_cs.add_material"
    bl_label = "Add material reference"
    bl_description = "Define a reference to existing Crystal Space material"

    def execute(self, context):
        new_mat = GetPreferences().MaterialRefs.add()

        return {'FINISHED'}


class B2CS_OT_csRemoveMaterial(bpy.types.Operator):
    bl_idname = "io_scene_cs.rmv_material"
    bl_label = "Remove material reference"
    bl_description = "Remove a reference to existing Crystal Space material"

    def execute(self, context):
        for i, mat in enumerate(GetPreferences().MaterialRefs):
            if mat.name == context.current_material.name:
                GetPreferences().MaterialRefs.remove(i)
                break

        return {'FINISHED'}


#============= Crystal Space Script panel ====================

def PostExportScript_name_update_func(self, context):
    self.name = bpy.path.display_name_from_filepath(self.path)


class PostExportScript(bpy.types.PropertyGroup):
    name = bpy.props.StringProperty(name="Name")
    path = bpy.props.StringProperty(
        name="Path", subtype='FILE_PATH', update=PostExportScript_name_update_func)
    enabled = bpy.props.BoolProperty(
        name="Enable this script to run",
        default=True,
    )

    @property
    def abspath(self):
        return bpy.path.abspath(self.path)

    @property
    def valid(self):
        return os.path.isfile(self.abspath) and os.access(self.abspath, os.X_OK)


@rnaType
class RENDER_PT_csPostExportScriptsPanel(csReferencesPanel, bpy.types.Panel):
    bl_label = "Post Export scripts"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        layout = self.layout

        row = layout.row(align=True)
        row.template_list(
            "UI_UL_list", "scripts", GetPreferences(), "postExportScripts",
                          GetPreferences(), "active_postExportScripts_index", rows=5, maxrows=5, type='DEFAULT')
        sub = row.column(align=True)
        sub.operator(
            "preferences.post_export_script_add", icon='ZOOMIN', text="")
        sub.operator(
            "preferences.post_export_script_remove", icon='ZOOMOUT', text="")
        sub.operator("preferences.post_export_script_move",
                     text="", icon='TRIA_UP').type = 'UP'
        sub.operator("preferences.post_export_script_move",
                     text="", icon='TRIA_DOWN').type = 'DOWN'

        if len(GetPreferences().postExportScripts) > GetPreferences().active_postExportScripts_index >= 0:
            slave = GetPreferences().postExportScripts[
                GetPreferences().active_postExportScripts_index]
            row = layout.row(align=True)
            box = row.box()
            row = box.row()
            row.label('Name: ' + slave.name)
            row = box.row()
            if not slave.valid:
                row.alert = True
            row.prop(slave, 'path')
            row = box.row()
            row.prop(slave, 'enabled')
            if not slave.valid:
                row = box.row()
                row.label('Script file does not exist or is not executable!')

        row = layout.row(align=True)
        row.label(
            'Scripts are executed with the first argument the set export directory path.')


class RENDER_OT_post_export_script_add(bpy.types.Operator):
    bl_idname = "preferences.post_export_script_add"
    bl_label = "Remove script"

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        prefs = GetPreferences()

        script = prefs.postExportScripts.add()
        script.name = "Unnamed_" + str(len(prefs.postExportScripts))
        prefs.active_postExportScripts_index = len(prefs.postExportScripts) - 1

        return {'FINISHED'}

    def invoke(self, context, event):
        return self.execute(context)


class RENDER_OT_post_export_script_remove(bpy.types.Operator):
    bl_idname = "preferences.post_export_script_remove"
    bl_label = "Remove script"

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        prefs = GetPreferences()

        if prefs.active_postExportScripts_index >= 0:
            prefs.postExportScripts.remove(
                prefs.active_postExportScripts_index)
            prefs.active_postExportScripts_index = len(
                prefs.postExportScripts) - 1

        return {'FINISHED'}

    def invoke(self, context, event):
        return self.execute(context)


class RENDER_OT_post_export_script_move(bpy.types.Operator):
    bl_idname = "preferences.post_export_script_move"
    bl_label = "Order script"

    type = bpy.props.StringProperty(name="Name")

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):

        prefs = GetPreferences()
        print(self.type)
        print(prefs.active_postExportScripts_index)

        direction = 1 if self.type == 'DOWN' else -1

        if prefs.active_postExportScripts_index >= 0:
            newpos = prefs.active_postExportScripts_index + direction
            if newpos < len(prefs.postExportScripts) and newpos > -1:
                prefs.postExportScripts.move(
                    prefs.active_postExportScripts_index, newpos)
                prefs.active_postExportScripts_index = newpos

        return {'FINISHED'}

    def invoke(self, context, event):
        return self.execute(context)
