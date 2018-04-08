import bpy
from .util import DecomposeMatrix


def PortalsAsCS(self, func, depth=0):
    """ Export portals defined in the current scene and
        return them as a list of objects
    """
    epsilon = 1e-7
    # Check if the portal is visible
    if self.hide:
        return

    # Check if the portal defines vertices
    if len(self.data.vertices) == 0:
        print("\nWARNING: portal '%s' not exported: this mesh has no vertex" %
              (self.name))
        return

    # Find the destination scene
    source = self
    destScName = source.portal.portalDestScene
    scenes = [s.name for s in bpy.data.scenes]
    if destScName == '' or not destScName in scenes:
        print("WARNING: portal '%s' not exported: target scene '%s' "
              "is not a valid scene name" % (source.name, destScName))
        return
    destScene = bpy.data.scenes[destScName]

    # Find the destination mesh object
    destObjName = source.portal.portalDestObject
    objects = [o.name for o in destScene.objects]
    if destObjName == '' or not destObjName in objects:
        print("WARNING: portal '%s' not exported: target mesh '%s' "
              "is not a valid object name" % (source.name, destObjName))
        return
    destObject = destScene.objects[destObjName]

    # Export the portal
    func(' ' * depth + '<portals name="%s">' % (self.uname))
    func(' ' * depth + '  <portal>')

    # Write the position of portal (i.e. the coordinates of its vertices)
    for v in source.data.vertices:
        vPos = source.relative_matrix * v.co
        func(' ' * depth + '    <v x="%f" z="%f" y="%f" />' % (tuple(vPos)))

    # Write the name of the destination sector
    func(' ' * depth + '    <sector>%s</sector>' % (destScene.uname))

    # Evaluate the warping transform
    # (a space warping portal changes the camera transform)
    currPos, currMat, scale = DecomposeMatrix(source.matrix_world)
    destPos, destMat, scale = DecomposeMatrix(destObject.matrix_world)
    totMat = destMat * currMat.inverted()
    destPos_after_rotate = destPos.to_4d() * totMat
    dpos = []
    # Vector added to the camera transform after the warping matrix is applied
    for i in range(len(currPos)):
        dpos.append(-(destPos_after_rotate[i] - currPos[i]))
    if (abs(dpos[0]) > epsilon or abs(dpos[1]) > epsilon or abs(dpos[2]) > epsilon):
        func(' ' * depth + '    <ww x="%f" y="%f" z="%f" />' %
             (dpos[0], dpos[2], dpos[1]))
    # Warping matrix
    itotMat = totMat.inverted()
    euler = itotMat.to_euler()
    quat = itotMat.to_quaternion()
    if abs(quat.angle) > epsilon:
        func(' ' * depth + '    <matrix>')
        # Order 'YZX' is important!
        if abs(quat.axis[2]) > epsilon:
            func(' ' * depth + '      <roty>%s</roty>' % (euler[2]))
        if abs(quat.axis[1]) > epsilon:
            func(' ' * depth + '      <rotz>%s</rotz>' % (-euler[1]))
        if abs(quat.axis[0]) > epsilon:
            func(' ' * depth + '      <rotx>%s</rotx>' % (-euler[0]))
        func(' ' * depth + '    </matrix>')

    # Set default portal flags
    func(' ' * depth + '    <zfill />')
    func(' ' * depth + '    <float />')
    func(' ' * depth + '  </portal>')
    func(' ' * depth + '  <priority>portal</priority>')
    func(' ' * depth + '</portals>')


bpy.types.Object.PortalsAsCS = PortalsAsCS
