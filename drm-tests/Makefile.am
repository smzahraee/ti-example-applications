AM_CFLAGS = $(filter-out -Wpointer-arith, $(WARN_CFLAGS))

AM_CFLAGS += \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/libkms/ \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	drm_clone \
	drm_extended \
	drm_z_alpha
else
noinst_PROGRAMS = \
	drm_clone \
	drm_extended \
	drm_z_alpha
endif

drm_clone_SOURCES = \
	buffers.c drm_clone.c buffers.h

drm_extended_SOURCES = \
	buffers.c drm_extended.c buffers.h

drm_z_alpha_SOURCES = \
	buffers.c drm_z_alpha.c buffers.h

drm_clone_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/libkms/libkms.la

drm_extended_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/libkms/libkms.la

drm_z_alpha_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/libkms/libkms.la

if HAVE_CAIRO
AM_CFLAGS += $(CAIRO_CFLAGS)
drm_clone_LDADD += $(CAIRO_LIBS)
drm_extended_LDADD += $(CAIRO_LIBS)
drm_z_alpha_LDADD += $(CAIRO_LIBS)
endif
