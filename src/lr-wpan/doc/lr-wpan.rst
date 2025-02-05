.. include:: replace.txt
.. highlight:: cpp
.. highlight:: bash

IEEE 802.15.4: Low-Rate Wireless Personal Area Network (LR-WPAN)
================================================================

This chapter describes the implementation of |ns3| models for the
low-rate, wireless personal area network (LR-WPAN) as specified by
IEEE standard 802.15.4 (2003,2006,2011). The current emphasis is on direct transmissions running on both,
slotted and unslotted mode (CSMA/CA) of IEEE 802.15.4 operation for use in Zigbee (TM) and 6loWPAN networks.

Both beacon and non-beacon modes are supported as well as the bootstrap mechanism (scan and association).

In general, this document describes what is modeled and how it is modeled; the following section Scope and
Limitations provides more details about what is not covered or what is missing in the model.

The model is implemented into the ``src/lrwpan/`` folder.

The model design closely follows the standard from an architectural standpoint.

.. _fig-lr-wpan-arch:

.. figure:: figures/lr-wpan-arch.*

    Architecture and scope of lr-wpan models

The grey areas in the figure (adapted from Fig 3. of IEEE Std. 802.15.4-2006)
show the scope of the model.

The Spectrum NetDevice from Nicola Baldo is the basis for the implementation.

The implementation also borrows some ideas from the ns-2 models developed by
Zheng and Lee.

The APIs closely follow the standard, adapted for |ns3| naming conventions
and idioms.  The APIs are organized around the concept of service primitives
as shown in the following figure adapted from Figure 14 of
IEEE Std. 802.15.4-2006.

.. _fig-lr-wpan-primitives:

.. figure:: figures/lr-wpan-primitives.*

    Service primitives

The APIs primitives are organized around four conceptual services and service access
points (SAP):

* MAC data service (MCPS)
* MAC management service  (MLME)
* PHY data service (PD)
* PHY management service (PLME)

In general, primitives are standardized as follows (e.g. Sec 7.1.1.1.1
of IEEE 802.15.4-2006):::

  MCPS-DATA.request(
                    SrcAddrMode,
                    DstAddrMode,
                    DstPANId,
                    DstAddr,
                    msduLength,
                    msdu,
                    msduHandle,
                    TxOptions,
                    SecurityLevel,
                    KeyIdMode,
                    KeySource,
                    KeyIndex
                   )

In |ns3| this maps to classes, structs and methods such as:::

  struct McpsDataRequestParameters
  {
    uint8_t m_srcAddrMode;
    uint8_t m_dstAddrMode;
    ...
  };

  void
  LrWpanMac::McpsDataRequest(McpsDataRequestParameters params)
  {
  ...
  }

The MAC primitives currently supported by the |ns3| model are:

* MCPS-DATA (Request, Confirm, Indication)
* MLME-START (Request, Confirm)
* MLME-SCAN (Request, Confirm)
* MLME-BEACON-NOFIFY (Indication)
* MLME-ASSOCIATE.Request (Request, Confirm, Response, Indication)
* MLME-POLL (Confirm)
* MLME-COMM-STATUS (Indication)
* MLME-SYNC (Request)
* MLME-SYNC-LOSS (Indication)
* MLME-SET (Request, Confirm)
* MLME-GET (Request, Confirm)


The PHY primitives currently supported by the |ns3| model are:

* PLME-CCA (Request, Confirm)
* PD-DATA (Request, Confirm, Indication)
* PLME-SET-TRX-STATE (Request, Confirm)
* PLME-SET (Request, Confirm)
* PLME-GET (Request, Confirm)

For more information on primitives, See IEEE 802.15.4-2011, Table 8.

In addition, this module provides a configuration for connecting various components.
The current design of the ``LrWpanNetDevice`` allows for the configuration of different instances of ``LrWpanMac``, which can only be set before the initialization of the NetDevice.
This design can be extended in the future to support other subclasses of ``LrWpanMacBase``, enabling the configuration of different types of MAC layers using the same ``LrWpanNetDevice``.

The ``LrWpanNetDevice`` presented in this module is intended to be used with other technology profiles, such as the 6LoWPAN and ZigBee stack.




Scope and Limitations
---------------------

- Indirect data transmissions are not supported but planned for a future update.
- Devices are capable of associating with a single PAN coordinator.
- Interference is modeled as AWGN but this is currently not thoroughly tested.
- The standard describes the support of multiple PHY band-modulations but currently, only 250kbps O-QPSK (channel page 0) is supported.
- Active and passive MAC scans are able to obtain a LQI value from a beacon frame, however, the scan primitives assumes LQI is correctly implemented and does not check the validity of its value.
- Configuration of the ED thresholds are currently not supported.
- Coordinator realignment command is only supported in orphan scans.
- Disassociation primitives are not supported.
- Security is not supported.
- Guaranteed Time Slots (GTS) are not supported.
- Not all attributes are supported by the MLME-SET and MLME-GET primitives.
- Indirect transmissions are only supported during the association process.
- RSSI is not supported as this is part of the 2015 revision and the current implementation only supports until the 2011 revision.
- PHY and MAC are currently not supported by the attribute system. To change the behavior of the PHY and MAC the standard SET primitives (e.g. MLME-SET.request) must be used.

The PHY layer
-------------

The physical layer components consist of a PHY model, an error rate model,
and a loss model. The PHY state transitions are roughly model after
ATMEL's AT86RF233.

.. _fig-lr-wpan-phy:

.. figure:: figures/lr-wpan-phy.*

    Ns-3 lr-wpan PHY basic operating mode state diagram

The error rate model presently models the error rate
for IEEE 802.15.4 2.4 GHz AWGN channel for OQPSK; the model description can
be found in IEEE Std 802.15.4-2006, section E.4.1.7.   The Phy model is
based on SpectrumPhy and it follows specification described in section 6
of IEEE Std 802.15.4-2006. It models PHY service specifications, PPDU
formats, PHY constants and PIB attributes. It currently only supports
the transmit power spectral density mask specified in 2.4 GHz per section
6.5.3.1. The noise power density assumes uniformly distributed thermal
noise across the frequency bands. The loss model can fully utilize all
existing simple (non-spectrum phy) loss models. The Phy model uses
the existing single spectrum channel model.
The physical layer is modeled on packet level, that is, no preamble/SFD
detection is done. Packet reception will be started with the first bit of the
preamble (which is not modeled), if the SNR is more than -5 dB, see IEEE
Std 802.15.4-2006, appendix E, Figure E.2. Reception of the packet will finish
after the packet was completely transmitted. Other packets arriving during
reception will add up to the interference/noise.

Rx sensitivity is defined as the weakest possible signal point at which a receiver can receive and decode a packet with a high success rate.
According to the standard (IEEE Std 802.15.4-2006, section 6.1.7), this
corresponds to the point where the packet error rate is under 1% for 20 bytes PSDU
reference packets (11 bytes MAC header + 7 bytes payload (MSDU) + FCS 2 bytes). Setting low Rx sensitivity values (increasing the radio hearing capabilities)
have the effect to receive more packets (and at a greater distance) but it raises the probability to have dropped packets at the
MAC layer or the probability of corrupted packets. By default, the receiver sensitivity is set to the maximum theoretical possible value of -106.58 dBm for the supported IEEE 802.15.4 O-QPSK 250kps.
This rx sensitivity is set for the "perfect radio" which only considers the floor noise, in essence, this do not include the noise factor (noise introduced by imperfections in the demodulator chip or external factors).
The receiver sensitivity can be changed to different values using ``SetRxSensitivity`` function in the PHY to simulate the hearing capabilities of different compliant radio transceivers (the standard minimum compliant Rx sensitivity is -85 dBm).:

::

                                                              (defined by the standard)
   NoiseFloor          Max Sensitivity                          Min Sensitivity
   -106.987dBm          -106.58dBm                                   -85dBm
    |-------------------------|------------------------------------------|
                          Noise Factor = 1
                              | <--------------------------------------->|
                                    Acceptable sensitivity range

The example ``lr-wpan-per-plot.cc`` shows that at given Rx sensitivity, packets are dropped regardless of their theoretical error probability.
This program outputs a file named ``802.15.4-per-vs-rxSignal.plt``.
Loading this file into gnuplot yields a file ``802.15.4-per-vs-rsSignal.eps``, which can
be converted to pdf or other formats. Packet payload size, Tx power and Rx sensitivity can be configured.
The point where the blue line crosses with the PER indicates the Rx sensitivity. The default output is shown below.

.. _fig-802-15-4-per-sens:

.. figure:: figures/802-15-4-per-sens.*

    Default output of the program ``lr-wpan-per-plot.cc``

The MAC layer
-------------

The MAC at present implements both, the unslotted CSMA/CA (non-beacon mode) and
the slotted CSMA/CA (beacon-enabled mode). The beacon-enabled mode supports only
direct transmissions. Indirect transmissions and Guaranteed Time Slots (GTS) are
currently not supported.

The present implementation supports a single PAN coordinator, support for additional
coordinators is under consideration for future releases.

The implemented MAC is similar to Contiki's NullMAC, i.e., a MAC without sleep
features. The radio is assumed to be always active (receiving or transmitting),
of completely shut down. Frame reception is not disabled while performing the
CCA.

The main API supported is the data transfer API
(McpsDataRequest/Indication/Confirm).  CSMA/CA according to Stc 802.15.4-2006,
section 7.5.1.4 is supported. Frame reception and rejection according to
Std 802.15.4-2006, section 7.5.6.2 is supported, including acknowledgements.
Both short and extended addressing are supported. Various trace sources are
supported, and trace sources can be hooked to sinks.

Scan and Association
~~~~~~~~~~~~~~~~~~~~

The implemented |ns3| MAC layer supports scanning. Typically, a scanning request is preceded
by an association request but these can be used independently.
|ns3| IEEE 802.15.4 MAC layer supports 4 types of scanning:

* **Energy Detection (ED) Scan:** In an energy scan, a device or a coordinator scan a set number of channels looking for traces of energy. The maximum energy registered during a given amount of time is stored. Energy scan is typically used to measure the quality of a channel at any given time. For this reason, coordinators often use this scan before initiating a PAN on a channel.

* **Active Scan:** A device sends ``beacon request commands`` on a set number of channels looking for a PAN coordinator. The receiving coordinator must be configured on non-beacon mode. Coordinators on beacon-mode ignore these requests. The coordinators who accept the request, respond with a beacon. After an active scan take place, during the association process devices extract the information in the PAN descriptors from the collected beacons and based on this information (e.g. channel, LQI level), choose a coordinator to associate with.

* **Passive Scan:** In a passive scan, no ``beacon requests commands`` are sent. Devices scan a set number of channels looking for beacons currently being transmitted (coordinators in beacon-mode). Like in the active scan, the information from beacons is stored in PAN descriptors and used by the device to choose a coordinator to associate with.

* **Orphan Scan:** Orphan scan is used typically by device as a result of repeated communication failure attempts with a coordinator. In other words, an orphan scan represents the intent of a device to relocate its coordinator. In some situations, it can be used by devices higher layers to not only rejoin a network but also join a network for the first time. In an orphan scan, a device send a ``orphan notification command`` to a given list of channels. If a coordinator receives this notification, it responds to the device with a ``coordinator realignment command``.

In active and passive scans, the link quality indicator (LQI) is the main parameter used to
determine the optimal coordinator. LQI values range from 0 to 255. Where 255 is the highest quality link value and 0 the lowest. Typically, a link lower than 127 is considered a link with poor quality.

In LR-WPAN, association is used to join PANs. All devices in LR-WPAN must belong to a PAN to communicate. |ns3| uses a classic association procedure described in the standard. The standard also covers a more effective association procedure known as fast association (See IEEE 802.15.4-2015, fastA) but this association is currently not supported by |ns3|. Alternatively, |ns3| can do a "quick and dirty" association using either ```LrWpanHelper::AssociateToPan``` or ```LrWpanHelper::AssociateToBeaconPan```. These functions are used when a preset association can be done. For example, when the relationships between existing nodes and coordinators are known and can be set before the beginning of the simulation. In other situations, like in many networks in real deployments or in large networks, it is desirable that devices "associate themselves" with the best possible available coordinator candidates. This is a process known as bootstrap, and simulating this process makes it possible to demonstrate the kind of situations a node would face in which large networks to associate in real environment.

Bootstrap (a.k.a. network initialization) is possible with a combination of scan and association MAC primitives. Details on the general process for this network initialization is described in the standard. Bootstrap is a complex process that not only requires the scanning networks, but also the exchange of command frames and the use of a pending transaction list (indirect transmissions) in the coordinator to store command frames. The following summarizes the whole process:

.. _fig-lr-wpan-assocSequence:

.. figure:: figures/lr-wpan-assocSequence.*

Bootstrap as whole depends on procedures that also take place on higher layers of devices and coordinators. These procedures are briefly described in the standard but out of its scope (See IEE 802.15.4-2011 Section 5.1.3.1.). However, these procedures are necessary for a "complete bootstrap" process. In the examples in |ns3|, these high layer procedures are only briefly implemented to demonstrate a complete example that shows the use of scan and association. A full high layer (e.g. such as those found in Zigbee and Thread protocol stacks) should complete these procedures more robustly.
Examples such as  ``lr-wpan-bootstrap.cc`` demonstrate the whole bootstrap process (including scanning and association) while doing some place holder of procedures that take place in higher layers which are not part of standard focus.

A key element to remember is that bootstrap have 2 objectives:

1. Enable devices to join a new formed network (associate).
2. Assign short addresses and PAN ID.

Devices that have the short address ``FF:FF`` are not associated an cannot participate in any unicast communication in the network.
Devices that have the short address ``FF:FE`` and have a valid PAN ID can communicate with other devices in the network using the
extended address mode. In this mode, devices will use its 64 bit address (A.K.A. extended address) to communicate in the network.

Before ns-3.44, the MAC association struggled to handle scenarios where the association response command arrived before the acknowledgment (ACK)
for a data request command. This issue could arise in saturated networks and is caused by a delayed data request command ACK
(which do not use CSMA/CA but can be retried several times if necessary). This situation highlighted a problem and a design flaw in the original standard, where association responses
are sent immediately after sending a data request acknowledgments. As a result, their arrival could become inverted. However,
the current MAC implementation is now capable of correctly reacting to this delayed ACK issue.

Finally, a fixed association is possible in |ns3| without the use of the bootstrap process. For this purpose, the ``LrWpanHelper::CreateAssociatedPan``
is used. See the Helpers subsection for more details.

MAC transmission Queues
~~~~~~~~~~~~~~~~~~~~~~~

By default, ``Tx queue`` and ``Ind Tx queue`` (the pending transaction list) are not limited but they can configure to drop packets after they
reach a limit of elements (transaction overflow). Additionally, the ``Ind Tx queue`` drop packets when the packet has been longer than
``macTransactionPersistenceTime`` (transaction expiration). Expiration of packets in the Tx queue is not supported.
Finally, packets in the ``Tx queue`` may be dropped due to excessive transmission retries or channel access failure.

MAC addresses
~~~~~~~~~~~~~

Contrary to other technologies, a IEEE 802.15.4 has 2 different kind of addresses:

* Long addresses (64 bits)
* Short addresses (16 bits)

The 64-bit addresses are unique worldwide, and set by the device vendor (in a real device).
The 16-bit addresses are not guaranteed to be unique, and they are typically either assigned
during the devices deployment, or assigned dynamically during the device bootstrap.

The other relevant "address" to consider is the PanId (16 bits), which represents the PAN
the device is attached to.

Due to the limited number of available bytes in a packet, IEEE 802.15.4 tries to use short
addresses instead of long addresses, even though the two might be used at the same time.

For the sake of communicating with the upper layers, and in particular to generate auto-configured
IPv6 addresses, each NetDevice must identify itself with a MAC address. The MAC addresses are
also used during packet reception, so it is important to use them consistently.

Focusing on IPv6 Stateless address autoconfiguration (SLAAC), there are two relevant RFCs to
consider: RFC 4944 and RFC 6282, and the two differ on how to build the IPv6 address given
the NetDevice address.

RFC 4944 mandates that the IID part of the IPv6 address is calculated as ``YYYY:00ff:fe00:XXXX``,
while RFC 6282 mandates that the IID part of the IPv6 address is calculated as ``0000:00ff:fe00:XXXX``
where ``XXXX`` is the device short address, and ``YYYY`` is the PanId.
In both cases the U/L bit must be set to local, so in the RFC 4944 the PanId might have one bit flipped.

In order to facilitate interoperability, and to avoid unwanted module dependencies, the |ns3|
implementation moves the IID calculation in the ``LrWpanNetDevice::GetAddress ()``, which will
return an ``Address`` formatted properly, i.e.:

* The Long address (a ``Mac64Address``) if the Short address has not been set, or
* A properly formatted 48-bit pseudo-address (a ``Mac48Address``) if the short address has been set.

The 48-bit pseudo-address is generated according to either RFC 4944 or RFC 6282 depending on the
configuration of an Attribute (``PseudoMacAddressMode``).

The default is to use RFC 6282 style addresses.

Note that, on reception, a packet might contain either a short or a long address. This is reflected
in the upper-layer notification callback, which can contain either the pseudo-address (48 bits) or
the long address (64 bit) of the sender.

Note also that RFC 4944 or RFC 6282 are the RFCs defining the IPv6 address compression formats
(HC1 and IPHC respectively). It is definitely not a good idea to either mix devices using different
pseudo-address format or compression types in the same network. This point is further discussed
in the ``sixlowpan`` module documentation.


Usage
-----


Helpers
~~~~~~~

The helper is patterned after other device helpers.  In particular,
tracing (ascii and pcap) is enabled similarly, and enabling of all
lr-wpan log components is performed similarly.  Use of the helper
is exemplified in many of the examples found in ``src\lrwpan\examples``.

When the helper is used, the default propagation loss model added to the channel is the ``LogDistancePropagationLossModel`` with default parameters.
The default delay model used is ``ConstantSpeedPropagationDelayMode``. Alternatively, users can specify the models used by the helper with the
``SetPropagationDelayModel`` and the ``AddPropagationLossModel`` functions. The helper can also be used to form a fully associated PANs:

::

    NodeContainer nodes;
    nodes.Create(4);
    LrWpanHelper lrWpanHelper;
    // The propagation and Delay Model are optional, a default is used if not provided.
    lrWpanHelper.SetPropagationDelayModel("ns3::ConstantSpeedPropagationDelayModel");
    lrWpanHelper.AddPropagationLossModel("ns3::LogDistancePropagationLossModel");
    // Create and associate a PAN with addresses, The first node is used as coordinator
    lrWpanHelper.CreateAssociatedPan(lrwpanDevices, 0xCAFE);
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);

In the example shown above, the function ``CreateAssociatedPan`` forms a star topology PAN with devices sharing the same PAN ID (In the example 0xCAFE).
Devices are assigned both 64 bit addresses (extended addresses) and 16 bit addresses (short addresses) and can use either to communicate
when the configuration is completed. The first node in the ``NodeContainer`` is assigned the role of PAN coordinator.
The number of nodes provided to the helper are limited by the number of possible assignable short addresses ([00:01]-[FF:FD]).

It is also possible to manually assign the PAN ID, the extended address and the short address without the use of a helper. To know how to do this,
refer to the ``lr-wpan-data.cc`` example.

For more complex topologies an auto-configured address assignation scheme is recommended,
the bootstrap process (scan and association) described by the standard can help with this.

**Important:** Devices that does not have short addresses assigned (Any address other than ``FF:FF``) on PAN IDs cannot communicate in lr-wpan network. Make sure both
are present in your simulated devices using any of the schemes described.


Finally, the ``LrWpanHelper`` can be used to generate pcap or ascii traces.
These pcap files can be later on visualized in Wireshark.

::

    // Tracing
    lrWpanHelper.EnablePcapAll(std::string("lr-wpan-data"), true);
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream("lr-wpan-data.tr");
    lrWpanHelper.EnableAsciiAll(stream);

Attributes
~~~~~~~~~~

The following is a list of attributes used to configure the behavior of the PHY layer:

* ``PostReceptionErrorModel``: An optional packet error model can be added to the receive packet process after any propagation-based (SNR-based) error models have been applied. Typically this is used to force specific packet drops, for testing purposes.


The following is a list of attributes that can be set directly to configure the
general behavior of the **MAC layer**. Most of this attributes have a counter part with
the PIB attributes and constants described by the standard.

* ``PanId``: Set the 16-bit identifier of the associated PAN.

Traces
~~~~~~


The following is a list of the trace source that can be used to monitor the behavior of the **PHY layer**:

* ``TrxStateValue``: The state of the finit state machine of the PHY.
* ``TrxState``: The state of the transceiver.
* ``PhyTxBegin``: Indicates that a packet has begun transmitting over the channel medium.
* ``PhyTxEnd``: Indicates that a packet has been completely transmitted over the channel.
* ``PhyTxDrop``: Indicates that a packet has been dropped by the device during transmission.
* ``PhyRxBegin``: Indicates that a packet has begun being received from the channel medium by the receiver.
* ``PhyRxEnd``: Indicates that a packet has been completely received from the channel medium.
* ``PhyRxDrop``: Indicates that a packet has been dropped by the device during reception.

The following is a list of the trace sources that can be used to monitor the behavior of the **MAC layer**:

* ``MacTxEnqueue``: Trace source indicating a packet has been enqueued in the transaction queue.
* ``MacTxDequeue``: Trace source indicating a packet has was dequeued from the transaction queue.
* ``MacIndTxEnqueue``: Trace source indicating a packet has been enqueued in the indirect transaction queue.
* ``MacIndTxDequeue``: Trace source indicating a packet has was dequeued from the indirect transaction queue.
* ``MacTx``: Trace source indicating a packet has arrived for transmission by this device.
* ``MacTxOk``: Trace source indicating a packet has been successfully sent.
* ``MacTxDrop``: Trace source indicating a packet has been dropped during transmission.
* ``MacIndTxDrop``: Trace source indicating a packet has been dropped from the indirect transaction queue (The pending transaction list).
* ``MacPromiscRx``: A packet has been received by this device, has been passed up from the physical layer and is being forwarded up the local protocol stack.
* ``MacRx``: A packet has been received by this device, has been passed up from the physical layer and is being forwarded up the local protocol stack.
* ``MacRxDrop``: Trace source indicating a packet was received, but dropped before being forwarded up the stack.
* ``Sniffer``: Trace source simulating a non-promiscuous packet sniffer attached to the device.
* ``PromiscSniffer``: Trace source simulating a promiscuous packet sniffer attached to the device.
* ``MacStateValue``: Shows the current state of LrWpan MAC layer.
* ``MacIncSuperframeStatus``: When using the beacon mode, shows the period status of the incoming superframe.
* ``MacOutSuperframeStatus``: When using the beacon mode, shows the period status of the outgoing superframe.
* ``MacState``: The state of LrWpan Mac.
* ``MacSentPkt``: Trace source reporting some information about the sent packet.
* ``IfsEnd``: Reports the end of an Interframe space period (IFS).

Examples and Tests
------------------

The following examples have been written, which can be found in ``src/lr-wpan/examples/``:

* ``lr-wpan-data.cc``:  A simple example showing end-to-end data transfer.
* ``lr-wpan-error-distance-plot.cc``:  An example to plot variations of the packet success ratio as a function of distance.
* ``lr-wpan-per-plot.cc``: An example to plot the theoretical and experimental packet error rate (PER) as a function of receive signal.
* ``lr-wpan-error-model-plot.cc``:  An example to test the phy.
* ``lr-wpan-packet-print.cc``:  An example to print out the MAC header fields.
* ``lr-wpan-phy-test.cc``:  An example to test the phy.
* ``lr-wpan-ed-scan.cc``:  Simple example showing the use of energy detection (ED) scan in the MAC.
* ``lr-wpan-active-scan.cc``:  A simple example showing the use of an active scan in the MAC.
* ``lr-wpan-mlme.cc``: Demonstrates the use of lr-wpan beacon mode. Nodes use a manual association (i.e. No bootstrap) in this example.
* ``lr-wpan-bootstrap.cc``:  Demonstrates the use of scanning and association working together to initiate a PAN.
* ``lr-wpan-orphan-scan.cc``: Demonstrates the use of an orphan scanning in a simple network joining procedure.


In particular, the module enables a very simplified end-to-end data
transfer scenario, implemented in ``lr-wpan-data.cc``.  The figure
shows a sequence of events that are triggered when the MAC receives
a DataRequest from the higher layer.  It invokes a Clear Channel
Assessment (CCA) from the PHY, and if successful, sends the frame
down to the PHY where it is transmitted over the channel and results
in a DataIndication on the peer node.

.. _fig-lr-wpan-data:

.. figure:: figures/lr-wpan-data-example.*

    Data example for simple LR-WPAN data transfer end-to-end

The example ``lr-wpan-error-distance-plot.cc`` plots the packet success
ratio (PSR) as a function of distance, using the default LogDistance
propagation loss model and the 802.15.4 error model.  The channel (default 11),
packet size (default PSDU 20 bytes = 11 bytes MAC header + data payload), transmit power (default 0 dBm)
and Rx sensitivity (default -106.58 dBm) can be varied by command line arguments.
The program outputs a file named ``802.15.4-psr-distance.plt``.
Loading this file into gnuplot yields a file ``802.15.4-psr-distance.eps``, which can
be converted to pdf or other formats.  The following image shows the output
of multiple runs using different Rx sensitivity values. A higher Rx sensitivity (lower dBm) results
in a increased communication distance but also makes the radio susceptible to more interference from
surrounding devices.

.. _fig-802-15-4-psr-distance:

.. figure:: figures/802-15-4-psr-distance.*

    Default output of the program ``lr-wpan-error-distance-plot.cc``

The following tests have been written, which can be found in ``src/lr-wpan/tests/``:

* ``lr-wpan-ack-test.cc``:  Check that acknowledgments are being used and issued in the correct order.
* ``lr-wpan-cca-test.cc``: Test the behavior of CCA under specific circumstances (Hidden terminal and CCA vulnerable windows)
* ``lr-wpan-collision-test.cc``:  Test correct reception of packets with interference and collisions.
* ``lr-wpan-ed-test.cc``: Test the energy detection (ED) capabilities of the Lr-Wpan implementation.
* ``lr-wpan-error-model-test.cc``:  Check that the error model gives predictable values.
* ``lr-wpan-ifs-test.cc``:  Check that the Intraframe Spaces (IFS) are being used and issued in the correct order.
* ``lr-wpan-mac-test.cc``: Test various MAC capabilities such different types of scanning and RX_ON_WHEN_IDLE.
* ``lr-wpan-packet-test.cc``:  Test the 802.15.4 MAC header/trailer classes
* ``lr-wpan-pd-plme-sap-test.cc``:  Test the PLME and PD primitives from Lr-wpan's PHY.
* ``lr-wpan-spectrum-value-helper-test.cc``:  Test that the conversion between power (expressed as a scalar quantity) and spectral power, and back again, falls within a 25% tolerance across the range of possible channels and input powers.
* ``lr-wpan-slotted-csmaca-test.cc``:  Test the transmission and deferring of data packets in the Contention Access Period (CAP) for the slotted CSMA/CA (beacon-enabled mode).

The test suite ``lr-wpan-cca-test.cc`` demenostrate some known CCA behaviors.
The test suite includes a test that demonstrates the well known hidden terminal problem. The second test
included in the test suite shows a known vulnerability window (192us) in CCA that can cause a false positive identification
of channel as IDLE caused by the turnAround delay between the CCA and the actual transmission of the frame.

Validation
----------

The model has not been validated against real hardware.  The error model
has been validated against the data in IEEE Std 802.15.4-2006,
section E.4.1.7 (Figure E.2). The MAC behavior (CSMA backoff) has been
validated by hand against expected behavior.  The below plot is an example
of the error model validation and can be reproduced by running
``lr-wpan-error-model-plot.cc``:

.. _fig-802-15-4-ber:

.. figure:: figures/802-15-4-ber.*

    Default output of the program ``lr-wpan-error-model-plot.cc``

References
----------

[`1 <https://ieeexplore.ieee.org/document/1700009>`_] Wireless Medium Access Control (MAC) and Physical Layer (PHY) Specifications for Low-Rate Wireless Personal Area Networks (WPANs), IEEE Computer Society, IEEE Std 802.15.4-2006, 8 September 2006.

[`2 <https://ieeexplore.ieee.org/document/6012487>`_] IEEE Standard for Local and metropolitan area networks--Part 15.4: Low-Rate Wireless Personal Area Networks (LR-WPANs)," in IEEE Std 802.15.4-2011 (Revision of IEEE Std 802.15.4-2006) , vol., no., pp.1-314, 5 Sept. 2011, doi: 10.1109/IEEESTD.2011.6012487.

[`3 <https://www.mdpi.com/2079-9292/11/24/4090>`_] J. Zheng and Myung J. Lee, "A comprehensive performance study of IEEE 802.15.4," Sensor Network Operations, IEEE Press, Wiley Interscience, Chapter 4, pp. 218-237, 2006.

[`4 <https://dl.acm.org/doi/10.1145/3442555.3442574>`_] Alberto Gallegos Ramonet and Taku Noguchi. 2020. LR-WPAN: Beacon Enabled Direct Transmissions on Ns-3. In 2020 the 6th International Conference on Communication and Information Processing (ICCIP 2020). Association for Computing Machinery, New York, NY, USA, 115–122. https://doi.org/10.1145/3442555.3442574.

[`5 <https://www.mdpi.com/2079-9292/11/24/4090>`_] Gallegos Ramonet, A.; Noguchi, T. Performance Analysis of IEEE 802.15.4 Bootstrap Process. Electronics 2022, 11, 4090. https://doi.org/10.3390/electronics11244090.

