import bpy
import operator

from io_scene_cs.utilities import rnaType, settings, GetPreferences, prepend_draw, append_draw
from bpy.types import PropertyGroup

from io_scene_cs.ui.idref import IDRefContainer, IDRefProperty


class csObjectPanel():
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        ob = context.active_object
        r = (
            ob and ob.type == 'MESH' and not ob.portal.enabled and not ob.IsTriangleMesh())
        rd = context.scene.render
        return r and (rd.engine in cls.COMPAT_ENGINES)


class SelectFactoryRef(bpy.types.Operator):
    bl_idname = "object.select_fact_ref"
    bl_label = "Select CS factory"

    def avail_factories(self, context):
        items = [(str(i), f.name, f.vfs)
                 for i, f in enumerate(GetPreferences().FactoryRefs)]
        items.append((str(-1), ' NONE', 'None'))
        return sorted(items, key=operator.itemgetter(1))
    select_factory = bpy.props.EnumProperty(
        items=avail_factories, name="Available CS factories")

    @classmethod
    def poll(cls, context):
        return context.mode == 'OBJECT'

    def execute(self, context):
        ob = context.scene.objects.active
        if int(self.select_factory) != -1:
            ob.b2cs.csFactoryName = GetPreferences().FactoryRefs[
                int(self.select_factory)].name
            ob.b2cs.csFactoryVfs = GetPreferences().FactoryRefs[
                int(self.select_factory)].vfs
        else:
            ob.b2cs.csFactoryName = ''
            ob.b2cs.csFactoryVfs = ''
        return {'FINISHED'}


@rnaType
class OBJECT_PT_csFactoryRef(csObjectPanel, bpy.types.Panel):
    bl_label = "Crystal Space Factories"
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        ob = context.active_object

        if ob.type == 'MESH':
            # Draw a checkbox to define current mesh object as a CS factory
            # reference
            layout = self.layout

            row = layout.row()
            row.prop(ob.b2cs, "type")

            layout.template_object_ref(
                ob.b2cs, 'viscull_mesh', name='Viscull mesh')

            # if ob.game.use_collision_bounds and ob.game.collision_bounds_type=='TRIANGLE_MESH':
            # layout.template_object_ref(ob.b2cs, 'collission_mesh',
            # name='Collision mesh')

            if not ob.data.b2cs.no_shadow_cast:
                layout.template_object_ref(
                    ob.b2cs, 'shadow_mesh', name='Shadow mesh')

            row = layout.row()
            row.prop(ob.b2cs, "csFactRef")

            if ob.b2cs.csFactRef:
                # Let the user select a CS factory
                row = layout.row()
                if ob.b2cs.csFactoryName == '':
                    row.operator_menu_enum(
                        "object.select_fact_ref", "select_factory", text=SelectFactoryRef.bl_label)
                else:
                    row.operator_menu_enum(
                        "object.select_fact_ref", "select_factory", text=ob.b2cs.csFactoryName)
                    # Verify that factory reference still exists
                    factories = [f.name for f in GetPreferences().FactoryRefs]
                    if not ob.b2cs.csFactoryName in factories:
                        row = layout.row()
                        row.label(
                            text="WARNING: this factory reference has been deleted!", icon='ERROR')


@prepend_draw(type='PHYSICS_PT_game_physics')
def PHYSICS_PT_game_physics_prepend_draw(self, context):
    if context.scene.render.engine == 'CRYSTALSPACE':
        ob = context.active_object
        layout = self.layout
        row = layout.row()
        if ob.hasSupportedPhysicsEnabled():
            row.label(text="Physics type supported in CS", icon='INFO')
        else:
            row.label(text="Physics type not supported in CS", icon='ERROR')


@append_draw(type='PHYSICS_PT_game_collision_bounds')
def PHYSICS_PT_game_collision_bounds_prepend_draw(self, context):
    ob = context.active_object
    layout = self.layout
    # if ob.game.use_collision_bounds and ob.game.collision_bounds_type=='TRIANGLE_MESH':
    # layout.template_object_ref(ob.b2cs, 'collission_mesh', name='Collision
    # mesh')


@IDRefContainer
@settings(type='Object')
class CrystalSpaceSettingsObject(PropertyGroup):
    type = bpy.props.EnumProperty(
        name="Object Type",
        description="",
        items=(
            ('DEFAULT', "Default", "Default"),
            ('TERRAIN', "Terrain", "Terrain")
        ),
        default='DEFAULT')

    csFactRef = bpy.props.BoolProperty(
        name="Object replaced by a CS factory object",
        description="Replace this object by a Crystal Space factory",
        default=False)
    csFactoryName = bpy.props.StringProperty(
        name="Reference of CS factory",
        description="Name of an existing Crystal Space factory",
        default="")
    csFactoryVfs = bpy.props.StringProperty(
        name="VFS path of CS factory",
        description="VFS path of a Crystal Space library file",
        default="")

    def make_poll(id):
        def poll(prop, o):
            return o.type == 'MESH' and not o.IsTriangleMesh() and o.GetTriangleMesh(id) is None and o != bpy.context.object and o.parent is None
        return poll

    def make_select(id):
        def select(prop, value):
            print('select', id, value)
            value.MakeThisATriangleMesh(id, bpy.context.object)
        return select

    def make_unselect(id):
        def unselect(prop, value):
            print('unselect', id, value)
            value.UnMakeThisATriangleMesh(id)
        return unselect

    viscull_mesh = IDRefProperty(name="Viscull Mesh", idtype='OBJECT', poll=make_poll(
        'viscull'), select=make_select('viscull'), unselect=make_unselect('viscull'))
    shadow_mesh = IDRefProperty(name="Shadow Mesh", idtype='OBJECT', poll=make_poll(
        'shadow'), select=make_select('shadow'), unselect=make_unselect('shadow'))
    # collission_mesh = IDRefProperty(name="Collission Mesh", idtype='OBJECT',
    # poll=make_poll('collission'), select=make_select('collission'),
    # unselect=make_unselect('collission'))
