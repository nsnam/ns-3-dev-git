.. include:: replace.txt
.. highlight:: cpp

VirtualNetDevice
----------------

This is the introduction to VirtualNetDevice, to complement the VirtualNetDevice
model doxygen.

Overview of the VirtualNetDevice
********************************

The ``VirtualNetDevice`` is a "virtual" NetDevice implementation, similar
to Linux TUN/TAP interfaces.  Instead of sending packets over a simulated
channel, it delegates the actual transmission of L2 frames to user code via a
callback configured with ``SetSendCallback()``.

The user code may also inject frames into the device using the
``VirtualNetDevice::Receive()`` method.  Together, these features allow one to
build tunnels (for example IP-over-UDP-over-IP, or IP-over-IP) without needing
to implement a full NetDevice subclass.

Model Description
*****************

The source code for this module is written in the directory
``src/virtual-net-device``.

The device exposes a standard NetDevice interface and can be attached to a node
and configured with an address and MTU.  By default, the device reports itself
as a point-to-point device, does not require ARP, and supports ``SendFrom``;
these behaviors can be configured via:

* ``SetIsPointToPoint()``
* ``SetNeedsArp()``
* ``SetSupportsSendFrom()``

Usage
*****

Typical usage is to create a device, install it on a node, configure a transmit
callback, and then forward packets to and from some external mechanism (e.g., a
socket or a tunnel endpoint) in user code.

A minimal sketch is as follows:

::

  Ptr<VirtualNetDevice> dev = CreateObject<VirtualNetDevice>();
  dev->SetAddress(Mac48Address::Allocate());
  dev->SetSendCallback(MakeCallback(&MyTunnelEndpoint::Send, endpoint));
  node->AddDevice(dev);

To inject a frame into the node's protocol stack, call
``VirtualNetDevice::Receive()`` with the appropriate protocol number and source
and destination addresses.

Traces
******

The standard set of MAC-level NetDevice trace sources is provided:

* ``MacTx``: Trace source indicating a packet has arrived for transmission by
  this device
* ``MacRx``: Trace source for packets successfully received in non-promiscuous
  mode
* ``MacPromiscRx``: Trace source for packets received in promiscuous mode and
  fired only when a promiscuous receive callback is installed via
  ``SetPromiscReceiveCallback``
* ``Sniffer`` / ``PromiscSniffer``: Trace sources simulating non-promiscuous
  and promiscuous packet sniffers
