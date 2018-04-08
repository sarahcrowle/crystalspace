# <pep8 compliant>
# Graciously stolen from, thanks Lucas_t!
# http://www.gitorious.org/blender-trunk/addons/source/25fbdf38ea59422b618ab6488651c15d0ac42167:py/scripts/addons/modifier_nodes/pynodes_framework/idref.py#L83

import bpy
from bpy_types import RNAMetaPropGroup


def get_idtype_list_prop(idtype):
    typemap = {
        'ACTION': 'actions',
        'ARMATURE': 'armatures',
        'BRUSH': 'brushes',
        'CAMERA': 'cameras',
        'CURVE': 'curves',
        'FONT': 'fonts',
        'GREASE_PENCIL': 'grease_pencil',
        'GROUP': 'groups',
        'IMAGE': 'images',
        'LAMP': 'lamps',
        'LATTICE': 'lattices',
        'LIBRARY': 'libraries',
        'MASK': 'masks',
        'MATERIAL': 'materials',
        'MESH': 'meshes',
        'METABALL': 'metaballs',
        'MOVIECLIP': 'movieclips',
        'NODE_GROUP': 'node_groups',
        'OBJECT': 'objects',
        'PARTICLES': 'particles',
        'SCENE': 'scenes',
        'SCREEN': 'screens',
        'SCRIPT': 'scripts',
        'SHAPE_KEY': 'shape_keys',
        'SOUND': 'sounds',
        'SPEAKER': 'speakers',
        'TEXT': 'texts',
        'TEXTURE': 'textures',
        'WINDOW_MANAGER': 'window_managers',
        'WORLD': 'worlds',
    }
    return typemap[idtype]


def get_idtype_list(idtype):
    prop = get_idtype_list_prop(idtype)
    return lambda: getattr(bpy.data, prop, [])


def bpy_register_idref(cls, attr, idrefprop):
    idlist = get_idtype_list(idrefprop.idtype)
    name_attr = "%s__name__" % attr
    idtype_attr = "%s__idtype__" % attr

    setattr(cls, idtype_attr, idrefprop.idtype)

    def prop_get_name(self):
        return self.get(name_attr, "")

    def prop_set_name(self, value):
        print('prop_set')
        idvalue = idlist().get(value, None)
        oldvalue = idlist().get(self.get(name_attr, ""), None)
        if idvalue is not None:
            if idrefprop.poll and not idrefprop.poll(self, idvalue):
                return
        if idrefprop.unselect and oldvalue is not None:
            idrefprop.unselect(self, oldvalue)
        if idvalue is not None or (not value and 'NEVER_NULL' not in idrefprop.options):
            if idrefprop.select and idvalue is not None:
                idrefprop.select(self, idvalue)
            self[name_attr] = value
        if idvalue is not None and 'FAKE_USER' in idrefprop.options:
            idvalue.use_fake_user = True

    setattr(cls, name_attr, bpy.props.StringProperty(
        name="%s ID Name" % idrefprop.name,
        description="ID data block name for pseudo IDRef pointer",
        options={'HIDDEN'} | (idrefprop.options & {
                              'ANIMATABLE', 'SKIP_SAVE', 'LIBRARY_EDITABLE'}),
        update=idrefprop.update,
        get=prop_get_name,
        set=prop_set_name,
    ))

    def prop_get(self):
        name = self.get(name_attr, "")
        value = idlist().get(name, None)
        # Reset the name idproperty if invalid
        # XXX this is not 100% reliable, but better than keeping invalid names
        # around
        if value is None and name_attr in self:
            s = getattr(self, name_attr)
            s = ""
        return value

    def prop_set(self, value):
        if value is None:
            if 'NEVER_NULL' in idrefprop.options:
                return
            del self[name_attr]
        else:
            if idrefprop.poll and not idrefprop.poll(self, value):
                return
            if value.name not in idlist():
                return
            if 'FAKE_USER' in idrefprop.options:
                value.use_fake_user = True
            self[name_attr] = value.name

    def prop_del(self):
        delattr(self, name_attr)
        delattr(self, idtype_attr)

    prop = property(prop_get, prop_set, prop_del, idrefprop.description)
    # Note: replaces the temporary IDRefProperty item!
    setattr(cls, attr, prop)


def bpy_unregister_idref(cls, attr):
    delattr(cls, attr)


def IDRefContainer(cls):
    idref_items = [(attr, item)
                   for attr, item in cls.__dict__.items() if isinstance(item, IDRefProperty)]
    for attr, item in idref_items:
        bpy_register_idref(cls, attr, item)
    return cls


class IDRefProperty():

    def __init__(self, name="", description="", idtype='OBJECT', options={'ANIMATABLE', 'FAKE_USER'}, update=None, poll=None, select=None, unselect=None):
        self.name = name
        self.description = description
        self.idtype = idtype
        self.options = options
        self.update = update
        self.poll = poll
        self.select = select
        self.unselect = unselect

# XXX could be injected into UILayout as a template


def draw_idref(layout, data, prop):
    row = layout.row(align=True)
    idtype = getattr(data, "%s__idtype__" % prop)
    row.prop_search(data, "%s__name__" %
                    prop, bpy.data, get_idtype_list_prop(idtype), text="")
