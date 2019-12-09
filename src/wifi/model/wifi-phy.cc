/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/mobility-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/error-model.h"
#include "wifi-phy.h"
#include "wifi-phy-tag.h"
#include "ampdu-tag.h"
#include "wifi-utils.h"
#include "frame-capture-model.h"
#include "preamble-detection-model.h"
#include "wifi-radio-energy-model.h"
#include "error-rate-model.h"
#include "wifi-net-device.h"
#include "ht-configuration.h"
#include "he-configuration.h"
#include "mpdu-aggregator.h"
#include "wifi-phy-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPhy");

/****************************************************************
 *       The actual WifiPhy class
 ****************************************************************/

NS_OBJECT_ENSURE_REGISTERED (WifiPhy);

/**
 * This table maintains the mapping of valid ChannelNumber to
 * Frequency/ChannelWidth pairs.  If you want to make a channel applicable
 * to all standards, then you may use the WIFI_PHY_STANDARD_UNSPECIFIED
 * standard to represent this, as a wildcard.  If you want to limit the
 * configuration of a particular channel/frequency/width to a particular
 * standard(s), then you can specify one or more such bindings.
 */
WifiPhy::ChannelToFrequencyWidthMap WifiPhy::m_channelToFrequencyWidth =
{
  // 802.11b uses width of 22, while OFDM modes use width of 20
  { std::make_pair (1, WIFI_PHY_STANDARD_80211b), std::make_pair (2412, 22) },
  { std::make_pair (1, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2412, 20) },
  { std::make_pair (2, WIFI_PHY_STANDARD_80211b), std::make_pair (2417, 22) },
  { std::make_pair (2, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2417, 20) },
  { std::make_pair (3, WIFI_PHY_STANDARD_80211b), std::make_pair (2422, 22) },
  { std::make_pair (3, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2422, 20) },
  { std::make_pair (4, WIFI_PHY_STANDARD_80211b), std::make_pair (2427, 22) },
  { std::make_pair (4, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2427, 20) },
  { std::make_pair (5, WIFI_PHY_STANDARD_80211b), std::make_pair (2432, 22) },
  { std::make_pair (5, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2432, 20) },
  { std::make_pair (6, WIFI_PHY_STANDARD_80211b), std::make_pair (2437, 22) },
  { std::make_pair (6, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2437, 20) },
  { std::make_pair (7, WIFI_PHY_STANDARD_80211b), std::make_pair (2442, 22) },
  { std::make_pair (7, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2442, 20) },
  { std::make_pair (8, WIFI_PHY_STANDARD_80211b), std::make_pair (2447, 22) },
  { std::make_pair (8, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2447, 20) },
  { std::make_pair (9, WIFI_PHY_STANDARD_80211b), std::make_pair (2452, 22) },
  { std::make_pair (9, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2452, 20) },
  { std::make_pair (10, WIFI_PHY_STANDARD_80211b), std::make_pair (2457, 22) },
  { std::make_pair (10, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2457, 20) },
  { std::make_pair (11, WIFI_PHY_STANDARD_80211b), std::make_pair (2462, 22) },
  { std::make_pair (11, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2462, 20) },
  { std::make_pair (12, WIFI_PHY_STANDARD_80211b), std::make_pair (2467, 22) },
  { std::make_pair (12, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2467, 20) },
  { std::make_pair (13, WIFI_PHY_STANDARD_80211b), std::make_pair (2472, 22) },
  { std::make_pair (13, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (2472, 20) },
  // Only defined for 802.11b
  { std::make_pair (14, WIFI_PHY_STANDARD_80211b), std::make_pair (2484, 22) },

  // Now the 5GHz channels; UNSPECIFIED for 802.11a/n/ac/ax channels
  // 20 MHz channels
  { std::make_pair (36, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5180, 20) },
  { std::make_pair (40, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5200, 20) },
  { std::make_pair (44, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5220, 20) },
  { std::make_pair (48, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5240, 20) },
  { std::make_pair (52, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5260, 20) },
  { std::make_pair (56, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5280, 20) },
  { std::make_pair (60, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5300, 20) },
  { std::make_pair (64, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5320, 20) },
  { std::make_pair (100, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5500, 20) },
  { std::make_pair (104, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5520, 20) },
  { std::make_pair (108, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5540, 20) },
  { std::make_pair (112, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5560, 20) },
  { std::make_pair (116, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5580, 20) },
  { std::make_pair (120, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5600, 20) },
  { std::make_pair (124, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5620, 20) },
  { std::make_pair (128, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5640, 20) },
  { std::make_pair (132, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5660, 20) },
  { std::make_pair (136, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5680, 20) },
  { std::make_pair (140, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5700, 20) },
  { std::make_pair (144, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5720, 20) },
  { std::make_pair (149, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5745, 20) },
  { std::make_pair (153, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5765, 20) },
  { std::make_pair (157, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5785, 20) },
  { std::make_pair (161, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5805, 20) },
  { std::make_pair (165, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5825, 20) },
  { std::make_pair (169, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5845, 20) },
  { std::make_pair (173, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5865, 20) },
  { std::make_pair (177, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5885, 20) },
  { std::make_pair (181, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5905, 20) },
  // 40 MHz channels
  { std::make_pair (38, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5190, 40) },
  { std::make_pair (46, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5230, 40) },
  { std::make_pair (54, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5270, 40) },
  { std::make_pair (62, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5310, 40) },
  { std::make_pair (102, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5510, 40) },
  { std::make_pair (110, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5550, 40) },
  { std::make_pair (118, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5590, 40) },
  { std::make_pair (126, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5630, 40) },
  { std::make_pair (134, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5670, 40) },
  { std::make_pair (142, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5710, 40) },
  { std::make_pair (151, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5755, 40) },
  { std::make_pair (159, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5795, 40) },
  { std::make_pair (167, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5835, 40) },
  { std::make_pair (175, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5875, 40) },
  // 80 MHz channels
  { std::make_pair (42, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5210, 80) },
  { std::make_pair (58, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5290, 80) },
  { std::make_pair (106, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5530, 80) },
  { std::make_pair (122, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5610, 80) },
  { std::make_pair (138, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5690, 80) },
  { std::make_pair (155, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5775, 80) },
  { std::make_pair (171, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5855, 80) },
  // 160 MHz channels
  { std::make_pair (50, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5250, 160) },
  { std::make_pair (114, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5570, 160) },
  { std::make_pair (163, WIFI_PHY_STANDARD_UNSPECIFIED), std::make_pair (5815, 160) },

  // 802.11p (10 MHz channels at the 5.855-5.925 band
  { std::make_pair (172, WIFI_PHY_STANDARD_80211_10MHZ), std::make_pair (5860, 10) },
  { std::make_pair (174, WIFI_PHY_STANDARD_80211_10MHZ), std::make_pair (5870, 10) },
  { std::make_pair (176, WIFI_PHY_STANDARD_80211_10MHZ), std::make_pair (5880, 10) },
  { std::make_pair (178, WIFI_PHY_STANDARD_80211_10MHZ), std::make_pair (5890, 10) },
  { std::make_pair (180, WIFI_PHY_STANDARD_80211_10MHZ), std::make_pair (5900, 10) },
  { std::make_pair (182, WIFI_PHY_STANDARD_80211_10MHZ), std::make_pair (5910, 10) },
  { std::make_pair (184, WIFI_PHY_STANDARD_80211_10MHZ), std::make_pair (5920, 10) }
};

TypeId
WifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiPhy")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("Frequency",
                   "The operating center frequency (MHz)",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WifiPhy::GetFrequency,
                                         &WifiPhy::SetFrequency),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ChannelWidth",
                   "Whether 5MHz, 10MHz, 20MHz, 22MHz, 40MHz, 80 MHz or 160 MHz.",
                   UintegerValue (20),
                   MakeUintegerAccessor (&WifiPhy::GetChannelWidth,
                                         &WifiPhy::SetChannelWidth),
                   MakeUintegerChecker<uint16_t> (5, 160))
    .AddAttribute ("ChannelNumber",
                   "If set to non-zero defined value, will control Frequency and ChannelWidth assignment",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WifiPhy::SetChannelNumber,
                                         &WifiPhy::GetChannelNumber),
                   MakeUintegerChecker<uint8_t> (0, 196))
    .AddAttribute ("EnergyDetectionThreshold",
                   "The energy of a received signal should be higher than "
                   "this threshold (dbm) to allow the PHY layer to detect the signal.",
                   DoubleValue (-101.0),
                   MakeDoubleAccessor (&WifiPhy::SetEdThreshold),
                   MakeDoubleChecker<double> (),
                   TypeId::DEPRECATED, "Replaced by RxSensitivity.")
    .AddAttribute ("RxSensitivity",
                   "The energy of a received signal should be higher than "
                   "this threshold (dBm) for the PHY to detect the signal.",
                   DoubleValue (-101.0),
                   MakeDoubleAccessor (&WifiPhy::SetRxSensitivity,
                                       &WifiPhy::GetRxSensitivity),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("CcaEdThreshold",
                   "The energy of a non Wi-Fi received signal should be higher than "
                   "this threshold (dbm) to allow the PHY layer to declare CCA BUSY state. "
                   "This check is performed on the 20 MHz primary channel only.",
                   DoubleValue (-62.0),
                   MakeDoubleAccessor (&WifiPhy::SetCcaEdThreshold,
                                       &WifiPhy::GetCcaEdThreshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxGain",
                   "Transmission gain (dB).",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&WifiPhy::SetTxGain,
                                       &WifiPhy::GetTxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxGain",
                   "Reception gain (dB).",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&WifiPhy::SetRxGain,
                                       &WifiPhy::GetRxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxPowerLevels",
                   "Number of transmission power levels available between "
                   "TxPowerStart and TxPowerEnd included.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&WifiPhy::m_nTxPower),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("TxPowerEnd",
                   "Maximum available transmission level (dbm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&WifiPhy::SetTxPowerEnd,
                                       &WifiPhy::GetTxPowerEnd),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxPowerStart",
                   "Minimum available transmission level (dbm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&WifiPhy::SetTxPowerStart,
                                       &WifiPhy::GetTxPowerStart),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxNoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0 (usually 290 K)\".",
                   DoubleValue (7),
                   MakeDoubleAccessor (&WifiPhy::SetRxNoiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("State",
                   "The state of the PHY layer.",
                   PointerValue (),
                   MakePointerAccessor (&WifiPhy::m_state),
                   MakePointerChecker<WifiPhyStateHelper> ())
    .AddAttribute ("ChannelSwitchDelay",
                   "Delay between two short frames transmitted on different frequencies.",
                   TimeValue (MicroSeconds (250)),
                   MakeTimeAccessor (&WifiPhy::m_channelSwitchDelay),
                   MakeTimeChecker ())
    .AddAttribute ("Antennas",
                   "The number of antennas on the device.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&WifiPhy::GetNumberOfAntennas,
                                         &WifiPhy::SetNumberOfAntennas),
                   MakeUintegerChecker<uint8_t> (1, 8))
    .AddAttribute ("MaxSupportedTxSpatialStreams",
                   "The maximum number of supported TX spatial streams."
                   "This parameter is only valuable for 802.11n/ac/ax STAs and APs.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&WifiPhy::GetMaxSupportedTxSpatialStreams,
                                         &WifiPhy::SetMaxSupportedTxSpatialStreams),
                   MakeUintegerChecker<uint8_t> (1, 8))
    .AddAttribute ("MaxSupportedRxSpatialStreams",
                   "The maximum number of supported RX spatial streams."
                   "This parameter is only valuable for 802.11n/ac/ax STAs and APs.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&WifiPhy::GetMaxSupportedRxSpatialStreams,
                                         &WifiPhy::SetMaxSupportedRxSpatialStreams),
                   MakeUintegerChecker<uint8_t> (1, 8))
    .AddAttribute ("ShortGuardEnabled",
                   "Whether or not short guard interval is enabled for HT/VHT transmissions."
                   "This parameter is only valuable for 802.11n/ac/ax STAs and APs.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&WifiPhy::GetShortGuardInterval,
                                        &WifiPhy::SetShortGuardInterval),
                   MakeBooleanChecker (),
                   TypeId::DEPRECATED, "Use the HtConfiguration instead")
    .AddAttribute ("GuardInterval",
                   "Whether 800ns, 1600ns or 3200ns guard interval is used for HE transmissions."
                   "This parameter is only valuable for 802.11ax STAs and APs.",
                   TimeValue (NanoSeconds (3200)),
                   MakeTimeAccessor (&WifiPhy::GetGuardInterval,
                                     &WifiPhy::SetGuardInterval),
                   MakeTimeChecker (NanoSeconds (800), NanoSeconds (3200)),
                   TypeId::DEPRECATED, "Use the HeConfiguration instead")
    .AddAttribute ("GreenfieldEnabled",
                   "Whether or not Greenfield is enabled."
                   "This parameter is only valuable for 802.11n STAs and APs.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&WifiPhy::GetGreenfield,
                                        &WifiPhy::SetGreenfield),
                   MakeBooleanChecker (),
                   TypeId::DEPRECATED, "Use the HtConfiguration instead")
    .AddAttribute ("ShortPlcpPreambleSupported",
                   "Whether or not short PLCP preamble is supported."
                   "This parameter is only valuable for 802.11b STAs and APs."
                   "Note: 802.11g APs and STAs always support short PLCP preamble.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&WifiPhy::GetShortPlcpPreambleSupported,
                                        &WifiPhy::SetShortPlcpPreambleSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("FrameCaptureModel",
                   "Ptr to an object that implements the frame capture model",
                   PointerValue (),
                   MakePointerAccessor (&WifiPhy::m_frameCaptureModel),
                   MakePointerChecker <FrameCaptureModel> ())
    .AddAttribute ("PreambleDetectionModel",
                   "Ptr to an object that implements the preamble detection model",
                   PointerValue (),
                   MakePointerAccessor (&WifiPhy::m_preambleDetectionModel),
                   MakePointerChecker <PreambleDetectionModel> ())
    .AddAttribute ("PostReceptionErrorModel",
                   "An optional packet error model can be added to the receive "
                   "packet process after any propagation-based (SNR-based) error "
                   "models have been applied. Typically this is used to force "
                   "specific packet drops, for testing purposes.",
                   PointerValue (),
                   MakePointerAccessor (&WifiPhy::m_postReceptionErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddTraceSource ("PhyTxBegin",
                     "Trace source indicating a packet "
                     "has begun transmitting over the channel medium",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyTxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxEnd",
                     "Trace source indicating a packet "
                     "has been completely transmitted over the channel. "
                     "NOTE: the only official WifiPhy implementation "
                     "available to this date never fires "
                     "this trace source.",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyTxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxDrop",
                     "Trace source indicating a packet "
                     "has been dropped by the device during transmission",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxBegin",
                     "Trace source indicating a packet "
                     "has begun being received from the channel medium "
                     "by the device",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxEnd",
                     "Trace source indicating a packet "
                     "has been completely received from the channel medium "
                     "by the device",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyRxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet "
                     "has been dropped by the device during reception",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MonitorSnifferRx",
                     "Trace source simulating a wifi device in monitor mode "
                     "sniffing all received frames",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyMonitorSniffRxTrace),
                     "ns3::WifiPhy::MonitorSnifferRxTracedCallback")
    .AddTraceSource ("MonitorSnifferTx",
                     "Trace source simulating the capability of a wifi device "
                     "in monitor mode to sniff all frames being transmitted",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyMonitorSniffTxTrace),
                     "ns3::WifiPhy::MonitorSnifferTxTracedCallback")
    .AddTraceSource ("EndOfHePreamble",
                     "Trace source indicating the end of the 802.11ax preamble (after training fields)",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyEndOfHePreambleTrace),
                     "ns3::WifiPhy::EndOfHePreambleTracedCallback")
  ;
  return tid;
}

WifiPhy::WifiPhy ()
  : m_txMpduReferenceNumber (0xffffffff),
    m_rxMpduReferenceNumber (0xffffffff),
    m_endRxEvent (),
    m_endPlcpRxEvent (),
    m_endPreambleDetectionEvent (),
    m_standard (WIFI_PHY_STANDARD_UNSPECIFIED),
    m_isConstructed (false),
    m_channelCenterFrequency (0),
    m_initialFrequency (0),
    m_frequencyChannelNumberInitialized (false),
    m_channelWidth (0),
    m_channelAccessRequested (false),
    m_txSpatialStreams (0),
    m_rxSpatialStreams (0),
    m_channelNumber (0),
    m_initialChannelNumber (0),
    m_totalAmpduSize (0),
    m_totalAmpduNumSymbols (0),
    m_currentEvent (0),
    m_wifiRadioEnergyModel (0),
    m_timeLastPreambleDetected (Seconds (0))
{
  NS_LOG_FUNCTION (this);
  m_random = CreateObject<UniformRandomVariable> ();
  m_state = CreateObject<WifiPhyStateHelper> ();
}

WifiPhy::~WifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
WifiPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_endRxEvent.Cancel ();
  m_endPlcpRxEvent.Cancel ();
  m_endPreambleDetectionEvent.Cancel ();
  m_device = 0;
  m_mobility = 0;
  m_state = 0;
  m_wifiRadioEnergyModel = 0;
  m_postReceptionErrorModel = 0;
  m_deviceRateSet.clear ();
  m_deviceMcsSet.clear ();
}

void
WifiPhy::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_isConstructed = true;
  if (m_frequencyChannelNumberInitialized == true)
    {
      NS_LOG_DEBUG ("Frequency already initialized");
      return;
    }
  InitializeFrequencyChannelNumber ();
}

Ptr<WifiPhyStateHelper>
WifiPhy::GetState (void) const
{
  return m_state;
}

void
WifiPhy::SetReceiveOkCallback (RxOkCallback callback)
{
  m_state->SetReceiveOkCallback (callback);
}

void
WifiPhy::SetReceiveErrorCallback (RxErrorCallback callback)
{
  m_state->SetReceiveErrorCallback (callback);
}

void
WifiPhy::RegisterListener (WifiPhyListener *listener)
{
  m_state->RegisterListener (listener);
}

void
WifiPhy::UnregisterListener (WifiPhyListener *listener)
{
  m_state->UnregisterListener (listener);
}

void
WifiPhy::SetCapabilitiesChangedCallback (Callback<void> callback)
{
  m_capabilitiesChangedCallback = callback;
}

void
WifiPhy::InitializeFrequencyChannelNumber (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_frequencyChannelNumberInitialized == false, "Initialization called twice");

  // If frequency has been set to a non-zero value during attribute
  // construction phase, the frequency and channel width will drive the
  // initial configuration.  If frequency has not been set, but both
  // standard and channel number have been set, that pair will instead
  // drive the configuration, and frequency and channel number will be
  // aligned
  if (m_initialFrequency != 0)
    {
      SetFrequency (m_initialFrequency);
    }
  else if (m_initialChannelNumber != 0 && GetStandard () != WIFI_PHY_STANDARD_UNSPECIFIED)
    {
      SetChannelNumber (m_initialChannelNumber);
    }
  else if (m_initialChannelNumber != 0 && GetStandard () == WIFI_PHY_STANDARD_UNSPECIFIED)
    {
      NS_FATAL_ERROR ("Error, ChannelNumber " << +GetChannelNumber () << " was set by user, but neither a standard nor a frequency");
    }
  m_frequencyChannelNumberInitialized = true;
}

void
WifiPhy::SetEdThreshold (double threshold)
{
  SetRxSensitivity (threshold);
}

void
WifiPhy::SetRxSensitivity (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_rxSensitivityW = DbmToW (threshold);
}

double
WifiPhy::GetRxSensitivity (void) const
{
  return WToDbm (m_rxSensitivityW);
}

void
WifiPhy::SetCcaEdThreshold (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_ccaEdThresholdW = DbmToW (threshold);
}

double
WifiPhy::GetCcaEdThreshold (void) const
{
  return WToDbm (m_ccaEdThresholdW);
}

void
WifiPhy::SetRxNoiseFigure (double noiseFigureDb)
{
  NS_LOG_FUNCTION (this << noiseFigureDb);
  m_interference.SetNoiseFigure (DbToRatio (noiseFigureDb));
  m_interference.SetNumberOfReceiveAntennas (GetNumberOfAntennas ());
}

void
WifiPhy::SetTxPowerStart (double start)
{
  NS_LOG_FUNCTION (this << start);
  m_txPowerBaseDbm = start;
}

double
WifiPhy::GetTxPowerStart (void) const
{
  return m_txPowerBaseDbm;
}

void
WifiPhy::SetTxPowerEnd (double end)
{
  NS_LOG_FUNCTION (this << end);
  m_txPowerEndDbm = end;
}

double
WifiPhy::GetTxPowerEnd (void) const
{
  return m_txPowerEndDbm;
}

void
WifiPhy::SetNTxPower (uint8_t n)
{
  NS_LOG_FUNCTION (this << +n);
  m_nTxPower = n;
}

uint8_t
WifiPhy::GetNTxPower (void) const
{
  return m_nTxPower;
}

void
WifiPhy::SetTxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_txGainDb = gain;
}

double
WifiPhy::GetTxGain (void) const
{
  return m_txGainDb;
}

void
WifiPhy::SetRxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_rxGainDb = gain;
}

double
WifiPhy::GetRxGain (void) const
{
  return m_rxGainDb;
}

void
WifiPhy::SetGreenfield (bool greenfield)
{
  NS_LOG_FUNCTION (this << greenfield);
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
  if (device)
    {
      Ptr<HtConfiguration> htConfiguration = device->GetHtConfiguration ();
      if (htConfiguration)
        {
          htConfiguration->SetGreenfieldSupported (greenfield);
        }
    }
  m_greenfield = greenfield;
}

bool
WifiPhy::GetGreenfield (void) const
{
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
  if (device)
    {
      Ptr<HtConfiguration> htConfiguration = device->GetHtConfiguration ();
      if (htConfiguration)
        {
          return htConfiguration->GetGreenfieldSupported ();
        }
    }
  return m_greenfield;
}

void
WifiPhy::SetShortGuardInterval (bool shortGuardInterval)
{
  NS_LOG_FUNCTION (this << shortGuardInterval);
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
  if (device)
    {
      Ptr<HtConfiguration> htConfiguration = device->GetHtConfiguration ();
      if (htConfiguration)
        {
          htConfiguration->SetShortGuardIntervalSupported (shortGuardInterval);
        }
    }
  m_shortGuardInterval = shortGuardInterval;
}

bool
WifiPhy::GetShortGuardInterval (void) const
{
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
  if (device)
    {
      Ptr<HtConfiguration> htConfiguration = device->GetHtConfiguration ();
      if (htConfiguration)
        {
          return htConfiguration->GetShortGuardIntervalSupported ();
        }
    }
  return m_shortGuardInterval;
}

void
WifiPhy::SetGuardInterval (Time guardInterval)
{
  NS_LOG_FUNCTION (this << guardInterval);
  NS_ASSERT (guardInterval == NanoSeconds (800) || guardInterval == NanoSeconds (1600) || guardInterval == NanoSeconds (3200));
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
  if (device)
    {
      Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
      if (heConfiguration)
        {
          heConfiguration->SetGuardInterval (guardInterval);
        }
    }
  m_guardInterval = guardInterval;
}

Time
WifiPhy::GetGuardInterval (void) const
{
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
  if (device)
    {
      Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
      if (heConfiguration)
        {
          return heConfiguration->GetGuardInterval ();
        }
    }
  return m_guardInterval;
}

void
WifiPhy::SetShortPlcpPreambleSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_shortPreamble = enable;
}

bool
WifiPhy::GetShortPlcpPreambleSupported (void) const
{
  return m_shortPreamble;
}

void
WifiPhy::SetDevice (const Ptr<NetDevice> device)
{
  m_device = device;
  //TODO: to be removed once deprecated API is cleaned up
  Ptr<HtConfiguration> htConfiguration = DynamicCast<WifiNetDevice> (device)->GetHtConfiguration ();
  if (htConfiguration)
    {
      htConfiguration->SetShortGuardIntervalSupported (m_shortGuardInterval);
      htConfiguration->SetGreenfieldSupported (m_greenfield);
    }
  Ptr<HeConfiguration> heConfiguration = DynamicCast<WifiNetDevice> (device)->GetHeConfiguration ();
  if (heConfiguration)
    {
      heConfiguration->SetGuardInterval (m_guardInterval);
    }
}

Ptr<NetDevice>
WifiPhy::GetDevice (void) const
{
  return m_device;
}

void
WifiPhy::SetMobility (const Ptr<MobilityModel> mobility)
{
  m_mobility = mobility;
}

Ptr<MobilityModel>
WifiPhy::GetMobility (void) const
{
  if (m_mobility != 0)
    {
      return m_mobility;
    }
  else
    {
      return m_device->GetNode ()->GetObject<MobilityModel> ();
    }
}

void
WifiPhy::SetErrorRateModel (const Ptr<ErrorRateModel> rate)
{
  m_interference.SetErrorRateModel (rate);
  m_interference.SetNumberOfReceiveAntennas (GetNumberOfAntennas ());
}

void
WifiPhy::SetPostReceptionErrorModel (const Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_postReceptionErrorModel = em;
}

void
WifiPhy::SetFrameCaptureModel (const Ptr<FrameCaptureModel> model)
{
  m_frameCaptureModel = model;
}

void
WifiPhy::SetPreambleDetectionModel (const Ptr<PreambleDetectionModel> model)
{
  m_preambleDetectionModel = model;
}

void
WifiPhy::SetWifiRadioEnergyModel (const Ptr<WifiRadioEnergyModel> wifiRadioEnergyModel)
{
  m_wifiRadioEnergyModel = wifiRadioEnergyModel;
}

double
WifiPhy::GetPowerDbm (uint8_t power) const
{
  NS_ASSERT (m_txPowerBaseDbm <= m_txPowerEndDbm);
  NS_ASSERT (m_nTxPower > 0);
  double dbm;
  if (m_nTxPower > 1)
    {
      dbm = m_txPowerBaseDbm + power * (m_txPowerEndDbm - m_txPowerBaseDbm) / (m_nTxPower - 1);
    }
  else
    {
      NS_ASSERT_MSG (m_txPowerBaseDbm == m_txPowerEndDbm, "cannot have TxPowerEnd != TxPowerStart with TxPowerLevels == 1");
      dbm = m_txPowerBaseDbm;
    }
  return dbm;
}

Time
WifiPhy::GetChannelSwitchDelay (void) const
{
  return m_channelSwitchDelay;
}

double
WifiPhy::CalculateSnr (WifiTxVector txVector, double ber) const
{
  return m_interference.GetErrorRateModel ()->CalculateSnr (txVector, ber);
}

void
WifiPhy::ConfigureDefaultsForStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211a:
      SetChannelWidth (20);
      SetFrequency (5180);
      // Channel number should be aligned by SetFrequency () to 36
      NS_ASSERT (GetChannelNumber () == 36);
      break;
    case WIFI_PHY_STANDARD_80211b:
      SetChannelWidth (22);
      SetFrequency (2412);
      // Channel number should be aligned by SetFrequency () to 1
      NS_ASSERT (GetChannelNumber () == 1);
      break;
    case WIFI_PHY_STANDARD_80211g:
      SetChannelWidth (20);
      SetFrequency (2412);
      // Channel number should be aligned by SetFrequency () to 1
      NS_ASSERT (GetChannelNumber () == 1);
      break;
    case WIFI_PHY_STANDARD_80211_10MHZ:
      SetChannelWidth (10);
      SetFrequency (5860);
      // Channel number should be aligned by SetFrequency () to 172
      NS_ASSERT (GetChannelNumber () == 172);
      break;
    case WIFI_PHY_STANDARD_80211_5MHZ:
      SetChannelWidth (5);
      SetFrequency (5860);
      // Channel number should be aligned by SetFrequency () to 0
      NS_ASSERT (GetChannelNumber () == 0);
      break;
    case WIFI_PHY_STANDARD_holland:
      SetChannelWidth (20);
      SetFrequency (5180);
      // Channel number should be aligned by SetFrequency () to 36
      NS_ASSERT (GetChannelNumber () == 36);
      break;
    case WIFI_PHY_STANDARD_80211n_2_4GHZ:
      SetChannelWidth (20);
      SetFrequency (2412);
      // Channel number should be aligned by SetFrequency () to 1
      NS_ASSERT (GetChannelNumber () == 1);
      break;
    case WIFI_PHY_STANDARD_80211n_5GHZ:
      SetChannelWidth (20);
      SetFrequency (5180);
      // Channel number should be aligned by SetFrequency () to 36
      NS_ASSERT (GetChannelNumber () == 36);
      break;
    case WIFI_PHY_STANDARD_80211ac:
      SetChannelWidth (80);
      SetFrequency (5210);
      // Channel number should be aligned by SetFrequency () to 42
      NS_ASSERT (GetChannelNumber () == 42);
      break;
    case WIFI_PHY_STANDARD_80211ax_2_4GHZ:
      SetChannelWidth (20);
      SetFrequency (2412);
      // Channel number should be aligned by SetFrequency () to 1
      NS_ASSERT (GetChannelNumber () == 1);
      break;
    case WIFI_PHY_STANDARD_80211ax_5GHZ:
      SetChannelWidth (80);
      SetFrequency (5210);
      // Channel number should be aligned by SetFrequency () to 42
      NS_ASSERT (GetChannelNumber () == 42);
      break;
    case WIFI_PHY_STANDARD_UNSPECIFIED:
    default:
      NS_LOG_WARN ("Configuring unspecified standard; performing no action");
      break;
    }
}

void
WifiPhy::Configure80211a (void)
{
  NS_LOG_FUNCTION (this);

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate48Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate54Mbps ());
}

void
WifiPhy::Configure80211b (void)
{
  NS_LOG_FUNCTION (this);

  m_deviceRateSet.push_back (WifiPhy::GetDsssRate1Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate2Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate5_5Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate11Mbps ());
}

void
WifiPhy::Configure80211g (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211b ();

  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate9Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate24Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate48Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate54Mbps ());
}

void
WifiPhy::Configure80211_10Mhz (void)
{
  NS_LOG_FUNCTION (this);

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate27MbpsBW10MHz ());
}

void
WifiPhy::Configure80211_5Mhz (void)
{
  NS_LOG_FUNCTION (this);

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate1_5MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate2_25MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate13_5MbpsBW5MHz ());
}

void
WifiPhy::ConfigureHolland (void)
{
  NS_LOG_FUNCTION (this);

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate54Mbps ());
}

void
WifiPhy::ConfigureHtDeviceMcsSet (void)
{
  NS_LOG_FUNCTION (this);

  bool htFound = false;
  for (std::vector<uint8_t>::size_type i = 0; i < m_bssMembershipSelectorSet.size (); i++)
    {
      if (m_bssMembershipSelectorSet[i] == HT_PHY)
        {
          htFound = true;
          break;
        }
    }
  if (htFound)
    {
      // erase all HtMcs modes from deviceMcsSet
      std::size_t index = m_deviceMcsSet.size () - 1;
      for (std::vector<WifiMode>::reverse_iterator rit = m_deviceMcsSet.rbegin (); rit != m_deviceMcsSet.rend (); ++rit, --index)
        {
          if (m_deviceMcsSet[index].GetModulationClass () == WIFI_MOD_CLASS_HT)
            {
              m_deviceMcsSet.erase (m_deviceMcsSet.begin () + index);
            }
        }
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs0 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs1 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs2 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs3 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs4 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs5 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs6 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs7 ());
      if (GetMaxSupportedTxSpatialStreams () > 1)
        {
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs8 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs9 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs10 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs11 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs12 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs13 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs14 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs15 ());
        }
      if (GetMaxSupportedTxSpatialStreams () > 2)
        {
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs16 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs17 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs18 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs19 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs20 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs21 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs22 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs23 ());
        }
      if (GetMaxSupportedTxSpatialStreams () > 3)
        {
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs24 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs25 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs26 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs27 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs28 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs29 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs30 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs31 ());
        }
    }
}

void
WifiPhy::Configure80211n (void)
{
  NS_LOG_FUNCTION (this);
  if (Is2_4Ghz (GetFrequency ()))
    {
      Configure80211b ();
      Configure80211g ();
    }
  if (Is5Ghz (GetFrequency ()))
    {
      Configure80211a ();
    }
  m_bssMembershipSelectorSet.push_back (HT_PHY);
  ConfigureHtDeviceMcsSet ();
}

void
WifiPhy::Configure80211ac (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211n ();

  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs0 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs1 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs2 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs3 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs4 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs5 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs6 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs7 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs8 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs9 ());

  m_bssMembershipSelectorSet.push_back (VHT_PHY);
}

void
WifiPhy::Configure80211ax (void)
{
  NS_LOG_FUNCTION (this);
  if (Is5Ghz (GetFrequency ()))
    {
      Configure80211ac ();
    }
  else
    {
      Configure80211n ();
    }

  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs0 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs1 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs2 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs3 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs4 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs5 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs6 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs7 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs8 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs9 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs10 ());
  m_deviceMcsSet.push_back (WifiPhy::GetHeMcs11 ());

  m_bssMembershipSelectorSet.push_back (HE_PHY);
}

bool
WifiPhy::DefineChannelNumber (uint8_t channelNumber, WifiPhyStandard standard, uint16_t frequency, uint16_t channelWidth)
{
  NS_LOG_FUNCTION (this << +channelNumber << standard << frequency << channelWidth);
  ChannelNumberStandardPair p = std::make_pair (channelNumber, standard);
  ChannelToFrequencyWidthMap::const_iterator it;
  it = m_channelToFrequencyWidth.find (p);
  if (it != m_channelToFrequencyWidth.end ())
    {
      NS_LOG_DEBUG ("channel number/standard already defined; returning false");
      return false;
    }
  FrequencyWidthPair f = std::make_pair (frequency, channelWidth);
  m_channelToFrequencyWidth[p] = f;
  return true;
}

uint8_t
WifiPhy::FindChannelNumberForFrequencyWidth (uint16_t frequency, uint16_t width) const
{
  NS_LOG_FUNCTION (this << frequency << width);
  bool found = false;
  FrequencyWidthPair f = std::make_pair (frequency, width);
  ChannelToFrequencyWidthMap::const_iterator it = m_channelToFrequencyWidth.begin ();
  while (it != m_channelToFrequencyWidth.end ())
    {
      if (it->second == f)
        {
          found = true;
          break;
        }
      ++it;
    }
  if (found)
    {
      NS_LOG_DEBUG ("Found, returning " << +it->first.first);
      return (it->first.first);
    }
  else
    {
      NS_LOG_DEBUG ("Not found, returning 0");
      return 0;
    }
}

void
WifiPhy::ConfigureChannelForStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  // If the user has configured both Frequency and ChannelNumber, Frequency
  // takes precedence
  if (GetFrequency () != 0)
    {
      // If Frequency is already set, then see whether a ChannelNumber can
      // be found that matches Frequency and ChannelWidth. If so, configure
      // the ChannelNumber to that channel number. If not, set ChannelNumber to zero.
      NS_LOG_DEBUG ("Frequency set; checking whether a channel number corresponds");
      uint8_t channelNumberSearched = FindChannelNumberForFrequencyWidth (GetFrequency (), GetChannelWidth ());
      if (channelNumberSearched)
        {
          NS_LOG_DEBUG ("Channel number found; setting to " << +channelNumberSearched);
          SetChannelNumber (channelNumberSearched);
        }
      else
        {
          NS_LOG_DEBUG ("Channel number not found; setting to zero");
          SetChannelNumber (0);
        }
    }
  else if (GetChannelNumber () != 0)
    {
      // If the channel number is known for this particular standard or for
      // the unspecified standard, configure using the known values;
      // otherwise, this is a configuration error
      NS_LOG_DEBUG ("Configuring for channel number " << +GetChannelNumber ());
      FrequencyWidthPair f = GetFrequencyWidthForChannelNumberStandard (GetChannelNumber (), standard);
      if (f.first == 0)
        {
          // the specific pair of number/standard is not known
          NS_LOG_DEBUG ("Falling back to check WIFI_PHY_STANDARD_UNSPECIFIED");
          f = GetFrequencyWidthForChannelNumberStandard (GetChannelNumber (), WIFI_PHY_STANDARD_UNSPECIFIED);
        }
      if (f.first == 0)
        {
          NS_FATAL_ERROR ("Error, ChannelNumber " << +GetChannelNumber () << " is unknown for this standard");
        }
      else
        {
          NS_LOG_DEBUG ("Setting frequency to " << f.first << "; width to " << +f.second);
          SetFrequency (f.first);
          SetChannelWidth (f.second);
        }
    }
}

void
WifiPhy::ConfigureStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  m_standard = standard;
  m_isConstructed = true;
  if (m_frequencyChannelNumberInitialized == false)
    {
      InitializeFrequencyChannelNumber ();
    }
  if (GetFrequency () == 0 && GetChannelNumber () == 0)
    {
      ConfigureDefaultsForStandard (standard);
    }
  else
    {
      // The user has configured either (or both) Frequency or ChannelNumber
      ConfigureChannelForStandard (standard);
    }
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211a:
      Configure80211a ();
      break;
    case WIFI_PHY_STANDARD_80211b:
      Configure80211b ();
      break;
    case WIFI_PHY_STANDARD_80211g:
      Configure80211g ();
      break;
    case WIFI_PHY_STANDARD_80211_10MHZ:
      Configure80211_10Mhz ();
      break;
    case WIFI_PHY_STANDARD_80211_5MHZ:
      Configure80211_5Mhz ();
      break;
    case WIFI_PHY_STANDARD_holland:
      ConfigureHolland ();
      break;
    case WIFI_PHY_STANDARD_80211n_2_4GHZ:
    case WIFI_PHY_STANDARD_80211n_5GHZ:
      Configure80211n ();
      break;
    case WIFI_PHY_STANDARD_80211ac:
      Configure80211ac ();
      break;
    case WIFI_PHY_STANDARD_80211ax_2_4GHZ:
    case WIFI_PHY_STANDARD_80211ax_5GHZ:
      Configure80211ax ();
      break;
    case WIFI_PHY_STANDARD_UNSPECIFIED:
    default:
      NS_ASSERT (false);
      break;
    }
}

WifiPhyStandard
WifiPhy::GetStandard (void) const
{
  return m_standard;
}

void
WifiPhy::SetFrequency (uint16_t frequency)
{
  NS_LOG_FUNCTION (this << frequency);
  if (m_isConstructed == false)
    {
      NS_LOG_DEBUG ("Saving frequency configuration for initialization");
      m_initialFrequency = frequency;
      return;
    }
  if (GetFrequency () == frequency)
    {
      NS_LOG_DEBUG ("No frequency change requested");
      return;
    }
  if (frequency == 0)
    {
      DoFrequencySwitch (0);
      NS_LOG_DEBUG ("Setting frequency and channel number to zero");
      m_channelCenterFrequency = 0;
      m_channelNumber = 0;
      return;
    }
  // If the user has configured both Frequency and ChannelNumber, Frequency
  // takes precedence.  Lookup the channel number corresponding to the
  // requested frequency.
  uint8_t nch = FindChannelNumberForFrequencyWidth (frequency, GetChannelWidth ());
  if (nch != 0)
    {
      NS_LOG_DEBUG ("Setting frequency " << frequency << " corresponds to channel " << +nch);
      if (DoFrequencySwitch (frequency))
        {
          NS_LOG_DEBUG ("Channel frequency switched to " << frequency << "; channel number to " << +nch);
          m_channelCenterFrequency = frequency;
          m_channelNumber = nch;
        }
      else
        {
          NS_LOG_DEBUG ("Suppressing reassignment of frequency");
        }
    }
  else
    {
      NS_LOG_DEBUG ("Channel number is unknown for frequency " << frequency);
      if (DoFrequencySwitch (frequency))
        {
          NS_LOG_DEBUG ("Channel frequency switched to " << frequency << "; channel number to " << 0);
          m_channelCenterFrequency = frequency;
          m_channelNumber = 0;
        }
      else
        {
          NS_LOG_DEBUG ("Suppressing reassignment of frequency");
        }
    }
}

uint16_t
WifiPhy::GetFrequency (void) const
{
  return m_channelCenterFrequency;
}

void
WifiPhy::SetChannelWidth (uint16_t channelwidth)
{
  NS_LOG_FUNCTION (this << channelwidth);
  NS_ASSERT_MSG (channelwidth == 5 || channelwidth == 10 || channelwidth == 20 || channelwidth == 22 || channelwidth == 40 || channelwidth == 80 || channelwidth == 160, "wrong channel width value");
  bool changed = (m_channelWidth == channelwidth);
  m_channelWidth = channelwidth;
  AddSupportedChannelWidth (channelwidth);
  if (changed && !m_capabilitiesChangedCallback.IsNull ())
    {
      m_capabilitiesChangedCallback ();
    }
}

uint16_t
WifiPhy::GetChannelWidth (void) const
{
  return m_channelWidth;
}

void
WifiPhy::SetNumberOfAntennas (uint8_t antennas)
{
  NS_ASSERT_MSG (antennas > 0 && antennas <= 4, "unsupported number of antennas");
  m_numberOfAntennas = antennas;
  m_interference.SetNumberOfReceiveAntennas (antennas);
}

uint8_t
WifiPhy::GetNumberOfAntennas (void) const
{
  return m_numberOfAntennas;
}

void
WifiPhy::SetMaxSupportedTxSpatialStreams (uint8_t streams)
{
  NS_ASSERT (streams <= GetNumberOfAntennas ());
  bool changed = (m_txSpatialStreams == streams);
  m_txSpatialStreams = streams;
  ConfigureHtDeviceMcsSet ();
  if (changed && !m_capabilitiesChangedCallback.IsNull ())
    {
      m_capabilitiesChangedCallback ();
    }
}

uint8_t
WifiPhy::GetMaxSupportedTxSpatialStreams (void) const
{
  return m_txSpatialStreams;
}

void
WifiPhy::SetMaxSupportedRxSpatialStreams (uint8_t streams)
{
  NS_ASSERT (streams <= GetNumberOfAntennas ());
  bool changed = (m_rxSpatialStreams == streams);
  m_rxSpatialStreams = streams;
  if (changed && !m_capabilitiesChangedCallback.IsNull ())
    {
      m_capabilitiesChangedCallback ();
    }
}

uint8_t
WifiPhy::GetMaxSupportedRxSpatialStreams (void) const
{
  return m_rxSpatialStreams;
}

uint8_t
WifiPhy::GetNBssMembershipSelectors (void) const
{
  return static_cast<uint8_t> (m_bssMembershipSelectorSet.size ());
}

uint8_t
WifiPhy::GetBssMembershipSelector (uint8_t selector) const
{
  return m_bssMembershipSelectorSet[selector];
}

void
WifiPhy::AddSupportedChannelWidth (uint16_t width)
{
  NS_LOG_FUNCTION (this << width);
  for (std::vector<uint32_t>::size_type i = 0; i != m_supportedChannelWidthSet.size (); i++)
    {
      if (m_supportedChannelWidthSet[i] == width)
        {
          return;
        }
    }
  NS_LOG_FUNCTION ("Adding " << width << " to supported channel width set");
  m_supportedChannelWidthSet.push_back (width);
}

std::vector<uint16_t>
WifiPhy::GetSupportedChannelWidthSet (void) const
{
  return m_supportedChannelWidthSet;
}

WifiPhy::FrequencyWidthPair
WifiPhy::GetFrequencyWidthForChannelNumberStandard (uint8_t channelNumber, WifiPhyStandard standard) const
{
  ChannelNumberStandardPair p = std::make_pair (channelNumber, standard);
  FrequencyWidthPair f = m_channelToFrequencyWidth[p];
  return f;
}

void
WifiPhy::SetChannelNumber (uint8_t nch)
{
  NS_LOG_FUNCTION (this << +nch);
  if (m_isConstructed == false)
    {
      NS_LOG_DEBUG ("Saving channel number configuration for initialization");
      m_initialChannelNumber = nch;
      return;
    }
  if (GetChannelNumber () == nch)
    {
      NS_LOG_DEBUG ("No channel change requested");
      return;
    }
  if (nch == 0)
    {
      // This case corresponds to when there is not a known channel
      // number for the requested frequency.  There is no need to call
      // DoChannelSwitch () because DoFrequencySwitch () should have been
      // called by the client
      NS_LOG_DEBUG ("Setting channel number to zero");
      m_channelNumber = 0;
      return;
    }

  // First make sure that the channel number is defined for the standard
  // in use
  FrequencyWidthPair f = GetFrequencyWidthForChannelNumberStandard (nch, GetStandard ());
  if (f.first == 0)
    {
      f = GetFrequencyWidthForChannelNumberStandard (nch, WIFI_PHY_STANDARD_UNSPECIFIED);
    }
  if (f.first != 0)
    {
      if (DoChannelSwitch (nch))
        {
          NS_LOG_DEBUG ("Setting frequency to " << f.first << "; width to " << +f.second);
          m_channelCenterFrequency = f.first;
          SetChannelWidth (f.second);
          m_channelNumber = nch;
        }
      else
        {
          // Subclass may have suppressed (e.g. waiting for state change)
          NS_LOG_DEBUG ("Channel switch suppressed");
        }
    }
  else
    {
      NS_FATAL_ERROR ("Frequency not found for channel number " << +nch);
    }
}

uint8_t
WifiPhy::GetChannelNumber (void) const
{
  return m_channelNumber;
}

bool
WifiPhy::DoChannelSwitch (uint8_t nch)
{
  m_powerRestricted = false;
  m_channelAccessRequested = false;
  if (!IsInitialized ())
    {
      //this is not channel switch, this is initialization
      NS_LOG_DEBUG ("initialize to channel " << +nch);
      return true;
    }

  NS_ASSERT (!IsStateSwitching ());
  switch (m_state->GetState ())
    {
    case WifiPhyState::RX:
      NS_LOG_DEBUG ("drop packet because of channel switching while reception");
      m_endPlcpRxEvent.Cancel ();
      m_endRxEvent.Cancel ();
      m_endPreambleDetectionEvent.Cancel ();
      goto switchChannel;
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("channel switching postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetChannelNumber, this, nch);
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      if (m_endPreambleDetectionEvent.IsRunning ())
        {
          m_endPreambleDetectionEvent.Cancel ();
          m_endRxEvent.Cancel ();
        }
      goto switchChannel;
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("channel switching ignored in sleep mode");
      break;
    default:
      NS_ASSERT (false);
      break;
    }

  return false;

switchChannel:

  NS_LOG_DEBUG ("switching channel " << +GetChannelNumber () << " -> " << +nch);
  m_state->SwitchToChannelSwitching (GetChannelSwitchDelay ());
  m_interference.EraseEvents ();
  /*
   * Needed here to be able to correctly sensed the medium for the first
   * time after the switching. The actual switching is not performed until
   * after m_channelSwitchDelay. Packets received during the switching
   * state are added to the event list and are employed later to figure
   * out the state of the medium after the switching.
   */
  return true;
}

bool
WifiPhy::DoFrequencySwitch (uint16_t frequency)
{
  m_powerRestricted = false;
  m_channelAccessRequested = false;
  if (!IsInitialized ())
    {
      //this is not channel switch, this is initialization
      NS_LOG_DEBUG ("start at frequency " << frequency);
      return true;
    }

  NS_ASSERT (!IsStateSwitching ());
  switch (m_state->GetState ())
    {
    case WifiPhyState::RX:
      NS_LOG_DEBUG ("drop packet because of channel/frequency switching while reception");
      m_endPlcpRxEvent.Cancel ();
      m_endRxEvent.Cancel ();
      m_endPreambleDetectionEvent.Cancel ();
      goto switchFrequency;
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("channel/frequency switching postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetFrequency, this, frequency);
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      if (m_endPreambleDetectionEvent.IsRunning ())
        {
          m_endPreambleDetectionEvent.Cancel ();
          m_endRxEvent.Cancel ();
        }
      goto switchFrequency;
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("frequency switching ignored in sleep mode");
      break;
    default:
      NS_ASSERT (false);
      break;
    }

  return false;

switchFrequency:

  NS_LOG_DEBUG ("switching frequency " << GetFrequency () << " -> " << frequency);
  m_state->SwitchToChannelSwitching (GetChannelSwitchDelay ());
  m_interference.EraseEvents ();
  /*
   * Needed here to be able to correctly sensed the medium for the first
   * time after the switching. The actual switching is not performed until
   * after m_channelSwitchDelay. Packets received during the switching
   * state are added to the event list and are employed later to figure
   * out the state of the medium after the switching.
   */
  return true;
}

void
WifiPhy::SetSleepMode (void)
{
  NS_LOG_FUNCTION (this);
  m_powerRestricted = false;
  m_channelAccessRequested = false;
  switch (m_state->GetState ())
    {
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetSleepMode, this);
      break;
    case WifiPhyState::RX:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of current reception");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetSleepMode, this);
      break;
    case WifiPhyState::SWITCHING:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of channel switching");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetSleepMode, this);
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      NS_LOG_DEBUG ("setting sleep mode");
      m_state->SwitchToSleep ();
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("already in sleep mode");
      break;
    default:
      NS_ASSERT (false);
      break;
    }
}

void
WifiPhy::SetOffMode (void)
{
  NS_LOG_FUNCTION (this);
  m_powerRestricted = false;
  m_channelAccessRequested = false;
  m_endPlcpRxEvent.Cancel ();
  m_endRxEvent.Cancel ();
  m_endPreambleDetectionEvent.Cancel ();
  m_state->SwitchToOff ();
}

void
WifiPhy::ResumeFromSleep (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state->GetState ())
    {
    case WifiPhyState::TX:
    case WifiPhyState::RX:
    case WifiPhyState::IDLE:
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::SWITCHING:
      {
        NS_LOG_DEBUG ("not in sleep mode, there is nothing to resume");
        break;
      }
    case WifiPhyState::SLEEP:
      {
        NS_LOG_DEBUG ("resuming from sleep mode");
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW);
        m_state->SwitchFromSleep (delayUntilCcaEnd);
        break;
      }
    default:
      {
        NS_ASSERT (false);
        break;
      }
    }
}

void
WifiPhy::ResumeFromOff (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state->GetState ())
    {
    case WifiPhyState::TX:
    case WifiPhyState::RX:
    case WifiPhyState::IDLE:
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::SWITCHING:
    case WifiPhyState::SLEEP:
      {
        NS_LOG_DEBUG ("not in off mode, there is nothing to resume");
        break;
      }
    case WifiPhyState::OFF:
      {
        NS_LOG_DEBUG ("resuming from off mode");
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW);
        m_state->SwitchFromOff (delayUntilCcaEnd);
        break;
      }
    default:
      {
        NS_ASSERT (false);
        break;
      }
    }
}

WifiMode
WifiPhy::GetHtPlcpHeaderMode ()
{
  return WifiPhy::GetHtMcs0 ();
}

WifiMode
WifiPhy::GetVhtPlcpHeaderMode ()
{
  return WifiPhy::GetVhtMcs0 ();
}

WifiMode
WifiPhy::GetHePlcpHeaderMode ()
{
  return WifiPhy::GetHeMcs0 ();
}

Time
WifiPhy::GetPreambleDetectionDuration (void)
{
  return MicroSeconds (4);
}

Time
WifiPhy::GetPlcpTrainingSymbolDuration (WifiTxVector txVector)
{
  uint8_t Ndltf, Neltf;
  //We suppose here that STBC = 0.
  //If STBC > 0, we need a different mapping between Nss and Nltf (IEEE 802.11n-2012 standard, page 1682).
  if (txVector.GetNss () < 3)
    {
      Ndltf = txVector.GetNss ();
    }
  else if (txVector.GetNss () < 5)
    {
      Ndltf = 4;
    }
  else if (txVector.GetNss () < 7)
    {
      Ndltf = 6;
    }
  else
    {
      Ndltf = 8;
    }

  if (txVector.GetNess () < 3)
    {
      Neltf = txVector.GetNess ();
    }
  else
    {
      Neltf = 4;
    }

  switch (txVector.GetPreambleType ())
    {
    case WIFI_PREAMBLE_HT_MF:
      return MicroSeconds (4 + (4 * Ndltf) + (4 * Neltf));
    case WIFI_PREAMBLE_HT_GF:
      return MicroSeconds ((4 * Ndltf) + (4 * Neltf));
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_VHT_MU:
      return MicroSeconds (4 + (4 * Ndltf));
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_HE_MU:
      return MicroSeconds (4 + (8 * Ndltf));
    default:
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetPlcpHtSigHeaderDuration (WifiPreamble preamble)
{
  switch (preamble)
    {
    case WIFI_PREAMBLE_HT_MF:
    case WIFI_PREAMBLE_HT_GF:
      //HT-SIG
      return MicroSeconds (8);
    default:
      //no HT-SIG for non HT
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetPlcpSigA1Duration (WifiPreamble preamble)
{
  switch (preamble)
    {
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_MU:
      //VHT-SIG-A1 and HE-SIG-A1
      return MicroSeconds (4);
    default:
      // no SIG-A1
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetPlcpSigA2Duration (WifiPreamble preamble)
{
  switch (preamble)
    {
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_MU:
      //VHT-SIG-A2 and HE-SIG-A2
      return MicroSeconds (4);
    default:
      // no SIG-A2
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetPlcpSigBDuration (WifiPreamble preamble)
{
  switch (preamble)
    {
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_MU:
      return MicroSeconds (4);
    default:
      // no SIG-B
      return MicroSeconds (0);
    }
}

WifiMode
WifiPhy::GetPlcpHeaderMode (WifiTxVector txVector)
{
  switch (txVector.GetMode ().GetModulationClass ())
    {
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
    case WIFI_MOD_CLASS_HE:
      switch (txVector.GetChannelWidth ())
        {
        case 5:
          return WifiPhy::GetOfdmRate1_5MbpsBW5MHz ();
        case 10:
          return WifiPhy::GetOfdmRate3MbpsBW10MHz ();
        case 20:
        case 40:
        case 80:
        case 160:
        default:
          //(Section 18.3.2 "PLCP frame format"; IEEE Std 802.11-2012)
          //actually this is only the first part of the PlcpHeader,
          //because the last 16 bits of the PlcpHeader are using the
          //same mode of the payload
          return WifiPhy::GetOfdmRate6Mbps ();
        }
    case WIFI_MOD_CLASS_ERP_OFDM:
      return WifiPhy::GetErpOfdmRate6Mbps ();
    case WIFI_MOD_CLASS_DSSS:
    case WIFI_MOD_CLASS_HR_DSSS:
      if (txVector.GetPreambleType () == WIFI_PREAMBLE_LONG || txVector.GetMode () == WifiPhy::GetDsssRate1Mbps ())
        {
          //(Section 16.2.3 "PLCP field definitions" and Section 17.2.2.2 "Long PPDU format"; IEEE Std 802.11-2012)
          return WifiPhy::GetDsssRate1Mbps ();
        }
      else //WIFI_PREAMBLE_SHORT
        {
          //(Section 17.2.2.3 "Short PPDU format"; IEEE Std 802.11-2012)
          return WifiPhy::GetDsssRate2Mbps ();
        }
    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return WifiMode ();
    }
}

Time
WifiPhy::GetPlcpHeaderDuration (WifiTxVector txVector)
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  switch (txVector.GetMode ().GetModulationClass ())
    {
    case WIFI_MOD_CLASS_OFDM:
      {
        switch (txVector.GetChannelWidth ())
          {
          case 20:
          default:
            //(Section 18.3.3 "PLCP preamble (SYNC))" and Figure 18-4 "OFDM training structure"; IEEE Std 802.11-2012)
            //also (Section 18.3.2.4 "Timing related parameters" Table 18-5 "Timing-related parameters"; IEEE Std 802.11-2012)
            //We return the duration of the SIGNAL field only, since the
            //SERVICE field (which strictly speaking belongs to the PLCP
            //header, see Section 18.3.2 and Figure 18-1) is sent using the
            //payload mode.
            return MicroSeconds (4);
          case 10:
            //(Section 18.3.2.4 "Timing related parameters" Table 18-5 "Timing-related parameters"; IEEE Std 802.11-2012)
            return MicroSeconds (8);
          case 5:
            //(Section 18.3.2.4 "Timing related parameters" Table 18-5 "Timing-related parameters"; IEEE Std 802.11-2012)
            return MicroSeconds (16);
          }
      }
    case WIFI_MOD_CLASS_HT:
      {
        //L-SIG
        //IEEE 802.11n Figure 20.1
        switch (preamble)
          {
          case WIFI_PREAMBLE_HT_MF:
          default:
            return MicroSeconds (4);
          case WIFI_PREAMBLE_HT_GF:
            return MicroSeconds (0);
          }
      }
    case WIFI_MOD_CLASS_ERP_OFDM:
    case WIFI_MOD_CLASS_VHT:
      //L-SIG
      return MicroSeconds (4);
    case WIFI_MOD_CLASS_HE:
      //LSIG + R-LSIG
      return MicroSeconds (8);
    case WIFI_MOD_CLASS_DSSS:
    case WIFI_MOD_CLASS_HR_DSSS:
      if ((preamble == WIFI_PREAMBLE_SHORT) && (txVector.GetMode ().GetDataRate (22) > 1000000))
        {
          //(Section 17.2.2.3 "Short PPDU format" and Figure 17-2 "Short PPDU format"; IEEE Std 802.11-2012)
          return MicroSeconds (24);
        }
      else //WIFI_PREAMBLE_LONG
        {
          //(Section 17.2.2.2 "Long PPDU format" and Figure 17-1 "Short PPDU format"; IEEE Std 802.11-2012)
          return MicroSeconds (48);
        }
    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetStartOfPacketDuration (WifiTxVector txVector)
{
  return MicroSeconds (4);
}

Time
WifiPhy::GetPlcpPreambleDuration (WifiTxVector txVector)
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  switch (txVector.GetMode ().GetModulationClass ())
    {
    case WIFI_MOD_CLASS_OFDM:
      {
        switch (txVector.GetChannelWidth ())
          {
          case 20:
          default:
            //(Section 18.3.3 "PLCP preamble (SYNC))" Figure 18-4 "OFDM training structure"
            //also Section 18.3.2.3 "Modulation-dependent parameters" Table 18-4 "Modulation-dependent parameters"; IEEE Std 802.11-2012)
            return MicroSeconds (16);
          case 10:
            //(Section 18.3.3 "PLCP preamble (SYNC))" Figure 18-4 "OFDM training structure"
            //also Section 18.3.2.3 "Modulation-dependent parameters" Table 18-4 "Modulation-dependent parameters"; IEEE Std 802.11-2012)
            return MicroSeconds (32);
          case 5:
            //(Section 18.3.3 "PLCP preamble (SYNC))" Figure 18-4 "OFDM training structure"
            //also Section 18.3.2.3 "Modulation-dependent parameters" Table 18-4 "Modulation-dependent parameters"; IEEE Std 802.11-2012)
            return MicroSeconds (64);
          }
      }
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
    case WIFI_MOD_CLASS_HE:
      //L-STF + L-LTF
      return MicroSeconds (16);
    case WIFI_MOD_CLASS_ERP_OFDM:
      return MicroSeconds (16);
    case WIFI_MOD_CLASS_DSSS:
    case WIFI_MOD_CLASS_HR_DSSS:
      if ((preamble == WIFI_PREAMBLE_SHORT) && (txVector.GetMode ().GetDataRate (22) > 1000000))
        {
          //(Section 17.2.2.3 "Short PPDU format)" Figure 17-2 "Short PPDU format"; IEEE Std 802.11-2012)
          return MicroSeconds (72);
        }
      else //WIFI_PREAMBLE_LONG
        {
          //(Section 17.2.2.2 "Long PPDU format)" Figure 17-1 "Long PPDU format"; IEEE Std 802.11-2012)
          return MicroSeconds (144);
        }
    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetPayloadDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency)
{
  return GetPayloadDuration (size, txVector, frequency, NORMAL_MPDU, 0);
}

Time
WifiPhy::GetPayloadDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency,
                             MpduType mpdutype, uint8_t incFlag)
{
  WifiMode payloadMode = txVector.GetMode ();
  NS_LOG_FUNCTION (size << payloadMode);

  double stbc = 1;
  if (txVector.IsStbc ()
      && (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_HT
          || payloadMode.GetModulationClass () == WIFI_MOD_CLASS_VHT))
    {
      stbc = 2;
    }

  double Nes = 1;
  //todo: improve logic to reduce the number of if cases
  //todo: extend to NSS > 4 for VHT rates
  if (payloadMode == GetHtMcs21 ()
      || payloadMode == GetHtMcs22 ()
      || payloadMode == GetHtMcs23 ()
      || payloadMode == GetHtMcs28 ()
      || payloadMode == GetHtMcs29 ()
      || payloadMode == GetHtMcs30 ()
      || payloadMode == GetHtMcs31 ())
    {
      Nes = 2;
    }
  if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      if (txVector.GetChannelWidth () == 40
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () >= 8)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 80
          && txVector.GetNss () == 2
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 80
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 80
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () == 9)
        {
          Nes = 3;
        }
      if (txVector.GetChannelWidth () == 80
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 4)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 80
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 3;
        }
      if (txVector.GetChannelWidth () == 160
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 2
          && payloadMode.GetMcsValue () >= 4)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 2
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 3;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () >= 3)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () >= 5)
        {
          Nes = 3;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 4;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 2)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 4)
        {
          Nes = 3;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 5)
        {
          Nes = 4;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 6;
        }
    }

  Time symbolDuration = Seconds (0);
  switch (payloadMode.GetModulationClass ())
    {
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_ERP_OFDM:
      {
        //(Section 18.3.2.4 "Timing related parameters" Table 18-5 "Timing-related parameters"; IEEE Std 802.11-2012
        //corresponds to T_{SYM} in the table)
        switch (txVector.GetChannelWidth ())
          {
          case 20:
          default:
            symbolDuration = MicroSeconds (4);
            break;
          case 10:
            symbolDuration = MicroSeconds (8);
            break;
          case 5:
            symbolDuration = MicroSeconds (16);
            break;
          }
        break;
      }
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
      {
        //if short GI data rate is used then symbol duration is 3.6us else symbol duration is 4us
        //In the future has to create a stationmanager that only uses these data rates if sender and receiver support GI
        uint16_t gi = txVector.GetGuardInterval ();
        NS_ASSERT (gi == 400 || gi == 800);
        symbolDuration = NanoSeconds (3200 + gi);
      }
      break;
    case WIFI_MOD_CLASS_HE:
      {
        //if short GI data rate is used then symbol duration is 3.6us else symbol duration is 4us
        //In the future has to create a stationmanager that only uses these data rates if sender and receiver support GI
        uint16_t gi = txVector.GetGuardInterval ();
        NS_ASSERT (gi == 800 || gi == 1600 || gi == 3200);
        symbolDuration = NanoSeconds (12800 + gi);
      }
      break;
    default:
      break;
    }

  double numDataBitsPerSymbol = payloadMode.GetDataRate (txVector) * symbolDuration.GetNanoSeconds () / 1e9;

  double numSymbols = 0;
  if (mpdutype == FIRST_MPDU_IN_AGGREGATE)
    {
      //First packet in an A-MPDU
      numSymbols = (stbc * (16 + size * 8.0 + 6 * Nes) / (stbc * numDataBitsPerSymbol));
      if (incFlag == 1)
        {
          m_totalAmpduSize += size;
          m_totalAmpduNumSymbols += numSymbols;
        }
    }
  else if (mpdutype == MIDDLE_MPDU_IN_AGGREGATE)
    {
      //consecutive packets in an A-MPDU
      numSymbols = (stbc * size * 8.0) / (stbc * numDataBitsPerSymbol);
      if (incFlag == 1)
        {
          m_totalAmpduSize += size;
          m_totalAmpduNumSymbols += numSymbols;
        }
    }
  else if (mpdutype == LAST_MPDU_IN_AGGREGATE)
    {
      //last packet in an A-MPDU
      uint32_t totalAmpduSize = m_totalAmpduSize + size;
      numSymbols = lrint (stbc * ceil ((16 + totalAmpduSize * 8.0 + 6 * Nes) / (stbc * numDataBitsPerSymbol)));
      NS_ASSERT (m_totalAmpduNumSymbols <= numSymbols);
      numSymbols -= m_totalAmpduNumSymbols;
      if (incFlag == 1)
        {
          m_totalAmpduSize = 0;
          m_totalAmpduNumSymbols = 0;
        }
    }
  else if (mpdutype == NORMAL_MPDU || mpdutype == SINGLE_MPDU)
    {
      //Not an A-MPDU or single MPDU (i.e. the current payload contains both service and padding)
      //The number of OFDM symbols in the data field when BCC encoding
      //is used is given in equation 19-32 of the IEEE 802.11-2016 standard.
      numSymbols = lrint (stbc * ceil ((16 + size * 8.0 + 6.0 * Nes) / (stbc * numDataBitsPerSymbol)));
    }
  else
    {
      NS_FATAL_ERROR ("Unknown MPDU type");
    }

  switch (payloadMode.GetModulationClass ())
    {
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_ERP_OFDM:
      {
        //Add signal extension for ERP PHY
        if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ())) + MicroSeconds (6);
          }
        else
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ()));
          }
      }
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
      {
        if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_HT && Is2_4Ghz (frequency)
            && (mpdutype == NORMAL_MPDU || mpdutype == SINGLE_MPDU || mpdutype == LAST_MPDU_IN_AGGREGATE)) //at 2.4 GHz
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ())) + MicroSeconds (6);
          }
        else //at 5 GHz
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ()));
          }
      }
    case WIFI_MOD_CLASS_HE:
      {
        if (Is2_4Ghz (frequency)
            && ((mpdutype == NORMAL_MPDU || mpdutype == SINGLE_MPDU || mpdutype == LAST_MPDU_IN_AGGREGATE))) //at 2.4 GHz
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ())) + MicroSeconds (6);
          }
        else //at 5 GHz
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ()));
          }
      }
    case WIFI_MOD_CLASS_DSSS:
    case WIFI_MOD_CLASS_HR_DSSS:
      return MicroSeconds (lrint (ceil ((size * 8.0) / (payloadMode.GetDataRate (22) / 1.0e6))));
    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return MicroSeconds (0);
    }
}

Time
WifiPhy::CalculatePlcpPreambleAndHeaderDuration (WifiTxVector txVector)
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  Time duration = GetPlcpPreambleDuration (txVector)
    + GetPlcpHeaderDuration (txVector)
    + GetPlcpHtSigHeaderDuration (preamble)
    + GetPlcpSigA1Duration (preamble)
    + GetPlcpSigA2Duration (preamble)
    + GetPlcpTrainingSymbolDuration (txVector)
    + GetPlcpSigBDuration (preamble);
  return duration;
}

Time
WifiPhy::CalculateTxDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency,
                              MpduType mpdutype, uint8_t incFlag)
{
  Time duration = CalculatePlcpPreambleAndHeaderDuration (txVector)
    + GetPayloadDuration (size, txVector, frequency, mpdutype, incFlag);
  return duration;
}

Time
WifiPhy::CalculateTxDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency)
{
  return CalculateTxDuration (size, txVector, frequency, NORMAL_MPDU, 0);
}

void
WifiPhy::NotifyTxBegin (Ptr<const Packet> packet, double txPowerW)
{
  if (IsAmpdu (packet))
    {
      std::list<Ptr<const Packet>> mpdus = MpduAggregator::PeekMpdus (packet);
      for (auto & mpdu : mpdus)
        {
          m_phyTxBeginTrace (mpdu, txPowerW);
        }
    }
  else
    {
      m_phyTxBeginTrace (packet, txPowerW);
    }
}

void
WifiPhy::NotifyTxEnd (Ptr<const Packet> packet)
{
  if (IsAmpdu (packet))
    {
      std::list<Ptr<const Packet>> mpdus = MpduAggregator::PeekMpdus (packet);
      for (auto & mpdu : mpdus)
        {
          m_phyTxEndTrace (mpdu);
        }
    }
  else
    {
      m_phyTxEndTrace (packet);
    }
}

void
WifiPhy::NotifyTxDrop (Ptr<const Packet> packet)
{
  if (IsAmpdu (packet))
    {
      std::list<Ptr<const Packet>> mpdus = MpduAggregator::PeekMpdus (packet);
      for (auto & mpdu : mpdus)
        {
          m_phyTxDropTrace (mpdu);
        }
    }
  else
    {
      m_phyTxDropTrace (packet);
    }
}

void
WifiPhy::NotifyRxBegin (Ptr<const Packet> packet)
{
  if (IsAmpdu (packet))
    {
      std::list<Ptr<const Packet>> mpdus = MpduAggregator::PeekMpdus (packet);
      for (auto & mpdu : mpdus)
        {
          m_phyRxBeginTrace (mpdu);
        }
    }
  else
    {
      m_phyRxBeginTrace (packet);
    }
}

void
WifiPhy::NotifyRxEnd (Ptr<const Packet> packet)
{
  if (IsAmpdu (packet))
    {
      std::list<Ptr<const Packet>> mpdus = MpduAggregator::PeekMpdus (packet);
      for (auto & mpdu : mpdus)
        {
          m_phyRxEndTrace (mpdu);
        }
    }
  else
    {
      m_phyRxEndTrace (packet);
    }
}

void
WifiPhy::NotifyRxDrop (Ptr<const Packet> packet, WifiPhyRxfailureReason reason)
{
  if (IsAmpdu (packet))
    {
      std::list<Ptr<const Packet>> mpdus = MpduAggregator::PeekMpdus (packet);
      for (auto & mpdu : mpdus)
        {
          m_phyRxDropTrace (mpdu, reason);
        }
    }
  else
    {
      m_phyRxDropTrace (packet, reason);
    }
}

void
WifiPhy::NotifyMonitorSniffRx (Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector,
                               SignalNoiseDbm signalNoise, std::vector<bool> statusPerMpdu)
{
  MpduInfo aMpdu;
  if (txVector.IsAggregation ())
    {
      //Expand A-MPDU
      aMpdu.mpduRefNumber = ++m_rxMpduReferenceNumber;
      std::list<Ptr<const Packet>> ampduSubframes = MpduAggregator::PeekAmpduSubframes (packet);
      size_t numberOfMpdus = ampduSubframes.size ();
      NS_ABORT_MSG_IF (statusPerMpdu.size () != numberOfMpdus, "Should have one reception status per MPDU");
      size_t i = 0;
      aMpdu.type = (numberOfMpdus == 1) ? SINGLE_MPDU: FIRST_MPDU_IN_AGGREGATE;
      for (const auto & subframe : ampduSubframes)
        {
          if (statusPerMpdu.at (i)) //packet received without error, hand over to sniffer
            {
              m_phyMonitorSniffRxTrace (subframe, channelFreqMhz, txVector, aMpdu, signalNoise);
            }
          ++i;
          aMpdu.type = (i == (numberOfMpdus - 1)) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
        }
    }
  else
    {
      aMpdu.type = NORMAL_MPDU;
      NS_ABORT_MSG_IF (statusPerMpdu.size () != 1, "Should have one reception status for normal MPDU");
      m_phyMonitorSniffRxTrace (packet, channelFreqMhz, txVector, aMpdu, signalNoise);
    }
}

void
WifiPhy::NotifyMonitorSniffTx (Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector)
{
  MpduInfo aMpdu;
  if (txVector.IsAggregation ())
    {
      //Expand A-MPDU
      aMpdu.mpduRefNumber = ++m_txMpduReferenceNumber;
      std::list<Ptr<const Packet>> ampduSubframes = MpduAggregator::PeekAmpduSubframes (packet);
      size_t numberOfMpdus = ampduSubframes.size ();
      size_t i = 0;
      aMpdu.type = (numberOfMpdus == 1) ? SINGLE_MPDU: FIRST_MPDU_IN_AGGREGATE;
      for (const auto & subframe : ampduSubframes)
        {
          m_phyMonitorSniffTxTrace (subframe, channelFreqMhz, txVector, aMpdu);
          ++i;
          aMpdu.type = (i == (numberOfMpdus - 1)) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
        }
    }
  else
    {
      aMpdu.type = NORMAL_MPDU;
      m_phyMonitorSniffTxTrace (packet, channelFreqMhz, txVector, aMpdu);
    }
}

void
WifiPhy::NotifyEndOfHePreamble (HePreambleParameters params)
{
  m_phyEndOfHePreambleTrace (params);
}

void
WifiPhy::SendPacket (Ptr<const Packet> packet, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << packet << txVector);
  /* Transmission can happen if:
   *  - we are syncing on a packet. It is the responsibility of the
   *    MAC layer to avoid doing this but the PHY does nothing to
   *    prevent it.
   *  - we are idle
   */
  NS_ASSERT (!m_state->IsStateTx () && !m_state->IsStateSwitching ());

  if (txVector.GetNss () > GetMaxSupportedTxSpatialStreams ())
    {
      NS_FATAL_ERROR ("Unsupported number of spatial streams!");
    }

  if (m_state->IsStateSleep ())
    {
      NS_LOG_DEBUG ("Dropping packet because in sleep mode");
      NotifyTxDrop (packet);
      return;
    }

  Time txDuration = CalculateTxDuration (packet->GetSize (), txVector, GetFrequency ());
  NS_ASSERT (txDuration.IsStrictlyPositive ());

  if (m_endPreambleDetectionEvent.IsRunning ())
    {
      m_endPreambleDetectionEvent.Cancel ();
    }
  if (m_endPlcpRxEvent.IsRunning ())
    {
      m_endPlcpRxEvent.Cancel ();
    }
  if (m_endRxEvent.IsRunning ())
    {
      m_endRxEvent.Cancel ();
    }
  if (m_state->IsStateRx ())
    {
      m_interference.NotifyRxEnd ();
    }

  if (m_powerRestricted)
    {
      NS_LOG_DEBUG ("Transmitting with power restriction");
    }
  else
    {
      NS_LOG_DEBUG ("Transmitting without power restriction");
    }

  NotifyTxBegin (packet, DbmToW (GetTxPowerForTransmission (txVector) + GetTxGain ()));
  NotifyMonitorSniffTx (packet, GetFrequency (), txVector);
  m_state->SwitchToTx (txDuration, packet, GetPowerDbm (txVector.GetTxPowerLevel ()), txVector);

  Ptr<Packet> newPacket = packet->Copy (); // obtain non-const Packet
  WifiPhyTag oldtag;
  newPacket->RemovePacketTag (oldtag);
  if (m_state->GetState () == WifiPhyState::OFF)
    {
      NS_LOG_DEBUG ("Transmission canceled because device is OFF");
      return;
    }

  if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HT)
    {
      HtSigHeader htSig;
      htSig.SetMcs (txVector.GetMode ().GetMcsValue ());
      htSig.SetChannelWidth (txVector.GetChannelWidth ());
      htSig.SetHtLength (packet->GetSize ());
      htSig.SetAggregation (txVector.IsAggregation ());
      htSig.SetShortGuardInterval (txVector.GetGuardInterval () == 400);
      newPacket->AddHeader (htSig);
    }
  else if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      VhtSigHeader vhtSig;
      vhtSig.SetMuFlag (txVector.GetPreambleType () == WIFI_PREAMBLE_VHT_MU);
      vhtSig.SetChannelWidth (txVector.GetChannelWidth ());
      vhtSig.SetShortGuardInterval (txVector.GetGuardInterval () == 400);
      uint32_t nSymbols = (static_cast<double> ((txDuration - CalculatePlcpPreambleAndHeaderDuration (txVector)).GetNanoSeconds ()) / (3200 + txVector.GetGuardInterval ()));
      vhtSig.SetShortGuardIntervalDisambiguation ((nSymbols % 10) == 9);
      vhtSig.SetSuMcs (txVector.GetMode ().GetMcsValue ());
      vhtSig.SetNStreams (txVector.GetNss ());
      newPacket->AddHeader (vhtSig);
    }
  else if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      HeSigHeader heSig;
      heSig.SetMuFlag (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_MU);
      heSig.SetMcs (txVector.GetMode ().GetMcsValue ());
      heSig.SetBssColor (txVector.GetBssColor ());
      heSig.SetChannelWidth (txVector.GetChannelWidth ());
      heSig.SetGuardIntervalAndLtfSize (txVector.GetGuardInterval (), 2/*NLTF currently unused*/);
      heSig.SetNStreams (txVector.GetNss ());
      newPacket->AddHeader (heSig);
    }
  if ((txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_DSSS) || (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS))
    {
      DsssSigHeader sig;
      sig.SetRate (txVector.GetMode ().GetDataRate (22));
      Time psduDuration = txDuration - CalculatePlcpPreambleAndHeaderDuration (txVector);
      sig.SetLength (psduDuration.GetMicroSeconds ());
      newPacket->AddHeader (sig);
    }
  else if ((txVector.GetMode ().GetModulationClass () != WIFI_MOD_CLASS_HT) || (txVector.GetPreambleType () != WIFI_PREAMBLE_HT_GF))
    {
      LSigHeader sig;
      if ((txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_OFDM) || (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM))
        {
          sig.SetRate (txVector.GetMode ().GetDataRate (txVector), GetChannelWidth ());
          sig.SetLength (packet->GetSize ());
        }
      else //HT, VHT or HE
      {
          uint8_t sigExtention = 0;
          if (Is2_4Ghz (GetFrequency ()))
            {
              sigExtention = 6;
            }
          uint8_t m = 0;
          if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_SU)
            {
              m = 2;
            }
          else if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_MU)
            {
              m = 1;
            }
          //Equation 27-11 of IEEE P802.11/D4.0
          uint16_t length = ((ceil ((static_cast<double> (txDuration.GetNanoSeconds () - (20 * 1000) - (sigExtention * 1000)) / 1000) / 4.0) * 3) - 3 - m);
          sig.SetLength (length);
      }
      newPacket->AddHeader (sig);
    }

  uint8_t isFrameComplete = 1;
  if (m_wifiRadioEnergyModel != 0 && m_wifiRadioEnergyModel->GetMaximumTimeInState (WifiPhyState::TX) < txDuration)
    {
      isFrameComplete = 0;
    }
  WifiPhyTag tag (txVector.GetPreambleType (), txVector.GetMode ().GetModulationClass (), isFrameComplete);
  newPacket->AddPacketTag (tag);

  StartTx (newPacket, txVector, txDuration);

  m_channelAccessRequested = false;
  m_powerRestricted = false;
}

void
WifiPhy::StartReceiveHeader (Ptr<Event> event, Time rxDuration)
{
  NS_LOG_FUNCTION (this << event->GetPacket () << event->GetTxVector () << event << rxDuration);
  NS_ASSERT (!IsStateRx ());
  NS_ASSERT (m_endPlcpRxEvent.IsExpired ());

  InterferenceHelper::SnrPer snrPer = m_interference.CalculateLegacyPhyHeaderSnrPer (event);
  double snr = snrPer.snr;
  NS_LOG_DEBUG ("snr(dB)=" << RatioToDb (snrPer.snr) << ", per=" << snrPer.per);

  if (!m_preambleDetectionModel || (m_preambleDetectionModel->IsPreambleDetected (event->GetRxPowerW (), snr, m_channelWidth)))
    {
      m_state->SwitchToRx (rxDuration);
      NotifyRxBegin (event->GetPacket ());

      m_timeLastPreambleDetected = Simulator::Now ();

      WifiTxVector txVector = event->GetTxVector ();

      if (txVector.GetNss () > GetMaxSupportedRxSpatialStreams ())
        {
          NS_LOG_DEBUG ("Packet reception could not be started because not enough RX antennas");
          NotifyRxDrop (event->GetPacket (), UNSUPPORTED_SETTINGS);
          MaybeCcaBusyDuration ();
          return;
        }

      if ((txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HT) && (txVector.GetPreambleType () == WIFI_PREAMBLE_HT_GF))
        {
          //No legacy PHY header for HT GF
          Time remainingPreambleHeaderDuration = CalculatePlcpPreambleAndHeaderDuration (txVector) - GetPreambleDetectionDuration ();
          m_endPlcpRxEvent = Simulator::Schedule (remainingPreambleHeaderDuration, &WifiPhy::StartReceivePayload, this, event);
        }
      else
        {
          //Schedule end of legacy PHY header
          Time remainingPreambleAndLegacyHeaderDuration = GetPlcpPreambleDuration (txVector) + GetPlcpHeaderDuration (txVector) - GetPreambleDetectionDuration ();
          m_endPlcpRxEvent = Simulator::Schedule (remainingPreambleAndLegacyHeaderDuration, &WifiPhy::ContinueReceiveHeader, this, event);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop packet because PHY preamble detection failed");
      NotifyRxDrop (event->GetPacket (), PREAMBLE_DETECT_FAILURE);
      m_interference.NotifyRxEnd ();
    }
  // Like CCA-SD, CCA-ED is governed by the 4μs CCA window to flag CCA-BUSY
  // for any received signal greater than the CCA-ED threshold.
  MaybeCcaBusyDuration ();
}

void
WifiPhy::ContinueReceiveHeader (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << event->GetPacket () << event->GetTxVector () << event);
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (m_endPlcpRxEvent.IsExpired ());

  InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculateLegacyPhyHeaderSnrPer (event);

  if (m_random->GetValue () > snrPer.per) //legacy PHY header reception succeeded
    {
      NS_LOG_DEBUG ("Received legacy PHY header");
      WifiTxVector txVector = event->GetTxVector ();
      Time remainingPreambleHeaderDuration = CalculatePlcpPreambleAndHeaderDuration (txVector) - GetPlcpPreambleDuration (txVector) - GetPlcpHeaderDuration (txVector);
      m_endPlcpRxEvent = Simulator::Schedule (remainingPreambleHeaderDuration, &WifiPhy::StartReceivePayload, this, event);
    }
  else //legacy PHY header reception failed
    {
      NS_LOG_DEBUG ("Abort reception because legacy PHY header reception failed");
      AbortCurrentReception (L_SIG_FAILURE);
    }
}

void
WifiPhy::StartReceivePreamble (Ptr<Packet> packet, double rxPowerW, Time rxDuration)
{
  NS_LOG_FUNCTION (this << packet << rxPowerW << rxDuration);
  WifiPhyTag tag;
  bool found = packet->RemovePacketTag (tag);
  if (!found)
    {
      NS_FATAL_ERROR ("Received Wi-Fi Signal with no WifiPhyTag");
      return;
    }

  WifiPreamble preamble = tag.GetPreambleType ();
  WifiModulationClass modulation = tag.GetModulation ();
  WifiTxVector txVector;
  txVector.SetPreambleType (preamble);
  if ((modulation == WIFI_MOD_CLASS_DSSS) || (modulation == WIFI_MOD_CLASS_HR_DSSS))
    {
      DsssSigHeader dsssSigHdr;
      found = packet->RemoveHeader (dsssSigHdr);
      if (!found)
        {
          NS_FATAL_ERROR ("Received 802.11b signal with no SIG field");
          return;
        }
      txVector.SetChannelWidth (22);
      for (uint8_t i = 0; i < GetNModes (); i++)
        {
          WifiMode mode = GetMode (i);
          if (((mode.GetModulationClass() == WIFI_MOD_CLASS_DSSS) || (mode.GetModulationClass() == WIFI_MOD_CLASS_HR_DSSS))
              && (mode.GetDataRate (22) == dsssSigHdr.GetRate ()))
            {
              txVector.SetMode (mode);
              break;
            }
        }
    }
  else if ((modulation != WIFI_MOD_CLASS_HT) || (preamble != WIFI_PREAMBLE_HT_GF))
    {
      LSigHeader lSigHdr;
      found = packet->RemoveHeader (lSigHdr);
      if (!found)
        {
          NS_FATAL_ERROR ("Received OFDM 802.11 signal with no SIG field");
          return;
        }
      uint16_t channelWidth = GetChannelWidth ();
      txVector.SetChannelWidth (channelWidth > 20 ? 20 : channelWidth);
      for (uint8_t i = 0; i < GetNModes (); i++)
        {
          WifiMode mode = GetMode (i);
          if (mode.GetDataRate (GetChannelWidth ()) == lSigHdr.GetRate (GetChannelWidth ()))
            {
              txVector.SetMode (mode);
              break;
            }
        }
    }
  if (modulation == WIFI_MOD_CLASS_HT)
    {
      HtSigHeader htSigHdr;
      found = packet->RemoveHeader (htSigHdr);
      if (!found)
        {
          NS_FATAL_ERROR ("Received 802.11n signal with no HT-SIG field");
          return;
        }
      txVector.SetChannelWidth (htSigHdr.GetChannelWidth ());
      for (uint8_t i = 0; i < GetNMcs (); i++)
        {
          WifiMode mode = GetMcs (i);
          if ((mode.GetModulationClass () == WIFI_MOD_CLASS_HT) && (mode.GetMcsValue () == htSigHdr.GetMcs ()))
            {
              txVector.SetMode (mode);
              txVector.SetNss (1 + (txVector.GetMode ().GetMcsValue () / 8));
              break;
            }
        }
      txVector.SetGuardInterval(htSigHdr.GetShortGuardInterval () ? 400 : 800);
      txVector.SetAggregation (htSigHdr.GetAggregation ());
    }
  else if (modulation == WIFI_MOD_CLASS_VHT)
    {
      VhtSigHeader vhtSigHdr;
      vhtSigHdr.SetMuFlag (preamble == WIFI_PREAMBLE_VHT_MU);
      found = packet->RemoveHeader (vhtSigHdr);
      if (!found)
        {
          NS_FATAL_ERROR ("Received 802.11ac signal with no VHT-SIG field");
          return;
        }
      txVector.SetChannelWidth (vhtSigHdr.GetChannelWidth ());
      txVector.SetNss (vhtSigHdr.GetNStreams ());
      for (uint8_t i = 0; i < GetNMcs (); i++)
        {
          WifiMode mode = GetMcs (i);
          if ((mode.GetModulationClass () == WIFI_MOD_CLASS_VHT) && (mode.GetMcsValue () == vhtSigHdr.GetSuMcs ()))
            {
              txVector.SetMode (mode);
              break;
            }
        }
      txVector.SetGuardInterval (vhtSigHdr.GetShortGuardInterval () ? 400 : 800);
      if (IsAmpdu (packet))
        {
          txVector.SetAggregation (true);
        }
    }
  else if (modulation == WIFI_MOD_CLASS_HE)
    {
      HeSigHeader heSigHdr;
      heSigHdr.SetMuFlag (preamble == WIFI_PREAMBLE_HE_MU);
      found = packet->RemoveHeader (heSigHdr);
      if (!found)
        {
          NS_FATAL_ERROR ("Received 802.11ax signal with no HE-SIG field");
          return;
        }
      txVector.SetChannelWidth (heSigHdr.GetChannelWidth ());
      txVector.SetNss (heSigHdr.GetNStreams ());
      for (uint8_t i = 0; i < GetNMcs (); i++)
        {
          WifiMode mode = GetMcs (i);
          if ((mode.GetModulationClass () == WIFI_MOD_CLASS_HE) && (mode.GetMcsValue () == heSigHdr.GetMcs ()))
            {
              txVector.SetMode (mode);
              break;
            }
        }
      txVector.SetGuardInterval (heSigHdr.GetGuardInterval ());
      txVector.SetBssColor (heSigHdr.GetBssColor ());
      if (IsAmpdu (packet))
        {
          txVector.SetAggregation (true);
        }
    }

  Ptr<Event> event;
  event = m_interference.Add (packet,
                              txVector,
                              rxDuration,
                              rxPowerW);

  if (m_state->GetState () == WifiPhyState::OFF)
    {
      NS_LOG_DEBUG ("Cannot start RX because device is OFF");
      return;
    }

  if (tag.GetFrameComplete () == 0)
    {
      NS_LOG_DEBUG ("Packet reception stopped because transmitter has been switched off");
      return;
    }

  if (!txVector.GetModeInitialized ())
    {
      //If SetRate method was not called above when filling in txVector, this means the PHY does support the rate indicated in PHY SIG headers
      NS_LOG_DEBUG ("drop packet because of unsupported RX mode");
      NotifyRxDrop (packet, UNSUPPORTED_SETTINGS);
      return;
    }

  Time endRx = Simulator::Now () + rxDuration;
  switch (m_state->GetState ())
    {
    case WifiPhyState::SWITCHING:
      NS_LOG_DEBUG ("drop packet because of channel switching");
      NotifyRxDrop (packet, NOT_ALLOWED);
      /*
       * Packets received on the upcoming channel are added to the event list
       * during the switching state. This way the medium can be correctly sensed
       * when the device listens to the channel for the first time after the
       * switching e.g. after channel switching, the channel may be sensed as
       * busy due to other devices' tramissions started before the end of
       * the switching.
       */
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the completion of the channel switching.
          MaybeCcaBusyDuration ();
          return;
        }
      break;
    case WifiPhyState::RX:
      NS_ASSERT (m_currentEvent != 0);
      if (m_frameCaptureModel != 0
          && m_frameCaptureModel->IsInCaptureWindow (m_timeLastPreambleDetected)
          && m_frameCaptureModel->CaptureNewFrame (m_currentEvent, event))
        {
          AbortCurrentReception (FRAME_CAPTURE_PACKET_SWITCH);
          NS_LOG_DEBUG ("Switch to new packet");
          StartRx (event, rxPowerW, rxDuration);
        }
      else
        {
          NS_LOG_DEBUG ("Drop packet because already in Rx (power=" <<
                        rxPowerW << "W)");
          NotifyRxDrop (packet, NOT_ALLOWED);
          if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
            {
              //that packet will be noise _after_ the reception of the currently-received packet.
              MaybeCcaBusyDuration ();
              return;
            }
        }
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("Drop packet because already in Tx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (packet, NOT_ALLOWED);
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the transmission of the currently-transmitted packet.
          MaybeCcaBusyDuration ();
          return;
        }
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      StartRx (event, rxPowerW, rxDuration);
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("Drop packet because in sleep mode");
      NotifyRxDrop (packet, NOT_ALLOWED);
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
}

void
WifiPhy::MaybeCcaBusyDuration ()
{
  //We are here because we have received the first bit of a packet and we are
  //not going to be able to synchronize on it
  //In this model, CCA becomes busy when the aggregation of all signals as
  //tracked by the InterferenceHelper class is higher than the CcaBusyThreshold

  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW);
  if (!delayUntilCcaEnd.IsZero ())
    {
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
WifiPhy::StartReceivePayload (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << event->GetPacket () << event->GetTxVector ());
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
  NS_ASSERT (m_endRxEvent.IsExpired ());
  WifiTxVector txVector = event->GetTxVector ();
  WifiMode txMode = txVector.GetMode ();
  bool canReceivePayload;
  if ((txMode.GetModulationClass () == WIFI_MOD_CLASS_HT) || (txMode.GetModulationClass () == WIFI_MOD_CLASS_VHT) || (txMode.GetModulationClass () == WIFI_MOD_CLASS_HE))
    {
      InterferenceHelper::SnrPer snrPer;
      snrPer = m_interference.CalculateNonLegacyPhyHeaderSnrPer (event);
      canReceivePayload = (m_random->GetValue () > snrPer.per);
    }
  else
    {
      //If we are here, this means legacy PHY header was already successfully received
      canReceivePayload = true;
    }
  if (canReceivePayload) //plcp reception succeeded
    {
      if (IsModeSupported (txMode) || IsMcsSupported (txMode))
        {
          Time payloadDuration = event->GetEndTime () - event->GetStartTime () - CalculatePlcpPreambleAndHeaderDuration (txVector);
          m_endRxEvent = Simulator::Schedule (payloadDuration, &WifiPhy::EndReceive, this, event);
          NS_LOG_DEBUG ("Receiving payload");
          if (txMode.GetModulationClass () == WIFI_MOD_CLASS_HE)
            {
              HePreambleParameters params;
              params.rssiW = event->GetRxPowerW ();
              params.bssColor = event->GetTxVector ().GetBssColor ();
              NotifyEndOfHePreamble (params);
            }
        }
      else //mode is not allowed
        {
          NS_LOG_DEBUG ("Drop packet because it was sent using an unsupported mode (" << txMode << ")");
          NotifyRxDrop (event->GetPacket (), UNSUPPORTED_SETTINGS);
        }
    }
  else //plcp reception failed
    {
      NS_LOG_DEBUG ("Drop packet because non-legacy PHY header reception failed");
      NotifyRxDrop (event->GetPacket (), SIG_A_FAILURE);
    }
}

void
WifiPhy::EndReceive (Ptr<Event> event)
{
  Time psduDuration = event->GetEndTime () - event->GetStartTime ();
  NS_LOG_FUNCTION (this << event->GetPacket () << event->GetTxVector () << event << psduDuration);
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  double snr = m_interference.CalculateSnr (event);
  std::vector<bool> statusPerMpdu;
  SignalNoiseDbm signalNoise;

  Ptr<const Packet> packet = event->GetPacket ();
  Time relativeStart = NanoSeconds (0);
  bool receptionOkAtLeastForOneMpdu = false;
  std::pair<bool, SignalNoiseDbm> rxInfo;
  WifiTxVector txVector = event->GetTxVector ();
  if (txVector.IsAggregation ())
    {
      //Extract all MPDUs of the A-MPDU to compute per-MPDU PER stats
      //TODO remove PLCP header first
      std::list<Ptr<const Packet>> ampduSubframes = MpduAggregator::PeekAmpduSubframes (packet);
      size_t nbOfRemainingMpdus = ampduSubframes.size ();
      Time remainingAmpduDuration = event->GetEndTime () - event->GetStartTime ();
      MpduType mpdutype = (nbOfRemainingMpdus == 1) ? SINGLE_MPDU : FIRST_MPDU_IN_AGGREGATE;
      for (const auto & subframe : ampduSubframes)
        {
          Time mpduDuration = GetPayloadDuration (subframe->GetSize (), txVector, GetFrequency (), mpdutype, 1);
          remainingAmpduDuration -= mpduDuration;
          --nbOfRemainingMpdus;
          if (nbOfRemainingMpdus == 0 && !remainingAmpduDuration.IsZero ()) //no more MPDU coming
            {
              mpduDuration += remainingAmpduDuration; //apply a correction just in case rounding had induced slight shift
            }
          rxInfo = GetReceptionStatus (MpduAggregator::PeekMpduInAmpduSubframe (subframe), event, relativeStart, mpduDuration);
          NS_LOG_DEBUG ("Extracted MPDU #" << ampduSubframes.size () - nbOfRemainingMpdus - 1 << ": duration: " << mpduDuration.GetNanoSeconds () << "ns" <<
                        ", correct reception: " << rxInfo.first <<
                        ", Signal/Noise: " << rxInfo.second.signal << "/" << rxInfo.second.noise << "dBm");
          signalNoise = rxInfo.second; //same information for all MPDUs
          statusPerMpdu.push_back (rxInfo.first);
          receptionOkAtLeastForOneMpdu |= rxInfo.first;
          relativeStart += mpduDuration;
          mpdutype = (nbOfRemainingMpdus == 1) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
        }
    }
  else
    {
      //Simple MPDU
      rxInfo = GetReceptionStatus (packet, event, relativeStart, psduDuration);
      signalNoise = rxInfo.second; //same information for all MPDUs
      statusPerMpdu.push_back (rxInfo.first);
      receptionOkAtLeastForOneMpdu = rxInfo.first;
    }

  if (receptionOkAtLeastForOneMpdu)
    {
      NotifyMonitorSniffRx (packet, GetFrequency (), txVector, signalNoise, statusPerMpdu);
      m_state->SwitchFromRxEndOk (packet->Copy (), snr, txVector, statusPerMpdu);
    }
  else
    {
      m_state->SwitchFromRxEndError (packet->Copy (), snr);
    }

  m_interference.NotifyRxEnd ();
  m_currentEvent = 0;
}

std::pair<bool, SignalNoiseDbm>
WifiPhy::GetReceptionStatus (Ptr<const Packet> mpdu, Ptr<Event> event, Time relativeMpduStart, Time mpduDuration)
{
  NS_LOG_FUNCTION (this << mpdu << event->GetTxVector () << event << relativeMpduStart << mpduDuration);
  InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculatePayloadSnrPer (event, std::make_pair (relativeMpduStart, relativeMpduStart + mpduDuration));

  NS_LOG_DEBUG ("mode=" << (event->GetTxVector ().GetMode ().GetDataRate (event->GetTxVector ())) <<
                ", snr(dB)=" << RatioToDb (snrPer.snr) << ", per=" << snrPer.per << ", size=" << mpdu->GetSize () <<
                ", relativeStart = " << relativeMpduStart.GetNanoSeconds () << "ns, duration = " << mpduDuration.GetNanoSeconds () << "ns");

  // There are two error checks: PER and receive error model check.
  // PER check models is typical for Wi-Fi and is based on signal modulation;
  // Receive error model is optional, if we have an error model and
  // it indicates that the packet is corrupt, drop the packet.
  SignalNoiseDbm signalNoise;
  signalNoise.signal = WToDbm (event->GetRxPowerW ());
  signalNoise.noise = WToDbm (event->GetRxPowerW () / snrPer.snr);
  if (m_random->GetValue () > snrPer.per &&
      !(m_postReceptionErrorModel && m_postReceptionErrorModel->IsCorrupt (mpdu->Copy ())))
    {
      NS_LOG_DEBUG ("Reception succeeded: " << mpdu->ToString ());
      NotifyRxEnd (mpdu);
      return std::make_pair (true, signalNoise);
    }
  else
    {
      NS_LOG_DEBUG ("Reception failed: " << mpdu->ToString ());
      NotifyRxDrop (mpdu, ERRONEOUS_FRAME);
      return std::make_pair (false, signalNoise);
    }
}

void
WifiPhy::EndReceiveInterBss (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_channelAccessRequested)
    {
      m_powerRestricted = false;
    }
}

void
WifiPhy::NotifyChannelAccessRequested (void)
{
  NS_LOG_FUNCTION (this);
  m_channelAccessRequested = true;
}

// Clause 15 rates (DSSS)

WifiMode
WifiPhy::GetDsssRate1Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate1Mbps",
                                     WIFI_MOD_CLASS_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetDsssRate2Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate2Mbps",
                                     WIFI_MOD_CLASS_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     4);
  return mode;
}


// Clause 18 rates (HR/DSSS)

WifiMode
WifiPhy::GetDsssRate5_5Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate5_5Mbps",
                                     WIFI_MOD_CLASS_HR_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetDsssRate11Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate11Mbps",
                                     WIFI_MOD_CLASS_HR_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     256);
  return mode;
}


// Clause 19.5 rates (ERP-OFDM)

WifiMode
WifiPhy::GetErpOfdmRate6Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate6Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate9Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate9Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate12Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate12Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate18Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate18Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate24Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate24Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate36Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate36Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate48Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate48Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate54Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate54Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


// Clause 17 rates (OFDM)

WifiMode
WifiPhy::GetOfdmRate6Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate6Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate9Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate9Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate12Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate12Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate18Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate18Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate24Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate24Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate36Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate36Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate48Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate48Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate54Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate54Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


// 10 MHz channel rates

WifiMode
WifiPhy::GetOfdmRate3MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate3MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate4_5MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate4_5MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate6MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate6MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate9MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate9MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate12MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate12MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate18MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate18MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate24MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate24MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate27MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate27MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


// 5 MHz channel rates

WifiMode
WifiPhy::GetOfdmRate1_5MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate1_5MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate2_25MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate2_25MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate3MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate3MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate4_5MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate4_5MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate6MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate6MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate9MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate9MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate12MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate12MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate13_5MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate13_5MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


// Clause 20

WifiMode
WifiPhy::GetHtMcs0 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs0", 0, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs1 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs1", 1, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs2 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs2", 2, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs3 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs3", 3, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs4 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs4", 4, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs5 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs5", 5, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs6 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs6", 6, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs7 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs7", 7, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs8 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs8", 8, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs9 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs9", 9, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs10 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs10", 10, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs11 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs11", 11, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs12 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs12", 12, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs13 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs13", 13, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs14 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs14", 14, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs15 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs15", 15, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs16 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs16", 16, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs17 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs17", 17, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs18 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs18", 18, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs19 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs19", 19, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs20 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs20", 20, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs21 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs21", 21, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs22 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs22", 22, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs23 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs23", 23, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs24 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs24", 24, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs25 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs25", 25, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs26 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs26", 26, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs27 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs27", 27, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs28 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs28", 28, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs29 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs29", 29, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs30 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs30", 30, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs31 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs31", 31, WIFI_MOD_CLASS_HT);
  return mcs;
}


// Clause 22

WifiMode
WifiPhy::GetVhtMcs0 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs0", 0, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs1 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs1", 1, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs2 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs2", 2, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs3 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs3", 3, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs4 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs4", 4, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs5 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs5", 5, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs6 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs6", 6, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs7 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs7", 7, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs8 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs8", 8, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs9 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs9", 9, WIFI_MOD_CLASS_VHT);
  return mcs;
}

// Clause 26

WifiMode
WifiPhy::GetHeMcs0 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs0", 0, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs1 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs1", 1, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs2 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs2", 2, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs3 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs3", 3, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs4 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs4", 4, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs5 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs5", 5, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs6 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs6", 6, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs7 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs7", 7, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs8 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs8", 8, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs9 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs9", 9, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs10 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs10", 10, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs11 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs11", 11, WIFI_MOD_CLASS_HE);
  return mcs;
}

bool
WifiPhy::IsModeSupported (WifiMode mode) const
{
  for (uint8_t i = 0; i < GetNModes (); i++)
    {
      if (mode == GetMode (i))
        {
          return true;
        }
    }
  return false;
}

bool
WifiPhy::IsMcsSupported (WifiMode mcs) const
{
  for (uint8_t i = 0; i < GetNMcs (); i++)
    {
      if (mcs == GetMcs (i))
        {
          return true;
        }
    }
  return false;
}

uint8_t
WifiPhy::GetNModes (void) const
{
  return static_cast<uint8_t> (m_deviceRateSet.size ());
}

WifiMode
WifiPhy::GetMode (uint8_t mode) const
{
  return m_deviceRateSet[mode];
}

uint8_t
WifiPhy::GetNMcs (void) const
{
  return static_cast<uint8_t> (m_deviceMcsSet.size ());
}

WifiMode
WifiPhy::GetMcs (uint8_t mcs) const
{
  return m_deviceMcsSet[mcs];
}

bool
WifiPhy::IsStateCcaBusy (void) const
{
  return m_state->IsStateCcaBusy ();
}

bool
WifiPhy::IsStateIdle (void) const
{
  return m_state->IsStateIdle ();
}

bool
WifiPhy::IsStateRx (void) const
{
  return m_state->IsStateRx ();
}

bool
WifiPhy::IsStateTx (void) const
{
  return m_state->IsStateTx ();
}

bool
WifiPhy::IsStateSwitching (void) const
{
  return m_state->IsStateSwitching ();
}

bool
WifiPhy::IsStateSleep (void) const
{
  return m_state->IsStateSleep ();
}

bool
WifiPhy::IsStateOff (void) const
{
  return m_state->IsStateOff ();
}

Time
WifiPhy::GetDelayUntilIdle (void)
{
  return m_state->GetDelayUntilIdle ();
}

Time
WifiPhy::GetLastRxStartTime (void) const
{
  return m_state->GetLastRxStartTime ();
}

void
WifiPhy::SwitchMaybeToCcaBusy (void)
{
  NS_LOG_FUNCTION (this);
  //We are here because we have received the first bit of a packet and we are
  //not going to be able to synchronize on it
  //In this model, CCA becomes busy when the aggregation of all signals as
  //tracked by the InterferenceHelper class is higher than the CcaBusyThreshold

  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW);
  if (!delayUntilCcaEnd.IsZero ())
    {
      NS_LOG_DEBUG ("Calling SwitchMaybeToCcaBusy for " << delayUntilCcaEnd.As (Time::S));
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
WifiPhy::AbortCurrentReception (WifiPhyRxfailureReason reason)
{
  NS_LOG_FUNCTION (this << reason);
  if (m_endPreambleDetectionEvent.IsRunning ())
    {
      m_endPreambleDetectionEvent.Cancel ();
    }
  if (m_endPlcpRxEvent.IsRunning ())
    {
      m_endPlcpRxEvent.Cancel ();
    }
  if (m_endRxEvent.IsRunning ())
    {
      m_endRxEvent.Cancel ();
    }
  NotifyRxDrop (m_currentEvent->GetPacket (), reason);
  m_interference.NotifyRxEnd ();
  bool is_failure = (reason != OBSS_PD_CCA_RESET);
  m_state->SwitchFromRxAbort (is_failure);
  m_currentEvent = 0;
}

void
WifiPhy::ResetCca (bool powerRestricted, double txPowerMaxSiso, double txPowerMaxMimo)
{
  NS_LOG_FUNCTION (this << powerRestricted << txPowerMaxSiso << txPowerMaxMimo);
  m_powerRestricted = powerRestricted;
  m_txPowerMaxSiso = txPowerMaxSiso;
  m_txPowerMaxMimo = txPowerMaxMimo;
  NS_ASSERT ((m_currentEvent->GetEndTime () - Simulator::Now ()).IsPositive ());
  Simulator::Schedule (m_currentEvent->GetEndTime () - Simulator::Now (), &WifiPhy::EndReceiveInterBss, this);
  AbortCurrentReception (OBSS_PD_CCA_RESET);
}

double
WifiPhy::GetTxPowerForTransmission (WifiTxVector txVector) const
{
  NS_LOG_FUNCTION (this << m_powerRestricted);
  if (!m_powerRestricted)
    {
      return GetPowerDbm (txVector.GetTxPowerLevel ());
    }
  else
    {
      if (txVector.GetNss () > 1)
        {
          return std::min (m_txPowerMaxMimo, GetPowerDbm (txVector.GetTxPowerLevel ()));
        }
      else
        {
          return std::min (m_txPowerMaxSiso, GetPowerDbm (txVector.GetTxPowerLevel ()));
        }
    }
}

void
WifiPhy::StartRx (Ptr<Event> event, double rxPowerW, Time rxDuration)
{
  NS_LOG_FUNCTION (this << event->GetPacket () << event->GetTxVector () << event << rxPowerW << rxDuration);

  NS_LOG_DEBUG ("sync to signal (power=" << rxPowerW << "W)");
  m_interference.NotifyRxStart (); //We need to notify it now so that it starts recording events

  if (!m_endPreambleDetectionEvent.IsRunning ())
    {
      Time startOfPreambleDuration = GetPreambleDetectionDuration ();
      Time remainingRxDuration = rxDuration - startOfPreambleDuration;
      m_endPreambleDetectionEvent = Simulator::Schedule (startOfPreambleDuration, &WifiPhy::StartReceiveHeader, this,
                                                         event, remainingRxDuration);
    }
  else if ((m_frameCaptureModel != 0) && (rxPowerW > m_currentEvent->GetRxPowerW ()))
    {
      NS_LOG_DEBUG ("Received a stronger signal during preamble detection: drop current packet and switch to new packet");
      NotifyRxDrop (m_currentEvent->GetPacket (), PREAMBLE_DETECTION_PACKET_SWITCH);
      m_interference.NotifyRxEnd ();
      m_endPreambleDetectionEvent.Cancel ();
      m_interference.NotifyRxStart ();
      Time startOfPreambleDuration = GetPreambleDetectionDuration ();
      Time remainingRxDuration = rxDuration - startOfPreambleDuration;
      m_endPreambleDetectionEvent = Simulator::Schedule (startOfPreambleDuration, &WifiPhy::StartReceiveHeader, this,
                                                         event, remainingRxDuration);
    }
  else
    {
      NS_LOG_DEBUG ("Drop packet because RX is already decoding preamble");
      NotifyRxDrop (event->GetPacket (), NOT_ALLOWED);
      return;
    }
  m_currentEvent = event;
}

int64_t
WifiPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}

std::ostream& operator<< (std::ostream& os, WifiPhyState state)
{
  switch (state)
    {
    case WifiPhyState::IDLE:
      return (os << "IDLE");
    case WifiPhyState::CCA_BUSY:
      return (os << "CCA_BUSY");
    case WifiPhyState::TX:
      return (os << "TX");
    case WifiPhyState::RX:
      return (os << "RX");
    case WifiPhyState::SWITCHING:
      return (os << "SWITCHING");
    case WifiPhyState::SLEEP:
      return (os << "SLEEP");
    case WifiPhyState::OFF:
      return (os << "OFF");
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state");
      return (os << "INVALID");
    }
}

} //namespace ns3

namespace {

/**
 * Constructor class
 */
static class Constructor
{
public:
  Constructor ()
  {
    ns3::WifiPhy::GetDsssRate1Mbps ();
    ns3::WifiPhy::GetDsssRate2Mbps ();
    ns3::WifiPhy::GetDsssRate5_5Mbps ();
    ns3::WifiPhy::GetDsssRate11Mbps ();
    ns3::WifiPhy::GetErpOfdmRate6Mbps ();
    ns3::WifiPhy::GetErpOfdmRate9Mbps ();
    ns3::WifiPhy::GetErpOfdmRate12Mbps ();
    ns3::WifiPhy::GetErpOfdmRate18Mbps ();
    ns3::WifiPhy::GetErpOfdmRate24Mbps ();
    ns3::WifiPhy::GetErpOfdmRate36Mbps ();
    ns3::WifiPhy::GetErpOfdmRate48Mbps ();
    ns3::WifiPhy::GetErpOfdmRate54Mbps ();
    ns3::WifiPhy::GetOfdmRate6Mbps ();
    ns3::WifiPhy::GetOfdmRate9Mbps ();
    ns3::WifiPhy::GetOfdmRate12Mbps ();
    ns3::WifiPhy::GetOfdmRate18Mbps ();
    ns3::WifiPhy::GetOfdmRate24Mbps ();
    ns3::WifiPhy::GetOfdmRate36Mbps ();
    ns3::WifiPhy::GetOfdmRate48Mbps ();
    ns3::WifiPhy::GetOfdmRate54Mbps ();
    ns3::WifiPhy::GetOfdmRate3MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate4_5MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate6MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate9MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate12MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate18MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate24MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate27MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate1_5MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate2_25MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate3MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate4_5MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate6MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate9MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate12MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate13_5MbpsBW5MHz ();
    ns3::WifiPhy::GetHtMcs0 ();
    ns3::WifiPhy::GetHtMcs1 ();
    ns3::WifiPhy::GetHtMcs2 ();
    ns3::WifiPhy::GetHtMcs3 ();
    ns3::WifiPhy::GetHtMcs4 ();
    ns3::WifiPhy::GetHtMcs5 ();
    ns3::WifiPhy::GetHtMcs6 ();
    ns3::WifiPhy::GetHtMcs7 ();
    ns3::WifiPhy::GetHtMcs8 ();
    ns3::WifiPhy::GetHtMcs9 ();
    ns3::WifiPhy::GetHtMcs10 ();
    ns3::WifiPhy::GetHtMcs11 ();
    ns3::WifiPhy::GetHtMcs12 ();
    ns3::WifiPhy::GetHtMcs13 ();
    ns3::WifiPhy::GetHtMcs14 ();
    ns3::WifiPhy::GetHtMcs15 ();
    ns3::WifiPhy::GetHtMcs16 ();
    ns3::WifiPhy::GetHtMcs17 ();
    ns3::WifiPhy::GetHtMcs18 ();
    ns3::WifiPhy::GetHtMcs19 ();
    ns3::WifiPhy::GetHtMcs20 ();
    ns3::WifiPhy::GetHtMcs21 ();
    ns3::WifiPhy::GetHtMcs22 ();
    ns3::WifiPhy::GetHtMcs23 ();
    ns3::WifiPhy::GetHtMcs24 ();
    ns3::WifiPhy::GetHtMcs25 ();
    ns3::WifiPhy::GetHtMcs26 ();
    ns3::WifiPhy::GetHtMcs27 ();
    ns3::WifiPhy::GetHtMcs28 ();
    ns3::WifiPhy::GetHtMcs29 ();
    ns3::WifiPhy::GetHtMcs30 ();
    ns3::WifiPhy::GetHtMcs31 ();
    ns3::WifiPhy::GetVhtMcs0 ();
    ns3::WifiPhy::GetVhtMcs1 ();
    ns3::WifiPhy::GetVhtMcs2 ();
    ns3::WifiPhy::GetVhtMcs3 ();
    ns3::WifiPhy::GetVhtMcs4 ();
    ns3::WifiPhy::GetVhtMcs5 ();
    ns3::WifiPhy::GetVhtMcs6 ();
    ns3::WifiPhy::GetVhtMcs7 ();
    ns3::WifiPhy::GetVhtMcs8 ();
    ns3::WifiPhy::GetVhtMcs9 ();
    ns3::WifiPhy::GetHeMcs0 ();
    ns3::WifiPhy::GetHeMcs1 ();
    ns3::WifiPhy::GetHeMcs2 ();
    ns3::WifiPhy::GetHeMcs3 ();
    ns3::WifiPhy::GetHeMcs4 ();
    ns3::WifiPhy::GetHeMcs5 ();
    ns3::WifiPhy::GetHeMcs6 ();
    ns3::WifiPhy::GetHeMcs7 ();
    ns3::WifiPhy::GetHeMcs8 ();
    ns3::WifiPhy::GetHeMcs9 ();
    ns3::WifiPhy::GetHeMcs10 ();
    ns3::WifiPhy::GetHeMcs11 ();
  }
} g_constructor; ///< the constructor

}
