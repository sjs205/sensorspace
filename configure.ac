# Process this file with autoconf to produce a configure script.
AC_INIT([sensorspace], [0.0.0.1], swannonline@googlmail.com)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(src/sensorspace.h)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([subdir-objects])

AM_INIT_AUTOMAKE

# Check for programs
AC_PROG_RANLIB
AM_PROG_CC_C_O

# Debug enabled?
AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--enable-debug], [enable debugging, default: no]),
  [case "${enableval}" in  yes) debug=true ;;  no)  debug=false ;; *)
  AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;  esac],
  [debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

# Checks for header files.
AC_CHECK_HEADERS([signal.h fcntl.h stdint.h stdlib.h])
AC_CHECK_HEADERS([string.h sys/ioctl.h sys/socket.h sys/types.h unistd.h])
AC_CHECK_HEADERS([sys/stat.h termio.h sys/time.h])
AC_CHECK_HEADERS([assert.h errno.h ])

AC_OUTPUT(Makefile src/Makefile src/reading/Makefile src/serial/Makefile src/controller/Makefile)
