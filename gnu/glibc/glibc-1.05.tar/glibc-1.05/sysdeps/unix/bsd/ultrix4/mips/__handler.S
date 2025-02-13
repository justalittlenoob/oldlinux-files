/* Copyright (C) 1992 Free Software Foundation, Inc.
   Contributed by Brendan Kehoe (brendan@cs.widener.edu).
   Also hacked by Ian Lance Taylor (ian@airs.com).

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

#include <sysdep.h>

/* This function saves all the registers, calls the
   user function, and then executes a sigreturn system call.  The
   sigreturn call wants the address of a sigcontext structure.  This
   is all hideously system dependent and, for all intents and
   purposes, undocumented.

   When we enter here, a3 holds the user's signal handler.  We are
   supposed to fill in the context given in a2, and then pass it and
   the first two arguments to the user's function.  If the user's
   function returns, we execute a sigreturn system call.

   The sc_onstack, sc_mask and sc_pc elements of the context are
   already set by the kernel.  For some reason we don't have to save
   the floating point state or the coprocessor state; the kernel may
   have saved them for us, or it doesn't use them.  */

.set noat
ENTRY (__handler)
	/* Store zero and the asm temp reg.  */
	sw $0, 12(a2)
	sw AT, 16(a2)

	/* Put v1 in sc_regs[3].  */
	sw v1, 24(a2)

	/* Save the caller saved registers in sc_regs[8..15].  */
	sw t0, 44(a2)
	sw t1, 48(a2)
	sw t2, 52(a2)
	sw t3, 56(a2)
	sw t4, 60(a2)
	sw t5, 64(a2)
	sw t6, 48(a2)
	sw t7, 52(a2)

	/* Save the callee saved registers in sc_regs[16..23].  */
	sw s0, 76(a2)
	sw s1, 80(a2)
	sw s2, 84(a2)
	sw s3, 88(a2)
	sw s4, 92(a2)
	sw s5, 96(a2)
	sw s6, 100(a2)
	sw s7, 104(a2)

	/* Save the code generator registers in sc_regs[24] & sc_regs[25].  */
	sw t8, 108(a2)
	sw t9, 112(a2)

	/* Save the kernel temp regs in sc_regs[26] & sc_regs[27]. */
	sw k0, 116(a2)
	sw k1, 120(a2)

	/* Save the global pointer in sc_regs[28].  */
	sw gp, 124(a2)

	/* ... and also the return address in sc_regs[31].  */
	sw ra, 136(a2)

	/* Save the floating pointer and the stack pointer in
	   sc_regs[29] and sc_regs[30].  */
	sw sp, 128(a2)
	sw $fp, 132(a2)

	/* Save the mul/div stuff in sc_mdlo and sc_mdhi.  */
	mflo t0
	sw t0, 140(a2)
	mfhi t0
	sw t0, 144(a2)

	/* Move the stack up four.  This will save the context.  */
	addu sp, sp, -32
	sw a2, 16(sp)

	/* Call their handler with the signal, code, and context; note
	   this will clobber the context.  */
	.set noreorder
	jal ra, a3
	nop
	.set reorder

	/* When we come back, restore the context and pass it right
	   on into sigreturn().  */
	lw a0, 16(sp)

	/* Do a sigreturn syscall; this doesn't return.  */
	la v0, __sigreturn
	jal ra, v0
