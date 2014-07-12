#ifndef _GLR_TEX_CACHE_H_
#define _GLR_TEX_CACHE_H_

#include <GL/gl.h>
#include <glib.h>

typedef struct _GlrTexCache GlrTexCache;

typedef struct
{
  GLuint tex_id;
  guint16 left;
  guint16 top;
  guint16 width;
  guint16 height;
} GlrTexSurface;

GlrTexCache *         glr_tex_cache_ref               (GlrTexCache *self);
void                  glr_tex_cache_unref             (GlrTexCache *self);

const GlrTexSurface * glr_tex_cache_lookup_font_glyph (GlrTexCache *self,
                                                       const gchar *face_filename,
                                                       guint        face_index,
                                                       gsize        font_size,
                                                       guint32      unicode_char);

#endif /* _GLR_TEX_CACHE_H_ */
