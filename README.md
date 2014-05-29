biosal is a distributed BIOlogical Sequence Analysis Library.

This is a work in progress.

# Technologies

- Name: biosal
- Language: C99 (ISO/IEC 9899:1999)
- Model: actor model
- Technologies: MPI, Pthreads (IEEE Std 1003.1c - 1995)

# Branches

Branch | Browse | HTTPS | SSH
--- | --- | --- | ---
 master | https://github.com/GeneAssembly/biosal/tree/master | https://github.com/GeneAssembly/biosal.git | git@github.com:GeneAssembly/biosal.git
 entropy (Fangfang's development branch) | https://github.com/levinas/biosal/tree/entropy | https://github.com/levinas/biosal.git | git@github.com:levinas/biosal.git
 energy (Seb's development branch) | https://github.com/sebhtml/biosal/tree/energy | https://github.com/sebhtml/biosal.git | git@github.com:sebhtml/biosal.git

# License (see LICENSE.md)

[The BSD 2-Clause License](http://opensource.org/licenses/BSD-2-Clause)

# Tickets

https://github.com/GeneAssembly/biosal/issues?state=open

# Actor model links

- [Why say Actor Model instead of message passing?](http://lambda-the-ultimate.org/node/4683)
- [Actors: a model of concurrent computation in distributed systems](http://dl.acm.org/citation.cfm?id=7929)
- [A universal modular ACTOR for malism for artificial intelligence](http://dl.acm.org/citation.cfm?id=1624804)
- [Actor model](http://en.wikipedia.org/wiki/Actor_model)

# Languages using actors

- [Erlang](http://www.erlang.org/)
- [Scala](http://www.scala-lang.org/)
- [D](http://dlang.org/)

# Design

| Concept | Description | Structure |
---
| Message | Information with a source and a destination | struct bsal_message |
---
| Actor | Something that receives messages | struct bsal_actor |
---
| Work | 2-tuple with an Actor and a Message | struct bsal_work |
---
| Node | A runtime system that can be connected to other Node instances (see [Erlang's definition http://www.erlang.org/doc/reference_manual/distributed.html]) | struct bsal_node |
---
| Worker | An object that performs work for a living | bsal_worker_thread |
---
| Queue | Each Worker has a Work Queue and a Message Queue | bsal_fifo |


Work to be done has to be formulated in term of actors (bsal_actor).
A bsal_actor has a name, do something when it receives a bsal_message.
A bsal_message is however first received by a bsal_node. The bsal_node
prepares a bsal_work and gives it to a bsal_worker_pool. The bsal_worker_pool
assigns the bsal_work to a bsal_worker_thread. Finally,  the bsal_worker_thread calls
receive using the bsal_actor and bsal_message presented inside the bsal_work.

- [Linux workqueue https://www.kernel.org/doc/Documentation/workqueue.txt]

# Runtime

The number of bsal_node nodes is set by mpiexec -n @number_of_bsal_nodes
The number of bsal_worker_thread objects on each bsal_node is set with
-workers-by-node

The following command starts 256 bsal_node node nodes (there is 1 MPI rank per
bsal_node) and 64 bsal_worker_thread workers per bsal_node for a total of
256 * 64 = 16384 distributed workers.

Because the whole thing is event - driven by inbound and outbound messages,
a single bsal_node can run much more bsal_actor actors than the number of
bsal_worker_thread workers.

# Application programming interface

Functions that can be called within an actor context
| Function | Description |
| bsal_actor_spawn | Spawn a new actor and return its name |
| bsal_actor_send | Send a message |
| bsal_actor_die | Die |

# Tests

see examples/ and tests/
