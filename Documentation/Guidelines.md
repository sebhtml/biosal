Here are some guidelines to creating actor applications with
Thorium.

1. First, over-decomposition should be favored. An actor should do one thing
and do it well (this is similar to the UNIX philosophy).

2. Messages should be small, but not too small to avoid a low
computation-to-communication ratio. Buffer sizes can be adjusted
so that actor receive computations last longer than communications.

3. The request-reply message pattern is needed frequently to avoid
communication imbalance. Such an imbalance is also called
producer-consumer imbalance.

4. An actor can avoid running for too long by sending ACTION_YIELD to
itself.

5. Another trick to keep the "pipeline" busy is to have more than 1 in-flight
message for any destination (at the expense of memory usage).


----------

References

- Patterns for Overlapping Communication and Computation
    http://charm.cs.illinois.edu/newPapers/09-30/paper.pdf
