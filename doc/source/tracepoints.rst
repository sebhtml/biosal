LTTng tracepoints
=================

To use LTTng to trace Thorium applications, the compilation option
"CONFIG\_LTTNG" is required.

To get a complete list, use the command **lttng list -u**.

To learn how to use LTTng, go to http://lttng.org/docs/

Actor events
------------

-  thorium\_actor:receive\_enter
-  thorium\_actor:receive\_exit

Message delivery path
---------------------

Here is a list of LTTng tracepoints for the delivery path of messages.

-  thorium\_message:actor\_send
-  thorium\_message:worker\_send
-  thorium\_message:worker\_send\_enqueue
-  thorium\_message:node\_send
-  thorium\_message:node\_send\_system
-  thorium\_message:node\_send\_dispatch
-  thorium\_message:node\_dispatch\_message
-  thorium\_message:worker\_pool\_enqueue
-  thorium\_message:transport\_send
-  thorium\_message:transport\_receive
-  thorium\_message:node\_receive
-  thorium\_message:worker\_receive
-  thorium\_message:actor\_receive

Other tracepoints (not with LTTng)
==================================

These tracepoints are outdated. LTTng is better.

-  actor:receive\_enter
-  actor:receive\_exit

-  transport:send
-  transport:receive

-  node:run\_loop\_print
-  node:run\_loop\_receive
-  node:run\_loop\_run
-  node:run\_loop\_send
-  node:run\_loop\_pool\_work
-  node:run\_loop\_test\_requests
-  node:run\_loop\_do\_triage


