#include "glr-tex-cache.h"

#include <GLES3/gl3.h>
#include <glib.h>
#include "glr-priv.h"
#include FT_GLYPH_H
#include FT_LCD_FILTER_H
#include FT_MODULE_H
#include <math.h>

#define MAX_GLYPH_TEXTURES 8

// @FIXME: Don't hardcode texture sizes, read limits from environment
#define GLYPH_TEX_WIDTH  1024
#define GLYPH_TEX_HEIGHT 4096

typedef struct
{
  uint32_t x;
  uint32_t width;
  uint32_t first_y;
  GLuint tex_id;
} TexColumn;

typedef struct
{
  GLuint id;
  GList *columns;
  TexColumn *right_most_column;
} Texture;

struct _GlrTexCache
{
  int ref_count;

  GlrContext *context;

  FT_Library ft_lib;
  GHashTable *font_entries;
  GHashTable *font_faces;

  Texture glyph_texs[MAX_GLYPH_TEXTURES];
  uint32_t num_glyph_texs;
  uint32_t current_glyph_tex;

  GLuint tex_table[64];
};

static void
glr_tex_cache_free (GlrTexCache *self)
{
  int i;

  g_hash_table_unref (self->font_faces);
  g_hash_table_unref (self->font_entries);

  FT_Done_Library (self->ft_lib);

  for (i = 0; i < self->num_glyph_texs; i++)
    {
      Texture *tex = &self->glyph_texs[i];

      glDeleteTextures (1, &tex->id);
      g_list_free_full (tex->columns, g_free);
    }

  g_slice_free (GlrTexCache, self);
  self = NULL;

  g_print ("GlrTexCache freed\n");
}

static void
free_tex_surface (GlrTexSurface *surface)
{
  g_slice_free (GlrTexSurface, surface);
}

static FT_Face
load_font_face (GlrTexCache *self,
                const char  *face_filename,
                uint32_t     face_index)
{
  int err;
  FT_Face face;

  if (self->ft_lib == NULL)
    {
      // initialize TrueType2 library
      err = FT_Init_FreeType (&self->ft_lib);
      if (err != 0)
        {
          g_printerr ("Error while initializing FreeType2 lib: %d\n", err);
          return NULL;
        }

      FT_Library_SetLcdFilter (self->ft_lib, FT_LCD_FILTER_DEFAULT);
    }

  // create a new face
  err = FT_New_Face (self->ft_lib,
                     face_filename,
                     face_index,
                     &face);
  if (err == FT_Err_Unknown_File_Format)
    {
      g_printerr ("Error loading the font face. Format not supported: %d\n", err);
      return NULL;
    }
  else if (err != 0)
    {
      g_printerr ("Error loading the font face. Font file cannot be read: %d\n", err);
      return NULL;
    }

  return face;
}

static int
compare_column_widths (gconstpointer a, gconstpointer b)
{
  const TexColumn *c1 = a;
  const TexColumn *c2 = b;

  if (c1->width < c2->width)
    return -1;
  else if (c1->width > c2->width)
    return 1;
  else
    return 0;
}

static Texture *
allocate_new_glyph_texture (GlrTexCache *self)
{
  Texture *tex;

  if (self->num_glyph_texs == MAX_GLYPH_TEXTURES)
    return NULL;

  tex = &self->glyph_texs[self->num_glyph_texs];
  self->num_glyph_texs++;

  tex->columns = NULL;
  tex->right_most_column = NULL;

  tex->id = self->num_glyph_texs - 1;

  GLuint _tex;
  glGenTextures (1, &_tex);
  self->tex_table[tex->id] = _tex;
  glActiveTexture (GL_TEXTURE0 + tex->id);
  glBindTexture (GL_TEXTURE_2D, _tex);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D (GL_TEXTURE_2D,
                0,
                GL_R8,
                GLYPH_TEX_WIDTH, GLYPH_TEX_HEIGHT,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                NULL);

  return tex;
}

static TexColumn *
find_surface_for_glyph (GlrTexCache *self,
                        uint32_t     width,
                        uint32_t     height)
{
  GList *node;
  TexColumn *column = NULL;
  int i;
  Texture *tex;

  if (width >= GLYPH_TEX_WIDTH || height >= GLYPH_TEX_HEIGHT)
    return NULL;

  for (i = 0; i < MAX_GLYPH_TEXTURES; i++)
    {
      if (i == self->num_glyph_texs)
        tex = allocate_new_glyph_texture (self);
      else
        tex = &self->glyph_texs[i];

      if (tex == NULL)
        return NULL;

      /* look for an existing column that has enough space left at the bottom */
      node = tex->columns;
      while (node != NULL)
        {
          column = node->data;
          if (column->width >= width && GLYPH_TEX_HEIGHT - column->first_y >= height)
            break;

          node = g_list_next (node);
          column = NULL;
        }

      if (column == NULL)
        {
          /* need to create a new column */
          node = g_list_last (tex->columns);
          if (node == NULL)
            {
              /* there is no columns defined yet, create a new one */
              column = g_new0 (TexColumn, 1);
              column->x = 0;
              column->width = width;
              column->first_y = 0;
              column->tex_id = tex->id;

              tex->columns = g_list_insert_sorted (tex->columns,
                                                   column,
                                                   compare_column_widths);
            }
          else
            {
              g_assert (tex->right_most_column != NULL);
              if (GLYPH_TEX_WIDTH - (tex->right_most_column->x + tex->right_most_column->width) < width)
                continue;

              column = g_new0 (TexColumn, 1);
              column->x = tex->right_most_column->x + tex->right_most_column->width;
              column->width = width;
              column->first_y = 0;
              column->tex_id = tex->id;

              tex->columns = g_list_insert_sorted (tex->columns,
                                                   column,
                                                   compare_column_widths);
            }
        }

      if (column != NULL)
        {
          if (tex->right_most_column == NULL ||
              column->x + column->width + 1 > tex->right_most_column->x + tex->right_most_column->width)
            {
              tex->right_most_column = column;
            }

          break;
        }
    }

  return column;
}

static const GlrTexSurface *
lookup_font_glyph (GlrTexCache *self,
                   const char  *surface_id,
                   FT_Face      face,
                   size_t       font_size,
                   uint32_t     glyph_index)
{
  GlrTexSurface *surface = NULL;

  /* set char size */
  int err;
  /* @FIXME: read device resolution from GlrTarget */
  err = FT_Set_Char_Size (face,             /* handle to face object           */
                          0,                /* char_width in 1/64th of points  */
                          font_size * 64,   /* char_height in 1/64th of points */
                          96,               /* horizontal device resolution    */
                          96);              /* vertical device resolution      */
  if (err != 0)
    {
      g_printerr ("Error setting the face size: %d\n", err);
      goto out;
    }

  /* load glyph */
  err = FT_Load_Glyph (face, glyph_index, FT_LOAD_NO_BITMAP);
  if (err != 0)
    {
      g_printerr ("Error loading glyph: %d\n", err);
      goto out;
    }

  /* render glyph if it is not embedded bitmap */
  if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
    {
      err = FT_Render_Glyph (face->glyph, FT_RENDER_MODE_LCD);
      if (err != 0)
        {
          g_printerr ("Error rendering glyph: %d\n", err);
          goto out;
        }
    }

  /* upload glyph bitmap to texture */
  FT_Bitmap bmp = face->glyph->bitmap;
  TexColumn *column = find_surface_for_glyph (self, bmp.width + 1, bmp.rows + 1);
  if (column == NULL)
    {
      /* @FIXME: no space found for the new glyph,
         we need a flushing mechanism */
      g_printerr ("Out of space in the texture cache\n");
      goto out;
    }

  glActiveTexture (GL_TEXTURE0 + column->tex_id);
  glBindTexture (GL_TEXTURE_2D, self->tex_table[column->tex_id]);
  glTexSubImage2D (GL_TEXTURE_2D,
                   0,
                   column->x + 1, column->first_y + 1,
                   bmp.width, bmp.rows,
                   GL_RED,
                   GL_UNSIGNED_BYTE,
                   bmp.buffer);


  surface = g_slice_new (GlrTexSurface);
  surface->tex_id = column->tex_id;
  surface->left = (column->x + 1) / ((float) GLYPH_TEX_WIDTH);
  surface->top = (column->first_y + 1) / (float) GLYPH_TEX_HEIGHT;
  surface->width = bmp.width / ((float) GLYPH_TEX_WIDTH);
  surface->height = bmp.rows / ((float) GLYPH_TEX_HEIGHT);
  surface->pixel_left = face->glyph->bitmap_left;
  surface->pixel_top = face->glyph->bitmap_top;
  surface->pixel_width = bmp.width / 3;
  surface->pixel_height = bmp.rows;

  column->first_y += bmp.rows + 1;

  g_hash_table_insert (self->font_entries,
                       g_strdup (surface_id),
                       surface);

 out:
  return surface;
}

/* internal API */

GlrTexCache *
glr_tex_cache_new (GlrContext *context)
{
  GlrTexCache *self;

  self = g_slice_new0 (GlrTexCache);
  self->ref_count = 1;

  self->context = context;

  self->font_entries =
    g_hash_table_new_full (g_str_hash,
                           g_str_equal,
                           g_free,
                           (GDestroyNotify) free_tex_surface);
  self->font_faces = g_hash_table_new_full (g_str_hash,
                                            g_str_equal,
                                            g_free,
                                            (GDestroyNotify) FT_Done_Face);

  /* @FIXME: calculate maximum texture sizes instead of hardcoding */
  GLint max_texture_size;
  glGetIntegerv (GL_MAX_TEXTURE_SIZE, &max_texture_size);
  // g_print ("%d\n", max_texture_size);

  /* texture id table */
  memset (self->tex_table, 0x00, 64);

  return self;
}

/* public API */

GlrTexCache *
glr_tex_cache_ref (GlrTexCache *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

void
glr_tex_cache_unref (GlrTexCache *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    glr_tex_cache_free (self);
}

FT_Face
glr_tex_cache_lookup_face (GlrTexCache *self,
                           const char  *face_filename,
                           uint8_t      face_index)
{
  FT_Face face;
  char *face_id;

  face_id = g_strdup_printf ("%s:%u", face_filename, face_index);
  face = g_hash_table_lookup (self->font_faces, face_id);
  if (face == NULL)
    {
      face = load_font_face (self, face_filename, face_index);
      if (face == NULL)
        {
          g_free (face_id);
          return NULL;
        }

      g_hash_table_insert (self->font_faces, g_strdup (face_id), face);
    }
  g_free (face_id);

  return face;
}

const GlrTexSurface *
glr_tex_cache_lookup_font_glyph (GlrTexCache *self,
                                 const char  *face_filename,
                                 uint8_t      face_index,
                                 size_t       font_size,
                                 uint32_t     code_point)
{
  FT_Face face;
  char *surface_id;
  const GlrTexSurface *surface = NULL;

  surface_id = g_strdup_printf ("%s:%u:%lu:%u",
                                face_filename,
                                face_index,
                                font_size,
                                code_point);
  surface = g_hash_table_lookup (self->font_entries, surface_id);
  if (surface != NULL)
    goto out;

  face = glr_tex_cache_lookup_face (self, face_filename, face_index);
  if (face == NULL)
    {
      /* @TODO: font face could not be loaded, throw error */
      g_printerr ("Failed to load font from '%s'\n", face_filename);
      goto out;
    }

  surface = lookup_font_glyph (self,
                               surface_id,
                               face,
                               font_size,
                               code_point);

 out:
  g_free (surface_id);

  return surface;
}
