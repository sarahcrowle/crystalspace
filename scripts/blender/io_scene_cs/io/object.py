import bpy
from mathutils import *
import math

import os

from .util import *
from .transform import *
from .renderbuffer import *
from .morphtarget import *
from .material import *
from io_scene_cs.utilities import GetPreferences


class Hierarchy:
    OBJECTS = {}
    exportedFactories = []
    libraryReferences = []

    def __init__(self, anObject, anEmpty):
        self.object = anObject
        self.empty = anEmpty
        # uname = anEmpty.uname if anEmpty else anObject.uname
        uname = anObject.uname
        if uname in Hierarchy.OBJECTS:
            self.id = Hierarchy.OBJECTS[uname]
        else:
            self.id = len(Hierarchy.OBJECTS)
            Hierarchy.OBJECTS[uname] = self.id

    def _Get(obj):
        return obj, [Hierarchy._Get(child) for child in obj.children]

    def Get(self):
        return Hierarchy._Get(self.object)

    @property
    def uname(self):
        return "hier" + str(self.id)

    def AsCSRef(self, func, depth=0, dirName='factories/', animesh=False):
        if self.object.b2cs.csFactRef:
            if self.object.b2cs.csFactoryName != '' and self.object.b2cs.csFactoryVfs != '':
                if self.object.b2cs.csFactoryVfs not in Hierarchy.libraryReferences:
                    Hierarchy.libraryReferences.append(
                        self.object.b2cs.csFactoryVfs)
                    func(' ' * depth + '<library>%s</library>' %
                         (self.object.b2cs.csFactoryVfs))
            else:
                print("WARNING: Object '%s' references an invalid CS factory" %
                      (self.object.uname))
        elif self.object.parent_type != 'BONE' \
                and self.object.data.name not in Hierarchy.exportedFactories:
            if animesh:
                func(' ' * depth + '<library>%s%s</library>' %
                     (dirName, self.object.uname))
            else:
                func(' ' * depth + '<library>%s%s</library>' %
                     (dirName, self.object.data.uname))

    def GetDependencies(self):
        dependencies = EmptyDependencies()

        subMeshess = {}
        objects = self.Get()

        def Gets(objs):
            for ob, children in objs:
                if ob.data not in subMeshess:
                    subMeshess[ob.data] = ob.data.GetSubMeshes()
                Gets(children)

        Gets([objects])

        for subs in subMeshess.values():
            for sub in subs:
                MergeDependencies(dependencies, sub.GetDependencies())

        return dependencies

#======== Genmesh and Animesh ============================================
    def WriteCSLibHeader(self, func, animesh=False):
        """ Write an xml header for a CS library
            param animesh: indicates if the library decribes an animesh
        """
        func('<?xml version="1.0" encoding="UTF-8"?>')
        func("<library xmlns=\"http://crystalspace3d.org/xml/library\" changeToken=\"%s\">" %
             (self.object.data.get('changeToken', 0)))

        if animesh:
            func('  <plugins>')
            func(
                '    <plugin name="animesh">crystalspace.mesh.loader.animesh</plugin>')
            func(
                '    <plugin name="animeshfact">crystalspace.mesh.loader.factory.animesh</plugin>')
            func(
                '    <plugin name="skeletonfact">crystalspace.mesh.loader.factory.skeleton</plugin>')
            func('  </plugins>')

    def WriteCSAnimeshHeader(self, func, depth, skelRef=True):
        """ Write an xml header for a CS animesh factory
            param skelRef: indicates if a skeleton library should be
                  specified for this mesh
        """
        # Reference skeleton as a CS library
        if skelRef and self.object.type == 'ARMATURE':
            func(" " * depth + "<library>factories/%s_rig</library>" %
                 (self.object.uname))
            func(" " * depth + "<library>factories/%s_model</library>" %
                 (self.object.uname))

        # Animesh factory header
        func(" " * depth + "<meshfact name=\"%s\">" % (self.object.uname))
        func(
            " " * depth + "  <plugin>crystalspace.mesh.loader.factory.animesh</plugin>")

        # Render priority and Z-mode properties
        mat = self.object.GetDefaultMaterial()
        if mat != None:
            if mat.b2cs.priority != 'object':
                func(' ' * depth + '  <priority>%s</priority>' %
                     (mat.b2cs.priority))
            if mat.b2cs.zbuf_mode != 'zuse':
                func(' ' * depth + '  <%s/>' % (mat.b2cs.zbuf_mode))

        # Shadow properties
        noshadowreceive = noshadowcast = limitedshadowcast = False
        for child in self.object.children:
            if child.type == 'MESH' and child.parent_type != 'BONE':
                if child.data.b2cs.no_shadow_receive:
                    noshadowreceive = True
                if child.data.b2cs.no_shadow_cast:
                    noshadowcast = True
                if child.data.b2cs.limited_shadow_cast:
                    limitedshadowcast = True
        if noshadowreceive:
            func(' ' * depth + '  <noshadowreceive />')
        if noshadowcast:
            func(' ' * depth + '  <noshadowcast />')
        if limitedshadowcast:
            func(' ' * depth + '  <limitedshadowcast />')

    def AsCSLib(self, path='', animesh=False, **kwargs):
        """ Export this Blender mesh as a CS library file entitled '<mesh name>' in the
            '<path>/factories' folder; it is exported as a general mesh or an animated
            mesh depending of the 'animesh' parameter; if an armature is defined for an
            animesh, it is exported as a library file entitled '<mesh name>_rig' in the
            'factories' folder
        """

        def Write(fi):
            def write(data):
                fi.write(data + '\n')
            return write

        if self.uname in Hierarchy.exportedFactories:
            print('Skipping "%s" factory export, already done' %
                  (self.uname))
            return

        if animesh:
            filePath = Join(path, 'factories/', self.object.uname)
        else:
            filePath = Join(path, 'factories/', self.object.data.uname)

        if os.path.exists(filePath):
            print('Export exists %s (%s)' % (self.object.uname, filePath))
            fafi = open(filePath, 'r')
            fafi.readline()  # Discard first line
            line = fafi.readline().strip()
            fafi.close()
            if line == "<library xmlns=\"http://crystalspace3d.org/xml/library\" changeToken=\"%s\">" % (self.object.data.get('changeToken', 0)):
                print(' No change')
                return
            else:
                print(' Updating')

        # Export mesh
        fa = open(filePath, 'w')

        self.WriteCSLibHeader(Write(fa), animesh)
        if not GetPreferences().sharedMaterial:
            objectDeps = self.object.GetDependencies()
            ExportMaterials(Write(fa), 2, objectDeps)

        if self.object.b2cs.type == 'DEFAULT':
            if animesh:
                self.WriteCSAnimeshHeader(Write(fa), 2)
            self.WriteCSMeshBuffers(
                Write(fa), 2, path, animesh, dontClose=False)
        elif self.object.b2cs.type == 'TERRAIN':
            self.object.WriteCSTerrain(Write(fa), 2, path)

        fa.close()
        Hierarchy.exportedFactories.append(self.object.data.name)

        # Export skeleton and animations
        if self.object.type == 'ARMATURE' and self.object.data.bones:
            skel = self.object
            print('Exporting skeleton and animations:',
                  Join(path, 'factories/', '%s_rig' % (skel.name)))
            fb = open(Join(path, 'factories/', '%s_rig' % (skel.name)), 'w')
            skel.data.AsCSSkelAnim(Write(fb), 2, skel, dontClose=False)
            fb.close()
            
            fb = open(Join(path, 'factories/', '%s_model' % (skel.name)), 'w')
            skel.data.AsCSSkelModel(Write(fb), 2, skel)
            fb.close()

    def WriteCSMeshBuffers(self, func, depth=0, path='', animesh=False, dontClose=False, **kwargs):
        """ Write an xml decription of a Blender object and its children of type 'MESH':
            - animesh=False: mesh and children without armature neither morph targets
              are exported as imbricated Crystal Space general mesh factories
            - animesh=True: mesh and children with armature and/or morph targets
              are exported as a Crystal Space animated mesh factory
        """

        if dontClose and self.object.data.name in Hierarchy.exportedFactories:
            print('Skipping "%s" factory export, already done' %
                  (self.object.data.uname))
            return

        # Build CS mapping buffers and submeshes for this object
        print("Building CS mapping buffers...")
        meshData = []
        subMeshess = []
        mappingBuffers = []
        mappingVertices = []
        mappingNormals = []
        scales = []
        objects = self.Get()

        def Gets(objs, indexV):
            total = 0
            for ob, children in objs:
                numCSVertices = 0
                if (ob.type == 'MESH') and (not ob.hide):
                    # Socket objects must be exported as general meshes
                    if animesh and ob.parent_type == 'BONE':
                        continue
                    indexObject = find(
                        lambda obCpy: obCpy.data == ob.data, meshData)
                    if indexObject == None:
                        # Get object's scale
                        scale = ob.GetScale()
                        scales.append(scale)
                        # Get a deep copy of the object
                        if ((animesh and ob.parent and ob.parent.type == 'ARMATURE') or
                            (not animesh and ob.parent and ob.parent.type == 'MESH'))   \
                           and ob.parent_type != 'BONE':
                            # If the mesh is child of another object (i.e. an armature in case of
                            # animesh export and a mesh in case of genmesh export), transform the
                            # copied object relatively to its parent
                            obCpy = ob.GetTransformedCopy(
                                ob.relative_matrix, True)
                        else:
                            obCpy = ob.GetTransformedCopy()

                        # Tessellate the copied mesh object
                        obCpy.data.update_faces()
                        meshData.append(obCpy)
                        # Generate mapping buffers
                        mapVert, mapBuf, norBuf = obCpy.data.GetCSMappingBuffers(
                        )
                        numCSVertices = len(mapVert)
                        if GetPreferences().enableDoublesided and obCpy.data.show_double_sided:
                            numCSVertices = 2 * len(mapVert)
                        # Generate submeshes
                        subMeshess.append(
                            obCpy.data.GetSubMeshes(obCpy.name, mapBuf, indexV))
                        mappingBuffers.append(mapBuf)
                        mappingVertices.append(mapVert)
                        mappingNormals.append(norBuf)
                        if animesh:
                            indexV += numCSVertices

                        warning = "(WARNING: double sided mesh implies duplication of its vertices)" \
                            if GetPreferences().enableDoublesided and obCpy.data.show_double_sided else ""
                        print('number of CS vertices for mesh "%s" = %s  %s' %
                              (obCpy.name, numCSVertices, warning))

                total += numCSVertices + Gets(children, indexV)
            return total

        totalVertices = Gets([objects], 0)
        print('--> Total number of CS vertices = %s' % (totalVertices))

        # Export meshes as imbricated CS general mesh factories
        import copy
        if not animesh:
            def Export(objs, d):
                for ob, children in objs:
                    export = (ob.type == 'MESH' and not ob.hide)
                    if export:
                        indexObject = find(lambda obCpy: obCpy.name[
                                           :-4] == ob.name[:len(obCpy.name[:-4])], meshData)
                        lib = "from library '%s'" % (
                            bpy.path.abspath(ob.library.filepath)) if ob.library else ''
                        print('EXPORT mesh "%s" %s' % (ob.uname, lib))
                        args = copy.deepcopy(kwargs)
                        args['meshData'] = [meshData[indexObject]]
                        args['subMeshes'] = subMeshess[indexObject]
                        args['mappingBuffers'] = [mappingBuffers[indexObject]]
                        args['mappingVertices'] = [
                            mappingVertices[indexObject]]
                        args['mappingNormals'] = [mappingNormals[indexObject]]
                        args['scales'] = [scales[indexObject]]
                        args['dontClose'] = True
                        ob.AsCSGenmeshLib(func, d, **args)
                        if dontClose:
                            Hierarchy.exportedFactories.append(
                                self.object.data.name)

                    Export(children, d + 2)
                    if export:
                        func(" " * d + "</meshfact>")

            Export([objects], depth)

        # Export meshes as a CS animated mesh factory
        else:
            args = copy.deepcopy(kwargs)
            args['meshData'] = meshData
            args['subMeshess'] = subMeshess
            args['mappingBuffers'] = mappingBuffers
            args['mappingVertices'] = mappingVertices
            args['mappingNormals'] = mappingNormals
            args['scales'] = scales
            self.AsCSAnimeshLib(func, depth, totalVertices, dontClose, **args)
            if dontClose:
                Hierarchy.exportedFactories.append(self.object.data.name)

        if not dontClose:
            func("</library>")

        # Delete working copies of the exported objects
        for obCpy in meshData:
            bpy.data.objects.remove(obCpy)

#======== Animesh ========================================================
    def AsCSAnimeshLib(self, func, depth=0, totalVertices=0, dontClose=False, **kwargs):
        """ Write an xml description of this mesh, including children meshes,
            as a CS animated mesh factory (i.e. material, render buffers, submeshes,
            bone influences, morph targets, sockets)
        """

        # Recover submeshes from kwargs
        subMeshess = kwargs.get('subMeshess', [])

        # Export the animesh factory
        func(" " * depth + "  <params>")

        # Take the first found material as default object material
        mat = self.object.GetDefaultMaterial()
        if mat != None:
            mat_name = mat.b2cs.csMaterialName if mat.b2cs.csMatRef else mat.uname
            func(" " * depth + "    <material>%s</material>" % (mat_name))
        else:
            func(" " * depth + "    <material>%s</material>" %
                 (self.object.uv_texture if self.object.uv_texture != None else 'None'))

        # Export object's render buffers
        print('EXPORT factory "%s"' % (self.object.uname))
        for buf in GetRenderBuffers(**kwargs):
            buf.AsCS(func, depth + 4, True)

        # Export bone influences
        if self.object.type == 'ARMATURE':
            # Specify skeleton name
            func(' ' * depth + '    <skeleton>%s_rig</skeleton>' %
                 (self.object.uname))

            func(' ' * depth + '    <boneinfluences>')
            for influences in self.object.data.GetBoneInfluences(**kwargs):
                func(' ' * depth + '      <bi bone="%s" weight="%.4f"/>' %
                     (influences[0], influences[1]))
            func(' ' * depth + '    </boneinfluences>')

        # Export object's submeshes
        for submeshes in subMeshess:
            for sub in submeshes:
                sub.AsCS(func, depth + 4, True)

        # Export morph targets
        for mt in GetMorphTargets(totalVertices, **kwargs):
            mt.AsCS(func, depth + 4)

        # Find sockets
        if self.object.type == 'ARMATURE':
            socketList = {}
            for ob in bpy.data.objects:
                if ob.parent == self.object and ob.parent_type == 'BONE' and ob.parent_bone and not ob.game.use_collision_bounds:
                    if ob.parent_bone not in socketList.keys():
                        socketList[ob.parent_bone] = []
                    socketList[ob.parent_bone].append(ob)

            # Export sockets
            bones = self.object.data.bones
            for boneName in socketList.keys():
                for socketObject in socketList[boneName]:
                    bone = bones[boneName]
                    bone.AsCSSocket(func, depth + 4, socketObject, self.object)

        func(' ' * depth + '    <tangents automatic="true"/>')
        func(" " * depth + "  </params>")

        if not dontClose:
            func(" " * depth + "</meshfact>")


#======== Genmesh ========================================================
def ExportCollisionBoundTransform(object, func, depth=0):
    # func(' '*depth + '<move x="0.0" y="0.065" z="0.0" />')
    # func(' '*depth + '<rotate rotx="0.44879896" />')
    pass


def ExportCollisionBounds(object, func, depth=0):
    '''
    #http://wiki.blender.org/index.php/User:Sculptorjim/Game_Engine/Physics/Collision_Bounds
    (Default)
      For Dynamic and Static objects, it is a Triangle Mesh (see below).
      For everything else, it is a Sphere (see below).
    Capsule - A cylinder with hemispherical caps, like a pill.
      Radius of the hemispheres is the greater of the x or y extent.
      Height is the z bounds
    Box - The x,y,z bounding box, as defined above.
    Sphere -
      Radius is defined by the object's scale (visible in the N properties panel) times the physics radius (can be found in Physics » Attributes » Radius.
      Note: This is the only bounds that respects the Radius option.
    Cylinder
      Radius is the greater of the x or y extent.
      Height is the z bounds.
    Cone
      Base radius is the greater of the x or y extent.
      Height is the z bounds.
    Convex Hull - Forms a shrink-wrapped, simplified geometry around the object.
      For the math, see Wikipedia's entry on Convex Hull or Wolfram's entry on Convex Hull.
      For a demo, see the image to the right, where we have sketched a hull around Suzanne's profile:
    Triangle mesh - Most expensive, but most precise. Collision will happen with all of triangulated polygons, instead of using a virtual mesh to approximate that collision.
    '''

    TYPE = 'TRIANGLE_MESH' if object.game.physics_type in [
        'DYNAMIC', 'STATIC'] else 'SPHERE'
    if object.game.use_collision_bounds:
        TYPE = object.game.collision_bounds_type

    bbox = [Vector(b) for b in object.bound_box]
    max_x = max_y = max_z = -9999999
    min_x = min_y = min_z = 99999999
    for v in bbox:
        if max_x < v.x:
            max_x = v.x
        if max_y < v.y:
            max_y = v.y
        if max_z < v.z:
            max_z = v.z

        if min_x > v.x:
            min_x = v.x
        if min_y > v.y:
            min_y = v.y
        if min_z > v.z:
            min_z = v.z

    # TODO: why do i suck at math!?
    # cone and cylinder bottoms do not align with actual object bottoms in CS
    # needs some offset, min_z - object_origin??
    old = object.location
    offset = [(min_x + max_x) * 0.5,
              (min_y + max_y) * 0.5, (min_z + max_z) * 0.5]

    dimensions = [max_x - min_x, max_y - min_y, max_z - min_z]
    print(dimensions)
    print(old)
    print(offset)
    print(min_z)
    # dimensions = object.dimensions

    func(' ' * depth + '<collider>')
    depth = depth + 2
    func(' ' * depth + '<!-- dimensions: %s %s  %s -->' %
         (TYPE, str(dimensions), str(offset)))
    if TYPE == 'CAPSULE':
        rad = max(dimensions[0], dimensions[1]) / 2.0
        height = dimensions[2]
        func(' ' * depth + '<capsule length="%f" radius="%f">' % (height, rad))
        # ExportCollisionBoundTransform(object, func, depth+1)
        func(' ' * depth + '<move x="%f" y="%f" z="%f" />' %
             (offset[0], offset[2], offset[1]))
        func(' ' * depth + '</capsule>')
    elif TYPE == 'BOX':
        func(' ' * depth + '<box x="%f" y="%f" z="%f">' %
             (dimensions[0], dimensions[2], dimensions[1]))
        # ExportCollisionBoundTransform(object, func, depth+1)
        func(' ' * depth + '<move x="%f" y="%f" z="%f" />' %
             (offset[0], offset[2], offset[1]))
        func(' ' * depth + '</box>')
    elif TYPE == 'SPHERE':
        # TODO: does not do what it says above
        rad = max(max(dimensions[0], dimensions[1]), dimensions[2]) / 2.0
        func(' ' * depth + '<sphere radius="%f">' % (rad))
        # ExportCollisionBoundTransform(object, func, depth+1)
        func(' ' * depth + '<move x="%f" y="%f" z="%f" />' %
             (offset[0], offset[2], offset[1]))
        func(' ' * depth + '</sphere>')
    elif TYPE == 'CYLINDER':
        rad = max(dimensions[0], dimensions[1]) / 2.0
        height = dimensions[2]
        func(' ' * depth + '<cylinder length="%f" radius="%f">' %
             (height, rad))
        # ExportCollisionBoundTransform(object, func, depth+1)
        func(' ' * depth + '<move x="%f" y="%f" z="%f" />' %
             (offset[0], offset[2], offset[1]))
        func(' ' * depth + '</cylinder>')
    elif TYPE == 'CONE':
        rad = max(dimensions[0], dimensions[1]) / 2.0
        height = dimensions[2]
        func(' ' * depth + '<cone length="%f" radius="%f">' % (height, rad))
        # ExportCollisionBoundTransform(object, func, depth+1)
        func(' ' * depth + '<move x="%f" y="%f" z="%f" />' %
             (offset[0], offset[2], offset[1]))
        func(' ' * depth + '<rotate><rotx>1.57</rotx></rotate>')
        func(' ' * depth + '</cone>')
    elif TYPE == 'CONVEX_HULL':
        func(' ' * depth + '<convexmesh />')
    elif TYPE == 'TRIANGLE_MESH':
        func(' ' * depth + '<concave />')

    depth = depth - 2
    func(' ' * depth + '</collider>')


def AsCSGenmeshLib(self, func, depth=0, **kwargs):
    """ Write an xml description of this mesh as a CS general mesh factory
        (i.e. header, material, render buffers and submeshes)
    """

    # Write genmesh header
    func(' ' * depth + '<meshfact name=\"%s\">' % (self.data.uname))
    func(' ' * depth +
         '  <plugin>crystalspace.mesh.loader.factory.genmesh</plugin>')
    if self.data.b2cs.use_imposter:
        func(
            ' ' * depth + '  <imposter range="100.0" tolerance="0.4" camera_tolerance="0.4" shader="lighting_imposter"/>')
    mat = self.GetDefaultMaterial()
    if mat != None:
        if mat.b2cs.priority != 'object':
            func(' ' * depth + '  <priority>%s</priority>' % (mat.priority))
        if mat.b2cs.zbuf_mode != 'zuse':
            func(' ' * depth + '  <%s/>' % (mat.zbuf_mode))
    if self.data.b2cs.no_shadow_receive:
        func(' ' * depth + '  <noshadowreceive />')
    if self.data.b2cs.no_shadow_cast:
        func(' ' * depth + '  <noshadowcast />')
    if self.data.b2cs.limited_shadow_cast:
        func(' ' * depth + '  <limitedshadowcast />')
    if self.data.b2cs.lighter2_vertexlight:
        func(
            ' ' * depth + '  <key name="lighter2" editoronly="yes" vertexlight="yes" />')
    if not self.data.b2cs.lighter2_selfshadow:
        func(
            ' ' * depth + '  <key name="lighter2" editoronly="yes" noselfshadow="yes" />')
    if self.data.b2cs.lighter2_lmscale > 0.0:
        func(' ' * depth + '  <key name="lighter2" editoronly="yes" lmscale="%f" />' %
             (self.data.b2cs.lighter2_lmscale))
    func(' ' * depth + '  <params>')

    # Recover submeshes from kwargs
    subMeshes = kwargs.get('subMeshes', [])

    # Take the first found material as default object material
    if mat != None:
        if mat.b2cs.csMatRef:
            func(" " * depth + "    <material>%s</material>" %
                 (mat.b2cs.csMaterialName))
        else:
            func(" " * depth + "    <material>%s</material>" % (mat.uname))

            if not mat.HasDiffuseTexture() and mat.uv_texture != 'None':
                func(' ' * depth + '    <shadervar type="texture" name="tex diffuse">%s</shadervar>' %
                     (mat.uv_texture))

    else:
        func(" " * depth + "    <material>%s</material>" %
             (self.uv_texture if self.uv_texture != None else 'None'))

    # Export mesh's render buffers
    for buf in GetRenderBuffers(**kwargs):
        buf.AsCS(func, depth + 4, False)

    # Export mesh's submeshes
    if len(subMeshes) == 1:
        subMeshes[0].AsCSTriangles(func, depth + 4, False)
    else:
        for sub in subMeshes:
            sub.AsCS(func, depth + 4, False)

    if 'meshData' in kwargs:
        meshData = kwargs['meshData']
        # We know there is only one mesh in the data.
        if meshData[0].data.use_auto_smooth:
            func(" " * depth + "    <autonormals/>")

    func(' ' * depth + '  </params>')

    # Physics
    print("physics %s" % (self.game.physics_type))
    if self.hasSupportedPhysicsEnabled():  # and bpy.context.scene.HasBulletPhysicsEnabled():
        func(' ' * depth + ' <addon plugin="crystalspace.physics.loader">')
        if self.game.physics_type == 'RIGID_BODY':
            mass = self.game.mass
            func(' ' * depth + '   <rigidbody mass="%f" friction="1.0" elasticity="0.8">' %
                 (mass))  # TODO
            ExportCollisionBounds(self, func, depth + 5)
            func(' ' * depth + '   </rigidbody>')

        elif self.game.physics_type == 'SOFT_BODY':
            mass = self.game.mass
            friction = self.game.soft_body.dynamic_friction
            stiffness = self.game.soft_body.linear_stiffness
            func(' ' * depth + '   <softbody mass="%f" friction="%f">' %
                 (mass, friction))
            func(' ' * depth + '     <meshfact duplication="none" />')
            func(' ' * depth + '     <stiffness linear="%f" angular="1.0" volume="1.0" />' %
                 (stiffness))  # TODO
            func(' ' * depth + '   </softbody>')

        func(' ' * depth + ' </addon>')

    # Don't close 'meshfact' flag if another mesh object is defined as
    # an imbricated genmesh factory of this factory
    if not kwargs.get('dontClose', False):
        func(' ' * depth + '</meshfact>')

bpy.types.Object.AsCSGenmeshLib = AsCSGenmeshLib


#======== Object =========================================================

# Property defining an UV texture's name for a mesh ('None' if not defined)
bpy.types.Object.uv_texture = bpy.props.StringProperty(default='None')


def ObjectAsCS(self, func, depth=0, **kwargs):
    """ Write a reference to this object (as part of a sector in the 'world' file);
        only mesh, armature, group and lamp objects are described
    """
    name = self.uname
    if 'name' in kwargs:
        name = kwargs['name'] + ':' + name

    if self.type == 'MESH' and self.portal.enabled:
        self.PortalsAsCS(func, depth)

    elif self.type == 'MESH' and self.b2cs.type == 'TERRAIN':
        self.TerrainAsCS(func, depth, **kwargs)

    elif self.type == 'MESH':
        isValidRef = self.b2cs.csFactRef and self.b2cs.csFactoryName != '' and self.b2cs.csFactoryVfs != ''
        if self.b2cs.csFactRef and not isValidRef:
            return

        if isValidRef or (len(self.data.vertices) != 0 and len(self.data.all_faces) != 0):
            # Temporary disable of meshref support because of incompatibility
            # issues with lighter2 that needs to be genmesh aware in order to
            # pack the lightmaps per submeshes
            # func(' '*depth +'<meshref name="%s_object">'%(self.uname))
            # func(' '*depth +'  <factory>%s</factory>'%(self.data.uname))
            if isValidRef:
                # Only use meshref when object is defined as a reference to a
                # CS factory
                func(' ' * depth + '<meshref name="%s_object">' % (name))
                func(' ' * depth + '  <factory>%s</factory>' %
                     (self.b2cs.csFactoryName))
            else:
                func(' ' * depth + '<meshobj name="%s_object">' % (name))
                if self.parent and self.parent.type == 'ARMATURE':
                    func(
                        ' ' * depth + '  <plugin>crystalspace.mesh.loader.animesh</plugin>')
                else:
                    func(
                        ' ' * depth + '  <plugin>crystalspace.mesh.loader.genmesh</plugin>')
                func(' ' * depth + '  <params>')
                func(' ' * depth + '    <factory>%s</factory>' %
                     (self.data.uname))
                func(' ' * depth + '  </params>')

            if self.parent and self.parent_type == 'BONE':
                matrix = self.matrix_world
            else:
                if 'transform' in kwargs:
                    matrix = kwargs['transform']
                else:
                    matrix = self.relative_matrix

            MatrixAsCS(matrix, func, depth + 2, noScale=not isValidRef)

            if isValidRef:
                func(' ' * depth + '</meshref>')
            else:
                func(' ' * depth + '</meshobj>')

            # Process array modifiers
            self.ExportArrayModifier(func, depth, **kwargs)

    elif self.type == 'ARMATURE':
        # Temporary disable of meshref support because of incompatibility
        # issues with lighter2 that needs to be genmesh aware in order to
        # pack the lightmaps per submeshes
        # func(' '*depth +'<meshref name="%s_object">'%(name))
        # func(' '*depth +'  <factory>%s</factory>'%(name))

        func(' ' * depth + '<meshobj name="%s_object">' % (self.uname))
        func(' ' * depth +
             '  <plugin>crystalspace.mesh.loader.animesh</plugin>')
        func(' ' * depth + '  <params>')
        func(' ' * depth + '    <factory>%s</factory>' % (self.uname))
        func(' ' * depth + '  </params>')

        matrix = self.relative_matrix
        if 'transform' in kwargs:
            matrix = kwargs['transform'] * matrix
        MatrixAsCS(matrix, func, depth + 2)

        # func(' '*depth +'</meshref>')
        func(' ' * depth + '</meshobj>')

    elif self.type == 'LAMP':

        color = Color (self.data.color)

        # Removal of the gamma color correction made by Blender
        import math
        color[0] = math.sqrt(color[0]);
        color[1] = math.sqrt(color[1]);
        color[2] = math.sqrt(color[2]);

        func(' ' * depth + '<light name="%s">' % (name))

        # Attenuations types
        if self.data.type == 'POINT' or self.data.type == 'SPOT' :

            if self.data.falloff_type == 'LINEAR_QUADRATIC_WEIGHTED':
                if self.data.quadratic_attenuation < 0.000001:
                    color *= self.data.energy
                    color *= self.data.distance
                    func(' ' * depth + '  <attenuation c="%f" l="%f" q="%f">clq</attenuation>' %
                         (self.data.distance, self.data.linear_attenuation, 0.0))

                elif self.data.linear_attenuation < 0.000001:
                    color *= self.data.energy
                    color *= self.data.distance * self.data.distance
                    func(' ' * depth + '  <attenuation c="%f" l="%f" q="%f">clq</attenuation>' %
                         (self.data.distance * self.data.distance, 0.0, self.data.quadratic_attenuation))

                else:
                    print("WARNING: Composition of linear and quadratic terms are not allowed for the falloff type of the light '%s'" %
                          (name))

                    # Fallback on either linear or quadratic
                    if self.data.linear_attenuation > self.data.quadratic_attenuation:
                        print("Falling back to linear attenuation")
                        color *= self.data.energy
                        color *= self.data.distance
                        func(' ' * depth + '  <attenuation c="%f" l="%f" q="%f">clq</attenuation>' %
                             (self.data.distance, self.data.linear_attenuation, 0.0))

                    else:
                        print("Falling back to quadratic attenuation")
                        color *= self.data.energy
                        color *= self.data.distance * self.data.distance
                        func(' ' * depth + '  <attenuation c="%f" l="%f" q="%f">clq</attenuation>' %
                             (self.data.distance * self.data.distance, 0.0, self.data.quadratic_attenuation))

            elif self.data.falloff_type == 'INVERSE_LINEAR':
                color *= self.data.energy
                color *= self.data.distance
                func(' ' * depth + '  <attenuation c="%f" l="%f" q="%f">clq</attenuation>' %
                     (self.data.distance, 1.0, 0.0))

            elif self.data.falloff_type == 'INVERSE_SQUARE':
                color *= self.data.energy
                color *= self.data.distance * self.data.distance
                func(' ' * depth + '  <attenuation c="%f" l="%f" q="%f">clq</attenuation>' %
                     (self.data.distance * self.data.distance, 0.0, 1.0))

            elif self.data.falloff_type == 'CONSTANT':
                func(' ' * depth + '  <attenuation>none</attenuation>')

            else:
                print("WARNING: The falloff type of the light '%s' is not supported" % (name))

            if self.data.use_sphere:
                func(' ' * depth + '  <radius>%f</radius>' % (self.data.distance))

            else:
                func(' ' * depth + '  <radius>%f</radius>' % (self.data.distance * 4))

        # Conversion of the light transform from Blender to CS:
        # - In Blender, a light with an identity transform will look down although
        #   it will look backward in CS (hence a rotation of 90 degree).
        # - In Blender, the rotation part of the transform appears to be from lamp to
        #   world, although the translation is from world to lamp. Hence an inversion
        #   of the rotation part.
        matrix = self.relative_matrix
        if 'transform' in kwargs:
            matrix = kwargs['transform'] * matrix

        import mathutils
        import math
        rotationX = mathutils.Matrix.Rotation(math.radians(90.0), 4, 'X')
        origin = matrix.to_translation()
        # Isolate the rotation part of the initial matrix in order to ensure that it
        # is invertible
        matrix = rotationX * matrix.to_quaternion().inverted().to_matrix().to_4x4()
        matrix.translation = origin

        # Shadow properties
        # For some reason, the 'use_shadow' property does not appear for the 'HEMI' lamp type
        if self.data.type != 'HEMI' and not self.data.use_shadow:
            func(' ' * depth + '  <noshadows />')

        # Other specific light type properties
        if self.data.type == 'SPOT':
            func(' ' * depth + '  <type>spot</type>')

            MatrixAsCS(matrix, func, depth + 2,
                       noScale=True, noTranslation=False)

            import math  # math.radians(90), math.degrees(1.5707963)
            outer = math.degrees(self.data.spot_size) / \
                2  # Blender specifies the lit angle, CS has outer to center as angle
            inner = outer - (outer * self.data.spot_blend)
            func(' ' * depth + '  <spotlightfalloff inner="%f" outer="%f" />' %
                 (inner, outer))

        elif self.data.type == 'SUN':
            color *= self.data.energy
            func(' ' * depth + '  <type>directional</type>')
            # TODO: A radius of 10 000 is not always suited
            func(' ' * depth + '  <radius>10000.0</radius>')
            MatrixAsCS(matrix, func, depth + 2,
                       noScale=True, noTranslation=False)

        elif self.data.type == 'POINT':
            # Flip Y and Z axis.
            func(' ' * depth + '  <center x="%f" z="%f" y="%f" />' %
                 tuple(matrix.to_translation()))

        else:
            print("WARNING: The type of the light '%s' is not supported" % (name))
        
        func(' ' * depth + '  <color red="%f" green="%f" blue="%f" />' % tuple(color))

        # Trimesh data of the light
        if len(self.children):  # TODO: only support first child, perhaps merge the meshes?
            data = self.children[0].data
            data.calc_tessface()
                               # Compute and update the list of tessellated
                               # faces (faces of 3 or 4 vertices)
            func(' ' * depth + '<trimesh>')
            func(' ' * depth + ' <id>clip</id>')
            func(' ' * depth + ' <mesh>')
            for v in data.vertices:
                pos = v.co
                # Flip Y and Z axis.
                func(' ' * depth + '   <v x="%f" y="%f" z="%f" />' %
                     (pos.x, pos.z, pos.y))

            for face_num, face in enumerate(data.tessfaces):
                indices = [v for v in face.vertices]
                if len(indices) == 3:
                    func(' ' * depth + '   <t v1="%d" v2="%d" v3="%d" />' %
                         (indices[2], indices[1], indices[0],))
                else:
                    func(' ' * depth + '   <t v1="%d" v2="%d" v3="%d" />' %
                         (indices[2], indices[1], indices[0],))
                    func(' ' * depth + '   <t v1="%d" v2="%d" v3="%d" />' %
                         (indices[3], indices[2], indices[0],))

            func(' ' * depth + ' </mesh>')
            func(' ' * depth + '</trimesh>')

        func(' ' * depth + '</light>')

    elif self.type == 'EMPTY':
        if self.dupli_type == 'GROUP' and self.dupli_group:
            if self.dupli_group.b2cs.doMerge:
                self.dupli_group.AsCS(
                    func, depth, transform=self.matrix_world)
            else:
                for ob in self.dupli_group.objects:
                    if not ob.parent:
                        ob.AsCS(
                            func, depth, transform=self.matrix_world, name=self.uname)
        else:
            func(' ' * depth + '<node name="%s">' % (name))
            func(' ' * depth + '  <position x="%f" z="%f" y="%f" />' %
                 tuple(self.matrix_world.to_translation()))
            func(' ' * depth + '</node>')

        # TODO: really put everything on the top level??
        # Handle children: translate to top level.
        for obj in self.children:
            if obj.IsExportable():
                obj.AsCS(
                    func, depth, transform=self.matrix_world, name=self.uname)

    elif self.type != 'CAMERA' and self.type != 'CURVE':
        print("WARNING: Object '%s' of type '%s' is not supported!" %
              (self.uname, self.type))

bpy.types.Object.AsCS = ObjectAsCS


def IsExportable(self):
    """ Check if this object can be exported
    """
    # Hidden objects are never exported
    # (except for empty objects, which are composed of other objects)
    if self.hide and self.type != 'EMPTY':
        return False

    # Armature objects are exported if at least one of
    # its child meshes is visible
    if self.type == 'ARMATURE':
        for child in self.children:
            if child.parent_type != 'BONE' and not child.hide:
                return True
        return False

    # Mesh objects are exported as individual mesh factories if
    # - they are not submeshes of a visible armature,
    # - they are not submeshes of a visible mesh,
    # - they are not a special mesh (viscull, shadow, collission)
    # - they have a non null number of vertices and faces,
    # - they are socket objects
    if self.type == 'MESH':
        def IsChildOfExportedFactory(ob):
            ''' Test if ob is child of a visible mesh or armature
                (and not linked by a socket to a bone)
            '''
            if ob.parent_type == 'BONE' and ob.parent_bone:
                return False
            if ob.parent:
                return (ob.parent.type == 'ARMATURE' and not ob.parent.hide) or \
                       (ob.parent.type == 'MESH' and not ob.parent.hide)
            return False

        if not IsChildOfExportedFactory(self) and not self.IsTriangleMesh() \
                and len(self.data.vertices) != 0 and len(self.data.all_faces) != 0:
            return True
        return False

    # The export of Empty objects depends of their components
    if self.type == 'EMPTY':
        # Groups of objects are exported if the group itself and
        # all its composing elements are visible
        if self.dupli_type == 'GROUP' and self.dupli_group:
            if self.hide or not self.dupli_group.CheckVisibility():
                return False

            # Check if this group is part of a merging group
            mergingGroup = self.hasMergingGroup()
            if mergingGroup:
                # This group is exported only if all objects of the
                # merging group are visible
                for gr in mergingGroup.objects:
                    if gr != self:
                        if gr.hide:
                            return False
                        if not gr.dupli_group.CheckVisibility():
                            return False
        else:
            # Empty objects which are not groups are exported as map nodes
            # (notice that each of their components can be individually exported)
            return True

    # All other types of objects are always exportable (if they are visible)
    return True

bpy.types.Object.IsExportable = IsExportable


def ObjectDependencies(self, empty=None):
    """ Get the dependencies of this object and its children, i.e. textures,
        materials, armatures, meshes and groups associated with it
    """
    dependencies = EmptyDependencies()
    if self.type == 'ARMATURE':
        # Object with skeleton ==> 'A' type (animesh)
        if self.children:
            hier = Hierarchy(self, empty)
            print('ObjectDependencies: ', hier.uname, '-', self.uname)
            dependencies['A'][hier.uname] = hier
            MergeDependencies(dependencies, self.GetMaterialDependencies())

    elif self.type == 'MESH':
        if self.b2cs.type == 'TERRAIN':
            hier = Hierarchy(self, empty)
            dependencies['F'][hier.uname] = hier
            MergeDependencies(
                dependencies, self.GetTerrainMaterialDependencies())
        elif self.data.shape_keys:
            # Mesh with morph targets ==> 'A' type (animesh)
            hier = Hierarchy(self, empty)
            print('ObjectDependencies: ', hier.uname, '-', self.uname)
            dependencies['A'][hier.uname] = hier
            # Material of the mesh ==> 'M' type
            # Associated textures  ==> 'T' type
            MergeDependencies(dependencies, self.GetMaterialDependencies())
        else:
            # Mesh without skeleton neither morph target
            # or child of a bone (attached by socket) ==> 'F' type (genmesh)
            hier = Hierarchy(self, empty)
            print('ObjectDependencies: ', hier.uname, '-', self.uname)
            dependencies['F'][hier.uname] = hier
            # Material of the mesh ==> 'M' type
            # Associated textures  ==> 'T' type
            MergeDependencies(dependencies, self.GetMaterialDependencies())

    elif self.type == 'EMPTY':
        if self.dupli_type == 'GROUP' and self.dupli_group:
            # Group of objects ==> 'G' type
            if self.dupli_group.b2cs.doMerge:
                dependencies['G'][self.dupli_group.uname] = self.dupli_group
                MergeDependencies(
                    dependencies, self.dupli_group.GetDependencies())
            else:
                for ob in [ob for ob in self.dupli_group.objects if not ob.parent]:
                    MergeDependencies(dependencies, ob.GetDependencies(self))

    return dependencies

bpy.types.Object.GetDependencies = ObjectDependencies


def GetDefaultMaterial(self, notifications=True):
    """ Get the default material of this armature or mesh object
    """
    # Initialize default material with 'None'
    mat = None

    # Armature object
    if self.type == 'ARMATURE':
        # Take the first found material among children as default object
        # material
        foundMaterial = False
        for child in self.children:
            if child.type == 'MESH' and child.parent_type != 'BONE':
                mat = child.GetDefaultMaterial(notifications)
                if mat != None:
                    foundMaterial = True
                    break
        if not foundMaterial and notifications:
            print('WARNING: armature object "%s" has no child with texture coordinates' %
                  (self.uname))
    elif self.type == 'MESH':
        # Mesh object
        if self.data.GetFirstMaterial():
            # Take the first defined material as default object material
            mat = self.data.GetFirstMaterial()
        elif notifications:
            if len(self.data.uv_textures) == 0:
                print('WARNING: mesh object "%s" has no texture coordinates' %
                      (self.uname))

    return mat

bpy.types.Object.GetDefaultMaterial = GetDefaultMaterial


def GetMaterialDeps(self):
    """ Get materials and textures associated with this object
    """
    dependencies = EmptyDependencies()
    if self.type == 'MESH':
        # Material of the mesh ==> 'M' type
        # and associated textures ==> 'T' type
        for mat in self.materials:
            dependencies['M'][mat.uname] = mat
            MergeDependencies(dependencies, mat.GetDependencies())

        # Search for a possible diffuse texture that overlays underlying
        # materials
        if self.data and self.data.uv_textures and self.data.uv_textures.active:
            # UV texture of the mesh ==> 'T' type
            for index, facedata in enumerate(self.data.uv_textures.active.data):
                if facedata.image:
                    if facedata.image.uname not in dependencies['T'].keys():
                        dependencies['T'][
                            facedata.image.uname] = facedata.image

                    material = self.data.GetMaterial(
                        self.data.all_faces[index].material_index)
                    if material:
                        material.uv_texture = facedata.image.uname
                    else:
                        # Create a material if the mesh has a texture but no
                        # material
                        dependencies['TM'][
                            facedata.image.uname] = facedata.image
                        if self.uv_texture == 'None':
                            self.uv_texture = facedata.image.uname

    return dependencies

bpy.types.Object.GetMaterialDeps = GetMaterialDeps


def GetMaterialDependencies(self):
    """ Get materials and textures associated with this object
        and its children
    """
    dependencies = self.GetMaterialDeps()
    for child in self.children:
        if child.type == 'MESH' and child.parent_type != 'BONE':
            MergeDependencies(dependencies, child.GetMaterialDependencies())
    return dependencies

bpy.types.Object.GetMaterialDependencies = GetMaterialDependencies


def HasImposter(self):
    """ Indicates if this object uses an imposter
    """
    if self.type == 'MESH':
        if self.data and self.data.b2cs.use_imposter:
            return True
    elif self.type == 'EMPTY':
        if self.dupli_type == 'GROUP' and self.dupli_group:
            if self.dupli_group.HasImposter():
                return True
    return False

bpy.types.Object.HasImposter = HasImposter


def GetScale(self):
    """ Get object scale
    """
    if self.parent and self.parent_type == 'BONE':
        pscale = self.parent.scale
        oscale = self.scale
        scale = Vector(
            (pscale[0] * oscale[0], pscale[1] * oscale[1], pscale[2] * oscale[2]))
    elif self.parent and self.parent.type == 'ARMATURE':
        scale = self.parent.scale
    else:
        scale = self.scale

    return scale

bpy.types.Object.GetScale = GetScale


def GetTransformedCopy(self, matrix=None, animesh=False):
    """ Get a deep copy of this object, transformed by matrix,
        with applied modifiers
    """
    # Make a copy of the current object
    obCpy = self.copy()
    # Make a deep copy of the object data block
    obCpy.data = self.data.copy()
    # Transform the copied object
    if matrix:
        obCpy.data.transform(matrix)
        obCpy.data.calc_normals()

    if animesh or len(self.users_scene) == 0 or self.type != 'MESH':
        return obCpy

    # Don't apply modifiers if the CS property 'array_as_meshobj' is set and
    # the object contains only 'FIXED_COUNT' array modifiers:
    # this type of modifier is exported as instances of object factory in 'world' file
    # (cfr. ExportArrayModifier() )
    if self.data.b2cs.array_as_meshobj:
        arrays = [mod for mod in obCpy.modifiers
                  if mod.type == 'ARRAY' and mod.fit_type == 'FIXED_COUNT']
        if len(arrays) == len(obCpy.modifiers):
            return obCpy

    # Arbitrary take the first scene containing current object
    scene = self.users_scene[0]
    # Create a deep copy of mesh data block with applied modifiers
    modifiedObject = obCpy.copy()
    modifiedObject.data = obCpy.to_mesh(scene, True, 'PREVIEW')
    return modifiedObject

bpy.types.Object.GetTransformedCopy = GetTransformedCopy


def ExportArrayModifier(self, func, depth, **kwargs):
    """ If current object contains only array modifiers with fixed count copies,
        export each object copy as a factory instance ('meshobj')
    """
    # Don't export array copies of this mesh if CS property 'array_as_meshobj'
    # is not set
    if not self.data.b2cs.array_as_meshobj:
        return

    # If stack contains other modifiers than arrays with fixed number of copies,
    # don't export the array copies (they have been already applied to the mesh
    # by method GetTransformedCopy() )
    arrays = [mod for mod in self.modifiers
              if mod.type == 'ARRAY' and mod.fit_type == 'FIXED_COUNT' and mod.count > 1]
    if len(arrays) != len(self.modifiers):
        return

    # Export all array modifiers of the stack
    for modifier in self.modifiers:

        # Get transformation matrix of base object
        matrix = self.relative_matrix
        if 'transform' in kwargs:
            matrix = kwargs['transform'] * matrix

        # Calculate array transformation:
        # - object offset
        objoffset = Matrix.Identity(4)
        if modifier.use_object_offset:
            objoffset = modifier.offset_object.relative_matrix * \
                matrix.inverted()

        # - constant offset
        offset = Matrix.Identity(4)
        if modifier.use_constant_offset:
            cstoffset = modifier.constant_offset_displace
            m = Matrix.Translation(cstoffset)
            offset = m * offset

        # - relative offset
        if modifier.use_relative_offset:
            scaling = modifier.relative_offset_displace
            if scaling != [-1.0, -1.0, -1.0]:
                # get bounding box of base object
                bbsize = self.GetBBoxSize()
                m = Matrix.Identity(4)
                for i in range(3):
                    if scaling[i] != 0.0:
                        m[i][3] = scaling[i] * bbsize[i]
                offset = m * offset

        # Create instances of base object respecting the array modifier
        # (since an instance of mesh factory has already been exported,
        # only N-1 copies of the mesh are generated)
        tt = Matrix.Identity(4)
        ot = matrix
        mtot = matrix

        for cnt in range(modifier.count - 1):
            func(' ' * depth + '<meshobj name="%s_array_object_%i">' %
                 (self.uname, cnt + 1))
            func(
                ' ' * depth + '  <plugin>crystalspace.mesh.loader.genmesh</plugin>')
            func(' ' * depth + '  <params>')
            func(' ' * depth + '    <factory>%s</factory>' % (self.data.uname))
            func(' ' * depth + '  </params>')

            # Calculate transformation matrix of mesh factory instance
            loc, rot, scale = DecomposeMatrix(mtot)
            roffset = Matrix.Translation((rot * offset).to_translation())
            tt = roffset * tt
            ot = objoffset * ot
            mtot = tt * ot

            MatrixAsCS(mtot, func, depth + 2)
            func(' ' * depth + '</meshobj>')

    # TODO: process other types of ARRAY modifiers (fit_type = 'FIT_LENGTH' or 'FIT_CURVE')
    # and other ARRAY options

bpy.types.Object.ExportArrayModifier = ExportArrayModifier


def GetBBoxSize(self):
    """ Return the size of object bounding box in a 3 dimensional vector
        (no modifier is applied to the object)
    """

    if not self.type == 'MESH' or not self.data or len(self.data.vertices) == 0:
        return

    posmin = Vector(self.data.vertices[0].co)
    posmax = Vector(self.data.vertices[0].co)
    for vi in self.data.vertices:
        for i in range(3):
            if vi.co[i] < posmin[i]:
                posmin[i] = vi.co[i]
            if vi.co[i] > posmax[i]:
                posmax[i] = vi.co[i]
    BBsize = [abs(posmax[0] - posmin[0]), abs(
        posmax[1] - posmin[1]), abs(posmax[2] - posmin[2])]
    return BBsize

bpy.types.Object.GetBBoxSize = GetBBoxSize


def GetTriangleMesh(self, id):
    '''
    id is one of
    - viscull
    - collission
    - shadow
    '''
    for ob in bpy.data.objects:
        for contraint in ob.constraints:
            if contraint.GetConstraintType() == id and contraint.target == self:
                return ob

    return None

bpy.types.Object.GetTriangleMesh = GetTriangleMesh


def IsTriangleMesh(self):
    for contraint in self.constraints:
        if contraint.GetConstraintType():
            return True
    return False

bpy.types.Object.IsTriangleMesh = IsTriangleMesh


def MakeThisATriangleMesh(self, id, target):
    constraint = self.constraints.new('COPY_TRANSFORMS')
    constraint.name = id
    constraint.target = target
    self.draw_type = 'WIRE'

    return None

bpy.types.Object.MakeThisATriangleMesh = MakeThisATriangleMesh


def UnMakeThisATriangleMesh(self, id):
    for constraint in self.constraints:
        if constraint.GetConstraintType() == id:
            self.constraints.remove(constraint)
    self.draw_type = 'TEXTURED'

    return None

bpy.types.Object.UnMakeThisATriangleMesh = UnMakeThisATriangleMesh


def hasSupportedPhysicsEnabled(self):
    return self.game.physics_type in ['RIGID_BODY', 'SOFT_BODY']

bpy.types.Object.hasSupportedPhysicsEnabled = hasSupportedPhysicsEnabled
