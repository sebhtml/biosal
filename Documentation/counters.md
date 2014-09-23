# Thorium event counters

To print counters at the node level, add -print-counters to the command.


At the beginning, this line will be printed:

biosal> node/0: 5 threads, 4 workers


At the end, counters will be printed like this:

biosal> Counters for node/0
BSAL_COUNTER_SPAWNED_ACTORS 3
BSAL_COUNTER_KILLED_ACTORS 3
 balance BSAL_COUNTER_BALANCE_ACTORS 0
BSAL_COUNTER_RECEIVED_MESSAGES 36
  BSAL_COUNTER_RECEIVED_MESSAGES_FROM_SELF 36
  BSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF 0
BSAL_COUNTER_SENT_MESSAGES 36
  BSAL_COUNTER_SENT_MESSAGES_TO_SELF 36
  BSAL_COUNTER_SENT_MESSAGES_NOT_TO_SELF 0
 balance BSAL_COUNTER_BALANCE_MESSAGES 0
BSAL_COUNTER_RECEIVED_BYTES 84
  BSAL_COUNTER_RECEIVED_BYTES_FROM_SELF 84
  BSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF 0
BSAL_COUNTER_SENT_BYTES 84
  BSAL_COUNTER_SENT_BYTES_TO_SELF 84
 BSAL_COUNTER_SENT_BYTES_NOT_TO_SELF 0
 balance BSAL_COUNTER_BALANCE_BYTES 0


# Thorium transport profiler

-enable-transport-profiler


# Processor load

-print-load


# Actor load profiler

-enable-actor-load-profiler
