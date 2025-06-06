/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-phy.h"

#include "error-rate-model.h"
#include "frame-capture-model.h"
#include "interference-helper.h"
#include "preamble-detection-model.h"
#include "wifi-net-device.h"
#include "wifi-ppdu.h"
#include "wifi-psdu.h"
#include "wifi-radio-energy-model.h"
#include "wifi-utils.h"

#include "ns3/attribute-container.h"
#include "ns3/channel.h"
#include "ns3/dsss-phy.h"
#include "ns3/eht-phy.h" //also includes OFDM, HT, VHT and HE
#include "ns3/erp-ofdm-phy.h"
#include "ns3/error-model.h"
#include "ns3/ht-configuration.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/tuple.h"
#include "ns3/vht-configuration.h"

#include <algorithm>
#include <numeric>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                                                      \
    WIFI_PHY_NS_LOG_APPEND_CONTEXT(                                                                \
        (m_device && (m_device->GetNPhys() > m_phyId) && m_device->GetPhy(m_phyId)                 \
             ? m_device->GetPhy(m_phyId)                                                           \
             : nullptr))

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiPhy");

/****************************************************************
 *       The actual WifiPhy class
 ****************************************************************/

NS_OBJECT_ENSURE_REGISTERED(WifiPhy);

TypeId
WifiPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WifiPhy")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddAttribute("Channel",
                          "The channel attached to this PHY",
                          TypeId::ATTR_GET,
                          PointerValue(),
                          MakePointerAccessor(&WifiPhy::GetChannel),
                          MakePointerChecker<Channel>())
            .AddAttribute(
                "ChannelSettings",
                "A vector of tuple {channel number, channel width (MHz), PHY band, primary20 "
                "index} "
                "describing the settings of the operating channel for each segment. "
                "The primary20 index (only the value set for the first segment is used) "
                "is the index of the primary 20 MHz channel within the operating channel "
                "(0 indicates the 20 MHz subchannel with the lowest center frequency) and "
                "is only valid if the width of the operating channel is a multiple of 20 MHz. "
                "If the standard for this object has not been set yet, the value of this "
                "attribute is saved and will be used to set the operating channel when the "
                "standard is configured. If the PHY band is left unspecified, the default "
                "band for the configured standard is used. If the channel width and the "
                "channel number are both 0, the default channel width for the configured "
                "standard and band are used. If the channel number is 0, the default "
                "channel number for the configured standard, band and channel width is used. "
                "Note that the channel width can be left unspecified (0) if the channel "
                "number uniquely identify a frequency channel for the given standard and band.",
                StringValue("{0, 0, BAND_UNSPECIFIED, 0}"),
                MakeAttributeContainerAccessor<
                    TupleValue<UintegerValue, UintegerValue, EnumValue<WifiPhyBand>, UintegerValue>,
                    ';'>(static_cast<void (WifiPhy::*)(const ChannelSegments&)>(
                    &WifiPhy::SetOperatingChannel)),
                MakeAttributeContainerChecker<
                    TupleValue<UintegerValue, UintegerValue, EnumValue<WifiPhyBand>, UintegerValue>,
                    ';'>(MakeTupleChecker<UintegerValue,
                                          UintegerValue,
                                          EnumValue<WifiPhyBand>,
                                          UintegerValue>(
                    MakeUintegerChecker<uint8_t>(0, 233),
                    MakeUintegerChecker<MHz_u>(0, 320),
                    MakeEnumChecker(WifiPhyBand::WIFI_PHY_BAND_2_4GHZ,
                                    "BAND_2_4GHZ",
                                    WifiPhyBand::WIFI_PHY_BAND_5GHZ,
                                    "BAND_5GHZ",
                                    WifiPhyBand::WIFI_PHY_BAND_6GHZ,
                                    "BAND_6GHZ",
                                    WifiPhyBand::WIFI_PHY_BAND_UNSPECIFIED,
                                    "BAND_UNSPECIFIED"),
                    MakeUintegerChecker<uint8_t>(0, 15))))
            .AddAttribute("Frequency",
                          "The center frequency (MHz) of the current operating channel.",
                          TypeId::ATTR_GET,
                          UintegerValue(0),
                          MakeUintegerAccessor(&WifiPhy::GetFrequency),
                          MakeUintegerChecker<int64_t>())
            .AddAttribute("ChannelNumber",
                          "The channel number of the current operating channel.",
                          TypeId::ATTR_GET,
                          UintegerValue(0),
                          MakeUintegerAccessor(&WifiPhy::GetChannelNumber),
                          MakeUintegerChecker<uint8_t>(0, 233))
            .AddAttribute(
                "ChannelWidth",
                "The width in MHz of the current operating channel (5, 10, 20, 22, 40, 80, 160 or "
                "320). If 80+80MHz is used, this corresponds to the total channel width, hence 160 "
                "MHz.",
                TypeId::ATTR_GET,
                UintegerValue(0),
                MakeUintegerAccessor(&WifiPhy::GetChannelWidth),
                MakeUintegerChecker<MHz_u>(5, 320))
            .AddAttribute(
                "Primary20MHzIndex",
                "The index of the primary 20 MHz channel within the current operating channel "
                "(0 indicates the 20 MHz subchannel with the lowest center frequency).",
                UintegerValue(0),
                MakeUintegerAccessor(&WifiPhy::GetPrimary20Index),
                MakeUintegerChecker<uint8_t>(0, 7))
            .AddAttribute("FixedPhyBand",
                          "If set to true, changing PHY band is prohibited after initialization.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&WifiPhy::SetFixedPhyBand, &WifiPhy::HasFixedPhyBand),
                          MakeBooleanChecker())
            .AddAttribute(
                "MaxRadioBw",
                "The maximum width supported by the radio. It is not possible to configure an "
                "operating channel with a total width larger than this value. A value of 0 means "
                "no restriction.",
                TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                DoubleValue(MHz_u{0}),
                MakeDoubleAccessor(&WifiPhy::m_maxRadioBw),
                MakeDoubleChecker<MHz_u>())
            .AddAttribute(
                "RxSensitivity",
                "The energy of a received signal should be higher than "
                "this threshold (dBm) for the PHY to detect the signal. "
                "This threshold refers to a width of 20 MHz and will be "
                "scaled to match the width of the received signal.",
                DoubleValue(-101.0),
                MakeDoubleAccessor(&WifiPhy::SetRxSensitivity, &WifiPhy::GetRxSensitivity),
                MakeDoubleChecker<dBm_u>())
            .AddAttribute(
                "CcaEdThreshold",
                "The energy of all received signals should be higher than "
                "this threshold (dBm) in the primary channel to allow the PHY layer "
                "to declare CCA BUSY state.",
                DoubleValue(-62.0),
                MakeDoubleAccessor(&WifiPhy::SetCcaEdThreshold, &WifiPhy::GetCcaEdThreshold),
                MakeDoubleChecker<dBm_u>())
            .AddAttribute("CcaSensitivity",
                          "The energy of a received wifi signal should be higher than "
                          "this threshold (dBm) in the primary channel to allow the PHY layer "
                          "to declare CCA BUSY state.",
                          DoubleValue(-82.0),
                          MakeDoubleAccessor(&WifiPhy::SetCcaSensitivityThreshold,
                                             &WifiPhy::GetCcaSensitivityThreshold),
                          MakeDoubleChecker<dBm_u>())
            .AddAttribute("TxGain",
                          "Transmission gain (dB).",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&WifiPhy::SetTxGain, &WifiPhy::GetTxGain),
                          MakeDoubleChecker<dB_u>())
            .AddAttribute("RxGain",
                          "Reception gain (dB).",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&WifiPhy::SetRxGain, &WifiPhy::GetRxGain),
                          MakeDoubleChecker<dB_u>())
            .AddAttribute("TxPowerLevels",
                          "Number of transmission power levels available between "
                          "TxPowerStart and TxPowerEnd included.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&WifiPhy::m_nTxPower),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("TxPowerEnd",
                          "Maximum available transmission level (dBm).",
                          DoubleValue(16.0206),
                          MakeDoubleAccessor(&WifiPhy::SetTxPowerEnd, &WifiPhy::GetTxPowerEnd),
                          MakeDoubleChecker<dBm_u>())
            .AddAttribute("TxPowerStart",
                          "Minimum available transmission level (dBm).",
                          DoubleValue(16.0206),
                          MakeDoubleAccessor(&WifiPhy::SetTxPowerStart, &WifiPhy::GetTxPowerStart),
                          MakeDoubleChecker<dBm_u>())
            .AddAttribute(
                "RxNoiseFigure",
                "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                "\"the difference in decibels (dB) between"
                " the noise output of the actual receiver to the noise output of an "
                " ideal receiver with the same overall gain and bandwidth when the receivers "
                " are connected to sources at the standard noise temperature T0 (usually 290 K)\".",
                DoubleValue(7),
                MakeDoubleAccessor(&WifiPhy::SetRxNoiseFigure),
                MakeDoubleChecker<dB_u>())
            .AddAttribute("State",
                          "The state of the PHY layer.",
                          PointerValue(),
                          MakePointerAccessor(&WifiPhy::m_state),
                          MakePointerChecker<WifiPhyStateHelper>())
            .AddAttribute("ChannelSwitchDelay",
                          "Delay between two short frames transmitted on different frequencies.",
                          TimeValue(MicroSeconds(250)),
                          MakeTimeAccessor(&WifiPhy::m_channelSwitchDelay),
                          MakeTimeChecker(Seconds(0)))
            .AddAttribute(
                "Antennas",
                "The number of antennas on the device.",
                UintegerValue(1),
                MakeUintegerAccessor(&WifiPhy::GetNumberOfAntennas, &WifiPhy::SetNumberOfAntennas),
                MakeUintegerChecker<uint8_t>(1, 8))
            .AddAttribute("MaxSupportedTxSpatialStreams",
                          "The maximum number of supported TX spatial streams."
                          "This parameter is only valuable for 802.11n/ac/ax STAs and APs.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&WifiPhy::GetMaxSupportedTxSpatialStreams,
                                               &WifiPhy::SetMaxSupportedTxSpatialStreams),
                          MakeUintegerChecker<uint8_t>(1, 8))
            .AddAttribute("MaxSupportedRxSpatialStreams",
                          "The maximum number of supported RX spatial streams."
                          "This parameter is only valuable for 802.11n/ac/ax STAs and APs.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&WifiPhy::GetMaxSupportedRxSpatialStreams,
                                               &WifiPhy::SetMaxSupportedRxSpatialStreams),
                          MakeUintegerChecker<uint8_t>(1, 8))
            .AddAttribute("ShortPlcpPreambleSupported",
                          "Whether or not short PHY preamble is supported."
                          "This parameter is only valuable for 802.11b STAs and APs."
                          "Note: 802.11g APs and STAs always support short PHY preamble.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&WifiPhy::GetShortPhyPreambleSupported,
                                              &WifiPhy::SetShortPhyPreambleSupported),
                          MakeBooleanChecker())
            .AddAttribute("FrameCaptureModel",
                          "Ptr to an object that implements the frame capture model",
                          PointerValue(),
                          MakePointerAccessor(&WifiPhy::m_frameCaptureModel),
                          MakePointerChecker<FrameCaptureModel>())
            .AddAttribute("PreambleDetectionModel",
                          "Ptr to an object that implements the preamble detection model",
                          PointerValue(),
                          MakePointerAccessor(&WifiPhy::m_preambleDetectionModel),
                          MakePointerChecker<PreambleDetectionModel>())
            .AddAttribute("PostReceptionErrorModel",
                          "An optional packet error model can be added to the receive "
                          "packet process after any propagation-based (SNR-based) error "
                          "models have been applied. Typically this is used to force "
                          "specific packet drops, for testing purposes.",
                          PointerValue(),
                          MakePointerAccessor(&WifiPhy::m_postReceptionErrorModel),
                          MakePointerChecker<ErrorModel>())
            .AddAttribute("InterferenceHelper",
                          "Ptr to an object that implements the interference helper",
                          PointerValue(),
                          MakePointerAccessor(&WifiPhy::m_interference),
                          MakePointerChecker<InterferenceHelper>())
            .AddAttribute("Sifs",
                          "The duration of the Short Interframe Space. "
                          "NOTE that the default value is overwritten by the value defined "
                          "by the standard; if you want to set this attribute, you have to "
                          "do it after that the PHY object is initialized.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&WifiPhy::m_sifs),
                          MakeTimeChecker())
            .AddAttribute("Slot",
                          "The duration of a slot. "
                          "NOTE that the default value is overwritten by the value defined "
                          "by the standard; if you want to set this attribute, you have to "
                          "do it after that the PHY object is initialized.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&WifiPhy::m_slot),
                          MakeTimeChecker())
            .AddAttribute("Pifs",
                          "The duration of the PCF Interframe Space. "
                          "NOTE that the default value is overwritten by the value defined "
                          "by the standard; if you want to set this attribute, you have to "
                          "do it after that the PHY object is initialized.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&WifiPhy::m_pifs),
                          MakeTimeChecker())
            .AddAttribute("PowerDensityLimit",
                          "The mean equivalent isotropically radiated power density"
                          "limit (in dBm/MHz) set by regulators.",
                          DoubleValue(100.0), // set to a high value so as to have no effect
                          MakeDoubleAccessor(&WifiPhy::m_powerDensityLimit),
                          MakeDoubleChecker<dBm_per_MHz_u>())
            .AddAttribute("NotifyMacHdrRxEnd",
                          "Whether the PHY is capable of notifying the MAC about the end of "
                          "the reception of the MAC header of every MPDU.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&WifiPhy::m_notifyRxMacHeaderEnd),
                          MakeBooleanChecker())
            .AddTraceSource(
                "PhyTxBegin",
                "Trace source indicating a packet has begun transmitting over the medium; "
                "the packet holds a single MPDU even if the MPDU is transmitted within an A-MPDU "
                "(in which case this trace fires for each MPDU in the "
                "A-MPDU).",
                MakeTraceSourceAccessor(&WifiPhy::m_phyTxBeginTrace),
                "ns3::WifiPhy::PhyTxBeginTracedCallback")
            .AddTraceSource(
                "PhyTxPsduBegin",
                "Trace source indicating a PSDU has begun transmitting over the channel medium; "
                "this trace returns a WifiConstPsduMap with a single element (in the case of SU "
                "PPDU) "
                "or multiple elements (in the case of MU PPDU)",
                MakeTraceSourceAccessor(&WifiPhy::m_phyTxPsduBeginTrace),
                "ns3::WifiPhy::PsduTxBeginCallback")
            .AddTraceSource("PhyTxEnd",
                            "Trace source indicating a packet "
                            "has been completely transmitted over the channel.",
                            MakeTraceSourceAccessor(&WifiPhy::m_phyTxEndTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyTxDrop",
                            "Trace source indicating a packet "
                            "has been dropped by the device during transmission",
                            MakeTraceSourceAccessor(&WifiPhy::m_phyTxDropTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyRxBegin",
                            "Trace source indicating a packet "
                            "has begun being received from the channel medium "
                            "by the device",
                            MakeTraceSourceAccessor(&WifiPhy::m_phyRxBeginTrace),
                            "ns3::WifiPhy::PhyRxBeginTracedCallback")
            .AddTraceSource("PhyRxPayloadBegin",
                            "Trace source indicating the reception of the "
                            "payload of a PPDU has begun",
                            MakeTraceSourceAccessor(&WifiPhy::m_phyRxPayloadBeginTrace),
                            "ns3::WifiPhy::PhyRxPayloadBeginTracedCallback")
            .AddTraceSource("PhyRxMacHeaderEnd",
                            "Trace source indicating the MAC header of an MPDU has been "
                            "completely received.",
                            MakeTraceSourceAccessor(&WifiPhy::m_phyRxMacHeaderEndTrace),
                            "ns3::WifiPhy::PhyRxMacHeaderEndTracedCallback")
            .AddTraceSource("PhyRxEnd",
                            "Trace source indicating a packet "
                            "has been completely received from the channel medium "
                            "by the device",
                            MakeTraceSourceAccessor(&WifiPhy::m_phyRxEndTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyRxDrop",
                            "Trace source indicating a packet "
                            "has been dropped by the device during reception",
                            MakeTraceSourceAccessor(&WifiPhy::m_phyRxDropTrace),
                            "ns3::WifiPhy::PhyRxDropTracedCallback")
            .AddTraceSource("PhyRxPpduDrop",
                            "Trace source indicating a ppdu "
                            "has been dropped by the device during reception",
                            MakeTraceSourceAccessor(&WifiPhy::m_phyRxPpduDropTrace),
                            "ns3::WifiPhy::PhyRxPpduDropTracedCallback")
            .AddTraceSource("MonitorSnifferRx",
                            "Trace source simulating a wifi device in monitor mode "
                            "sniffing all received frames",
                            MakeTraceSourceAccessor(&WifiPhy::m_phyMonitorSniffRxTrace),
                            "ns3::WifiPhy::MonitorSnifferRxTracedCallback")
            .AddTraceSource("MonitorSnifferTx",
                            "Trace source simulating the capability of a wifi device "
                            "in monitor mode to sniff all frames being transmitted",
                            MakeTraceSourceAccessor(&WifiPhy::m_phyMonitorSniffTxTrace),
                            "ns3::WifiPhy::MonitorSnifferTxTracedCallback")
            .AddTraceSource("SignalTransmission",
                            "Trace start of signal transmission",
                            MakeTraceSourceAccessor(&WifiPhy::m_signalTransmissionCb),
                            "ns3::SpectrumWifiPhy::SignalTransmissionCallback");
    return tid;
}

WifiPhy::WifiPhy()
    : m_phyId(0),
      m_txMpduReferenceNumber(0xffffffff),
      m_rxMpduReferenceNumber(0xffffffff),
      m_endPhyRxEvent(),
      m_endTxEvent(),
      m_currentEvent(nullptr),
      m_previouslyRxPpduUid(UINT64_MAX),
      m_standard(WIFI_STANDARD_UNSPECIFIED),
      m_maxModClassSupported(WIFI_MOD_CLASS_UNKNOWN),
      m_band(WIFI_PHY_BAND_UNSPECIFIED),
      m_sifs(),
      m_slot(),
      m_pifs(),
      m_powerRestricted(false),
      m_channelAccessRequested(false),
      m_txSpatialStreams(1),
      m_rxSpatialStreams(1),
      m_wifiRadioEnergyModel(nullptr),
      m_timeLastPreambleDetected()
{
    NS_LOG_FUNCTION(this);
    m_random = CreateObject<UniformRandomVariable>();
    m_state = CreateObject<WifiPhyStateHelper>();
}

WifiPhy::~WifiPhy()
{
    NS_LOG_FUNCTION(this);
}

void
WifiPhy::DoInitialize()
{
    NS_LOG_FUNCTION(this);

    // This method ensures that the local mobility model pointer holds
    // a pointer to the Node's aggregated mobility model (if one exists)
    // in the case that the user has not directly called SetMobility()
    // on this WifiPhy during simulation setup.  If the mobility model
    // needs to be added or changed during simulation runtime, users must
    // call SetMobility() on this object.

    if (!m_mobility)
    {
        NS_ABORT_MSG_UNLESS(m_device && m_device->GetNode(),
                            "Either install a MobilityModel on this object or ensure that this "
                            "object is part of a Node and NetDevice");
        m_mobility = m_device->GetNode()->GetObject<MobilityModel>();
        if (!m_mobility)
        {
            NS_LOG_WARN("Mobility not found, propagation models might not work properly");
        }
    }
}

void
WifiPhy::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_device = nullptr;
    m_mobility = nullptr;
    m_frameCaptureModel = nullptr;
    m_preambleDetectionModel = nullptr;
    m_wifiRadioEnergyModel = nullptr;
    m_postReceptionErrorModel = nullptr;
    if (m_interference)
    {
        m_interference->Dispose();
    }
    m_interference = nullptr;
    m_random = nullptr;
    m_state = nullptr;

    Reset();

    // this should be done after calling the Reset function
    for (auto& phyEntity : m_phyEntities)
    {
        phyEntity.second = nullptr;
    }
    m_phyEntities.clear();
}

std::map<WifiModulationClass, Ptr<PhyEntity>>&
WifiPhy::GetStaticPhyEntities()
{
    static std::map<WifiModulationClass, Ptr<PhyEntity>> g_staticPhyEntities;
    return g_staticPhyEntities;
}

Ptr<WifiPhyStateHelper>
WifiPhy::GetState() const
{
    return m_state;
}

void
WifiPhy::SetReceiveOkCallback(RxOkCallback callback)
{
    m_state->SetReceiveOkCallback(callback);
}

void
WifiPhy::SetReceiveErrorCallback(RxErrorCallback callback)
{
    m_state->SetReceiveErrorCallback(callback);
}

void
WifiPhy::RegisterListener(const std::shared_ptr<WifiPhyListener>& listener)
{
    m_state->RegisterListener(listener);
    if (IsInitialized())
    {
        // provide CCA busy information upon registering a PHY listener
        SwitchMaybeToCcaBusy(nullptr);
    }
}

void
WifiPhy::UnregisterListener(const std::shared_ptr<WifiPhyListener>& listener)
{
    m_state->UnregisterListener(listener);
}

void
WifiPhy::SetCapabilitiesChangedCallback(Callback<void> callback)
{
    m_capabilitiesChangedCallback = callback;
}

void
WifiPhy::SetRxSensitivity(dBm_u threshold)
{
    NS_LOG_FUNCTION(this << threshold);
    m_rxSensitivity = threshold;
}

dBm_u
WifiPhy::GetRxSensitivity() const
{
    return m_rxSensitivity;
}

void
WifiPhy::SetCcaEdThreshold(dBm_u threshold)
{
    NS_LOG_FUNCTION(this << threshold);
    m_ccaEdThreshold = threshold;
}

dBm_u
WifiPhy::GetCcaEdThreshold() const
{
    return m_ccaEdThreshold;
}

void
WifiPhy::SetCcaSensitivityThreshold(dBm_u threshold)
{
    NS_LOG_FUNCTION(this << threshold);
    m_ccaSensitivityThreshold = threshold;
}

dBm_u
WifiPhy::GetCcaSensitivityThreshold() const
{
    return m_ccaSensitivityThreshold;
}

void
WifiPhy::SetRxNoiseFigure(dB_u noiseFigure)
{
    NS_LOG_FUNCTION(this << noiseFigure);
    if (m_interference)
    {
        m_interference->SetNoiseFigure(DbToRatio(noiseFigure));
    }
    m_noiseFigure = noiseFigure;
}

void
WifiPhy::SetTxPowerStart(dBm_u start)
{
    NS_LOG_FUNCTION(this << start);
    m_txPowerBase = start;
}

dBm_u
WifiPhy::GetTxPowerStart() const
{
    return m_txPowerBase;
}

void
WifiPhy::SetTxPowerEnd(dBm_u end)
{
    NS_LOG_FUNCTION(this << end);
    m_txPowerEnd = end;
}

dBm_u
WifiPhy::GetTxPowerEnd() const
{
    return m_txPowerEnd;
}

void
WifiPhy::SetNTxPower(uint8_t n)
{
    NS_LOG_FUNCTION(this << +n);
    m_nTxPower = n;
}

uint8_t
WifiPhy::GetNTxPower() const
{
    return m_nTxPower;
}

void
WifiPhy::SetTxGain(dB_u gain)
{
    NS_LOG_FUNCTION(this << gain);
    m_txGain = gain;
}

dB_u
WifiPhy::GetTxGain() const
{
    return m_txGain;
}

void
WifiPhy::SetRxGain(dB_u gain)
{
    NS_LOG_FUNCTION(this << gain);
    m_rxGain = gain;
}

dB_u
WifiPhy::GetRxGain() const
{
    return m_rxGain;
}

void
WifiPhy::SetShortPhyPreambleSupported(bool enable)
{
    NS_LOG_FUNCTION(this << enable);
    m_shortPreamble = enable;
}

bool
WifiPhy::GetShortPhyPreambleSupported() const
{
    return m_shortPreamble;
}

void
WifiPhy::SetDevice(const Ptr<WifiNetDevice> device)
{
    m_device = device;
}

Ptr<WifiNetDevice>
WifiPhy::GetDevice() const
{
    return m_device;
}

void
WifiPhy::SetMobility(const Ptr<MobilityModel> mobility)
{
    m_mobility = mobility;
}

Ptr<MobilityModel>
WifiPhy::GetMobility() const
{
    return m_mobility;
}

void
WifiPhy::SetPhyId(uint8_t phyId)
{
    NS_LOG_FUNCTION(this << phyId);
    m_phyId = phyId;
}

uint8_t
WifiPhy::GetPhyId() const
{
    return m_phyId;
}

void
WifiPhy::SetInterferenceHelper(const Ptr<InterferenceHelper> helper)
{
    NS_LOG_FUNCTION(this << helper);
    m_interference = helper;
    m_interference->SetNoiseFigure(DbToRatio(m_noiseFigure));
    m_interference->SetNumberOfReceiveAntennas(m_numberOfAntennas);
}

void
WifiPhy::SetErrorRateModel(const Ptr<ErrorRateModel> model)
{
    NS_LOG_FUNCTION(this << model);
    NS_ASSERT(m_interference);
    m_interference->SetErrorRateModel(model);
}

void
WifiPhy::SetPostReceptionErrorModel(const Ptr<ErrorModel> em)
{
    NS_LOG_FUNCTION(this << em);
    m_postReceptionErrorModel = em;
}

void
WifiPhy::SetFrameCaptureModel(const Ptr<FrameCaptureModel> model)
{
    m_frameCaptureModel = model;
}

void
WifiPhy::SetPreambleDetectionModel(const Ptr<PreambleDetectionModel> model)
{
    m_preambleDetectionModel = model;
}

void
WifiPhy::SetWifiRadioEnergyModel(const Ptr<WifiRadioEnergyModel> wifiRadioEnergyModel)
{
    m_wifiRadioEnergyModel = wifiRadioEnergyModel;
}

dBm_u
WifiPhy::GetPower(uint8_t powerLevel) const
{
    NS_ASSERT(m_txPowerBase <= m_txPowerEnd);
    NS_ASSERT(m_nTxPower > 0);
    dBm_u dbm;
    if (m_nTxPower > 1)
    {
        dbm = m_txPowerBase + dB_u{powerLevel * (m_txPowerEnd - m_txPowerBase) / (m_nTxPower - 1)};
    }
    else
    {
        NS_ASSERT_MSG(m_txPowerBase == m_txPowerEnd,
                      "cannot have TxPowerEnd != TxPowerStart with TxPowerLevels == 1");
        dbm = m_txPowerBase;
    }
    return dbm;
}

Time
WifiPhy::GetChannelSwitchDelay() const
{
    return m_channelSwitchDelay;
}

double
WifiPhy::CalculateSnr(const WifiTxVector& txVector, double ber) const
{
    return m_interference->GetErrorRateModel()->CalculateSnr(txVector, ber);
}

const Ptr<const PhyEntity>
WifiPhy::GetStaticPhyEntity(WifiModulationClass modulation)
{
    const auto it = GetStaticPhyEntities().find(modulation);
    NS_ABORT_MSG_IF(it == GetStaticPhyEntities().cend(),
                    "Unimplemented Wi-Fi modulation class " << modulation);
    return it->second;
}

Ptr<PhyEntity>
WifiPhy::GetPhyEntity(WifiModulationClass modulation) const
{
    const auto it = m_phyEntities.find(modulation);
    NS_ABORT_MSG_IF(it == m_phyEntities.cend(),
                    "Unsupported Wi-Fi modulation class " << modulation);
    return it->second;
}

Ptr<PhyEntity>
WifiPhy::GetPhyEntity(WifiStandard standard) const
{
    return GetPhyEntity(GetModulationClassForStandard(standard));
}

Ptr<PhyEntity>
WifiPhy::GetLatestPhyEntity() const
{
    return GetPhyEntity(m_standard);
}

Ptr<PhyEntity>
WifiPhy::GetPhyEntityForPpdu(const Ptr<const WifiPpdu> ppdu) const
{
    NS_ABORT_IF(!ppdu);
    const auto modulation = ppdu->GetModulation();
    if (modulation > m_phyEntities.rbegin()->first)
    {
        // unsupported modulation: start reception process with latest PHY entity
        return GetLatestPhyEntity();
    }
    if (modulation < WIFI_MOD_CLASS_HT)
    {
        // for non-HT (duplicate), call the latest PHY entity since some extra processing can be
        // done in PHYs implemented in HT and later (e.g. channel width selection for non-HT
        // duplicates)
        return GetLatestPhyEntity();
    }
    return GetPhyEntity(modulation);
}

void
WifiPhy::AddStaticPhyEntity(WifiModulationClass modulation, Ptr<PhyEntity> phyEntity)
{
    NS_ASSERT_MSG(!GetStaticPhyEntities().contains(modulation),
                  "The PHY entity has already been added. The setting should only be done once per "
                  "modulation class");
    GetStaticPhyEntities()[modulation] = phyEntity;
}

void
WifiPhy::AddPhyEntity(WifiModulationClass modulation, Ptr<PhyEntity> phyEntity)
{
    NS_LOG_FUNCTION(this << modulation);
    NS_ABORT_MSG_IF(!GetStaticPhyEntities().contains(modulation),
                    "Cannot add an unimplemented PHY to supported list. Update the former first.");
    NS_ASSERT_MSG(m_phyEntities.find(modulation) == m_phyEntities.end(),
                  "The PHY entity has already been added. The setting should only be done once per "
                  "modulation class");
    phyEntity->SetOwner(this);
    m_phyEntities[modulation] = phyEntity;
}

void
WifiPhy::SetSifs(Time sifs)
{
    m_sifs = sifs;
}

Time
WifiPhy::GetSifs() const
{
    return m_sifs;
}

void
WifiPhy::SetSlot(Time slot)
{
    m_slot = slot;
}

Time
WifiPhy::GetSlot() const
{
    return m_slot;
}

void
WifiPhy::SetPifs(Time pifs)
{
    m_pifs = pifs;
}

Time
WifiPhy::GetPifs() const
{
    return m_pifs;
}

void
WifiPhy::Configure80211a()
{
    NS_LOG_FUNCTION(this);
    AddPhyEntity(WIFI_MOD_CLASS_OFDM, Create<OfdmPhy>());

    // See Table 17-21 "OFDM PHY characteristics" of 802.11-2016
    SetSifs(OFDM_SIFS_TIME_20MHZ);
    SetSlot(OFDM_SLOT_TIME_20MHZ);
    SetPifs(GetSifs() + GetSlot());
    // See Table 10-5 "Determination of the EstimatedAckTxTime based on properties
    // of the PPDU causing the EIFS" of 802.11-2016
}

void
WifiPhy::Configure80211b()
{
    NS_LOG_FUNCTION(this);
    Ptr<DsssPhy> phyEntity = Create<DsssPhy>();
    AddPhyEntity(WIFI_MOD_CLASS_HR_DSSS, phyEntity);
    AddPhyEntity(WIFI_MOD_CLASS_DSSS, phyEntity); // when plain DSSS modes are used

    SetSifs(DSSS_SIFS_TIME);
    SetSlot(DSSS_SLOT_TIME);
    SetPifs(GetSifs() + GetSlot());
    // See Table 10-5 "Determination of the EstimatedAckTxTime based on properties
    // of the PPDU causing the EIFS" of 802.11-2016
}

void
WifiPhy::Configure80211g()
{
    NS_LOG_FUNCTION(this);
    // See Table 18-5 "ERP characteristics" of 802.11-2016
    // Slot time defaults to the "long slot time" of 20 us in the standard
    // according to mixed 802.11b/g deployments.  Short slot time is enabled
    // if the user sets the ShortSlotTimeSupported flag to true and when the BSS
    // consists of only ERP STAs capable of supporting this option.
    Configure80211b();
    AddPhyEntity(WIFI_MOD_CLASS_ERP_OFDM, Create<ErpOfdmPhy>());
}

void
WifiPhy::Configure80211p()
{
    NS_LOG_FUNCTION(this);
    if (GetChannelWidth() == MHz_u{10})
    {
        AddPhyEntity(WIFI_MOD_CLASS_OFDM, Create<OfdmPhy>(OFDM_PHY_10_MHZ));

        // See Table 17-21 "OFDM PHY characteristics" of 802.11-2016
        SetSifs(OFDM_SIFS_TIME_10MHZ);
        SetSlot(OFDM_SLOT_TIME_10MHZ);
        SetPifs(GetSifs() + GetSlot());
    }
    else if (GetChannelWidth() == MHz_u{5})
    {
        AddPhyEntity(WIFI_MOD_CLASS_OFDM, Create<OfdmPhy>(OFDM_PHY_5_MHZ));

        // See Table 17-21 "OFDM PHY characteristics" of 802.11-2016
        SetSifs(OFDM_SIFS_TIME_5MHZ);
        SetSlot(OFDM_SLOT_TIME_5MHZ);
        SetPifs(GetSifs() + GetSlot());
    }
    else
    {
        NS_FATAL_ERROR("802.11p configured with a wrong channel width!");
    }
}

void
WifiPhy::Configure80211n()
{
    NS_LOG_FUNCTION(this);
    if (m_band == WIFI_PHY_BAND_2_4GHZ)
    {
        Configure80211g();
    }
    else
    {
        Configure80211a();
    }
    AddPhyEntity(WIFI_MOD_CLASS_HT, Create<HtPhy>(m_txSpatialStreams));
}

void
WifiPhy::Configure80211ac()
{
    NS_LOG_FUNCTION(this);
    Configure80211n();
    AddPhyEntity(WIFI_MOD_CLASS_VHT, Create<VhtPhy>());
}

void
WifiPhy::Configure80211ax()
{
    NS_LOG_FUNCTION(this);
    if (m_band == WIFI_PHY_BAND_2_4GHZ)
    {
        Configure80211n();
    }
    else
    {
        Configure80211ac();
    }
    AddPhyEntity(WIFI_MOD_CLASS_HE, Create<HePhy>());
}

void
WifiPhy::Configure80211be()
{
    NS_LOG_FUNCTION(this);
    Configure80211ax();
    AddPhyEntity(WIFI_MOD_CLASS_EHT, Create<EhtPhy>());
}

void
WifiPhy::SetMaxModulationClassSupported(WifiModulationClass modClass)
{
    NS_LOG_FUNCTION(this << modClass);
    m_maxModClassSupported = modClass;
}

WifiModulationClass
WifiPhy::GetMaxModulationClassSupported() const
{
    return m_maxModClassSupported;
}

void
WifiPhy::ConfigureStandard(WifiStandard standard)
{
    NS_LOG_FUNCTION(this << standard);

    NS_ABORT_MSG_IF(m_standard != WIFI_STANDARD_UNSPECIFIED && standard != m_standard,
                    "Cannot change standard");

    m_standard = standard;

    if (m_maxModClassSupported == WIFI_MOD_CLASS_UNKNOWN)
    {
        m_maxModClassSupported = GetModulationClassForStandard(m_standard);
    }

    if (!m_operatingChannel.IsSet())
    {
        NS_LOG_DEBUG("Setting the operating channel first");
        SetOperatingChannel(m_channelSettings);
        // return because we are called back by SetOperatingChannel
        return;
    }

    // this function is called when changing PHY band, hence we have to delete
    // the previous PHY entities
    m_phyEntities.clear();

    switch (standard)
    {
    case WIFI_STANDARD_80211a:
        Configure80211a();
        break;
    case WIFI_STANDARD_80211b:
        Configure80211b();
        break;
    case WIFI_STANDARD_80211g:
        Configure80211g();
        break;
    case WIFI_STANDARD_80211p:
        Configure80211p();
        break;
    case WIFI_STANDARD_80211n:
        Configure80211n();
        break;
    case WIFI_STANDARD_80211ac:
        Configure80211ac();
        break;
    case WIFI_STANDARD_80211ax:
        Configure80211ax();
        break;
    case WIFI_STANDARD_80211be:
        Configure80211be();
        break;
    case WIFI_STANDARD_UNSPECIFIED:
    default:
        NS_ASSERT_MSG(false, "Unsupported standard");
        break;
    }
}

WifiPhyBand
WifiPhy::GetPhyBand() const
{
    return m_band;
}

WifiStandard
WifiPhy::GetStandard() const
{
    return m_standard;
}

const WifiPhyOperatingChannel&
WifiPhy::GetOperatingChannel() const
{
    return m_operatingChannel;
}

MHz_u
WifiPhy::GetFrequency() const
{
    return m_operatingChannel.GetFrequency();
}

uint8_t
WifiPhy::GetChannelNumber() const
{
    return m_operatingChannel.GetNumber();
}

MHz_u
WifiPhy::GetChannelWidth() const
{
    return m_operatingChannel.GetTotalWidth();
}

uint8_t
WifiPhy::GetPrimary20Index() const
{
    return m_operatingChannel.GetPrimaryChannelIndex(MHz_u{20});
}

void
WifiPhy::SetFixedPhyBand(bool enable)
{
    m_fixedPhyBand = enable;
}

bool
WifiPhy::HasFixedPhyBand() const
{
    return m_fixedPhyBand;
}

MHz_u
WifiPhy::GetTxBandwidth(WifiMode mode, MHz_u maxAllowedWidth) const
{
    auto modulation = mode.GetModulationClass();
    if (modulation == WIFI_MOD_CLASS_DSSS || modulation == WIFI_MOD_CLASS_HR_DSSS)
    {
        return MHz_u{22};
    }

    return std::min({GetChannelWidth(), GetMaximumChannelWidth(modulation), maxAllowedWidth});
}

void
WifiPhy::SetOperatingChannel(const WifiPhyOperatingChannel& channel)
{
    NS_LOG_FUNCTION(this << channel);
    ChannelSegments segments{};
    for (std::size_t segmentId = 0; segmentId < channel.GetNSegments(); ++segmentId)
    {
        segments.emplace_back(channel.GetNumber(segmentId),
                              channel.GetWidth(segmentId),
                              channel.GetPhyBand(),
                              channel.GetPrimaryChannelIndex(MHz_u{20}));
    }
    SetOperatingChannel(segments);
}

void
WifiPhy::SetOperatingChannel(const ChannelTuple& tuple)
{
    SetOperatingChannel(ChannelSegments{tuple});
}

void
WifiPhy::SetOperatingChannel(const ChannelSegments& channelSegments)
{
    NS_LOG_FUNCTION(this << +std::get<0>(channelSegments.front())
                         << std::get<1>(channelSegments.front())
                         << static_cast<WifiPhyBand>(std::get<2>(channelSegments.front()))
                         << +std::get<3>(channelSegments.front()));

    m_channelSettings = channelSegments;

    if (m_standard == WIFI_STANDARD_UNSPECIFIED)
    {
        NS_LOG_DEBUG("Channel information will be applied when a standard is configured");
        return;
    }

    if (IsInitialized())
    {
        const auto delay = GetDelayUntilChannelSwitch();
        if (!delay.has_value())
        {
            // switching channel is not possible now
            return;
        }
        if (delay.value().IsStrictlyPositive())
        {
            // switching channel has been postponed
            void (WifiPhy::*fp)(const ChannelSegments&) = &WifiPhy::SetOperatingChannel;
            Simulator::Schedule(delay.value(), fp, this, channelSegments);
            return;
        }
    }

    // channel can be switched now.
    DoChannelSwitch();
}

std::optional<Time>
WifiPhy::GetDelayUntilChannelSwitch()
{
    if (!IsInitialized())
    {
        // this is not channel switch, this is initialization
        NS_LOG_DEBUG("Before initialization, nothing to do");
        return Seconds(0);
    }

    std::optional<Time> delay;
    switch (m_state->GetState())
    {
    case WifiPhyState::RX:
        NS_LOG_DEBUG("drop packet because of channel switching while reception");
        AbortCurrentReception(CHANNEL_SWITCHING);
        delay = Seconds(0);
        break;
    case WifiPhyState::TX:
        NS_LOG_DEBUG("channel switching postponed until end of current transmission");
        delay = GetDelayUntilIdle();
        break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
        Reset();
        delay = Seconds(0);
        break;
    case WifiPhyState::SWITCHING:
        delay = Seconds(0);
        break;
    case WifiPhyState::SLEEP:
        NS_LOG_DEBUG("channel switching ignored in sleep mode");
        break;
    default:
        NS_ASSERT(false);
        break;
    }

    return delay;
}

void
WifiPhy::DoChannelSwitch()
{
    NS_LOG_FUNCTION(this);

    m_powerRestricted = false;
    m_channelAccessRequested = false;

    // Update unspecified parameters with default values
    std::optional<uint8_t> prevChannelNumber{};
    for (auto& [number, width, band, primary20] : m_channelSettings)
    {
        if (band == WIFI_PHY_BAND_UNSPECIFIED)
        {
            band = GetDefaultPhyBand(m_standard);
        }
        if (width == MHz_u{0} && number == 0)
        {
            width = GetDefaultChannelWidth(m_standard, static_cast<WifiPhyBand>(band));
        }
        if (number == 0)
        {
            number =
                WifiPhyOperatingChannel::GetDefaultChannelNumber(width,
                                                                 m_standard,
                                                                 static_cast<WifiPhyBand>(band),
                                                                 prevChannelNumber);
        }
        prevChannelNumber = number;
    }

    // We need to call SetStandard if this is the first time we set a channel or we
    // are changing PHY band. Checking if the new PHY band is different than the
    // previous one covers both cases because initially the PHY band is unspecified
    bool changingPhyBand =
        (static_cast<WifiPhyBand>(std::get<2>(m_channelSettings.front())) != m_band);

    NS_ABORT_MSG_IF(IsInitialized() && m_fixedPhyBand && changingPhyBand,
                    "Trying to change PHY band while prohibited.");

    m_band = static_cast<WifiPhyBand>(std::get<2>(m_channelSettings.front()));

    NS_LOG_DEBUG("switching channel");
    std::vector<FrequencyChannelInfo> segments{};
    std::transform(m_channelSettings.cbegin(),
                   m_channelSettings.cend(),
                   std::back_inserter(segments),
                   [this](const auto& channelTuple) {
                       return FrequencyChannelInfo{std::get<0>(channelTuple),
                                                   MHz_u{0},
                                                   std::get<1>(channelTuple),
                                                   m_band};
                   });
    m_operatingChannel.Set(segments, m_standard);
    m_operatingChannel.SetPrimary20Index(std::get<3>(m_channelSettings.front()));

    // check that the channel width is supported
    if (const auto chWidth = GetChannelWidth();
        (m_maxRadioBw != MHz_u{0}) && (chWidth > m_maxRadioBw))
    {
        // throw an exception instead of using NS_ABORT_MSG for unit testing this code
        throw std::runtime_error("Attempting to set a " + std::to_string(chWidth) +
                                 " MHz channel on a station only supporting " +
                                 std::to_string(m_maxRadioBw) + " MHz operation");
    }

    if (changingPhyBand)
    {
        ConfigureStandard(m_standard);
    }

    FinalizeChannelSwitch();

    if (IsInitialized())
    {
        // notify channel switching
        m_state->SwitchToChannelSwitching(GetChannelSwitchDelay());
        /*
         * Needed here to be able to correctly sensed the medium for the first
         * time after the switching. The actual switching is not performed until
         * after m_channelSwitchDelay. Packets received during the switching
         * state are added to the event list and are employed later to figure
         * out the state of the medium after the switching.
         */
        SwitchMaybeToCcaBusy(nullptr);
    }
}

void
WifiPhy::SetNumberOfAntennas(uint8_t antennas)
{
    NS_LOG_FUNCTION(this << +antennas);
    NS_ASSERT_MSG(antennas > 0 && antennas <= 8, "unsupported number of antennas");
    m_numberOfAntennas = antennas;
    if (m_interference)
    {
        m_interference->SetNumberOfReceiveAntennas(antennas);
    }
}

uint8_t
WifiPhy::GetNumberOfAntennas() const
{
    return m_numberOfAntennas;
}

void
WifiPhy::SetMaxSupportedTxSpatialStreams(uint8_t streams)
{
    NS_ASSERT(streams <= GetNumberOfAntennas());
    bool changed = (m_txSpatialStreams != streams);
    m_txSpatialStreams = streams;
    if (changed)
    {
        auto phyEntity = m_phyEntities.find(WIFI_MOD_CLASS_HT);
        if (phyEntity != m_phyEntities.end())
        {
            Ptr<HtPhy> htPhy = DynamicCast<HtPhy>(phyEntity->second);
            if (htPhy)
            {
                htPhy->SetMaxSupportedNss(
                    m_txSpatialStreams); // this is essential to have the right MCSs configured
            }

            if (!m_capabilitiesChangedCallback.IsNull())
            {
                m_capabilitiesChangedCallback();
            }
        }
    }
}

uint8_t
WifiPhy::GetMaxSupportedTxSpatialStreams() const
{
    return m_txSpatialStreams;
}

void
WifiPhy::SetMaxSupportedRxSpatialStreams(uint8_t streams)
{
    NS_ASSERT(streams <= GetNumberOfAntennas());
    bool changed = (m_rxSpatialStreams != streams);
    m_rxSpatialStreams = streams;
    if (changed && !m_capabilitiesChangedCallback.IsNull())
    {
        m_capabilitiesChangedCallback();
    }
}

uint8_t
WifiPhy::GetMaxSupportedRxSpatialStreams() const
{
    return m_rxSpatialStreams;
}

std::list<uint8_t>
WifiPhy::GetBssMembershipSelectorList() const
{
    std::list<uint8_t> list;
    for (const auto& phyEntity : m_phyEntities)
    {
        Ptr<HtPhy> htPhy = DynamicCast<HtPhy>(phyEntity.second);
        if (htPhy)
        {
            list.emplace_back(htPhy->GetBssMembershipSelector());
        }
    }
    return list;
}

void
WifiPhy::SetSleepMode(bool forceSleepInRx)
{
    NS_LOG_FUNCTION(this);
    m_powerRestricted = false;
    m_channelAccessRequested = false;
    switch (m_state->GetState())
    {
    case WifiPhyState::TX:
        NS_LOG_DEBUG("setting sleep mode postponed until end of current transmission");
        Simulator::Schedule(GetDelayUntilIdle(), &WifiPhy::SetSleepMode, this, forceSleepInRx);
        break;
    case WifiPhyState::RX:
        NS_LOG_DEBUG("setting sleep mode"
                     << (forceSleepInRx ? "" : "postponed until end of current reception"));
        if (forceSleepInRx)
        {
            AbortCurrentReception(WifiPhyRxfailureReason::SLEEPING);
            m_state->SwitchToSleep();
        }
        else
        {
            Simulator::Schedule(GetDelayUntilIdle(), &WifiPhy::SetSleepMode, this, forceSleepInRx);
        }
        break;
    case WifiPhyState::SWITCHING:
        NS_LOG_DEBUG("setting sleep mode postponed until end of channel switching");
        Simulator::Schedule(GetDelayUntilIdle(), &WifiPhy::SetSleepMode, this, forceSleepInRx);
        break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
        NS_LOG_DEBUG("setting sleep mode");
        // The PHY object may be in CCA_BUSY state because it is receiving a preamble. Cancel
        // preamble events before switching to sleep state
        Reset();
        m_state->SwitchToSleep();
        break;
    case WifiPhyState::SLEEP:
        NS_LOG_DEBUG("already in sleep mode");
        break;
    default:
        NS_ASSERT(false);
        break;
    }
}

void
WifiPhy::SetOffMode()
{
    NS_LOG_FUNCTION(this);
    m_powerRestricted = false;
    m_channelAccessRequested = false;
    Reset();
    m_state->SwitchToOff();
}

void
WifiPhy::ResumeFromSleep()
{
    NS_LOG_FUNCTION(this);
    switch (m_state->GetState())
    {
    case WifiPhyState::TX:
    case WifiPhyState::RX:
    case WifiPhyState::IDLE:
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::SWITCHING: {
        NS_LOG_DEBUG("not in sleep mode, there is nothing to resume");
        break;
    }
    case WifiPhyState::SLEEP: {
        NS_LOG_DEBUG("resuming from sleep mode");
        m_state->SwitchFromSleep();
        SwitchMaybeToCcaBusy();
        break;
    }
    default: {
        NS_ASSERT(false);
        break;
    }
    }
}

void
WifiPhy::ResumeFromOff()
{
    NS_LOG_FUNCTION(this);
    switch (m_state->GetState())
    {
    case WifiPhyState::TX:
    case WifiPhyState::RX:
    case WifiPhyState::IDLE:
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::SWITCHING:
    case WifiPhyState::SLEEP: {
        NS_LOG_DEBUG("not in off mode, there is nothing to resume");
        break;
    }
    case WifiPhyState::OFF: {
        NS_LOG_DEBUG("resuming from off mode");
        m_state->SwitchFromOff();
        SwitchMaybeToCcaBusy();
        break;
    }
    default: {
        NS_ASSERT(false);
        break;
    }
    }
}

Time
WifiPhy::GetPreambleDetectionDuration()
{
    return MicroSeconds(4);
}

Time
WifiPhy::GetStartOfPacketDuration(const WifiTxVector& txVector)
{
    return MicroSeconds(4);
}

Time
WifiPhy::GetPayloadDuration(uint32_t size,
                            const WifiTxVector& txVector,
                            WifiPhyBand band,
                            MpduType mpdutype,
                            uint16_t staId)
{
    uint32_t totalAmpduSize;
    double totalAmpduNumSymbols;
    return GetPayloadDuration(size,
                              txVector,
                              band,
                              mpdutype,
                              false,
                              totalAmpduSize,
                              totalAmpduNumSymbols,
                              staId);
}

Time
WifiPhy::GetPayloadDuration(uint32_t size,
                            const WifiTxVector& txVector,
                            WifiPhyBand band,
                            MpduType mpdutype,
                            bool incFlag,
                            uint32_t& totalAmpduSize,
                            double& totalAmpduNumSymbols,
                            uint16_t staId)
{
    return GetStaticPhyEntity(txVector.GetModulationClass())
        ->GetPayloadDuration(size,
                             txVector,
                             band,
                             mpdutype,
                             incFlag,
                             totalAmpduSize,
                             totalAmpduNumSymbols,
                             staId);
}

Time
WifiPhy::CalculatePhyPreambleAndHeaderDuration(const WifiTxVector& txVector)
{
    return GetStaticPhyEntity(txVector.GetModulationClass())
        ->CalculatePhyPreambleAndHeaderDuration(txVector);
}

Time
WifiPhy::CalculateTxDuration(uint32_t size,
                             const WifiTxVector& txVector,
                             WifiPhyBand band,
                             uint16_t staId)
{
    NS_ASSERT(txVector.IsValid(band));
    Time duration = CalculatePhyPreambleAndHeaderDuration(txVector) +
                    GetPayloadDuration(size, txVector, band, NORMAL_MPDU, staId);
    NS_ASSERT(duration.IsStrictlyPositive());
    return duration;
}

Time
WifiPhy::CalculateTxDuration(Ptr<const WifiPsdu> psdu,
                             const WifiTxVector& txVector,
                             WifiPhyBand band)
{
    return CalculateTxDuration(GetWifiConstPsduMap(psdu, txVector), txVector, band);
}

Time
WifiPhy::CalculateTxDuration(const WifiConstPsduMap& psduMap,
                             const WifiTxVector& txVector,
                             WifiPhyBand band)
{
    NS_ASSERT(txVector.IsValid(band));
    return GetStaticPhyEntity(txVector.GetModulationClass())
        ->CalculateTxDuration(psduMap, txVector, band);
}

uint32_t
WifiPhy::GetMaxPsduSize(WifiModulationClass modulation)
{
    return GetStaticPhyEntity(modulation)->GetMaxPsduSize();
}

void
WifiPhy::NotifyTxBegin(const WifiConstPsduMap& psdus, Watt_u txPower)
{
    if (!m_phyTxBeginTrace.IsEmpty())
    {
        for (const auto& psdu : psdus)
        {
            for (auto& mpdu : *PeekPointer(psdu.second))
            {
                m_phyTxBeginTrace(mpdu->GetProtocolDataUnit(), txPower);
            }
        }
    }
}

void
WifiPhy::NotifyTxEnd(const WifiConstPsduMap& psdus)
{
    if (!m_phyTxEndTrace.IsEmpty())
    {
        for (const auto& psdu : psdus)
        {
            for (auto& mpdu : *PeekPointer(psdu.second))
            {
                m_phyTxEndTrace(mpdu->GetProtocolDataUnit());
            }
        }
    }
}

void
WifiPhy::NotifyTxDrop(Ptr<const WifiPsdu> psdu)
{
    if (!m_phyTxDropTrace.IsEmpty())
    {
        for (auto& mpdu : *PeekPointer(psdu))
        {
            m_phyTxDropTrace(mpdu->GetProtocolDataUnit());
        }
    }
}

void
WifiPhy::NotifyRxBegin(Ptr<const WifiPsdu> psdu, const RxPowerWattPerChannelBand& rxPowersW)
{
    if (psdu && !m_phyRxBeginTrace.IsEmpty())
    {
        for (auto& mpdu : *PeekPointer(psdu))
        {
            m_phyRxBeginTrace(mpdu->GetProtocolDataUnit(), rxPowersW);
        }
    }
}

void
WifiPhy::NotifyRxEnd(Ptr<const WifiPsdu> psdu)
{
    if (psdu && !m_phyRxEndTrace.IsEmpty())
    {
        for (auto& mpdu : *PeekPointer(psdu))
        {
            m_phyRxEndTrace(mpdu->GetProtocolDataUnit());
        }
    }
}

void
WifiPhy::NotifyRxDrop(Ptr<const WifiPsdu> psdu, WifiPhyRxfailureReason reason)
{
    if (psdu && !m_phyRxDropTrace.IsEmpty())
    {
        for (auto& mpdu : *PeekPointer(psdu))
        {
            m_phyRxDropTrace(mpdu->GetProtocolDataUnit(), reason);
        }
    }
}

void
WifiPhy::NotifyRxPpduDrop(Ptr<const WifiPpdu> ppdu, WifiPhyRxfailureReason reason)
{
    NotifyRxDrop(GetAddressedPsduInPpdu(ppdu), reason);
    m_phyRxPpduDropTrace(ppdu, reason);
}

void
WifiPhy::NotifyMonitorSniffRx(Ptr<const WifiPsdu> psdu,
                              MHz_u channelFreq,
                              const WifiTxVector& txVector,
                              SignalNoiseDbm signalNoise,
                              const std::vector<bool>& statusPerMpdu,
                              uint16_t staId)
{
    MpduInfo aMpdu;
    if (psdu->IsAggregate())
    {
        // Expand A-MPDU
        NS_ASSERT_MSG(txVector.IsAggregation(),
                      "TxVector with aggregate flag expected here according to PSDU");
        aMpdu.mpduRefNumber = ++m_rxMpduReferenceNumber;
        size_t nMpdus = psdu->GetNMpdus();
        NS_ASSERT_MSG(statusPerMpdu.size() == nMpdus, "Should have one reception status per MPDU");
        if (!m_phyMonitorSniffRxTrace.IsEmpty())
        {
            aMpdu.type = (psdu->IsSingle()) ? SINGLE_MPDU : FIRST_MPDU_IN_AGGREGATE;
            for (size_t i = 0; i < nMpdus;)
            {
                if (statusPerMpdu.at(i)) // packet received without error, hand over to sniffer
                {
                    m_phyMonitorSniffRxTrace(psdu->GetAmpduSubframe(i),
                                             static_cast<uint16_t>(channelFreq),
                                             txVector,
                                             aMpdu,
                                             signalNoise,
                                             staId);
                }
                ++i;
                aMpdu.type =
                    (i == (nMpdus - 1)) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
            }
        }
    }
    else
    {
        NS_ASSERT_MSG(statusPerMpdu.size() == 1,
                      "Should have one reception status for normal MPDU");
        if (!m_phyMonitorSniffRxTrace.IsEmpty())
        {
            aMpdu.type = NORMAL_MPDU;
            m_phyMonitorSniffRxTrace(psdu->GetPacket(),
                                     static_cast<uint16_t>(channelFreq),
                                     txVector,
                                     aMpdu,
                                     signalNoise,
                                     staId);
        }
    }
}

void
WifiPhy::NotifyMonitorSniffTx(Ptr<const WifiPsdu> psdu,
                              MHz_u channelFreq,
                              const WifiTxVector& txVector,
                              uint16_t staId)
{
    MpduInfo aMpdu;
    if (psdu->IsAggregate())
    {
        // Expand A-MPDU
        NS_ASSERT_MSG(txVector.IsAggregation(),
                      "TxVector with aggregate flag expected here according to PSDU");
        aMpdu.mpduRefNumber = ++m_rxMpduReferenceNumber;
        if (!m_phyMonitorSniffTxTrace.IsEmpty())
        {
            size_t nMpdus = psdu->GetNMpdus();
            aMpdu.type = (psdu->IsSingle()) ? SINGLE_MPDU : FIRST_MPDU_IN_AGGREGATE;
            for (size_t i = 0; i < nMpdus;)
            {
                m_phyMonitorSniffTxTrace(psdu->GetAmpduSubframe(i),
                                         channelFreq,
                                         txVector,
                                         aMpdu,
                                         staId);
                ++i;
                aMpdu.type =
                    (i == (nMpdus - 1)) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
            }
        }
    }
    else
    {
        if (!m_phyMonitorSniffTxTrace.IsEmpty())
        {
            aMpdu.type = NORMAL_MPDU;
            m_phyMonitorSniffTxTrace(psdu->GetPacket(), channelFreq, txVector, aMpdu, staId);
        }
    }
}

std::optional<Time>
WifiPhy::GetTimeToPreambleDetectionEnd() const
{
    for (const auto& [modClass, phyEntity] : m_phyEntities)
    {
        if (auto remTime = phyEntity->GetTimeToPreambleDetectionEnd())
        {
            return remTime;
        }
    }
    return std::nullopt;
}

std::optional<Time>
WifiPhy::GetTimeToMacHdrEnd(uint16_t staId) const
{
    for (auto& [modClass, phyEntity] : m_phyEntities)
    {
        if (auto remTime = phyEntity->GetTimeToMacHdrEnd(staId))
        {
            return remTime;
        }
    }
    return std::nullopt;
}

WifiConstPsduMap
WifiPhy::GetWifiConstPsduMap(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    return GetStaticPhyEntity(txVector.GetModulationClass())->GetWifiConstPsduMap(psdu, txVector);
}

void
WifiPhy::Send(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *psdu << txVector);
    Send(GetWifiConstPsduMap(psdu, txVector), txVector);
}

void
WifiPhy::Send(const WifiConstPsduMap& psdus, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdus << txVector);
    /* Transmission can happen if:
     *  - we are syncing on a packet. It is the responsibility of the
     *    MAC layer to avoid doing this but the PHY does nothing to
     *    prevent it.
     *  - we are idle
     */
    NS_ASSERT(!m_state->IsStateTx() && !m_state->IsStateSwitching());
    NS_ASSERT(m_endTxEvent.IsExpired());

    if (!txVector.IsValid(m_band))
    {
        NS_FATAL_ERROR("TX-VECTOR is invalid!");
    }

    uint8_t nss = 0;
    if (txVector.IsMu())
    {
        // We do not support mixed OFDMA and MU-MIMO
        if (txVector.IsDlMuMimo())
        {
            nss = txVector.GetNssTotal();
        }
        else
        {
            nss = txVector.GetNssMax();
        }
    }
    else
    {
        nss = txVector.GetNss();
    }

    if (nss > GetMaxSupportedTxSpatialStreams())
    {
        NS_FATAL_ERROR("Unsupported number of spatial streams!");
    }

    if (m_state->IsStateSleep())
    {
        NS_LOG_DEBUG("Dropping packet because in sleep mode");
        for (const auto& psdu : psdus)
        {
            NotifyTxDrop(psdu.second);
        }
        return;
    }

    const auto txDuration = CalculateTxDuration(psdus, txVector, GetPhyBand());

    if (const auto timeToPreambleDetectionEnd = GetTimeToPreambleDetectionEnd();
        timeToPreambleDetectionEnd && !m_currentEvent)
    {
        // PHY is in the initial few microseconds during which the
        // start of RX has occurred but the preamble detection period
        // has not elapsed
        AbortCurrentReception(SIGNAL_DETECTION_ABORTED_BY_TX);
    }
    else if (timeToPreambleDetectionEnd || m_currentEvent)
    {
        AbortCurrentReception(RECEPTION_ABORTED_BY_TX);
    }

    if (m_powerRestricted)
    {
        NS_LOG_DEBUG("Transmitting with power restriction for " << txDuration.As(Time::NS));
    }
    else
    {
        NS_LOG_DEBUG("Transmitting without power restriction for " << txDuration.As(Time::NS));
    }

    if (m_state->GetState() == WifiPhyState::OFF)
    {
        NS_LOG_DEBUG("Transmission canceled because device is OFF");
        return;
    }

    auto ppdu = GetPhyEntity(txVector.GetModulationClass())->BuildPpdu(psdus, txVector, txDuration);
    m_previouslyRxPpduUid = UINT64_MAX; // reset (after creation of PPDU) to use it only once

    const auto txPower = DbmToW(GetTxPowerForTransmission(ppdu) + GetTxGain());
    NotifyTxBegin(psdus, txPower);
    if (!m_phyTxPsduBeginTrace.IsEmpty())
    {
        m_phyTxPsduBeginTrace(psdus, txVector, txPower);
    }
    for (const auto& psdu : psdus)
    {
        NotifyMonitorSniffTx(psdu.second, GetFrequency(), txVector, psdu.first);
    }
    m_state->SwitchToTx(txDuration, psdus, GetPower(txVector.GetTxPowerLevel()), txVector);

    if (m_wifiRadioEnergyModel &&
        m_wifiRadioEnergyModel->GetMaximumTimeInState(WifiPhyState::TX) < txDuration)
    {
        ppdu->SetTruncatedTx();
    }

    m_endTxEvent =
        Simulator::Schedule(txDuration, &WifiPhy::TxDone, this, psdus); // TODO: fix for MU

    StartTx(ppdu);
    ppdu->ResetTxVector();

    m_channelAccessRequested = false;
    m_powerRestricted = false;
}

void
WifiPhy::TxDone(const WifiConstPsduMap& psdus)
{
    NS_LOG_FUNCTION(this << psdus);
    NotifyTxEnd(psdus);
    Reset();
    // we might have received signals during TX
    SwitchMaybeToCcaBusy();
}

uint64_t
WifiPhy::GetPreviouslyRxPpduUid() const
{
    return m_previouslyRxPpduUid;
}

void
WifiPhy::SetPreviouslyRxPpduUid(uint64_t uid)
{
    NS_ASSERT(m_standard >= WIFI_STANDARD_80211be);
    m_previouslyRxPpduUid = uid;
}

void
WifiPhy::Reset()
{
    NS_LOG_FUNCTION(this);
    m_currentPreambleEvents.clear();
    if (m_interference && (m_currentEvent || GetTimeToPreambleDetectionEnd()))
    {
        m_interference->NotifyRxEnd(Simulator::Now(), GetCurrentFrequencyRange());
    }
    m_currentEvent = nullptr;
    for (auto& phyEntity : m_phyEntities)
    {
        phyEntity.second->CancelAllEvents();
    }
    m_endPhyRxEvent.Cancel();
    m_endTxEvent.Cancel();
}

void
WifiPhy::StartReceivePreamble(Ptr<const WifiPpdu> ppdu,
                              RxPowerWattPerChannelBand& rxPowersW,
                              Time rxDuration)
{
    NS_LOG_FUNCTION(this << ppdu << rxDuration);
    WifiModulationClass modulation = ppdu->GetModulation();
    NS_ASSERT(m_maxModClassSupported != WIFI_MOD_CLASS_UNKNOWN);
    if (auto it = m_phyEntities.find(modulation);
        it != m_phyEntities.end() && modulation <= m_maxModClassSupported)
    {
        it->second->StartReceivePreamble(ppdu, rxPowersW, rxDuration);
    }
    else
    {
        // TODO find a fallback PHY for receiving the PPDU (e.g. 11a for 11ax due to preamble
        // structure)
        NS_LOG_DEBUG("Unsupported modulation received (" << modulation << "), consider as noise");
        m_interference->Add(ppdu, rxDuration, rxPowersW, GetCurrentFrequencyRange());
        SwitchMaybeToCcaBusy();
    }
}

std::optional<std::reference_wrapper<const WifiTxVector>>
WifiPhy::GetInfoIfRxingPhyHeader() const
{
    if (m_endPhyRxEvent.IsPending())
    {
        NS_ASSERT_MSG(m_currentEvent, "No current event while receiving PHY header");
        return std::cref(m_currentEvent->GetPpdu()->GetTxVector());
    }
    return std::nullopt;
}

void
WifiPhy::EndReceiveInterBss()
{
    NS_LOG_FUNCTION(this);
    if (!m_channelAccessRequested)
    {
        m_powerRestricted = false;
    }
}

void
WifiPhy::NotifyChannelAccessRequested()
{
    NS_LOG_FUNCTION(this);
    m_channelAccessRequested = true;
}

bool
WifiPhy::IsModeSupported(WifiMode mode) const
{
    for (const auto& phyEntity : m_phyEntities)
    {
        if (phyEntity.second->IsModeSupported(mode))
        {
            return true;
        }
    }
    return false;
}

WifiMode
WifiPhy::GetDefaultMode() const
{
    // Start from oldest standards and move up (guaranteed by fact that WifModulationClass is
    // ordered)
    for (const auto& phyEntity : m_phyEntities)
    {
        for (const auto& mode : *(phyEntity.second))
        {
            return mode;
        }
    }
    NS_ASSERT_MSG(false, "Should have found at least one default mode");
    return WifiMode();
}

bool
WifiPhy::IsMcsSupported(WifiModulationClass modulation, uint8_t mcs) const
{
    const auto phyEntity = m_phyEntities.find(modulation);
    if (phyEntity == m_phyEntities.end())
    {
        return false;
    }
    return phyEntity->second->IsMcsSupported(mcs);
}

std::list<WifiMode>
WifiPhy::GetModeList() const
{
    std::list<WifiMode> list;
    for (const auto& phyEntity : m_phyEntities)
    {
        if (!phyEntity.second->HandlesMcsModes()) // to exclude MCSs from search
        {
            for (const auto& mode : *(phyEntity.second))
            {
                list.emplace_back(mode);
            }
        }
    }
    return list;
}

std::list<WifiMode>
WifiPhy::GetModeList(WifiModulationClass modulation) const
{
    std::list<WifiMode> list;
    const auto phyEntity = m_phyEntities.find(modulation);
    if (phyEntity != m_phyEntities.end())
    {
        if (!phyEntity->second->HandlesMcsModes()) // to exclude MCSs from search
        {
            for (const auto& mode : *(phyEntity->second))
            {
                list.emplace_back(mode);
            }
        }
    }
    return list;
}

uint16_t
WifiPhy::GetNMcs() const
{
    uint16_t numMcs = 0;
    for (const auto& phyEntity : m_phyEntities)
    {
        if (phyEntity.second->HandlesMcsModes()) // to exclude non-MCS modes from search
        {
            numMcs += phyEntity.second->GetNumModes();
        }
    }
    return numMcs;
}

std::list<WifiMode>
WifiPhy::GetMcsList() const
{
    std::list<WifiMode> list;
    for (const auto& phyEntity : m_phyEntities)
    {
        if (phyEntity.second->HandlesMcsModes()) // to exclude non-MCS modes from search
        {
            for (const auto& mode : *(phyEntity.second))
            {
                list.emplace_back(mode);
            }
        }
    }
    return list;
}

std::list<WifiMode>
WifiPhy::GetMcsList(WifiModulationClass modulation) const
{
    std::list<WifiMode> list;
    auto phyEntity = m_phyEntities.find(modulation);
    if (phyEntity != m_phyEntities.end())
    {
        if (phyEntity->second->HandlesMcsModes()) // to exclude non-MCS modes from search
        {
            for (const auto& mode : *(phyEntity->second))
            {
                list.emplace_back(mode);
            }
        }
    }
    return list;
}

WifiMode
WifiPhy::GetMcs(WifiModulationClass modulation, uint8_t mcs) const
{
    NS_ASSERT_MSG(IsMcsSupported(modulation, mcs), "Unsupported MCS");
    return m_phyEntities.at(modulation)->GetMcs(mcs);
}

bool
WifiPhy::IsStateCcaBusy() const
{
    return m_state->IsStateCcaBusy();
}

bool
WifiPhy::IsStateIdle() const
{
    return m_state->IsStateIdle();
}

bool
WifiPhy::IsStateRx() const
{
    return m_state->IsStateRx();
}

bool
WifiPhy::IsStateTx() const
{
    return m_state->IsStateTx();
}

bool
WifiPhy::IsStateSwitching() const
{
    return m_state->IsStateSwitching();
}

bool
WifiPhy::IsStateSleep() const
{
    return m_state->IsStateSleep();
}

bool
WifiPhy::IsStateOff() const
{
    return m_state->IsStateOff();
}

Time
WifiPhy::GetDelayUntilIdle()
{
    return m_state->GetDelayUntilIdle();
}

Time
WifiPhy::GetLastRxStartTime() const
{
    return m_state->GetLastRxStartTime();
}

Time
WifiPhy::GetLastRxEndTime() const
{
    return m_state->GetLastRxEndTime();
}

void
WifiPhy::SwitchMaybeToCcaBusy(const Ptr<const WifiPpdu> ppdu /* = nullptr */)
{
    NS_LOG_FUNCTION(this);
    GetLatestPhyEntity()->SwitchMaybeToCcaBusy(ppdu);
}

void
WifiPhy::NotifyCcaBusy(const Ptr<const WifiPpdu> ppdu, Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    GetLatestPhyEntity()->NotifyCcaBusy(ppdu, duration, WIFI_CHANLIST_PRIMARY);
}

void
WifiPhy::AbortCurrentReception(WifiPhyRxfailureReason reason)
{
    NS_LOG_FUNCTION(this << reason);
    if (reason != OBSS_PD_CCA_RESET ||
        m_currentEvent) // Otherwise abort has already been called previously
    {
        if (reason == SIGNAL_DETECTION_ABORTED_BY_TX)
        {
            for (auto signalDetectEvent : m_currentPreambleEvents)
            {
                NotifyRxPpduDrop(signalDetectEvent.second->GetPpdu(),
                                 SIGNAL_DETECTION_ABORTED_BY_TX);
            }
        }
        for (auto& phyEntity : m_phyEntities)
        {
            phyEntity.second->CancelAllEvents();
        }
        m_endPhyRxEvent.Cancel();
        m_interference->NotifyRxEnd(Simulator::Now(), GetCurrentFrequencyRange());
        if (!m_currentEvent)
        {
            return;
        }
        NotifyRxPpduDrop(m_currentEvent->GetPpdu(), reason);
        if (reason == OBSS_PD_CCA_RESET)
        {
            m_state->SwitchFromRxAbort(GetChannelWidth());
        }
        if (reason == RECEPTION_ABORTED_BY_TX)
        {
            Reset();
        }
        else
        {
            for (auto it = m_currentPreambleEvents.begin(); it != m_currentPreambleEvents.end();
                 ++it)
            {
                if (it->second == m_currentEvent)
                {
                    it = m_currentPreambleEvents.erase(it);
                    break;
                }
            }
            m_currentEvent = nullptr;
        }
    }
}

void
WifiPhy::ResetCca(bool powerRestricted, dBm_u txPowerMaxSiso, dBm_u txPowerMaxMimo)
{
    NS_LOG_FUNCTION(this << powerRestricted << txPowerMaxSiso << txPowerMaxMimo);
    // This method might be called multiple times when receiving TB PPDUs with a BSS color
    // different than the one of the receiver. The first time this method is called, the call
    // to AbortCurrentReception sets m_currentEvent to 0. Therefore, we need to check whether
    // m_currentEvent is not 0 before executing the instructions below.
    if (m_currentEvent)
    {
        m_powerRestricted = powerRestricted;
        m_txPowerMaxSiso = txPowerMaxSiso;
        m_txPowerMaxMimo = txPowerMaxMimo;
        NS_ASSERT((m_currentEvent->GetEndTime() - Simulator::Now()).IsPositive());
        Simulator::Schedule(m_currentEvent->GetEndTime() - Simulator::Now(),
                            &WifiPhy::EndReceiveInterBss,
                            this);
        Simulator::ScheduleNow(&WifiPhy::AbortCurrentReception,
                               this,
                               OBSS_PD_CCA_RESET); // finish processing field first
    }
}

dBm_u
WifiPhy::GetTxPowerForTransmission(Ptr<const WifiPpdu> ppdu) const
{
    NS_LOG_FUNCTION(this << m_powerRestricted << ppdu);
    const auto& txVector = ppdu->GetTxVector();
    // Get transmit power before antenna gain
    dBm_u txPower;
    if (!m_powerRestricted)
    {
        txPower = GetPower(txVector.GetTxPowerLevel());
    }
    else
    {
        if (txVector.GetNssMax() > 1 || txVector.GetNssTotal() > 1)
        {
            txPower = std::min(m_txPowerMaxMimo, GetPower(txVector.GetTxPowerLevel()));
        }
        else
        {
            txPower = std::min(m_txPowerMaxSiso, GetPower(txVector.GetTxPowerLevel()));
        }
    }

    // Apply power density constraint on EIRP
    const auto channelWidth = ppdu->GetTxChannelWidth();
    dBm_per_MHz_u txPowerDbmPerMhz =
        (txPower + GetTxGain()) - RatioToDb(channelWidth); // account for antenna gain since EIRP
    NS_LOG_INFO("txPower=" << txPower << "dBm with txPowerDbmPerMhz=" << txPowerDbmPerMhz
                           << " over " << channelWidth << " MHz");
    txPower = std::min(txPowerDbmPerMhz, m_powerDensityLimit) + RatioToDb(channelWidth);
    txPower -= GetTxGain(); // remove antenna gain since will be added right afterwards
    NS_LOG_INFO("txPower=" << txPower
                           << "dBm after applying m_powerDensityLimit=" << m_powerDensityLimit);
    return txPower;
}

Ptr<const WifiPsdu>
WifiPhy::GetAddressedPsduInPpdu(Ptr<const WifiPpdu> ppdu) const
{
    // TODO: wrapper. See if still needed
    return GetPhyEntityForPpdu(ppdu)->GetAddressedPsduInPpdu(ppdu);
}

int64_t
WifiPhy::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    int64_t currentStream = stream;
    m_random->SetStream(currentStream++);
    currentStream += m_interference->GetErrorRateModel()->AssignStreams(currentStream);
    return (currentStream - stream);
}

std::ostream&
operator<<(std::ostream& os, RxSignalInfo rxSignalInfo)
{
    os << "SNR:" << RatioToDb(rxSignalInfo.snr) << " dB"
       << ", RSSI:" << rxSignalInfo.rssi << " dBm";
    return os;
}

uint8_t
WifiPhy::GetPrimaryChannelNumber(MHz_u primaryChannelWidth) const
{
    return m_operatingChannel.GetPrimaryChannelNumber(primaryChannelWidth, m_standard);
}

Hz_u
WifiPhy::GetSubcarrierSpacing() const
{
    Hz_u subcarrierSpacing{0};
    switch (GetStandard())
    {
    case WIFI_STANDARD_80211a:
    case WIFI_STANDARD_80211g:
    case WIFI_STANDARD_80211b:
    case WIFI_STANDARD_80211n:
    case WIFI_STANDARD_80211ac:
        subcarrierSpacing = SUBCARRIER_FREQUENCY_SPACING;
        break;
    case WIFI_STANDARD_80211p:
        if (GetChannelWidth() == MHz_u{5})
        {
            subcarrierSpacing = SUBCARRIER_FREQUENCY_SPACING / 4;
        }
        else
        {
            subcarrierSpacing = SUBCARRIER_FREQUENCY_SPACING / 2;
        }
        break;
    case WIFI_STANDARD_80211ax:
    case WIFI_STANDARD_80211be:
        subcarrierSpacing = SUBCARRIER_FREQUENCY_SPACING_HE;
        break;
    default:
        NS_FATAL_ERROR("Standard unknown: " << GetStandard());
        break;
    }
    return subcarrierSpacing;
}

} // namespace ns3
