.. include:: replace.txt
.. highlight:: cpp

++++++++++++++++++++
Design Documentation
++++++++++++++++++++


Existing multi-homed devices may share transmitting resources (e.g., antennas) among multiple technologies
(Wi-Fi, Bluetooth, cellular, etc.) to reduce their complexity, cost and power consumption. Such an
approach requires the presence of an arbitrator that allocates transmitting resources to the various
technologies and technology-specific components that take appropriate actions based on the allocation
made by the arbitrator.

In the |ns3| simulator, the ``coex`` module provides an infrastructure to manage the coexistence of
multiple technologies sharing common transmitting resources.


Overview of the model
*********************

The operations of an arbitrator are modelled by the ``Arbitrator`` base class included in the
``coex`` module. An arbitrator object must be aggregated to the node to add support for
coexistence mechanisms. Additionally, every technology sharing the common transmitting resources
can interact with the arbitrator via a ``DeviceManager`` object. The relationship among arbitrator,
device manager and ``NetDevice`` is shown in Fig. :ref:`fig-coex-arch`, where Wi-Fi is used as an
example of technology involved in the coexistence mechanisms.

.. _fig-coex-arch:

.. figure:: figures/coex-architecture.*
   :align: center

   Illustration of the architecture used to support coexistence mechanisms

The coex arbitrator receives requests to use the shared resources from a device manager, determines
whether they can be accepted (e.g., they do not refer to a time interval in the past, the events do not
last longer than their periodicity, etc.) and forwards them (as coexistence events) to the other device managers.
It is up to the device managers to take appropriate actions when notified of a coex event, such as
making sure that the transmitting resources are not used while being allocated to another technology
and informing the netdevice object that appropriate countermeasures must be taken.
