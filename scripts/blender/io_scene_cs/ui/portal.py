import bpy
import operator

from io_scene_cs.utilities import rnaType, settings
from bpy.types import PropertyGroup


class csPortalPanel():
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        ob = bpy.context.active_object
        r = (
            ob and ob.type == 'MESH' and ob.data and not ob.b2cs.csFactRef and not ob.IsTriangleMesh())
        rd = context.scene.render
        return r and (rd.engine in cls.COMPAT_ENGINES)


class SelectDestScene(bpy.types.Operator):
    bl_idname = "object.select_dest_scene"
    bl_label = "Select scene"

    def avail_scenes(self, context):
        items = [(str(i), s.name, s.name)
                 for i, s in enumerate(bpy.data.scenes)]
        items.append((str(-1), ' NONE', 'None'))
        return sorted(items, key=operator.itemgetter(1))
    select_scenes = bpy.props.EnumProperty(
        items=avail_scenes, name="Available scenes")

    @classmethod
    def poll(cls, context):
        return context.mode == 'OBJECT'

    def execute(self, context):
        ob = context.scene.objects.active
        if int(self.select_scenes) != -1:
            ob.portal.portalDestScene = bpy.data.scenes[
                int(self.select_scenes)].name
        else:
            ob.portal.portalDestScene = ''
        ob.portal.portalDestObject = ''
        return {'FINISHED'}


class SelectDestObject(bpy.types.Operator):
    bl_idname = "object.select_dest_object"
    bl_label = "Select object"

    def avail_objects(self, context):
        items = []
        ob = context.scene.objects.active
        if ob.portal.portalDestScene != '':
            scene = bpy.data.scenes[ob.portal.portalDestScene]
            items = [(str(i), o.name, o.name)
                     for i, o in enumerate(scene.objects) if o.type == 'MESH']
            items.append((str(-1), ' NONE', 'None'))
        return sorted(items, key=operator.itemgetter(1))

    select_objects = bpy.props.EnumProperty(
        items=avail_objects, name="Available objects")

    @classmethod
    def poll(cls, context):
        return context.mode == 'OBJECT'

    def execute(self, context):
        ob = context.scene.objects.active
        scene = bpy.data.scenes[ob.portal.portalDestScene]
        if int(self.select_objects) != -1:
            ob.portal.portalDestObject = scene.objects[
                int(self.select_objects)].name
        else:
            ob.portal.portalDestObject = ''
        return {'FINISHED'}


class OBJECT_OT_csCreatePortal(bpy.types.Operator):
    bl_idname = "object.createportal"
    bl_label = "Create portal operator"
    bl_description = "Create a portal mesh in destination scene"

    def invoke(self, context, event):
        source = bpy.context.active_object
        destScene = bpy.data.scenes[source.portal.portalDestScene]
        currScene = bpy.context.scene

        # Generate portal name
        objnames = [o.name for o in bpy.data.objects]
        nametmp = "portalto" + currScene.name + "_"
        i = 0
        while True:
            i = i + 1
            name = nametmp + str(i)
            if not name in objnames:
                break

        # Create a new mesh
        new_mesh = bpy.data.meshes.new(name)

        # Create a new object
        new_obj = bpy.data.objects.new(
            name, new_mesh)   # link the mesh data to the object

        # Get current vertices and faces
        new_verts = []
        num_verts = len(source.data.vertices)
        for i in range(num_verts):
            # inverse order of vertices
            v = source.data.vertices[num_verts - i - 1]
            new_vert = []
            for c in v.co:
                new_vert.append(c)
            new_verts.append(list(new_vert))

        new_faces = []
        for face in source.data.polygons:
            i = 0
            new_face = []
            for vi in face.vertices:
                new_face.append(num_verts - vi - 1)
                i = i + 1
            new_faces.append(list(new_face))

        # Define vertices and faces of new mesh object
        new_mesh.from_pydata(new_verts, [], new_faces)

        # Set object's location and orientation
        new_obj.matrix_world = source.matrix_world

        # Update geometry
        new_obj.data.validate(False)
        new_obj.data.update(calc_edges=True, calc_tessface=False)

        # Link object into destination scene
        destScene.objects.link(new_obj)
                               # link the object into the scene

        # Define object as destination mesh of current portal
        source.portal.portalDestObject = name

        # Define destination mesh as mirror portal
        new_obj.portal.enabled = True
        new_obj.portal.portalDestScene = currScene.name
        new_obj.portal.portalDestObject = source.name

        return{'FINISHED'}


@rnaType
class OBJECT_PT_csPortal(csPortalPanel, bpy.types.Panel):
    bl_label = "Crystal Space Portal"
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        ob = context.active_object

        if ob.type == 'MESH':
            # Draw a checkbox to define current mesh object as a portal
            layout = self.layout
            row = layout.row()
            row.prop(ob.portal, "enabled")

            if ob.portal.enabled:
            # Let the user select a target scene for portal
                row = layout.row()
                split = row.split(percentage=0.5)
                colL = split.column()
                colR = split.column()
                colL.label(text='Destination scene:', icon='SCENE_DATA')
                if ob.portal.portalDestScene == '':
                    colR.operator_menu_enum(
                        "object.select_dest_scene", "select_scenes", text=SelectDestScene.bl_label)
                else:
                    colR.operator_menu_enum(
                        "object.select_dest_scene", "select_scenes", text=ob.portal.portalDestScene)

                    # Let the user select a target mesh for portal
                    row = layout.row()
                    split = row.split(percentage=0.5)
                    colL = split.column()
                    colR = split.column()
                    colL.label(text='Destination object:', icon='OBJECT_DATA')
                    if ob.portal.portalDestObject == '':
                        colR.operator_menu_enum(
                            "object.select_dest_object", "select_objects", text=SelectDestObject.bl_label)

                        # Create a "Create portal" button
                        layout.operator(
                            OBJECT_OT_csCreatePortal.bl_idname, text="Create destination portal")
                    else:
                        colR.operator_menu_enum(
                            "object.select_dest_object", "select_objects", text=ob.portal.portalDestObject)

                        # Verify that destination object still exists
                        scene = bpy.data.scenes[ob.portal.portalDestScene]
                        objects = [o.name for o in scene.objects]
                        if not ob.portal.portalDestObject in objects:
                            row = layout.row()
                            row.label(
                                text="WARNING: destination objet has been deleted!", icon='ERROR')


@settings(type='Object', attribute='portal')
class CrystalSpaceSettingsPortal(PropertyGroup):
    enabled = bpy.props.BoolProperty(
        name="Portal",
        description="Whether or not this mesh object is defined as a CS portal",
        default=False)
    portalDestObject = bpy.props.StringProperty(
        name="Destination portal",
        description="Name of the destination mesh object for this portal",
        default="")
    portalDestScene = bpy.props.StringProperty(
        name="Destination scene",
        description="Name of the destination scene of this portal",
        default="")
