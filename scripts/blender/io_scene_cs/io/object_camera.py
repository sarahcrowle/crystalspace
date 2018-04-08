import bpy
from .util import DecomposeMatrix
import mathutils


def CameraAsCS(self, func, depth, scene):
    """ Export camera as a CS start location of current scene;
        if no camera is defined, set a default start position
    """
    if self == None:
        # Set a default camera if none was found in current scene
        func(' ' * depth + '<start name="Camera">')
        func(' ' * depth + '  <sector>%s</sector>' % (scene.uname))
        func(' ' * depth + '  <position x="0" y="0" z="0" />')
        func(' ' * depth + '  <up x="0" y="1" z="0" />')
        func(' ' * depth + '  <forward x="0" y="0" z="1" />')
        func(' ' * depth + '</start>')
    else:
        func(' ' * depth + '<start name="%s">' % (self.uname))
        func(' ' * depth + '  <sector>%s</sector>' % (scene.uname))
        # Flip Y and Z axis.
        func(' ' * depth + '  <position x="%f" z="%f" y="%f" />' %
             tuple(self.location))
        loc, rot, scale = DecomposeMatrix(self.matrix_world)
        func(' ' * depth + '  <up x="%f" z="%f" y="%f" />' %
             tuple(rot * mathutils.Vector((0.0, 1.0, 0.0))))
        func(' ' * depth + '  <forward x="%f" z="%f" y="%f" />' %
             tuple(rot * mathutils.Vector((0.0, 0.0, -1.0))))
        func(' ' * depth + '</start>')

bpy.types.Object.CameraAsCS = CameraAsCS
