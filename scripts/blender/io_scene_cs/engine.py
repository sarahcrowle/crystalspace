import bpy


class CrystalSpace(bpy.types.RenderEngine):
    bl_idname = 'CRYSTALSPACE'
    bl_label = "CrystalSpace"

    def render(self, scene):
        print('Do test renders with Blender Render')


#
# Load specific panels by adding ourselfs to COMPAT_ENGINES
#
def fake_poll(poll_function):
    # print(poll_function)
    @classmethod
    def poll(cls, context):
        rd = context.scene.render
        return poll_function(context) and (rd.engine != 'CRYSTALSPACE')
    return poll

# Small hack, PHYSICS_PT_add doesn't have COMPAT_ENGINES, override its
# poll function instead
from bl_ui import properties_physics_common
properties_physics_common.PHYSICS_PT_add.poll = fake_poll(
    properties_physics_common.PHYSICS_PT_add.poll)
del properties_physics_common


# /usr/share/blender/2.68/scripts/startup/bl_ui
def register():
    # for name in ['scene', 'texture', 'object', 'particle', 'material']:
    for name in ['scene', 'texture', 'object', 'particle', ]:
        name = 'properties_' + name
        module = __import__('bl_ui.' + name).__dict__[name]
        for member in dir(module):
            subclass = getattr(module, member)
            try:
                subclass.COMPAT_ENGINES.add('CRYSTALSPACE')
            except:
                pass
        del module

    for name in ['armature', 'bone', 'camera', 'curve', 'empty', 'lattice', 'mesh', 'metaball', 'modifier', 'speaker']:
        name = 'properties_data_' + name
        module = __import__('bl_ui.' + name).__dict__[name]
        for member in dir(module):
            subclass = getattr(module, member)
            try:
                subclass.COMPAT_ENGINES.add('CRYSTALSPACE')
            except:
                pass
        del module

    from bl_ui import properties_material
    properties_material.MATERIAL_PT_context_material.COMPAT_ENGINES.add(
        'CRYSTALSPACE')
    properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.add('CRYSTALSPACE')
    del properties_material

    from bl_ui import properties_game
    properties_game.PHYSICS_PT_game_physics.COMPAT_ENGINES.add('CRYSTALSPACE')
    properties_game.PHYSICS_PT_game_collision_bounds.COMPAT_ENGINES.add(
        'CRYSTALSPACE')
    properties_game.PHYSICS_PT_game_obstacles.COMPAT_ENGINES.add(
        'CRYSTALSPACE')
    properties_game.WORLD_PT_game_physics.COMPAT_ENGINES.add('CRYSTALSPACE')
    # properties_game.WORLD_PT_game_physics_obstacles.COMPAT_ENGINES.add('CRYSTALSPACE')
    del properties_game

    from bl_ui import properties_data_lamp
    properties_data_lamp.DATA_PT_context_lamp.COMPAT_ENGINES.add(
        'CRYSTALSPACE')
    properties_data_lamp.DATA_PT_preview.COMPAT_ENGINES.add('CRYSTALSPACE')
    properties_data_lamp.DATA_PT_lamp.COMPAT_ENGINES.add('CRYSTALSPACE')

    properties_data_lamp.DATA_PT_area.COMPAT_ENGINES.add('CRYSTALSPACE')
    properties_data_lamp.DATA_PT_spot.COMPAT_ENGINES.add('CRYSTALSPACE')
    properties_data_lamp.DATA_PT_falloff_curve.COMPAT_ENGINES.add(
        'CRYSTALSPACE')
    properties_data_lamp.DATA_PT_custom_props_lamp.COMPAT_ENGINES.add(
        'CRYSTALSPACE')

    del properties_data_lamp
