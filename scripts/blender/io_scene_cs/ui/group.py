import bpy

from io_scene_cs.utilities import rnaType, settings
from bpy.types import PropertyGroup


class csGroupPanel():
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        ob = context.active_object
        r = False
        if (ob and ob.type == 'MESH') or (ob and ob.type == 'EMPTY'):
            r = len(ob.users_group) > 0
        rd = context.scene.render
        return r and (rd.engine in cls.COMPAT_ENGINES)


@rnaType
class OBJECT_PT_B2CS_groups(csGroupPanel, bpy.types.Panel):
    bl_label = "Crystal Space Mesh Groups"
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        layout = self.layout

        ob = context.active_object

        for group in ob.users_group:
            if ob.name in group.objects:

                row = layout.row()
                row.prop(group.b2cs, "doMerge")
                row.label(group.name)

                if group.b2cs.doMerge:
                    row = layout.row()
                    row.prop(group.b2cs, "groupedInstances")


@settings(type='Group')
class CrystalSpaceSettingsGroup(PropertyGroup):
    doMerge = bpy.props.BoolProperty(
        name="Merge",
        description="Merge the objects in this group into one on export",
        default=False)

    groupedInstances = bpy.props.BoolProperty(
        name="Grouped Instances",
        description="Export children as a grouped Instances",
        default=False)
