# SYNOPSIS
#
#   AX_CHECK_DEFINE(includefile, define, [ACTION-SUCCESS], [ACTION-FAILURE])
#
# DESCRIPTION
#
#    Check whether the given #define is available in the given #include file
#
# LICENSE
#
#    See the root directory of the libtpms project for the LICENSE
#
AC_DEFUN([AX_CHECK_DEFINE],
 [AC_PREREQ(2.63)
  AC_MSG_CHECKING(whether $2 is defined in $1)
  AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM([[#include $1]],
                    [[#ifndef $2
                    #error $2 not defined
                    #endif]])],
   [
     AC_MSG_RESULT([yes])
     [$3]
   ],
   [
     AC_MSG_RESULT([no])
     [$4]
   ]
  )
 ]
)
