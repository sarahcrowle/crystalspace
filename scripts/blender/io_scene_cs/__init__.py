__all__ = ["ui", "oi", "utilities", "engine", "xml"]

bl_info = {
    "name": "Export Crystal Space 3D format",
    "description": "Export meshes, scenes and animations",
    "author": "The Crystal Space team",
    "version": (2, 1),
    "blender": (2, 65, 0),
    "api": 39685,
    "location": "Render > Engine > CrystalSpace",
    "warning": "",
    "wiki_url": "http://www.crystalspace3d.org/docs/online/manual/Blender.html",
    "tracker_url": "http://crystalspace3d.org/trac/CS/report",
    "category": "Game Engine"}


if "bpy" in locals():
    import imp
    imp.reload(utilities)
    imp.reload(preferences)
    imp.reload(ui)
    imp.reload(io)
    imp.reload(engine)

    imp.reload(handlers)
    imp.reload(xml)
else:
    import os
    import bpy

    from . import utilities
    from . import preferences
    from . import ui
    from . import io
    from . import engine

    from . import handlers
    from . import xml

# Registration


def register():
    bpy.utils.register_module(__name__)
    utilities._register()
    engine.register()


def unregister():
    bpy.utils.unregister_module(__name__)
    utilities._unregister()

if __name__ == "__main__":
    register()
