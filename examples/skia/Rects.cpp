#include "SkExample.h"
#include "SkDevice.h"
#include <stdio.h>
#include <time.h>

#define WIDTH   960
#define HEIGHT  960

#define SCALE 120

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

class Rects : public SkExample {
public:
  Rects(SkExampleWindow* window)
  : SkExample(window)
  {
    fName = "Rects";

    fWindow->setupBackend(SkExampleWindow::kGPU_DeviceType);
    fWindow->setTitle("Rects - Skia");
    window->setSize(WIDTH, HEIGHT);
  }

protected:
  virtual void draw(SkCanvas* canvas) SK_OVERRIDE {
    static int frame = 0;
    SkPaint paint;

    frame++;

    // Clear background
    canvas->drawColor(SK_ColorWHITE);
    paint.setAntiAlias(true);

    int i, j;
    int scale = SCALE / 2;

    // filled rects
    paint.setStyle(SkPaint::kFill_Style);
    for (i = 0; i < scale; i++)
      for (j = 0; j < scale; j++)
        {
          double rotation = ((frame + i + j) / 25.0 * 180.0);
          long unsigned int scaling = 60;

          canvas->save();
          canvas->translate(SkIntToScalar(i * WIDTH / scale), SkIntToScalar(j * HEIGHT / scale));
          canvas->rotate(rotation);
          canvas->scale (cos ((M_PI/scaling) * ((frame + i + j) % scaling)) + 1,
                         cos ((M_PI/scaling) * ((frame + i + j) % scaling)) + 1);
          SkColor color = getColor(frame + i + j + ((i + j %2) * 3), 255);
          SkRect rect = SkRect::MakeXYWH(SkIntToScalar(0),
                                         SkIntToScalar(0),
                                         SkIntToScalar(WIDTH / scale / 2),
                                         SkIntToScalar(HEIGHT / scale / 2));
          paint.setColor(color);
          canvas->drawRect(rect, paint);
          canvas->restore();
        }

    // stroked rects
    paint.setStyle(SkPaint::kStroke_Style);
    for (i = 0; i < scale; i++)
      for (j = 0; j < scale; j++)
        {
          double rotation = ((frame + i + j) / 25.0 * 180.0);
          long unsigned int scaling = 75;

          canvas->save();
          canvas->translate(SkIntToScalar(i * WIDTH / scale + WIDTH/scale/2),
                            SkIntToScalar(j * HEIGHT / scale));
          canvas->rotate(-rotation);
          canvas->scale (cos ((M_PI/scaling) * ((frame + i + j) % scaling)) + 1,
                         cos ((M_PI/scaling) * ((frame + i + j) % scaling)) + 1);
          SkColor color = getColor(frame + i + j + ((i + j %2) * 3), 255);
          SkRect rect = SkRect::MakeXYWH(SkIntToScalar(0),
                                         SkIntToScalar(0),
                                         SkIntToScalar(WIDTH / scale / 2),
                                         SkIntToScalar(HEIGHT / scale / 2));
          paint.setColor(color);
          canvas->drawRect(rect, paint);
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
  return new Rects(window);
}
static SkExample::Registry registry(MyFactory);
