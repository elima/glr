#ifndef _UTILS_H_
#define _UTILS_H_

#include <glib.h>

typedef void (* DrawCallback)   (guint    frame,
                                 gfloat   zoom_factor,
                                 gpointer user_data);
typedef void (* ResizeCallback) (guint    width,
                                 guint    height,
                                 gpointer user_data);

gint utils_initialize_x11  (guint width, guint height, const gchar *win_title);
void utils_finalize_x11    (void);
gint utils_initialize_egl  (void);
void utils_finalize_egl    (void);

void utils_main_loop       (DrawCallback   draw_cb,
                            ResizeCallback resize_cb,
                            gpointer       user_data);

#endif /* _UTILS_H_ */
