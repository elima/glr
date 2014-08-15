CURRENT_DIR = $(shell pwd)
BUILD_DIR = $(CURRENT_DIR)/build/$(shell uname -m)

PKG_CONFIG_LIBS = glib-2.0 freetype2
LIBS = -lm -lGLESv2 -lEGL
CFLAGS = -ggdb -O0 -Wall -std=c99 -shared -fPIC \
	-DCURRENT_DIR=\"`pwd`\" -I$(BUILD_DIR)
SOURCES = \
	glr-context.c \
	glr-target.c \
	glr-canvas.c \
	glr-batch.c \
	glr-tex-cache.c \
	glr-style.c

HEADERS = \
	glr.h \
	glr-priv.h \
	glr-context.h \
	glr-target.h \
	glr-canvas.h \
	glr-batch.h \
	glr-tex-cache.h \
	glr-style.h \
	$(BUILD_DIR)/glr-shaders.h

ifeq ($(GLR_BACKEND), fbdev)
  LIBS += -lmali -L/usr/local/lib/mali/fbdev
endif

all: Makefile \
	$(BUILD_DIR)/libglr.so
	make -C examples

$(BUILD_DIR)/glr-shaders.h: vertex-shader-instanced-rects.glsl fragment-shader-instanced-rects.glsl
	mkdir -p $(BUILD_DIR)
	echo "const char *INSTANCED_FRAGMENT_SHADER_SRC =" > $@
	sed -e "s/^\(.*\)$$/\"\1 \\\n\"/" ./fragment-shader-instanced-rects.glsl >> $@
	echo ";\n" >> $@

	echo "const char *INSTANCED_VERTEX_SHADER_SRC =" >> $@
	sed -e "s/^\(.*\)$$/\"\1 \\\n\"/" ./vertex-shader-instanced-rects.glsl >> $@
	echo ";" >> $@

$(BUILD_DIR)/libglr.so: $(SOURCES) $(HEADERS)
	mkdir -p $(BUILD_DIR)
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		$(LIBS) \
		$(SOURCES) \
		-o $@

clean:
	rm -rf ./build
