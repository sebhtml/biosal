# Actor API

All the functions below (except **thorium_node_spawn** which is used
                to spawn initial actors) must be called within an actor context (inside a
**thorium_actor_receive_fn_t** function).
Actors spawned with **thorium_node_spawn** (initial actors) receive a message with action **ACTION_START**
when the system starts.

When creating actors, the developer needs to provides 3 functions: init
(**thorium_actor_init_fn_t**), destroy (**thorium_actor_destroy_fn_t**) and receive
(**thorium_actor_receive_fn_t**)
(with a **struct thorium_script**). init is called when the actor is spawned, destroy is called
when **ACTION_STOP** is received, and receive is called whenever a message is received.

Custom actor example: [buddy.h](../examples/mock/buddy.h) [buddy.c](../examples/mock/buddy.c)

# Header

```C
#include <engine/actor.h>
```

([engine/actor.h](../engine/actor.h))

# Message actions

## ACTION_START

```C
ACTION_START
```

A message with this action is sent to every actor present when the runtime system starts.

- Request message buffer: not application, this is a received message
- Responses: none

## ACTION_SPAWN

```C
ACTION_SPAWN
```

Spawn a remote actor. [Example](../examples/remote_spawn/table.c)

- Request message buffer: script name
- Responses:

```C
ACTION_SPAWN_REPLY
```

Spawn an actor remotely
- Response message buffer: actor name

## ACTION_STOP

```C
ACTION_STOP
```

Stop actor. This message action can only be sent to an actor by
itself.

- Request message buffer: empty
- Responses: none

## ACTION_PIN

```C
ACTION_PIN
```

Pin an actor. Can only be sent to an actor by itself.

- Request message buffer: empty
- Responses: none

## ACTION_UNPIN

```C
ACTION_UNPIN
```

Unpin an actor. Can only be sent to an actor by itself.

- Request message buffer: empty
- Responses: none

## ACTION_SYNCHRONIZE

The source started a synchronization. To accept the synchronization,
the reply ACTION_SYNCHRONIZE_REPLY must be sent.

## ACTION_SYNCHRONIZED

```C
ACTION_SYNCHRONIZED
```

Notification of completed synchronization (started with **thorium_actor_synchronize**).

- Request message buffer: not application, this is a received message
- Responses: none

# Functions

## thorium_actor_spawn

```C
int thorium_actor_spawn(struct thorium_actor *actor, int script);
```
Spawn a new actor and return its name. The supervisor assigned to the newly spawned actor is the actor
that calls **thorium_actor_spawn**.


## thorium_actor_send

```C
void thorium_actor_send(struct thorium_actor *actor, int destination, struct thorium_message *message);
```

Send a message to an actor.

## thorium_actor_concrete_actor

```C
void *thorium_actor_concrete_actor(struct thorium_actor *actor);
```

Get the state of an actor. This is used when implementing new
actors.


## thorium_actor_name

```C
int thorium_actor_name(struct thorium_actor *actor);
```

Get actor name.

## thorium_actor_supervisor

```C
int thorium_actor_supervisor(struct thorium_actor *actor);
```

Get supervisor name. The supervisor is the actor that spawned the current
actor.

## thorium_actor_argc

```C
int thorium_actor_argc(struct thorium_actor *actor);
```

Get command line argument count.

## thorium_actor_argv

```C
char **thorium_actor_argv(struct thorium_actor *actor);
```

Get command line arguments

## thorium_actor_send_range

```C
void thorium_actor_send_range(struct thorium_actor *actor, int first, int last, struct thorium_message *message);
```

Send a message to many actors in a range. The implementation uses
a binomial-tree algorithm.

## thorium_actor_synchronize

```C
void thorium_actor_synchronize(struct thorium_actor *actor, int first_actor, int last_actor);
```

Begin a synchronization. A binomial-tree algorithm is used.
A message with action ACTION_SYNCHRONIZED is received when the
synchronization has completed.

