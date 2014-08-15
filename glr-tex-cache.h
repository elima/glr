#ifndef _GLR_TEX_CACHE_H_
#define _GLR_TEX_CACHE_H_

#include <GLES3/gl3.h>
#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct _GlrTexCache GlrTexCache;

typedef struct
{
  GLuint tex_id;

  float left;
  float top;
  float width;
  float height;

  int16_t pixel_left;
  int16_t pixel_top;
  uint16_t pixel_width;
  uint16_t pixel_height;
} GlrTexSurface;

GlrTexCache *         glr_tex_cache_ref               (GlrTexCache *self);
void                  glr_tex_cache_unref             (GlrTexCache *self);

const GlrTexSurface * glr_tex_cache_lookup_font_glyph (GlrTexCache *self,
                                                       const char  *face_filename,
                                                       uint8_t      face_index,
                                                       size_t       font_size,
                                                       uint32_t     unicode_char);

FT_Face               glr_tex_cache_lookup_face       (GlrTexCache *self,
                                                       const char  *face_filename,
                                                       uint8_t      face_index);

#endif /* _GLR_TEX_CACHE_H_ */
