/* Definitions of target machine for GNU compiler.
   Motorola m88100 running Omron Luna/88k.
   Copyright (C) 1991 Free Software Foundation, Inc.
   Contributed by Jeffery Friedl (jfriedl@omron.co.jp)
   Currently supported by Tom Wood (wood@dg-rtp.dg.com)

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* The Omron Luna/88k is MACH and uses BSD a.out, not COFF or ELF.  */
#ifndef MACH
#define MACH
#endif
#define DBX_DEBUGGING_INFO
#define DEFAULT_GDB_EXTENSIONS 0

#include "aoutos.h"
#include "m88k.h"

/* Identify the compiler.  */
#undef  VERSION_INFO1
#define VERSION_INFO1 "Omron Luna/88k, "

/* Macros to be automatically defined.  */
#undef	CPP_PREDEFINES
#define CPP_PREDEFINES \
    "-DMACH -Dmc88100 -Dm88k -Dunix -Dluna -Dluna88k -D__CLASSIFY_TYPE__=2"

/* Specify extra dir to search for include files.  */
#undef	SYSTEM_INCLUDE_DIR
#define SYSTEM_INCLUDE_DIR "/usr/mach/include"

/* For the Omron Luna/88k, a float function returns a double in traditional
   mode (and a float in ansi mode).  */
#undef TRADITIONAL_RETURN_FLOAT
