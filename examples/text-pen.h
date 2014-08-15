#ifndef _TEXT_PEN_
#define _TEXT_PEN_

#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-glib.h>
#include <stdbool.h>

typedef enum
  {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_RIGHT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_JUSTIFY
  } TextAlignment;

typedef enum
  {
    TEXT_DIRECTION_LTR,
    TEXT_DIRECTION_RTL,
    TEXT_DIRECTION_TTB
  } TextDirection;

typedef struct
{
  int32_t x;
  int32_t y;
  uint32_t codepoint;
  uint32_t width;
} TextPenGlyph;

typedef struct
{
  /* config */
  int32_t start_x;
  int32_t start_y;
  int32_t line_spacing;
  int32_t letter_spacing;
  uint32_t line_width;
  uint8_t alignment;
  uint8_t direction;
  uint32_t space_codepoint;

  /* state */
  int32_t x;
  int32_t y;

  TextPenGlyph chars[2000];
  uint32_t char_count;
  uint32_t num_chars;

  int32_t line_start_index;
  int32_t word_start_index;

  bool wrap_last_word;
} TextPen;


void    text_pen_reset      (TextPen *self);

bool    text_pen_add_glyph  (TextPen               *self,
                             FT_Face                font_face,
                             const hb_glyph_info_t *glyph_info,
                             hb_glyph_position_t   *glyph_pos);

void    text_pen_flush_line (TextPen *self);

#endif // _TEXT_PEN_
