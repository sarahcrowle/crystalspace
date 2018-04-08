import bpy
from mathutils import *

from .util import *


def ObjectRelativeMatrix(self):
    return self.matrix_local
    '''
  mat = self.matrix
  if self.parent:
    mat = mat - self.parent.relative_matrix
  return mat
  '''

bpy.types.Object.relative_matrix = property(ObjectRelativeMatrix)


def MatrixAsCS(matrix, func, depth=0, noMove=False, noScale=True, noTranslation=False):
    ''' unique() is not defined in the new mathutils API '''

    loc, mat, scale = DecomposeMatrix(matrix)
    rot = mat.to_euler()
    if not noMove:
        func(' ' * depth + '<move>')
    # Flip Y and Z axis.
    if not noTranslation:
        func(' ' * depth + '  <v x="%s" z="%s" y="%s" />' % tuple(loc))
    if rot[0] or rot[1] or rot[2] or scale[0] != 1.0 or scale[1] != 1.0 or scale[2] != 1.0:
        func(' ' * depth + '  <matrix>')
        # Order 'YZX' is important!
        if rot[2]:
            func(' ' * depth + '    <roty>%s</roty>' % (rot[2]))
        if rot[1]:
            func(' ' * depth + '    <rotz>%s</rotz>' % (-rot[1]))
        if rot[0]:
            func(' ' * depth + '    <rotx>%s</rotx>' % (-rot[0]))
        if not noScale and (scale[0] != 1.0 or scale[1] != 1.0 or scale[2] != 1.0):
            func(' ' * depth + '    <scale x="%s" z="%s" y="%s" />' %
                 tuple(scale))
        func(' ' * depth + '  </matrix>')
    if not noMove:
        func(' ' * depth + '</move>')
