AC_INIT([cdirector], [1.0])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([cdirector.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([ncursesw], [initscr])

# Checks for header files.
AC_CHECK_HEADERS([locale.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_CHECK_FUNCS([getcwd memset mkdir setlocale])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
