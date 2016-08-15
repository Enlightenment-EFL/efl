#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Eo.h"
#include "constructors_mixin.h"
#include "constructors_simple6.h"

#define MY_CLASS SIMPLE6_CLASS

static void
_destructor(Eo *obj, void *class_data EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

static Efl_Op_Description op_descs [] = {
     EFL_OBJECT_OP_FUNC_OVERRIDE(efl_destructor, _destructor),
};

static const Efl_Class_Description class_desc = {
     EO_VERSION,
     "Simple6",
     EFL_CLASS_TYPE_REGULAR,
     EFL_CLASS_DESCRIPTION_OPS(op_descs),
     NULL,
     0,
     NULL,
     NULL
};

EFL_DEFINE_CLASS(simple6_class_get, &class_desc, EO_CLASS, NULL);

