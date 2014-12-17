Performance
===============

Factors:

queue implementation (no moves)
-------------------------------

linked list of rings, maximize reutilization with a recycle bin

good work scheduling algorithm
------------------------------

attempt count is currently 4

lockless rings for workers
--------------------------

One for works, one for messages

Concurrency
-----------

This is a good read:
http://disruptor.googlecode.com/files/Disruptor-1.0.pdf

2 important things:

-  Mutual exclusion when writing a shared variable
-  Visibility of changes in other threads

If any variable has only one writer, then mutual exclusion is enforced
without doing anything.

Profiling
---------

Tools used: perf, valgrind, gprof, gdb

To use perf, compile with scripts/perf/build-perf.sh
(-fno-omit-frame-pointer).

Then use scripts/perf/perf-record-lwp.sh to record a given thread (LWP).

perf top
--------

Otherwise, 'perf top' is your friend too.

-  perf top -e cache-misses
-  perf top -e branch-misses

Stateless actors
----------------

Stateless actors are more predictable in their performance and should be
selected whenever possible.
