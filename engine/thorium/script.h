
#ifndef BSAL_SCRIPT_H
#define BSAL_SCRIPT_H

struct bsal_actor;
struct bsal_message;

typedef void (*bsal_actor_receive_fn_t)(struct bsal_actor *self, struct bsal_message *message);
typedef void (*bsal_actor_init_fn_t)(struct bsal_actor *self);
typedef void (*bsal_actor_destroy_fn_t)(struct bsal_actor *self);

/*
 * An actor script defines the behavior of an actor.
 */
struct bsal_script {
    int identifier;
    bsal_actor_init_fn_t init;
    bsal_actor_destroy_fn_t destroy;
    bsal_actor_receive_fn_t receive;
    int size;
    char *name;
    char *description;
    char *author;
    char *version;
};

void bsal_script_init(struct bsal_script *self, int identifier,
                bsal_actor_init_fn_t init,
                bsal_actor_destroy_fn_t destroy,
                bsal_actor_receive_fn_t receive,
                int size,
                char *name, char *description, char *author, char *version);
void bsal_script_destroy(struct bsal_script *self);

bsal_actor_init_fn_t bsal_script_get_init(struct bsal_script *self);
bsal_actor_destroy_fn_t bsal_script_get_destroy(struct bsal_script *self);
bsal_actor_receive_fn_t bsal_script_get_receive(struct bsal_script *self);

int bsal_script_identifier(struct bsal_script *self);
int bsal_script_size(struct bsal_script *self);
char *bsal_script_name(struct bsal_script *self);
char *bsal_script_description(struct bsal_script *self);
char *bsal_script_author(struct bsal_script *self);
char *bsal_script_version(struct bsal_script *self);

#endif
