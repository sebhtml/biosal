
#ifndef BSAL_H
#define BSAL_H

#define BSAL_VERSION "/dev/null"

/* engine */

#include "core/engine/actor.h"
#include "core/engine/message.h"
#include "core/engine/node.h"

/* actor patterns */

#include "core/patterns/manager.h"

/* helpers */

#include "core/helpers/actor_helper.h"
#include "core/helpers/vector_helper.h"
#include "core/helpers/message_helper.h"

/* system */

#include <core/system/memory.h>

/* structures */

#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/map.h>
#include <core/structures/map_iterator.h>
#include <core/structures/set.h>
#include <core/structures/set_iterator.h>


/* input */

#include "genomics/input/input_stream.h"
#include "genomics/input/input_controller.h"

/* storage */

#include "genomics/storage/sequence_store.h"

#endif
