bin_PROGRAMS = pvrscope mpuload read32k_driver readproc waitforweston rprocinfo

mpuload_CFLAGS = \
	-O0 -g --static 

mpuload_SOURCES = mpuload.c

rprocinfo_CFLAGS = -O0 -g @DRM_CFLAGS@ @DCE_CFLAGS@
rprocinfo_LDADD = @DCE_LIBS@ -lpthread
rprocinfo_SOURCES = rprocinfo.c

pvrscope_CXXFLAGS = -I.
pvrscope_LDADD = $(top_srcdir)/cpuload-plugins/libPVRScopeDeveloper.a -lrt -lm -lpthread -ldl
pvrscope_SOURCES = pvrscope.cpp

# Specify any libraries that need to be built here.  Follow the convention
# libxxx.a otherwise autotools will complain about non-canonical names.
lib_LIBRARIES = libread32k.a

# Initialize to empty
include_HEADERS=

##
# Applications
##

read32k_driver_SOURCES= read32k_driver.c
read32k_driver_LDADD = libread32k.a -lm
read32k_driver_CFLAGS = -I.

readproc_SOURCES = readproc.c
readproc_LDADD = -lm

waitforweston_SOURCES = waitforweston.c
waitforweston_LDADD = libread32k.a -lm @DRM_LIBS@ -lpthread -lrt @WAYLAND_LIBS@ @UDEV_LIBS@
waitforweston_CFLAGS = -I. @DRM_CFLAGS@

##
# Libraries
##

libread32k_a_SOURCES = read32k.c
libread32k_a_CFLAGS = -I.
include_HEADERS+=./read32k.h
