AM_CFLAGS = $(filter-out -Wpointer-arith, $(WARN_CFLAGS))

bin_PROGRAMS = \
	simple-egl-readpixels

simple_egl_readpixels_SOURCES = \
	simple-egl-readpixels.c

simple_egl_readpixels_LDADD = \
	@EGL_LIBS@ @GLES2_LIBS@ @WAYLAND_LIBS@ -lm
