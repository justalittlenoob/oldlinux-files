/* Copyright (C) 1992 Free Software Foundation, Inc.
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

#include <ansidecl.h>
#include <stddef.h>
#include <setjmp.h>
#include <signal.h>

void
DEFUN(__sigjmp_save, (env, savemask), sigjmp_buf env AND int savemask)
{
  if (savemask)
    env[0].__savemask = sigprocmask (SIG_BLOCK, (sigset_t *) NULL,
				     &env[0].__sigmask) == 0;
  else
    env[0].__savemask = 0;
}
