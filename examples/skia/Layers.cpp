#include "SkExample.h"
#include "SkDevice.h"
#include <stdio.h>
#include <time.h>

#define WIDTH   960
#define HEIGHT  960

#define MAX_LAYERS 20
#define SCALE 10

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

class Layers : public SkExample {
public:
  Layers(SkExampleWindow* window)
  : SkExample(window)
  {
    fName = "Layers";

    fWindow->setupBackend(SkExampleWindow::kGPU_DeviceType);
    fWindow->setTitle("Layers - Skia");
    window->setSize(WIDTH, HEIGHT);
  }

protected:
  virtual void draw(SkCanvas* canvas) SK_OVERRIDE {
    static int frame = 0;
    SkPaint paint;

    frame++;

    // Clear background
    canvas->drawColor(SK_ColorGRAY);
    paint.setAntiAlias(true);

    int i, j, k;

    paint.setStyle(SkPaint::kFill_Style);

    for (k = 1; k <= MAX_LAYERS; k++)
      for (i = 0; i < SCALE; i++)
        for (j = 0; j < SCALE; j++)
          {
            canvas->save();
            canvas->translate(WIDTH / SCALE * i + k*20, HEIGHT / SCALE * j);
            canvas->rotate(0.0 - (frame % 1080) * ((MAX_LAYERS - k)/2.0 + 1.0));
            SkColor color = getColor(k * 500, 200);
            paint.setColor(color);
            SkRect rect = SkRect::MakeXYWH(SkIntToScalar(0),
                                           SkIntToScalar(0),
                                           SkIntToScalar(30 + WIDTH/SCALE/(k+1)),
                                           SkIntToScalar(30 + WIDTH/SCALE/(k+1)));
            SkRRect rrect;
            rrect.setRectXY(rect, 5, 5);
            canvas->drawRRect(rrect, paint);
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
  return new Layers(window);
}
static SkExample::Registry registry(MyFactory);
