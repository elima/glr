#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>

typedef void (* DrawCallback)   (uint32_t  frame,
                                 float     zoom_factor,
                                 void     *user_data);
typedef void (* ResizeCallback) (uint32_t  width,
                                 uint32_t  height,
                                 void     *user_data);

int  utils_initialize_egl  (uint32_t    win_width,
                            uint32_t    win_height,
                            const char *win_title);
void utils_finalize_egl    (void);

void utils_main_loop       (DrawCallback    draw_cb,
                            ResizeCallback  resize_cb,
                            void           *user_data);

#endif /* _UTILS_H_ */
