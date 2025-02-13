/* List lines of source files for GDB, the GNU debugger.
   Copyright (C) 1986, 1987, 1988, 1989, 1991 Free Software Foundation, Inc.

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

#include "defs.h"
#include "symtab.h"
#include "expression.h"
#include "language.h"
#include "command.h"
#include "gdbcmd.h"
#include "frame.h"

#ifdef USG
#include <sys/types.h>
#endif

#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gdbcore.h"
#include "regex.h"
#include "symfile.h"
#include "objfiles.h"

/* Prototypes for local functions. */

static int
open_source_file PARAMS ((struct symtab *));

static int
get_filename_and_charpos PARAMS ((struct symtab *, char **));

static void
reverse_search_command PARAMS ((char *, int));

static void
forward_search_command PARAMS ((char *, int));

static void
line_info PARAMS ((char *, int));

static void
list_command PARAMS ((char *, int));

static void
ambiguous_line_spec PARAMS ((struct symtabs_and_lines *));

static void
source_info PARAMS ((char *, int));

static void
show_directories PARAMS ((char *, int));

static void
find_source_lines PARAMS ((struct symtab *, int));

/* If we use this declaration, it breaks because of fucking ANSI "const" stuff
   on some systems.  We just have to not declare it at all, have it default
   to int, and possibly botch on a few systems.  Thanks, ANSIholes... */
/* extern char *strstr(); */

/* Path of directories to search for source files.
   Same format as the PATH environment variable's value.  */

char *source_path;

/* Symtab of default file for listing lines of.  */

struct symtab *current_source_symtab;

/* Default next line to list.  */

int current_source_line;

/* Default number of lines to print with commands like "list".
   This is based on guessing how many long (i.e. more than chars_per_line
   characters) lines there will be.  To be completely correct, "list"
   and friends should be rewritten to count characters and see where
   things are wrapping, but that would be a fair amount of work.  */

int lines_to_list = 10;

/* Line number of last line printed.  Default for various commands.
   current_source_line is usually, but not always, the same as this.  */

static int last_line_listed;

/* First line number listed by last listing command.  */

static int first_line_listed;


/* Set the source file default for the "list" command, specifying a
   symtab.  Sigh.  Behavior specification: If it is called with a
   non-zero argument, that is the symtab to select.  If it is not,
   first lookup "main"; if it exists, use the symtab and line it
   defines.  If not, take the last symtab in the symtab lists (if it
   exists) or the last symtab in the psymtab lists (if *it* exists).  If
   none of this works, report an error.   */

void
select_source_symtab (s)
     register struct symtab *s;
{
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  struct partial_symtab *ps;
  struct partial_symtab *cs_pst = 0;
  struct objfile *ofp;
  
  if (s)
    {
      current_source_symtab = s;
      current_source_line = 1;
      return;
    }

  /* Make the default place to list be the function `main'
     if one exists.  */
  if (lookup_symbol ("main", 0, VAR_NAMESPACE, 0, NULL))
    {
      sals = decode_line_spec ("main", 1);
      sal = sals.sals[0];
      free (sals.sals);
      current_source_symtab = sal.symtab;
      current_source_line = max (sal.line - (lines_to_list - 1), 1);
      if (current_source_symtab)
        return;
    }
  
  /* All right; find the last file in the symtab list (ignoring .h's).  */

  current_source_line = 1;

  for (ofp = object_files; ofp != NULL; ofp = ofp -> next)
    {
      for (s = ofp -> symtabs; s; s = s->next)
	{
	  char *name = s -> filename;
	  int len = strlen (name);
	  if (! (len > 2 && (strcmp (&name[len - 2], ".h") == 0)))
	    {
	      current_source_symtab = s;
	    }
	}
    }
  if (current_source_symtab)
    return;

  /* Howabout the partial symbol tables? */

  for (ofp = object_files; ofp != NULL; ofp = ofp -> next)
    {
      for (ps = ofp -> psymtabs; ps != NULL; ps = ps -> next)
	{
	  char *name = ps -> filename;
	  int len = strlen (name);
	  if (! (len > 2 && (strcmp (&name[len - 2], ".h") == 0)))
	    {
	      cs_pst = ps;
	    }
	}
    }
  if (cs_pst)
    {
      if (cs_pst -> readin)
	{
	  fatal ("Internal: select_source_symtab: readin pst found and no symtabs.");
	}
      else
	{
	  current_source_symtab = PSYMTAB_TO_SYMTAB (cs_pst);
	}
    }

  if (current_source_symtab)
    return;

  error ("Can't find a default source file");
}

static void
show_directories (ignore, from_tty)
     char *ignore;
     int from_tty;
{
  printf_filtered ("Source directories searched: %s\n", source_path);
}

/* Forget what we learned about line positions in source files,
   and which directories contain them;
   must check again now since files may be found in
   a different directory now.  */

void
forget_cached_source_info ()
{
  register struct symtab *s;
  register struct objfile *objfile;

  for (objfile = object_files; objfile != NULL; objfile = objfile -> next)
    {
      for (s = objfile -> symtabs; s != NULL; s = s -> next)
	{
	  if (s -> line_charpos != NULL)
	    {
	      mfree (objfile -> md, s -> line_charpos);
	      s -> line_charpos = NULL;
	    }
	  if (s -> fullname != NULL)
	    {
	      mfree (objfile -> md, s -> fullname);
	      s -> fullname = NULL;
	    }
	}
    }
}

void
init_source_path ()
{
  source_path = savestring ("$cdir:$cwd", /* strlen of it */ 10);
  forget_cached_source_info ();
}

/* Add zero or more directories to the front of the source path.  */
 
void
directory_command (dirname, from_tty)
     char *dirname;
     int from_tty;
{
  dont_repeat ();
  /* FIXME, this goes to "delete dir"... */
  if (dirname == 0)
    {
      if (query ("Reinitialize source path to empty? ", ""))
	{
	  free (source_path);
	  init_source_path ();
	}
    }
  else
    mod_path (dirname, &source_path);
  if (from_tty)
    show_directories ((char *)0, from_tty);
  forget_cached_source_info ();
}

/* Add zero or more directories to the front of an arbitrary path.  */

void
mod_path (dirname, which_path)
     char *dirname;
     char **which_path;
{
  char *old = *which_path;
  int prefix = 0;

  if (dirname == 0)
    return;

  dirname = strsave (dirname);
  make_cleanup (free, dirname);

  do
    {
      char *name = dirname;
      register char *p;
      struct stat st;

      {
	char *colon = strchr (name, ':');
	char *space = strchr (name, ' ');
	char *tab = strchr (name, '\t');
	if (colon == 0 && space == 0 && tab ==  0)
	  p = dirname = name + strlen (name);
	else
	  {
	    p = 0;
	    if (colon != 0 && (p == 0 || colon < p))
	      p = colon;
	    if (space != 0 && (p == 0 || space < p))
	      p = space;
	    if (tab != 0 && (p == 0 || tab < p))
	      p = tab;
	    dirname = p + 1;
	    while (*dirname == ':' || *dirname == ' ' || *dirname == '\t')
	      ++dirname;
	  }
      }

      if (p[-1] == '/')
	/* Sigh. "foo/" => "foo" */
	--p;
      *p = '\0';

      while (p[-1] == '.')
	{
	  if (p - name == 1)
	    {
	      /* "." => getwd ().  */
	      name = current_directory;
	      goto append;
	    }
	  else if (p[-2] == '/')
	    {
	      if (p - name == 2)
		{
		  /* "/." => "/".  */
		  *--p = '\0';
		  goto append;
		}
	      else
		{
		  /* "...foo/." => "...foo".  */
		  p -= 2;
		  *p = '\0';
		  continue;
		}
	    }
	  else
	    break;
	}

      if (name[0] == '~')
	name = tilde_expand (name);
      else if (name[0] != '/' && name[0] != '$')
	name = concat (current_directory, "/", name, NULL);
      else
	name = savestring (name, p - name);
      make_cleanup (free, name);

      /* Unless it's a variable, check existence.  */
      if (name[0] != '$') {
	if (stat (name, &st) < 0)
	  perror_with_name (name);
	if ((st.st_mode & S_IFMT) != S_IFDIR)
	  error ("%s is not a directory.", name);
      }

    append:
      {
	register unsigned int len = strlen (name);

	p = *which_path;
	while (1)
	  {
	    if (!strncmp (p, name, len)
		&& (p[len] == '\0' || p[len] == ':'))
	      {
		/* Found it in the search path, remove old copy */
		if (p > *which_path)
		  p--;			/* Back over leading colon */
		if (prefix > p - *which_path)
		  goto skip_dup;	/* Same dir twice in one cmd */
		strcpy (p, &p[len+1]);	/* Copy from next \0 or  : */
	      }
	    p = strchr (p, ':');
	    if (p != 0)
	      ++p;
	    else
	      break;
	  }
	if (p == 0)
	  {
	    /* If we have already tacked on a name(s) in this command,			   be sure they stay on the front as we tack on some more.  */
	    if (prefix)
	      {
		char *temp, c;

		c = old[prefix];
		old[prefix] = '\0';
		temp = concat (old, ":", name, NULL);
		old[prefix] = c;
		*which_path = concat (temp, "", &old[prefix], NULL);
		prefix = strlen (temp);
		free (temp);
	      }
	    else
	      {
		*which_path = concat (name, (old[0]? ":" : old), old, NULL);
		prefix = strlen (name);
	      }
	    free (old);
	    old = *which_path;
	  }
      }
  skip_dup: ;
    } while (*dirname != '\0');
}


static void
source_info (ignore, from_tty)
     char *ignore;
     int from_tty;
{
  register struct symtab *s = current_source_symtab;

  if (!s)
    {
      printf_filtered("No current source file.\n");
      return;
    }
  printf_filtered ("Current source file is %s\n", s->filename);
  if (s->dirname)
    printf_filtered ("Compilation directory is %s\n", s->dirname);
  if (s->fullname)
    printf_filtered ("Located in %s\n", s->fullname);
  if (s->nlines)
    printf_filtered ("Contains %d lines\n", s->nlines);

  printf_filtered("Source language %s.\n", language_str (s->language));
}



/* Open a file named STRING, searching path PATH (dir names sep by colons)
   using mode MODE and protection bits PROT in the calls to open.
   If TRY_CWD_FIRST, try to open ./STRING before searching PATH.
   (ie pretend the first element of PATH is ".")
   If FILENAMED_OPENED is non-null, set it to a newly allocated string naming
   the actual file opened (this string will always start with a "/".  We
   have to take special pains to avoid doubling the "/" between the directory
   and the file, sigh!  Emacs gets confuzzed by this when we print the
   source file name!!! 

   If a file is found, return the descriptor.
   Otherwise, return -1, with errno set for the last name we tried to open.  */

/*  >>>> This should only allow files of certain types,
    >>>>  eg executable, non-directory */
int
openp (path, try_cwd_first, string, mode, prot, filename_opened)
     char *path;
     int try_cwd_first;
     char *string;
     int mode;
     int prot;
     char **filename_opened;
{
  register int fd;
  register char *filename;
  register char *p, *p1;
  register int len;
  int alloclen;

  if (!path)
    path = ".";

  /* ./foo => foo */
  while (string[0] == '.' && string[1] == '/')
    string += 2;

  if (try_cwd_first || string[0] == '/')
    {
      filename = string;
      fd = open (filename, mode, prot);
      if (fd >= 0 || string[0] == '/')
	goto done;
    }

  alloclen = strlen (path) + strlen (string) + 2;
  filename = (char *) alloca (alloclen);
  fd = -1;
  for (p = path; p; p = p1 ? p1 + 1 : 0)
    {
      p1 = (char *) strchr (p, ':');
      if (p1)
	len = p1 - p;
      else
	len = strlen (p);

      if (len == 4 && p[0] == '$' && p[1] == 'c'
	           && p[2] == 'w' && p[3] == 'd') {
	/* Name is $cwd -- insert current directory name instead.  */
	int newlen;

	/* First, realloc the filename buffer if too short. */
	len = strlen (current_directory);
	newlen = len + strlen (string) + 2;
	if (newlen > alloclen) {
	  alloclen = newlen;
	  filename = (char *) alloca (alloclen);
	}
	strcpy (filename, current_directory);
      } else {
	/* Normal file name in path -- just use it.  */
	strncpy (filename, p, len);
	filename[len] = 0;
      }

      /* Remove trailing slashes */
      while (len > 0 && filename[len-1] == '/')
       filename[--len] = 0;

      strcat (filename+len, "/");
      strcat (filename, string);

      fd = open (filename, mode, prot);
      if (fd >= 0) break;
    }

 done:
  if (filename_opened)
    if (fd < 0)
      *filename_opened = (char *) 0;
    else if (filename[0] == '/')
      *filename_opened = savestring (filename, strlen (filename));
    else
      {
	/* Beware the // my son, the Emacs barfs, the botch that catch... */
	   
	*filename_opened = concat (current_directory, 
	   '/' == current_directory[strlen(current_directory)-1]? "": "/",
				   filename, NULL);
      }

  return fd;
}

/* Open a source file given a symtab S.  Returns a file descriptor
   or negative number for error.  */

static int
open_source_file (s)
     struct symtab *s;
{
  char *path = source_path;
  char *p;
  int result;
  char *fullname;

  /* Quick way out if we already know its full name */
  if (s->fullname) 
    {
      result = open (s->fullname, O_RDONLY);
      if (result >= 0)
        return result;
      /* Didn't work -- free old one, try again. */
      mfree (s->objfile->md, s->fullname);
      s->fullname = NULL;
    }

  if (s->dirname != NULL)
    {
      /* Replace a path entry of  $cdir  with the compilation directory name */
#define	cdir_len	5
      /* We cast strstr's result in case an ANSIhole has made it const,
	 which produces a "required warning" when assigned to a nonconst. */
      p = (char *)strstr (source_path, "$cdir");
      if (p && (p == path || p[-1] == ':')
	    && (p[cdir_len] == ':' || p[cdir_len] == '\0')) {
	int len;

	path = (char *)
	       alloca (strlen (source_path) + 1 + strlen (s->dirname) + 1);
	len = p - source_path;
	strncpy (path, source_path, len);		/* Before $cdir */
	strcpy (path + len, s->dirname);		/* new stuff */
	strcat (path + len, source_path + len + cdir_len); /* After $cdir */
      }
    }

  result = openp (path, 0, s->filename, O_RDONLY, 0, &s->fullname);
  if (result < 0)
    {
      /* Didn't work.  Try using just the basename. */
      p = basename (s->filename);
      if (p != s->filename)
	result = openp(path, 0, p, O_RDONLY,0, &s->fullname);
    }
  if (result >= 0)
    {
      fullname = s -> fullname;
      s -> fullname = mstrsave (s -> objfile -> md, s -> fullname);
      free (fullname);
    }
  return result;
}


/* Create and initialize the table S->line_charpos that records
   the positions of the lines in the source file, which is assumed
   to be open on descriptor DESC.
   All set S->nlines to the number of such lines.  */

static void
find_source_lines (s, desc)
     struct symtab *s;
     int desc;
{
  struct stat st;
  register char *data, *p, *end;
  int nlines = 0;
  int lines_allocated = 1000;
  int *line_charpos;
  long exec_mtime;
  int size;

  line_charpos = (int *) xmmalloc (s -> objfile -> md,
				   lines_allocated * sizeof (int));
  if (fstat (desc, &st) < 0)
    perror_with_name (s->filename);

  if (exec_bfd) {
    exec_mtime = bfd_get_mtime(exec_bfd);
    if (exec_mtime && exec_mtime < st.st_mtime)
      printf_filtered ("Source file is more recent than executable.\n");
  }

  /* st_size might be a large type, but we only support source files whose 
     size fits in an int.  FIXME. */
  size = (int) st.st_size;

#ifdef BROKEN_LARGE_ALLOCA
  data = (char *) xmalloc (size);
  make_cleanup (free, data);
#else
  data = (char *) alloca (size);
#endif
  if (myread (desc, data, size) < 0)
    perror_with_name (s->filename);
  end = data + size;
  p = data;
  line_charpos[0] = 0;
  nlines = 1;
  while (p != end)
    {
      if (*p++ == '\n'
	  /* A newline at the end does not start a new line.  */
	  && p != end)
	{
	  if (nlines == lines_allocated)
	    {
	      lines_allocated *= 2;
	      line_charpos =
		  (int *) xmrealloc (s -> objfile -> md, (char *) line_charpos,
				     sizeof (int) * lines_allocated);
	    }
	  line_charpos[nlines++] = p - data;
	}
    }
  s->nlines = nlines;
  s->line_charpos =
      (int *) xmrealloc (s -> objfile -> md, (char *) line_charpos,
			 nlines * sizeof (int));
}

/* Return the character position of a line LINE in symtab S.
   Return 0 if anything is invalid.  */

#if 0	/* Currently unused */

int
source_line_charpos (s, line)
     struct symtab *s;
     int line;
{
  if (!s) return 0;
  if (!s->line_charpos || line <= 0) return 0;
  if (line > s->nlines)
    line = s->nlines;
  return s->line_charpos[line - 1];
}

/* Return the line number of character position POS in symtab S.  */

int
source_charpos_line (s, chr)
    register struct symtab *s;
    register int chr;
{
  register int line = 0;
  register int *lnp;
    
  if (s == 0 || s->line_charpos == 0) return 0;
  lnp = s->line_charpos;
  /* Files are usually short, so sequential search is Ok */
  while (line < s->nlines  && *lnp <= chr)
    {
      line++;
      lnp++;
    }
  if (line >= s->nlines)
    line = s->nlines;
  return line;
}

#endif	/* 0 */


/* Get full pathname and line number positions for a symtab.
   Return nonzero if line numbers may have changed.
   Set *FULLNAME to actual name of the file as found by `openp',
   or to 0 if the file is not found.  */

static int
get_filename_and_charpos (s, fullname)
     struct symtab *s;
     char **fullname;
{
  register int desc, linenums_changed = 0;
  
  desc = open_source_file (s);
  if (desc < 0)
    {
      if (fullname)
	*fullname = NULL;
      return 0;
    }  
  if (fullname)
    *fullname = s->fullname;
  if (s->line_charpos == 0) linenums_changed = 1;
  if (linenums_changed) find_source_lines (s, desc);
  close (desc);
  return linenums_changed;
}

/* Print text describing the full name of the source file S
   and the line number LINE and its corresponding character position.
   The text starts with two Ctrl-z so that the Emacs-GDB interface
   can easily find it.

   MID_STATEMENT is nonzero if the PC is not at the beginning of that line.

   Return 1 if successful, 0 if could not find the file.  */

int
identify_source_line (s, line, mid_statement)
     struct symtab *s;
     int line;
     int mid_statement;
{
  if (s->line_charpos == 0)
    get_filename_and_charpos (s, (char **)NULL);
  if (s->fullname == 0)
    return 0;
  printf ("\032\032%s:%d:%d:%s:0x%x\n", s->fullname,
	  line, s->line_charpos[line - 1],
	  mid_statement ? "middle" : "beg",
	  get_frame_pc (get_current_frame()));
  current_source_line = line;
  first_line_listed = line;
  last_line_listed = line;
  current_source_symtab = s;
  return 1;
}

/* Print source lines from the file of symtab S,
   starting with line number LINE and stopping before line number STOPLINE.  */

void
print_source_lines (s, line, stopline, noerror)
     struct symtab *s;
     int line, stopline;
     int noerror;
{
  register int c;
  register int desc;
  register FILE *stream;
  int nlines = stopline - line;

  /* Regardless of whether we can open the file, set current_source_symtab. */
  current_source_symtab = s;
  current_source_line = line;
  first_line_listed = line;

  desc = open_source_file (s);
  if (desc < 0)
    {
      if (! noerror) {
	char *name = alloca (strlen (s->filename) + 100);
	sprintf (name, "%s:%d", s->filename, line);
        print_sys_errmsg (name, errno);
      }
      return;
    }

  if (s->line_charpos == 0)
    find_source_lines (s, desc);

  if (line < 1 || line > s->nlines)
    {
      close (desc);
      error ("Line number %d out of range; %s has %d lines.",
	     line, s->filename, s->nlines);
    }

  if (lseek (desc, s->line_charpos[line - 1], 0) < 0)
    {
      close (desc);
      perror_with_name (s->filename);
    }

  stream = fdopen (desc, "r");
  clearerr (stream);

  while (nlines-- > 0)
    {
      c = fgetc (stream);
      if (c == EOF) break;
      last_line_listed = current_source_line;
      printf_filtered ("%d\t", current_source_line++);
      do
	{
	  if (c < 040 && c != '\t' && c != '\n' && c != '\r')
	      printf_filtered ("^%c", c + 0100);
	  else if (c == 0177)
	    printf_filtered ("^?");
	  else
	    printf_filtered ("%c", c);
	} while (c != '\n' && (c = fgetc (stream)) >= 0);
    }

  fclose (stream);
}



/* 
  C++
  Print a list of files and line numbers which a user may choose from
  in order to list a function which was specified ambiguously
  (as with `list classname::overloadedfuncname', for example).
  The vector in SALS provides the filenames and line numbers.
  */
static void
ambiguous_line_spec (sals)
     struct symtabs_and_lines *sals;
{
  int i;

  for (i = 0; i < sals->nelts; ++i)
    printf_filtered("file: \"%s\", line number: %d\n",
		    sals->sals[i].symtab->filename, sals->sals[i].line);
}


static void
list_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  struct symtabs_and_lines sals, sals_end;
  struct symtab_and_line sal, sal_end;
  struct symbol *sym;
  char *arg1;
  int no_end = 1;
  int dummy_end = 0;
  int dummy_beg = 0;
  int linenum_beg = 0;
  char *p;

  if (!have_full_symbols () && !have_partial_symbols())
    error ("No symbol table is loaded.  Use the \"file\" command.");

  /* Pull in a current source symtab if necessary */
  if (current_source_symtab == 0 &&
      (arg == 0 || arg[0] == '+' || arg[0] == '-'))
    select_source_symtab (0);

  /* "l" or "l +" lists next ten lines.  */

  if (arg == 0 || !strcmp (arg, "+"))
    {
      if (current_source_symtab == 0)
	error ("No default source file yet.  Do \"help list\".");
      print_source_lines (current_source_symtab, current_source_line,
			  current_source_line + lines_to_list, 0);
      return;
    }

  /* "l -" lists previous ten lines, the ones before the ten just listed.  */
  if (!strcmp (arg, "-"))
    {
      if (current_source_symtab == 0)
	error ("No default source file yet.  Do \"help list\".");
      print_source_lines (current_source_symtab,
			  max (first_line_listed - lines_to_list, 1),
			  first_line_listed, 0);
      return;
    }

  /* Now if there is only one argument, decode it in SAL
     and set NO_END.
     If there are two arguments, decode them in SAL and SAL_END
     and clear NO_END; however, if one of the arguments is blank,
     set DUMMY_BEG or DUMMY_END to record that fact.  */

  arg1 = arg;
  if (*arg1 == ',')
    dummy_beg = 1;
  else
    {
      sals = decode_line_1 (&arg1, 0, 0, 0);

      if (! sals.nelts) return;  /*  C++  */
      if (sals.nelts > 1)
	{
	  ambiguous_line_spec (&sals);
	  free (sals.sals);
	  return;
	}

      sal = sals.sals[0];
      free (sals.sals);
    }

  /* Record whether the BEG arg is all digits.  */

  for (p = arg; p != arg1 && *p >= '0' && *p <= '9'; p++);
  linenum_beg = (p == arg1);

  while (*arg1 == ' ' || *arg1 == '\t')
    arg1++;
  if (*arg1 == ',')
    {
      no_end = 0;
      arg1++;
      while (*arg1 == ' ' || *arg1 == '\t')
	arg1++;
      if (*arg1 == 0)
	dummy_end = 1;
      else
	{
	  if (dummy_beg)
	    sals_end = decode_line_1 (&arg1, 0, 0, 0);
	  else
	    sals_end = decode_line_1 (&arg1, 0, sal.symtab, sal.line);
	  if (sals_end.nelts == 0) 
	    return;
	  if (sals_end.nelts > 1)
	    {
	      ambiguous_line_spec (&sals_end);
	      free (sals_end.sals);
	      return;
	    }
	  sal_end = sals_end.sals[0];
	  free (sals_end.sals);
	}
    }

  if (*arg1)
    error ("Junk at end of line specification.");

  if (!no_end && !dummy_beg && !dummy_end
      && sal.symtab != sal_end.symtab)
    error ("Specified start and end are in different files.");
  if (dummy_beg && dummy_end)
    error ("Two empty args do not say what lines to list.");
 
  /* if line was specified by address,
     first print exactly which line, and which file.
     In this case, sal.symtab == 0 means address is outside
     of all known source files, not that user failed to give a filename.  */
  if (*arg == '*')
    {
      if (sal.symtab == 0)
	error ("No source file for address %s.", local_hex_string(sal.pc));
      sym = find_pc_function (sal.pc);
      if (sym)
	{
	  printf_filtered ("%s is in ", local_hex_string(sal.pc));
	  fprint_symbol (stdout, SYMBOL_NAME (sym));
	  printf_filtered (" (%s:%d).\n", sal.symtab->filename, sal.line);
	}
      else
	printf_filtered ("%s is at %s:%d.\n",
			 local_hex_string(sal.pc), 
			 sal.symtab->filename, sal.line);
    }

  /* If line was not specified by just a line number,
     and it does not imply a symtab, it must be an undebuggable symbol
     which means no source code.  */

  if (! linenum_beg && sal.symtab == 0)
    error ("No line number known for %s.", arg);

  /* If this command is repeated with RET,
     turn it into the no-arg variant.  */

  if (from_tty)
    *arg = 0;

  if (dummy_beg && sal_end.symtab == 0)
    error ("No default source file yet.  Do \"help list\".");
  if (dummy_beg)
    print_source_lines (sal_end.symtab,
			max (sal_end.line - (lines_to_list - 1), 1),
			sal_end.line + 1, 0);
  else if (sal.symtab == 0)
    error ("No default source file yet.  Do \"help list\".");
  else if (no_end)
    print_source_lines (sal.symtab,
			max (sal.line - (lines_to_list / 2), 1),
			sal.line + (lines_to_list / 2), 0);
  else
    print_source_lines (sal.symtab, sal.line,
			(dummy_end
			 ? sal.line + lines_to_list
			 : sal_end.line + 1),
			0);
}

/* Print info on range of pc's in a specified line.  */

static void
line_info (arg, from_tty)
     char *arg;
     int from_tty;
{
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  CORE_ADDR start_pc, end_pc;
  int i;

  if (arg == 0)
    {
      sal.symtab = current_source_symtab;
      sal.line = last_line_listed;
      sals.nelts = 1;
      sals.sals = (struct symtab_and_line *)
	xmalloc (sizeof (struct symtab_and_line));
      sals.sals[0] = sal;
    }
  else
    {
      sals = decode_line_spec_1 (arg, 0);
      
      /* If this command is repeated with RET,
	 turn it into the no-arg variant.  */
      if (from_tty)
	*arg = 0;
    }

  /* C++  More than one line may have been specified, as when the user
     specifies an overloaded function name. Print info on them all. */
  for (i = 0; i < sals.nelts; i++)
    {
      sal = sals.sals[i];
      
      if (sal.symtab == 0)
	error ("No source file specified.");

      if (sal.line > 0
	  && find_line_pc_range (sal.symtab, sal.line, &start_pc, &end_pc))
	{
	  if (start_pc == end_pc)
	    printf_filtered ("Line %d of \"%s\" is at pc %s but contains no code.\n",
			     sal.line, sal.symtab->filename, local_hex_string(start_pc));
	  else
	    {
	      printf_filtered ("Line %d of \"%s\" starts at pc %s",
			       sal.line, sal.symtab->filename, 
			       local_hex_string(start_pc));
	      printf_filtered (" and ends at %s.\n",
			       local_hex_string(end_pc));
	    }
	  /* x/i should display this line's code.  */
	  set_next_address (start_pc);
	  /* Repeating "info line" should do the following line.  */
	  last_line_listed = sal.line + 1;
	}
      else
	printf_filtered ("Line number %d is out of range for \"%s\".\n",
			 sal.line, sal.symtab->filename);
    }
}

/* Commands to search the source file for a regexp.  */

/* ARGSUSED */
static void
forward_search_command (regex, from_tty)
     char *regex;
     int from_tty;
{
  register int c;
  register int desc;
  register FILE *stream;
  int line = last_line_listed + 1;
  char *msg;

  msg = (char *) re_comp (regex);
  if (msg)
    error (msg);

  if (current_source_symtab == 0)
    select_source_symtab (0);

  /* Search from last_line_listed+1 in current_source_symtab */

  desc = open_source_file (current_source_symtab);
  if (desc < 0)
    perror_with_name (current_source_symtab->filename);

  if (current_source_symtab->line_charpos == 0)
    find_source_lines (current_source_symtab, desc);

  if (line < 1 || line > current_source_symtab->nlines)
    {
      close (desc);
      error ("Expression not found");
    }

  if (lseek (desc, current_source_symtab->line_charpos[line - 1], 0) < 0)
    {
      close (desc);
      perror_with_name (current_source_symtab->filename);
    }

  stream = fdopen (desc, "r");
  clearerr (stream);
  while (1) {
/* FIXME!!!  We walk right off the end of buf if we get a long line!!! */
    char buf[4096];		/* Should be reasonable??? */
    register char *p = buf;

    c = getc (stream);
    if (c == EOF)
      break;
    do {
      *p++ = c;
    } while (c != '\n' && (c = getc (stream)) >= 0);

    /* we now have a source line in buf, null terminate and match */
    *p = 0;
    if (re_exec (buf) > 0)
      {
	/* Match! */
	fclose (stream);
	print_source_lines (current_source_symtab,
			   line, line+1, 0);
	current_source_line = max (line - lines_to_list / 2, 1);
	return;
      }
    line++;
  }

  printf_filtered ("Expression not found\n");
  fclose (stream);
}

/* ARGSUSED */
static void
reverse_search_command (regex, from_tty)
     char *regex;
     int from_tty;
{
  register int c;
  register int desc;
  register FILE *stream;
  int line = last_line_listed - 1;
  char *msg;

  msg = (char *) re_comp (regex);
  if (msg)
    error (msg);

  if (current_source_symtab == 0)
    select_source_symtab (0);

  /* Search from last_line_listed-1 in current_source_symtab */

  desc = open_source_file (current_source_symtab);
  if (desc < 0)
    perror_with_name (current_source_symtab->filename);

  if (current_source_symtab->line_charpos == 0)
    find_source_lines (current_source_symtab, desc);

  if (line < 1 || line > current_source_symtab->nlines)
    {
      close (desc);
      error ("Expression not found");
    }

  if (lseek (desc, current_source_symtab->line_charpos[line - 1], 0) < 0)
    {
      close (desc);
      perror_with_name (current_source_symtab->filename);
    }

  stream = fdopen (desc, "r");
  clearerr (stream);
  while (line > 1)
    {
/* FIXME!!!  We walk right off the end of buf if we get a long line!!! */
      char buf[4096];		/* Should be reasonable??? */
      register char *p = buf;

      c = getc (stream);
      if (c == EOF)
	break;
      do {
	*p++ = c;
      } while (c != '\n' && (c = getc (stream)) >= 0);

      /* We now have a source line in buf; null terminate and match.  */
      *p = 0;
      if (re_exec (buf) > 0)
	{
	  /* Match! */
	  fclose (stream);
	  print_source_lines (current_source_symtab,
			      line, line+1, 0);
	  current_source_line = max (line - lines_to_list / 2, 1);
	  return;
	}
      line--;
      if (fseek (stream, current_source_symtab->line_charpos[line - 1], 0) < 0)
	{
	  fclose (stream);
	  perror_with_name (current_source_symtab->filename);
	}
    }

  printf_filtered ("Expression not found\n");
  fclose (stream);
  return;
}

void
_initialize_source ()
{
  current_source_symtab = 0;
  init_source_path ();

  add_com ("directory", class_files, directory_command,
	   "Add directory DIR to beginning of search path for source files.\n\
Forget cached info on source file locations and line positions.\n\
DIR can also be $cwd for the current working directory, or $cdir for the\n\
directory in which the source file was compiled into object code.\n\
With no argument, reset the search path to $cdir:$cwd, the default.");

  add_cmd ("directories", no_class, show_directories,
	   "Current search path for finding source files.\n\
$cwd in the path means the current working directory.\n\
$cdir in the path means the compilation directory of the source file.",
	   &showlist);

  add_info ("source", source_info,
	    "Information about the current source file.");

  add_info ("line", line_info,
	    "Core addresses of the code for a source line.\n\
Line can be specified as\n\
  LINENUM, to list around that line in current file,\n\
  FILE:LINENUM, to list around that line in that file,\n\
  FUNCTION, to list around beginning of that function,\n\
  FILE:FUNCTION, to distinguish among like-named static functions.\n\
Default is to describe the last source line that was listed.\n\n\
This sets the default address for \"x\" to the line's first instruction\n\
so that \"x/i\" suffices to start examining the machine code.\n\
The address is also stored as the value of \"$_\".");

  add_com ("forward-search", class_files, forward_search_command,
	   "Search for regular expression (see regex(3)) from last line listed.");
  add_com_alias ("search", "forward-search", class_files, 0);

  add_com ("reverse-search", class_files, reverse_search_command,
	   "Search backward for regular expression (see regex(3)) from last line listed.");

  add_com ("list", class_files, list_command,
	   "List specified function or line.\n\
With no argument, lists ten more lines after or around previous listing.\n\
\"list -\" lists the ten lines before a previous ten-line listing.\n\
One argument specifies a line, and ten lines are listed around that line.\n\
Two arguments with comma between specify starting and ending lines to list.\n\
Lines can be specified in these ways:\n\
  LINENUM, to list around that line in current file,\n\
  FILE:LINENUM, to list around that line in that file,\n\
  FUNCTION, to list around beginning of that function,\n\
  FILE:FUNCTION, to distinguish among like-named static functions.\n\
  *ADDRESS, to list around the line containing that address.\n\
With two args if one is empty it stands for ten lines away from the other arg.");
  add_com_alias ("l", "list", class_files, 0);

  add_show_from_set
    (add_set_cmd ("listsize", class_support, var_uinteger,
		  (char *)&lines_to_list,
	"Set number of source lines gdb will list by default.",
		  &setlist),
     &showlist);
}
