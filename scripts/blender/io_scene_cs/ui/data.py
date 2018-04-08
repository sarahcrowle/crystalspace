import bpy

from io_scene_cs.utilities import rnaType, settings

from bpy.types import PropertyGroup


class csFactoryPanel():
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "data"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        ob = bpy.context.active_object
        rd = context.scene.render
        r = (ob and ob.type == 'MESH' and ob.data)
        return r and (rd.engine in cls.COMPAT_ENGINES)


@rnaType
class MESH_PT_csFactory(csFactoryPanel, bpy.types.Panel):
    bl_label = "Crystal Space Mesh Factory"
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        layout = self.layout

        ob = bpy.context.active_object

        if ob.type == 'MESH':
            ob = bpy.context.active_object.data

            split = layout.split()
            col1 = split.column(align=True)
            box1 = col1.box()

            row = box1.row()
            row.prop(ob.b2cs, "use_imposter")

            row = box1.row()
            row.prop(ob.b2cs, "no_shadow_receive")

            row = box1.row()
            row.prop(ob.b2cs, "no_shadow_cast")

            row = box1.row()
            row.prop(ob.b2cs, "limited_shadow_cast")

            col2 = split.column(align=True)
            box2 = col2.box()

            box2.label(text="Lighter2")

            row = box2.row()
            row.prop(ob.b2cs, "lighter2_vertexlight")

            row = box2.row()
            row.prop(ob.b2cs, "lighter2_selfshadow")

            row = box2.row()
            row.prop(ob.b2cs, "lighter2_lmscale")


@settings(type='Mesh')
class CrystalSpaceSettingsMesh(PropertyGroup):
    use_imposter = bpy.props.BoolProperty(
        name="Imposter mesh",
        description="Whether or not this mesh should use an imposter",
        default=False)

    no_shadow_receive = bpy.props.BoolProperty(
        name="No shadow receive",
        description="Whether or not shadows can be cast on this mesh",
        default=False)

    no_shadow_cast = bpy.props.BoolProperty(
        name="No shadow cast",
        description="Whether or not this mesh can cast shadows on other objects while in normal shadow casting mode",
        default=False)

    limited_shadow_cast = bpy.props.BoolProperty(
        name="Limited shadow cast",
        description="Whether or not this mesh can cast shadows on other objects while in limited shadow casting mode",
        default=False)

    lighter2_vertexlight = bpy.props.BoolProperty(
        name="Vertex lighting",
        description="Enable vertex lighting for this mesh in case lighter2 is used",
        default=False)

    lighter2_selfshadow = bpy.props.BoolProperty(
        name="Self shadowing",
        description="Enable self shadowing in case lighter2 is used",
        default=False)

    lighter2_lmscale = bpy.props.FloatProperty(
        name="LM Scale",
        description="Lightmap scale for lighter2 (higher means more detail)",
        default=0.0)

    array_as_meshobj = bpy.props.BoolProperty(
        name="Export arrays as mesh factory instances",
        description="Export fixed count array copies as instances" +
        " of object factory ('meshobj') in world file",
        default=False)
