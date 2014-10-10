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

## thorium_message_init

```C
void thorium_message_init(struct thorium_message *message, int action, int count, void *buffer);
```

Initialize a message. count is a number of bytes.

## thorium_message_source

```C
int thorium_message_source(struct thorium_message *message);
```

Get the source actor.

## thorium_message_action

```C
int thorium_message_action(struct thorium_message *message);
```

Get action.

## thorium_message_buffer

```C
void *thorium_message_buffer(struct thorium_message *message);
```

Get message buffer.

## thorium_message_count

```C
int thorium_message_count(struct thorium_message *message);
```

Get the number of bytes in the message buffer.

## thorium_message_destroy

```C
void thorium_message_destroy(struct thorium_message *message);
```

Destroy a message.


