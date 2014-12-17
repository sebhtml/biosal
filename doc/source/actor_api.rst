Actor API
=========

All the functions below (except **thorium\_node\_spawn** which is used
to spawn initial actors) must be called within an actor context (inside
a **thorium\_actor\_receive\_fn\_t** function). Actors spawned with
**thorium\_node\_spawn** (initial actors) receive a message with action
**ACTION\_START** when the system starts.

When creating actors, the developer needs to provides 3 functions: init
(**thorium\_actor\_init\_fn\_t**), destroy
(**thorium\_actor\_destroy\_fn\_t**) and receive
(**thorium\_actor\_receive\_fn\_t**) (with a **struct
thorium\_script**). init is called when the actor is spawned, destroy is
called when **ACTION\_STOP** is received, and receive is called whenever
a message is received.

Custom actor example: `buddy.h <../examples/mock/buddy.h>`__
`buddy.c <../examples/mock/buddy.c>`__

Header
--------

.. code-block:: c

    #include <engine/actor.h>

(`engine/actor.h <../engine/actor.h>`__)

Message actions
--------------------

.. code-block:: c

    ACTION_START

A message with this action is sent to every actor present when the
runtime system starts.

-  Request message buffer: not application, this is a received message
-  Responses: none

.. code-block:: c

    ACTION_SPAWN

Spawn a remote actor. `Example <../examples/remote_spawn/table.c>`__

-  Request message buffer: script name
-  Responses:

.. code-block:: c

    ACTION_STOP

Stop actor. This message action can only be sent to an actor by itself.

-  Request message buffer: empty
-  Responses: none

.. code-block:: c

    ACTION_PIN

Pin an actor. Can only be sent to an actor by itself.

-  Request message buffer: empty
-  Responses: none

.. code-block:: c

    ACTION_UNPIN

Unpin an actor. Can only be sent to an actor by itself.

-  Request message buffer: empty
-  Responses: none

.. code-block:: c

    ACTION_SYNCHRONIZE


The source started a synchronization. To accept the synchronization, the
reply ACTION\_SYNCHRONIZE\_REPLY must be sent.

.. code-block:: c

    ACTION_SYNCHRONIZED

Notification of completed synchronization (started with
**thorium\_actor\_synchronize**).

-  Request message buffer: not application, this is a received message
-  Responses: none

Functions
--------------

.. code-block:: c

    int thorium_actor_spawn(struct thorium_actor *actor, int script);

Spawn a new actor and return its name. The supervisor assigned to the
newly spawned actor is the actor that calls **thorium\_actor\_spawn**.

.. code-block:: c

    void thorium_actor_send(struct thorium_actor *actor, int destination, struct thorium_message *message);

Send a message to an actor.

.. code-block:: c

    void *thorium_actor_concrete_actor(struct thorium_actor *actor);

Get the state of an actor. This is used when implementing new actors.


.. code-block:: c

    int thorium_actor_name(struct thorium_actor *actor);

Get actor name.


.. code-block:: c

    int thorium_actor_supervisor(struct thorium_actor *actor);

Get supervisor name. The supervisor is the actor that spawned the
current actor.

.. code-block:: c

    int thorium_actor_argc(struct thorium_actor *actor);

Get command line argument count.


.. code-block:: c

    char **thorium_actor_argv(struct thorium_actor *actor);

Get command line arguments


.. code-block:: c

    void thorium_actor_send_range(struct thorium_actor *actor, int first, int last, struct thorium_message *message);

Send a message to many actors in a range. The implementation uses a
binomial-tree algorithm.


.. code-block:: c

    void thorium_actor_synchronize(struct thorium_actor *actor, int first_actor, int last_actor);

Begin a synchronization. A binomial-tree algorithm is used. A message
with action ACTION\_SYNCHRONIZED is received when the synchronization
has completed.
