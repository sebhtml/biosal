# Node API

The node API allows one to spawn the initial actors.
See this example: [main.c](../examples/remote_spawn/main.c)

# Functions

## thorium_node_init

```C
void thorium_node_init(struct thorium_node *node, int *argc, char ***argv);
```

Initialize a node.

## thorium_node_spawn

```C
int thorium_node_spawn(struct thorium_node *node, int script);
```

Spawn an actor. This is usually used to spawn the first actor of a node.
Actors spawned with this function will receive a message with tag ACTION_START.

## thorium_node_add_script

```C
void thorium_node_add_script(struct thorium_node *node, int name, struct thorium_script *script);
```

Add a script which describe the behavior of an actor (like in a movie).

## thorium_node_run

```C
void thorium_node_run(struct thorium_node *node);
```

Run a node.

## thorium_node_destroy

```C
void thorium_node_destroy(struct thorium_node *node);
```

Destroy a node
