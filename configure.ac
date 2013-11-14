#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT(example-apps, 0.1, https://git.ti.com/glsdk/example-applications)
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

#Initialize automake
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

#Check the version of pkg-config
PKG_PROG_PKG_CONFIG()

# Checks for programs.
AC_PROG_CC

#Test the C compiler
AC_LANG_C

# Checks for libraries.

#Checks the presence of packages
PKG_CHECK_MODULES(DRM, libdrm libdrm_omap)

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

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile vpe/Makefile drm-tests/Makefile thermal/Makefile messageq/messageq_ipc_linux/Makefile])
AC_OUTPUT
