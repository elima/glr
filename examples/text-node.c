#include "text-node.h"

struct TextNode
{
  FT_Face ft_face;
  hb_script_t script;
  hb_direction_t direction;
  hb_language_t lang;

  hb_font_t *hb_ft_font;
  hb_face_t *hb_ft_face;
  hb_buffer_t *buf;

  uint32_t glyph_count;
  hb_glyph_info_t *glyph_info;
  hb_glyph_position_t *glyph_pos;
};

static void
text_node_flush_text_pen (TextNode           *self,
                          TextPen            *pen,
                          TextNodeDrawCharCb  callback,
                          void               *user_data)
{
  int32_t k;

  for (k = 0; k < pen->num_chars; k++)
    {
      if (! callback (self,
                      pen->chars[k].codepoint,
                      pen->chars[k].x,
                      pen->chars[k].y,
                      user_data))
        {
          break;
        }
    }

  text_pen_flush_line (pen);
}

TextNode *
text_node_new (const char     *contents,
               FT_Face         ft_face,
               size_t          font_size,
               hb_script_t     script,
               hb_direction_t  direction,
               hb_language_t   lang)
{
  TextNode *self = calloc (1, sizeof (TextNode));

  self->ft_face = ft_face;
  FT_Reference_Face (ft_face);

  self->script = script;
  self->direction = direction;
  self->lang = lang;

  FT_Set_Char_Size (ft_face, 0, font_size * 64, 96, 96);

  self->hb_ft_font = hb_ft_font_create (ft_face, NULL);
  self->hb_ft_face = hb_ft_face_create (ft_face, NULL);

  self->buf = hb_buffer_create ();

  hb_buffer_set_unicode_funcs (self->buf, hb_glib_get_unicode_funcs ());
  hb_buffer_set_direction (self->buf, self->direction);
  hb_buffer_set_script (self->buf, self->script);
  hb_buffer_set_language (self->buf, self->lang);

  // layout the text
  hb_buffer_add_utf8 (self->buf,
                      contents,
                      strlen (contents),
                      0,
                      strlen (contents));
  hb_shape (self->hb_ft_font, self->buf, NULL, 0);

  self->glyph_info = hb_buffer_get_glyph_infos (self->buf,
                                                &self->glyph_count);
  self->glyph_pos = hb_buffer_get_glyph_positions (self->buf,
                                                   &self->glyph_count);

  return self;
}

void
text_node_free (TextNode *self)
{
  hb_buffer_destroy (self->buf);
  hb_font_destroy (self->hb_ft_font);
  hb_face_destroy (self->hb_ft_face);

  FT_Done_Face (self->ft_face);

  free (self);
}

void
text_node_draw (TextNode           *self,
                TextPen            *pen,
                TextNodeDrawCharCb  callback,
                void               *user_data)
{
  int32_t i;

  // @FIXME: consider all codepoints that can break lines instead of just space
  pen->space_codepoint = FT_Get_Char_Index (self->ft_face, 32);

  for (i = 0; i < self->glyph_count; ++i)
    {
      int32_t p = i;

      if (hb_buffer_get_direction (self->buf) == HB_DIRECTION_RTL)
        p = self->glyph_count - 1 - i;

      if (text_pen_add_glyph (pen,
                              self->ft_face,
                              &(self->glyph_info[p]),
                              &(self->glyph_pos[p])))
        text_node_flush_text_pen (self, pen, callback, user_data);
    }

  text_node_flush_text_pen (self, pen, callback, user_data);
}
