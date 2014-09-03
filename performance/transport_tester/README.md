mpiexec -n 4 performance/transport_tester/transport_tester -threads-per-node 8

Other options:

-minimum-buffer-size <minimum_buffer_size> (default: 16, in bytes)
-maximum-buffer-size <maximum_buffer_size> (default: 524288, in bytes)
-event-count <event_count> (default: 100000, this is the number of send events for each actor)
-concurrent-event-count <value> (default: 8, this is the maximum number of concurrent send events per actor)

The result should look like this:

process/1000000 PASSED 100404/100404, FAILED 0/100404
process/1000001 PASSED 99471/99471, FAILED 0/99471
process/1000002 PASSED 99814/99814, FAILED 0/99814
process/1000003 PASSED 100311/100311, FAILED 0/100311

