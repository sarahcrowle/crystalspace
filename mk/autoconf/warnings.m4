# warnings.m4                                                  -*- Autoconf -*-
#==============================================================================
# Copyright (C)2005-2008 by Eric Sunshine <sunshine@sunshineco.com>
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
# CS_COMPILER_WARNINGS([LANGUAGE], [CACHE-VAR], [ACTION-IF-FOUND],
#                      [ACTION-IF-NOT-FOUND])
#	Check how to enable compilation warnings. If LANGUAGE is not provided,
#	then `C' is assumed (other options include `C++').  If CACHE-VAR is not
#	provided, then it defaults to the name
#	"cs_cv_prog_{language}_enable_warnings" where {language} is the result
#	of CS_TR_SH_lang().  If an option for enabling warnings (such as
#	`-Wall') is discovered, then it is assigned to CACHE-VAR and
#	ACTION-IF-FOUND is invoked; otherwise the empty string is assigned to
#	CACHE-VAR and ACTION-IF-NOT-FOUND is invoked.
#
# IMPLEMENTATION NOTES
#
#	On some platforms, it is more appropriate to use -Wmost rather than
#	-Wall even if the compiler understands both, thus we attempt -Wmost
#	before -Wall.
#------------------------------------------------------------------------------
AC_DEFUN([CS_COMPILER_WARNINGS],
    [CS_CHECK_BUILD_FLAGS(
	[how to enable ]CS_LANG_RESOLVE([$1])[ compilation warnings],
	[_CS_WARNING_CACHE_VAR([$2], [$1], [enable_warnings])],
	[CS_CREATE_TUPLE([-Wmost]) CS_CREATE_TUPLE([-Wall])],
	[$1], [$3], [$4])])



#------------------------------------------------------------------------------
# CS_COMPILER_ERRORS([LANGUAGE], [CACHE-VAR], [ACTION-IF-FOUND],
#                    [ACTION-IF-NOT-FOUND])
#	Check how to promote compilation diganostics from warning to error
#	status. If LANGUAGE is not provided, then `C' is assumed (other options
#	include `C++').  If CACHE-VAR is not provided, then it defaults to the
#	name "cs_cv_prog_{language}_enable_errors" where {language} is the
#	result of CS_TR_SH_lang().  If an option for performing this promotion
#	(such as `-Werror') is discovered, then it is assigned to CACHE-VAR and
#	ACTION-IF-FOUND is invoked; otherwise the empty string is assigned to
#	CACHE-VAR and ACTION-IF-NOT-FOUND is invoked.
#------------------------------------------------------------------------------
AC_DEFUN([CS_COMPILER_ERRORS],
    [CS_CHECK_BUILD_FLAGS(
	[how to treat ]CS_LANG_RESOLVE([$1])[ warnings as errors],
	[_CS_WARNING_CACHE_VAR([$2], [$1], [enable_errors])],
	[CS_CREATE_TUPLE([-Werror])], [$1], [$3], [$4])])



#------------------------------------------------------------------------------
# CS_COMPILER_IGNORE_UNUSED([LANGUAGE], [CACHE-VAR], [ACTION-IF-FOUND],
#                           [ACTION-IF-NOT-FOUND])
#	Check how to instruct compiler to ignore unused variables and
#	arguments.  This option may be useful for code generated by tools, such
#	as Swig, Bison, and Flex, over which the client has no control, yet
#	wishes to compile without excessive diagnostic spew.  If LANGUAGE is
#	not provided, then `C' is assumed (other options include `C++').  If
#	CACHE-VAR is not provided, then it defaults to the name
#	"cs_cv_prog_{language}_ignore_unused" where {language} is the result of
#	CS_TR_SH_lang().  If an option (such as `-Wno-unused') is discovered,
#	then it is assigned to CACHE-VAR and ACTION-IF-FOUND is invoked;
#	otherwise the empty string is assigned to CACHE-VAR and
#	ACTION-IF-NOT-FOUND is invoked.
#------------------------------------------------------------------------------
AC_DEFUN([CS_COMPILER_IGNORE_UNUSED],
    [CS_CHECK_BUILD_FLAGS(
	[_CS_WARNING_SUPPRESS_MSG([unused variable], [$1])],
	[_CS_WARNING_CACHE_VAR([$2], [$1], [ignore_unused])],
	[CS_CREATE_TUPLE([-Wno-unused])], [$1], [$3], [$4])])



#------------------------------------------------------------------------------
# CS_COMPILER_IGNORE_UNINITIALIZED([LANGUAGE], [CACHE-VAR], [ACTION-IF-FOUND],
#                                  [ACTION-IF-NOT-FOUND])
#	Check how to instruct compiler to ignore uninitialized variables.  This
#	option may be useful for code generated by tools, such as Swig, Bison,
#	and Flex, over which the client has no control, yet wishes to compile
#	without excessive diagnostic spew.  If LANGUAGE is not provided, then
#	`C' is assumed (other options include `C++').  If CACHE-VAR is not
#	provided, then it defaults to the name
#	"cs_cv_prog_{language}_ignore_uninitialized" where {language} is the
#	result of CS_TR_SH_lang().  If an option (such as `-Wno-uninitialized')
#	is discovered, then it is assigned to CACHE-VAR and ACTION-IF-FOUND is
#	invoked; otherwise the empty string is assigned to CACHE-VAR and
#	ACTION-IF-NOT-FOUND is invoked.
#------------------------------------------------------------------------------
AC_DEFUN([CS_COMPILER_IGNORE_UNINITIALIZED],
    [CS_CHECK_BUILD_FLAGS(
	[_CS_WARNING_SUPPRESS_MSG([uninitialized variable], [$1])],
	[_CS_WARNING_CACHE_VAR([$2], [$1], [ignore_uninitialized])],
	[CS_CREATE_TUPLE([-Wno-uninitialized])], [$1], [$3], [$4])])



#------------------------------------------------------------------------------
# CS_COMPILER_IGNORE_PRAGMAS([LANGUAGE], [CACHE-VAR], [ACTION-IF-FOUND],
#                            [ACTION-IF-NOT-FOUND])
#	Check how to instruct compiler to ignore unrecognized #pragma
#	directives.  This option may be useful for code which contains
#	unprotected #pragmas which are not understood by all compilers.  If
#	LANGUAGE is not provided, then `C' is assumed (other options include
#	`C++').  If CACHE-VAR is not provided, then it defaults to the name
#	"cs_cv_prog_{language}_ignore_unknown_pragmas" where {language} is the
#	result of CS_TR_SH_lang().  If an option (such as
#	`-Wno-unknown-pragmas') is discovered, then it is assigned to CACHE-VAR
#	and ACTION-IF-FOUND is invoked; otherwise the empty string is assigned
#	to CACHE-VAR and ACTION-IF-NOT-FOUND is invoked.
#------------------------------------------------------------------------------
AC_DEFUN([CS_COMPILER_IGNORE_PRAGMAS],
    [CS_CHECK_BUILD_FLAGS(
	[_CS_WARNING_SUPPRESS_MSG([unknown [#pragma]], [$1])],
	[_CS_WARNING_CACHE_VAR([$2], [$1], [ignore_unknown_pragmas])],
	[CS_CREATE_TUPLE([-Wno-unknown-pragmas])], [$1], [$3], [$4])])



#------------------------------------------------------------------------------
# CS_COMPILER_IGNORE_LONG_DOUBLE([LANGUAGE], [CACHE-VAR], [ACTION-IF-FOUND],
#                                [ACTION-IF-NOT-FOUND])
#	Check how to instruct compiler to suppress warnings about `long double'
#	usage.  This option may be useful for code generated by tools, such as
#	Swig, Bison, and Flex, over which the client has no control, yet wishes
#	to compile without excessive diagnostic spew.  If LANGUAGE is not
#	provided, then `C' is assumed (other options include `C++').  If
#	CACHE-VAR is not provided, then it defaults to the name
#	"cs_cv_prog_{language}_ignore_long_double" where {language} is the
#	result of CS_TR_SH_lang().  If an option (such as `-Wno-long-double')
#	is discovered, then it is assigned to CACHE-VAR and ACTION-IF-FOUND is
#	invoked; otherwise the empty string is assigned to CACHE-VAR and
#	ACTION-IF-NOT-FOUND is invoked.
#
# IMPLEMENTATION NOTES
#	The -Wno-long-double option is specific to the Apple version of g++.
#	Furthermore, as of at least 2008-04-03, the
#	CS_CHECK_BUILD_FLAGS([-Wno-long-double]) check spuriously and
#	incorrectly *succeeds* with g++ 4.2.3 on non-Apple platforms even
#	though the compiler does not really accept the option. This is a g++
#	bug.  To work around this issue, the -Wno-long-double check is
#	performed only on Apple platforms.
#------------------------------------------------------------------------------
AC_DEFUN([CS_COMPILER_IGNORE_LONG_DOUBLE],
    [AC_REQUIRE([CS_CHECK_HOST])
    AS_IF([test "$cs_host_target" = macosx],
	[cs_ignore_long_double_tuples=CS_CREATE_TUPLE([-Wno-long-double])],
	[cs_ignore_long_double_tuples=''])
    CS_CHECK_BUILD_FLAGS(
	[_CS_WARNING_SUPPRESS_MSG([`long double'], [$1])],
	[_CS_WARNING_CACHE_VAR([$2], [$1], [ignore_long_double])],
	[$cs_ignore_long_double_tuples], [$1], [$3], [$4])])



#------------------------------------------------------------------------------
# CS_COMPILER_IGNORE_NON_VIRTUAL_DTOR([LANGUAGE], [CACHE-VAR],
#                                     [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#	Check how to instruct compiler to ignore non-virtual destructors in
#	classes containing virtual methods.  This option may be useful to
#	suppress warnings about code over which we have no control, such as
#	3rd-party headers or code generated by tools.  If LANGUAGE is not
#	provided, then `C' is assumed (other options include `C++').  If
#	CACHE-VAR is not provided, then it defaults to the name
#	"cs_cv_prog_{language}_ignore_non_virtual_dtor" where {language} is the
#	result of CS_TR_SH_lang().  If an option (such as
#	`-Wno-non-virtual-dtor') is discovered, then it is assigned to
#	CACHE-VAR and ACTION-IF-FOUND is invoked; otherwise the empty string is
#	assigned to CACHE-VAR and ACTION-IF-NOT-FOUND is invoked.
#------------------------------------------------------------------------------
AC_DEFUN([CS_COMPILER_IGNORE_NON_VIRTUAL_DTOR],
    [CS_CHECK_BUILD_FLAGS(
	[_CS_WARNING_SUPPRESS_MSG([non-virtual destructor], [$1])],
	[_CS_WARNING_CACHE_VAR([$2], [$1], [ignore_non_virtual_dtor])],
	[CS_CREATE_TUPLE([-Wno-non-virtual-dtor])], [$1], [$3], [$4])])



#------------------------------------------------------------------------------
# CS_COMPILER_IGNORE_CONST_STRINGS([LANGUAGE], [CACHE-VAR],
#                                  [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#	Check how to instruct compiler to ignore assignments of constant string
#	literals to char* variables or char* function arguments.  This option
#	may be useful to suppress warnings about code over which we have no
#	control, such as 3rd-party headers or code generated by tools.  If
#	LANGUAGE is not provided, then `C' is assumed (other options include
#	`C++').  If CACHE-VAR is not provided, then it defaults to the name
#	"cs_cv_prog_{language}_ignore_const_strings" where {language} is the
#	result of CS_TR_SH_lang().  If an option (such as `-Wno-write-strings')
#	is discovered, then it is assigned to CACHE-VAR and ACTION-IF-FOUND is
#	invoked; otherwise the empty string is assigned to CACHE-VAR and
#	ACTION-IF-NOT-FOUND is invoked.
#------------------------------------------------------------------------------
AC_DEFUN([CS_COMPILER_IGNORE_CONST_STRINGS],
    [CS_CHECK_BUILD_FLAGS(
	[_CS_WARNING_SUPPRESS_MSG([string constant to char*], [$1])],
	[_CS_WARNING_CACHE_VAR([$2], [$1], [ignore_const_strings])],
	[CS_CREATE_TUPLE([-Wno-write-strings])], [$1], [$3], [$4])])



#------------------------------------------------------------------------------
# _CS_WARNING_SUPPRESS_MSG(DESCRIPTION, [LANGUAGE])
#	Helper macro which composes the message for warning suppression
#	checks. DESCRIPTION is a human-readable description of the particular
#	check.  LANGUAGE is the language being checked, typically `C' or `C++'
#	(defaulting to `C' if not provided).
#------------------------------------------------------------------------------
AC_DEFUN([_CS_WARNING_SUPPRESS_MSG],
    [how to suppress CS_LANG_RESOLVE([$2]) $1 warnings])



#------------------------------------------------------------------------------
# _CS_WARNING_CACHE_VAR([CACHE-VAR], [LANGUAGE], DEFAULT-CACHE-VAR-SUFFIX)
#	Helper macro which returns the client-supplied cache variable name or
#	composes one from LANGUAGE and DEFAULT-CACHE-VAR-SUFFIX if not
#	provided.  LANGUAGE typically is `C' or `C++' (defaulting to `C' if not
#	provided).
#------------------------------------------------------------------------------
AC_DEFUN([_CS_WARNING_CACHE_VAR],
    [m4_default([$1],
        [CS_TR_SH_lang([cs_cv_prog_]CS_LANG_RESOLVE([$2])[_$3])])])