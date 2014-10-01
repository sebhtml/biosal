# Node API

The node API allows one to spawn the initial actors.
See this example: [main.c](../examples/remote_spawn/main.c)

# Functions

## bsal_node_init

```C
void bsal_node_init(struct bsal_node *node, int *argc, char ***argv);
```

Initialize a node.

## bsal_node_spawn

```C
int bsal_node_spawn(struct bsal_node *node, int script);
```

Spawn an actor. This is usually used to spawn the first actor of a node.
Actors spawned with this function will receive a message with tag ACTION_START.

## bsal_node_add_script

```C
void bsal_node_add_script(struct bsal_node *node, int name, struct bsal_script *script);
```

Add a script which describe the behavior of an actor (like in a movie).

## bsal_node_run

```C
void bsal_node_run(struct bsal_node *node);
```

Run a node.

## bsal_node_destroy

```C
void bsal_node_destroy(struct bsal_node *node);
```

Destroy a node
