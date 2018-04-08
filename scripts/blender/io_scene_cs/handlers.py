import bpy
from bpy.app.handlers import persistent
import time


def getCurrentMilliSeconds():
    return time.time() * 1000


@persistent
def post_ob_updated(scene):
    '''
    Triggers on:
    - material editting
    '''
    # Make sure CRYSTALSPACE render is by default selected for new scenes.
    if scene.render.engine != 'CRYSTALSPACE' and scene.get('newScene', True):
        HasCrystalSpaceEnabled = False
        for sc in bpy.data.scenes:
            if sc != scene and sc.render.engine == 'CRYSTALSPACE':
                HasCrystalSpaceEnabled = True
                break
        if HasCrystalSpaceEnabled:
            scene['newScene'] = False
            scene.render.engine = 'CRYSTALSPACE'

    if scene.is_updated or scene.is_updated_data:
        print("%s - Scene is_updated (post)" % scene.name)

    ob = scene.objects.active
    if ob is not None and ob.data is not None and not ob.is_updated:
        ob.data['changeToken'] = getCurrentMilliSeconds(
        )  # Constantly update changeToken of selected object, no cb when certain properties change :/

    if ob is not None and ob.is_updated:
            # ob.data['changeToken'] = getCurrentMilliSeconds() #Flag data as
            # changed so material changes get re-exported
        print("%s - Object is_updated (post)" % ob.name)


@persistent
def post_ob_updated_data(scene):
    '''
    Triggers on:
    - mesh data editting
    '''

    if scene.is_updated or scene.is_updated_data:
        print("%s - Scene is_updated_data (post)" % scene.name)

    ob = scene.objects.active
    if ob is not None and ob.is_updated_data:
        ob.data['changeToken'] = getCurrentMilliSeconds()
        print("%s - Object is_updated_data (post)" % ob.name)
        print("  %s(%d)" % (ob.data.name, ob.data['changeToken']))


@persistent
def post_ob_data_updated(scene):

    if scene.is_updated or scene.is_updated_data:
        print("%s - Scene post_ob_data_updated (post)" % scene.name)

    ob = scene.objects.active
    if ob is not None and ob.data is not None and ob.data.is_updated:
        print("%s - Object data is_updated (post)" % ob.data.name)


@persistent
def post_ob_data_updated_data(scene):

    if scene.is_updated or scene.is_updated_data:
        print("%s - Scene post_ob_data_updated_data (post)" % scene.name)

    ob = scene.objects.active
    if ob is not None and ob.data is not None and ob.data.is_updated_data:
        print("%s - Object data is_updated_data (post)" % ob.data.name)


bpy.app.handlers.scene_update_post.clear()
bpy.app.handlers.scene_update_post.clear()
bpy.app.handlers.scene_update_post.clear()
bpy.app.handlers.scene_update_post.clear()

bpy.app.handlers.scene_update_post.append(post_ob_updated)
bpy.app.handlers.scene_update_post.append(post_ob_updated_data)
bpy.app.handlers.scene_update_post.append(post_ob_data_updated)
bpy.app.handlers.scene_update_post.append(post_ob_data_updated_data)
