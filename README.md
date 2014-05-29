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



Key concepts
| Concept | Description | Structure |
| --- | --- | --- |
| Message | Information with a source and a destination | struct bsal_message |
| Actor | Something that receives messages | struct bsal_actor |
| Work | 2-tuple with an actor and a message | struct bsal_work |
| Node | A runtime system that can be connected to other nodes (see [Erlang's definition](http://www.erlang.org/doc/reference_manual/distributed.html) | struct bsal_node |
| Worker | An object that performs work for a living | bsal_worker_thread |
| Queue | Each worker has a work queue and a message queue | struct bsal_fifo |
| Worker Pool | A set of available workers inside a node | struct bsal_worker_pool |


## Workflow

The code has to be formulated in term of actors.
An actor has a name, and does something when it receives a message.
A message is however first received by a node. The node
prepares a work and gives it to a worker_pool. The worker pool
assigns the work to a worker. Finally, worker eventually calls
the corresponding receive function using the actor and message presented inside
the work.

When an actor receives a message, it can:

- send messages (bsal_actor_send);
- spawn actors (bsal_actor_spawn);
- 


- send a finite number of messages to other actors (bsal_actor_send);
- create a finite number of new actors (bnsal_actor_spawn);
- designate the behavior to be used for the next message it receives (bsal_actor_actor, bsal_actor_die)

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
| --- |
| bsal_node_spawn | Spawn an actor from the outside,  this is usually used to spawn the first actor of a node |
| bsal_actor_spawn | Spawn a new actor and return its name |
| bsal_actor_send | Send a message |
| bsal_actor_die | Die |
| bsal_actor_nodes | Get number of nodes |
| bsal_actor_pin | Pin the actor to an actor for memory affinity purposes |

# Tests

see examples/ and tests/

# Authors

see 'git log'
