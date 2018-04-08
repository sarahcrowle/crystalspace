
import bpy
import operator

from io_scene_cs.utilities import rnaType, GetPreferences, SHADERSETS, GetShaderSetName, settings, RENDERPRIORITIES, ZBUFFERMODES
from bpy.types import PropertyGroup

from io_scene_cs.ui.idref import IDRefContainer, IDRefProperty, draw_idref


class csMaterialPanel():
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "material"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        rd = context.scene.render
        r = (context.material or context.object)
        return r and (rd.engine in cls.COMPAT_ENGINES)


class SelectMaterialRef(bpy.types.Operator):
    bl_idname = "material.select_mat_ref"
    bl_label = "Select CS material"

    def avail_materials(self, context):
        items = [(str(i), m.name, m.vfs)
                 for i, m in enumerate(GetPreferences().MaterialRefs)]
        items.append((str(-1), ' NONE', 'None'))
        return sorted(items, key=operator.itemgetter(1))
    select_material = bpy.props.EnumProperty(
        items=avail_materials, name="Available CS materials")

    @classmethod
    def poll(cls, context):
        return context.material != None

    def execute(self, context):
        mat = context.material
        if int(self.select_material) != -1:
            mat.b2cs.csMaterialName = GetPreferences().MaterialRefs[
                int(self.select_material)].name
            mat.b2cs.csMaterialVfs = GetPreferences().MaterialRefs[
                int(self.select_material)].vfs
        else:
            mat.b2cs.csMaterialName = 'None'
            mat.b2cs.csMaterialVfs = ''
        return {'FINISHED'}

NAMES = {
    'use_map_color_diffuse': 'tex diffuse', 'use_map_normal': 'tex normal',
    'use_map_displacement': 'tex height', 'use_map_specular': 'tex specular', }
PROPS = ['use_map_color_diffuse', 'use_map_normal',
         'use_map_displacement', 'use_map_specular', ]


@rnaType
class MATERIAL_PT_B2CS__context_material(csMaterialPanel, bpy.types.Panel):
    bl_label = "Crystal Space Material"
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        layout = self.layout

        mat = context.material

        if mat:
            #------------------------------------------------------------------
            textures = {}

            for i, slot in enumerate(mat.texture_slots):
                if slot:
                    for p in PROPS:
                        if getattr(slot, p):
                            if slot.texture.type == 'IMAGE':
                                textures[p] = (i, slot.texture)

            box = layout.box()
            row = box.row()
            row.label("Textures")
            row.menu('MATERIAL_OT_textures_menu',
                     text="", icon='DOWNARROW_HLT')

            for p in PROPS:
                i, tex = textures.get(p, (None, None))
                if tex:
                    row = box.row()
                    col1 = row.column(align=True)
                    col1.label('', icon_value=layout.icon(tex))
                    col2 = row.column(align=True).row()
                    # col2.alignment = 'RIGHT'
                    c = col2.column(align=True)
                    c.prop(tex, 'image', text=NAMES[p])
                    c = col2.column(align=True)
                    op = c.operator(
                        'material.texture_remove', text="", icon='ZOOMOUT')
                    op.index = i

            #------------------------------------------------------------------
            box = layout.box()
            row = box.row()
            for prop in ['diffuse_color', 'specular_color']:
                row = box.row()
                col1 = row.column(align=True)
                col1.label(prop)
                col2 = row.column(align=True).row()
                col2.prop(mat, prop, text='')
            #------------------------------------------------------------------
            box = layout.box()
            row = box.row()
            row.label("Custom shader variables")

            row.menu('MATERIAL_OT_shadervars_menu',
                     text="", icon='DOWNARROW_HLT')

            for sv in mat.b2cs.shadervars:
                row = box.row()
                col1 = row.column(align=True)
                col1.prop(sv, 'name', text='')
                col2 = row.column(align=True).row()
                if sv.type == 'texture':
                    if sv.value:
                        col2.label('', icon_value=layout.icon(sv.value))
                    draw_idref(col2, sv, 'value')
                else:
                    col2.prop(sv, 'value', text='')
                i = list(getattr(mat.b2cs.shadervars, sv.__type__)).index(sv)
                op = col2.operator(
                    'material.shadervar_remove', text="", icon='ZOOMOUT')
                op.index = i
                op.type = sv.__type__
            #------------------------------------------------------------------

            layout = self.layout
            row = layout.row()
            row.prop(mat.b2cs, "priority")
            row = layout.row()
            row.prop(mat.b2cs, "zbuf_mode")

            # Draw a checkbox to define current material as a CS material
            # reference
            layout.separator()
            row = layout.row()
            row.prop(mat.b2cs, "csMatRef")

            if mat.b2cs.csMatRef:
                # Let the user select a CS material
                row = layout.row()
                if mat.b2cs.csMaterialName == 'None':
                    row.operator_menu_enum(
                        "material.select_mat_ref", "select_material", text=SelectMaterialRef.bl_label)
                else:
                    row.operator_menu_enum(
                        "material.select_mat_ref", "select_material", text=mat.b2cs.csMaterialName)
                    # Verify that factory reference still exists
                    materials = [m.name for m in GetPreferences().MaterialRefs]
                    if not mat.b2cs.csMaterialName in materials:
                        row = layout.row()
                        row.label(
                            text="WARNING: this material reference has been deleted!", icon='ERROR')

            else:
                # CS material properties (used if this material is not replaced by
                # a reference to an existing CS material)
                layout.separator()

                row = layout.row()
                row.prop(mat.b2cs, "shaderset")
                name = GetShaderSetName(mat.b2cs.shaderset)
                if name == 'water_plane':
                    row = layout.row()
                    row.prop(mat.water, "water_fog_color")
                    row = layout.row()
                    row.prop(mat.water, "water_perturb_scale")
                    row = layout.row()
                    row.prop(mat.water, "water_fog_density")


class MATERIAL_OT_textures_menu(bpy.types.Menu):
    bl_idname = "MATERIAL_OT_textures_menu"
    bl_label = "Create"

    def draw(self, context):
        mat = context.material

        textures = {}

        for i, slot in enumerate(mat.texture_slots):
            if slot:
                for p in PROPS:
                    if getattr(slot, p):
                        textures[p] = (i, slot.texture)

        for p in PROPS:
            if p not in textures:
                self.layout.operator(
                    "material.textures_add", text="Create " + NAMES[p]).type = p


class MATERIAL_OT_texture_add(bpy.types.Operator):
    bl_idname = "material.textures_add"
    bl_label = "Add texture"

    type = bpy.props.StringProperty()

    @classmethod
    def poll(cls, context):
        return context.material is not None

    def execute(self, context):
        mat = context.material

        texture = bpy.data.textures.new(
            mat.name + '_' + NAMES[self.type], 'IMAGE')
        slot = mat.texture_slots.add()
        for p in PROPS:
            setattr(slot, p, False)
        setattr(slot, self.type, True)
        slot.texture = texture

        return {'FINISHED'}

    def invoke(self, context, event):
        return self.execute(context)


class MATERIAL_OT_texture_remove(bpy.types.Operator):
    bl_idname = "material.texture_remove"
    bl_label = "Remove texture"

    type = bpy.props.StringProperty()
    index = bpy.props.IntProperty()

    @classmethod
    def poll(cls, context):
        return context.material is not None

    def execute(self, context):
        mat = context.material

        print(dir(mat.texture_slots))
        mat.texture_slots.clear(self.index)

        return {'FINISHED'}

    def invoke(self, context, event):
        return self.execute(context)


class MATERIAL_OT_shadervars_menu(bpy.types.Menu):
    bl_idname = "MATERIAL_OT_shadervars_menu"
    bl_label = "Create"

    def draw(self, context):
        mat = context.material
        for type in mat.b2cs.shadervars.getNames():
            self.layout.operator(
                "material.shadervar_add", text="Create " + type[:-1]).type = type


class MATERIAL_OT_shadervar_add(bpy.types.Operator):
    bl_idname = "material.shadervar_add"
    bl_label = "Add shadervar"

    type = bpy.props.StringProperty()

    @classmethod
    def poll(cls, context):
        return context.material is not None

    def execute(self, context):
        mat = context.material

        svs = getattr(mat.b2cs.shadervars, self.type)
        sv = svs.add()
        sv.name = 'Unnamed'

        return {'FINISHED'}

    def invoke(self, context, event):
        return self.execute(context)


class MATERIAL_OT_shadervar_remove(bpy.types.Operator):
    bl_idname = "material.shadervar_remove"
    bl_label = "Remove shadervar"

    type = bpy.props.StringProperty()
    index = bpy.props.IntProperty()

    @classmethod
    def poll(cls, context):
        return context.material is not None

    def execute(self, context):
        mat = context.material

        svs = getattr(mat.b2cs.shadervars, self.type)
        svs.remove(self.index)

        return {'FINISHED'}

    def invoke(self, context, event):
        return self.execute(context)


class ShaderVariable(bpy.types.PropertyGroup):
    name = bpy.props.StringProperty()

    @property
    def type(self):
        return self.__type__[:-1]


class ExpressionShaderVariable(ShaderVariable):
    __type__ = 'expressions'
    value = bpy.props.StringProperty()


class IntShaderVariable(ShaderVariable):
    __type__ = 'ints'
    value = bpy.props.IntProperty()


class FloatShaderVariable(ShaderVariable):
    __type__ = 'floats'
    value = bpy.props.FloatProperty()


class Vector2ShaderVariable(ShaderVariable):
    __type__ = 'vector2s'
    value = bpy.props.FloatVectorProperty(size=2)


class Vector3ShaderVariable(ShaderVariable):
    __type__ = 'vector3s'
    value = bpy.props.FloatVectorProperty(size=3)


class Vector4ShaderVariable(ShaderVariable):
    __type__ = 'vector4s'
    value = bpy.props.FloatVectorProperty(size=4)


class ColorShaderVariable(ShaderVariable):
    __type__ = 'colors'
    value = bpy.props.FloatVectorProperty(
        size=4, subtype='COLOR', default=(1, 1, 1, 1))


@IDRefContainer
class TextureShaderVariable(ShaderVariable):
    __type__ = 'textures'
    value = IDRefProperty(idtype='IMAGE')


class ShaderVariables(bpy.types.PropertyGroup):
    expressions = bpy.props.CollectionProperty(type=ExpressionShaderVariable)
    ints = bpy.props.CollectionProperty(type=IntShaderVariable)
    floats = bpy.props.CollectionProperty(type=FloatShaderVariable)
    vector2s = bpy.props.CollectionProperty(type=Vector2ShaderVariable)
    vector3s = bpy.props.CollectionProperty(type=Vector3ShaderVariable)
    vector4s = bpy.props.CollectionProperty(type=Vector4ShaderVariable)
    colors = bpy.props.CollectionProperty(type=ColorShaderVariable)
    textures = bpy.props.CollectionProperty(type=TextureShaderVariable)

    @property
    def all(self):
        names = self.getNames()
        shadervars = []
        for name in names:
            shadervars.extend(list(getattr(self, name)))
        return shadervars

    def getNames(self):
        names = []
        for e in dir(self):
            if not e.startswith('__') and not e == 'all' and not e == 'getNames':
                if getattr(self, e).__class__.__name__ == 'bpy_prop_collection_idprop':
                    names.append(e)
        return names

    def __iter__(self):
        from collections import Iterator

        class Iter(Iterator):

            def __init__(self, data):
                self.data = list(data)

            def __next__(self):
                if not self.data:
                    raise StopIteration
                return self.data.pop()
        return Iter(self.all)


@settings(type='Material')
class CrystalSpaceSettingsMaterial(PropertyGroup):
    shadervars = bpy.props.PointerProperty(type=ShaderVariables)

    shaderset = bpy.props.EnumProperty(
        name="ShaderSet",
        description="",
        items=SHADERSETS,
        # default="DEFAULT")
    )

    priority = bpy.props.EnumProperty(
        name="Render priority",
        description="Priority level in which the object will be renderered",
        items=RENDERPRIORITIES,
        default="object")

    zbuf_mode = bpy.props.EnumProperty(
        name="Z-buffer mode",
        description="Behavior of the rendering of the object regarding the Z-Buffer",
        items=ZBUFFERMODES,
        default="zuse")

    csMatRef = bpy.props.BoolProperty(
        name="Material replaced by a CS material",
        description="Replace this material by a Crystal Space material",
        default=False)

    csMaterialName = bpy.props.StringProperty(
        name="Reference of CS material",
        description="Name of an existing Crystal Space material",
        default="None")

    csMaterialVfs = bpy.props.StringProperty(
        name="VFS path of CS material",
        description="VFS path of a Crystal Space library file",
        default="")


@settings(type='Material', attribute='water')
class CrystalSpaceSettingsMaterial(PropertyGroup):
    water_fog_color = bpy.props.FloatVectorProperty(
        name="Water Fog Color",
        description="Fog color inside the water surface",
        subtype='COLOR',
        size=4,
        default=(0, 0.1, 0.15, 1)
    )

    water_fog_density = bpy.props.FloatProperty(
        name="Water Fog Density",
        description="Fog density inside the water surface",
        default=3.0)

    water_perturb_scale = bpy.props.FloatVectorProperty(
        name="Water Perturb Scale",
        description="Fog perturb scale",
        subtype='TRANSLATION',
        size=4,
        default=(0.9, 0.9, 0, 0))
