
struct thorium_actor;
struct thorium_message;

void thorium_actor_pack_proxy_message(struct thorium_actor *self,
                struct thorium_message *message, int real_source);
void thorium_actor_unpack_proxy_message(struct thorium_actor *self,
                struct thorium_message *message);


