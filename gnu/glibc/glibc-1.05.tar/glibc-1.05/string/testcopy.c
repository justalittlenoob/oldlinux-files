/* Copyright (C) 1990, 1991, 1992 Free Software Foundation, Inc.
   Contributed by Torbjorn Granlund (tege@sics.se).

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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

int
DEFUN(main, (argc, argv),
      int argc AND char **argv)
{
  char *mem, *memp;
  char *rand_mem;
  char *lo_around, *hi_around;
  int size, max_size;
  int src_off, dst_off;
  int i;
  int space_around = 10;

  max_size = 256;

  mem = malloc (max_size + 2 * max_size + 2 * space_around);
  rand_mem = malloc (max_size);
  lo_around = malloc (space_around);
  hi_around = malloc (space_around);
  memp = mem + space_around;

  /* Fill RAND_MEM with random bytes, each non-zero.  */
  for (i = 0; i < max_size; i++)
    {
      int x;
      do
	x = random ();
      while (x == 0);
      rand_mem[i] = x;
    }

  for (size = 0; size < max_size; size++)
    {
      printf("phase %d\n", size);
      for (src_off = 0; src_off <= 16; src_off++)
	{
	  for (dst_off = 0; dst_off <= 16; dst_off++)
	    {
	      /* Put zero around the intended destination, to check
		 that it's not clobbered.  */
	      for (i = 1; i < space_around; i++)
		{
		  memp[dst_off - i] = 0;
		  memp[dst_off + size - 1 + i] = 0;
		}

	      /* Fill the source area with known contents.  */
	      for (i = 0; i < size; i++)
		memp[src_off + i] = rand_mem[i];

	      /* Remember the contents around the destination area.
		 (It might not be what we wrote some lines above, since
		 the src area and the dst area overlap.)  */
	      for (i = 1; i < space_around; i++)
		{
		  lo_around[i] = memp[dst_off - i];
		  hi_around[i] = memp[dst_off + size - 1 + i];
		}

	      memmove (memp + dst_off, memp + src_off, size);

	      /* Check that the destination area has the same
		 contents we wrote to the source area.  */
	      for (i = 0; i < size; i++)
		{
		  if (memp[dst_off + i] != rand_mem[i])
		    abort ();
		}

	      /* Check that the area around the destination is not
		 clobbered.  */
	      for (i = 1; i < space_around; i++)
		{
		  if (memp[dst_off - i] != lo_around[i])
		    abort ();
		  if (memp[dst_off + size - 1 + i] != hi_around[i])
		    abort ();
		}
	    }
	}
    }

  puts ("Test succeeded.");

  return 0;
}
