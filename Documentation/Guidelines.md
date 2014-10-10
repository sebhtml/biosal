Here are some guidelines to creating actor applications with
Thorium. 

First, over-decomposition should be favored. An actor should do one thing
and do it well (this is similar to the UNIX philosophy).

Messages should be small, but not too small to avoid a low
computation-to-communication ratio.

The request-reply message pattern is needed frequently.



References

- Patterns for Overlapping Communication and Computation
    http://charm.cs.illinois.edu/newPapers/09-30/paper.pdf
