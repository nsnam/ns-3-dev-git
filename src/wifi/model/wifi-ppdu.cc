/*
 * Copyright (c) 2019 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 */

#include "wifi-ppdu.h"

#include "wifi-phy-operating-channel.h"
#include "wifi-psdu.h"

#include "ns3/log.h"

namespace
{
/**
 * Get the center frequency of each segment covered by the provided channel width. If the specified
 * channel width is contained in a single frequency segment, a single center frequency is returned.
 * If the specified channel width is spread over multiple frequency segments (e.g. 160 MHz if
 * operating channel is 80+80MHz), multiple center frequencies are returned.
 *
 * @param channel the operating channel of the PHY
 * @param channelWidth the channel width
 * @return the center frequency of each segment covered by the given width
 */
std::vector<ns3::MHz_u>
GetChannelCenterFrequenciesPerSegment(const ns3::WifiPhyOperatingChannel& channel,
                                      ns3::MHz_u channelWidth)
{
    if (!channel.IsSet())
    {
        return {};
    }
    std::vector<ns3::MHz_u> freqs{};
    const auto width = std::min(channelWidth, channel.GetWidth(0));
    const auto primarySegmentIndex = channel.GetPrimarySegmentIndex(width);
    const auto secondarySegmentIndex = channel.GetSecondarySegmentIndex(width);
    const auto primaryIndex = channel.GetPrimaryChannelIndex(channelWidth);
    const auto segmentIndices =
        ((channel.GetNSegments() < 2) || (channelWidth <= channel.GetWidth(primarySegmentIndex)))
            ? std::vector<uint8_t>{primarySegmentIndex}
            : std::vector<uint8_t>{primarySegmentIndex, secondarySegmentIndex};
    for (auto segmentIndex : segmentIndices)
    {
        const auto segmentFrequency = channel.GetFrequency(segmentIndex);
        const auto segmentWidth = channel.GetWidth(segmentIndex);
        // segmentOffset has to be an (unsigned) integer to ensure correct calculation
        const uint8_t segmentOffset = (primarySegmentIndex * (segmentWidth / channelWidth));
        const auto freq =
            segmentFrequency - (segmentWidth / 2.) + (primaryIndex - segmentOffset + 0.5) * width;
        freqs.push_back(freq);
    }
    return freqs;
}
} // namespace

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiPpdu");

WifiPpdu::WifiPpdu(Ptr<const WifiPsdu> psdu,
                   const WifiTxVector& txVector,
                   const WifiPhyOperatingChannel& channel,
                   uint64_t uid /* = UINT64_MAX */)
    : m_preamble(txVector.GetPreambleType()),
      m_modulation(txVector.IsValid() ? txVector.GetModulationClass() : WIFI_MOD_CLASS_UNKNOWN),
      m_txCenterFreqs(GetChannelCenterFrequenciesPerSegment(channel, txVector.GetChannelWidth())),
      m_uid(uid),
      m_txVector(txVector),
      m_operatingChannel(channel),
      m_truncatedTx(false),
      m_txPowerLevel(txVector.GetTxPowerLevel()),
      m_txAntennas(txVector.GetNTx()),
      m_txChannelWidth(txVector.GetChannelWidth())
{
    NS_LOG_FUNCTION(this << *psdu << txVector << channel << uid);
    m_psdus.insert(std::make_pair(SU_STA_ID, psdu));
}

WifiPpdu::WifiPpdu(const WifiConstPsduMap& psdus,
                   const WifiTxVector& txVector,
                   const WifiPhyOperatingChannel& channel,
                   uint64_t uid)
    : m_preamble(txVector.GetPreambleType()),
      m_modulation(txVector.IsValid() ? txVector.GetMode(psdus.begin()->first).GetModulationClass()
                                      : WIFI_MOD_CLASS_UNKNOWN),
      m_txCenterFreqs(GetChannelCenterFrequenciesPerSegment(channel, txVector.GetChannelWidth())),
      m_uid(uid),
      m_txVector(txVector),
      m_operatingChannel(channel),
      m_truncatedTx(false),
      m_txPowerLevel(txVector.GetTxPowerLevel()),
      m_txAntennas(txVector.GetNTx()),
      m_txChannelWidth(txVector.GetChannelWidth())
{
    NS_LOG_FUNCTION(this << psdus << txVector << channel << uid);
    m_psdus = psdus;
}

const WifiTxVector&
WifiPpdu::GetTxVector() const
{
    if (!m_txVector.has_value())
    {
        m_txVector = DoGetTxVector();
        m_txVector->SetTxPowerLevel(m_txPowerLevel);
        m_txVector->SetNTx(m_txAntennas);
        m_txVector->SetChannelWidth(m_txChannelWidth);
    }
    return m_txVector.value();
}

WifiTxVector
WifiPpdu::DoGetTxVector() const
{
    NS_FATAL_ERROR("This method should not be called for the base WifiPpdu class. Use the "
                   "overloaded version in the amendment-specific PPDU subclasses instead!");
    return WifiTxVector(); // should be overloaded
}

void
WifiPpdu::ResetTxVector() const
{
    NS_LOG_FUNCTION(this);
    m_txVector.reset();
}

void
WifiPpdu::UpdateTxVector(const WifiTxVector& updatedTxVector) const
{
    NS_LOG_FUNCTION(this << updatedTxVector);
    ResetTxVector();
    m_txVector = updatedTxVector;
}

Ptr<const WifiPsdu>
WifiPpdu::GetPsdu() const
{
    return m_psdus.begin()->second;
}

bool
WifiPpdu::IsTruncatedTx() const
{
    return m_truncatedTx;
}

void
WifiPpdu::SetTruncatedTx()
{
    NS_LOG_FUNCTION(this);
    m_truncatedTx = true;
}

WifiModulationClass
WifiPpdu::GetModulation() const
{
    return m_modulation;
}

MHz_u
WifiPpdu::GetTxChannelWidth() const
{
    return m_txChannelWidth;
}

std::vector<MHz_u>
WifiPpdu::GetTxCenterFreqs() const
{
    return m_txCenterFreqs;
}

bool
WifiPpdu::DoesOverlapChannel(MHz_u minFreq, MHz_u maxFreq) const
{
    NS_LOG_FUNCTION(this << minFreq << maxFreq);
    // all segments have the same width
    const MHz_u txChannelWidth = (m_txChannelWidth / m_txCenterFreqs.size());
    for (auto txCenterFreq : m_txCenterFreqs)
    {
        const auto minTxFreq = txCenterFreq - txChannelWidth / 2;
        const auto maxTxFreq = txCenterFreq + txChannelWidth / 2;
        /**
         * The PPDU does not overlap the channel in two cases.
         *
         * First non-overlapping case:
         *
         *                                        ┌─────────┐
         *                                PPDU    │ Nominal │
         *                                        │  Band   │
         *                                        └─────────┘
         *                                   minTxFreq   maxTxFreq
         *
         *       minFreq                       maxFreq
         *         ┌──────────────────────────────┐
         *         │           Channel            │
         *         └──────────────────────────────┘
         *
         * Second non-overlapping case:
         *
         *         ┌─────────┐
         * PPDU    │ Nominal │
         *         │  Band   │
         *         └─────────┘
         *    minTxFreq   maxTxFreq
         *
         *                 minFreq                       maxFreq
         *                   ┌──────────────────────────────┐
         *                   │           Channel            │
         *                   └──────────────────────────────┘
         */
        if ((minTxFreq < maxFreq) && (maxTxFreq > minFreq))
        {
            return true;
        }
    }
    return false;
}

uint64_t
WifiPpdu::GetUid() const
{
    return m_uid;
}

WifiPreamble
WifiPpdu::GetPreamble() const
{
    return m_preamble;
}

WifiPpduType
WifiPpdu::GetType() const
{
    return WIFI_PPDU_TYPE_SU;
}

uint16_t
WifiPpdu::GetStaId() const
{
    return SU_STA_ID;
}

Time
WifiPpdu::GetTxDuration() const
{
    NS_FATAL_ERROR("This method should not be called for the base WifiPpdu class. Use the "
                   "overloaded version in the amendment-specific PPDU subclasses instead!");
    return MicroSeconds(0); // should be overloaded
}

void
WifiPpdu::Print(std::ostream& os) const
{
    os << "[ preamble=" << m_preamble << ", modulation=" << m_modulation
       << ", truncatedTx=" << (m_truncatedTx ? "Y" : "N") << ", UID=" << m_uid << ", "
       << PrintPayload() << "]";
}

std::string
WifiPpdu::PrintPayload() const
{
    std::ostringstream ss;
    ss << "PSDU=" << *GetPsdu() << " ";
    return ss.str();
}

Ptr<WifiPpdu>
WifiPpdu::Copy() const
{
    NS_FATAL_ERROR("This method should not be called for the base WifiPpdu class. Use the "
                   "overloaded version in the amendment-specific PPDU subclasses instead!");
    return Ptr<WifiPpdu>(new WifiPpdu(*this), false);
}

std::ostream&
operator<<(std::ostream& os, const Ptr<const WifiPpdu>& ppdu)
{
    ppdu->Print(os);
    return os;
}

} // namespace ns3
