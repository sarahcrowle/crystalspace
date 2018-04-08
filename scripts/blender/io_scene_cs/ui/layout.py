import bpy

from io_scene_cs.ui.idref import get_idtype_list_prop


def ObjectReferenceLayout(self, data, prop, name=""):
    box = self.box()
    row = box.row()
    row.label(name)
    idtype = getattr(data, "%s__idtype__" % prop)
    row.prop_search(data, "%s__name__" %
                    prop, bpy.data, get_idtype_list_prop(idtype), text="")

    '''
  if mesh:
    row.context_pointer_set("current_viscullmesh", mesh)
    #row.label(mesh.name)
    row.operator_menu_enum("object.select_object_ref", "select_object", text=mesh.name, icon='OBJECT_DATA')
    row.operator("object.remove_object_ref", text="", icon='X', emboss=False)
  else:
    row.operator_menu_enum("object.select_object_ref", "select_object")
  '''
bpy.types.UILayout.template_object_ref = ObjectReferenceLayout
