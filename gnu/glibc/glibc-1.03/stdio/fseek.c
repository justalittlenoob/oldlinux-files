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
#include <errno.h>
#include <stdio.h>


/* Move the file position of STREAM to OFFSET
   bytes from the beginning of the file if WHENCE
   is SEEK_SET, the end of the file is it is SEEK_END,
   or the current position if it is SEEK_CUR.  */
int
DEFUN(fseek, (stream, offset, whence),
      register FILE *stream AND long int offset AND int whence)
{
  long int o;

  if (!__validfp(stream))
    {
      errno = EINVAL;
      return EOF;
    }

  stream->__eof = 0;

  if (stream->__put_limit > stream->__buffer)
    {
      /* Write out the pending data.  */
      if (__flshfp(stream, EOF) == EOF)
	return EOF;
    }

  /* Make sure we know the current offset info.  */
  if (__stdio_check_offset(stream) == EOF)
    return EOF;

  /* Check the WHENCE argument for validity, and process OFFSET
     into an absolute position in O.  By the end of this switch,
     either we have returned, or O contains an absolute position.  */
  o = offset;
  switch (whence)
    {
    default:
      errno = EINVAL;
      return EOF;

    case SEEK_END:
      /* We don't know where the end of the file is,
	 so seek to the position in the file the user asked
	 for, and then look where that is.  */
      if (stream->__io_funcs.__seek == NULL)
	{
	  errno = ESPIPE;
	  return EOF;
	}
      else
	{
	  fpos_t pos = (fpos_t) o;
	  if ((*stream->__io_funcs.__seek)(stream->__cookie, &pos,
					   SEEK_END) < 0)
	    {
	      if (errno == ESPIPE)
		stream->__io_funcs.__seek = NULL;
	      return EOF;
	    }
	  stream->__offset = pos;
	  /* Make O be absolute, rather than
	     relative to the end of the file.  */
	  o += pos;
	}

      /* Fall through to try an absolute seek.  */

    case SEEK_SET:
      /* Make O be relative to the buffer.  */
      o -= stream->__target;
      /* Make O be relative to the current position in the buffer.  */
      o -= stream->__bufp - stream->__buffer;

      /* Fall through to see if we can do it by
	 moving the pointer around in the buffer.  */

    case SEEK_CUR:
      /* If the offset is small enough, we can just
	 move the pointer around in the buffer.  */

#if 0	/* Why did I think this would ever work???  */
      if (stream->__put_limit > stream->__buffer)
	{
	  /* We are writing.  */
	  if (stream->__bufp + o >= stream->__buffer &&
	      stream->__put_limit > stream->__bufp + o &&
	      stream->__get_limit > stream->__bufp + o)
	    {
	      /* We have read all the data we will change soon.
		 We can just move the pointer around.  */
	      stream->__bufp += o;
	      return 0;
	    }
	  else
	    {
	      /* Flush the buffer.  */
	      if (__flshfp(stream, EOF) == EOF)
		return EOF;
	    }
	} else
#endif
      if (o < 0 ?
	  (-o <= stream->__bufp - stream->__buffer) :
	  o < stream->__get_limit - stream->__bufp)
	{
	  stream->__bufp += o;
	  return 0;
	}

      /* Turn it into an absolute seek.  */
      o += stream->__bufp - stream->__buffer;
      o += stream->__target;
      break;
    }

  /* O is now an absolute position, the new target.  */
  stream->__target = o;

  /* Set bufp and both end pointers to the beginning of the buffer.
     The next i/o will force a call to the input/output room function.  */
  stream->__bufp
    = stream->__get_limit = stream->__put_limit = stream->__buffer;

  /* If there is no seek function, seeks always fail.  */
  if (stream->__io_funcs.__seek == NULL)
    {
      /* This is preemptive, since we don't actually do the seeking.
	 But it makes more sense for fseek to to fail with ESPIPE
	 than for the next reading or writing operation to fail
	 that way.  */
      errno = ESPIPE;
      return EOF;
    }

  /* Don't actually seek.  The next reading or writing operation
     will force a call to the input or output room function,
     which will move to the target file position before reading or writing.  */
  return 0;
}
