
biosal is a distributed BIOlogical Sequence Actor Library.

biosal applications are written in the form of actors which send each other messages,
change their state, spawn other actors and eventually die of old age.
These actors run on top
of biosal runtime system called "The Thorium Engine" using the [API](Documentation/API.md). Thorium is a distributed actor machine.
Thorium uses script-wise symmetric actor placement and is targeted for high-performance computing.
Thorium is a general purpose actor model implementation.

# Exciting actor applications for genomics

| Name | Description |
| --- | --- |
| argonnite | k-mer counter |
| gc | Guanine-cytosine counter |
| spate | Exact, convenient, and scalable metagenome assembly and genome isolation for everyone (with emphasis on low-abundance species too) |

# Technologies

| Key | Value |
| --- | --- |
| Name | biosal |
| Computation model | actor model (Hewitt & Baker 1977) |
| Programming language | C99 (ISO/IEC 9899:1999) |
| Message passing | MPI 2.2 |
| Threads | Pthreads |
| License | [The BSD 2-Clause License](LICENSE.md) |

# Try it out

```bash
git clone https://github.com/GeneAssembly/biosal.git
cd biosal
make tests # run tests
make examples # run examples
```

- see [compilation options](Documentation/Compilation_Options.md)
- see [examples](examples) and [tests](tests)

# Branches

| Branch name | Person (alphabetical order) | Clone URL | Branch Build Status |
| --- | --- | --- | --- |
| [master](https://github.com/sebhtml/biosal/tree/master) | Sébastien Boisvert | [HTTPS](https://github.com/GeneAssembly/biosal.git) | unit-tests: [![Build Status](http://jenkins.biosal.anl.boisvert.info:8080/buildStatus/icon?job=biosal-clang-mpich-build)](http://jenkins.biosal.anl.boisvert.info:8080/job/biosal-clang-mpich-build/) all-tests (unit-tests, example-tests, application-tests): [![Build Status](http://jenkins.biosal.anl.boisvert.info:8080/buildStatus/icon?job=big-system-tests)](http://jenkins.biosal.anl.boisvert.info:8080/job/big-system-tests/) |
| [energy](https://github.com/sebhtml/biosal/tree/energy) | Sébastien Boisvert| [HTTPS](https://github.com/sebhtml/biosal.git) | |
| [pami](https://github.com/huyba/biosal/tree/master) | Huy Bui | [HTTPS](https://github.com/huyba/biosal.git) | |
| [granularity](https://github.com/gkthiruvathukal/biosal/tree/granularity) | George K. Thiruvathukal | [HTTPS](https://github.com/gkthiruvathukal/biosal.git) |[![Build Status](https://travis-ci.org/gkthiruvathukal/biosal.svg?branch=granularity)](https://travis-ci.org/gkthiruvathukal/biosal)  |
| [entropy](https://github.com/levinas/biosal/tree/entropy) | Fangfang Xia | [HTTPS](https://github.com/levinas/biosal.git) | |

There is also a read-only [mirror](https://github.com/GeneAssembly/biosal/tree/mirror).

The [**Integration Manager Workflow**](http://git-scm.com/about/distributed) is used.

# Product team
see [CREDITS.md](CREDITS.md)

# Community

- Issues: https://github.com/GeneAssembly/biosal/issues?state=open
- Website: https://github.com/GeneAssembly/biosal
- Mailing list: biosal@lists.cels.anl.gov https://lists.cels.anl.gov/mailman/listinfo/biosal
- Jenkins Continuous Integration: http://jenkins.biosal.anl.boisvert.info:8080

# Actor model

The actor model has actors and messages, mostly.

When an actor receives a message, it can ([Agha 1986](http://dl.acm.org/citation.cfm?id=7929), p. 12, 2.1.3):

- send a finite number of messages to other actors (**thorium_actor_send**);
- create a finite number of new actors (**thorium_actor_spawn**, **ACTION_SPAWN**);
- designate the behavior to be used for the next message it receives (**thorium_actor_concrete_actor**, **ACTION_STOP**).

Other names for the actor model: actors, virtual processors, activation frames, streams
([Hewitt, Bishop, Steiger 1973](http://dl.acm.org/citation.cfm?id=1624804)).

Also, in the actor model, the arrival order of messages is both arbitrary and unknown
([Agha 1986](http://dl.acm.org/citation.cfm?id=7929), p. 22, 2.4).

One of the most important requirements of actors is that of acquaintances. An actor can only send
message to one of its acquaintances.
Acquaintance vectors were introduced in ([Hewitt and Baker 1977](http://dspace.mit.edu/handle/1721.1/41962), p. 7,
                section III.3).
Any actor using an acquaintance vector is migratable by an actor machine.
An actor machine can distribute and balance actors according to some arbitrary rules wwhen
all the actors in an actor system use acquaintance vectors.

[Important actor model papers](Documentation/Actors.md)

## Actor model links

- [Why say Actor Model instead of message passing?](http://lambda-the-ultimate.org/node/4683)
- [Actors: a model of concurrent computation in distributed systems](http://dl.acm.org/citation.cfm?id=7929)
- [A universal modular ACTOR for malism for artificial intelligence](http://dl.acm.org/citation.cfm?id=1624804)
- [Actor model](http://en.wikipedia.org/wiki/Actor_model)

## Languages using actors

- [Erlang](http://www.erlang.org/)
- [Scala](http://www.scala-lang.org/)
- [D](http://dlang.org/)

# Design

Key concepts

| Concept | Description | Structure |
| --- | --- | --- |
| Message | Information with a source and a destination | struct thorium_message |
| Actor | Something that receives messages and behaves according to a script | struct thorium_actor |
| Actor mailbox | Messages for an actor are buffered there | struct core_fast_ring |
| Script | Describes the behavior of an actor ([Hewitt, Bishop, Steiger 1973](http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.77.7898))| struct thorium_script |
| Node | A runtime system that can be connected to other nodes (see [Erlang's definition](http://www.erlang.org/doc/reference_manual/distributed.html)) | struct thorium_node |
| Worker pool | A set of available workers inside a node | struct thorium_worker_pool |
| Worker | A instance that has a actor scheduling queue | struct thorium_worker |
| Scheduler | Each worker has a actor scheduling queue and an outbound message queue | struct thorium_scheduler |
| Transport | Each node has a transport subsystem for moving messages between nodes | struct thorium_transport |

# Implementation of the runtime system

The code has to be formulated in term of actors.
Actors are executes inside a controlled environment managed by the Thorium
engine, which is a runtime system.
An actor has a name, and does something when it receives a message.
A message is however first received by a node. The node
gives it to a worker_pool. The worker pool
assigns the actor to a worker. Finally, worker eventually calls
the corresponding receive function using the actor and message presented.

When an actor sends a message, the destination is either an actor on the
same node or an actor on another node. The runtime sends messages for actors
on other nodes with MPI. Otherwise, the message is prepared and given to
a actor directly on the same node.

## Runtime

Each biosal node is managed by an instance of the Thorium engine. These Thorium instances
speak to each other.
The number of nodes is set by mpiexec -n @number_of_thorium_nodes ./a.out.
The number of threads on each node is set with -threads-per-node.

The following command starts 256 nodes (there is 1 MPI rank per
node) and 64 threads per node for a total of
256 * 64 = 16384 threads.

```bash
mpiexec -n 256 ./a.out -threads-per-node 64
```

Because the whole thing is event-driven by inbound and outbound messages,
a single node can run much more actors than the number of
threads it has.

The runtime also supports asymmetric numbers of threads, but most platforms
have identical compute nodes (examples: IBM Blue Gene/Q, Cray XC30)
so this is not very useful. Example:

```bash
# launch 32 nodes with 32 threads, 244 threads, 32 threads, 244 threads, and so on
mpiexec -n 32 ./a.out -threads-per-node 32,244
```

## Design links

- [Linux workqueue](https://www.kernel.org/doc/Documentation/workqueue.txt)

# Other possible names

- name := biosal
- bio := Biological or Biology
- s := Sequence or Scalable or Salable or Salubrious or Satisfactory
- a := Analysis or Actor or Actors
- l := Library or Lift

Alternative name: BIOlogy Scalable Actor Library


# Tested platforms

- Cray XE6
- IBM Blue Gene/Q
- XPS 13 with Fedora
- Xeon E7-4830 with CentOS 6.5
- Apple Mavericks


