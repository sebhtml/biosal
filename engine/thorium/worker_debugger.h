
#ifndef WORKER_DEBUGGER_H
#define WORKER_DEBUGGER_H

struct thorium_balancer;
struct thorium_worker;

void thorium_worker_print_actors(struct thorium_worker *self, struct thorium_balancer *scheduler);

#endif
