Thorium event counters
======================

To print counters at the node level, add -print-counters to the command.

At the beginning, this line will be printed:

biosal> node/0: 5 threads, 4 workers

At the end, counters will be printed like this:

biosal> Counters for node/0 BSAL\_COUNTER\_SPAWNED\_ACTORS 3
BSAL\_COUNTER\_KILLED\_ACTORS 3 balance BSAL\_COUNTER\_BALANCE\_ACTORS 0
BSAL\_COUNTER\_RECEIVED\_MESSAGES 36
BSAL\_COUNTER\_RECEIVED\_MESSAGES\_FROM\_SELF 36
BSAL\_COUNTER\_RECEIVED\_MESSAGES\_NOT\_FROM\_SELF 0
BSAL\_COUNTER\_SENT\_MESSAGES 36 BSAL\_COUNTER\_SENT\_MESSAGES\_TO\_SELF
36 BSAL\_COUNTER\_SENT\_MESSAGES\_NOT\_TO\_SELF 0 balance
BSAL\_COUNTER\_BALANCE\_MESSAGES 0 BSAL\_COUNTER\_RECEIVED\_BYTES 84
BSAL\_COUNTER\_RECEIVED\_BYTES\_FROM\_SELF 84
BSAL\_COUNTER\_RECEIVED\_BYTES\_NOT\_FROM\_SELF 0
BSAL\_COUNTER\_SENT\_BYTES 84 BSAL\_COUNTER\_SENT\_BYTES\_TO\_SELF 84
BSAL\_COUNTER\_SENT\_BYTES\_NOT\_TO\_SELF 0 balance
BSAL\_COUNTER\_BALANCE\_BYTES 0

Thorium transport profiler
==========================

-enable-transport-profiler

Processor load
==============

-print-thorium-data

Actor load profiler
===================

-enable-actor-load-profiler
