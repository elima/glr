#include "SkExample.h"
#include "SkDevice.h"
#include <stdio.h>
#include <time.h>

#define WIDTH   960
#define HEIGHT  960

#define FONT_SIZE 24
#define SCALE 60

static void
log_times_per_second (const char *msg_format)
{
  static unsigned int counter = 0;
  static time_t last_time = 0;
  static int interval = 2;
  time_t cur_time;

  if (last_time == 0)
    last_time = time (NULL);

  counter++;

  cur_time = time (NULL);
  if (cur_time >= last_time + interval)
    {
      printf (msg_format, (double) (counter / interval));
      counter = 0;
      last_time = cur_time;
    }
}

class RectsAndText : public SkExample {
public:
  RectsAndText(SkExampleWindow* window)
  : SkExample(window)
  {
    fName = "RectsAndText";

    fWindow->setupBackend(SkExampleWindow::kGPU_DeviceType);
    fWindow->setTitle("RectsAndText - Skia");
    window->setSize(WIDTH, HEIGHT);
  }

protected:
  virtual void draw(SkCanvas* canvas) SK_OVERRIDE {
    static int frame = 0;
    SkPaint paint;

    frame++;

    // Clear background
    canvas->drawColor(SK_ColorBLACK);

    int i, j;
    int scale = SCALE;

    paint.setStyle(SkPaint::kFill_Style);

    for (i = 0; i < scale; i++)
      for (j = 0; j < scale; j++)
        {
          canvas->save();

          int step = (j * scale + i + (j % 2)) % 2;
          if (step == 1) {
            paint.setAntiAlias(true);
            canvas->translate(SkIntToScalar(i * WIDTH / scale), SkIntToScalar(j * HEIGHT / scale));
            canvas->rotate(- ((frame*2 + i + j) / 50.0 * 180.0));
            SkColor color = getColor(frame + i + j + ((i + j %2) * 3), 100);
            paint.setColor(color);
            canvas->scale(cos ((M_PI/60.0) * ((frame + i + j/(i+1)) % 60))*2 + 1.0,
                          cos ((M_PI/60.0) * ((frame + i + j/(i+1)) % 60))*2 + 1.0);
            SkRect rect = SkRect::MakeXYWH(SkIntToScalar(0),
                                           SkIntToScalar(0),
                                           SkIntToScalar(WIDTH / scale),
                                           SkIntToScalar(HEIGHT / scale));
            canvas->drawRect(rect, paint);
          } else {
            paint.setAntiAlias(false);
            canvas->translate(SkIntToScalar(i * WIDTH/scale - (j%3)*(FONT_SIZE/2)),
                              SkIntToScalar(j * HEIGHT / scale));
            canvas->rotate((frame*2 + i + j) / 75.0 * 180.0);
            SkColor color = getColor(frame + i + j + ((i + j %2) * 3), 100);
            paint.setColor(color);
            canvas->scale (1.0, 1.0);
            paint.setTextSize(FONT_SIZE);
            char ch[2] = { 0 };
            ch[0] = i % scale + 46;
            canvas->drawText(ch, 1, 0, 0, paint);
          }

          canvas->restore();
        }

    this->fWindow->inval(NULL);
    log_times_per_second("FPS: %04f\n");
  }

private:
  int max(int a, int b) {
    if (a > b)
      return a;
    return b;
  }

  int min(int a, int b) {
    if (a > b)
      return b;
    return a;
  }

  SkColor getColor(int hue, int alpha) {
    int redShift = 180;
    int greenShift = 60;
    int blueShift = -60;
    int red, green, blue;
    double r, g, b;
    r = (max(min((180 - abs(180 - (hue + redShift) % 360)), 120), 60) - 60) / 60.0;
    g = (max(min((180 -abs(180 - (hue + greenShift) % 360)), 120), 60) - 60) / 60.0;
    b = (max(min((180 -abs(180 - (hue + blueShift) % 360)), 120), 60) - 60) / 60.0;
    red = r * 255; green = g * 255; blue = b * 255;
    SkColor color(alpha << 24 | red << 16 | green << 8 | blue);
    return color;
  }
};

static SkExample* MyFactory(SkExampleWindow* window) {
  return new RectsAndText(window);
}
static SkExample::Registry registry(MyFactory);
