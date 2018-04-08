import bpy
import os
from bpy.props import StringProperty, BoolProperty, CollectionProperty, IntProperty
from bpy.types import AddonPreferences
from .utilities import rnaType, GetDefaultPath, GetDefaultPath, settings, GetPreferences, GetAddonPreferences, reloadShadersets

from bpy.types import PropertyGroup


@rnaType
class ShaderSetFile(PropertyGroup):
    name = StringProperty(
        name="Shaderset file",
        subtype='FILE_PATH',
        update=reloadShadersets
    )

    @property
    def abspath(self):
        return bpy.path.abspath(self.name)

    @property
    def valid(self):
        return os.path.isfile(self.abspath)


@rnaType
class ExportCSPreferences(AddonPreferences):
    bl_idname = __package__

    exportPath = StringProperty(
        name="Default Export Path",
        default=GetDefaultPath(False),
        subtype='DIR_PATH',
    )

    shaderSetFiles = CollectionProperty(
        type=ShaderSetFile
    )
    active_shaderSetFiles_index = IntProperty(
        default=-1,
        min=-1,
        max=65535
    )

    def draw(self, context):
        layout = self.layout
        row = layout.row(align=True)
        row.label(text="This is a preferences view for our addon")
        row = layout.row(align=True)
        row.label(text="Don't forget to click the 'Save User Settings'-button")
        row = layout.row(align=True)
        row.prop(self, "exportPath")

        row = layout.row(align=True)
        box = row.box()
        box.template_list("UI_UL_list", "files", self, "shaderSetFiles", self,
                          "active_shaderSetFiles_index", rows=5, maxrows=5, type='DEFAULT')
        sub = row.column(align=True)
        sub.operator(
            "addonpreferences.shadersetfile_add", icon='ZOOMIN', text="")
        sub.operator(
            "addonpreferences.shadersetfile_remove", icon='ZOOMOUT', text="")

        if len(self.shaderSetFiles) > self.active_shaderSetFiles_index >= 0:
            slave = self.shaderSetFiles[self.active_shaderSetFiles_index]
            row = layout.row(align=True)
            box = row.box()
            row = box.row()
            if not slave.valid:
                row.alert = True
            row.prop(slave, 'name')


class PREFERENCES_OT_shaderset_add(bpy.types.Operator):
    bl_idname = "addonpreferences.shadersetfile_add"
    bl_label = "Add"

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        prefs = GetAddonPreferences()

        script = prefs.shaderSetFiles.add()
        prefs.active_shaderSetFiles_index = len(prefs.shaderSetFiles) - 1

        return {'FINISHED'}

    def invoke(self, context, event):
        return self.execute(context)


class PREFERENCES_OT_shaderset_remove(bpy.types.Operator):
    bl_idname = "addonpreferences.shadersetfile_remove"
    bl_label = "Remove"

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        prefs = GetAddonPreferences()

        if prefs.active_shaderSetFiles_index >= 0:
            prefs.shaderSetFiles.remove(prefs.active_shaderSetFiles_index)
            prefs.active_shaderSetFiles_index = len(prefs.shaderSetFiles) - 1

        return {'FINISHED'}

    def invoke(self, context, event):
        return self.execute(context)


from .ui.renderlayer import csFactoryRef, csMaterialRef, PostExportScript


@settings(type='Text', attribute='preferences')
class CrystalSpaceSettingsGlobal(PropertyGroup):
    exportPath = StringProperty(
        name="Export Path",
        default=GetDefaultPath(),
        subtype='DIR_PATH',
    )

    console = BoolProperty(
        name="Console",
        description="Enable the '-console' flag of 'walktest'",
        default=True,
    )

    verbose = BoolProperty(
        name="Verbose",
        description="Enable the '-verbose=-scf' flag of 'walktest'",
        default=True,
    )

    silent = BoolProperty(
        name="Silent",
        description="Enable the '-silent' flag of 'walktest'",
        default=True,
    )

    bugplug = BoolProperty(
        name="Bugplug",
        description="Enable bugplug in 'walktest'",
        default=False,
    )

    library = BoolProperty(
        name="Export as a CS library",
        description="Export all mesh factories in an unique CS library file",
        default=False,
    )

    enableDoublesided = BoolProperty(
        name="Enable double sided meshes",
        description="Global enabling of the 'Double Sided' option for all meshes",
        default=False,
    )

    exportOnlyCurrentScene = BoolProperty(
        name="Export only current scene",
        description="Export only current scene",
        default=False,
    )

    sharedMaterial = BoolProperty(
        name="Shared materials and textures",
        description="Define all textures and materials in the world file",
        default=True,
    )

    FactoryRefs = CollectionProperty(
        name="Collection of Crystal Space factory references",
        description="Define references to existing Crystal Space factories",
        type=csFactoryRef,
    )

    MaterialRefs = CollectionProperty(
        name="Collection of Crystal Space material references",
        description="Define references to existing Crystal Space materials",
        type=csMaterialRef,
    )

    postExportScripts = CollectionProperty(
        name="Post Export Scripts",
        description="Collection of scripts to be executed when exporting has finished",
        type=PostExportScript
    )
    active_postExportScripts_index = IntProperty(
        name="Index of the active slave",
        description="",
        default=-1,
        min=-1,
        max=65535
    )
