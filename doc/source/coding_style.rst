Coding style
============

K & R style with 4 spaces for indentation
http://en.wikipedia.org/wiki/Indent\_style#K.26R\_style

The coding style is similar to the one used in Linux (aside from the 4
spaces instead of tabs).

see https://www.kernel.org/doc/Documentation/CodingStyle

Names
=====

+---------------------------------------------------+-----------------+-------------------+
| Project                                           | Symbol prefix   | Constant prefix   |
+===================================================+=================+===================+
| Thorium actor engine (engine/thorium/)            | thorium\_       | THORIUM\_         |
+---------------------------------------------------+-----------------+-------------------+
| core support code (core/) things (support code)   | core\_          | CORE\_            |
+---------------------------------------------------+-----------------+-------------------+
| Actor library                                     | biosal\_        | BIOSAL\_          |
+---------------------------------------------------+-----------------+-------------------+

Any message action must use the prefix ACTION\_. Any actor script
identifier must start with SCRIPT\_.

New actions can be generated with ./scripts/generate-random-tag.py

Use self for the current object like in Ruby, Smalltalk, Apple Swift or
in Objective C.

The name of function types end with \*\ *fn*\ t

Commits
=======

We try to use the git standardized tags in commits.

These are described here:

https://www.kernel.org/doc/Documentation/SubmittingPatches

Statements
==========

goto are not allowed in the BIOSAL project for readability purposes.

break and continue statements are allowed.

While having one single return statement in a function is easier to
read, this is not a requirement neither.

Doxygen
=======

Use the Javadoc style.

Example

/\*\* \* A function * * @param foo parameter 1 \* @param bar parameter 2
\* @return something \*/

See http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html

init / destroy
==============

Any data structure should have a constructor ending with *init and a
destructor ending with *\ destroy.

add / remove / get
==================

When a data structure has functions to add and/or remove and/or get
elements, the function names should be:

-  add (instead of push or insert)
-  remove (instead of delete; delete means erase whereas remove means
   take away)
-  get (instead of find)

@see
http://english.stackexchange.com/questions/52508/difference-between-delete-and-remove

Logging
=======

Inside an actor script, one should use thorium\_actor\_log instead of
printf.
