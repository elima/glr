CURRENT_DIR = $(shell pwd)
BUILD_DIR = $(CURRENT_DIR)/../build/$(shell uname -m)

PKG_CONFIG_LIBS = glib-2.0 freetype2 harfbuzz
LIBS = -lm -lglr -lGLESv2 -lEGL
CFLAGS = -ggdb -O0 -L$(BUILD_DIR) -Wall -std=c99 -DFONTS_DIR=\"`pwd`/fonts/\"
SOURCES = text-pen.c text-node.c
HEADERS = utils.h text-pen.h text-node.h

ifeq ($(GLR_BACKEND), fbdev)
  LIBS += -lmali -L/usr/local/lib/mali/fbdev
  SOURCES += utils-fbdev.c
else
  PKG_CONFIG_LIBS += x11
  SOURCES += utils-x11.c
endif

all: Makefile examples

examples: \
	$(BUILD_DIR)/rects-and-text \
	$(BUILD_DIR)/rects-threaded \
	$(BUILD_DIR)/layers \
	$(BUILD_DIR)/borders \
	$(BUILD_DIR)/text-layout \
	$(BUILD_DIR)/background-simple

$(BUILD_DIR)/rects-and-text: rects-and-text.c ${SOURCES} $(HEADERS)
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		${SOURCES} \
		rects-and-text.c \
		-o $@

$(BUILD_DIR)/rects-threaded: rects-threaded.c ${SOURCES} $(HEADERS)
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		${SOURCES} \
		rects-threaded.c \
		-o $@

$(BUILD_DIR)/layers: layers.c ${SOURCES}
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		${SOURCES} \
		layers.c \
		-o $@

$(BUILD_DIR)/borders: borders.c ${SOURCES} $(HEADERS)
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		${SOURCES} \
		borders.c \
		-o $@

$(BUILD_DIR)/text-layout: text-layout.c ${SOURCES} $(HEADERS)
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		${SOURCES} \
		text-layout.c \
		-o $@

$(BUILD_DIR)/background-simple: background-simple.c ${SOURCES} $(HEADERS)
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		${SOURCES} \
		background-simple.c \
		-o $@
