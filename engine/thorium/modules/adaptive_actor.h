
#ifndef ADAPTIVE_ACTOR_H
#define ADAPTIVE_ACTOR_H

#include <stdint.h>

struct thorium_actor;

#define DEFINE_FLAG(value) \
        (1 << value)

#define THORIUM_ADAPTATION_FLAG_SMALL_MESSAGES     DEFINE_FLAG(0)
#define THORIUM_ADAPTATION_FLAG_SCOPE_WORKER       DEFINE_FLAG(1)
#define THORIUM_ADAPTATION_FLAG_SCOPE_NODE         DEFINE_FLAG(2)

/*
 * With THORIUM_ADAPTATION_FLAG_SMALL_MESSAGES | THORIUM_ADAPTATION_FLAG_SCOPE_WORKER
 * ==================================================================================
 *
 * Any given message has a destination actor. Such an actor has a destination
 * node.
 *
 * With W workers per node, A actors per worker, and N nodes, there are
 * W*A local actors on each node and typically there are at most
 * (W*A)/N messages sent to a given destination at any given time assuming
 * a strict request-reply scheme.
 *
 * With N=256, A=100, W=22 (Cray XE6 or Cray XC30):
 * irb(main):007:0> 256.0 / (22 * 100)
 * => 0.11636363636363636
 *
 * With N=1024, A=100, W=15 (IBM Blue Gene/Q):
 * irb(main):008:0> 1024 / (15 * 100.0)
 * => 0.6826666666666666
 *
 * Ideally, the ratio should be around 10% from experience.
 *
 * Equation:
 *
 * A: actors per worker
 * W: workers per node
 * N: nodes per job
 *
 * R = N / (W * A)
 *
 * A = N / (W * R)
 *
 * R        N       W       A           Total worker count
 * -------------------------------------------------------
 * 0.10     4       7       5                         28
 * 0.10     256     22      116                     5632
 * 0.10     512     15      341                     7680
 * 0.10     1024    15      682                    15360
 * 0.10     2048    15      1365                   30720
 */
int thorium_actor_get_suggested_actor_count(struct thorium_actor *self,
                uint32_t flags);


#endif
