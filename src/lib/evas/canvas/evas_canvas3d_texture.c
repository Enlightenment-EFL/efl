#include "evas_common_private.h"
#include "evas_private.h"

#define MY_CLASS EVAS_CANVAS3D_TEXTURE_CLASS

static inline void
_texture_proxy_set(Evas_Canvas3D_Texture *texture, Evas_Object *eo_src, Evas_Object_Protected_Data *src)
{
   Evas_Canvas3D_Texture_Data *pd = eo_data_scope_get(texture, MY_CLASS);
   EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, src->proxy, Evas_Object_Proxy_Data, proxy_src)
     {
        proxy_src->proxy_textures = eina_list_append(proxy_src->proxy_textures, texture);
        proxy_src->redraw = EINA_TRUE;
     }
   EINA_COW_WRITE_END(evas_object_proxy_cow, src->proxy, proxy_src);

   pd->source = eo_src;
}

static inline void
_texture_proxy_unset(Evas_Canvas3D_Texture_Data *texture)
{
   Evas_Object_Protected_Data *src = eo_data_scope_get(texture->source, EVAS_OBJECT_CLASS);

   EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, src->proxy, Evas_Object_Proxy_Data, proxy_src);
     {
        proxy_src->proxy_textures = eina_list_remove(proxy_src->proxy_textures, texture);

        if (eina_list_count(proxy_src->proxy_textures) == 0 &&
            eina_list_count(proxy_src->proxies) == 0 &&
            proxy_src->surface != NULL)
          {
             Evas_Public_Data *e = src->layer->evas;
             e->engine.func->image_free(e->engine.data.output, proxy_src->surface);
             proxy_src->surface = NULL;
          }

        if (proxy_src->src_invisible)
          {
             proxy_src->src_invisible = EINA_FALSE;
             src->changed_src_visible = EINA_TRUE;
             evas_object_change(texture->source, src);
             evas_object_smart_member_cache_invalidate(texture->source,
                                                       EINA_FALSE, EINA_FALSE, EINA_TRUE);
          }
     }
   EINA_COW_WRITE_END(evas_object_proxy_cow, src->proxy, proxy_src);

   texture->source = NULL;
}

static inline void
_texture_proxy_subrender(Evas_Canvas3D_Texture *obj)
{
   /* Code taken from _proxy_subrender() in file evas_object_image.c */
   Eo *evas = NULL;
   evas = evas_common_evas_get(obj);
   Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);
   Evas_Canvas3D_Texture_Data *pd = eo_data_scope_get(obj, MY_CLASS);
   Evas_Object_Protected_Data *source;
   void *ctx;
   int w, h;
   Eina_Bool is_image;

   if (!pd->source)
     return;

   // TODO: replace this function by evas_render_proxy_subrender (as appropriate)

   source = eo_data_scope_get(pd->source, EVAS_OBJECT_CLASS);

   is_image = eo_isa(pd->source, EVAS_IMAGE_CLASS);

   EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, source->proxy, Evas_Object_Proxy_Data, proxy_write)
     {
        proxy_write->redraw = EINA_FALSE;

        if (is_image)
          {
             void *image = source->func->engine_data_get(pd->source);
             e->engine.func->image_size_get(e->engine.data.output, image, &w, &h);
          }
        else
          {
             w = source->cur->geometry.w;
             h = source->cur->geometry.h;
          }

        /* We need to redraw surface then */
        if ((proxy_write->surface) &&
            ((proxy_write->w != w) || (proxy_write->h != h)))
          {
             e->engine.func->image_free(e->engine.data.output, proxy_write->surface);
             proxy_write->surface = NULL;
          }

        /* FIXME: Hardcoded alpha 'on' */
        /* FIXME (cont): Should see if the object has alpha */
        if (!proxy_write->surface)
          {
             proxy_write->surface = e->engine.func->image_map_surface_new
               (e->engine.data.output, w, h, 1);
             if (!proxy_write->surface) goto end;
             proxy_write->w = w;
             proxy_write->h = h;
          }

        ctx = e->engine.func->context_new(e->engine.data.output);
        e->engine.func->context_color_set(e->engine.data.output, ctx, 0, 0,
                                          0, 0);
        e->engine.func->context_render_op_set(e->engine.data.output, ctx,
                                              EVAS_RENDER_COPY);
        e->engine.func->rectangle_draw(e->engine.data.output, ctx,
                                       proxy_write->surface, 0, 0, w, h,
                                       EINA_FALSE);
        e->engine.func->context_free(e->engine.data.output, ctx);

        ctx = e->engine.func->context_new(e->engine.data.output);

        if (is_image)
          {
             void *image = source->func->engine_data_get(pd->source);

             if (image)
               {
                  int imagew, imageh;
                  e->engine.func->image_size_get(e->engine.data.output, image,
                                                 &imagew, &imageh);
                  e->engine.func->image_draw(e->engine.data.output, ctx,
                                             proxy_write->surface, image,
                                             0, 0, imagew, imageh, 0, 0, w, h, 0, EINA_FALSE);
               }
          }
        else
          {
             Evas_Proxy_Render_Data proxy_render_data = {
                  .eo_proxy = NULL,
                  .proxy_obj = NULL,
                  .eo_src = pd->source,
                  .src_obj = source,
                  .source_clip = EINA_FALSE
             };

             evas_render_mapped(e, pd->source, source, ctx, proxy_write->surface,
                                -source->cur->geometry.x,
                                -source->cur->geometry.y,
                                2, 0, 0, e->output.w, e->output.h,
                                &proxy_render_data, 1, EINA_FALSE, EINA_FALSE);
          }

        e->engine.func->context_free(e->engine.data.output, ctx);
        proxy_write->surface = e->engine.func->image_dirty_region
           (e->engine.data.output, proxy_write->surface, 0, 0, w, h);
     }
 end:
   EINA_COW_WRITE_END(evas_object_proxy_cow, source->proxy, proxy_write);
}

static void
_texture_fini(Evas_Canvas3D_Texture *obj)
{
   Eo *evas = NULL;
   int i = 0;
   Eina_Iterator *it = NULL;
   void *data = NULL;
   Evas_Canvas3D_Material_Data *material = NULL;

   evas = evas_common_evas_get(obj);
   Evas_Canvas3D_Texture_Data *pd = eo_data_scope_get(obj, MY_CLASS);

   if (pd->engine_data)
     {
        Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);
        if (e->engine.func->texture_free)
          e->engine.func->texture_free(e->engine.data.output, pd->engine_data);
        pd->engine_data = NULL;
     }

   if (pd->materials)
     {
        it = eina_hash_iterator_key_new(pd->materials);
        EINA_ITERATOR_FOREACH(it, data)
          {
             material = eo_data_scope_get(data, EVAS_CANVAS3D_MATERIAL_CLASS);
             if (!material) continue;
             for (i = 0; i < EVAS_CANVAS3D_MATERIAL_ATTRIB_COUNT; i++)
               {
                  if (material->attribs[i].texture)
                       evas_canvas3d_texture_material_del(material->attribs[i].texture, data);
               }
          }
        eina_iterator_free(it);

        eina_hash_free(pd->materials);
        pd->materials = NULL;
     }

   if (pd->source)
     {
        _texture_proxy_unset(pd);
     }
}

static Eina_Bool
_texture_material_change_notify(const Eina_Hash *hash EINA_UNUSED, const void *key,
                                  void *data EINA_UNUSED, void *fdata)
{
   Evas_Canvas3D_Material *m = *(Evas_Canvas3D_Material **)key;
   evas_canvas3d_object_change(m, EVAS_CANVAS3D_STATE_MATERIAL_TEXTURE, (Evas_Canvas3D_Object *)fdata);
   return EINA_TRUE;
}

EOLIAN static void
_evas_canvas3d_texture_evas_canvas3d_object_change_notify(Eo *obj, Evas_Canvas3D_Texture_Data *pd, Evas_Canvas3D_State state EINA_UNUSED, Evas_Canvas3D_Object *ref EINA_UNUSED)
{

   if (pd->materials)
     eina_hash_foreach(pd->materials, _texture_material_change_notify, obj);
}

EOLIAN static void
_evas_canvas3d_texture_evas_canvas3d_object_update_notify(Eo *obj, Evas_Canvas3D_Texture_Data *pd)
{
   if (pd->source)
     {
        Eo *evas = NULL;
        evas = evas_common_evas_get(obj);
        Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);
        Evas_Object_Protected_Data *src = eo_data_scope_get(pd->source, EVAS_OBJECT_CLASS);

        if (pd->engine_data == NULL)
          {
             if (e->engine.func->texture_new)
               {
                  pd->engine_data =
                     e->engine.func->texture_new(e->engine.data.output, pd->atlas_enable);
               }

             if (pd->engine_data == NULL)
               {
                  ERR("Failed to create engine-side texture object.");
                  return;
               }
          }

        if (src->proxy->surface && !src->proxy->redraw)
          {
             if (e->engine.func->texture_image_set)
               {
                  e->engine.func->texture_image_set(e->engine.data.output,
                                                    pd->engine_data,
                                                    src->proxy->surface);
               }
             return;

          }

        pd->proxy_rendering = EINA_TRUE;
        _texture_proxy_subrender(obj);

        if (e->engine.func->texture_image_set)
          e->engine.func->texture_image_set(e->engine.data.output,
                                            pd->engine_data,
                                            src->proxy->surface);
        pd->proxy_rendering = EINA_FALSE;
     }
}

void
evas_canvas3d_texture_material_add(Evas_Canvas3D_Texture *texture, Evas_Canvas3D_Material *material)
{
   int count = 0;
   Evas_Canvas3D_Texture_Data *pd = eo_data_scope_get(texture, MY_CLASS);
   if (pd->materials == NULL)
     {
        pd->materials = eina_hash_pointer_new(NULL);

        if (pd->materials == NULL)
          {
             ERR("Failed to create hash table.");
             return;
          }
     }
   else
     count = (int)(uintptr_t)eina_hash_find(pd->materials, &material);

   /* Increase reference count or add new one if not exist. */
   eina_hash_set(pd->materials, &material, (const void *)(uintptr_t)(count + 1));
}

void
evas_canvas3d_texture_material_del(Evas_Canvas3D_Texture *texture, Evas_Canvas3D_Material *material)
{
   int count = 0;
   Evas_Canvas3D_Texture_Data *pd = eo_data_scope_get(texture, MY_CLASS);
   if (pd->materials == NULL)
     {
        ERR("No material to delete.");
        return;
     }

   count = (int)(uintptr_t)eina_hash_find(pd->materials, &material);

   if (count == 1)
     eina_hash_del(pd->materials, &material, NULL);
   else
     eina_hash_set(pd->materials, &material, (const void *)(uintptr_t)(count - 1));
}

EAPI Evas_Canvas3D_Texture *
evas_canvas3d_texture_add(Evas *e)
{
   MAGIC_CHECK(e, Evas, MAGIC_EVAS);
   return NULL;
   MAGIC_CHECK_END();
   Evas_Object *eo_obj = NULL;
   eo_add(&eo_obj, MY_CLASS, e);
   return eo_obj;
}


EOLIAN static Eo *
_evas_canvas3d_texture_eo_base_constructor(Eo *obj, Evas_Canvas3D_Texture_Data *pd EINA_UNUSED)
{
   obj = eo_constructor(eo_super(obj, MY_CLASS));
   pd->atlas_enable = EINA_TRUE;

   evas_canvas3d_object_type_set(obj, EVAS_CANVAS3D_OBJECT_TYPE_TEXTURE);

   return obj;
}

EOLIAN static void
_evas_canvas3d_texture_eo_base_destructor(Eo *obj, Evas_Canvas3D_Texture_Data *pd  EINA_UNUSED)
{
   _texture_fini(obj);
   eo_destructor(eo_super(obj, MY_CLASS));
}

EOLIAN static void
_evas_canvas3d_texture_data_set(Eo *obj, Evas_Canvas3D_Texture_Data *pd,
                          Evas_Colorspace color_format,
                          int w, int h, const void *data)
{
   Eo *evas = NULL;
   void *image = NULL;
   evas = evas_common_evas_get(obj);
   Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);

   if (!pd->engine_data && e->engine.func->texture_new)
     pd->engine_data = e->engine.func->texture_new(e->engine.data.output, pd->atlas_enable);
   if (!data)
     {
        ERR("Failure, image data is empty");
        return;
     }

   image = e->engine.func->image_new_from_data(e->engine.data.output, w, h, (DATA32 *)data, EINA_TRUE, color_format);
   if (!image)
     {
        ERR("Can't load image from data");
        return;
     }

   if (e->engine.func->texture_image_set)
     e->engine.func->texture_image_set(e->engine.data.output,
                                       pd->engine_data,
                                       image);
   e->engine.func->image_free(e->engine.data.output, image);
   evas_canvas3d_object_change(obj, EVAS_CANVAS3D_STATE_TEXTURE_DATA, NULL);
}

EOLIAN static void
_evas_canvas3d_texture_file_set(Eo *obj, Evas_Canvas3D_Texture_Data *pd, const char *file, const char *key)
{

   Evas_Image_Load_Opts lo;
   int load_error;
   Eo *evas = NULL;
   void *image;

   evas = evas_common_evas_get(obj);
   Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);

   if (!pd->engine_data && e->engine.func->texture_new)
     pd->engine_data = e->engine.func->texture_new(e->engine.data.output, pd->atlas_enable);

   memset(&lo, 0x0, sizeof(Evas_Image_Load_Opts));
   image = e->engine.func->image_load(e->engine.data.output,
                                      file, key, &load_error, &lo);
   if (!image)
     {
        ERR("Can't load image from file");
        return;
     }

   if (e->engine.func->texture_image_set)
     e->engine.func->texture_image_set(e->engine.data.output,
                                       pd->engine_data,
                                       image);

   e->engine.func->image_free(e->engine.data.output, image);
   evas_canvas3d_object_change(obj, EVAS_CANVAS3D_STATE_TEXTURE_DATA, NULL);
}

EAPI void
_evas_canvas3d_texture_source_set(Eo *obj , Evas_Canvas3D_Texture_Data *pd, Evas_Object *source)
{
   Eo *evas = NULL;
   evas = evas_common_evas_get(obj);
   Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);
   Evas_Object_Protected_Data *src;

   if (source == pd->source)
     return;

   _texture_fini(obj);

   if (source == NULL)
     return;

   if (evas_object_evas_get(source) != evas)
     {
        ERR("Not matching canvas.");
        return;
     }

   src = eo_data_scope_get(source, EVAS_OBJECT_CLASS);

   if (src->delete_me)
     {
        ERR("Source object is deleted.");
        return;
     }

   if (src->layer == NULL)
     {
        ERR("No evas surface associated with the source object.");
        return;
     }
   if (!pd->engine_data && e->engine.func->texture_new)
     pd->engine_data = e->engine.func->texture_new(e->engine.data.output, pd->atlas_enable);

   _texture_proxy_set(obj, source, src);
   evas_canvas3d_object_change(obj, EVAS_CANVAS3D_STATE_TEXTURE_DATA, NULL);
}

EOLIAN static void
_evas_canvas3d_texture_source_visible_set(Eo *obj EINA_UNUSED, Evas_Canvas3D_Texture_Data *pd, Eina_Bool visible)
{
   Evas_Object_Protected_Data *src_obj;

   if (pd->source == NULL)
     return;

   src_obj = eo_data_scope_get(pd->source, EVAS_OBJECT_CLASS);

   if (src_obj->proxy->src_invisible == !visible)
     return;

   EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, src_obj->proxy, Evas_Object_Proxy_Data, proxy_write)
     proxy_write->src_invisible = !visible;
   EINA_COW_WRITE_END(evas_object_proxy_cow, src_obj->proxy, proxy_write);

   src_obj->changed_src_visible = EINA_TRUE;
   evas_object_smart_member_cache_invalidate(pd->source, EINA_FALSE, EINA_FALSE, EINA_TRUE);
   evas_object_change(pd->source, src_obj);
}

EOLIAN static Eina_Bool
_evas_canvas3d_texture_source_visible_get(Eo *obj EINA_UNUSED, Evas_Canvas3D_Texture_Data *pd)
{
   Evas_Object_Protected_Data *src_obj;

   if (pd->source == NULL)
     return EINA_FALSE;

   src_obj = eo_data_scope_get(pd->source, EVAS_OBJECT_CLASS);
   return !src_obj->proxy->src_invisible;
}

EOLIAN static Evas_Colorspace
_evas_canvas3d_texture_color_format_get(const Eo *obj EINA_UNUSED, Evas_Canvas3D_Texture_Data *pd)
{
   Evas_Colorspace format = -1;
   Eo *evas = NULL;
   evas = evas_common_evas_get(obj);
   Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);

   if (e->engine.func->image_colorspace_get &&
       e->engine.func->texture_image_get)
     {
        void *image;

        image = e->engine.func->texture_image_get(e->engine.data.output, pd->engine_data);
        format = e->engine.func->image_colorspace_get(e->engine.data.output, image);
     }

   return format;
}

EOLIAN static void
_evas_canvas3d_texture_size_get(const Eo *obj, Evas_Canvas3D_Texture_Data *pd, int *w, int *h)
{
   Eo *evas = NULL;
   evas = evas_common_evas_get(obj);
   Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);
   if (e->engine.func->texture_size_get)
     {
        e->engine.func->texture_size_get(e->engine.data.output,
                                         pd->engine_data, w, h);
     }
}

EOLIAN static void
_evas_canvas3d_texture_wrap_set(Eo *obj, Evas_Canvas3D_Texture_Data *pd, Evas_Canvas3D_Wrap_Mode s, Evas_Canvas3D_Wrap_Mode t)
{
   Eo *evas = NULL;
   evas = evas_common_evas_get(obj);
   Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);
   if (e->engine.func->texture_wrap_set)
     {
        e->engine.func->texture_wrap_set(e->engine.data.output,
                                         pd->engine_data, s, t);
     }
   evas_canvas3d_object_change(obj, EVAS_CANVAS3D_STATE_TEXTURE_WRAP, NULL);
}

EOLIAN static void
_evas_canvas3d_texture_wrap_get(Eo *obj, Evas_Canvas3D_Texture_Data *pd, Evas_Canvas3D_Wrap_Mode *s, Evas_Canvas3D_Wrap_Mode *t)
{
   Eo *evas = NULL;
   evas = evas_common_evas_get(obj);
   Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);
   if (e->engine.func->texture_wrap_set)
     {
        e->engine.func->texture_wrap_get(e->engine.data.output,
                                         pd->engine_data, s, t);
     }
}

EOLIAN static void
_evas_canvas3d_texture_filter_set(Eo *obj, Evas_Canvas3D_Texture_Data *pd, Evas_Canvas3D_Texture_Filter min, Evas_Canvas3D_Texture_Filter mag)
{
   Eo *evas = NULL;
   evas = evas_common_evas_get(obj);
   Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);
   if (e->engine.func->texture_filter_set)
     {
        e->engine.func->texture_filter_set(e->engine.data.output,
                                           pd->engine_data, min, mag);
     }
   evas_canvas3d_object_change(obj, EVAS_CANVAS3D_STATE_TEXTURE_FILTER, NULL);
}

EOLIAN static void
_evas_canvas3d_texture_filter_get(const Eo *obj EINA_UNUSED, Evas_Canvas3D_Texture_Data *pd, Evas_Canvas3D_Texture_Filter *min, Evas_Canvas3D_Texture_Filter *mag)
{
   Eo *evas = NULL;
   evas = evas_common_evas_get(obj);
   Evas_Public_Data *e = eo_data_scope_get(evas, EVAS_CANVAS_CLASS);
   if (e->engine.func->texture_filter_get)
     {
        e->engine.func->texture_filter_get(e->engine.data.output,
                                           pd->engine_data, min, mag);
     }
}

EOLIAN static void
_evas_canvas3d_texture_atlas_enable_set(Eo *obj EINA_UNUSED, Evas_Canvas3D_Texture_Data *pd, Eina_Bool use_atlas)
{
   if (pd->atlas_enable != use_atlas) pd->atlas_enable = use_atlas;
}

EOLIAN static Eina_Bool
_evas_canvas3d_texture_atlas_enable_get(Eo *obj EINA_UNUSED, Evas_Canvas3D_Texture_Data *pd)
{
   return pd->atlas_enable;
}
#include "canvas/evas_canvas3d_texture.eo.c"
