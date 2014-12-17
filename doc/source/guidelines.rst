Guidelines
==============

Here are some guidelines to creating actor applications with Thorium
which provides no premptive multitasking (it uses a run-to-completion
approach).

1. Over-decomposition should be favored. An actor should do one thing
   and do it well (this is similar to the UNIX philosophy). Having a lot
   of actors per CPU core allows overlapping of communication and
   computation.

2. Messages should be small, but not too small to avoid a low
   computation-to-communication ratio. Buffer sizes can be adjusted so
   that actor receive computations last longer than communications. Big
   messages are not generally a good idea because typically an actor
   that receives a big message will hog the CPU core for a while

3. The request-reply message pattern is needed frequently to avoid
   communication imbalance. Such an imbalance is also called
   producer-consumer imbalance.

4. An actor can avoid running for too long by sending ACTION\_YIELD to
   itself. Such an actor is then responsible to figure out where the
   continuation should take place.

5. Another trick to keep the "pipeline" busy is to have more than 1
   in-flight message for any destination (at the expense of memory
   usage).

--------------

References

-  Patterns for Overlapping Communication and Computation
   http://charm.cs.illinois.edu/newPapers/09-30/paper.pdf

