# Actor API

These functions (except **bsal_node_spawn**) can be called within an actor context (inside a receive function).
Actors spawned with **bsal_node_spawn** (initial actors) receive a message with tag **BSAL_ACTOR_START**.

## Header

```C
#include <engine/actor.h>
```

## BSAL_ACTOR_START

```C
BSAL_ACTOR_START
```

A message with this tag is sent to every actor present when the runtime system starts.
- Request message buffer: -

Responses: -

## BSAL_ACTOR_PIN

```C
BSAL_ACTOR_PIN
```

Pin an actor to an worker for memory affinity purposes. Can only be sent to an actor by itself.
- Request message buffer: empty

Responses: none

## BSAL_ACTOR_UNPIN

```C
BSAL_ACTOR_UNPIN
```

Unpin an actor. Can only be sent to an actor by itself.
- Request message buffer: empty

Responses: none

## bsal_node_spawn

```C
int bsal_node_spawn(struct bsal_node *node, void *pointer, struct bsal_actor_vtable *vtable);
```

Spawn an actor from the outside, this is usually used to spawn the first actor of a node.
Actors spawned with this function will receive a message with tag BSAL_ACTOR_START.

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

## bsal_actor_spawn

```C
int bsal_actor_spawn(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable);
```
Spawn a new actor and return its name. The supervisor assigned to the newly spawned actor is the actor
that calls **bsal_actor_spawn**.


## bsal_actor_send

```C
void bsal_actor_send(struct bsal_actor *actor, int destination, struct bsal_message *message);
```

Send a message to an actor.

## bsal_actor_send_range

```C
void bsal_actor_send_range(struct bsal_actor *actor, int first, int last, struct bsal_message *message);
```

Send a message to many actors in a range. The implementation uses
a binomial-tree algorithm.

## bsal_actor_pointer

```C
void *bsal_actor_pointer(struct bsal_actor *actor);
```

Get the implementation of an actor. This is useful when implementing new
actors.

##  bsal_actor_die

```C
void bsal_actor_die(struct bsal_actor *actor);
```

Die.

## bsal_actor_nodes

```C
int bsal_actor_nodes(struct bsal_actor *actor);
```

Get number of nodes.

## bsal_actor_synchronize

```C
void bsal_actor_synchronize(struct bsal_actor *actor, int first_actor, int last_actor);
```

Begin a synchronization. A binomial-tree algorithm is used.
A message with tag BSAL_ACTOR_SYNCHRONIZED is received when the
synchronization has completed.

