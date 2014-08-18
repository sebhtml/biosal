
#ifndef THORIUM_SCRIPT_H
#define THORIUM_SCRIPT_H

struct thorium_actor;
struct thorium_message;

typedef void (*thorium_actor_receive_fn_t)(struct thorium_actor *self, struct thorium_message *message);
typedef void (*thorium_actor_init_fn_t)(struct thorium_actor *self);
typedef void (*thorium_actor_destroy_fn_t)(struct thorium_actor *self);

/*
 * An actor script defines the behavior of an actor.
 */
struct thorium_script {
    int identifier;
    thorium_actor_init_fn_t init;
    thorium_actor_destroy_fn_t destroy;
    thorium_actor_receive_fn_t receive;
    int size;
    char *name;
    char *description;
    char *author;
    char *version;
};

void thorium_script_init(struct thorium_script *self, int identifier,
                thorium_actor_init_fn_t init,
                thorium_actor_destroy_fn_t destroy,
                thorium_actor_receive_fn_t receive,
                int size,
                char *name, char *description, char *author, char *version);
void thorium_script_destroy(struct thorium_script *self);

thorium_actor_init_fn_t thorium_script_get_init(struct thorium_script *self);
thorium_actor_destroy_fn_t thorium_script_get_destroy(struct thorium_script *self);
thorium_actor_receive_fn_t thorium_script_get_receive(struct thorium_script *self);

int thorium_script_identifier(struct thorium_script *self);
int thorium_script_size(struct thorium_script *self);
char *thorium_script_name(struct thorium_script *self);
char *thorium_script_description(struct thorium_script *self);
char *thorium_script_author(struct thorium_script *self);
char *thorium_script_version(struct thorium_script *self);

#endif
