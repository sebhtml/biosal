# Message API

In the actor model, everything is either an actor or a message.
This API describes what can be done with messages. The list below
only includes the functions that are useful within an actor.

# Header

```C
#include <engine/message.h>
```

([engine/message.h](../engine/message.h))

# Functions

## bsal_message_init

```C
void bsal_message_init(struct bsal_message *message, int action, int count, void *buffer);
```

Initialize a message. count is a number of bytes.

## bsal_message_source

```C
int bsal_message_source(struct bsal_message *message);
```

Get the source actor.

## bsal_message_action

```C
int bsal_message_action(struct bsal_message *message);
```

Get action.

## bsal_message_buffer

```C
void *bsal_message_buffer(struct bsal_message *message);
```

Get message buffer.

## bsal_message_count

```C
int bsal_message_count(struct bsal_message *message);
```

Get the number of bytes in the message buffer.

## bsal_message_destroy

```C
void bsal_message_destroy(struct bsal_message *message);
```

Destroy a message.


