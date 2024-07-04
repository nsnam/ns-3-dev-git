.. include:: replace.txt
.. highlight:: bash

+++++++++++++++++++++
Testing Documentation
+++++++++++++++++++++

At present, most of the available documentation about testing and validation
exists in publications, some of which are referenced below.

Error model
***********

Validation results for the 802.11b error model are available in this
`technical report <http://www.nsnam.org/~pei/80211b.pdf>`__

Two clarifications on the results should be noted.  First, Figure 1-4
of the above reference
corresponds to the |ns3| NIST BER model.   In the program in the
Appendix of the paper (80211b.c), there are two constants used to generate
the data.  The first, packet size, is set to 1024 bytes.  The second,
"noise", is set to a value of 7 dB; this was empirically picked to align
the curves the best with the reported data from the CMU testbed.  Although
a value of 1.55 dB would correspond to the reported -99 dBm noise floor
from the CMU paper, a noise figure of 7 dB results in the best fit with the
CMU experimental data.  This default of 7 dB is the RxNoiseFigure in the
``ns3::YansWifiPhy`` model.  Other values for noise figure will shift the
curves leftward or rightward but not change the slope.

The curves can be reproduced by running the ``wifi-clear-channel-cmu.cc``
example program in the ``examples/wireless`` directory, and the figure produced
(when GNU Scientific Library (GSL) is enabled) is reproduced below in
Figure :ref:`fig-clear-channel-80211b`.

.. _fig-clear-channel-80211b:

.. figure:: figures/clear-channel.*
   :align: center

   Clear channel (AWGN) error model for 802.11b

Validation results for the 802.11a/g OFDM error model are available in this
`technical report <https://www.nsnam.org/~pei/80211ofdm.pdf>`__.  The curves
can be reproduced by running the ``wifi-ofdm-validation.cc`` example program
in the ``examples/wireless`` directory, and the figure is reproduced below
in Figure :ref:`fig-nist-frame-success-rate`.

.. _fig-nist-frame-success-rate:

.. figure:: figures/nist-frame-success-rate.*
   :align: center

   Frame error rate (NIST model) for 802.11a/g (OFDM) Wi-Fi

Similar curves for 802.11n/ac/ax can be obtained by running the ``wifi-ofdm-ht-validation.cc``,
``wifi-ofdm-vht-validation.cc`` and ``wifi-ofdm-he-validation.cc`` example programs
in the ``examples/wireless`` directory, and the figures are reproduced below
in Figure :ref:`fig-nist-frame-success-rate-n`, Figure :ref:`fig-nist-frame-success-rate-ac`
and Figure :ref:`fig-nist-frame-success-rate-ax`, respectively.
There is no validation for those curves yet.

.. _fig-nist-frame-success-rate-n:

.. figure:: figures/nist-frame-success-rate-n.*
   :align: center

   Frame error rate (NIST model) for 802.11n (HT OFDM) Wi-Fi

.. _fig-nist-frame-success-rate-ac:

.. figure:: figures/nist-frame-success-rate-ac.*
   :align: center

   Frame error rate (NIST model) for 802.11ac (VHT OFDM) Wi-Fi

.. _fig-nist-frame-success-rate-ax:

.. figure:: figures/nist-frame-success-rate-ax.*
   :align: center

   Frame error rate (NIST model) for 802.11ax (HE OFDM) Wi-Fi

MAC validation
**************
Validation of the 802.11 DCF MAC layer has been performed in [baldo2010]_.

802.11 PCF operation has been verified by running 'wifi-pcf' example with PCAP files generation enabled, and observing the frame exchange using Wireshark.

SpectrumWiFiPhy
***************

The SpectrumWifiPhy implementation has been verified to produce equivalent
results to the legacy YansWifiPhy by using the saturation and packet
error rate programs (described below) and toggling the implementation
between the two physical layers.

A basic unit test is provided using injection of hand-crafted packets to
a receiving Phy object, controlling the timing and receive power of
each packet arrival and checking the reception results.  However, most of
the testing of this Phy implementation has been performed using example
programs described below, and during the course of a (separate) LTE/Wi-Fi
coexistence study not documented herein.

Saturation performance
======================

The program ``examples/wireless/wifi-spectrum-saturation-example.cc``
allows user to select either the `SpectrumWifiPhy` or `YansWifiPhy` for
saturation tests.  The wifiType can be toggled by the argument
``'--wifiType=ns3::YansWifiPhy'`` or ``--wifiType=ns3::SpectrumWifiPhy'``

There isn't any difference in the output, which is to be expected because
this test is more of a test of the DCF than the physical layer.

By default, the program will use the `SpectrumWifiPhy` and will run
for 10 seconds of saturating UDP data, with 802.11n features enabled.
It produces this output for the main 802.11n rates (with short and long guard
intervals):

::

  wifiType: ns3::SpectrumWifiPhy distance: 1m
  index   MCS   width Rate (Mb/s) Tput (Mb/s) Received
      0     0      20       6.5     5.81381    4937
      1     1      20        13     11.8266   10043
      2     2      20      19.5     17.7935   15110
      3     3      20        26     23.7958   20207
      4     4      20        39     35.7331   30344
      5     5      20        52     47.6174   40436
      6     6      20      58.5     53.6102   45525
      7     7      20        65     59.5501   50569
    ...
     63    15      40       300     254.902  216459


The above output shows the first 8 (of 32) modes, and last mode, that will be
output from the program.  The first 8 modes correspond
to short guard interval disabled and channel bonding disabled.  The
subsequent 24 modes run by this program are variations with short guard
interval enabled (cases 9-16), and then with channel bonding enabled and
short guard first disabled then enabled (cases 17-32).  Cases 33-64 repeat
the same configurations but for two spatial streams (MIMO abstraction).

When run with the legacy YansWifiPhy, as in ``./ns3 run "wifi-spectrum-saturation-example --wifiType=ns3::YansWifiPhy"``, the same output is observed:

::

  wifiType: ns3::YansWifiPhy distance: 1m
  index   MCS   width Rate (Mb/s) Tput (Mb/s) Received
      0     0      20       6.5     5.81381    4937
      1     1      20        13     11.8266   10043
      2     2      20      19.5     17.7935   15110
      3     3      20        26     23.7958   20207
    ...

This is to be expected since YansWifiPhy and SpectrumWifiPhy use the
same error rate model in this case.

Packet error rate performance
=============================

The program ``examples/wireless/wifi-spectrum-per-example.cc`` allows users
to select either `SpectrumWifiPhy` or `YansWifiPhy`, as above, and select
the distance between the nodes, and to log the reception statistics and
received SNR (as observed by the WifiPhy::MonitorSnifferRx trace source), using a
Friis propagation loss model.  The transmit power is lowered from the default
of 40 mW (16 dBm) to 1 dBm to lower the baseline SNR; the distance between
the nodes can be changed to further change the SNR.  By default, it steps
through the same index values as in the saturation example (0 through 31)
for a 50m distance, for 10 seconds of simulation time, producing output such as:

::

  wifiType: ns3::SpectrumWifiPhy distance: 50m; time: 10; TxPower: 1 dBm (1.3 mW)
  index   MCS  Rate (Mb/s) Tput (Mb/s) Received Signal (dBm) Noise (dBm) SNR (dB)
      0     0      6.50        5.77    7414      -79.71      -93.97       14.25
      1     1     13.00       11.58   14892      -79.71      -93.97       14.25
      2     2     19.50       17.39   22358      -79.71      -93.97       14.25
      3     3     26.00       22.96   29521      -79.71      -93.97       14.25
      4     4     39.00        0.00       0         N/A         N/A         N/A
      5     5     52.00        0.00       0         N/A         N/A         N/A
      6     6     58.50        0.00       0         N/A         N/A         N/A
      7     7     65.00        0.00       0         N/A         N/A         N/A

As in the above saturation example, running this program with YansWifiPhy
will yield identical output.

Interference performance
========================

The program ``examples/wireless/wifi-spectrum-per-interference.cc`` is based
on the previous packet error rate example, but copies over the
WaveformGenerator from the unlicensed LTE interferer test, to allow
users to inject a non-Wi-Fi signal (using the ``--waveformPower`` argument)
from the command line.  Another difference with respect to the packet
error rate example program is that the transmit power is set back to the
default of 40 mW (16 dBm).  By default, the interference generator is off,
and the program should behave similarly to the other packet error rate example,
but by adding small
amounts of power (e.g. ``--waveformPower=0.001``), one will start to observe
SNR degradation and frame loss.

Some sample output with default arguments (no interference) is:

::

  ./ns3 run "wifi-spectrum-per-interference"

  wifiType: ns3::SpectrumWifiPhy distance: 50m; time: 10; TxPower: 16 dBm (40 mW)
  index   MCS  Rate (Mb/s) Tput (Mb/s) Received Signal (dBm)Noi+Inf(dBm) SNR (dB)
      0     0      6.50        5.77    7414      -64.69      -93.97       29.27
      1     1     13.00       11.58   14892      -64.69      -93.97       29.27
      2     2     19.50       17.39   22358      -64.69      -93.97       29.27
      3     3     26.00       23.23   29875      -64.69      -93.97       29.27
      4     4     39.00       34.90   44877      -64.69      -93.97       29.27
      5     5     52.00       46.51   59813      -64.69      -93.97       29.27
      6     6     58.50       52.39   67374      -64.69      -93.97       29.27
      7     7     65.00       58.18   74819      -64.69      -93.97       29.27
    ...

while a small amount of waveform power will cause frame losses to occur at
higher order modulations, due to lower SNR:

::

  ./ns3 run "wifi-spectrum-per-interference --waveformPower=0.001"

  wifiType: ns3::SpectrumWifiPhy distance: 50m; sent: 1000 TxPower: 16 dBm (40 mW)
  index   MCS Rate (Mb/s) Tput (Mb/s) Received Signal (dBm)Noi+Inf(dBm)  SNR (dB)
      0     0      6.50        5.77    7414      -64.69      -80.08       15.38
      1     1     13.00       11.58   14892      -64.69      -80.08       15.38
      2     2     19.50       17.39   22358      -64.69      -80.08       15.38
      3     3     26.00       23.23   29873      -64.69      -80.08       15.38
      4     4     39.00        0.41     531      -64.69      -80.08       15.38
      5     5     52.00        0.00       0         N/A         N/A         N/A
      6     6     58.50        0.00       0         N/A         N/A         N/A
      7     7     65.00        0.00       0         N/A         N/A         N/A
    ...

If ns3::YansWifiPhy is selected as the wifiType, the waveform generator will
not be enabled because only transmitters of type YansWifiPhy may be connected
to a YansWifiChannel.

The interference signal as received by the sending node is typically below
the default -62 dBm CCA Mode 1 threshold in this example.  If it raises
above, the sending node will suppress all transmissions.

Bianchi validation
******************

The program ``src/wifi/examples/wifi-bianchi.cc`` allows user to
compare ns-3 simulation results against the Bianchi model
presented in [bianchi2000]_ and [bianchi2005]_.

The MATLAB code used to generate the Bianchi model,
as well as the generated outputs, are provided in
the folder ``src/wifi/examples/reference``.
User can regenerate Bianchi results by running
``generate_bianchi.m`` in MATLAB.

By default, the program ``src/wifi/examples/wifi-bianchi.cc``
simulates an 802.11a adhoc ring scenario, with a PHY rate set to
54 Mbit/s, and loop from 5 stations to 50 stations, by a step of
5 stations. It generates a plt file, which allows user to quickly
generate an eps file using gnuplot and visualize the graph.

::

  ./ns3 run "wifi-bianchi"

.. _fig-wifi-bianchi-11a-54-adhoc:

.. figure:: figures/wifi-11a-p-1500-adhoc-r-54-min-5-max-50-step-5-throughput.*
   :align: center

   Bianchi throughput validation results for 802.11a 54 Mbps in adhoc configuration

The user has the possibility to select the standard (only
11a, 11b or 11g currently supported), to select the PHY rate (in Mbit/s),
as well as to choose between an adhoc or an infrastructure configuration.

When run for 802.11g 6 Mbit/s in infrastructure mode, the output is:

::

  ./ns3 run "wifi-bianchi --standard=11g --phyRate=6 --duration=500 --infra"

.. _fig-wifi-bianchi-11g-6-infra:

.. figure:: figures/wifi-11g-p-1500-infrastructure-r-6-min-5-max-50-step-5-throughput.*
   :align: center

   Bianchi throughput validation results for 802.11g 6 Mbps in infrastructure configuration

Multi-user transmissions validation
***********************************

The implementation of the OFDMA support has been validated against a theoretical model [magrin2021mu]_ .

A preliminary evaluation of the usage of OFDMA in 802.11ax, in terms of latency in non-saturated
conditions, throughput in saturated conditions and transmission range with UL OFDMA, is provided
in [avallone2021wcm]_ .

Channel Occupancy Helper Testing
********************************

The Channel Occupancy helper (WifiCoTraceHelper class) has been tested by comparing the occupancy results for single packet transmissions of various sizes (one, two, or three symbols) with the results predicted by offline calculation of the expected values.  Additionally, the helper has been validated in saturated traffic conditions as described in the research publication "Use of Channel Occupancy for Multi Link WiFi 7 Scheduler Design in ns-3" to be presented at COMSNETS 25 [kumar2025comsnets]_ .

Peer-to-Peer (P2P) performance
******************************

The program ``examples/wireless/wifi-p2p.cc`` allows user to compare performance with and without
the use of P2P links between non-AP STAs.

The scenario is made of a single AP, two non-AP STAs and four traffic flows: bidirectional flows between
the AP and the first non-AP STA together with bidirectional flows between the two non-AP STAs.

When the first STA is operating in P2P mode, the other STA is an ADHOC STA and is not associated to any AP.
Otherwise, both non-AP STAs are regular infrastructure STAs and are associated to the same AP.

Besides the P2P mode, the example also allows to configure various parameters:
- Type for AP/STAs (HT, VHT, HE, EHT)
- MAC settings: RTS threshold, A-MPDU size
- Traffic type (CBR, Video, RT gaming) and corresponding parameter per flow
- L4 protocol and ACI/TOS

The collected statistics are the throughput, the end-to-end latency and the packet loss rate per flow.
It also provides aggregated and average statistics for the two modes of operation.

By default, it runs with CBR traffic with all flows active and P2P enabled:
- AP to first STA is 100 Mbit/s CBR UDP traffic, same for the opposite direction
- each STA also sends 50 Mbit/s CBR UDP traffic to each others

::

  ./ns3 run "wifi-p2p --simulationTime=60s"

  Direction       Throughput [Mbit/s]     E2E latency [ms]        TX [packets]    RX [packets]    Packet loss rate [%]
  AP -> STA       98.3315                 5.73014                 749999          737486          1.6684
  STA -> AP       99.9765                 9.96459                 749999          749824          0.0233334
  STA -> ADHOC    49.9999                 7.84486                 749999          749999          0
  ADHOC -> STA    49.9904                 4.18082                 749999          749856          0.0190667
  STA <-> AP      198.308                 7.86493                 1499998         1487310         0.845868
  STA <-> STA     99.9903                 6.01302                 1499998         1499855         0.00953335
  Total           298.298                 6.93508                 2999996         2987165         0.427701
  Average backoff: 9.59999

When P2P is disabled, saturation point is already reached because traffic has to be forwarded first to the AP and hence
it experiences much more collisions which results in a larger amount of lost packets and hence a lower throughput:

::

  ./ns3 run "wifi-p2p --simulationTime=60s --p2p=0"

  Direction       Throughput [Mbit/s]     E2E latency [ms]        TX [packets]    RX [packets]    Packet loss rate [%]
  AP -> STA1      99.9907                 23.2452                 749999          749930          0.00920001
  STA1 -> AP      99.5927                 71.8015                 749999          746945          0.407201
  STA1 -> STA2    21.1353                 39.7197                 749999          317030          57.7293
  STA2 -> STA1    22.3199                 25.7392                 749999          334799          55.3601
  STA <-> AP      199.583                 47.4749                 1499998         1496875         0.2082
  STA <-> STA     43.4553                 32.5389                 1499998         651829          56.5447
  Total           243.039                 42.9439                 2999996         2148704         28.3764
  Average backoff: 9.66778

We can also verify P2P is improving the end-to-end latency, which is critical for real-time and video applications.
One can run the example with bidirectional UDP video traffic (BV6 model) between both non-AP STAs when P2P is enabled:

::

  ./ns3 run "wifi-p2p --simulationTime=60s --staToAdhocTrafficType=Video --adhocToStaTrafficType=Video --staToAdhocVideoTrafficModelClassIdentifier=BV6 --adhocToStaVideoTrafficModelClassIdentifier=BV6"

  Direction       Throughput [Mbit/s]     E2E latency [ms]        TX [packets]    RX [packets]    Packet loss rate [%]
  AP -> STA       99.9924                 1.08171                 749999          749943          0.00746668
  STA -> AP       99.9847                 1.24282                 749999          749885          0.0152
  STA -> ADHOC    15.106                  3.04908                 2979            2909            2.34978
  ADHOC -> STA    15.7247                 2.71837                 2990            2986            0.133779
  STA <-> AP      199.977                 1.16226                 1499998         1499828         0.0113333
  STA <-> STA     30.8307                 2.88157                 5969            5895            1.23974
  Total           230.808                 1.169                   1505967         1505723         0.0162022
  Average backoff: 8.62726

Running the same scenario with P2P disabled results in a larger end-to-end latency:

::

  ./ns3 run "wifi-p2p --simulationTime=60s --staToAdhocTrafficType=Video --adhocToStaTrafficType=Video --staToAdhocVideoTrafficModelClassIdentifier=BV6 --adhocToStaVideoTrafficModelClassIdentifier=BV6 --p2p=0"

  Direction       Throughput [Mbit/s]     E2E latency [ms]        TX [packets]    RX [packets]    Packet loss rate [%]
  AP -> STA1      99.9924                 1.84597                 749999          749943          0.00746668
  STA1 -> AP      99.9863                 1.42636                 749999          749897          0.0136
  STA1 -> STA2    14.9665                 6.65692                 2996            2880            3.87183
  STA2 -> STA1    15.9874                 6.38151                 3044            3033            0.361367
  STA <-> AP      199.979                 1.63617                 1499998         1499840         0.0105333
  STA <-> STA     30.9539                 6.51565                 6040            5913            2.10265
  Total           230.933                 1.65533                 1506038         1505753         0.0189238
  Average backoff: 8.6573

The example can also be run considering multi-link devices (only the peer device has to be an SLD).
Below, non-AP MLD has two links setup with AP MLD, and also uses one of these links for P2P:

::

  ./ns3 run "wifi-p2p --simulationTime=60s --p2p=1 --numLinksAp=2 --numLinksSta=2 --numLinksP2p=1"

  Direction       Throughput [Mbit/s]     E2E latency [ms]        TX [packets]    RX [packets]    Packet loss rate [%]
  AP -> STA       99.8485                 2.61151                 749999          748864          0.151334
  STA -> AP       99.7887                 2.56302                 749999          748415          0.2112
  STA -> ADHOC    49.9999                 3.2351                  749999          749999          0
  ADHOC -> STA    49.9916                 2.32146                 749999          749874          0.0166667
  STA <-> AP      199.637                 2.58727                 1499998         1497279         0.181267
  STA <-> STA     99.9915                 2.77832                 1499998         1499873         0.00833334
  Total           299.629                 2.68288                 2999996         2997152         0.0948001
  Average backoff: 9.26737

Below we can compare the results when P2P is disabled and both STAs are non-AP MLD with both
two links setup with the AP MLD. Even though both links are used, we still achieve larger
end-to-end latency and a higher packet loss rate than with P2P enabled:

::

  ./ns3 run "wifi-p2p --simulationTime=60s --p2p=0 --numLinksAp=2 --numLinksSta=2"

  Direction       Throughput [Mbit/s]     E2E latency [ms]        TX [packets]    RX [packets]    Packet loss rate [%]
  AP -> STA1      99.9451                 9.17126                 749999          749588          0.0548001
  STA1 -> AP      99.9324                 7.16858                 749999          749493          0.0674668
  STA1 -> STA2    43.9577                 16.6999                 749999          659365          12.0845
  STA2 -> STA1    42.3493                 16.3365                 749999          635240          15.3012
  STA <-> AP      199.877                 8.16998                 1499998         1499081         0.0611334
  STA <-> STA     86.307                  16.5216                 1499998         1294605         13.6929
  Total           286.184                 12.0401                 2999996         2793686         6.87701
  Average backoff: 9.58594

It is also possible to use a dedicated link for P2P to reach better performance if the non-AP STA
has more links than the amount of links setup with the AP MLD. Below is an example considering the
non-AP MLD STA with two of its three links setup with the AP MLD and its third link dedicated to P2P:

::

  ./ns3 run "wifi-p2p --simulationTime=60s --p2p=1 --numLinksAp=2 --numLinksSta=3 --numLinksP2p=1 --linksOverlap=0"

  Direction       Throughput [Mbit/s]     E2E latency [ms]        TX [packets]    RX [packets]    Packet loss rate [%]
  AP -> STA       99.9497                 0.80848                 749999          749623          0.0501334
  STA -> AP       99.9495                 0.826879                749999          749621          0.0504001
  STA -> ADHOC    49.9999                 0.627259                749999          749999          0
  ADHOC -> STA    49.9965                 0.603084                749999          749948          0.00680001
  STA <-> AP      199.899                 0.817679                1499998         1499244         0.0502667
  STA <-> STA     99.9965                 0.615172                1499998         1499947         0.0034
  Total           299.896                 0.716402                2999996         2999191         0.0268334
  Average backoff: 8.53559
