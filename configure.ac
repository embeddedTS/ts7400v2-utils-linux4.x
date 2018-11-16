#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(ts7400v2-utils-linux4.x, 1.0, support@embeddedarm.com)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/i2c-dev.h])

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/ioctl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([bzero getpagesize memset munmap select strstr strtoul strtoull])

AC_CONFIG_FILES([
	Makefile 
	src/Makefile 
])
AC_OUTPUT
