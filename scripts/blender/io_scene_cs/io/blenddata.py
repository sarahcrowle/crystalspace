import bpy
from io_scene_cs.utilities import GetPreferences


def GetExportableScenes(context):
    scenes = [bpy.context.scene] if GetPreferences().exportOnlyCurrentScene else [
        scene for scene in bpy.data.scenes if scene.b2cs.export]
    if len(scenes) == 0:
        # If no scenes are exported, atleast export the current scene, for
        # those schmucks that disable the export of the only scene.
        scenes = [bpy.context.scene]
    return scenes

bpy.types.BlendData.exportable_scenes = property(GetExportableScenes)
