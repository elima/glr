PKG_CONFIG_LIBS=glib-2.0 libglfw
LIBS=-lm -lglr -lGL
CFLAGS=-ggdb -O0 -L.. -Wall -DFONTS_DIR=\"`pwd`/fonts/\"

all: Makefile examples

examples: \
	layers \
	rects-and-text \
	rects-threaded

layers: layers.c
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		layers.c \
		-o layers

rects-and-text: rects-and-text.c
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		rects-and-text.c \
		-o rects-and-text

rects-threaded: rects-threaded.c
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		rects-threaded.c \
		-o rects-threaded
