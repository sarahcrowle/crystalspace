import bpy
import mathutils
from .util import *


def ObjectHasMergingGroup(self):
    for group in self.users_group:
        if group.b2cs.doMerge:
            return group
    return None

bpy.types.Object.hasMergingGroup = ObjectHasMergingGroup


def SceneDependencies(self):
    dependencies = EmptyDependencies()

    for ob in self.objects:
        if not ob.IsExportable():
            continue
        merge = False
        for group in ob.users_group:
            if group.b2cs.doMerge:
                dependencies['G'][group.uname] = group
                MergeDependencies(dependencies, group.GetDependencies())
                merge = True
        if not merge:
            MergeDependencies(dependencies, ob.GetDependencies())

    return dependencies

bpy.types.Scene.GetDependencies = SceneDependencies


def SceneAsCS(self, func, depth=0):
    """ Write an xml decription of this scene: it is exported as a CS sector
        with a reference to all mesh, armature, group, lamp and portal objects
        contained in this scene (no reference to cameras and socket objects)
    """
    print("- sector '%s'" % self.uname)
    func(' ' * depth + '<sector name="%s">' % (self.uname))

    # Export physics
    self.ScenePhysicsAsCS(func, depth + 1)

    objects = []

    # Export lamps and objects (as instancies of factories)
    for ob in self.exportable_toplevel_objects:
        print(" - '%s'" % (ob.uname))
        if ob not in objects:
            group = ob.hasMergingGroup()
            if group:
                group.AsCS(func, depth + 2)
                objects.extend([object for object in group.objects])
            else:
                ob.AsCS(func, depth + 2)
                objects.append(ob)

    func(' ' * depth + '</sector>')

bpy.types.Scene.AsCS = SceneAsCS


def GetExportableTopLevelObjects(scene):
    objects = []
    for o in scene.objects:
        if o.type not in ['CAMERA'] and o.parent is None:
            if o.IsExportable():
                if o.portal.enabled:
                    objects.insert(0, o)
                else:
                    objects.append(o)
            else:
                print("Skipping '%s'" % (o.uname))
    return objects

bpy.types.Scene.exportable_toplevel_objects = property(
    GetExportableTopLevelObjects)


def HasBulletPhysicsEnabled(self):
    return self.game_settings.physics_engine == 'BULLET'

bpy.types.Scene.HasBulletPhysicsEnabled = HasBulletPhysicsEnabled


def ScenePhysicsAsCS(self, func, depth=0):

    engine = None
    if self.HasBulletPhysicsEnabled():
        engine = 'crystalspace.physics.bullet'

    if engine:
        gravity = -1 * self.game_settings.physics_gravity

        func(' ' * depth + '<addon plugin="crystalspace.physics.loader">')
        func(' ' * depth + '  <collisionsector>')
        func(' ' * depth + '    <gravity x="0" y="%f" z="0" />' % (gravity))
        func(' ' * depth + '    <damping linear="0.1" angular="0.1" />')
             # TODO
        func(
            ' ' * depth + '    <autodisabling linear="0.8" angular="1.0" time="0" />')
             #TODO
        func(' ' * depth + '  </collisionsector>')
        func(' ' * depth + '</addon>')


bpy.types.Scene.ScenePhysicsAsCS = ScenePhysicsAsCS


def GetCameras(self):
    """ Get a list of all visible cameras of this scene
    """
    cameras = {}
    for ob in self.objects:
        if ob.type == 'CAMERA' and not ob.hide:
            cameras[ob.uname] = {'scene': self, 'camera': ob}
    return cameras

bpy.types.Scene.GetCameras = GetCameras
