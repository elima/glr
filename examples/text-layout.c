#include <glib.h>
#include <sys/time.h>
#include <math.h>

#include "../glr.h"
#include "../glr-tex-cache.h"
#include "utils.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <ftadvanc.h>
#include <ftsnames.h>
#include <tttables.h>

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-glib.h>

#define MSAA_SAMPLES 0

#define WINDOW_WIDTH    1280
#define WINDOW_HEIGHT    960

#define FONT_FILE  (FONTS_DIR "Cantarell-Regular.otf")
// #define FONT_FILE  (FONTS_DIR "DejaVuSansMono-Bold.ttf")
// #define FONT_FILE  (FONTS_DIR "Ubuntu-Title.ttf")
// #define FONT_FILE  (FONTS_DIR "amiri-regular.ttf")
// #define FONT_FILE  (FONTS_DIR "fireflysung.ttf")

#define FONT_SIZE 16
#define SCALE 5

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;
static GlrLayer *layer = NULL;

static guint window_width, window_height;
static gfloat zoom_factor = 1.0;

static void
draw_layer_in_thread (GlrLayer *layer, gpointer user_data)
{
  gint i, j;
  GlrFont font = {0};
  guint width, height;
  guint frame = * (guint *) user_data;

  font.face = FONT_FILE;
  font.face_index = 0;

  glr_target_get_size (target, &width, &height);

  font.size = FONT_SIZE;
  font.size = (guint) floor (FONT_SIZE * zoom_factor);

  guint scroll_speed = 120.0;
  gfloat offset_y = sin ((M_PI*2.0/scroll_speed) * (frame % scroll_speed)) * 250.0;
  glr_layer_translate (layer, 0, offset_y);

  FT_Face ft_face;
  hb_font_t *hb_ft_font;
  hb_face_t *hb_ft_face;
  const gchar *text =
    "SINCE the ancients (as we are told by Pappus), made great account on \
the science of mechanics in the investigation of natural things: and the \
moderns, laying aside substantial forms and occult qualities, have endeavoured \
to subject the phenomena of nature to the laws of mathematics, I \
have in this treatise cultivated mathematics so far as it regards philosophy. \
The ancients considered mechanics in a twofold respect; as rational, which \
proceeds accurately by demonstration; and practical. To practical me \
chanics all the manual arts belong, from which mechanics took its name. \
Rut as artificers do not work with perfect accuracy, it comes to pass that \
mechanics is so distinguished from geometry, that what is perfectly accurate \
is called geometrical, what is less so, is called mechanical. But the \
errors are not in the art, but in the artificers. He that works with less \
accuracy is an imperfect mechanic; and if any could work with perfect \
accuracy, he would be the most perfect mechanic of all; for the description \
if right lines and circles, upon which geometry is founded, belongs to me \
chanics. Geometry does not teach us to draw these lines, but requires \
them to be drawn; for it requires that the learner should first be taught \
to describe these accurately, before he enters upon geometry; then it shows \
how by these operations problems may be solved. To describe right lines \
and circles are problems, but not geometrical problems. The solution of \
these problems is required from mechanics; and by geometry the use of \
them, when so solved, is shown; and it is the glory of geometry that from \
those few principles, brought from without, it is able to produce so many \
things. Therefore geometry is founded in mechanical practice, and is \
nothing but that part of universal mechanics which accurately proposes \
and demonstrates the art of measuring. But since the manual arts are \
chiefly conversant in the moving of bodies, it comes to pass that geometry \
is commonly referred to their magnitudes, and mechanics to their motion. \
In this sense rational mechanics will be the science of motions resulting \
from any forces whatsoever, and of the forces required to produce any motions, \
accurately proposed and demonstrated.";

  // text = "هذه هي بعض النصوص العربي";
  // text =  "這是一些中文";

  ft_face = glr_tex_cache_lookup_face (glr_context_get_texture_cache (context),
                                       font.face,
                                       font.face_index);
  g_assert (ft_face != NULL);
  g_assert(! FT_Set_Char_Size (ft_face, 0, font.size * 64, 96, 96));
  hb_ft_font = hb_ft_font_create (ft_face, NULL);
  hb_ft_face = hb_ft_face_create (ft_face, NULL);

  hb_buffer_t *buf = hb_buffer_create();

  hb_buffer_set_unicode_funcs (buf, hb_glib_get_unicode_funcs ());
  hb_buffer_set_direction (buf, HB_DIRECTION_LTR);
  hb_buffer_set_script(buf, HB_SCRIPT_LATIN); /* see hb-unicode.h */
  hb_buffer_set_language (buf, hb_language_from_string ("en", 2));

  /* Layout the text */
  hb_buffer_add_utf8 (buf, text, strlen (text), 0, strlen (text));
  hb_shape (hb_ft_font, buf, NULL, 0);

  /* Hand the layout to cairo to render */
  guint glyph_count;
  hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos (buf, &glyph_count);
  hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions (buf, &glyph_count);

  gint string_width_in_pixels = 0;
  gint string_height_in_pixels = 0;
  gint px = 10, py = 0;
  for (i = 0; i < glyph_count; ++i)
    {
      string_width_in_pixels += glyph_pos[i].x_advance / 64;
      string_height_in_pixels = MAX (string_height_in_pixels, abs (glyph_pos[i].y_offset / 64));
    }


  guint line_spacing = font.size * 1.2;
  guint line_width = width - 120;
  guint letter_spacing = 0;

  px = 20;
  py += 20 + line_spacing;

  for (j = 0; j < SCALE; j++)
    {
      gint x, y;
      for (i = 0; i < glyph_count; ++i)
        {
          x = px + (glyph_pos[i].x_offset / 64);
          y = py - (glyph_pos[i].y_offset / 64);
          px += glyph_pos[i].x_advance / 64 + letter_spacing;
          py += glyph_pos[i].y_advance / 64;

          if (px >= line_width && glyph_info[i].codepoint == FT_Get_Char_Index (ft_face, 32))
            {
              px = 20;
              py += line_spacing;
              continue;
            }

          glr_layer_set_transform_origin (layer, -2.5, 0.5);
          // glr_layer_rotate (layer, ((frame + j)*3 + i) / 150.0 * 180.0);

          glr_layer_draw_char (layer,
                               glyph_info[i].codepoint,
                               x,
                               y,
                               &font,
                               glr_color_from_rgba (0, 0, 0, 255));
        }
    }

  hb_buffer_destroy (buf);
  hb_font_destroy (hb_ft_font);
  hb_face_destroy (hb_ft_face);

  /* it is necessary to always call finish() on on a layer, otherwise
     glr_canvas_finish_frame() will wait forever */
  glr_layer_finish (layer);
}

static void
draw_func (guint frame, gfloat _zoom_factor, gpointer user_data)
{
  guint current_width, current_height;

  zoom_factor = _zoom_factor;

  glr_target_get_size (target, &current_width, &current_height);

  glClearColor (1.0, 1.0, 1.0, 1.0);
  glClear (GL_COLOR_BUFFER_BIT);

  glr_canvas_start_frame (canvas);
  glr_canvas_clear (canvas, glr_color_from_rgba (255, 255, 255, 255));

  /* attach our layer */
  glr_canvas_attach_layer (canvas, 0, layer);

  /* invalidate layer and draw it */
  glr_layer_redraw_in_thread (layer, draw_layer_in_thread, &frame);

  /* finally, sync all layers and render */
  glr_canvas_finish_frame (canvas);

  /* blit target's framebuffer into default FBO */
  glBindFramebuffer (GL_FRAMEBUFFER, 0);
  glBindFramebuffer (GL_READ_FRAMEBUFFER, glr_target_get_framebuffer (target));
  glBlitFramebuffer (0, 0,
                     current_width, current_height,
                     0, window_height - current_height,
                     current_width, window_height,
                     GL_COLOR_BUFFER_BIT,
                     GL_NEAREST);
}

static void
resize_func (guint width, guint height, gpointer user_data)
{
  glr_target_resize (target, width, height);
  window_width = width;
  window_height = height;
}

gint
main (int argc, char* argv[])
{
  window_width = WINDOW_WIDTH;
  window_height = WINDOW_HEIGHT;

  /* init windowing system */
  utils_initialize_x11 (WINDOW_WIDTH, WINDOW_HEIGHT, "TextLayout");
  utils_initialize_egl ();

  /* init glr */
  context = glr_context_new ();
  target = glr_target_new (context, WINDOW_WIDTH, WINDOW_HEIGHT, MSAA_SAMPLES);
  canvas = glr_canvas_new (context, target);
  layer = glr_layer_new (context);

  /* start the show */
  utils_main_loop (draw_func, resize_func, NULL);

  /* clean up */
  glr_layer_unref (layer);
  glr_canvas_unref (canvas);
  glr_target_unref (target);
  glr_context_unref (context);

  utils_finalize_egl ();
  utils_finalize_x11 ();

  return 0;
}
