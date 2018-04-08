__all__ = ["bone", "data", "group", "object", "image", "material",
           "portal", "lamp", "modifier", "scene", "render", "renderlayer"]

from . import idref
from . import bone
from . import data
from . import group
from . import object
from . import image
from . import material
from . import portal
from . import lamp
from . import layout
from . import modifier
from . import scene
from . import render
from . import renderlayer

from io_scene_cs.utilities import prepend_draw, GetPreferences, HasCrystalSpace


@prepend_draw(type='INFO_HT_header')
def INFO_HT_header_prepend_draw(self, context):
    if context.scene.render.engine == 'CRYSTALSPACE':
        layout = self.layout
        layout.operator("io_scene_cs.export", text="", icon='SCENE_DATA')
        if not GetPreferences().library:
            if HasCrystalSpace():
                layout.operator("io_scene_cs.export_run", text="", icon='GAME')
                if context.active_object and context.active_object.type == 'MESH':
                    layout.operator(
                        "io_scene_cs.export_view", text="", icon='RENDER_STILL')
        layout.separator()
