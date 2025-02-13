#!/bin/bash
# list-glob.sh: Generating [list] in a for-loop using "globbing".

echo

for file in *
do
  ls -l "$file"  # Lists all files in $PWD (current directory).
  # Recall that the wild card character "*" matches everything,
  # however, in "globbing", it doesn't match dot-files.

  # If the pattern matches no file, it is expanded to itself.
  # To prevent this, set the nullglob option
  # (shopt -s nullglob).
  # Thanks, S.C.
done

echo; echo

for file in [jx]*
do
  rm -f $file    # Removes only files beginning with "j" or "x" in $PWD.
  echo "Removed file \"$file\"".
done

echo

exit 0
