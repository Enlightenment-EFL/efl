#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Eo.h"
#include "access_simple.h"
#include "access_simple_protected.h"

typedef struct
{
   Simple_Protected_Data protected;
   int a;
} Private_Data;

EAPI const Efl_Event_Description _EV_A_CHANGED =
        EFL_EVENT_DESCRIPTION("a,changed");

#define MY_CLASS SIMPLE_CLASS

static void
_a_set(Eo *obj, void *class_data, int a)
{
   Private_Data *pd = class_data;
   pd->a = a;
   printf("%s %d\n", __func__, pd->a);

   pd->protected.protected_x1 = a + 1;
   pd->protected.public.public_x2 = a + 2;

   efl_event_callback_call(obj, EV_A_CHANGED, &pd->a);
}

EAPI EFL_VOID_FUNC_BODYV(simple_a_set, EFL_FUNC_CALL(a), int a);

static Efl_Op_Description op_descs[] = {
     EFL_OBJECT_OP_FUNC(simple_a_set, _a_set),
};

static const Efl_Event_Description *event_desc[] = {
     EV_A_CHANGED,
     NULL
};

static const Efl_Class_Description class_desc = {
     EO_VERSION,
     "Simple",
     EFL_CLASS_TYPE_REGULAR,
     EFL_CLASS_DESCRIPTION_OPS(op_descs),
     event_desc,
     sizeof(Private_Data),
     NULL,
     NULL
};

EFL_DEFINE_CLASS(simple_class_get, &class_desc, EO_CLASS, NULL)

