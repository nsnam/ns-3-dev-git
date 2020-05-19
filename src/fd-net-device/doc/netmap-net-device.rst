Netmap NetDevice
-------------------------
.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

The ``fd-net-device`` module provides the ``NetmapNetDevice`` class, a class derived
from the ``FdNetDevice`` which is able to read and write traffic using a netmap file descriptor.
This netmap file descriptor must be associated to a real ethernet device in the host machine.
The ``NetmapNetDeviceHelper`` class supports the configuration of a ``NetmapNetDevice``.

netmap is a fast packets processing that bypasses the host networking stack and gains
direct access to network device.

Model Description
*****************



Design
======



Scope and Limitations
=====================
The main scope of ``NetmapNetDevice`` is to support the flow-control between the physical device and
the upper layer and using at best the computational resources to process packets.


Usage
*****



Helpers
=======



Attributes
==========



Output
======



Examples
========

Several examples are provided:

* ``fd-emu-onoff.cc``: This example is aimed at measuring the throughput of the
  NetmapNetDevice when using the NetmapNetDeviceHelper to attach the simulated
  device to a real device in the host machine. This is achieved by saturating
  the channel with TCP or UDP traffic.
* ``fd-emu-ping.cc``: This example uses the NetmapNetDevice to send ICMP
  traffic over a real device.
* ``fd-emu-tc.cc``: This example configures a router on a machine with two interfaces
  in emulated mode through netmap. The aim is to explore different qdiscs behaviours on the backlog of a
  device emulated bottleneck side.
* ``fd-emu-send.cc``: This example builds a node with a device in emulation mode through netmap.
  The aim is to measure the maximum tx rate in pps achievable with NetmapNetDevice on a specific machine.

Note that all the examples run in emulation mode through netmap (with NetmapNetDevice) and raw socket (with FdNetDevice).

Acknowledgments
***************
The NetmapNetDevice has been presented and evaluated in

* Pasquale Imputato, Stefano Avallone, Enhancing the fidelity of network emulation through direct access to device buffers, Journal of Network and Computer Applications, Volume 130, 2019, Pages 63-75, (http://www.sciencedirect.com/science/article/pii/S1084804519300220)
