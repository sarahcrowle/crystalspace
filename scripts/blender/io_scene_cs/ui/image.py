import bpy

from io_scene_cs.utilities import rnaType, settings
from bpy.types import PropertyGroup


def active_node_mat(mat):
    if mat is not None:
        mat_node = mat.active_node_material
        if mat_node:
            return mat_node
        else:
            return mat

    return None


def context_tex_datablock(context):
    idblock = context.material
    if idblock:
        return active_node_mat(idblock)

    idblock = context.lamp
    if idblock:
        return idblock

    idblock = context.world
    if idblock:
        return idblock

    idblock = context.brush
    return idblock


class csTexturePanel():
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "texture"
    # COMPAT_ENGINES must be defined in each subclass, external engines can
    # add themselves here

    @classmethod
    def poll(cls, context):
        tex = context.texture
        r = (tex and tex.type == 'IMAGE')
        rd = context.scene.render
        return r and (rd.engine in cls.COMPAT_ENGINES)


@rnaType
class TEXTURE_PT_B2CS_texture(csTexturePanel, bpy.types.Panel):
    bl_label = "Crystal Space Texture"
    COMPAT_ENGINES = {'CRYSTALSPACE'}

    def draw(self, context):
        layout = self.layout

        tex = context.texture_slot

        idblock = context_tex_datablock(context)

        if type(idblock) == bpy.types.Material:
            if idblock.type in ('SURFACE', 'HALO', 'WIRE'):

                props = ['use_map_color_diffuse', 'use_map_normal',
                         'use_map_displacement', 'use_map_specular', ]

                layout.label(text="Usage:")
                for prop in props:
                    text = prop.split('_')
                    row = layout.row(align=True)
                    row.prop(tex, prop, text=text[-1])
                    row.active = not any(
                        [getattr(tex, p) for p in props if p != prop])
                    row.enabled = row.active

                layout.label(text="Properties:")
                row = layout.row(align=True)
                if tex.texture.image:
                    row.prop(tex.texture.image.b2cs, "binAlpha")


@settings(type='Image')
class CrystalSpaceSettingsGroup(PropertyGroup):
    binAlpha = bpy.props.BoolProperty(
        name="Binary Alpha",
        description="Use binary alpha for this texture",
        default=False)
