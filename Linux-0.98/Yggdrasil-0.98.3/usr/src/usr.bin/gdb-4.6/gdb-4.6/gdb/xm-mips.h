/* Definitions to make GDB run on a mips box under 4.3bsd.
   Copyright (C) 1986, 1987, 1989 Free Software Foundation, Inc.
   Contributed by Per Bothner(bothner@cs.wisc.edu) at U.Wisconsin
   and by Alessandro Forin(af@cs.cmu.edu) at CMU

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#if !defined (HOST_BYTE_ORDER)
#define HOST_BYTE_ORDER LITTLE_ENDIAN
#endif

/* Get rid of any system-imposed stack limit if possible */

#define	SET_STACK_LIMIT_HUGE

#define KERNEL_U_ADDR 0 /* Not needed. */

#ifdef ultrix
extern char *strdup();
#endif

/* DECstation memcpy and memset return void *, not char *.  */

extern void *memcpy();
extern void *memset();
#define	MEM_FNS_DECLARED

/* Only used for core files on DECstations. */

#define REGISTER_U_ADDR(addr, blockend, regno) 		\
	if (regno < 38) addr = (NBPG*UPAGES) + (regno - 38)*sizeof(int);\
	else addr = 0; /* ..somewhere in the pcb */

/* Override copies of {fetch,store}_inferior_registers in infptrace.c.  */
#define FETCH_INFERIOR_REGISTERS

/* Kernel is a bit tenacious about sharing text segments, disallowing bpts.  */
#define	ONE_PROCESS_WRITETEXT
