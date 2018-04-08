# sharedlibs.m4                                                -*- Autoconf -*-
#==============================================================================
# Copyright (C)2003-2012 by Eric Sunshine <sunshine@sunshineco.com>
#
#    This library is free software; you can redistribute it and/or modify it
#    under the terms of the GNU Library General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or (at your
#    option) any later version.
#
#    This library is distributed in the hope that it will be useful, but
#    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
#    License for more details.
#
#    You should have received a copy of the GNU Library General Public License
#    along with this library; if not, write to the Free Software Foundation,
#    Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#==============================================================================
AC_PREREQ([2.56])

#------------------------------------------------------------------------------
# CS_CHECK_BUILD_LIBS_SHARED([HEADER-PROPERTY-SHARED],
#                            [HEADER-PROPERTY-STATIC],
#                            [EMITTER], [EMITTER-OPTIONS])
#
# Decides, based on platform and `configure' command line arguments, whether
# libraries should be built shared or static.
# The BUILD_SHARED_LIBS Jam variables is set to `yes' if shared libraries are
# enabled, `no` otherwise (i.e. libraries are static).
# Furthermore, the configuration header will define a preprocessor macro with
# the name given in HEADER-PROPERTY-SHARED if shared libraries are enabled;
# if shared libraries are disabled, a preprocessor macro with the name given
# in HEADER-PROPERTY-STATIC will be defined.
# HEADER-PROPERTY-STATIC is optional.
# How the requested preprocessor macros are defined is controlled by the
# EMITTER and EMITTER-OPTIONS parameters. By default, the CS_HEADER_PROPERTY
# facility is used.
#------------------------------------------------------------------------------
AC_DEFUN([CS_CHECK_BUILD_LIBS_SHARED],
    [_CS_CHECK_BUILD_LIBS_SHARED([$1], [$2],
        [m4_default([$3], [CS_HEADER_PROPERTY])], [$4])])
AC_DEFUN([_CS_CHECK_BUILD_LIBS_SHARED],
    [AC_MSG_CHECKING([whether to build shared libraries])
    case $host_os in
        cygwin*|linux*) enable_shared_default=yes ;;
        *) enable_shared_default=no ;;
    esac
    AC_ARG_ENABLE([shared], [AC_HELP_STRING([--enable-shared],
        [enable creation of shared libraries (default YES on Win32 and Linux, 
        NO otherwise)])],
        [], [enable_shared=$enable_shared_default])
    AC_MSG_RESULT([$enable_shared])
    CS_EMIT_BUILD_PROPERTY([BUILD_SHARED_LIBS], [$enable_shared])
    AS_IF([test $enable_shared = yes], 
      [$3([$1], [$4])],
      m4_ifval([$2], [$3([$2], [$4])], []))])
