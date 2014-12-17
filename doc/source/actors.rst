Actors
========

About the Actor Model
---------------------------

The theoretical actor model of computation uses actors and (active)
messages to encode any computation . The actor model
of computation introduced by Hewitt, Bishop and Steiger in
1973 [#HewittBishopSteigler]_ contains the concept of active messages
(behavior defined for a given message), in addition to actors which
embody both data (state) and behavior.

To simplify, an actor can be thought of as an event-driven object that
can receive messages. When receiving a message, the actor reacts to it
by doing one of the following actions (or a mix of
them) [#Agha1986Actors]_:

-   send a finite number of messages to actors (possibly to itself);

-   spawn a finite number of new actors (create actors);

-   set a replacement behavior (designate the behavior to be used for
    the next received message).

An actor has a name (similar to an object reference in object-oriented
programming (OOP) where the object could be anywhere on a network) and
in order to send a message to an other actor, the actor needs to be
acquainted (it needs to know – or to reference – the name of the other
actor). The behavior of an actor is specified in an actor
script (conceptually related to a class with bound methods in
object-oriented programming but not requiring all traditional OOP
features, e.g. inheritance). The actor model is therefore a theoretical
model of computation that provides an abstraction for computation and
communication (active messages) and an abstraction for data (actors).

In genomics, encapsulation (using objects/actors) is important for
robustness of the deliverable and productivity of the programmer whereas
message passing (typically using Message Passing Interface or MPI) has
proven to be one of the best abstraction to reach scalability and
portability.

With the actor model, messages are not ordered (point-to-point messages
are ordered in MPI). As such, control messages can be used to control
the nondeterminism in order to produce deterministic results with a
nondeterministic (the order of events) actor computation. Such control
structures can thus be interpreted as message passing idioms and
patterns [#Hewitt1977]_ [#Christopher1989]_, such as Request-Reply (today
known as client/server), distributed array processing, communicating
sequential processes, remote procedure calls (a specific implementation
of Request-Reply), broadcast and accumulation (collective
communication), data flow graphs [#Thiruvathukal1991]_, and dynamic
programming (among others).


orem ipsum [#f1]_ dolor sit amet ... [#f2]_

.. rubric:: Footnotes

.. [#HewittBishopSteigler] Hewitt, C., Bishop, P. & Steiger, R. A universal modular ACTOR formalism for artificial intelligence. In Proceedings of the 3rd international joint conference on Artificial intelligence, IJCAI'73, 235-245 (Morgan Kaufmann Publishers Inc., San Francisco, CA, USA, 1973). URL http://portal.acm.org/citation.cfm?id=1624804; http://ijcai.org/Past%20Proceedings/IJCAI-73/PDF/027B.pdf

.. [#Hewitt1977] Hewitt, C. & Baker, H. Laws for communicating parallel processes. Tech. Rep., MIT Artificial Intelligence Laboratory (1977). URL http://dspace.mit.edu/handle/1721.1/41962; http://dspace.mit.edu/bitstream/handle/1721.1/41962/AI\_WP\_134A.pdf

.. [#Christopher1989] paper

.. [#Thiruvathukal1991] paper

.. [#Agha1986Actors] Agha, G. Actors: A Model of Concurrent Computation in Distributed Systems (MIT Press, Cambridge, MA, USA, 1986). URL http://dl.acm.org/citation.cfm?id=7929.


.. todo::

   Everything after this line to be deleted after updating references to be inline (footnote) style.

References
------------------

- Hewitt, C., Bishop, P. & Steiger, R. A universal modular ACTOR formalism for artificial intelligence. In Proceedings of the 3rd international joint conference on Artificial intelligence, IJCAI'73, 235-245 (Morgan Kaufmann Publishers Inc., San Francisco, CA, USA, 1973). URL http://portal.acm.org/citation.cfm?id=1624804; http://ijcai.org/Past%20Proceedings/IJCAI-73/PDF/027B.pdf

- Hewitt, C. & Baker, H. Laws for communicating parallel processes. Tech. Rep., MIT Artificial Intelligence Laboratory (1977). URL http://dspace.mit.edu/handle/1721.1/41962; http://dspace.mit.edu/bitstream/handle/1721.1/41962/AI\_WP\_134A.pdf

- Agha, G. Actors: A Model of Concurrent Computation in Distributed Systems (MIT Press, Cambridge, MA, USA, 1986). URL http://dl.acm.org/citation.cfm?id=7929.

- Armstrong, J. Making reliable distributed systems in the presence of software errors (2003). URL http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.3.408; http://www.erlang.org/download/armstrong\_thesis\_2003.pdf.

Other Readings
----------------------

- Why has the actor model not succeeded? http://www.doc.ic.ac.uk/~nd/surprise\_97/journal/vol2/pjm2/
