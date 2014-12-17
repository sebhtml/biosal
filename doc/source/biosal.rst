About BIOSAL
=================

biosal is a distributed BIOlogical Sequence Actor Library.

biosal applications are written in the form of actors which send each other messages, change their state, spawn other actors and eventually die of old age. These actors run on top of biosal runtime system called Thorium using the API. Thorium is a distributed actor machine. Thorium uses script-wise symmetric actor placement and is targeted for high-performance computing. Thorium is a general purpose actor model implementation.

This manual is focused on Thorium and how to use it for writing applications. 

Getting BIOSAL
-----------------

.. code-block:: text

   git clone https://github.com/GeneAssembly/biosal.git
   cd biosal
   make tests # run tests
   make examples # run examples

We encourage you to work directly from the git repository as Thorium and BIOSAL are under active development.
Once you have cloned your repository, you can keep it up-to-date by using ``git pull`` and rebuilding the latest source using ``make``.

For more information about the project, including detailed build and configuration instructions, please visit https://github.com/GeneAssembly/biosal. 
Most of these instructions will eventually be migrated to this document.
