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

#include <ansidecl.h>
#include <stddef.h>
#include <signal.h>

/* If INTERRUPT is nonzero, make signal SIG interrupt system calls
   (causing them to fail with EINTR); if INTERRUPT is zero, make system
   calls be restarted after signal SIG.  */
int
DEFUN(siginterrupt, (sig, interrupt),
      int sig AND int interrupt)
{
  struct sigvec vec;

  if (sigvec(sig, (struct sigvec *) NULL, &vec) < 0)
    return -1;

  if (interrupt)
    vec.sv_flags |= SV_INTERRUPT;
  else
    vec.sv_flags &= ~SV_INTERRUPT;

  if (sigvec(sig, &vec, (struct sigvec *) NULL) < 0)
    return -1;

  return 0;
}
