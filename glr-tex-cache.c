#include "glr-tex-cache.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_LCD_FILTER_H
#include FT_MODULE_H
#include <math.h>

#define MAX_TEXTURES 4

#define GLYPH_TEX_WIDTH  2048
#define GLYPH_TEX_HEIGHT 4096

typedef struct
{
  guint x;
  guint width;
  guint first_y;
} TexColumn;

struct _GlrTexCache
{
  gint ref_count;

  FT_Library ft_lib;
  GHashTable *font_entries;
  GHashTable *font_faces;

  GLuint glyph_tex;

  GList *columns;
  TexColumn *right_most_column;

  guint8 *aux_buf;
};

static void
glr_tex_cache_free (GlrTexCache *self)
{
  g_slice_free1 (GLYPH_TEX_WIDTH * GLYPH_TEX_HEIGHT, self->aux_buf);

  g_hash_table_unref (self->font_faces);
  g_hash_table_unref (self->font_entries);

  FT_Done_Library (self->ft_lib);

  glDeleteTextures (1, &self->glyph_tex);

  g_list_free_full (self->columns, g_free);

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
                const gchar *face_filename,
                guint        face_index)
{
  gint err;
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

static gint
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

static TexColumn *
find_empty_area (GlrTexCache *self,
                 guint        width,
                 guint        height,
                 gboolean     only_alpha)
{
  GList *node;
  TexColumn *column = NULL;

  if (width >= GLYPH_TEX_WIDTH || height >= GLYPH_TEX_HEIGHT)
    return NULL;

  /* look for an existing column that has enough space left at the bottom */
  node = self->columns;
  while (node != NULL)
    {
      column = node->data;
      if (column->width >= width && GLYPH_TEX_HEIGHT - column->first_y >= height)
        break;

      node = g_list_next (node);
      column = NULL;
    }

  if (column != NULL)
    return column;

  /* need to create a new column */
  node = g_list_last (self->columns);
  if (node == NULL)
    {
      /* there is no columns defined yet, create a new one */
      column = g_new0 (TexColumn, 1);
      column->x = 0;
      column->width = width;
      column->first_y = 0;

      self->columns = g_list_insert_sorted (self->columns,
                                            column,
                                            compare_column_widths);
    }
  else
    {
      TexColumn *new_column;

      g_assert (self->right_most_column != NULL);
      if (GLYPH_TEX_WIDTH - (self->right_most_column->x + self->right_most_column->width) < width)
        return NULL;

      new_column = g_new0 (TexColumn, 1);
      new_column->x = self->right_most_column->x + self->right_most_column->width;
      new_column->width = width;
      new_column->first_y = 0;

      self->columns = g_list_insert_sorted (self->columns,
                                            new_column,
                                            compare_column_widths);
      return new_column;
    }

  return column;
}

/* public API */

GlrTexCache *
glr_tex_cache_new (void)
{
  GlrTexCache *self;

  self = g_slice_new0 (GlrTexCache);
  self->ref_count = 1;

  self->right_most_column = NULL;

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

  glEnable (GL_BLEND);

  /* create the font glyph texture */
  self->aux_buf = g_slice_alloc (GLYPH_TEX_WIDTH * GLYPH_TEX_HEIGHT);

  glEnable (GL_TEXTURE_2D);
  glActiveTexture (GL_TEXTURE1);
  glGenTextures (1, &self->glyph_tex);
  // g_print ("%u\n", self->glyph_tex);
  glBindTexture (GL_TEXTURE_2D, self->glyph_tex);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  memset (self->aux_buf, 0x00, GLYPH_TEX_WIDTH * GLYPH_TEX_HEIGHT);
  glTexImage2D (GL_TEXTURE_2D,
                0,
                GL_R8,
                GLYPH_TEX_WIDTH, GLYPH_TEX_HEIGHT,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                self->aux_buf);

  return self;
}

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

const GlrTexSurface *
glr_tex_cache_lookup_font_glyph (GlrTexCache *self,
                                 const gchar *face_filename,
                                 guint        face_index,
                                 gsize        font_size,
                                 guint32      unicode_char)
{
  FT_Face face;
  gchar *surface_id;
  gchar *face_id;
  GlrTexSurface *surface = NULL;

  surface_id = g_strdup_printf ("%s:%u:%lu:%u",
                                face_filename,
                                face_index,
                                font_size,
                                unicode_char);
  surface = g_hash_table_lookup (self->font_entries, surface_id);
  if (surface != NULL)
    goto out;

  face_id = g_strdup_printf ("%s:%u", face_filename, face_index);
  face = g_hash_table_lookup (self->font_faces, face_id);
  if (face == NULL)
    {
      face = load_font_face (self, face_filename, face_index);
      if (face == NULL)
        {
          /* @TODO: font face could not be loaded, throw error */
          g_free (face_id);
          goto out;
        }

      g_hash_table_insert (self->font_faces, g_strdup (face_id), face);
    }
  g_free (face_id);

  // set char size
  gint err;
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

  // get glyph index
  FT_UInt glyph_index;
  glyph_index = FT_Get_Char_Index (face, unicode_char);

  // load glyph
  err = FT_Load_Glyph (face, glyph_index, FT_LOAD_NO_BITMAP);
  if (err != 0)
    {
      g_printerr ("Error loading glyph: %d\n", err);
      goto out;
    }

  // render glyph if it is not embedded bitmap
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
  TexColumn *column = find_empty_area (self, bmp.width + 1, bmp.rows + 1, TRUE);
  if (column == NULL)
    {
      /* @FIXME: no space found for the new glyph,
         we need a flushing mechanism */
      // g_printerr ("Out of space in the texture cache\n");
      goto out;
    }

  glActiveTexture (GL_TEXTURE1);
  glBindTexture (GL_TEXTURE_2D, self->glyph_tex);
  glTexSubImage2D (GL_TEXTURE_2D,
                   0,
                   column->x + 1, column->first_y + 1,
                   bmp.width,
                   bmp.rows,
                   GL_RED,
                   GL_UNSIGNED_BYTE,
                   bmp.buffer);

  surface = g_slice_new (GlrTexSurface);
  surface->tex_id = self->glyph_tex;
  surface->left = column->x + 1;
  surface->top =  column->first_y + 1;
  surface->width = bmp.width;
  surface->height = bmp.rows;

  column->first_y += bmp.rows + 1;

  if (self->right_most_column == NULL ||
      column->x + column->width + 1 > self->right_most_column->x + self->right_most_column->width)
    {
      self->right_most_column = column;
    }

  g_hash_table_insert (self->font_entries,
                       g_strdup (surface_id),
                       surface);

 out:
  g_free (surface_id);

  return surface;
}
