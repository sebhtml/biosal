An Example
=============

Writing Thorium scripts is similar to writing object-oriented code. Although our framework is C-based, Thorium allows you to write your code in a modular way by separating the interfaces from the implementation.

As an example, let's consider how we'd write a simple, yet familiar, program: Hello. World.

You can study this code by looking in the folder ``biosal/examples/hello``.


As a first step, let's take a look at ``hello.h``:

.. literalinclude:: ../../examples/hello_world/hello.h
   :language: c

Let's take a look at each of the fragments that are essential in Thorium programming. (We will not focus our energy here on C basics, which are best addressed by other resources.)

First, we define a script *name*. In Thorium, names are always unique integers. This unique name can be chosen at random when you start creating a script. However, you must take care to ensure that this name is not used by any other scripts in your program (your own scripts and/or those in the BIOSAL or Thorium subsystems).

.. code-block:: c

   #define SCRIPT_HELLO 0xfbcedbb5

While this might at first be intimidating, fret not. You can run one of the convenience scripts we provide for getting a unique script name by visiting ``biosal/scripts`` and running ``generate-random-script.sh`` as follows:

.. code-block:: text

   $ cd biosal/scripts
   $ $ ./generate-random-script.sh 
   #define SCRIPT_CHANGE_ME 0xf1c5511

Voila! You have a script name. You can change this name, but we recommend keeping the prefix ``SCRIPT_``. (If you report problems to us, we're likely to check that you are making some attempt to follow coding conventions used by other scripts. It's really hard to help folks otherwise.)

Next, we set up the *state* for the actor. This is basically what you do whenever you write a C program: You create an abstract data type and use it to create instances (more on that when we look at the corresponding C code).

.. code-block:: c

   struct hello {
       struct core_vector initial_helloes;
   };

For the hello program, the state of an actor is pretty straightforward. We just need to keep a ``core_vector`` (a fancy word for arrays, included when you ``#include <biosal.h>``) of the fellow acquaintances. This way, we will know how many actors are saying hello. In a more advanced example, we will show how to do a more *distributed* style of hello that basically passes greetings around a ring of actors.


Lastly, you create an instance of a Thorium script, i.e. ``struct thorium_script``. The Thorium script is your conduit to actors programming. It allows you to specify the lifecycle of an actor (we'll look more closely at this shortly as well).

.. code-block:: c

   extern struct thorium_script hello_script;

So that's all their is to it. Much like object-oriented programming (found in C++, Java, and Scala, among other modern languages) you basically create the interface (in the .h file) and implementation (behavior, in a .c file). You can combine scripts to write modular parallel programs, which we think makes Thorium easy to develop and maintain.

Let's now take a look at the implementation details by looking at ``hello.c``. We'll take a look at this code by working from snippets (fragments of the entire code presented in logical order). You may pull up the full source code at this time if you prefer to study it in its entirety.

We start by creating the script. The script is the core magic that makes actor creation (and spawning, covered in a more advanced example) possible. 

.. literalinclude:: ../../examples/hello_world/hello.c
   :language: c
   :start-after: hello-hello_script
   :end-before: hello-hello_init

Let's take a close at each of the fields we're initializing in the script:

.. code-block:: c

   .identifier = SCRIPT_HELLO,

This sets the script name, which we defined in ``hello.h`` earlier.

We then specify the three key actor lifecycle functions (pointers). This is very similar to what happens in object-oriented programming, corresponding to the C++ concepts of constructors and desctructors, only much simplified. The ``init`` and ``destroy`` functions are always written the same way, regardless of actor script. Because actors have disciplined relationships with acquaintances, any *recursive* behavior will be handled by sending messages to do the work. That is, if an actor needs to tear down a structure, it asks its acquaintances to do so on its behalf.

.. code-block:: c

    .init = hello_init,
    .destroy = hello_destroy,
    .receive = hello_receive,

We'll go into the details of these methods shortly. Every time you write a script, you will provide definitions of these.

Lastly, we set the ``size`` field, which tells the size of the underyling structure and ``name`` to a user-friendly name (in case you were wondering why we use ``int``, we do have a way of getting a more descriptive name, which can be extremely helpful in debugging and logging situations).

.. code-block:: c

    .size = sizeof(struct hello),
    .name = "hello"

So the structure initialization is mostly straightforward. In most situations, you'll just copy and paste this to create new scripts, changing a few names to be script-specific in nature.

Now let's take a look at how we implement the script's methods.

.. todo:: 

   George will continue from here.

.. literalinclude:: ../../examples/hello_world/hello.c
   :language: c
   :start-after: hello-hello_init
   :end-before: hello-hello_destroy


.. literalinclude:: ../../examples/hello_world/hello.c
   :language: c
   :start-after: hello-hello_destroy
   :end-before: hello-hello_receive
