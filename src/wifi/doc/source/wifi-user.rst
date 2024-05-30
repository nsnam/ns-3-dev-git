.. include:: replace.txt
.. highlight:: cpp

.. _sec-wifi-user-doc:

++++++++++++++++++
User Documentation
++++++++++++++++++

Using the WifiNetDevice
***********************

The modularity provided by the implementation makes low-level configuration of
the WifiNetDevice powerful but complex. For this reason, we provide some helper
classes to perform common operations in a simple matter, and leverage the |ns3|
attribute system to allow users to control the parameterization of the underlying
models.

Users who use the low-level |ns3| API and who wish to add a WifiNetDevice to
their node must create an instance of a WifiNetDevice, plus a number of
constituent objects, and bind them together appropriately (the WifiNetDevice is
very modular in this regard, for future extensibility). At the low-level API,
this can be done with about 20 lines of code (see ``ns3::WifiHelper::Install``,
and ``ns3::YansWifiPhyHelper::Create``). They also must create, at some point, a
Channel, which also contains a number of constituent objects (see
``ns3::YansWifiChannelHelper::Create``).

However, a few helpers are available for users to add these devices and channels
with only a few lines of code, if they are willing to use defaults, and the
helpers provide additional API to allow the passing of attribute values to
change default values.  Commonly used attribute values are listed in the
Attributes section.  The scripts in ``examples/wireless`` can be browsed to
see how this is done.  Next, we describe the common steps to create a WifiNetDevice
from the bottom layer (Channel) up to the device layer (WifiNetDevice).

To create a WifiNetDevice, users need to follow these steps:

* Decide on which physical layer framework, the ``SpectrumWifiPhy`` or
  ``YansWifiPhy``, to use.  This will affect which Channel and Phy type to use.
* Configure the Channel: Channel takes care of getting signal
  from one device to other devices on the same Wi-Fi channel.
  The main configurations of WifiChannel are propagation loss model and propagation delay model.
* Configure the WifiPhy: WifiPhy takes care of actually sending and receiving wireless
  signal from Channel.  Here, WifiPhy decides whether each frame will be successfully
  decoded or not depending on the received signal strength and noise.  Thus, the main
  configuration of WifiPhy is the error rate model, which is the one that actually
  calculates the probability of successfully decoding the frame based on the signal.
* Configure WifiMac: this step is more related to the architecture and device level.
  The users configure the wifi architecture (i.e. ad-hoc or ap-sta) and whether QoS (802.11e),
  HT (802.11n) and/or VHT (802.11ac) and/or HE (802.11ax) features are supported or not.
* Create WifiDevice: at this step, users configure the desired wifi standard
  (e.g. **802.11b**, **802.11g**, **802.11a**, **802.11n**, **802.11ac** or **802.11ax**) and rate control algorithm.
* Configure mobility: finally, a mobility model is (usually) required before WifiNetDevice
  can be used; even if the devices are stationary, their relative positions
  are needed for propagation loss calculations.

The following sample code illustrates a typical configuration using mostly
default values in the simulator, and infrastructure mode:

.. sourcecode:: cpp

  NodeContainer wifiStaNode;
  wifiStaNode.Create(10);   // Create 10 station node objects
  NodeContainer wifiApNode;
  wifiApNode.Create(1);   // Create 1 access point node object

  // Create a channel helper and phy helper, and then create the channel
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
  phy.SetChannel(channel.Create());

  // Create a WifiMacHelper, which is reused across STA and AP configurations
  WifiMacHelper mac;

  // Create a WifiHelper, which will use the above helpers to create
  // and install Wifi devices.  Configure a Wifi standard to use, which
  // will align various parameters in the Phy and Mac to standard defaults.
  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211n);
  // Declare NetDeviceContainers to hold the container returned by the helper
  NetDeviceContainer wifiStaDevices;
  NetDeviceContainer wifiApDevice;

  // Perform the installation
  mac.SetType("ns3::StaWifiMac");
  wifiStaDevices = wifi.Install(phy, mac, wifiStaNodes);
  mac.SetType("ns3::ApWifiMac");
  wifiApDevice = wifi.Install(phy, mac, wifiApNode);

At this point, the 11 nodes have Wi-Fi devices configured, attached to a
common channel.  The rest of this section describes how additional
configuration may be performed.

YansWifiChannelHelper
=====================

The YansWifiChannelHelper has an unusual name. Readers may wonder why it is
named this way. The reference is to the `yans simulator
<https://dl.acm.org/doi/pdf/10.1145/1190455.1190467?download=true>`_ from which this model is taken. The
helper can be used to create a YansWifiChannel with a default PropagationLoss and
PropagationDelay model.

Users will typically type code such as:

.. sourcecode:: cpp

  YansWifiChannelHelper wifiChannelHelper = YansWifiChannelHelper::Default();
  Ptr<Channel> wifiChannel = wifiChannelHelper.Create();

to get the defaults.  Specifically, the default is a channel model with a
propagation delay equal to a constant, the speed of light (``ns3::ConstantSpeedPropagationDelayModel``),
and a propagation loss based on a default log distance model (``ns3::LogDistancePropagationLossModel``), using a default exponent of 3.
Please note that the default log distance model is configured with a reference
loss of 46.6777 dB at reference distance of 1m.  The reference loss of 46.6777 dB
was calculated using Friis propagation loss model at 5.15 GHz.  The reference loss
must be changed if **802.11b**, **802.11g**, **802.11n** (at 2.4 GHz) or **802.11ax** (at 2.4 GHz) are used since they operate at 2.4 Ghz.

Note the distinction above in creating a helper object vs. an actual simulation
object.  In |ns3|, helper objects (used at the helper API only) are created on
the stack (they could also be created with operator new and later deleted).
However, the actual |ns3| objects typically inherit from ``class ns3::Object``
and are assigned to a smart pointer.  See the chapter in the |ns3| manual for
a discussion of the |ns3| object model, if you are not familiar with it.

The following two methods are useful when configuring YansWifiChannelHelper:

* ``YansWifiChannelHelper::AddPropagationLoss`` adds a PropagationLossModel; if one or more PropagationLossModels already exist, the new model is chained to the end
* ``YansWifiChannelHelper::SetPropagationDelay`` sets a PropagationDelayModel (not chainable)

YansWifiPhyHelper
=================

Physical devices (base class ``ns3::WifiPhy``) connect to ``ns3::YansWifiChannel`` models in
|ns3|.  We need to create WifiPhy objects appropriate for the YansWifiChannel; here
the ``YansWifiPhyHelper`` will do the work.

The YansWifiPhyHelper class configures an object factory to create instances of
a ``YansWifiPhy`` and adds some other objects to it, including possibly a
supplemental ErrorRateModel and a pointer to a MobilityModel. The user code is
typically:

.. sourcecode:: cpp

  YansWifiPhyHelper wifiPhyHelper;
  wifiPhyHelper.SetChannel(wifiChannel);

The default YansWifiPhyHelper is configured with TableBasedErrorRateModel
(``ns3::TableBasedErrorRateModel``). You can change the error rate model by
calling the ``YansWifiPhyHelper::SetErrorRateModel`` method.

Optionally, if pcap tracing is needed, a user may use the following
command to enable pcap tracing:

.. sourcecode:: cpp

  WifiPhyHelper::SetPcapDataLinkType(enum SupportedPcapDataLinkTypes dlt)

|ns3| supports RadioTap and Prism tracing extensions for 802.11.

For MLD devices, it is also possible to select one of these PCAP capture type:

.. sourcecode:: cpp

  WifiPhyHelper::SetPcapCaptureType(PcapCaptureType type)

|ns3| supports three PCAP capture types for MLD devices:
* a single PCAP file for the device, regardless of PHYs and links

.. sourcecode:: cpp

  phyHelper.SetPcapCaptureType(WifiPhyHelper::PcapCaptureType::PCAP_PER_DEVICE);

* a PCAP file generated per PHY (default behavior)

.. sourcecode:: cpp

  phyHelper.SetPcapCaptureType(WifiPhyHelper::PcapCaptureType::PCAP_PER_PHY);

* a PCAP file generated per link

.. sourcecode:: cpp

  phyHelper.SetPcapCaptureType(WifiPhyHelper::PcapCaptureType::PCAP_PER_LINK);

The PCAP files will always be generated with the suffix of nodeId-deviceId.pcap, unless the user has enabled
object names on either the Node or the NetDevice, in which case the string name will be used. When PCAP files
are generated per PHY, that suffix is nodeId-deviceId-linkId.pcap. When PCAP files are generated per link, the
suffix is nodeId-deviceId-phyId.pcap.

When PCAP files are generated per link, if there is no generated PCAP for a given link, it means
no packet has been transmitted nor received on that link. Since PHYs of non-AP MLDs may swap their
links during setup, the link ID appended to the PCAP file might be higher than the highest PHY ID.

In case of SLD devices, the configuration of the capture type has no impact since a single PCAP file
will always be generated per device.

Note that we haven't actually created any WifiPhy objects yet; we've just
prepared the YansWifiPhyHelper by telling it which channel it is connected to.
The Phy objects are created in the next step.

In order to enable 802.11n/ac/ax/be MIMO, the number of antennas as well as the number of supported spatial streams need to be configured.
For example, this code enables MIMO with 2 antennas and 2 spatial streams:

.. sourcecode:: cpp

 wifiPhyHelper.Set("Antennas", UintegerValue(2));
 wifiPhyHelper.Set("MaxSupportedTxSpatialStreams", UintegerValue(2));
 wifiPhyHelper.Set("MaxSupportedRxSpatialStreams", UintegerValue(2));

It is also possible to configure less streams than the number of antennas in order to benefit from diversity gain, and to define different MIMO capabilities for downlink and uplink.
For example, this code configures a node with 3 antennas that supports 2 spatial streams in downstream and 1 spatial stream in upstream:

.. sourcecode:: cpp

 wifiPhyHelper.Set("Antennas", UintegerValue(3));
 wifiPhyHelper.Set("MaxSupportedTxSpatialStreams", UintegerValue(2));
 wifiPhyHelper.Set("MaxSupportedRxSpatialStreams", UintegerValue(1));

802.11n PHY layer can support both 20 (default) or 40 MHz channel width, and 802.11ac/ax PHY layer can use either 20, 40, 80 (default) or 160 MHz channel width.  See below for further documentation on setting the frequency, channel width, and channel number.

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211ac);
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue("VhtMcs9"),
                               "ControlMode", StringValue("VhtMcs0"));

  //Install PHY and MAC
  Ssid ssid = Ssid("ns3-wifi");
  WifiMacHelper mac;

  mac.SetType("ns3::StaWifiMac",
               "Ssid", SsidValue(ssid),
               "ActiveProbing", BooleanValue(false));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install(phy, mac, wifiStaNode);

  mac.SetType("ns3::ApWifiMac",
               "Ssid", SsidValue(ssid));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install(phy, mac, wifiApNode);

.. _Channel-settings:

Channel, frequency, channel width, and band configuration
=========================================================

There is a unique ``ns3::WifiPhy`` attribute, named ``ChannelSettings``, that
enables to set channel number, channel width, frequency band and primary20 index
for each segment all together, in order to eliminate the possibility of inconsistent settings.
The ``ChannelSettings`` attribute can be set in a number of ways (see below) by
providing either a StringValue object or an AttributeContainerValue object:

* Defining a StringValue object to set the ``ChannelSettings`` attribute

.. sourcecode:: cpp

  StringValue value("{38, 40, BAND_5GHZ, 0}");

* Defining an AttributeContainerValue object to set the ``ChannelSettings`` attribute

.. sourcecode:: cpp

  AttributeContainerValue<TupleValue<UintegerValue, UintegerValue, EnumValue, UintegerValue>, ';'> value;
  value.Set(WifiPhy::ChannelSegments{{38, 40, WIFI_PHY_BAND_5GHZ, 0}});

In both cases, the operating channel will be channel 38 in the 5 GHz band, which
has a width of 40 MHz, and the primary20 channel will be the 20 MHz subchannel
with the lowest center frequency (index 0).

The operating channel settings can then be configured in a number of ways:

* by setting global configuration default; e.g.

.. sourcecode:: cpp

  Config::SetDefault("ns3::WifiPhy::ChannelSettings", StringValue("{38, 40, BAND_5GHZ, 0}"));

* by setting an attribute value in the helper; e.g.

.. sourcecode:: cpp

  AttributeContainerValue<TupleValue<UintegerValue, UintegerValue, EnumValue, UintegerValue>, ';'> value;
  value.Set(WifiPhy::ChannelSegments{{38, 40, WIFI_PHY_BAND_5GHZ, 0}});
  YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default();
  wifiPhyHelper.Set("ChannelSettings", value);

* by setting the WifiHelper::SetStandard(enum WifiStandard) method; and

* by performing post-installation configuration of the option, either
  via a Ptr to the WifiPhy object, or through the Config namespace; e.g.:

.. sourcecode:: cpp

  Config::Set("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
              StringValue("{38, 40, BAND_5GHZ, 0}"));

This section provides guidance on how to properly configure these settings.

WifiHelper::SetStandard()
+++++++++++++++++++++++++

``WifiHelper::SetStandard()`` is a method required to set various parameters
in the Mac and Phy to standard values, but also to check that the channel
settings as described above are allowed. For instance, a channel in the 2.4 GHz
band cannot be configured if the standard is 802.11ac, or a channel in the 6 GHz
band can only be configured if the standard is 802.11ax (or beyond).

``WifiHelper::SetStandard()`` can also be called by passing a string with the
standard name. For example any of the strings "802.11ax", "11ax" or "HE" can
be passed in order to set the standard as ``WIFI_STANDARD_80211ax``.

The following values for WifiStandard are defined in
``src/wifi/model/wifi-standards.h``:

.. sourcecode:: cpp

  WIFI_STANDARD_80211a,
  WIFI_STANDARD_80211b,
  WIFI_STANDARD_80211g,
  WIFI_STANDARD_80211p,
  WIFI_STANDARD_80211n,
  WIFI_STANDARD_80211ac,
  WIFI_STANDARD_80211ax,
  WIFI_STANDARD_80211be

By default, the WifiHelper (the typical use case for WifiPhy creation) will
configure the WIFI_STANDARD_80211ax standard by default.  Other values
for standards should be passed explicitly to the WifiHelper object.

If user has not already configured ChannelSettings when SetStandard is called,
the user obtains default values, as described next.

Default settings for the operating channel
++++++++++++++++++++++++++++++++++++++++++

Not all the parameters in the channel settings have to be set to a valid value,
but they can be left unspecified, in which case default values are substituted
as soon as the WifiStandard is set. Here are the rules (applied in the given order):

* If the band is unspecified (i.e., it is set to WIFI_PHY_BAND_UNSPECIFIED or
  "BAND_UNSPECIFIED"), the default band for the configured standard is set
  (5 GHz band for 802.11{a, ac, ax, be, p} and 2.4 GHz band for all the others).

* If both the channel width and the channel number are unspecified (i.e., they
  are set to zero), the default channel width for the configured standard and
  band is set (22 MHz for 802.11b, 10 MHz for 802.11p, 80 MHz for 802.11ac,
  for 802.11ax and for 802.11be if the band is 5 GHz, and 20 MHz for all other cases).

* If the channel width is unspecified but the channel number is valid, the settings
  are valid only if there exists a unique channel with the given number for the
  configured standard and band, in which case the channel width is set to the width
  of such unique channel. Otherwise, the simulation aborts.

* If the channel number is unspecified (i.e., it is set to zero), the default
  channel number for the configured standard, band and channel width is used
  (the default channel number is the first one in the list of channels that can
  be used with the configured standard, band and channel width)

Following are a few examples to clarify these rules:

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211ac);
  YansWifiPhyHelper phyHelper;
  phyHelper.Set("ChannelSettings", StringValue("{58, 0, BAND_5GHZ, 0}"));
  // channel width unspecified
  // -> it is set to 80 MHz (width of channel 58)

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211n);
  YansWifiPhyHelper phyHelper;
  phyHelper.Set("ChannelSettings", StringValue("{0, 40, BAND_5GHZ, 0}"));
  // channel number unspecified
  // -> it is set to channel 38 (first 40 MHz channel in the 5GHz band)

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211ax);
  YansWifiPhyHelper phyHelper;
  phyHelper.Set("ChannelSettings", StringValue("{0, 0, BAND_2_4GHZ, 0}"));
  // both channel number and width unspecified
  // -> width set to 20 MHz (default width for 802.11ax in the 2.4 GHZ band)
  // -> channel number set to 1 (first 20 MHz channel in the 2.4 GHz band)

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211a);
  YansWifiPhyHelper phyHelper;
  phyHelper.Set("ChannelSettings", StringValue("{0, 0, BAND_UNSPECIFIED, 0}"));
  // band, channel number and width unspecified
  // -> band is set to WIFI_PHY_BAND_5GHZ (default band for 802.11a)
  // -> width set to 20 MHz (default width for 802.11a in the 5 GHZ band)
  // -> channel number set to 36 (first 20 MHz channel in the 5 GHz band)


The default value for the ChannelSettings attribute leaves all the parameters
unspecified, except for the primary20 index, which is equal to zero.

WifiPhy::Frequency
++++++++++++++++++

The configured WifiPhy channel center frequency can be got via the attribute
``Frequency`` in the class ``WifiPhy``.  It is expressed in units of MHz.

Note that this is a change in definition from ns-3.25 and earlier releases,
where this attribute referred to the start of the overall frequency band
on which the channel resides, not the specific channel center frequency.

WifiPhy::ChannelWidth
+++++++++++++++++++++

The configured WifiPhy channel width can be got via the attribute ``ChannelWidth``
in the class ``WifiPhy``.  It is expressed in units of MHz.

WifiPhy::ChannelNumber
++++++++++++++++++++++

Several channel numbers are defined and well-known in practice.  However,
valid channel numbers vary by geographical region around the world, and
there is some overlap between the different standards.

The configured WifiPhy channel number can be got via the attribute ``ChannelNumber``
in the class ``WifiPhy``.

In |ns3|, a ChannelNumber may be defined or unknown.  These terms
are not found in the code; they are just used to describe behavior herein.

If a ChannelNumber is defined, it means that WifiPhy has stored a
map of ChannelNumber to the center frequency and channel width commonly
known for that channel in practice.  For example:

* Channel 1, when IEEE 802.11b is configured, corresponds to a channel
  width of 22 MHz and a center frequency of 2412 MHz.

* Channel 36, when IEEE 802.11n is configured at 5 GHz, corresponds to
  a channel width of 20 MHz and a center frequency of 5180 MHz.

The following channel numbers are well-defined for 2.4 GHz standards:

* channels 1-14 with ChannelWidth of 22 MHz for 802.11b
* channels 1-14 with ChannelWidth of 20 MHz for 802.11n-2.4GHz and 802.11g

The following channel numbers are well-defined for 5 GHz standards:

.. table:: 5 GHz channel numbers
   :widths: 20 70

   +------------------+-------------------------------------------+
   | ``ChannelWidth`` | ``ChannelNumber``                         |
   +------------------+-------------------------------------------+
   | 20 MHz           | 36, 40, 44, 48, 52, 56, 60, 64, 100,      |
   |                  | 104, 108, 112, 116, 120, 124,             |
   |                  | 128, 132, 136, 140, 144,                  |
   |                  | 149, 153, 161, 165, 169                   |
   +------------------+-------------------------------------------+
   | 40 MHz           | 38, 46, 54, 62, 102, 110, 118, 126,       |
   |                  | 134, 142, 151, 159                        |
   +------------------+-------------------------------------------+
   | 80 MHz           | 42, 58, 106, 122, 138, 155                |
   +------------------+-------------------------------------------+
   | 160 MHz          | 50, 114                                   |
   +------------------+-------------------------------------------+
   | 10 MHz (802.11p) | 172, 174, 176, 178, 180, 182, 184         |
   +------------------+-------------------------------------------+
   | 5 MHz (802.11p)  | 171, 173, 175, 177, 179, 181, 183         |
   +------------------+-------------------------------------------+


The following channel numbers are well-defined for 6 GHz standards (802.11ax only):

.. table:: 6 GHz channel numbers
   :widths: 20 70

   +------------------+-------------------------------------------+
   | ``ChannelWidth`` | ``ChannelNumber``                         |
   +------------------+-------------------------------------------+
   | 20 MHz           | 1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41,  |
   |                  | 45, 49, 53, 57, 61, 65, 69, 73, 77, 81,   |
   |                  | 85, 89, 93, 97, 101, 105, 109, 113, 117,  |
   |                  | 121, 125, 129, 133, 137, 141, 145, 149,   |
   |                  | 153, 157, 161, 165, 169, 173, 177, 181,   |
   |                  | 185, 189, 193, 197, 201, 205, 209, 213,   |
   |                  | 217, 221, 225, 229, 233                   |
   +------------------+-------------------------------------------+
   | 40 MHz           | 3, 11, 19, 27, 35, 43, 51, 59, 67, 75,    |
   |                  | 83, 91, 99, 107, 115, 123, 131, 139, 147, |
   |                  | 155, 163, 171, 179, 187, 195, 203, 211,   |
   |                  | 219, 227                                  |
   +------------------+-------------------------------------------+
   | 80 MHz           | 7, 23, 39, 55, 71, 87, 103, 119, 135,     |
   |                  | 151, 167, 183, 199, 215                   |
   +------------------+-------------------------------------------+
   | 160 MHz          | 15, 47, 79, 111, 143, 175, 207            |
   +------------------+-------------------------------------------+


The channel number may be set either before or after creation of the
WifiPhy object.

If an unknown channel number (other than zero) is configured, the
simulator will exit with an error; for instance, such as:

.. sourcecode:: cpp

  Ptr<WifiPhy> wifiPhy = ...;
  wifiPhy->SetAttribute("ChannelSettings", StringValue("{1321, 20, BAND_5GHZ, 0}"));

The known channel numbers are defined in the implementation file
``src/wifi/model/wifi-phy-operating-channel.cc``. Of course, this file may be edited
by users to extend to additional channel numbers.

If a known channel number is configured against an incorrect value
of the WifiPhyStandard, the simulator will exit with an error; for instance,
such as:

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211n);
  ...
  Ptr<WifiPhy> wifiPhy = ...;
  wifiPhy->SetAttribute("ChannelSettings", StringValue("{14, 20, BAND_5GHZ, 0}"));

In the above, while channel number 14 is well-defined in practice for 802.11b
only, it is for 2.4 GHz band, not 5 GHz band.

WifiPhy::Primary20MHzIndex
++++++++++++++++++++++++++

The configured WifiPhy primary 20MHz channel index can be got via the attribute
``Primary20MHzIndex`` in the class ``WifiPhy``.

Non-contiguous 160 MHz operating channel configuration (80+80 MHz)
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

An 80+80 MHz operating channel can be configured by setting each 80 MHz channel
for each segment. For example, the following example code snippet configures an
non-contiguous 160 MHz channel where the first segment operates on channel 42
and the second segment operates on channel 106:

::

  Ptr<WifiPhy> wifiPhy = ...;
  wifiPhy->SetAttribute("ChannelSettings", StringValue("{42, 80, BAND_5GHZ, 0};{106, 80, BAND_5GHZ, 0}"));

It is still possible to have channel numbers set to zero in order to select the default 80+80 MHz channel:

::

  Ptr<WifiPhy> wifiPhy = ...;
  wifiPhy->SetAttribute("ChannelSettings", StringValue("{0, 80, BAND_UNSPECIFIED, 0};{0, 80, BAND_UNSPECIFIED, 0}"));

which results in the use of channels 42 and 106.

Note that only the primary 20 MHz channel index of the first specified segment is considered.
80+80MHz requires the use of ``SpectrumWifiPhy``.

Order of operation issues
+++++++++++++++++++++++++

Channel settings can be configured either before or after the wifi standard.
If the channel settings are configured before the wifi standard, the channel
settings are stored and applied when the wifi standard is configured.
Otherwise, they are applied immediately.

The wifi standard can be configured only once, i.e., it is not possible to
change standard during a simulation. It is instead possible to change the
channel settings at any time.


SpectrumWifiPhyHelper
=====================

The API for this helper closely tracks the API of the YansWifiPhyHelper,
with the exception that a channel of type ``ns3::SpectrumChannel`` instead
of type ``ns3::YansWifiChannel`` must be used with it.

Its API has been extended for 802.11be multi-link and EMLSR in order to
attach multiple spectrum channels to a same PHY. For that purpose, a user
may use the following command to attach a spectrum channel to the PHY objects
that will be created upon a call to ``ns3::WifiHelper::Install``:

.. sourcecode:: cpp

  SpectrumWifiPhyHelper::AddChannel(const Ptr<SpectrumChannel> channel,
                                    const FrequencyRange& freqRange)

where FrequencyRange is a structure that contains the start and stop frequencies
expressed in MHz which corresponds to the spectrum portion that is covered by the channel.

WifiMacHelper
=============

The next step is to configure the MAC model. We use WifiMacHelper to accomplish this.
WifiMacHelper takes care of both the MAC low model and MAC high model, and configures an object factory to create instances of a ``ns3::WifiMac``.
It is used to configure MAC parameters like type of MAC, and to select whether 802.11/WMM-style QoS and/or 802.11n-style High Throughput (HT)
and/or 802.11ac-style Very High Throughput (VHT) support and/or 802.11ax-style High Efficiency (HE) support are/is required.

By default, it creates an ad-hoc MAC instance that does not have 802.11e/WMM-style QoS nor 802.11n-style High Throughput (HT)
nor 802.11ac-style Very High Throughput (VHT) nor 802.11ax-style High Efficiency (HE) support enabled.

For example the following user code configures a non-QoS and non-HT/non-VHT/non-HE MAC that
will be a non-AP STA in an infrastructure network where the AP has SSID ``ns-3-ssid``:

.. sourcecode:: cpp

    WifiMacHelper wifiMacHelper;
    Ssid ssid = Ssid("ns-3-ssid");
    wifiMacHelper.SetType("ns3::StaWifiMac",
                         "Ssid", SsidValue(ssid),
                         "ActiveProbing", BooleanValue(false));

The following code shows how to create an AP with QoS enabled:

.. sourcecode:: cpp

  WifiMacHelper wifiMacHelper;
  wifiMacHelper.SetType("ns3::ApWifiMac",
                        "Ssid", SsidValue(ssid),
                        "QosSupported", BooleanValue(true),
                        "BeaconGeneration", BooleanValue(true),
                        "BeaconInterval", TimeValue(Seconds(2.5)));

To create ad-hoc MAC instances, simply use ``ns3::AdhocWifiMac`` instead of ``ns3::StaWifiMac`` or ``ns3::ApWifiMac``.

With QoS-enabled MAC models it is possible to work with traffic belonging to
four different Access Categories (ACs): **AC_VO** for voice traffic,
**AC_VI** for video traffic, **AC_BE** for best-effort
traffic and **AC_BK** for background traffic.

When selecting **802.11n** as the desired wifi standard, both 802.11e/WMM-style QoS and 802.11n-style High Throughput (HT) support gets enabled.
Similarly when selecting **802.11ac** as the desired wifi standard, 802.11e/WMM-style QoS, 802.11n-style High Throughput (HT) and 802.11ac-style Very High Throughput (VHT)
support gets enabled. And when selecting **802.11ax** as the desired wifi standard, 802.11e/WMM-style QoS, 802.11n-style High Throughput (HT),
802.11ac-style Very High Throughput (VHT) and 802.11ax-style High Efficiency (HE) support gets enabled.

For MAC instances that have QoS support enabled, the ``ns3::WifiMacHelper`` can be also used to set:

* block ack threshold (number of packets for which block ack mechanism should be used);
* block ack inactivity timeout.

For example the following user code configures a MAC that will be a non-AP STA with QoS enabled and a block ack threshold for AC_BE set to 2 packets,
in an infrastructure network where the AP has SSID ``ns-3-ssid``:

.. sourcecode:: cpp

    WifiMacHelper wifiMacHelper;
    Ssid ssid = Ssid("ns-3-ssid");
    wifiMacHelper.SetType("ns3::StaWifiMac",
                         "Ssid", SsidValue(ssid),
                         "QosSupported", BooleanValue(true),
                         "BE_BlockAckThreshold", UintegerValue(2),
                         "ActiveProbing", BooleanValue(false));

For MAC instances that have 802.11n-style High Throughput (HT) and/or 802.11ac-style Very High Throughput (VHT) and/or 802.11ax-style High Efficiency (HE) support enabled,
the ``ns3::WifiMacHelper`` can be also used to set:

* MSDU aggregation parameters for a particular Access Category (AC) in order to use 802.11n/ac A-MSDU feature;
* MPDU aggregation parameters for a particular Access Category (AC) in order to use 802.11n/ac A-MPDU feature.

By default, MSDU aggregation feature is disabled for all ACs and MPDU aggregation is enabled for AC_VI and AC_BE, with a maximum aggregation size of 65535 bytes.

For example the following user code configures a MAC that will be a non-AP STA with HT and QoS enabled, MPDU aggregation enabled for AC_VO with a maximum aggregation size of 65535 bytes, and MSDU aggregation enabled for AC_BE with a maximum aggregation size of 7935 bytes,
in an infrastructure network where the AP has SSID ``ns-3-ssid``:

.. sourcecode:: cpp

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    WifiMacHelper wifiMacHelper;
    Ssid ssid = Ssid("ns-3-ssid");
    wifiMacHelper.SetType("ns3::StaWifiMac",
                          "Ssid", SsidValue(ssid),
                          "VO_MaxAmpduSize", UintegerValue(65535),
                          "BE_MaxAmsduSize", UintegerValue(7935),
                          "ActiveProbing", BooleanValue(false));

802.11ax APs support sending multi-user frames via DL OFDMA and UL OFDMA if a Multi-User Scheduler is
aggregated to the wifi MAC(by default no scheduler is aggregated). WifiMacHelper enables to aggregate
a Multi-User Scheduler to an AP and set its parameters:

.. sourcecode:: cpp

    WifiMacHelper wifiMacHelper;
    wifiMacHelper.SetMultiUserScheduler("ns3::RrMultiUserScheduler",
                                        "EnableUlOfdma", BooleanValue(true),
                                        "EnableBsrp", BooleanValue(false));

The Ack Manager is in charge of selecting the acknowledgment method among the three
available methods(see section :ref:`wifi-mu-ack-sequences` ). The default ack manager
enables to select the acknowledgment method, e.g.:

.. sourcecode:: cpp

    Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                       EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));

Selection of the Access Category (AC)
+++++++++++++++++++++++++++++++++++++

Since ns-3.26, the QosTag is no longer used to assign a user priority to an MSDU.
Instead, the selection of the Access Category (AC) for an MSDU is based on the
value of the DS field in the IP header of the packet (ToS field in case of IPv4,
Traffic Class field in case of IPv6). Details on how to set the ToS field of IPv4
packets are given in the :ref:`Type-of-service` section of the documentation. In
summary, users can create an address of type :cpp:class:`ns3::InetSocketAddress`
with the desired type of service value and pass it to the application helpers:

.. sourcecode:: cpp

    InetSocketAddress destAddress(ipv4Address, udpPort);
    OnOffHelper onoff("ns3::UdpSocketFactory", destAddress);
    onoff.SetAttribute("Tos", UintegerValue(tos));

Mapping the values of the DS field onto user priorities is performed similarly to the
Linux mac80211 subsystem. Basically, the :cpp:func:`ns3::WifiNetDevice::SelectQueue()`
method sets the user priority (UP) of an MSDU to the three most significant
bits of the DS field. The Access Category is then determined based on the user priority
according to the following table:

===  ===============
UP   Access Category
===  ===============
 7     AC_VO
 6     AC_VO
 5     AC_VI
 4     AC_VI
 3     AC_BE
 0     AC_BE
 2     AC_BK
 1     AC_BK
===  ===============

ToS and DSCP values map onto user priorities and access categories according
to the following table.

============  ============  ==  ===============
DiffServ PHB  ToS (binary)  UP  Access Category
============  ============  ==  ===============
EF            101110xx      5   AC_VI
AF11          001010xx      1   AC_BK
AF21          010010xx      2   AC_BK
AF31          011010xx      3   AC_BE
AF41          100010xx      4   AC_VI
AF12          001100xx      1   AC_BK
AF22          010100xx      2   AC_BK
AF32          011100xx      3   AC_BE
AF42          100100xx      4   AC_VI
AF13          001110xx      1   AC_BK
AF23          010110xx      2   AC_BK
AF33          011110xx      3   AC_BE
AF43          100110xx      4   AC_VI
CS0           000000xx      0   AC_BE
CS1           001000xx      1   AC_BK
CS2           010000xx      2   AC_BK
CS3           011000xx      3   AC_BE
CS4           100000xx      4   AC_VI
CS5           101000xx      5   AC_VI
CS6           110000xx      6   AC_VO
CS7           111000xx      7   AC_VO
============  ============  ==  ===============

So, for example, a ToS equal to 0xc0 (binary 11000000)
will map to CS6, User Priority 6, and Access Category AC_VO.
Also, the ns3-wifi-ac-mapping test suite (defined in
src/test/ns3wifi/wifi-ac-mapping-test-suite.cc) can provide additional
useful information.

Note that :cpp:func:`ns3::WifiNetDevice::SelectQueue()` also sets the packet
priority to the user priority, thus overwriting the value determined by the
socket priority (users can read :ref:`Socket-options` for details on how to
set the packet priority). Also, given that the Traffic Control layer calls
:cpp:func:`ns3::WifiNetDevice::SelectQueue()` before enqueuing the packet
into a queue disc, it turns out that queuing disciplines (such as
PfifoFastQueueDisc) that classifies packets based on their priority will
use the user priority instead of the socket priority.

WifiHelper
==========

We're now ready to create WifiNetDevices. First, let's create
a WifiHelper with default settings:

.. sourcecode:: cpp

  WifiHelper wifiHelper;

What does this do?  It sets the default wifi standard to **802.11a** and sets the RemoteStationManager to
``ns3::ArfWifiManager``.  You can change the RemoteStationManager by calling the
``WifiHelper::SetRemoteStationManager`` method. To change the wifi standard, call the
``WifiHelper::SetStandard`` method with the desired standard.

Now, let's use the wifiPhyHelper and wifiMacHelper created above to install WifiNetDevices
on a set of nodes in a NodeContainer "c":

.. sourcecode:: cpp

  NetDeviceContainer wifiContainer = WifiHelper::Install(wifiPhyHelper, wifiMacHelper, c);

This creates the WifiNetDevice which includes also a WifiRemoteStationManager, a
WifiMac, and a WifiPhy (connected to the matching Channel).

The ``WifiHelper::SetStandard`` method sets various default timing parameters as defined in the selected standard version, overwriting values that may exist or have been previously configured.
In order to change parameters that are overwritten by ``WifiHelper::SetStandard``, this should be done post-install using ``Config::Set``:

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211n);
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("HtMcs7"), "ControlMode", StringValue("HtMcs0"));

  //Install PHY and MAC
  Ssid ssid = Ssid("ns3-wifi");

  WifiMacHelper mac;
  mac.SetType("ns3::StaWifiMac",
  "Ssid", SsidValue(ssid),
  "ActiveProbing", BooleanValue(false));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install(phy, mac, wifiStaNode);

  mac.SetType("ns3::ApWifiMac",
  "Ssid", SsidValue(ssid));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install(phy, mac, wifiApNode);

  //Once install is done, we overwrite the standard timing values
  Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/Slot", TimeValue(MicroSeconds(slot)));
  Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/Sifs", TimeValue(MicroSeconds(sifs)));
  Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/Pifs", TimeValue(MicroSeconds(pifs)));

The WifiHelper can be used to set the attributes of the default ack policy selector
(``ConstantWifiAckPolicySelector``) or to select a different (user provided) ack
policy selector, for each of the available Access Categories. As an example, the
following code can be used to set the BaThreshold attribute of the default ack
policy selector associated with BE AC to 0.5:

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetAckPolicySelectorForAc(AC_BE, "ns3::ConstantWifiAckPolicySelector",
                                 "BaThreshold", DoubleValue(0.5));

The WifiHelper is also used to configure OBSS PD spatial reuse for 802.11ax.
The following lines configure a WifiHelper to support OBSS PD spatial reuse
using the ``ConstantObssPdAlgorithm`` with a threshold set to -72 dBm:

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetObssPdAlgorithm("ns3::ConstantObssPdAlgorithm",
                          "ObssPdLevel", DoubleValue(-72.0));

There are many other |ns3| attributes that can be set on the above helpers to
deviate from the default behavior; the example scripts show how to do some of
this reconfiguration.

WifiPhyRxTraceHelper
====================
The ``WifiPhyRxTraceHelper`` can be used to collect statistics and records of Wi-Fi
reception events at the physical layer, such as whether a PPDU was received or was dropped for
some reason, and whether a PPDU overlapped in time (collided) with one or more other PPDUs on the
channel.  Two possible types of studies that may be helped by this feature are:

1) Wi-Fi PPDU and MPDU reception outcomes to assess the performance of spatial reuse mechanisms

2) Calculation of airtime fairness between contending stations on a network

The trace helper works by hooking PHY-level traces on the applicable nodes, devices, and
links and maintaining per-receiver records of all of the Wi-Fi PPDUs that are received,
from each receiver's vantage point.  These records include a listing of all other PPDUs
that may have overlapped in time and/or frequency for a given PPDU, and the received
signal power of each PPDU involved, and the outcome of the attempt at reception (whether
the PPDU was dropped entirely, or whether the PPDU was partially or fully decoded).

This trace helper collects records of all Wi-Fi PPDus received on every PHY that is enabled
for the helper.  As an example, consider the scenario in Figure :ref:`wifi-phy-rx-trace-helper`,
depicting the arrival of three PPDUs that overlap in time.  The trace helper keeps a record
of each PPDU arrival, on each enabled PHY, and also keeps track of the PPDUs that collided
with it, if any.  In the figure, the first PPDU overlaps in time with both the second and
third PPDU.   Likewise, the second PPDU overlaps in time with the first and third PPDUs, and
the third PPDU overlaps in time with the first and second.  All of these relationships (each
frame arrival, the frames that overlapped in time with it), from the perspective of each
PHY, are stored in internal data structures.  Additionally, the reception outcome of each
PPDU, and its constituent MPDUs, is tracked.  This database of reception events can be
exported by the trace helper for fine-grained tabulation of spatial reuse events of interest,
or can be summarized by some built-in statistics printing methods.

.. _wifi-phy-rx-trace-helper:

.. figure:: figures/wifi-phy-rx-trace-helper.*

   *WifiPhyRxTraceHelper scenario of interest*

The output of this trace helper is provided in three formats:

1) Built-in print methods to allow printing of key statistics with a single C++ statement,

2) Export of a trace statistics object allowing the user to post-process or format the
   data according to the user's formatting choice, and

3) Export of the complete reception records, allowing the user to perform their own
   custom processing of the data.

A sample usage of this trace helper is as follows, as demonstrated in the example program
``src/wifi/examples/wifi-phy-rx-trace-helper-example.cc``.  First, declare an instance
of it as one might do with other Wi-Fi helpers:

.. sourcecode:: cpp

  WifiPhyRxTraceHelper rxTraceHelper;

You may need to include the trace helper's header if it is not otherwise included by the
wifi module header:

.. sourcecode:: cpp

  #include "ns3/wifi-phy-rx-trace-helper.h"

Next, enable the trace helper on some subset of the Wi-Fi nodes in the simulation.  This step
will hook traces on all of the nodes enabled.  It is important to include all of the nodes
within reception range of the nodes of interest, because PPDUs need to be traced at
both transmission and reception times.  If you are only interested in statistics or records
on a given receiver, you can later (as explained below) request for only the statistics
of interest.  For example, if you are interested in a particular AP, in a particular BSS,
and there is an interfering BSS nearby, you will want to enable all of the nodes within
interference range of the AP of interest (even if not in the same BSS).  So, in general,
you will want to ``Enable()`` the trace helper on all Wi-Fi nodes in the simulation,
unless you want to explore pruning this set for runtime scaling reasons.

In this example, we will enable on all of the Wi-Fi nodes:

.. sourcecode:: cpp

    NodeContainer c;
    c.Create(2);

    ...

    NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, c.Get(1));
    NetDeviceContainer staDevice = wifi.Install(wifiPhy, wifiMac, c.Get(0));

    ...

    rxTraceHelper.Enable(c);

The important thing to note in the above is that ``Enable()`` must be called after
Wi-Fi devices are installed, because it will hook traces on what has been installed
up to that point.

The next configuration aspect is to establish a start and stop time for PPDU reception
collection.  In the example program, application data is not sent before 1 second,
and we want to capture all data PPDUs, so the ``Start()`` and ``Stop()`` times are set
as follows:

.. sourcecode:: cpp

    rxTraceHelper.Start(Seconds(1));
    // The last packet will be sent at time 1 sec. + (numPackets - 1) * interval
    // Configure the stop time to be 1 sec. later than this.
    Time stopTime = Seconds(1) + (numPackets - 1) * interval + Seconds(1);
    rxTraceHelper.Stop(stopTime);

The start and stop times can be tuned to whatever collection window is desired.  Furthermore,
there is a ``Reset()`` method that will clear all of the PPDU records and restart collection;
this can be used if one wants to periodically sample the statistics in different time
intervals.

Next, run the simulation, and either during the run or after the simulation has completed,
harvest the statistics.  As noted above, there are three main ways to output data.
Furthermore, there are C++ method overloads that allow users to narrow the scope of
output data.

The first option is to use ``PrintStatistics()``, which will dump some statistics with
built-in formatting, such as follows:

.. sourcecode:: cpp

    traceHelper.PrintStatistics();

This will result in a number of statistics being printed out.  The statistics will
be aggregated for all nodes that are enabled.  The following is some of the default
output of the sample program:

.. sourcecode:: text

   *** Print statistics for all nodes using built-in print method:
   Total PPDUs Received: 1
   Total Non-Overlapping PPDUs Received: 1
   Total Overlapping PPDUs Received: 0

   Successful PPDUs: 1
   Failed PPDUs: 0

   Total MPDUs: 1
   Total Successful MPDUs: 1
   Total Failed MPDUs: 0


In the above simple case, there is one PPDU successfully received, and within this PPDU,
there was one MPDU which was successfully decoded.  No other PPDUs overlapped in time
with this PPDU.

Although this is a PHY trace helper, there is one important aspect from the MAC layer
that is used to classify PPDUs.  When reporting statistics for a given node, a PPDU
is counted only if there was at least one MPDU intended for the node.  If the MPDU
has a MAC address that is either broadcast or belongs to the device (unicast), that
PPDU will be counted.  If, instead, a receiver locks onto and overhears a PPDU
that ultimately will be discarded because the receiver was not an intended receiver, that
PPDU reception or failure will be excluded from the statistics.

The sample program next shows how ``PrintStatistics`` can take an argument:

.. sourcecode:: cpp

    rxTraceHelper.PrintStatistics(c.Get(0)->GetId());
    rxTraceHelper.PrintStatistics(c.Get(1));

In the above, the statistics can be limited to a particular node (either passed in
by Node ID or by Node pointer).  Furthermore, although not shown in the example,
the method can be further downscoped to also take a device index argument and
(in the case of multi-link operation (MLO)) a specific link ID.  In the above example,
all devices and all MLO links will be included in the output.

Users can write their own printing methods or further process the statistics by
calling ``GetStatistics()`` as the following example demonstrates:

.. sourcecode:: cpp

    auto stats = rxTraceHelper.GetStatistics();
    std::cout << "  numOverlapppingPpdu: " << stats.m_numOverlappingPpdu << std::endl;
    std::cout << "  numNonOverlapppingPpdu: " << stats.m_numNonOverlappingPpdu << std::endl;
    std::cout << "  numPpduFailed: " << stats.m_numPpduFailed << std::endl;
    std::cout << "  numPpduSuccess: " << stats.m_numPpduSuccess << std::endl;
    std::cout << "  numMpduSuccess: " << stats.m_numMpduSuccess << std::endl;
    std::cout << "  numMpduFailed: " << stats.m_numMpduFailed << std::endl;

Similar to ``PrintStatistics()``, ``GetStatistics()`` with no arguments will collect
a statistics output structure covering all nodes, while additional arguments of
Node ID, device index, and link ID can be used to constrain the statistics report.

Finally, the output of the internal data structure that tracks PPDU receptions can
be exported, with the ``GetPpduRecords()`` method, which can be called
without any arguments, and with arguments of nodeId, deviceId, and linkId:

.. sourcecode:: cpp

    auto optionalRecords = rxTraceHelper.GetPpduRecords(1);

The above statements asks for records from receiving node 1 (the AP).  The example
iterates the returned vector, printing out the size of this structure, as well
as some data fields:

.. sourcecode:: text

  *** Records vector has size of 3
  First record:
    first PPDU's RSSI (dBm): -30.6633
    first PPDU's receiver ID: 1
    first PPDU's sender ID: 0
    first PPDU's start time: 1
    first PPDU's end time: 1.00008
    first PPDU's number of MPDUs: 1
    first PPDU's sender device ID: 0
  Second record:
    second PPDU's RSSI (dBm): -30.6633
    second PPDU's receiver ID: 1
    second PPDU's sender ID: 0
    second PPDU's start time: 1.00027
    second PPDU's end time: 1.00032
    second PPDU's number of MPDUs: 1
    second PPDU's sender device ID: 0
  Third record:
    third PPDU's RSSI (dBm): -30.6633
    third PPDU's receiver ID: 1
    third PPDU's sender ID: 0
    third PPDU's start time: 1.00039
    third PPDU's end time: 1.00072
    third PPDU's number of MPDUs: 1
    third PPDU's sender device ID: 0

That the size of the vector is 3 may be surprising, considering that we have observed
above in this example that only one PPDU is being counted in the statistics.  However,
in this example, there is only one data PPDU but also additional management and control
frames.  Specifically, the first PPDU is a Block ACK ADDBA_REQUEST frame, the second PPDU
is an ACK of the AP's ADDBA_RESPONSE frame, and the third PPDU record corresponds to the
actual data PPDU, starting at time 1.00039 and ending at 1.00072.
The management and control frames (which also include beacons) are filtered out above by
the ``GetStatistics()`` methods, but when the raw PPDU records are retrieved, all PPDUs
received are available and the user is responsible for further filtering as they see fit.

WifiTxStatsHelper
=================

The ``WifiTxStatsHelper`` is complementary to the ``WifiPhyRxTraceHelper``.  Where the latter
is used to collect statistics of Wi-Fi reception events at the physical layer, the former
is used for collecting statistics on the transmit side at the MAC layer, including the
number of transmissions and retransmissions of an MPDU, and the total number of successful,
retransmitted, and failed MPDUs.  In addition, the helper tracks
the duration of time that MPDUs are enqueued, including the durations that they reside
within the MAC queue. For failed MPDUs, their timestamps of drop and reasons for being dropped
are collected.

The trace helper works by hooking MAC- and PHY-level traces on the applicable nodes, devices,
and links (in multi-link operation), and maintaining per-MPDU records of every transmission.
We explain the operation by way of first reviewing how a simulation is configured to
use the helper, and then reviewing the public API.

A sample usage of this trace helper is as follows, as can be explored by extending an
existing Wi-fi example program.  First, declare an instance of the helper, as one might do
with other Wi-Fi helpers, prior to calling ``Simulation::Run()``:

.. sourcecode:: cpp

  WifiTxStatsHelper txStatsHelper;

You may need to include the trace helper's header if it is not otherwise included by the
wifi module header:

.. sourcecode:: cpp

  #include "ns3/wifi-tx-stats-helper.h"

Next, enable the trace helper on some subset of the Wi-Fi nodes in the simulation.  This step
will hook traces on all of the nodes or devices enabled.  Two methods are provided, depending
on whether a NodeContainer or NetDeviceContainer is enabled:

.. sourcecode:: cpp

  NodeContainer n;
  txStatsHelper.Enable(n);

or

.. sourcecode:: cpp

  NetDeviceContainer d;
  txStatsHelper.Enable(d);

By default, statistics will be collected for MPDUs enqueued between the start and stop times, which default to
collecting statistics for the whole simulation.  To limit the duration, the following methods
can be used, as an example:

.. sourcecode:: cpp

  txStatsHelper.Start(Seconds(1));
  txStatsHelper.Stop(Seconds(10));

Alternatively, the constructor is overloaded to allow these arguments to be passed there instead:

.. sourcecode:: cpp

  WifiTxStatsHelper txStatsHelper(Seconds(1), Seconds(10));

Finally, a ``Reset()`` method is provided, to clear the success and failure records.  If
this method is called, any in-progress MPDUs (i.e., those that have been queued but have
neither been positively acked nor reached the maximum number of retries) are kept, but
any completed records are deleted.

The following main methods provide the output:

.. sourcecode:: cpp

   CountPerNodeDevice_t GetSuccessesByNodeDevice() const;
   CountPerNodeDevice_t GetFailuresByNodeDevice() const;
   CountPerNodeDevice_t GetRetransmissionsByNodeDevice() const;
   uint64_t GetSuccesses() const;
   uint64_t GetFailures() const;
   uint64_t GetRetransmissions() const;
   Time GetDuration() const;
   const SuccessRecords& GetSuccessRecords(
       WifiTxStatsHelper::MultiLinkSuccessType type = FIRST_LINK_IN_SET) const;
   const FailureRecords& GetFailureRecords() const;

For Multi-Link Operation (MLO) configurations, the records of successful MPDU transmissions
are also available on a more granular, per-link basis:

.. sourcecode:: cpp

   const SuccessRecords& GetSuccessRecords(
       WifiTxStatsHelper::MultiLinkSuccessType type = FIRST_LINK_IN_SET) const;

MPDU transmission failures can also be filtered on the drop reason type with the following methods:

.. sourcecode:: cpp

   CountPerNodeDevice_t GetFailuresByNodeDevice(WifiMacDropReason reason) const;
   uint64_t GetFailures(WifiMacDropReason reason) const;

The first three methods return unordered maps with counts of successful MPDU transmissions,
failed MPDU transmissions, and retransmitted MPDUs (for eventually successful MPDUs).

1. ``GetSuccessesByNodeDevice()``: The returned data structure counts the number of
   successful MPDU transmissions that were observed, indexed by a {node ID, device ID}
   tuple.  If Multi-Link Operation is in use on the device, an acknowledged MPDU will be
   counted as one success regardless of how many links it was transmitted on.

   a. ``GetSuccessesByNodeDeviceLink()``: The returned data structure counts the number of
      successful MPDU transmissions that were observed, indexed by {node ID, device ID, link ID}
      tuple.  The link ID corresponds to the link for which the MPDU was successfully
      acknowledged. If `type` is set to `FIRST_LINK_IN_SET` (default), then the success
      is counted on the first link in the MPDU's in-flight link ID set. Otherwise, the success
      is counted for all links in the MPDU's in-flight link ID set.

2. ``GetFailuresByNodeDevice()``:  This returned data structure counts the number of failures
   (maximum retries reached) that were observed, indexed by {node ID, device ID} tuple.
   Per-link statistics are not available for failures (which may have experienced transmissions
   on different links).

   a. ``GetFailuresByNodeDevice(WifiMacDropReason reason)``: This overloaded method filters for
      failure counts that match the provided drop reason.

3. ``GetRetransmissionsByNodeDevice()``:  This returned data structure counts the number of
   retransmissions that were observed, indexed by {node ID, device ID} tuple.  Per-link
   statistics are not available for retransmissions.  Additionally, retransmissions of MPDUs
   that were not ultimately successful are not counted.

The second three methods provide aggregated counts across all nodes and devices enabled for
the helper.  These counts should be the same as if the hashed maps of the first three methods
were iterated, and the counts for each entry summed. The `GetFailures()` method has an
overloaded version (`uint64_t GetFailures(WifiMacDropReason reason) const;`) to get the
count of failed MPDU transmissions due to a given reason. If `type` is set to
`FIRST_LINK_IN_SET` (default) for the `GetSuccessRecords` method, then the successful
MPDU's record is saved in the `std::list<MpduRecord>` of the first link in the
MPDU's in-flight link ID set. Otherwise, the record is saved in the lists of all links
in the MPDU's in-flight link ID set.

The ``GetDuration()`` method will return the duration since the helper was started or last reset.

More detailed statistics can be obtained by the methods ``GetSuccessRecords()`` and
``GetFailureRecords()``.  These methods export unordered maps of success and failure records,
in the form of ``struct MpduRecord``, and organized by node, and device, and also by
link-id in the case of success records (link-id is not available for failure records).
Each ``MpduRecord`` tracks the time of enqueue, the time of first transmission, the ack time,
and the dequeue time (a dequeue occurs when the MPDU is either positively acked or is discarded
due to too many retransmissions).  The node ID, device ID, MPDU sequence number, and TID are
recorded, and the number of retransmissions (if any) are counted.  Finally, for successful
MPDUs, the ``m_successLinkId`` field tracks the link ID on which the MPDU was ultimately
acknowledged.  To obtain a complete accounting of all MPDUs tracked by the helper, both
``GetSuccessRecords()`` and ``GetFailureRecords()`` must be queried; all MPDU results
will end up in one of the two data structures.

The example program ``src/wifi/examples/wifi-bianchi.cc`` provides an example use of this
helper, by setting the program option ``--useTxHelper`` to true.

WifiCoTraceHelper
=================

The ``WifiCoTraceHelper`` (channel occupancy trace helper) can be used to collect statistics of Wi-Fi channel occupancy, as observed from the perspective of the WifiPhy state of devices or links (in the case of multi-link devices).  The ``WifiPhyStateHelper`` tracks the state of the WifiPhy corresponding to whether it is in idle, transmitting, receiving, or some other state.  This helper object can be added to ns-3 Wi-Fi simulations to track the duration and percentage of time spent in each state.

Wi-Fi channel access relies also on additional busy states that conceptually reside in the MAC layer, corresponding to the Network Allocation Vector (NAV).  The idle states reported herein correspond to the PHY (physical) carrier sense state and not the MAC (virtual) carrier sense state.

In case of an EMLSR device, PHY objects keep switching among links. This helper aggregates a PHY duration to the link it is operating on at that instant. Be aware that switching PHY objects for an EMLSR device might result in unexpected statistics. For example, if auxiliary PHY is configured to not switch in EMLSR configurations then a link could have no PHY object operating on it intermittently. As a result, the total recorded duration on all links would not be same which does not occur for non-EMLSR devices.

This helper can be added to a simulation program with statements such as the following:

.. sourcecode:: cpp

    WifiCoTraceHelper coHelper{Seconds(1.0), Seconds(11.0)};

The above statement, if placed just prior to the call to ``Simulator::Run(),``
will declare a helper object and configure it to collect statistics between
one and eleven seconds.  However, WifiNetDevices must still be added, such
as the following sample code:

.. sourcecode:: cpp

    coHelper.Enable(nodeOrDeviceContainer);

Then finally, print the results after ``Simulator::Run()`` returns:

.. sourcecode:: cpp

    coHelper.PrintStatistics(std::cout);

This will print output such as:

.. sourcecode:: text

    ---- COT for AP:0 ----
    Showing duration by states:
    IDLE:       +5.69s  (56.93%)
    CCA_BUSY:   +1.18s  (11.80%)
    TX:       +301.90ms  (3.02%)
    RX:         +2.83s  (28.25%)

    ---- COT for STA0:0 ----
    Showing duration by states:
    IDLE:       +5.71s  (57.06%)
    CCA_BUSY: +377.52ms  (3.78%)
    TX:         +1.63s  (16.31%)
    RX:         +2.29s  (22.85%)

You can export statistics from this helper for use cases such as printing in your own formatted statements:

.. sourcecode:: cpp

    auto records = coHelper.GetDeviceRecords();

You can refer an example program in ``src/wifi/examples/wifi-co-trace-example.cc`` on how to use these APIs.

HT configuration
================

HT is an acronym for High Throughput, a term synonymous with the IEEE 802.11n
standard.  Once the ``ns3::WifiHelper::Install`` has been called and the
user sets the standard to a variant that supports HT capabilities (802.11n,
802.11ac, or 802.11ax), an HT configuration object will automatically be
created for the device.  The configuration object is used to store and
manage HT-specific attributes.

802.11n/ac PHY layer can use either long (800 ns) or short (400 ns) OFDM guard intervals. To configure this parameter for a given device, the following lines of code could be used (in this example, it enables the support of a short guard interval for the first station):

.. sourcecode:: cpp

 Ptr<NetDevice> nd = wifiStaDevices.Get(0);
 Ptr<WifiNetDevice> wnd = nd->GetObject<WifiNetDevice>();
 Ptr<HtConfiguration> htConfiguration = wnd->GetHtConfiguration();
 htConfiguration->SetShortGuardIntervalSupported(true);

It is also possible to configure HT-specific attributes using ``Config::Set``.
The following line of code enables the support of a short guard interval for all stations:

.. sourcecode:: cpp

 Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue(true));

VHT configuration
=================

IEEE 802.11ac devices are also known as supporting Very High Throughput (VHT).  Once the ``ns3::WifiHelper::Install`` has been called and either the 802.11ac
or 802.11ax 5 GHz standards are configured, a VHT configuration object will be
automatically created to manage VHT-specific attributes.

As of ns-3.29, however, there are no VHT-specific configuration items to
manage; therefore, this object is a placeholder for future growth.

HE configuration
================

IEEE 802.11ax is also known as High Efficiency (HE).  Once the ``ns3::WifiHelper::Install`` has been called and IEEE 802.11ax configured as the standard, an
HE configuration object will automatically be created to manage HE-specific
attributes for 802.11ax devices.

802.11ax PHY layer can use either 3200 ns, 1600 ns or 800 ns OFDM guard intervals. To configure this parameter, the following lines of code could be used (in this example, it enables the support of 1600 ns guard interval), such as in this example code snippet:

.. sourcecode:: cpp

 Ptr<NetDevice> nd = wifiStaDevices.Get(0);
 Ptr<WifiNetDevice> wnd = nd->GetObject<WifiNetDevice>();
 Ptr<HeConfiguration> heConfiguration = wnd->GetHeConfiguration();
 heConfiguration->SetGuardInterval(NanoSeconds(1600));

802.11ax allows compressed Block ACKs containing a 256-bits bitmap, making
possible transmissions of A-MPDUs containing up to 256 MPDUs, depending on the
negotiated buffer size. In order to configure the buffer size of an 802.11ax device,
the following line of code could be used:

.. sourcecode:: cpp

 WifiMacHelper mac;
 mac.SetType("ns3::StaWifiMac", "MpduBufferSize", UintegerValue(256));

For transmitting large MPDUs, it might also be needed to increase the maximum
aggregation size (see above).

When using UL MU transmissions, solicited TB PPDUs can arrive at the AP with a
different delay, due to the different propagation delay from the various stations.
In real systems, late TB PPDUs cause a variable amount of interference depending on
the receiver's sensitivity. This phenomenon can be modeled through the
``ns3::HeConfiguration::MaxTbPpduDelay`` attribute, which defines the maximum delay
with which a TB PPDU can arrive with respect to the first TB PPDU in order to be
decoded properly. TB PPDUs arriving after more than ``MaxTbPpduDelay`` since the
first TB PPDU are discarded and considered as interference.

Mobility configuration
======================

Finally, a mobility model must be configured on each node with Wi-Fi device.
Mobility model is used for calculating propagation loss and propagation delay.
Two examples are provided in the next section.
Users are referred to the chapter on :ref:`Mobility` module for detailed information.

Example configuration
=====================

We provide two typical examples of how a user might configure a Wi-Fi network --
one example with an ad-hoc network and one example with an infrastructure network.
The two examples were modified from the two examples in the ``examples/wireless`` folder
(``wifi-simple-adhoc.cc`` and ``wifi-simple-infra.cc``).
Users are encouraged to see examples in the ``examples/wireless`` folder.

AdHoc WifiNetDevice configuration
+++++++++++++++++++++++++++++++++

In this example, we create two ad-hoc nodes equipped with 802.11a Wi-Fi devices.
We use the ``ns3::ConstantSpeedPropagationDelayModel`` as the propagation delay model and
``ns3::LogDistancePropagationLossModel`` with the exponent of 3.0 as the propagation loss model.
Both devices are configured with ``ConstantRateWifiManager`` at the fixed rate of 12Mbps.
Finally, we manually place them by using the ``ns3::ListPositionAllocator``:

.. sourcecode:: cpp

  std::string phyMode("OfdmRate12Mbps");

  NodeContainer c;
  c.Create(2);

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211a);

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11
  wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                 "Exponent", DoubleValue(3.0));
  wifiPhy.SetChannel(wifiChannel.Create());

  // Add a non-QoS upper mac, and disable rate control (i.e. ConstantRateWifiManager)
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode",StringValue(phyMode),
                               "ControlMode",StringValue(phyMode));
  // Set it to adhoc mode
  wifiMac.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);

  // Configure mobility
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  positionAlloc->Add(Vector(5.0, 0.0, 0.0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);

  // other set up (e.g. InternetStack, Application)

Infrastructure (access point and clients) WifiNetDevice configuration
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This is a typical example of how a user might configure an access point and a set of clients.
In this example, we create one access point and two clients.
Each node is equipped with 802.11b Wi-Fi device:

.. sourcecode:: cpp

  std::string phyMode("DsssRate1Mbps");

  NodeContainer ap;
  ap.Create(1);
  NodeContainer stas;
  stas.Create(2);

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default();
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11
  wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  // reference loss must be changed since 802.11b is operating at 2.4GHz
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                 "Exponent", DoubleValue(3.0),
                                 "ReferenceLoss", DoubleValue(40.0459));
  wifiPhy.SetChannel(wifiChannel.Create());

  // Add a non-QoS upper mac, and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode",StringValue(phyMode),
                               "ControlMode",StringValue(phyMode));

  // Setup the rest of the upper mac
  Ssid ssid = Ssid("wifi-default");

  // setup AP.
  wifiMac.SetType("ns3::ApWifiMac",
                  "Ssid", SsidValue(ssid));
  NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, ap);
  NetDeviceContainer devices = apDevice;

  // setup STAs.
  wifiMac.SetType("ns3::StaWifiMac",
                  "Ssid", SsidValue(ssid),
                  "ActiveProbing", BooleanValue(false));
  NetDeviceContainer staDevices = wifi.Install(wifiPhy, wifiMac, stas);
  devices.Add(staDevices);

  // Configure mobility
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  positionAlloc->Add(Vector(5.0, 0.0, 0.0));
  positionAlloc->Add(Vector(0.0, 5.0, 0.0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(ap);
  mobility.Install(sta);

  // other set up (e.g. InternetStack, Application)

Multiple RF interfaces configuration
++++++++++++++++++++++++++++++++++++

.. sourcecode:: cpp

  NodeContainer ap;
  ap.Create(1);
  NodeContainer sta;
  sta.Create(1);

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211be);

  // Create multiple spectrum channels
  Ptr<MultiModelSpectrumChannel> spectrumChannel2_4Ghz =
      CreateObject<MultiModelSpectrumChannel>();
  Ptr<MultiModelSpectrumChannel> spectrumChannel5Ghz =
      CreateObject<MultiModelSpectrumChannel>();
  Ptr<MultiModelSpectrumChannel> spectrumChannel6Ghz =
      CreateObject<MultiModelSpectrumChannel>();

  // optional: set up propagation loss model separately for each spectrum channel

  // SpectrumWifiPhyHelper (3 links)
  SpectrumWifiPhyHelper phy(3);
  phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.SetPcapCaptureType(WifiPhyHelper::PcapCaptureType::PCAP_PER_LINK);
  phy.AddChannel(spectrumChannel2_4Ghz, WIFI_SPECTRUM_2_4_GHZ);
  phy.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);
  phy.AddChannel(spectrumChannel6Ghz, WIFI_SPECTRUM_6_GHZ);

  // configure operating channel for each link
  phy.Set(0, "ChannelSettings", StringValue("{42, 0, BAND_2_4GHZ, 0}"));
  phy.Set(1, "ChannelSettings", StringValue("{42, 0, BAND_5GHZ, 0}"));
  phy.Set(2, "ChannelSettings", StringValue("{215, 0, BAND_6GHZ, 0}"));

  // configure rate manager for each link
  wifi.SetRemoteStationManager(0,
                               "ns3::ConstantRateWifiManager",
                               "DataMode", StringValue("EhtMcs11"),
                               "ControlMode", StringValue("ErpOfdmRate24Mbps"));
  wifi.SetRemoteStationManager(1,
                               "ns3::ConstantRateWifiManager",
                               "DataMode", StringValue("EhtMcs9"),
                               "ControlMode", StringValue("OfdmRate24Mbps"));
  wifi.SetRemoteStationManager(2,
                               "ns3::ConstantRateWifiManager",
                               "DataMode", StringValue("EhtMcs7"),
                               "ControlMode", StringValue("HeMcs4"));

  // setup AP.
  wifiMac.SetType("ns3::ApWifiMac",
                  "Ssid", SsidValue(ssid));
  NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, ap);
  NetDeviceContainer devices = apDevice;

  // setup STA.
  wifiMac.SetType("ns3::StaWifiMac",
                  "Ssid", SsidValue(ssid),
                  "ActiveProbing", BooleanValue(false));
  NetDeviceContainer staDevice = wifi.Install(wifiPhy, wifiMac, sta);
  devices.Add(staDevice);

  // Configure mobility
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  positionAlloc->Add(Vector(5.0, 0.0, 0.0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);

  // other set up (e.g. InternetStack, Application)

EMLSR configuration
+++++++++++++++++++

**DISCLAIMER** EMLSR support is still experimental. Simulations with EMLSR activated may crash unexpectedly.

Proper EMLSR operations require the configuration of multiple RF interfaces, as illustrated above.
Therefore, in order to configure EMLSR operations, we can start from the configuration shown above
and apply the changes described in the following. Firstly, we need to activate EMLSR on both the
AP MLD and the EMLSR client. This can be done by configuring the appropriate ``EhtConfiguration``
attribute through the wifi helper:

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211be);
  wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(true));

The ``EhtConfiguration`` class also provides attributes for the various EMLSR parameters that an
AP MLD that supports EMLSR has to advertise. Given that these attributes are simply ignored by
non-AP MLDs, we can use the same wifi helper to configure such parameters:

.. sourcecode:: cpp

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211be);
  wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(true));
  wifi.ConfigEhtOptions("TransitionTimeout", TimeValue(MicroSeconds(1024)));
  wifi.ConfigEhtOptions("MediumSyncDuration", TimeValue(MicroSeconds(3200)));
  wifi.ConfigEhtOptions("MsdOfdmEdThreshold", IntegerValue(-72);
  wifi.ConfigEhtOptions("MsdMaxNTxops", UintegerValue(0)); // unlimited attempts

EMLSR parameters that are client specific can be configured through the attributes of the EMLSR
Manager class and subclass. The ``WifiMacHelper`` class provides the ``SetEmlsrManager``
method to conveniently set the type of EMLSR Manager and its attributes:

.. sourcecode:: cpp

  // setup STA.
  wifiMac.SetType("ns3::StaWifiMac",
                  "Ssid", SsidValue(ssid),
                  "ActiveProbing", BooleanValue(false));
  wifiMac.SetEmlsrManager("ns3::DefaultEmlsrManager",
                          "EmlsrLinkSet",
                          StringValue("0,1,2"),  // enable EMLSR on all EMLSR links
                          "MainPhyId",
                          UintegerValue(1),
                          "EmlsrPaddingDelay",
                          TimeValue(MicroSeconds(32)),
                          "EmlsrTransitionDelay",
                          TimeValue(MicroSeconds(128)),
                          "SwitchAuxPhy",
                          BooleanValue(false),
                          "AuxPhyChannelWidth",
                          UintegerValue(40));
  NetDeviceContainer staDevice = wifi.Install(wifiPhy, wifiMac, sta);
  devices.Add(staDevice);

Note that the channel switch delay should be less than the transition delay:

.. sourcecode:: cpp

  // SpectrumWifiPhyHelper (3 links)
  SpectrumWifiPhyHelper phy(3);
  phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.SetPcapCaptureType(WifiPhyHelper::PcapCaptureType::PCAP_PER_LINK);
  phy.AddChannel(spectrumChannel2_4Ghz, WIFI_SPECTRUM_2_4_GHZ);
  phy.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);
  phy.AddChannel(spectrumChannel6Ghz, WIFI_SPECTRUM_6_GHZ);
  phy.Set("ChannelSwitchDelay", TimeValue(MicroSeconds(100)));

