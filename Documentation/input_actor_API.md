# Input actor API

How to use: spawn an **bsal_input_actor** actor and send it messages
to interact with it in order to read sequence data.

## Header

```C
#include <input/input_actor.h>
```

## BSAL_INPUT_ACTOR_OPEN

```C
BSAL_INPUT_ACTOR_OPEN
```

Open an input file.

- Request message buffer: File path (char *)
- Responses:

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

```C
BSAL_INPUT_ACTOR_COUNT
```

Count entries

- Request message buffer: -
- Responses:

```C
BSAL_INPUT_ACTOR_COUNT_PROGRESS
```

- Condition: This is sent to notify supervisor about the progression.
- Response message buffer: entries (int)

```C
BSAL_INPUT_ACTOR_COUNT_RESULT
```

- Condition: Counting has finished
- Response message buffer: entries (int)

## BSAL_INPUT_ACTOR_CLOSE

```C
BSAL_INPUT_ACTOR_CLOSE
```
Close the file

- Request message buffer: -
- Responses: none

