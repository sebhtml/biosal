# Message API

In the actor model, everything is either an actor or a message.
This API describe what can be done with messages. The list below
only includes the functions that are useful within an actor.

# Header

```C
#include <engine/engine.h>
```

([engine/engine.h](../engine/engine.h))

# Functions

## bsal_message_init

```C
void bsal_message_init(struct bsal_message *message, int tag, int count, void *buffer);
```

Initialize a message.

## bsal_message_destroy

```C
void bsal_message_destroy(struct bsal_message *message);
```

Destroy a message.

## bsal_message_source

```C
int bsal_message_source(struct bsal_message *message);
```

Get the source actor.

## bsal_message_tag

```C
int bsal_message_tag(struct bsal_message *message);
```

Get tag.

## bsal_message_set_tag

```C
void bsal_message_set_tag(struct bsal_message *message, int tag);
```

Set tag.

## bsal_message_buffer

```C
void *bsal_message_buffer(struct bsal_message *message);
```

Get message buffer.

## bsal_message_set_buffer

```C
void bsal_message_set_buffer(struct bsal_message *message, void *buffer);
```

Set message buffer. This does not do any copy.

## bsal_message_count

```C
int bsal_message_count(struct bsal_message *message);
```

Get the number of bytes in the message buffer.

## bsal_message_set_count

```C
void bsal_message_set_count(struct bsal_message *message, int count);
```

Set buffer byte count.
