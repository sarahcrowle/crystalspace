import bpy
from io_scene_cs.utilities import GetPreferences


def WorldAsCS(self, func, depth=0):
    """ Write an xml decription of this world: mainly physics settings
    """

    scenes = [bpy.context.scene] if GetPreferences().exportOnlyCurrentScene else [
        scene for scene in bpy.data.scenes if scene.b2cs.export]
    engine = None
    for scene in scenes:
        print('ENGINE: ' + scene.game_settings.physics_engine)
        if scene.game_settings.physics_engine == 'BULLET':
            engine = 'crystalspace.physics.bullet'
            break

    if engine:
        func(' ' * depth + '<addon plugin="crystalspace.physics.loader">')
        func(' ' * depth + '  <collisionsystem plugin="%s">' % (engine))
        func(' ' * depth + '    <simulationspeed speed="1.0" />')  # TODO
        func(
            ' ' * depth + '    <stepparams timestep="0.0166" maxstep="1" iterations="10" />')
             #TODO
        func(' ' * depth + '    <internalscale scale="1.0" />')  # TODO
        func(' ' * depth + '    <softbodyenabled enabled="true" />')  # TODO
        func(' ' * depth + '    <damping linear="0.1" angular="0.1" />')
             # TODO
        func(' ' * depth + '  </collisionsystem>')
        func(' ' * depth + '</addon>')

bpy.types.World.AsCS = WorldAsCS
