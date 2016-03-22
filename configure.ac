#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT(example-apps, 1.0, https://git.ti.com/glsdk/example-applications)
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

dnl required versions of gstreamer and plugins-base
GST_REQUIRED=1.2.3

#Initialize automake
AM_INIT_AUTOMAKE([foreign dist-bzip2 silent-rules subdir-objects -Wall])
AM_MAINTAINER_MODE([enable])

# Enable quiet compiles on automake 1.11.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([no])])

# Initialize libtool
LT_INIT
#Check the version of pkg-config
PKG_PROG_PKG_CONFIG()

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O

#Test the C compiler
AC_LANG_C

# Checks for libraries.

#Checks the presence of packages
PKG_CHECK_MODULES(DRM, libdrm libdrm_omap)
PKG_CHECK_MODULES(WAYLAND, wayland-egl wayland-cursor wayland-client)
PKG_CHECK_MODULES(UDEV, libudev)

PKG_CHECK_MODULES(KMS, libkms)

PKG_CHECK_MODULES(CAIRO, libcairo, [HAVE_CAIRO=yes], [HAVE_CAIRO=no])
if test "x$HAVE_CAIRO" = "xyes"; then
        AC_DEFINE(HAVE_CAIRO, 1, [Have CAIRO support])
        AC_MSG_NOTICE([Detected libcairo])
else
        AC_MSG_WARN([No libcairo support detected])
fi
AM_CONDITIONAL(ENABLE_CAIRO, [test "x$HAVE_CAIRO" = xyes])

PKG_CHECK_MODULES(GTHREAD, gthread-2.0)

dnl Check for the required version of GStreamer core (and gst-plugins-base)
dnl This will export GST_CFLAGS and GST_LIBS variables for use in Makefile.am
AC_ARG_ENABLE(gst, AC_HELP_STRING([--disable-gst], [GStreamer tests are
enabled by default. To disable gstreamer tests --disable-gst
default:yes)]), [GST_ENABLE=$enableval], [GST_ENABLE=yes]) AM_CONDITIONAL(HAVE_GST, [test "x$GST_ENABLE" = xyes]) if test "x$GST_ENABLE" = xyes; then
        PKG_CHECK_MODULES(GST, [
          gstreamer-1.0 >= $GST_REQUIRED
        ], [
          AC_SUBST(GST_CFLAGS)
          AC_SUBST(GST_LIBS)
        ], [
          AC_MSG_ERROR([
              You need to install or upgrade the GStreamer development
              packages on your system. On debian-based systems these are
              libgstreamer1.0-dev and libgstreamer-plugins-base1.0-dev.
              on RPM-based systems gstreamer1.0-devel, libgstreamer1.0-devel
              or similar. The minimum version required is $GST_REQUIRED.
          ])
        ])
fi

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile vpe/Makefile vip/Makefile drm-tests/Makefile thermal/Makefile messageq/messageq_ipc_linux/Makefile gst/Makefile bandwidth-tool/Makefile cpuload-plugins/Makefile])
AC_OUTPUT
