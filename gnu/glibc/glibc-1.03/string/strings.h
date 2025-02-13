/* Copyright (C) 1991 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

/* Compatibility with BSD string(3).  */

#ifndef	_STRINGS_H

#define	_STRINGS_H	1
#include <features.h>

#ifndef	_STRING_H
#include <string.h>
#endif	/* string.h  */

/* These do not depend on __USE_BSD because
   <strings.h> is a BSD-only header.  */

/* Find the first occurrence of C in S.  */
extern char *EXFUN(index, (CONST char *__s, int __c));
/* Find the last occurrence of C in S.  */
extern char *EXFUN(rindex, (CONST char *__s, int __c));


#endif	/* strings.h  */
