import bpy


def GetConstraintType(self):
    if self.type == 'COPY_TRANSFORMS':
        return self.name.split('.')[0]
    else:
        return None

bpy.types.Constraint.GetConstraintType = GetConstraintType
