Open-MPI 1.8.1
==============

vader bug
---------

The vader btl is broken and it produces SIGSEGV signals. The workaround
is to use '--mca btl ^vader'.

https://github.com/open-mpi/ompi/issues/235

sm bug
------

Also, sometimes the sm btl hangs. To solve that, add '--mca btl ^sm' to
the mpiexec command.

http://www.open-mpi.org/community/lists/devel/2014/06/15055.php

MPICH 3.1.1
===========

MPI\_Test bug
-------------

Sometimes, calling MPI\_Test will call poll() internally, and sometimes
poll() hangs.

The following link is a similar problem:

http://trac.mpich.org/projects/mpich/ticket/1910

Cray MPI (based on MPICH) 6.0.2
===============================

get\_count.c at line 34
-----------------------

The module cray-mpich/6.0.2 sometimes throws this (when using MPI\_Test
+ MPI\_Irecv + MPI\_Isend):

Assertion failed in file
/notbackedup/tmp/ulib/mpt/nightly/6.0/081313/mpich2/src/mpi/datatype/get\_count.c
at line 34: size >= 0 && status->count >= 0
