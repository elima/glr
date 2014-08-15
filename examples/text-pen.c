#include <stdbool.h>
#include "text-pen.h"

void
text_pen_reset (TextPen *self)
{
  self->x = self->start_x;
  self->y = self->start_y + self->line_spacing;
  self->char_count = 0;
  self->num_chars = 0;
  self->line_start_index = -1;
  self->word_start_index = -1;
  self->wrap_last_word = false;
}

bool
text_pen_add_glyph (TextPen               *self,
                    FT_Face                font_face,
                    const hb_glyph_info_t *glyph_info,
                    hb_glyph_position_t   *glyph_pos)
{
  uint32_t index;
  int32_t word_start_index = 0;
  int32_t i;

  if (glyph_info->codepoint == self->space_codepoint &&
      self->line_start_index == -1)
    {
      return false;
    }

  index = self->char_count;
  self->char_count++;
  self->num_chars = self->char_count;

  self->chars[index].x = self->x + (glyph_pos->x_offset / 64);
  self->chars[index].y = self->y - (glyph_pos->y_offset / 64);
  self->chars[index].codepoint = glyph_info->codepoint;
  // self->chars[index].width = glyph_pos->x_advance / 64;

  self->x += glyph_pos->x_advance / 64 + self->letter_spacing;
  self->y += glyph_pos->y_advance / 64;

  if (self->line_start_index == -1)
    self->line_start_index = index;

  /* @TODO: consider any non-word character. By now, only space */
  if (glyph_info->codepoint == self->space_codepoint)
    {
      word_start_index = self->word_start_index;
      self->word_start_index = -1;
    }
  else if (self->word_start_index == -1)
    {
      self->word_start_index = index;
    }

  if (self->x >= self->start_x + self->line_width)
    {
      /* check if we can break line */
      if (self->word_start_index == -1)
        {
          if (self->wrap_last_word)
            {
              int32_t dx;

              self->wrap_last_word = false;

              self->num_chars = word_start_index;
              dx = self->start_x - self->chars[word_start_index].x;

              for (i = word_start_index; i < self->char_count - 1; i++)
                {
                  self->chars[i].x += dx;
                  self->chars[i].y += self->line_spacing;
                  // self->chars[i].y -= self->line_spacing;
                }

              self->x += dx;
            }
          else
            {
              self->x = self->start_x;
              self->num_chars = self->char_count;
              self->line_start_index = -1;
            }

          self->y += self->line_spacing;
          // self->y -= self->line_spacing;

          return true;
        }
      else
        {
          self->wrap_last_word = true;
        }
    }

  return false;
}

void
text_pen_flush_line (TextPen *self)
{
  int32_t num_chars_left;
  int32_t i;

  num_chars_left = self->char_count - self->num_chars;
  // g_print ("%d\n", num_chars_left);

  if (self->num_chars > 0)
    {
      for (i = 0; i < num_chars_left; i++)
        {
          self->chars[i].x = self->chars[self->num_chars - 1 + i].x;
          self->chars[i].y = self->chars[self->num_chars - 1 + i].y;
          self->chars[i].codepoint = self->chars[self->num_chars - 1 + i].codepoint;
        }
    }

  self->char_count = MAX (0, num_chars_left);
  self->num_chars = self->char_count;
}
