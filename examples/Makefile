PKG_CONFIG_LIBS=glib-2.0 x11 egl
LIBS=-lm -lglr -lGL
CFLAGS=-ggdb -O0 -L.. -Wall -DFONTS_DIR=\"`pwd`/fonts/\"
SOURCES=utils.c

all: Makefile examples

examples: \
	rects-threaded \
	rects-and-text \
	layers \
	borders

rects-and-text: rects-and-text.c ${SOURCES}
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		${SOURCES} \
		rects-and-text.c \
		-o rects-and-text

rects-threaded: rects-threaded.c ${SOURCES}
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		${SOURCES} \
		rects-threaded.c \
		-o rects-threaded

layers: layers.c ${SOURCES}
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		${SOURCES} \
		layers.c \
		-o layers

borders: borders.c ${SOURCES}
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		${SOURCES} \
		borders.c \
		-o borders
