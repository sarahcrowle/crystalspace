import bpy

from io_scene_cs.utilities import rnaType, settings
from bpy.types import PropertyGroup


class csLampPanel():
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "data"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        ob = bpy.context.active_object
        r = (ob and ob.type == 'LAMP' and ob.data)
        rd = context.scene.render
        return r and (rd.engine in cls.COMPAT_ENGINES)


@rnaType
class MESH_PT_csLamp(csLampPanel, bpy.types.Panel):
    bl_label = "Crystal Space Properties"
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        layout = self.layout

        ob = bpy.context.active_object

        if ob.type == 'LAMP':
            ob = bpy.context.active_object.data
            row = layout.row()
            row.prop(ob, "use_shadow", "Cast shadows")
