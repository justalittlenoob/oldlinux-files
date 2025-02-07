/* BFD library support routines for constructors
   Copyright (C) 1990-1991 Free Software Foundation, Inc.

   Hacked by Steve Chamberlain of Cygnus Support. With some help from
   Judy Chamberlain too.


This file is part of BFD, the Binary File Descriptor library.

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

/*
SECTION
	Constructors

	Classes in C++ have 'constructors' and 'destructors'.  These
	are functions which are called automatically by the language
	whenever data of a class is created or destroyed.  Class data
	which is static data may also be have a type which requires
	'construction', the contructor must be called before the data
	can be referenced, so the contructor must be called before the
	program begins. 

	The common solution to this problem is for the compiler to
	call a magic function as the first statement <<main>>.
	This magic function, (often called <<__main>>) runs around
	calling the constructors for all the things needing it.

	With COFF the compile has a bargain with the linker et al.
	All constructors are given strange names, for example
	<<__GLOBAL__$I$foo>> might be the label of a contructor for
	the class @var{foo}.  The solution on unfortunate systems
	(most system V machines) is to perform a partial link on all
	the .o files, do an <<nm>> on the result, run <<awk>> or some
	such over the result looking for strange <<__GLOBAL__$>>
	symbols, generate a C program from this, compile it and link
	with the partially linked input. This process is usually
	called <<collect>>. 

	Some versions of <<a.out>> use something called the
	<<set_vector>> mechanism.  The constructor symbols are output
	from the compiler with a special stab code saying that they
	are constructors, and the linker can deal with them directly. 

	BFD allows applications (ie the linker) to deal with
	constructor information independently of their external
	implimentation by providing a set of entry points for the
	indiviual object back ends to call which maintains a database
	of the contructor information.  The application can
	interrogate the database to find out what it wants.  The
	construction data essential for the linker to be able to
	perform its job are: 

	o asymbol
	The asymbol of the contructor entry point contains all the
	information necessary to call the function. 

	o table id
	The type of symbol, ie is it a contructor, a destructor or
	something else someone dreamed up to make our lives difficult.

	This module takes this information and then builds extra
	sections attached to the bfds which own the entry points.  It
	creates these sections as if they were tables of pointers to
	the entry points, and builds relocation entries to go with
	them so that the tables can be relocated along with the data
	they reference. 

	These sections are marked with a special bit
	(<<SEC_CONSTRUCTOR>>) which the linker notices and do with
	what it wants.

*/

#include <bfd.h>
#include <sysdep.h>
#include <libbfd.h>



/*
INTERNAL_FUNCTION
	bfd_constructor_entry 

SYNOPSIS
	void bfd_constructor_entry(bfd *abfd, 
		asymbol **symbol_ptr_ptr,
		CONST char*type);


DESCRIPTION
	This function is called with an a symbol describing the
	function to be called, an string which descibes the xtor type,
	eg something like "CTOR" or "DTOR" would be fine. And the bfd
	which owns the function. Its duty is to create a section
	called "CTOR" or "DTOR" or whatever if the bfd doesn't already
	have one, and grow a relocation table for the entry points as
	they accumulate.

*/

 
void DEFUN(bfd_constructor_entry,(abfd, symbol_ptr_ptr, type),
	   bfd *abfd AND
	   asymbol **symbol_ptr_ptr AND
	   CONST char *type)

{
    /* Look up the section we're using to store the table in */
    asection *rel_section = bfd_get_section_by_name (abfd, type);
    if (rel_section == (asection *)NULL) {
	rel_section = bfd_make_section (abfd, type);
	rel_section->flags = SEC_CONSTRUCTOR;
	rel_section->alignment_power = 2;
    }

    /* Create a relocation into the section which references the entry
       point */
   {
       arelent_chain *reloc = (arelent_chain *)bfd_alloc(abfd,
							 sizeof(arelent_chain));

/*       reloc->relent.section = (asection *)NULL;*/
       reloc->relent.addend = 0;

       reloc->relent.sym_ptr_ptr = symbol_ptr_ptr;
       reloc->next = rel_section->constructor_chain;
       rel_section->constructor_chain = reloc;
       reloc->relent.address = rel_section->_cooked_size;
       /* ask the cpu which howto to use */
       reloc->relent.howto = bfd_reloc_type_lookup(abfd, BFD_RELOC_CTOR);
       rel_section->_cooked_size += sizeof(int *);
       rel_section->reloc_count++;
   }

}
