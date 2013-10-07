AM_CFLAGS = $(filter-out -Wpointer-arith, $(WARN_CFLAGS))

AM_CFLAGS += \
	@KMS_CFLAGS@ @DRM_CFLAGS@ 


bin_PROGRAMS = \
	drmclone \
	drmextended \
	drmzalpha

drmclone_SOURCES = \
	buffers.c drm_clone.c buffers.h

drmclone_LDADD = \
	@KMS_LIBS@ @DRM_LIBS@

drmextended_SOURCES = \
	buffers.c drm_extended.c buffers.h

drmextended_LDADD = \
	@KMS_LIBS@ @DRM_LIBS@

drmzalpha_SOURCES = \
	buffers.c drm_z_alpha.c buffers.h

drmzalpha_LDADD = \
	@KMS_LIBS@ @DRM_LIBS@

if ENABLE_CAIRO
AM_CFLAGS += $(CAIRO_CFLAGS)
drmclone_LDADD += $(CAIRO_LIBS)
endif
