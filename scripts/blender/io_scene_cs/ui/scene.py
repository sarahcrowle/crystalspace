import bpy

from io_scene_cs.utilities import rnaType, settings, GetPreferences
from bpy.types import PropertyGroup


class csSectorPanel():
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "scene"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        rd = context.scene.render
        return (rd.engine in cls.COMPAT_ENGINES)


@rnaType
class SCENE_PT_csSector(csSectorPanel, bpy.types.Panel):
    bl_label = "Crystal Space Sector"
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        layout = self.layout

        ob = bpy.context.scene

        split = layout.split()
        col1 = split.column(align=True)
        box1 = col1.box()

        row = box1.row()
        row.prop(ob.b2cs, "export")

        if GetPreferences().exportOnlyCurrentScene:
            row = box1.row()
            row.label(
                text="exportOnlyCurrentScene enabled so this won't have any effect.")


@settings(type='Scene')
class CrystalSpaceSettingsScene(PropertyGroup):
    export = bpy.props.BoolProperty(
        name="Export this scene",
        description="Whether to export this scene",
        default=True)
