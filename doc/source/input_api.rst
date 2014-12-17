Input actor API
===============

How to use: spawn an actor with script BSAL\_INPUT\_STREAM\_SCRIPT and
send it messages to interact with it in order to read sequence data.

Header
------

.. code:: C

    #include <input/input_stream.h>

(`input/input\_stream.h <../input/input_stream.h>`__)

Message tags
============

BSAL\_INPUT\_OPEN
-----------------

.. code:: C

    BSAL_INPUT_OPEN

Open an input file.

-  Request message buffer: File path (char \*)
-  Responses:

.. code:: C

    BSAL_INPUT_OPEN_OK

Successful - Response message buffer: empty

.. code:: C

    BSAL_INPUT_ERROR_FILE_NOT_FOUND

File not found - Response message buffer: empty

.. code:: C

    BSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED

Format not supported - Response message buffer: empty

BSAL\_INPUT\_COUNT
------------------

.. code:: C

    BSAL_INPUT_COUNT

Count entries

-  Request message buffer: empty
-  Responses:

.. code:: C

    BSAL_INPUT_COUNT_PROGRESS

This is sent to notify supervisor about the progression. - Response
message buffer: entries (int)

.. code:: C

    BSAL_INPUT_COUNT_RESULT

Counting has finished - Response message buffer: entries (int)

.. code:: C

    BSAL_INPUT_ERROR_NOT_OPEN

the actor is not open - Respons message buffer: empty

BSAL\_INPUT\_GET\_SEQUENCE
--------------------------

.. code:: C

    BSAL_INPUT_GET_SEQUENCE

Read a sequence.

-  Request message buffer: empty
-  Responses:

.. code:: C

    BSAL_INPUT_GET_SEQUENCE_REPLY

The actor is open and there is a sequence to read - Response message
buffer: sequence number (int), sequence (char \*)

.. code:: C

    BSAL_INPUT_GET_SEQUENCE_END

there is nothing more to read - Response message buffer: empty

.. code:: C

    BSAL_INPUT_ERROR_NOT_OPEN

the actor is not open - Respons message buffer: empty

BSAL\_INPUT\_CLOSE
------------------

.. code:: C

    BSAL_INPUT_CLOSE

Close the file

-  Request message buffer: empty
-  Responses:

.. code:: C

    BSAL_INPUT_CLOSE_OK

successfully closed - Respons message buffer: empty

.. code:: C

    BSAL_INPUT_ERROR_NOT_OPEN

the actor is not open - Respons message buffer: empty
