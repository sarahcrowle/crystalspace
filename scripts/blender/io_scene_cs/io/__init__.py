import bpy

import os

from .util import *
from .transform import *
from .image import *
from .renderbuffer import *
from .submesh import *
from .data import *
from .object import *
from .group import *
from .scene import *
from .world import *
from .constraint import *
from .material import ExportMaterials
from io_scene_cs.utilities import GetPreferences

from . import blenddata
from . import object_portal
from . import object_camera
from . import object_terrain


def Write(fi):
    def write(data):
        fi.write(data + '\n')
    return write


def Export(path):
    # Clean cached data
    Hierarchy.exportedFactories = []

    # Set interaction mode to 'OBJECT' mode
    editMode = False
    for ob in bpy.data.objects:
        if ob.mode == 'EDIT':
            if bpy.ops.object.mode_set.poll():
                bpy.ops.object.mode_set(mode='OBJECT')
                editMode = True
                break

    # Export Blender data to Crystal Space format
    exportAsLibrary = GetPreferences().library
    if exportAsLibrary:
        print("\nEXPORTING: " + Join(path, 'library')
              + " ====================================")
        print(
            "  All objects of the world are exported as factories in 'library' file.\n")
        ExportLibrary(path)

    else:
        print("\nEXPORTING: " + Join(path, 'world')
              + " ====================================")
        print(
            "  All scenes composing this world are exported as sectors in the 'world' file ;\n",
            " all objects of these scenes are exported as separated libraries.\n")
        ExportWorld(path)

    # Restore interaction mode
    if editMode:
        if bpy.ops.object.mode_set.poll():
            bpy.ops.object.mode_set(mode='EDIT')

    # Launch post export scripts
    import shlex
    import subprocess
    for script in GetPreferences().postExportScripts:
        if script.valid and script.enabled:
            exportPath = GetExportPath()

            args = shlex.split(script.abspath + ' ' + exportPath)
            print(args)
            output = subprocess.call(args)
            print(output)


def ExportWorld(path):
    """ Export scenes composing the world as sectors in the 'world' file,
        saved in the directory specified by 'path'. All objects of these scenes
        are exported as separated libraries in the 'factories' subfolder.
    """

    # Create the export directory for textures
    if not os.path.exists(Join(path, 'textures/')):
        os.makedirs(Join(path, 'textures/'))

    # Create the export directory for factories
    if not os.path.exists(Join(path, 'factories/')):
        os.makedirs(Join(path, 'factories/'))

    scenes = bpy.data.exportable_scenes

    # Get data about all objects composing this world
    deps = util.EmptyDependencies()
    cameras = {}
    for scene in scenes:
        MergeDependencies(deps, scene.GetDependencies())
        cameras.update(scene.GetCameras())

    # Create a 'world' file containing the xml description of the scenes
    f = open(Join(path, 'world'), 'w')
    Write(f)('<?xml version="1.0" encoding="UTF-8"?>')
    Write(f)('<world xmlns=\"http://crystalspace3d.org/xml/library\">')

    if bpy.context.scene and bpy.context.scene.world:
        bpy.context.scene.world.AsCS(Write(f), 2)

    # Export shared materials and textures in world file
    if GetPreferences().sharedMaterial:
        fmt = open(Join(path, 'materials'), 'w')
        Write(fmt)('<?xml version="1.0" encoding="UTF-8"?>')
        Write(fmt)('<library xmlns=\"http://crystalspace3d.org/xml/library\">')
        ExportMaterials(Write(fmt), 2, deps)
        Write(fmt)('</library>')
        fmt.close()
        Write(f)(' ' * 2 + '<library>%s</library>' % ('materials'))

    # Export the objects composing the world
    for typ in deps:
        if typ == 'A':
            # Animated meshes
            for name, fact in deps[typ].items():
                print('\nEXPORT OBJECT "%s" AS A CS ANIMATED MESH\n' %
                      (fact.object.uname))
                print('Writing fact', fact.uname, ':', Join(
                    path, 'factories/', fact.object.uname))
                fact.AsCSRef(Write(f), 2, 'factories/', animesh=True)
                # Export animesh factory
                fact.AsCSLib(path, animesh=True)
        elif typ == 'F':
            # General meshes
            for name, fact in deps[typ].items():
                print('\nEXPORT OBJECT "%s" AS A CS %s\n' %
                      (fact.object.uname, fact.object.b2cs.type))
                print('Writing fact', fact.uname, ':', Join(
                    path, 'factories/', fact.object.uname))
                fact.AsCSRef(Write(f), 2, 'factories/', animesh=False)
                # Export genmesh factory
                if not fact.object.b2cs.csFactRef:
                    fact.AsCSLib(path, animesh=False)
        elif typ == 'G':
            # Groups of objects
            for name, group in deps[typ].items():
                print('\nEXPORT GROUP "%s" AS A CS GENERAL MESH\n' %
                      (group.uname))
                print('Writing group', Join(path, 'factories/', group.uname))
                group.AsCSRef(Write(f), 2, 'factories/')
                # Export group of genmeshes
                group.AsCSLib(path)
        elif typ == 'M':
            # Materials
            for name, mat in deps[typ].items():
                if mat.b2cs.csMatRef and mat.b2cs.csMaterialName != 'None' and mat.b2cs.csMaterialVfs != '':
                    if mat.b2cs.csMaterialVfs not in Hierarchy.libraryReferences:
                        # Export references to CS libraries defining materials
                        Hierarchy.libraryReferences.append(
                            mat.b2cs.csMaterialVfs)
                        Write(f)(' ' * 2 + '<library>%s</library>' %
                             (mat.b2cs.csMaterialVfs))

    # Export cameras
    if cameras:
        ExportCameras(Write(f), 2, cameras)
    else:
        # Set a default camera if none is defined
        bpy.types.Object.CameraAsCS(None, Write(f), 2, bpy.context.scene)

    # Export scenes as CS sectors in the 'world' file
    print("\nEXPORT SCENES:")
    for scene in scenes:
        scene.AsCS(Write(f), 2)

    Write(f)('</world>')
    f.close()

    Hierarchy.exportedFactories = []
    Hierarchy.libraryReferences = []

    print(
        "\nEXPORTING complete ==================================================")


def ExportLibrary(path):
    """ Export all objects composing the world as factories in a single 'library' file,
        placed in the directory specified by 'path'.
    """

    # Create the export directory for textures
    if not os.path.exists(Join(path, 'textures/')):
        os.makedirs(Join(path, 'textures/'))

    # Get data about all objects composing the world
    deps = util.EmptyDependencies()
    for scene in bpy.data.scenes:
        MergeDependencies(deps, scene.GetDependencies())

    # Create a 'library' file containing the xml description of the objects
    f = open(Join(path, 'library'), 'w')
    Write(f)('<?xml version="1.0" encoding="UTF-8"?>')
    Write(f)('<library xmlns=\"http://crystalspace3d.org/xml/library\">')

    # Export references to CS libraries defining materials
    for name, mat in deps['M'].items():
        if mat.b2cs.csMatRef and mat.b2cs.csMaterialName != 'None' and mat.b2cs.csMaterialVfs != '':
            if mat.b2cs.csMaterialVfs not in Hierarchy.libraryReferences:
                Hierarchy.libraryReferences.append(mat.b2cs.csMaterialVfs)
                Write(f)(' ' * 2 + '<library>%s</library>' %
                     (mat.b2cs.csMaterialVfs))

    # Export the textures/materials/shaders of the objects
    ExportMaterials(Write(f), 2, deps)

    # Export the objects composing the Blender world
    for typ in deps:
        if typ == 'A':
            # Animated meshes
            for name, fact in deps[typ].items():
                ob = fact.object
                print('\nEXPORT OBJECT "%s" AS A CS ANIMATED MESH\n' %
                      (ob.name))
                print('Writing fact', fact.uname, 'in', Join(path, 'library'))
                # Export animesh factory
                fact.WriteCSAnimeshHeader(Write(f), 2, skelRef=False)
                # Export skeleton and animations
                if ob.type == 'ARMATURE' and ob.data.bones:
                    print(
                        'Exporting skeleton and animations: %s_rig' % (ob.name))
                    ob.data.AsCSSkelAnim(Write(f), 4, ob, dontClose=True)
                # Export mesh buffers
                fact.WriteCSMeshBuffers(
                    Write(f), 2, path, animesh=True, dontClose=True)
                Write(f)('  </meshfact>')
        elif typ == 'F':
            # General meshes
            for name, fact in deps[typ].items():
                ob = fact.object
                print('\nEXPORT OBJECT "%s" AS A CS GENERAL MESH\n' %
                      (ob.name))
                print('Writing fact', fact.uname, 'in', Join(path, 'library'))
                # Export genmesh factory
                if not fact.object.b2cs.csFactRef:
                    fact.WriteCSMeshBuffers(
                        Write(f), 2, path, animesh=False, dontClose=True)
        elif typ == 'G':
            # Groups of objects
            for name, group in deps[typ].items():
                print('\nEXPORT GROUP "%s" AS A CS GENERAL MESH\n' %
                      (group.uname))
                print('Writing group', group.uname,
                      'in', Join(path, 'library'))
                # Export group of meshes
                use_imposter = group.HasImposter()
                group.WriteCSGroup(Write(f), 2, use_imposter, dontClose=True)

    Write(f)('</library>')
    f.close()

    Hierarchy.libraryReferences = []

    print(
        "\nEXPORTING complete ==================================================")


#===== static method ExportCameras ==============================

def ExportCameras(func, depth, cameras):
    """ Export cameras sorted by names; each camera is described
        as a CS start location for the scene it belongs to
        param cameras: list of cameras and their associated scene
    """
    keylist = cameras.keys()
    for camName in sorted(keylist, key=str.lower):
        cam = cameras[camName]
        cam['camera'].CameraAsCS(func, depth, cam['scene'])
