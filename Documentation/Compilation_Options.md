
Some compilation options are activated with ifdef statements.

Other option can be changed at compile time.

| Option | Possible values | Default value | Description | Notes |
| --- |
| CC | anything (icc, mpicc, gcc, cc, craycc, and so on) | mpicc | C compiler (C 1999) | |
| CFLAGS |  | -O3 -g | Compilation flags | |
| LDFLAGS |  | -lm -lpthread | Linking options | |
| CONFIG_CLOCK_GETTIME | y or n | y | Enable real-time clock (time in nanoseconds) | Disabled on Apple Mac |
| CONFIG_LTTNG | y or n | n | Enable LTTng tracepoints for tracing | This is useful to understand bottlenecks and performance issues |
| CONFIG_DEBUG | y or n | n | Enable assertions in the code tree | This may produces slightly slower code | Useful for debugging. |
| CONFIG_ZLIB | y or n | y | Enable support for zlib-compressed files | |
| CONFIG_PAMI | y or n | n | Enable PAMI transport (IBM Parallel Active Message Interface) | Only works on IBM Blue Gene/Q |
| CONFIG_MPI | y or n | y | Enable MPI transport (Message Passing Interface) | Portable |
