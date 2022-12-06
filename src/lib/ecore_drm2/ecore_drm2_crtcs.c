#include "ecore_drm2_private.h"

static Eina_Thread_Queue *thq = NULL;

typedef struct
{
   Eina_Thread_Queue_Msg head;
   Ecore_Drm2_Thread_Op_Code code;
} Thread_Msg;

static void
_ecore_drm2_crtc_state_thread_send(Ecore_Drm2_Thread_Op_Code code)
{
   Thread_Msg *msg;
   void *ref;

   msg = eina_thread_queue_send(thq, sizeof(Thread_Msg), &ref);
   msg->code = code;
   eina_thread_queue_send_done(thq, ref);
}

static void
_ecore_drm2_crtc_state_debug(Ecore_Drm2_Crtc *crtc)
{
   DBG("CRTC Atomic State Fill Complete");
   DBG("\tCrtc: %d", crtc->state->obj_id);
   DBG("\t\tMode: %d", crtc->state->mode.value);
   DBG("\t\tActive: %lu", (long)crtc->state->active.value);
}

static void
_ecore_drm2_crtc_state_fill(Ecore_Drm2_Crtc *crtc)
{
   Ecore_Drm2_Crtc_State *cstate;
   drmModeObjectPropertiesPtr oprops;
   unsigned int i = 0;

   /* try to allocate space for CRTC Atomic state */
   crtc->state = calloc(1, sizeof(Ecore_Drm2_Crtc_State));
   if (!crtc->state)
     {
        ERR("Could not allocate space for CRTC state");
        return;
     }

   cstate = crtc->state;
   cstate->obj_id = crtc->drmCrtc->crtc_id;

   /* get the properties of this crtc from drm */
   oprops =
     sym_drmModeObjectGetProperties(crtc->fd, cstate->obj_id,
                                    DRM_MODE_OBJECT_CRTC);
   if (!oprops)
     {
        free(crtc->state);
        return;
     }

   for (; i < oprops->count_props; i++)
     {
        drmModePropertyPtr prop;

        /* try to get this individual property */
        prop = sym_drmModeGetProperty(crtc->fd, oprops->props[i]);
        if (!prop) continue;

        /* check for the properties we are interested in */
        if (!strcmp(prop->name, "MODE_ID"))
          {
             drmModePropertyBlobPtr bp;

             cstate->mode.id = prop->prop_id;
             cstate->mode.value = oprops->prop_values[i];

             if (!cstate->mode.value)
               {
                  cstate->mode.len = 0;
                  goto cont;
               }

             bp = sym_drmModeGetPropertyBlob(crtc->fd, cstate->mode.value);
             if (!bp) goto cont;

             if ((!cstate->mode.data) ||
                 memcmp(cstate->mode.data, bp->data, bp->length) != 0)
               {
                  cstate->mode.data = eina_memdup(bp->data, bp->length, 1);
               }

             cstate->mode.len = bp->length;

             if (cstate->mode.value != 0)
               sym_drmModeCreatePropertyBlob(crtc->fd, bp->data, bp->length,
                                             &cstate->mode.value);

             sym_drmModeFreePropertyBlob(bp);
          }
        else if (!strcmp(prop->name, "ACTIVE"))
          {
             cstate->active.id = prop->prop_id;
             cstate->active.flags = prop->flags;
             cstate->active.value = oprops->prop_values[i];
          }
        /* TODO: We don't actually use this value yet */
        /* else if (!strcmp(prop->name, "BACKGROUND_COLOR")) */
        /*   { */
        /*      cstate->background.id = prop->prop_id; */
        /*      cstate->background.flags = prop->flags; */
        /*      cstate->background.value = oprops->prop_values[i]; */
        /*   } */

cont:
        sym_drmModeFreeProperty(prop);
     }

   sym_drmModeFreeObjectProperties(oprops);

   /* send message to thread for debug printing crtc state */
   _ecore_drm2_crtc_state_thread_send(ECORE_DRM2_THREAD_CODE_DEBUG);
}

static void
_ecore_drm2_crtc_state_thread(void *data, Ecore_Thread *thread)
{
   Ecore_Drm2_Crtc *crtc;
   Thread_Msg *msg;
   void *ref;

   crtc = data;

   eina_thread_name_set(eina_thread_self(), "Ecore-drm2-crtc");

   while (!ecore_thread_check(thread))
     {
        msg = eina_thread_queue_wait(thq, &ref);
        if (msg)
          {
             switch (msg->code)
               {
                case ECORE_DRM2_THREAD_CODE_FILL:
                  _ecore_drm2_crtc_state_fill(crtc);
                  break;
                case ECORE_DRM2_THREAD_CODE_DEBUG:
                  _ecore_drm2_crtc_state_debug(crtc);
                  break;
                default:
                  break;
               }
             eina_thread_queue_wait_done(thq, ref);
          }
     }
}

static void
_ecore_drm2_crtc_state_thread_notify(void *data EINA_UNUSED, Ecore_Thread *thread EINA_UNUSED, void *msg)
{
   /* Ecore_Drm2_Crtc *crtc; */

   /* crtc = data; */

   free(msg);
}

static Ecore_Drm2_Crtc *
_ecore_drm2_crtc_create(Ecore_Drm2_Device *dev, drmModeCrtcPtr dcrtc, uint32_t pipe)
{
   Ecore_Drm2_Crtc *crtc;

   /* try to allocate space for a crtc */
   crtc = calloc(1, sizeof(Ecore_Drm2_Crtc));
   if (!crtc)
     {
        ERR("Could not allocate space for CRTC");
        return NULL;
     }

   crtc->id = dcrtc->crtc_id;
   crtc->fd = dev->fd;
   crtc->pipe = pipe;
   crtc->drmCrtc = dcrtc;

   /* add this crtc to the list */
   dev->crtcs = eina_list_append(dev->crtcs, crtc);

   return crtc;
}

Eina_Bool
_ecore_drm2_crtcs_create(Ecore_Drm2_Device *dev)
{
   Ecore_Drm2_Crtc *crtc;
   drmModeCrtcPtr c;
   drmModeRes *res;
   int i = 0;

   /* try to get drm resources */
   res = sym_drmModeGetResources(dev->fd);
   if (!res) return EINA_FALSE;

   thq = eina_thread_queue_new();

   for (; i < res->count_crtcs; i++)
     {
        /* try to get this crtc from drm */
        c = sym_drmModeGetCrtc(dev->fd, res->crtcs[i]);

        /* try to create a crtc */
        crtc = _ecore_drm2_crtc_create(dev, c, i);
        if (!crtc) goto err;

        /* NB: Use an explicit thread to fill crtc atomic state */
        crtc->thread =
          ecore_thread_feedback_run(_ecore_drm2_crtc_state_thread,
                                    _ecore_drm2_crtc_state_thread_notify,
                                    NULL, NULL, crtc, EINA_TRUE);
     }

   sym_drmModeFreeResources(res);
   return EINA_TRUE;

err:
   eina_thread_queue_free(thq);
   thq = NULL;
   _ecore_drm2_crtcs_destroy(dev);
   sym_drmModeFreeCrtc(c);
   sym_drmModeFreeResources(res);
   return EINA_FALSE;
}

void
_ecore_drm2_crtcs_destroy(Ecore_Drm2_Device *dev)
{
   Ecore_Drm2_Crtc *crtc;

   EINA_LIST_FREE(dev->crtcs, crtc)
     {
        if (crtc->thread) ecore_thread_cancel(crtc->thread);
        if (crtc->drmCrtc) sym_drmModeFreeCrtc(crtc->drmCrtc);
        free(crtc->state);
        free(crtc);
     }

   if (thq)
     {
        eina_thread_queue_free(thq);
        thq = NULL;
     }
}

void
_ecore_drm2_crtc_mode_set(Ecore_Drm2_Crtc *crtc EINA_UNUSED, Ecore_Drm2_Display_Mode *mode EINA_UNUSED, int x EINA_UNUSED, int y EINA_UNUSED)
{
   /* TODO: add code to actually set crtc mode */
}