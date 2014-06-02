## bsal_node_spawn

```C
int bsal_node_spawn(struct bsal_node *node, int script);
```

Spawn an actor from the outside, this is usually used to spawn the first actor of a node.
Actors spawned with this function will receive a message with tag BSAL_ACTOR_START.

##

```C
void bsal_node_add_script(struct bsal_node *node, int script, struct bsal_script *script);
```
