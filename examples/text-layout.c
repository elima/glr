#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#include "../glr.h"
#include "../glr-tex-cache.h"
#include "utils.h"
#include "text-node.h"
#include "text-pen.h"

#define M_PI 3.14159265358979323846

#define MSAA_SAMPLES 0

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

#define FONT_FILE_LTR  (FONTS_DIR "Cantarell-Regular.otf")
// #define FONT_FILE_LTR  (FONTS_DIR "DejaVuSansMono-Bold.ttf")
// #define FONT_FILE_LTR  (FONTS_DIR "Ubuntu-Title.ttf")
#define FONT_FILE_RTL  (FONTS_DIR "amiri-regular.ttf")
#define FONT_FILE_TTB  (FONTS_DIR "fireflysung.ttf")

#define FONT_SIZE 16
#define SCALE 4

extern char * strdup (const char *s);

static const gchar *text_english =
  "SINCE the ancients (as we are told by Pappus), made great account on "
  "the science of mechanics in the investigation of natural things: and the "
  "moderns, laying aside substantial forms and occult qualities, have endeavoured "
  "to subject the phenomena of nature to the laws of mathematics, I "
  "have in this treatise cultivated mathematics so far as it regards philosophy. "
  "The ancients considered mechanics in a twofold respect; as rational, which "
  "proceeds accurately by demonstration; and practical. To practical mechanics "
  "all the manual arts belong, from which mechanics took its name. "
  "Rut as artificers do not work with perfect accuracy, it comes to pass that "
  "mechanics is so distinguished from geometry, that what is perfectly accurate "
  "is called geometrical, what is less so, is called mechanical. But the "
  "errors are not in the art, but in the artificers. He that works with less "
  "accuracy is an imperfect mechanic; and if any could work with perfect "
  "accuracy, he would be the most perfect mechanic of all; for the description "
  "if right lines and circles, upon which geometry is founded, belongs to me "
  "chanics. Geometry does not teach us to draw these lines, but requires "
  "them to be drawn; for it requires that the learner should first be taught "
  "to describe these accurately, before he enters upon geometry; then it shows "
  "how by these operations problems may be solved. To describe right lines "
  "and circles are problems, but not geometrical problems. The solution of "
  "these problems is required from mechanics; and by geometry the use of "
  "them, when so solved, is shown; and it is the glory of geometry that from "
  "those few principles, brought from without, it is able to produce so many "
  "things. Therefore geometry is founded in mechanical practice, and is "
  "nothing but that part of universal mechanics which accurately proposes "
  "and demonstrates the art of measuring. But since the manual arts are "
  "chiefly conversant in the moving of bodies, it comes to pass that geometry "
  "is commonly referred to their magnitudes, and mechanics to their motion. "
  "In this sense rational mechanics will be the science of motions resulting "
  "from any forces whatsoever, and of the forces required to produce any motions, "
  "accurately proposed and demonstrated. ";

static const char *text_arabic =
  "ولد إسحاق نيوتن في 25 ديسمبر 1642 (وفق التقويم اليولياني المعمول "
  "به في إنجلترا في ذلك الوقت، الموافق 4 يناير 1643 وفق التقويم الحديث). "
  "في مزرعة وولسثورب في وولسثورب-كلوستروورث, في مقاطعة لينكونشير، بعد وفاة "
  "والده بثلاثة أشهر، الذي كان يعمل مزارعًا واسمه أيضًا إسحاق نيوتن. جاء "
  "والدته المخاض وولدت إبنها بعد ساعة أو ساعتين من منتصف الليل وكان القمر "
  "في تلك الليلة بدرًا، ونظرًا لولادته مبكرًا فقد كان طفلاً ضئيل الحجم حتى "
  "أن امرأتان كانتا تعتنيان بأمه أُرسلتا لجلب الدواء من الجوار ولكنهما بدلاً"
  "من أن تسرعا في جلب ذلك الدواء قررتا أن تستريحا في الطريق ظنًا منهما أن "
  "الطفل المولود ربما قد فارق الحياة بسبب حجمه. وقد وصفته أمه حنا إيسكوف "
  "بأنه يمكن وضعه في الكوارت. وعندما بلغ الثالثة من عمره، تزوجت أمه وانتقلت "
  "للعيش في منزل زوجها الجديد، تاركةً ابنها في رعاية جدته لأمه والتي كانت "
  "تدعى مارغريت آيسكوف. لم يحبب إسحاق زوج أمه، بل كان يحس بمشاعر عدائية "
  "تجاه أمه لزواجها منه. صورة مرسومة لنيوتن عام 1702. ومن عمر الثانية عشر "
  "إلى السابعة عشر، التحق نيوتن بمدرسة الملك في جرانتهام، وقد ترك نيوتن "
  "المدرسة في أكتوبر 1659، ليعود إلى مزرعة وولسثورب، حيث وجد أمه قد ترملت "
  "من جديد، ووجدها قد خططت لجعله مزارعًا كأبيه، إلا أنه كان يكره الزراعة. "
  "أقنع هنري ستوكس أحد المعلمين في مدرسة الملك أمه بإعادته للمدرسة، فاستطاع "
  "بذلك نيوتن أن يكمل تعليمه. وبدافع الانتقام من الطلاب المشاغبين، استطاع "
  "نيوتن أن يثبت أنه الطالب الأفضل في المدرسة. اعتبر سيمون بارون-كوهين "
  "عالم النفس في كامبريدج ذلك دليلاً على إصابة نيوتن بمتلازمة أسبرجر. في يونيو "
  "1661، تم قبوله في كلية الثالوث بكامبريدج كطالب عامل، وهو نظام كان شائعً"
  "ا حينها يتضمن دفع الطالب لمصاريف أقل من أقرانه على أن يقوم بأعمال مقابل ذلك. "
  "في ذلك الوقت، استندت تعاليم الكلية على تعاليم أرسطو، التي أكملها نيوتن بتعاليم "
  "الفلاسفة الحديثين كرينيه ديكارت، وعلماء الفلك أمثال نيكولاس كوبرنيكوس وجاليليو "
  "جاليلي ويوهانس كيبلر. في عام 1665، اكتشف نيوتن نظرية ذات الحدين العامة، وبدأ "
  "في تطوير نظرية رياضية أخرى أصبحت في وقت لاحق حساب التفاضل والتكامل. وبعد فترة "
  "وجيزة، حصل نيوتن على درجته العلمية في أغسطس 1665، ثم أغلقت الجامعة كإجراء "
  "احترازي مؤقت ضد الطاعون العظيم.";

static const char *text_cyrilic =
  "Исак Нютон (на английски: Isaac Newton, произнася се Айзък Нютън) е английски "
  "физик, математик, астроном, философ, алхимик и богослов. Приносът на Нютон в "
  "развитието на математиката и различни области на физиката изиграва важна роля в "
  "Научната революция и той е „смятан от мнозина за най-великият и най-влиятелен "
  "учен живял някога“. В областта на механиката Нютон открива закона за всемирното "
  "привличане и чрез предложените Закони за движение поставя основите на класическата "
  "механика. Освен това той формулира принципа за запазване на импулса и момента на "
  "импулса и пръв показва, че движението на небесните тела и на предметите на Земята "
  "се подчинява на общи закони, демонстрирайки съответствието между законите на Кеплер "
  "за движението на планетите и собствената му теория за гравитацията и премахвайки "
  "последните съмнения към хелиоцентричната теория. Сред многобройните проблеми, които "
  "изследва Нютон, са също разлагането и природата на светлината, скоростта на звука, "
  "охлаждането, произходът на звездите, хронологията на Библията, природата на Светата "
  "Троица. Той конструира първия действащ рефлекторен телескоп и развива своя теория "
  "за цветовете, основана на наблюденията на разлагането на бялата светлина с призма. "
  "Работейки над проблемите на физиката, Исак Нютон поставя началото, едновременно и "
  "независимо от Готфрид Лайбниц, на математическия анализ, който е в основата на "
  "развитието на науката до наши дни. Той също така описва разлагането на бином, "
  "повдигнат на степен, създава числен метод за намиране на корените на функция и "
  "допринася за изследванията на степенните редове. Нютон е силно религиозен, "
  "придържа се към неортодоксални християнски възгледи и освен на научна тема "
  "пише текстове и в областта на библейската херменевтика и окултизма. Той отказва "
  "да стане свещеник и да получи последно причастие, като вероятно отхвърля догмата "
  "за Светата Троица.";

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;

static uint32_t window_width, window_height;
static float zoom_factor = 1.0;

static bool need_redraw = true;

static TextNode *text_node_en = NULL;
static GlrFont font_en = {0};

static TextNode *text_node_ar = NULL;
static GlrFont font_ar = {0};

static TextNode *text_node_cy = NULL;
static GlrFont font_cy = {0};

struct DrawData
{
  GlrCanvas *canvas;
  GlrFont *font;
  float offset_y;
  GlrColor color;
};

static bool
draw_char_callback (TextNode *text_node,
                    uint32_t  codepoint,
                    int32_t   left,
                    int32_t   top,
                    void     *user_data)
{
  struct DrawData *data = user_data;

  /*
  if (top + data->offset_y + data->font->size < 0.0
      || top + data->offset_y - data->font->size > window_height)
    {
      return false;
    }
  */

  glr_canvas_draw_char (data->canvas,
                        codepoint,
                        left,
                        top + data->offset_y,
                        data->font,
                        data->color);

  return true;
}

static void
draw_frame (uint32_t frame)
{
  uint32_t width;
  GlrStyle style = GLR_STYLE_DEFAULT;

  width = window_width;

  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 10.0);
  glr_background_set_color (&(style.background),
                            glr_color_from_rgba (255, 255, 255, 255));

  int32_t padding = 20;
  uint32_t panel_width = (width - padding*4) / 2;

  TextPen pen;
  pen.start_y = padding;
  pen.line_spacing = font_en.size * 1.4;
  pen.line_width = panel_width - padding * 2.0;
  pen.letter_spacing = 0;
  pen.alignment = TEXT_ALIGN_LEFT;

  struct DrawData data = {
    canvas, NULL, 0.0, glr_color_from_rgba (0, 0, 0, 255)
  };

  // draw english text
  pen.start_x = width/2 - panel_width + padding - padding/2;
  pen.direction = TEXT_DIRECTION_LTR;
  text_pen_reset (&pen);

  data.font = &font_en;
  data.offset_y = 0.0;

  for (int i = 0; i < SCALE; i++)
    text_node_draw (text_node_en, &pen, draw_char_callback, &data);

  // draw cyrilic text
  pen.start_x = width/2 + padding/2 + padding;
  pen.direction = TEXT_DIRECTION_LTR;
  text_pen_reset (&pen);

  data.font = &font_cy;
  data.offset_y = 0.0;

  for (int i = 0; i < SCALE; i++)
    text_node_draw (text_node_cy, &pen, draw_char_callback, &data);
}

static void
draw_func (uint32_t frame, float _zoom_factor, void *user_data)
{
  if (_zoom_factor != zoom_factor)
    {
      need_redraw = true;
      zoom_factor = _zoom_factor;
    }

  /* invalidate canvas and draw */
  if (need_redraw)
    {
      FT_Face ft_face;

      // english text
      if (text_node_en != NULL)
        {
          text_node_free (text_node_en);
          text_node_en = NULL;
        }

      font_en.face = FONT_FILE_LTR;
      font_en.face_index = 0;
      font_en.size = (uint32_t) floor (FONT_SIZE * zoom_factor);

      ft_face = glr_tex_cache_lookup_face (glr_context_get_texture_cache (context),
                                           font_en.face,
                                           font_en.face_index);
      assert (ft_face != NULL);

      text_node_en = text_node_new (text_english,
                                    ft_face,
                                    font_en.size,
                                    HB_SCRIPT_LATIN,
                                    HB_DIRECTION_LTR,
                                    hb_language_from_string ("en", 2));

      // cyrilic text
      if (text_node_cy != NULL)
        {
          text_node_free (text_node_cy);
          text_node_cy = NULL;
        }

      font_cy.face = FONT_FILE_LTR;
      font_cy.face_index = 0;
      font_cy.size = (uint32_t) floor (FONT_SIZE * zoom_factor);

      ft_face = glr_tex_cache_lookup_face (glr_context_get_texture_cache (context),
                                           font_cy.face,
                                           font_cy.face_index);
      assert (ft_face != NULL);

      text_node_cy = text_node_new (text_cyrilic,
                                    ft_face,
                                    font_cy.size,
                                    HB_SCRIPT_LATIN,
                                    HB_DIRECTION_LTR,
                                    hb_language_from_string ("bg", 2));

      glr_canvas_clear (canvas, glr_color_from_rgba (255, 255, 255, 255));
      glr_canvas_reset_transform (canvas);
      draw_frame (frame);

      need_redraw = false;
    }
  else
    {
      glClearColor (1.0, 1.0, 1.0, 1.0);
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

  glr_canvas_reset_transform (canvas);

  static const uint32_t anim_speed = 360.0;
  static float anim_factor = 0.0;
  static uint32_t anim_frame = 0;

  static const uint32_t scroll_speed = 180.0;
  static float scroll_factor = 0.0;
  static uint32_t scroll_frame = 0.0;

  if ((uint32_t) ((frame / (anim_speed / 2))) % 2 == 0)
    {
      anim_frame++;
      anim_factor = sin ((M_PI*2.0/anim_speed) * ((anim_frame + (anim_speed/4)) % anim_speed));
    }
  else
    {
      scroll_frame++;
      scroll_factor = sin ((M_PI*2.0/scroll_speed) * ((scroll_frame + (scroll_speed/2)) % scroll_speed));
    }

  glr_canvas_translate (canvas,
                        anim_factor * 470.0,
                        fabs (anim_factor) * 150.0 + scroll_factor * 600.0 - 300.0,
                        -2500.0 + fabs (anim_factor) * 3300.0);

  glr_canvas_rotate (canvas,
                     -M_PI/2.0 + fabs (anim_factor) * M_PI/2.0,
                     0.0, // M_PI/2.0 - fabs (anim_factor) * M_PI/2.0,
                     M_PI + anim_factor * M_PI);
  glr_canvas_set_transform_origin (canvas,
                                   window_width/2.0,
                                   window_height/2.0,
                                   0.0);

  glr_canvas_flush (canvas);

  // if canvas has a target, blit its framebuffer into default FBO
  if (glr_canvas_get_target (canvas) != NULL)
    {
      uint32_t target_width, target_height;
      glr_target_get_size (target, &target_width, &target_height);

      glBindFramebuffer (GL_FRAMEBUFFER, 0);
      glClearColor (0.0, 0.0, 0.0, 0.0);
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glBindFramebuffer (GL_READ_FRAMEBUFFER, glr_target_get_framebuffer (target));
      glBlitFramebuffer (0, 0,
                         target_width, target_height,
                         0, window_height - target_height,
                         target_width, window_height,
                         GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                         GL_NEAREST);
    }
}

static void
resize_func (uint32_t width, uint32_t height, void *user_data)
{
  glr_target_resize (target, width, height);
  window_width = width;
  window_height = height;

  need_redraw = true;

  glViewport (0, 0, width, height);
}

int
main (int argc, char* argv[])
{
  /* init windowing system */
  utils_initialize_egl (WINDOW_WIDTH, WINDOW_HEIGHT, "Text layout");

  /* init glr */
  context = glr_context_new ();
  target = glr_target_new (context, WINDOW_WIDTH, WINDOW_HEIGHT, MSAA_SAMPLES);
  canvas = glr_canvas_new (context, MSAA_SAMPLES > 0 ? target : NULL);

  /* start the show */
  utils_main_loop (draw_func, resize_func, NULL);

  /* clean up */
  glr_canvas_unref (canvas);
  glr_target_unref (target);
  glr_context_unref (context);

  utils_finalize_egl ();

  return 0;
}
