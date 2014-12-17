Message API
===========

In the actor model, everything is either an actor or a message. This API
describes what can be done with messages. The list below only includes
the functions that are useful within an actor.

Header
--------

.. code-block:: c

    #include <engine/message.h>

(`engine/message.h <../engine/message.h>`__)

Functions
------------

thorium\_message\_init
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

    void thorium_message_init(struct thorium_message *message, int action, int count, void *buffer);

Initialize a message. count is a number of bytes.

thorium\_message\_source
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

    int thorium_message_source(struct thorium_message *message);

Get the source actor.

thorium\_message\_action
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

    int thorium_message_action(struct thorium_message *message);

Get action.

thorium\_message\_buffer
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

    void *thorium_message_buffer(struct thorium_message *message);

Get message buffer.

thorium\_message\_count
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

    int thorium_message_count(struct thorium_message *message);

Get the number of bytes in the message buffer.

thorium\_message\_destroy
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

    void thorium_message_destroy(struct thorium_message *message);

Destroy a message.
