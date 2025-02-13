#!/bin/bash

# Indirect variable referencing.
# This has a few of the attributes of references in C++.


a=letter_of_alphabet
letter_of_alphabet=z

echo "a = $a"           # Direct reference.

echo "Now a = ${!a}"    # Indirect reference.
# The ${!variable} notation is greatly superior to the old "eval var1=\$$var2"

echo

t=table_cell_3
table_cell_3=24
echo "t = ${!t}"        # t = 24
table_cell_3=387
echo "Value of t changed to ${!t}"    # 387

# This is useful for referencing members of an array or table,
# or for simulating a multi-dimensional array.
# An indexing option would have been nice (sigh).

exit 0
