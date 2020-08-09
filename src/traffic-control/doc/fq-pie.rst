.. include:: replace.txt
.. highlight:: cpp
.. highlight:: bash

FqPIE queue disc
------------------

This chapter describes the FqPIE ([Hoe16]_) queue disc implementation in |ns3|.

The FlowQueue-PIE (FQ-PIE) algorithm is similar to the FlowQueue part of FlowQueue-CoDel (FQ-CoDel) algorithm available in ns-3-dev/src/traffic-control/doc/fq-codel.rst.
The documentation for PIE is available in ns-3-dev/src/traffic-control/doc/pie.rst.


Model Description
*****************

The source code for the FqPIE queue disc is located in the directory
``src/traffic-control/model`` and consists of 2 files `fq-pie-queue-disc.h`
and `fq-pie-queue-disc.cc` defining a FqPieQueueDisc class and a helper
FqPieFlow class. The code was ported to |ns3| based on Linux kernel code
implemented by Mohit P. Tahiliani.

The Model Description is similar to the FqCoDel documentation mentioned above.


References
==========

.. [CAK16] https://github.com/torvalds/linux/blob/master/net/sched/sch_fq_pie.c , Implementation of FqPIE in Linux
.. [PIE] https://ieeexplore.ieee.org/document/9000684

Attributes
==========

The key attributes that the FqPieQueue class holds include the following:

Most of the key attributes are similar to the FqCoDel implementation mentioned above.
Some differences are:
Absence of ``MinBytes``, ``Interval`` and ``Target`` parameter.
Some extra parameters are
* ``MarkEcnThreshold:`` ECN marking threshold (RFC 8033 suggests 0.1 (i.e., 10%) default).
* ``MeanPktSize:`` Average of packet size.
* ``A:`` Decrement value of drop probability. Default value is 1./4096 .
* ``B:`` The Threshold after which Blue is enabled. Default value is 400ms.
* ``Tupdate:`` Time period to calculate drop probability
* ``Supdate:`` Start time of the update timer
* ``DequeueThreshold:`` Minimum queue size in bytes before dequeue rate is measured
* ``QueueDelayReference:`` Desired queue delay
* ``MaxBurstAllowance:`` Current max burst allowance before random drop
* ``UseDequeueRateEstimator:`` Enable/Disable usage of Dequeue Rate Estimator
* ``UseCapDropAdjustment:`` Enable/Disable Cap Drop Adjustment feature mentioned in RFC 8033
* ``UseDerandomization:`` Enable/Disable Derandomization feature mentioned in RFC 8033


Examples
========

A typical usage pattern is to create a traffic control helper and to configure type
and attributes of queue disc and filters from the helper. For example, FqPIE
can be configured as follows:

.. sourcecode:: cpp

  TrafficControlHelper tch;
  tch.SetRootQueueDisc ("ns3::FqPieQueueDisc", "DropBatchSize", UintegerValue (1)
                                                 "Perturbation", UintegerValue (256));
  QueueDiscContainer qdiscs = tch.Install (devices);

Validation
**********

The FqPie model is tested using :cpp:class:`FqPieQueueDiscTestSuite` class defined in `src/test/ns3tc/fq-pie-queue-test-suite.cc`.

The tests are similar to the ones for FqCoDel queue disc mentioned in first section of this document.
The test suite can be run using the following commands::

  $ ./waf configure --enable-examples --enable-tests
  $ ./waf build
  $ ./test.py -s fq-pie-queue-disc

or::

  $ NS_LOG="FqPieQueueDisc" ./waf --run "test-runner --suite=fq-pie-queue-disc"