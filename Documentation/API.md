This API only lists the public interface elements.
The library has much more under its hood.

Also, this is in heavy development.

# Main API functions

Send a message

```C
/*
 * Send function
 */
void thorium_actor_send(struct thorium_actor *self, int destination, struct thorium_message *message);


/*
 * \return This function returns the name of the spawned actor.
 */
int thorium_actor_spawn(struct thorium_actor *self, int script);


/*
 * Get the current actor name.
 */
int thorium_actor_name(struct thorium_actor *self);

/*
 * Get the concrete actor (as a void *) from the abstract actor (struct thorium_actor *
 */
void *thorium_actor_concrete_actor(struct thorium_actor *self);

```

# API sections

- [Actor API](actor_API.md)
- [Message API](message_API.md)
- [Node API](node_API.md)
- [Input API](input_API.md)

# Examples

- [Examples](../examples)
