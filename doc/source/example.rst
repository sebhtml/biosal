An Example
=============

Writing Thorium scripts is similar to writing object-oriented code. Although our framework is C-based, Thorium allows you to write your code in a modular way by separating the interfaces from the implementation.

As an example, let's consider how we'd write a simple, yet familiar, program: Hello. World.

You can study this code by looking in the folder ``biosal/examples/hello``.


As a first step, let's take a look at ``hello.h``:

.. literalinclude:: ../../examples/hello_world/hello.h
   :language: c
   :linenos:

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

Let's now take a look at the implementation details.

