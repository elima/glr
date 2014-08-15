#ifndef _TEXT_NODE_H
#define _TEXT_NODE_H_

#include "text-pen.h"

typedef struct TextNode TextNode;

typedef bool (* TextNodeDrawCharCb) (TextNode *self,
                                     uint32_t  codepoint,
                                     int32_t   left,
                                     int32_t   top,
                                     void     *user_data);

TextNode * text_node_new  (const char     *contents,
                           FT_Face         ft_face,
                           size_t          font_size,
                           hb_script_t     script,
                           hb_direction_t  direction,
                           hb_language_t   lang);
void       text_node_free (TextNode *self);

void       text_node_draw (TextNode           *self,
                           TextPen            *pen,
                           TextNodeDrawCharCb  callback,
                           void               *user_data);

#endif // _TEXT_NODE_H_
