import bpy

from io_scene_cs.utilities import rnaType, settings
from bpy.types import PropertyGroup


class csBonePanel():
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "bone"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        ob = context.active_pose_bone
        rd = context.scene.render
        return ob and (rd.engine in cls.COMPAT_ENGINES)


@rnaType
class BONE_PT_B2CS_bone(csBonePanel, bpy.types.Panel):
    bl_label = "Crystal Space"
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        layout = self.layout

        ob = context.active_pose_bone

        layout.prop(ob.b2cs, "endOfChain")


@settings(type='PoseBone')
class CrystalSpaceSettingsGroup(PropertyGroup):
    endOfChain = bpy.props.BoolProperty(
        name="End of chain",
        description="",
        default=False)
