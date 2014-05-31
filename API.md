# Notice

This API only lists the public interface elements.
The library has much more under its hood.

Also, this is in heavy development.

# Actor API

These functions (except **bsal_node_spawn**) can be called within an actor context (inside a receive function).
Actors spawned with **bsal_node_spawn** (initial actors) receive a message with tag **BSAL_ACTOR_START**.

## Header

```C
#include <engine/actor.h>
```

## BSAL_ACTOR_START

- Request message tag: BSAL_ACTOR_START
- Description: A message with this tag is sent to every actor present when the runtime system starts.
- Request message buffer: -

Responses: -

## BSAL_ACTOR_BARRIER_REPLY

- Request message tag: BSAL_ACTOR_BARRIER_REPLY
- Description: Notification of barrier progression.
- Request message buffer: -

Responses: -

## bsal_node_spawn

Spawn an actor from the outside, this is usually used to spawn the first actor of a node.
Actors spawned with this function will receive a message with tag BSAL_ACTOR_START.

```C
int bsal_node_spawn(struct bsal_node *node, void *pointer, struct bsal_actor_vtable *vtable);
```

## bsal_actor_name

Get actor name.

```C
int bsal_actor_name(struct bsal_actor *actor);
```

## bsal_actor_supervisor

Get supervisor name. The supervisor is the actor that spawned the current
actor.

```C
int bsal_actor_supervisor(struct bsal_actor *actor);
```

## bsal_actor_argc

Get command line argument count.

```C
int bsal_actor_argc(struct bsal_actor *actor);
```

## bsal_actor_argv

Get command line arguments

```C
char **bsal_actor_argv(struct bsal_actor *actor);
```

## bsal_actor_spawn

Spawn a new actor and return its name. The supervisor assigned to the newly spawned actor is the actor
that calls **bsal_actor_spawn**.

```C
int bsal_actor_spawn(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable);
```

## bsal_actor_send

Send a message to an actor.

```C
void bsal_actor_send(struct bsal_actor *actor, int destination, struct bsal_message *message);
```

## bsal_actor_send_range

Send a message to many actors in a range.

```C
void bsal_actor_send_range(struct bsal_actor *actor, int first, int last,
                struct bsal_message *message);
```

## bsal_actor_pointer

Get the implementation of an actor. This is useful when implementing new
actors.

```C
void *bsal_actor_pointer(struct bsal_actor *actor);
```

##  bsal_actor_die

Die.

```C
void bsal_actor_die(struct bsal_actor *actor);
```

## bsal_actor_nodes

Get number of nodes.

```C
int bsal_actor_nodes(struct bsal_actor *actor);
```

## bsal_actor_barrier

Begin a barrier.

```C
void bsal_actor_barrier(struct bsal_actor *actor, int first_actor, int last_actor);
```

## bsal_actor_barrier_completed

Verify is a barrier has completed. An actor is notified
of a barrier progression with a message with tag
BSAL_ACTOR_BARRIER_REPLY.

```C
int bsal_actor_barrier_completed(struct bsal_actor *actor);
```

## bsal_actor_pin

Pin an actor to an worker for memory affinity purposes.

```C
void bsal_actor_pin(struct bsal_actor *actor);
```

## bsal_actor_unpin

Unpin an actor.

```C
void bsal_actor_unpin(struct bsal_actor *actor);
```

# Input actor API

How to use: spawn an bsal_input_actor actor and send it messages
to interact with it in order to read sequence data.

## Header

```C
#include <input/input_actor.h>
```

## BSAL_INPUT_ACTOR_OPEN

- Request message tag:

```C
BSAL_INPUT_ACTOR_OPEN
```

- Description: Open an input file.
- Request message buffer: File path (char *)

Responses:

```C
BSAL_INPUT_ACTOR_OPEN_OK
```

- Condition: Successful
- Response message buffer: -

```C
BSAL_INPUT_ACTOR_OPEN_NOT_FOUND
```

- Condition: File not found
- Response message buffer: -

```C
BSAL_INPUT_ACTOR_OPEN_NOT_SUPPORTED
```

- Condition: Format not supported
- Response message buffer: -



## BSAL_INPUT_ACTOR_COUNT

- Request message tag: BSAL_INPUT_ACTOR_COUNT
- Description: Count entries
- Request message buffer: -

Responses:

- Response message tag: BSAL_INPUT_ACTOR_COUNT_PROGRESS
- Condition: This is sent to notify supervisor about the progression.
- Response message buffer: entries (int)

- Response message tag: BSAL_INPUT_ACTOR_COUNT_RESULT
- Condition: Counting has finished
- Response message buffer: entries (int)

## BSAL_INPUT_ACTOR_CLOSE

- Request message tag: BSAL_INPUT_ACTOR_CLOSE
- Description: Close the file
- Request message buffer: -

Responses: -
