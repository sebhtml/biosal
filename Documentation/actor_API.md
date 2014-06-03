# Actor API

All the functions below (except **bsal_node_spawn** which is used
                to spawn initial actors) must be called within an actor context (inside a
**bsal_actor_receive_fn_t** function).
Actors spawned with **bsal_node_spawn** (initial actors) receive a message with tag **BSAL_ACTOR_START**
when the system starts.

When creating actors, the developer needs to provides 3 functions: init
(**bsal_actor_init_fn_t**), destroy (**bsal_actor_destroy_fn_t**) and receive
(**bsal_actor_receive_fn_t**)
(with a **struct bsal_script**). init is called when the actor is spawned, destroy is called
when **BSAL_ACTOR_STOP** is received, and receive is called whenever a message is received.

Custom actor example: [buddy.h](../examples/mock/buddy.h) [buddy.c](../examples/mock/buddy.c)

# Header

```C
#include <engine/actor.h>
```

([engine/actor.h](../engine/actor.h))

# Message tags

## BSAL_ACTOR_START

```C
BSAL_ACTOR_START
```

A message with this tag is sent to every actor present when the runtime system starts.

- Request message buffer: not application, this is a received message
- Responses: none

## BSAL_ACTOR_SPAWN

```C
BSAL_ACTOR_SPAWN
```

Spawn a remote actor. [Example](../examples/remote_spawn/table.c)

- Request message buffer: script name
- Responses:

```C
BSAL_ACTOR_SPAWN_REPLY
```

Spawn an actor remotely
- Response message buffer: actor name

## BSAL_ACTOR_STOP

```C
BSAL_ACTOR_STOP
```

Stop actor. This message tag can only be sent to an actor by
itself.

- Request message buffer: empty
- Responses: none

## BSAL_ACTOR_PIN

```C
BSAL_ACTOR_PIN
```

Pin an actor. Can only be sent to an actor by itself.

- Request message buffer: empty
- Responses: none

## BSAL_ACTOR_UNPIN

```C
BSAL_ACTOR_UNPIN
```

Unpin an actor. Can only be sent to an actor by itself.

- Request message buffer: empty
- Responses: none

## BSAL_ACTOR_SYNCHRONIZED

```C
BSAL_ACTOR_SYNCHRONIZED
```

Notification of completed synchronization (started with **bsal_actor_synchronize**).

- Request message buffer: not application, this is a received message
- Responses: none

# Functions

## bsal_actor_spawn

```C
int bsal_actor_spawn(struct bsal_actor *actor, int script);
```
Spawn a new actor and return its name. The supervisor assigned to the newly spawned actor is the actor
that calls **bsal_actor_spawn**.


## bsal_actor_send

```C
void bsal_actor_send(struct bsal_actor *actor, int destination, struct bsal_message *message);
```

Send a message to an actor.

## bsal_actor_concrete_actor

```C
void *bsal_actor_concrete_actor(struct bsal_actor *actor);
```

Get the state of an actor. This is used when implementing new
actors.

## bsal_actor_nodes

```C
int bsal_actor_nodes(struct bsal_actor *actor);
```

Get number of nodes.

## bsal_actor_name

```C
int bsal_actor_name(struct bsal_actor *actor);
```

Get actor name.

## bsal_actor_supervisor

```C
int bsal_actor_supervisor(struct bsal_actor *actor);
```

Get supervisor name. The supervisor is the actor that spawned the current
actor.

## bsal_actor_argc

```C
int bsal_actor_argc(struct bsal_actor *actor);
```

Get command line argument count.

## bsal_actor_argv

```C
char **bsal_actor_argv(struct bsal_actor *actor);
```

Get command line arguments

## bsal_actor_send_range

```C
void bsal_actor_send_range(struct bsal_actor *actor, int first, int last, struct bsal_message *message);
```

Send a message to many actors in a range. The implementation uses
a binomial-tree algorithm.

## bsal_actor_synchronize

```C
void bsal_actor_synchronize(struct bsal_actor *actor, int first_actor, int last_actor);
```

Begin a synchronization. A binomial-tree algorithm is used.
A message with tag BSAL_ACTOR_SYNCHRONIZED is received when the
synchronization has completed.

