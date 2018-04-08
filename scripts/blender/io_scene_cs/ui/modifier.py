import bpy

from io_scene_cs.utilities import rnaType


class csModifierPanel():
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "modifier"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        ob = bpy.context.active_object
        arrays = [mod for mod in ob.modifiers
                  if mod.type == 'ARRAY' and mod.fit_type == 'FIXED_COUNT']
        r = (ob and ob.type == 'MESH' and ob.data and
             len(ob.modifiers) != 0 and len(arrays) == len(ob.modifiers))
        rd = context.scene.render
        return r and (rd.engine in cls.COMPAT_ENGINES)


@rnaType
class MESH_PT_csModifier(csModifierPanel, bpy.types.Panel):
    bl_label = "Crystal Space Modifiers"
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        ob = context.active_object

        if ob.type == 'MESH' and len(ob.modifiers) != 0:
            # Draw a checkbox to decide how to export object's modifiers
            layout = self.layout
            row = layout.row()
            row.prop(ob.data.b2cs, "array_as_meshobj")
