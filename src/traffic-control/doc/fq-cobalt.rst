.. include:: replace.txt
.. highlight:: cpp
.. highlight:: bash

FqCobalt queue disc
------------------

This chapter describes the FqCobalt ([Hoe16]_) queue disc implementation in |ns3|.

The FlowQueue-Cobalt (FQ-Cobalt) algorithm is similar to FlowQueue-CoDel (FQ-CoDel) algorithm available in ns-3-dev/src/traffic-control/doc/fq-codel.rst.
The documentation for Cobalt is available in ns-3-dev/src/traffic-control/doc/cobalt.rst.

FqCobalt is one of the crucial piece needed to complete to CAKE module `<https://ieeexplore.ieee.org/document/8475045>`

Model Description
*****************

The source code for the FqCobalt queue disc is located in the directory
``src/traffic-control/model`` and consists of 2 files `fq-cobalt-queue-disc.h`
and `fq-cobalt-queue-disc.cc` defining a FqCobaltQueueDisc class and a helper
FqCobaltFlow class. The code was ported to |ns3| based on Linux kernel code
implemented by Jonathan Morton.

The Model Description is similar to the FqCoDel documentation mentioned above.


References
==========

.. [CAK16] https://github.com/torvalds/linux/blob/master/net/sched/sch_cake.c , Implementation of CAKE in Linux

Attributes
==========

The key attributes that the FqCobaltQueue class holds include the following:

Most of the key attributes are similar to the FqCoDel implementation mentioned above.
Some differences are:
Absence of ``MinBytes`` parameter.
Some extra parameters are
* ``Pdrop:`` Value of drop probability.
* ``Increment:`` Increment value of drop probability. Default value is 1./256 .
* ``Decrement:`` Decrement value of drop probability. Default value is 1./4096 .
* ``BlueThreshold:`` The Threshold after which Blue is enabled. Default value is 400ms.

Note that if the user wants to disable Blue Enhancement then the user can set
it to a large value for example Time::Max().
Examples
========

A typical usage pattern is to create a traffic control helper and to configure type
and attributes of queue disc and filters from the helper. For example, FqCobalt
can be configured as follows:

.. sourcecode:: cpp

  TrafficControlHelper tch;
  tch.SetRootQueueDisc ("ns3::FqCobaltQueueDisc", "DropBatchSize", UintegerValue (1)
                                                 "Perturbation", UintegerValue (256));
  QueueDiscContainer qdiscs = tch.Install (devices);

Validation
**********

The FqCobalt model is tested using :cpp:class:`FqCobaltQueueDiscTestSuite` class defined in `src/test/ns3tc/codel-queue-test-suite.cc`.

The tests are similar to the ones for FqCoDel queue disc mentioned in first section of this document.
The test suite can be run using the following commands::

  $ ./waf configure --enable-examples --enable-tests
  $ ./waf build
  $ ./test.py -s fq-cobalt-queue-disc

or::

  $ NS_LOG="FqCobaltQueueDisc" ./waf --run "test-runner --suite=fq-cobalt-queue-disc"