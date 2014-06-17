
#ifndef BSAL_H
#define BSAL_H

#define BSAL_VERSION "/dev/null"

/* engine */

#include "engine/actor.h"
#include "engine/message.h"
#include "engine/node.h"


/* input */

#include "input/input_stream.h"
#include "input/input_controller.h"


/* storage */

#include "storage/sequence_store.h"


/* actor patterns */

#include "patterns/manager.h"


/* helpers */

#include "helpers/actor_helper.h"
#include "helpers/vector_helper.h"
#include "helpers/message_helper.h"


/* system */

#include <system/memory.h>


#endif
