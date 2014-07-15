PKG_CONFIG_LIBS=glib-2.0 freetype2 egl
LIBS=-lm -lGL
CFLAGS=-ggdb -O0 -Wall -shared -fPIC -DCURRENT_DIR=\"`pwd`\"

all: libglr.so
	make -C examples

libglr.so: Makefile \
	glr-context.c glr-context.h \
	glr-target.c glr-target.h \
	glr-canvas.c glr-canvas.h \
	glr-batch.c glr-batch.h \
	glr-symbols.c glr-symbols.h \
	glr-paint.c glr-paint.h \
	glr-layer.c glr-layer.h \
	glr-tex-cache.c glr-tex-cache.h \
	glr.h glr-priv.h \
	glr-style.c glr-style.h
	gcc ${CFLAGS} \
		`pkg-config --libs --cflags ${PKG_CONFIG_LIBS}` \
		${LIBS} \
		glr-context.c \
		glr-target.c \
		glr-canvas.c \
		glr-batch.c \
		glr-symbols.c \
		glr-paint.c \
		glr-layer.c \
		glr-tex-cache.c \
		glr-style.c \
		-o libglr.so
