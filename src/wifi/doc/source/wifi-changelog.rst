.. include:: replace.txt
.. highlight:: cpp

.. _sec-wifi-changelog-doc:

++++++++++++++++++++++++++++++++++++++++
API and behavior changes across releases
++++++++++++++++++++++++++++++++++++++++

This page tracks the most relevant changes to the API and behavior of the wifi
module occurred across the various releases of |ns3|.

Wifi MAC queue scheduler refactor
+++++++++++++++++++++++++++++++++

As of ns-3.48, the ``WifiMacQueueSchedulerImpl`` template base class has
absorbed the ``DropPolicy`` attribute and the ``HasToDropBeforeEnqueue``
implementation that previously lived in ``FcfsWifiQueueScheduler``, so that
every subclass (including the new ``RrWifiQueueScheduler``) inherits the same
drop behavior. As part of this change:

* The nested enum ``ns3::FcfsWifiQueueScheduler::DropPolicy`` has been
  removed; use ``ns3::WifiMacQueueSchedulerImpl::DropPolicy`` instead. The
  attribute path (``.../FcfsWifiQueueScheduler/DropPolicy``) still resolves
  via inheritance and is unchanged for users configuring via ``Config::Set``
  or the attribute string.
* The free struct ``ns3::FcfsPrio`` (and its comparison operators) has been
  removed. ``FcfsWifiQueueScheduler`` now derives from
  ``WifiMacQueueSchedulerImpl<WifiSchedPrecedence<Time>>``; new subclasses
  should similarly parameterize the base with ``WifiSchedPrecedence<...>``.

ChannelSettings attribute
+++++++++++++++++++++++++

Prior to ns-3.36, channels, channel widths, and operating bands were set
separately.  As of ns-3.36, a new tuple object that we call ChannelSettings has
consolidated all of these settings.  Users should specify the channel number,
channel width, frequency band, and primary channel index as a tuple (and
continue to set the Wi-Fi standard separately). Since ns-3.43, this attribute
can contain multiple tuples to support 80+80MHz operating channels.

For instance, where pre-ns-3.36 code may have said::

    WifiPhyHelper phy;
    phy.Set("ChannelNumber", UintegerValue(36));

the equivalent new code is::

    WifiPhyHelper phy;
    phy.Set("ChannelSettings", StringValue("{36, 20, BAND_5GHZ, 0}"));

which denotes that channel 36 is used on a 20 MHz channel in the 5GHz band,
and because a larger channel width greater than 20 MHz is not being used, there
is no need to indicate the primary 20 MHz channel so it is set to zero in the
last argument. Users can read :ref:`Channel-settings` for more details.
