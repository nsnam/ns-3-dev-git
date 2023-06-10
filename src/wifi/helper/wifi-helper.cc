/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "wifi-helper.h"

#include "ns3/ampdu-subframe-header.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/config.h"
#include "ns3/eht-configuration.h"
#include "ns3/eht-ppdu.h"
#include "ns3/he-configuration.h"
#include "ns3/ht-configuration.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/obss-pd-algorithm.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/qos-utils.h"
#include "ns3/radiotap-header.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/vht-configuration.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mac-trailer.h"

#include <bit>
#include <memory>

namespace
{
/**
 * Helper function to place the value to set in the correct bit(s) of a radiotap subfield.
 *
 * @param mask the mask of the corresponding subfield
 * @param val the value the subfield should be set to
 * @return the value placed at the correct position based on the mask
 */
uint32_t
GetRadiotapField(uint32_t mask, uint32_t val)
{
    const auto shift = std::countr_zero(mask);
    return (val << shift) & mask;
}
} // namespace

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiHelper");

/**
 * ASCII trace PHY transmit sink with context
 * @param stream the output stream
 * @param context the context name
 * @param p the packet
 * @param mode the wifi mode
 * @param preamble the wifi preamble
 * @param txLevel the transmit power level
 */
static void
AsciiPhyTransmitSinkWithContext(Ptr<OutputStreamWrapper> stream,
                                std::string context,
                                Ptr<const Packet> p,
                                WifiMode mode,
                                WifiPreamble preamble,
                                uint8_t txLevel)
{
    NS_LOG_FUNCTION(stream << context << p << mode << preamble << txLevel);
    auto pCopy = p->Copy();
    WifiMacTrailer fcs;
    pCopy->RemoveTrailer(fcs);
    *stream->GetStream() << "t " << Simulator::Now().GetSeconds() << " " << context << " " << mode
                         << " " << *pCopy << " " << fcs << std::endl;
}

/**
 * ASCII trace PHY transmit sink without context
 * @param stream the output stream
 * @param p the packet
 * @param mode the wifi mode
 * @param preamble the wifi preamble
 * @param txLevel the transmit power level
 */
static void
AsciiPhyTransmitSinkWithoutContext(Ptr<OutputStreamWrapper> stream,
                                   Ptr<const Packet> p,
                                   WifiMode mode,
                                   WifiPreamble preamble,
                                   uint8_t txLevel)
{
    NS_LOG_FUNCTION(stream << p << mode << preamble << txLevel);
    auto pCopy = p->Copy();
    WifiMacTrailer fcs;
    pCopy->RemoveTrailer(fcs);
    *stream->GetStream() << "t " << Simulator::Now().GetSeconds() << " " << mode << " " << *pCopy
                         << " " << fcs << std::endl;
}

/**
 * ASCII trace PHY receive sink with context
 * @param stream the output stream
 * @param context the context name
 * @param p the packet
 * @param snr the SNR
 * @param mode the wifi mode
 * @param preamble the wifi preamble
 */
static void
AsciiPhyReceiveSinkWithContext(Ptr<OutputStreamWrapper> stream,
                               std::string context,
                               Ptr<const Packet> p,
                               double snr,
                               WifiMode mode,
                               WifiPreamble preamble)
{
    NS_LOG_FUNCTION(stream << context << p << snr << mode << preamble);
    auto pCopy = p->Copy();
    WifiMacTrailer fcs;
    pCopy->RemoveTrailer(fcs);
    *stream->GetStream() << "r " << Simulator::Now().GetSeconds() << " " << mode << " " << context
                         << " " << *pCopy << " " << fcs << std::endl;
}

/**
 * ASCII trace PHY receive sink without context
 * @param stream the output stream
 * @param p the packet
 * @param snr the SNR
 * @param mode the wifi mode
 * @param preamble the wifi preamble
 */
static void
AsciiPhyReceiveSinkWithoutContext(Ptr<OutputStreamWrapper> stream,
                                  Ptr<const Packet> p,
                                  double snr,
                                  WifiMode mode,
                                  WifiPreamble preamble)
{
    NS_LOG_FUNCTION(stream << p << snr << mode << preamble);
    auto pCopy = p->Copy();
    WifiMacTrailer fcs;
    pCopy->RemoveTrailer(fcs);
    *stream->GetStream() << "r " << Simulator::Now().GetSeconds() << " " << mode << " " << *pCopy
                         << " " << fcs << std::endl;
}

WifiPhyHelper::WifiPhyHelper(uint8_t nLinks)
    : m_pcapDlt{PcapHelper::DLT_IEEE802_11},
      m_pcapType{PcapCaptureType::PCAP_PER_PHY}
{
    NS_ABORT_IF(nLinks == 0);
    m_phys.resize(nLinks);
    m_errorRateModel.resize(nLinks);
    m_frameCaptureModel.resize(nLinks);
    m_preambleDetectionModel.resize(nLinks);

    SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel");
}

WifiPhyHelper::~WifiPhyHelper()
{
}

void
WifiPhyHelper::Set(std::string name, const AttributeValue& v)
{
    for (auto& phy : m_phys)
    {
        phy.Set(name, v);
    }
}

void
WifiPhyHelper::Set(uint8_t linkId, std::string name, const AttributeValue& v)
{
    m_phys.at(linkId).Set(name, v);
}

void
WifiPhyHelper::DisablePreambleDetectionModel()
{
    m_preambleDetectionModel.clear();
    m_preambleDetectionModel.resize(m_phys.size());
}

Ptr<PcapFileWrapper>
WifiPhyHelper::GetOrCreatePcapFile(const std::shared_ptr<PcapFilesInfo>& info, uint8_t phyId)
{
    uint8_t fileIdx;
    switch (info->pcapType)
    {
    case WifiPhyHelper::PcapCaptureType::PCAP_PER_DEVICE:
        fileIdx = 0;
        break;
    case WifiPhyHelper::PcapCaptureType::PCAP_PER_PHY:
        fileIdx = phyId;
        break;
    case WifiPhyHelper::PcapCaptureType::PCAP_PER_LINK:
        if (const auto linkId = info->device->GetMac()->GetLinkForPhy(phyId))
        {
            fileIdx = *linkId;
            break;
        }
        return nullptr;
    default:
        NS_ABORT_MSG("Unexpected PCAP capture type");
        return nullptr;
    }

    if (!info->files.contains(fileIdx))
    {
        // file does not exist yet, create it
        auto tmp = info->commonFilename;

        // find the last point in the filename
        auto pos = info->commonFilename.find_last_of('.');
        // if not found, set pos to filename size
        pos = (pos == std::string::npos) ? info->commonFilename.size() : pos;

        // insert PHY/link ID only for multi-link devices, unless a single PCAP is generated for the
        // device
        if ((info->device->GetNPhys() > 1) && (info->pcapType != PcapCaptureType::PCAP_PER_DEVICE))
        {
            tmp.insert(pos, "-" + std::to_string(fileIdx));
        }

        PcapHelper pcapHelper;
        auto file = pcapHelper.CreateFile(tmp, std::ios::out, info->pcapDlt);
        info->files.emplace(fileIdx, file);
    }

    return info->files.at(fileIdx);
}

void
WifiPhyHelper::PcapSniffTxEvent(const std::shared_ptr<PcapFilesInfo>& info,
                                uint8_t phyId,
                                Ptr<const Packet> packet,
                                uint16_t channelFreqMhz,
                                WifiTxVector txVector,
                                MpduInfo aMpdu,
                                uint16_t staId)
{
    auto file = GetOrCreatePcapFile(info, phyId);
    if (!file)
    {
        return;
    }
    switch (info->pcapDlt)
    {
    case PcapHelper::DLT_IEEE802_11:
        file->Write(Simulator::Now(), packet);
        return;
    case PcapHelper::DLT_PRISM_HEADER: {
        NS_FATAL_ERROR("PcapSniffTxEvent(): DLT_PRISM_HEADER not implemented");
        return;
    }
    case PcapHelper::DLT_IEEE802_11_RADIO: {
        Ptr<Packet> p = packet->Copy();
        RadiotapHeader header;
        GetRadiotapHeader(header,
                          p,
                          channelFreqMhz,
                          info->device->GetPhy(phyId)->GetPrimary20Index(),
                          txVector,
                          aMpdu,
                          staId);
        p->AddHeader(header);
        file->Write(Simulator::Now(), p);
        return;
    }
    default:
        NS_ABORT_MSG("PcapSniffTxEvent(): Unexpected data link type " << info->pcapDlt);
    }
}

void
WifiPhyHelper::PcapSniffRxEvent(const std::shared_ptr<PcapFilesInfo>& info,
                                uint8_t phyId,
                                Ptr<const Packet> packet,
                                uint16_t channelFreqMhz,
                                WifiTxVector txVector,
                                MpduInfo aMpdu,
                                SignalNoiseDbm signalNoise,
                                uint16_t staId)
{
    auto file = GetOrCreatePcapFile(info, phyId);
    if (!file)
    {
        return;
    }
    switch (info->pcapDlt)
    {
    case PcapHelper::DLT_IEEE802_11:
        file->Write(Simulator::Now(), packet);
        return;
    case PcapHelper::DLT_PRISM_HEADER: {
        NS_FATAL_ERROR("PcapSniffRxEvent(): DLT_PRISM_HEADER not implemented");
        return;
    }
    case PcapHelper::DLT_IEEE802_11_RADIO: {
        Ptr<Packet> p = packet->Copy();
        RadiotapHeader header;
        GetRadiotapHeader(header,
                          p,
                          channelFreqMhz,
                          info->device->GetPhy(phyId)->GetPrimary20Index(),
                          txVector,
                          aMpdu,
                          staId,
                          signalNoise);
        p->AddHeader(header);
        file->Write(Simulator::Now(), p);
        return;
    }
    default:
        NS_ABORT_MSG("PcapSniffRxEvent(): Unexpected data link type " << info->pcapDlt);
    }
}

void
WifiPhyHelper::GetRadiotapHeader(RadiotapHeader& header,
                                 Ptr<Packet> packet,
                                 uint16_t channelFreqMhz,
                                 uint8_t p20Index,
                                 const WifiTxVector& txVector,
                                 MpduInfo aMpdu,
                                 uint16_t staId,
                                 SignalNoiseDbm signalNoise)
{
    header.SetAntennaSignalPower(signalNoise.signal);
    header.SetAntennaNoisePower(signalNoise.noise);
    GetRadiotapHeader(header, packet, channelFreqMhz, p20Index, txVector, aMpdu, staId);
}

void
WifiPhyHelper::GetRadiotapHeader(RadiotapHeader& header,
                                 Ptr<Packet> packet,
                                 uint16_t channelFreqMhz,
                                 uint8_t p20Index,
                                 const WifiTxVector& txVector,
                                 MpduInfo aMpdu,
                                 uint16_t staId)
{
    const auto preamble = txVector.GetPreambleType();
    const auto modClass = txVector.GetModulationClass();
    const auto channelWidth = txVector.GetChannelWidth();
    const auto gi = txVector.GetGuardInterval();

    header.SetTsft(Simulator::Now().GetMicroSeconds());

    uint8_t frameFlags = RadiotapHeader::FRAME_FLAG_NONE;
    // Our capture includes the FCS, so we set the flag to say so.
    frameFlags |= RadiotapHeader::FRAME_FLAG_FCS_INCLUDED;
    if (preamble == WIFI_PREAMBLE_SHORT)
    {
        frameFlags |= RadiotapHeader::FRAME_FLAG_SHORT_PREAMBLE;
    }
    if (gi.GetNanoSeconds() == 400)
    {
        frameFlags |= RadiotapHeader::FRAME_FLAG_SHORT_GUARD;
    }
    header.SetFrameFlags(frameFlags);

    uint8_t mcs = 0;
    uint8_t nss = 1;
    uint64_t rate = 0;
    if (modClass < WIFI_MOD_CLASS_HT)
    {
        rate = txVector.GetMode(staId).GetDataRate(channelWidth, gi, 1) * nss / 500000;
        header.SetRate(static_cast<uint8_t>(rate));
    }
    else
    {
        mcs = txVector.GetMode(staId).GetMcsValue();
        nss = txVector.GetNss(staId);
    }

    RadiotapHeader::ChannelFields channelFields{.frequency = channelFreqMhz};
    switch (rate)
    {
    case 2:  // 1Mbps
    case 4:  // 2Mbps
    case 10: // 5Mbps
    case 22: // 11Mbps
        channelFields.flags |= RadiotapHeader::CHANNEL_FLAG_CCK;
        break;
    default:
        channelFields.flags |= RadiotapHeader::CHANNEL_FLAG_OFDM;
        break;
    }
    if (channelFreqMhz < 2500)
    {
        channelFields.flags |= RadiotapHeader::CHANNEL_FLAG_SPECTRUM_2GHZ;
    }
    else
    {
        channelFields.flags |= RadiotapHeader::CHANNEL_FLAG_SPECTRUM_5GHZ;
    }
    header.SetChannelFields(channelFields);

    if (modClass == WIFI_MOD_CLASS_HT)
    {
        RadiotapHeader::McsFields mcsFields{.mcs = mcs};

        mcsFields.known |= RadiotapHeader::MCS_KNOWN_INDEX | RadiotapHeader::MCS_KNOWN_BANDWIDTH |
                           RadiotapHeader::MCS_KNOWN_GUARD_INTERVAL |
                           RadiotapHeader::MCS_KNOWN_HT_FORMAT | RadiotapHeader::MCS_KNOWN_NESS |
                           RadiotapHeader::MCS_KNOWN_FEC_TYPE | RadiotapHeader::MCS_KNOWN_STBC;

        if (channelWidth == MHz_u{40})
        {
            mcsFields.flags |= RadiotapHeader::MCS_FLAGS_BANDWIDTH_40;
        }

        if (gi.GetNanoSeconds() == 400)
        {
            mcsFields.flags |= RadiotapHeader::MCS_FLAGS_GUARD_INTERVAL;
        }

        const auto ness = txVector.GetNess();
        if (ness & 0x01) // bit 1
        {
            mcsFields.flags |= RadiotapHeader::MCS_FLAGS_NESS_BIT_0;
        }
        if (ness & 0x02) // bit 2
        {
            mcsFields.flags |= RadiotapHeader::MCS_KNOWN_NESS_BIT_1;
        }

        if (txVector.IsStbc())
        {
            mcsFields.flags |= RadiotapHeader::MCS_FLAGS_STBC_STREAMS;
        }

        header.SetMcsFields(mcsFields);
    }

    if (txVector.IsAggregation())
    {
        RadiotapHeader::AmpduStatusFields ampduStatusFields{.referenceNumber = aMpdu.mpduRefNumber};
        ampduStatusFields.flags |= RadiotapHeader::A_MPDU_STATUS_LAST_KNOWN;
        /* For PCAP file, MPDU Delimiter and Padding should be removed by the MAC Driver */
        AmpduSubframeHeader hdr;
        uint32_t extractedLength;
        packet->RemoveHeader(hdr);
        extractedLength = hdr.GetLength();
        packet = packet->CreateFragment(0, static_cast<uint32_t>(extractedLength));
        if (aMpdu.type == LAST_MPDU_IN_AGGREGATE || (hdr.GetEof() && hdr.GetLength() > 0))
        {
            ampduStatusFields.flags |= RadiotapHeader::A_MPDU_STATUS_LAST;
        }
        header.SetAmpduStatus(ampduStatusFields);
    }

    if (modClass == WIFI_MOD_CLASS_VHT)
    {
        RadiotapHeader::VhtFields vhtFields{};

        vhtFields.known |= RadiotapHeader::VHT_KNOWN_STBC;
        if (txVector.IsStbc())
        {
            vhtFields.flags |= RadiotapHeader::VHT_FLAGS_STBC;
        }

        vhtFields.known |= RadiotapHeader::VHT_KNOWN_GUARD_INTERVAL;
        if (gi.GetNanoSeconds() == 400)
        {
            vhtFields.flags |= RadiotapHeader::VHT_FLAGS_GUARD_INTERVAL;
        }

        vhtFields.known |=
            RadiotapHeader::VHT_KNOWN_BEAMFORMED | RadiotapHeader::VHT_KNOWN_BANDWIDTH;
        // TODO: bandwidths can be provided with sideband info
        if (channelWidth == MHz_u{40})
        {
            vhtFields.bandwidth = 1;
        }
        else if (channelWidth == MHz_u{80})
        {
            vhtFields.bandwidth = 4;
        }
        else if (channelWidth == MHz_u{160})
        {
            vhtFields.bandwidth = 11;
        }

        // only SU PPDUs are currently supported
        vhtFields.mcsNss.at(0) |= (nss & 0x0f) | ((mcs << 4) & 0xf0);

        header.SetVhtFields(vhtFields);
    }

    if (modClass == WIFI_MOD_CLASS_HE)
    {
        RadiotapHeader::HeFields heFields{};
        heFields.data1 = RadiotapHeader::HE_DATA1_BSS_COLOR_KNOWN |
                         RadiotapHeader::HE_DATA1_DATA_MCS_KNOWN |
                         RadiotapHeader::HE_DATA1_BW_RU_ALLOC_KNOWN;
        if (preamble == WIFI_PREAMBLE_HE_ER_SU)
        {
            heFields.data1 |= RadiotapHeader::HE_DATA1_FORMAT_EXT_SU;
        }
        else if (preamble == WIFI_PREAMBLE_HE_MU)
        {
            heFields.data1 |=
                RadiotapHeader::HE_DATA1_FORMAT_MU | RadiotapHeader::HE_DATA1_SPTL_REUSE2_KNOWN;
        }
        else if (preamble == WIFI_PREAMBLE_HE_TB)
        {
            heFields.data1 |= RadiotapHeader::HE_DATA1_FORMAT_TRIG;
        }

        heFields.data2 = RadiotapHeader::HE_DATA2_GI_KNOWN;
        if (preamble == WIFI_PREAMBLE_HE_MU || preamble == WIFI_PREAMBLE_HE_TB)
        {
            heFields.data2 |=
                RadiotapHeader::HE_DATA2_RU_OFFSET_KNOWN |
                // HeRu indices start at 1 whereas RadioTap starts at 0
                GetRadiotapField(RadiotapHeader::HE_DATA2_RU_OFFSET,
                                 txVector.GetHeMuUserInfo(staId).ru.GetIndex() - 1) |
                GetRadiotapField(RadiotapHeader::HE_DATA2_PRISEC_80_SEC,
                                 !txVector.GetHeMuUserInfo(staId).ru.GetPrimary80MHz());
        }

        heFields.data3 =
            GetRadiotapField(RadiotapHeader::HE_DATA3_BSS_COLOR, txVector.GetBssColor()) |
            GetRadiotapField(RadiotapHeader::HE_DATA3_DATA_MCS, mcs);

        heFields.data4 = (preamble == WIFI_PREAMBLE_HE_MU)
                             ? GetRadiotapField(RadiotapHeader::HE_DATA4_MU_STA_ID, staId)
                             : 0;

        heFields.data5 = 0;
        if (preamble == WIFI_PREAMBLE_HE_MU || preamble == WIFI_PREAMBLE_HE_TB)
        {
            HeRu::RuType ruType = txVector.GetHeMuUserInfo(staId).ru.GetRuType();
            switch (ruType)
            {
            case HeRu::RU_26_TONE:
                heFields.data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_26T;
                break;
            case HeRu::RU_52_TONE:
                heFields.data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_52T;
                break;
            case HeRu::RU_106_TONE:
                heFields.data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_106T;
                break;
            case HeRu::RU_242_TONE:
                heFields.data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_242T;
                break;
            case HeRu::RU_484_TONE:
                heFields.data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_484T;
                break;
            case HeRu::RU_996_TONE:
                heFields.data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_996T;
                break;
            case HeRu::RU_2x996_TONE:
                heFields.data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_2x996T;
                break;
            default:
                NS_ABORT_MSG("Unexpected RU type");
            }
        }
        else if (channelWidth == MHz_u{40})
        {
            heFields.data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_40MHZ;
        }
        else if (channelWidth == MHz_u{80})
        {
            heFields.data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_80MHZ;
        }
        else if (channelWidth == MHz_u{160})
        {
            heFields.data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_160MHZ;
        }
        if (gi.GetNanoSeconds() == 1600)
        {
            heFields.data5 |= RadiotapHeader::HE_DATA5_GI_1_6;
        }
        else if (gi.GetNanoSeconds() == 3200)
        {
            heFields.data5 |= RadiotapHeader::HE_DATA5_GI_3_2;
        }

        header.SetHeFields(heFields);
    }

    if (preamble == WIFI_PREAMBLE_HE_MU)
    {
        RadiotapHeader::HeMuFields heMuFields{};
        // TODO: fill in fields
        header.SetHeMuFields(heMuFields);
        RadiotapHeader::HeMuOtherUserFields heMuOtherUserFields{};
        // TODO: fill in fields
        header.SetHeMuOtherUserFields(heMuOtherUserFields);
    }

    if (IsEht(preamble))
    {
        RadiotapHeader::UsigFields usigFields{};
        usigFields.common = RadiotapHeader::USIG_COMMON_PHY_VER_KNOWN |
                            RadiotapHeader::USIG_COMMON_BW_KNOWN |
                            RadiotapHeader::USIG_COMMON_BSS_COLOR_KNOWN;
        switch (static_cast<uint16_t>(channelWidth))
        {
        case 20:
            usigFields.common |= GetRadiotapField(RadiotapHeader::USIG_COMMON_BW,
                                                  RadiotapHeader::USIG_COMMON_BW_20MHZ);
            break;
        case 40:
            usigFields.common |= GetRadiotapField(RadiotapHeader::USIG_COMMON_BW,
                                                  RadiotapHeader::USIG_COMMON_BW_40MHZ);
            break;
        case 80:
            usigFields.common |= GetRadiotapField(RadiotapHeader::USIG_COMMON_BW,
                                                  RadiotapHeader::USIG_COMMON_BW_80MHZ);
            break;
        case 160:
            usigFields.common |= GetRadiotapField(RadiotapHeader::USIG_COMMON_BW,
                                                  RadiotapHeader::USIG_COMMON_BW_160MHZ);
            break;
        default:
            NS_ABORT_MSG("Unexpected channel width");
            break;
        }
        usigFields.common |=
            GetRadiotapField(RadiotapHeader::USIG_COMMON_BSS_COLOR, txVector.GetBssColor());
        if (preamble == WIFI_PREAMBLE_EHT_MU)
        {
            usigFields.mask = RadiotapHeader::USIG2_MU_B0_B1_PPDU_TYPE |
                              RadiotapHeader::USIG2_MU_B9_B10_SIG_MCS |
                              RadiotapHeader::USIG2_MU_B3_B7_PUNCTURED_INFO;
            usigFields.value = GetRadiotapField(RadiotapHeader::USIG2_MU_B0_B1_PPDU_TYPE,
                                                txVector.GetEhtPpduType()) |
                               GetRadiotapField(RadiotapHeader::USIG2_MU_B9_B10_SIG_MCS,
                                                txVector.GetSigBMode().GetMcsValue());
            std::optional<bool> isLow80MHz;
            if (txVector.IsDlMu() && channelWidth > MHz_u{80})
            {
                const auto isLowP80 = p20Index < (channelWidth / MHz_u{40});
                const auto isP80 = txVector.GetHeMuUserInfo(staId).ru.GetPrimary80MHz();
                isLow80MHz = (isLowP80 && isP80) || (!isLowP80 && !isP80);
            }
            const auto puncturedChannelInfo =
                EhtPpdu::GetPuncturedInfo(txVector.GetInactiveSubchannels(),
                                          txVector.GetEhtPpduType(),
                                          isLow80MHz);
            usigFields.value |= GetRadiotapField(RadiotapHeader::USIG2_MU_B3_B7_PUNCTURED_INFO,
                                                 puncturedChannelInfo);
        }
        else
        {
            usigFields.mask = RadiotapHeader::USIG2_TB_B0_B1_PPDU_TYPE;
            usigFields.value = GetRadiotapField(RadiotapHeader::USIG2_TB_B0_B1_PPDU_TYPE,
                                                txVector.GetEhtPpduType());
        }
        header.SetUsigFields(usigFields);
    }

    if (preamble == WIFI_PREAMBLE_EHT_MU)
    {
        RadiotapHeader::EhtFields ehtFields{};
        ehtFields.known = RadiotapHeader::EHT_KNOWN_GI | RadiotapHeader::EHT_KNOWN_RU_MRU_SIZE_OM |
                          RadiotapHeader::EHT_KNOWN_RU_MRU_INDEX_OM;
        switch (gi.GetNanoSeconds())
        {
        case 800:
            ehtFields.data.at(0) =
                GetRadiotapField(RadiotapHeader::EHT_DATA0_GI, RadiotapHeader::EHT_DATA0_GI_800_NS);
            break;
        case 1600:
            ehtFields.data.at(0) = GetRadiotapField(RadiotapHeader::EHT_DATA0_GI,
                                                    RadiotapHeader::EHT_DATA0_GI_1600_NS);
            break;
        case 3200:
            ehtFields.data.at(0) = GetRadiotapField(RadiotapHeader::EHT_DATA0_GI,
                                                    RadiotapHeader::EHT_DATA0_GI_3200_NS);
            break;
        default:
            NS_ABORT_MSG("Unexpected guard interval");
            break;
        }
        ehtFields.data.at(1) = RadiotapHeader::EHT_DATA1_RU_ALLOC_CC_1_1_1_KNOWN;
        const auto ruType = (txVector.GetEhtPpduType() == 1) ? HeRu::GetRuType(channelWidth)
                                                             : txVector.GetRu(staId).GetRuType();
        switch (ruType)
        {
        case HeRu::RU_26_TONE:
            ehtFields.data.at(1) |= GetRadiotapField(RadiotapHeader::EHT_DATA1_RU_MRU_SIZE,
                                                     RadiotapHeader::EHT_DATA1_RU_MRU_SIZE_26);
            break;
        case HeRu::RU_52_TONE:
            ehtFields.data.at(1) |= GetRadiotapField(RadiotapHeader::EHT_DATA1_RU_MRU_SIZE,
                                                     RadiotapHeader::EHT_DATA1_RU_MRU_SIZE_52);
            break;
        case HeRu::RU_106_TONE:
            ehtFields.data.at(1) |= GetRadiotapField(RadiotapHeader::EHT_DATA1_RU_MRU_SIZE,
                                                     RadiotapHeader::EHT_DATA1_RU_MRU_SIZE_106);
            break;
        case HeRu::RU_242_TONE:
            ehtFields.data.at(1) |= GetRadiotapField(RadiotapHeader::EHT_DATA1_RU_MRU_SIZE,
                                                     RadiotapHeader::EHT_DATA1_RU_MRU_SIZE_242);
            break;
        case HeRu::RU_484_TONE:
            ehtFields.data.at(1) |= GetRadiotapField(RadiotapHeader::EHT_DATA1_RU_MRU_SIZE,
                                                     RadiotapHeader::EHT_DATA1_RU_MRU_SIZE_484);
            break;
        case HeRu::RU_996_TONE:
            ehtFields.data.at(1) |= GetRadiotapField(RadiotapHeader::EHT_DATA1_RU_MRU_SIZE,
                                                     RadiotapHeader::EHT_DATA1_RU_MRU_SIZE_996);
            break;
        case HeRu::RU_2x996_TONE:
            ehtFields.data.at(1) |= GetRadiotapField(RadiotapHeader::EHT_DATA1_RU_MRU_SIZE,
                                                     RadiotapHeader::EHT_DATA1_RU_MRU_SIZE_2x996);
            break;
        default:
            NS_ABORT_MSG("Unexpected RU type");
            break;
        }
        const auto ruIndex =
            (txVector.GetEhtPpduType() == 1) ? 1 : txVector.GetRu(staId).GetIndex();
        const auto& ruAllocation = txVector.GetRuAllocation(p20Index);
        ehtFields.data.at(1) |=
            GetRadiotapField(RadiotapHeader::EHT_DATA1_RU_MRU_INDEX, ruIndex) |
            GetRadiotapField(RadiotapHeader::EHT_DATA1_RU_ALLOC_CC_1_1_1, ruAllocation.at(0));
        if (channelWidth >= MHz_u{40})
        {
            ehtFields.data.at(2) =
                RadiotapHeader::EHT_DATA2_RU_ALLOC_CC_2_1_1_KNOWN |
                GetRadiotapField(RadiotapHeader::EHT_DATA2_RU_ALLOC_CC_2_1_1, ruAllocation.at(1));
        }
        if (channelWidth >= MHz_u{80})
        {
            ehtFields.data.at(2) |=
                RadiotapHeader::EHT_DATA2_RU_ALLOC_CC_1_1_2_KNOWN |
                RadiotapHeader::EHT_DATA2_RU_ALLOC_CC_2_1_2_KNOWN |
                GetRadiotapField(RadiotapHeader::EHT_DATA2_RU_ALLOC_CC_1_1_2, ruAllocation.at(2)) |
                GetRadiotapField(RadiotapHeader::EHT_DATA2_RU_ALLOC_CC_2_1_2, ruAllocation.at(3));
        }
        if (channelWidth >= MHz_u{160})
        {
            ehtFields.data.at(3) =
                RadiotapHeader::EHT_DATA3_RU_ALLOC_CC_1_2_1_KNOWN |
                RadiotapHeader::EHT_DATA3_RU_ALLOC_CC_2_2_1_KNOWN |
                RadiotapHeader::EHT_DATA3_RU_ALLOC_CC_1_2_2_KNOWN |
                GetRadiotapField(RadiotapHeader::EHT_DATA3_RU_ALLOC_CC_1_2_1, ruAllocation.at(4)) |
                GetRadiotapField(RadiotapHeader::EHT_DATA3_RU_ALLOC_CC_2_2_1, ruAllocation.at(5)) |
                GetRadiotapField(RadiotapHeader::EHT_DATA3_RU_ALLOC_CC_1_2_2, ruAllocation.at(6));
            ehtFields.data.at(4) =
                RadiotapHeader::EHT_DATA4_RU_ALLOC_CC_2_2_2_KNOWN |
                GetRadiotapField(RadiotapHeader::EHT_DATA4_RU_ALLOC_CC_2_2_2, ruAllocation.at(7));
            ehtFields.known |= RadiotapHeader::EHT_KNOWN_PRIMARY_80;
            const auto isLowP80 = p20Index < (channelWidth / MHz_u{40});
            ehtFields.data.at(1) |=
                GetRadiotapField(RadiotapHeader::EHT_DATA1_PRIMARY_80,
                                 (isLowP80 ? RadiotapHeader::EHT_DATA1_PRIMARY_80_LOWEST
                                           : RadiotapHeader::EHT_DATA1_PRIMARY_80_HIGHEST));
        }
        // TODO: handle 320 MHz when supported
        uint32_t userInfo = RadiotapHeader::EHT_USER_INFO_STA_ID_KNOWN |
                            RadiotapHeader::EHT_USER_INFO_MCS_KNOWN |
                            RadiotapHeader::EHT_USER_INFO_NSS_KNOWN_O |
                            RadiotapHeader::EHT_USER_INFO_DATA_FOR_USER |
                            GetRadiotapField(RadiotapHeader::EHT_USER_INFO_STA_ID, staId) |
                            GetRadiotapField(RadiotapHeader::EHT_USER_INFO_MCS, mcs) |
                            GetRadiotapField(RadiotapHeader::EHT_USER_INFO_NSS_O, nss);
        ehtFields.userInfo.push_back(userInfo);
        header.SetEhtFields(ehtFields);
    }
}

void
WifiPhyHelper::SetPcapDataLinkType(SupportedPcapDataLinkTypes dlt)
{
    switch (dlt)
    {
    case DLT_IEEE802_11:
        m_pcapDlt = PcapHelper::DLT_IEEE802_11;
        return;
    case DLT_PRISM_HEADER:
        m_pcapDlt = PcapHelper::DLT_PRISM_HEADER;
        return;
    case DLT_IEEE802_11_RADIO:
        m_pcapDlt = PcapHelper::DLT_IEEE802_11_RADIO;
        return;
    default:
        NS_ABORT_MSG("WifiPhyHelper::SetPcapFormat(): Unexpected format");
    }
}

PcapHelper::DataLinkType
WifiPhyHelper::GetPcapDataLinkType() const
{
    return m_pcapDlt;
}

void
WifiPhyHelper::SetPcapCaptureType(PcapCaptureType type)
{
    m_pcapType = type;
}

WifiPhyHelper::PcapCaptureType
WifiPhyHelper::GetPcapCaptureType() const
{
    return m_pcapType;
}

void
WifiPhyHelper::EnablePcapInternal(std::string prefix,
                                  Ptr<NetDevice> nd,
                                  bool promiscuous,
                                  bool explicitFilename)
{
    NS_LOG_FUNCTION(this << prefix << nd << promiscuous << explicitFilename);

    // All of the Pcap enable functions vector through here including the ones
    // that are wandering through all of devices on perhaps all of the nodes in
    // the system. We can only deal with devices of type WifiNetDevice.
    Ptr<WifiNetDevice> device = nd->GetObject<WifiNetDevice>();
    if (!device)
    {
        NS_LOG_INFO("WifiHelper::EnablePcapInternal(): Device "
                    << &device << " not of type ns3::WifiNetDevice");
        return;
    }

    NS_ABORT_MSG_IF(device->GetPhys().empty(),
                    "WifiPhyHelper::EnablePcapInternal(): Phy layer in WifiNetDevice must be set");

    PcapHelper pcapHelper;
    std::string filename;
    if (explicitFilename)
    {
        filename = prefix;
    }
    else
    {
        filename = pcapHelper.GetFilenameFromDevice(prefix, device);
    }

    auto info = std::make_shared<PcapFilesInfo>(filename, m_pcapDlt, m_pcapType, device);
    for (auto& phy : device->GetPhys())
    {
        phy->TraceConnectWithoutContext(
            "MonitorSnifferTx",
            MakeBoundCallback(&WifiPhyHelper::PcapSniffTxEvent, info, phy->GetPhyId()));
        phy->TraceConnectWithoutContext(
            "MonitorSnifferRx",
            MakeBoundCallback(&WifiPhyHelper::PcapSniffRxEvent, info, phy->GetPhyId()));
    }
}

void
WifiPhyHelper::EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
                                   std::string prefix,
                                   Ptr<NetDevice> nd,
                                   bool explicitFilename)
{
    // All of the ASCII enable functions vector through here including the ones
    // that are wandering through all of devices on perhaps all of the nodes in
    // the system. We can only deal with devices of type WifiNetDevice.
    Ptr<WifiNetDevice> device = nd->GetObject<WifiNetDevice>();
    if (!device)
    {
        NS_LOG_INFO("WifiHelper::EnableAsciiInternal(): Device "
                    << device << " not of type ns3::WifiNetDevice");
        return;
    }

    // Our trace sinks are going to use packet printing, so we have to make sure
    // that is turned on.
    Packet::EnablePrinting();

    uint32_t nodeid = nd->GetNode()->GetId();
    uint32_t deviceid = nd->GetIfIndex();
    std::ostringstream oss;

    // If we are not provided an OutputStreamWrapper, we are expected to create
    // one using the usual trace filename conventions and write our traces
    // without a context since there will be one file per context and therefore
    // the context would be redundant.
    if (!stream)
    {
        // Set up an output stream object to deal with private ofstream copy
        // constructor and lifetime issues. Let the helper decide the actual
        // name of the file given the prefix.
        AsciiTraceHelper asciiTraceHelper;

        std::string filename;
        if (explicitFilename)
        {
            filename = prefix;
        }
        else
        {
            filename = asciiTraceHelper.GetFilenameFromDevice(prefix, device);
        }

        // find the last point in the filename
        auto pos = filename.find_last_of('.');
        // if not found, set pos to filename size
        pos = (pos == std::string::npos) ? filename.size() : pos;

        for (uint8_t linkId = 0; linkId < device->GetNPhys(); linkId++)
        {
            std::string tmp = filename;
            if (device->GetNPhys() > 1)
            {
                // insert LinkId only for multi-link devices
                tmp.insert(pos, "-" + std::to_string(linkId));
            }
            auto theStream = asciiTraceHelper.CreateFileStream(tmp);
            // We could go poking through the PHY and the state looking for the
            // correct trace source, but we can let Config deal with that with
            // some search cost.  Since this is presumably happening at topology
            // creation time, it doesn't seem much of a price to pay.
            oss.str("");
            oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
                << "/$ns3::WifiNetDevice/Phys/" << +linkId << "/State/RxOk";
            Config::ConnectWithoutContext(
                oss.str(),
                MakeBoundCallback(&AsciiPhyReceiveSinkWithoutContext, theStream));

            oss.str("");
            oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
                << "/$ns3::WifiNetDevice/Phys/" << +linkId << "/State/Tx";
            Config::ConnectWithoutContext(
                oss.str(),
                MakeBoundCallback(&AsciiPhyTransmitSinkWithoutContext, theStream));
        }

        return;
    }

    // If we are provided an OutputStreamWrapper, we are expected to use it, and
    // to provide a context. We are free to come up with our own context if we
    // want, and use the AsciiTraceHelper Hook*WithContext functions, but for
    // compatibility and simplicity, we just use Config::Connect and let it deal
    // with coming up with a context.
    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::WifiNetDevice/Phy/State/RxOk";
    Config::Connect(oss.str(), MakeBoundCallback(&AsciiPhyReceiveSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::WifiNetDevice/Phy/State/Tx";
    Config::Connect(oss.str(), MakeBoundCallback(&AsciiPhyTransmitSinkWithContext, stream));
}

WifiHelper::~WifiHelper()
{
}

WifiHelper::WifiHelper()
    : m_standard(WIFI_STANDARD_80211ax),
      m_selectQueueCallback(&SelectQueueByDSField),
      m_enableFlowControl(true)
{
    SetRemoteStationManager("ns3::IdealWifiManager");
    m_htConfig.SetTypeId("ns3::HtConfiguration");
    m_vhtConfig.SetTypeId("ns3::VhtConfiguration");
    m_heConfig.SetTypeId("ns3::HeConfiguration");
    m_ehtConfig.SetTypeId("ns3::EhtConfiguration");
}

namespace
{
/// Map strings to WifiStandard enum values
const std::unordered_map<std::string, WifiStandard> WIFI_STANDARDS_NAME_MAP{
    // clang-format off
    {"802.11a",  WIFI_STANDARD_80211a},
    {"11a",      WIFI_STANDARD_80211a},

    {"802.11b",  WIFI_STANDARD_80211b},
    {"11b",      WIFI_STANDARD_80211b},

    {"802.11g",  WIFI_STANDARD_80211g},
    {"11g",      WIFI_STANDARD_80211g},

    {"802.11p",  WIFI_STANDARD_80211p},
    {"11p",      WIFI_STANDARD_80211p},

    {"802.11n",  WIFI_STANDARD_80211n},
    {"11n",      WIFI_STANDARD_80211n},
    {"HT",       WIFI_STANDARD_80211n},

    {"802.11ac", WIFI_STANDARD_80211ac},
    {"11ac",     WIFI_STANDARD_80211ac},
    {"VHT",      WIFI_STANDARD_80211ac},

    {"802.11ad", WIFI_STANDARD_80211ad},
    {"11ad",     WIFI_STANDARD_80211ad},

    {"802.11ax", WIFI_STANDARD_80211ax},
    {"11ax",     WIFI_STANDARD_80211ax},
    {"HE",       WIFI_STANDARD_80211ax},

    {"802.11be", WIFI_STANDARD_80211be},
    {"11be",     WIFI_STANDARD_80211be},
    {"EHT",      WIFI_STANDARD_80211be},
    // clang-format on
};
} // namespace

void
WifiHelper::SetStandard(WifiStandard standard)
{
    m_standard = standard;
}

void
WifiHelper::SetStandard(const std::string& standard)
{
    NS_ABORT_MSG_IF(!WIFI_STANDARDS_NAME_MAP.contains(standard),
                    "Specified Wi-Fi standard " << standard << " is currently not supported");
    SetStandard(WIFI_STANDARDS_NAME_MAP.at(standard));
}

void
WifiHelper::DisableFlowControl()
{
    m_enableFlowControl = false;
}

void
WifiHelper::SetSelectQueueCallback(SelectQueueCallback f)
{
    m_selectQueueCallback = f;
}

NetDeviceContainer
WifiHelper::Install(const WifiPhyHelper& phyHelper,
                    const WifiMacHelper& macHelper,
                    NodeContainer::Iterator first,
                    NodeContainer::Iterator last) const
{
    NetDeviceContainer devices;
    for (auto i = first; i != last; ++i)
    {
        Ptr<Node> node = *i;
        Ptr<WifiNetDevice> device = CreateObject<WifiNetDevice>();
        node->AddDevice(device);
        device->SetStandard(m_standard);
        if (m_standard == WIFI_STANDARD_UNSPECIFIED)
        {
            NS_FATAL_ERROR("No standard specified!");
            return devices;
        }
        if (m_standard >= WIFI_STANDARD_80211n)
        {
            auto htConfiguration = m_htConfig.Create<HtConfiguration>();
            device->SetHtConfiguration(htConfiguration);
        }
        if (m_standard >= WIFI_STANDARD_80211ac)
        {
            // Create the VHT Configuration object even if the PHY band is 2.4GHz
            // (WifiNetDevice::GetVhtConfiguration() checks the PHY band being used).
            // This approach allows us not to worry about deleting this object when
            // the PHY band is switched from 5GHz to 2.4GHz and creating this object
            // when the PHY band is switched from 2.4GHz to 5GHz.
            auto vhtConfiguration = m_vhtConfig.Create<VhtConfiguration>();
            device->SetVhtConfiguration(vhtConfiguration);
        }
        if (m_standard >= WIFI_STANDARD_80211ax)
        {
            auto heConfiguration = m_heConfig.Create<HeConfiguration>();
            device->SetHeConfiguration(heConfiguration);
        }
        if (m_standard >= WIFI_STANDARD_80211be)
        {
            auto ehtConfiguration = m_ehtConfig.Create<EhtConfiguration>();
            device->SetEhtConfiguration(ehtConfiguration);
        }
        std::vector<Ptr<WifiRemoteStationManager>> managers;
        std::vector<Ptr<WifiPhy>> phys = phyHelper.Create(node, device);
        device->SetPhys(phys);
        // if only one remote station manager model was provided, replicate it for all the links
        auto stationManagers = m_stationManager;
        if (stationManagers.size() == 1 && phys.size() > 1)
        {
            stationManagers.resize(phys.size(), stationManagers[0]);
        }
        NS_ABORT_MSG_IF(stationManagers.size() != phys.size(),
                        "Number of station manager models ("
                            << stationManagers.size() << ") does not match the number of links ("
                            << phys.size() << ")");
        for (std::size_t i = 0; i < phys.size(); i++)
        {
            phys[i]->ConfigureStandard(m_standard);
            managers.push_back(stationManagers[i].Create<WifiRemoteStationManager>());
        }
        device->SetRemoteStationManagers(managers);
        Ptr<WifiMac> mac = macHelper.Create(device, m_standard);
        if ((m_standard >= WIFI_STANDARD_80211ax) && (m_obssPdAlgorithm.IsTypeIdSet()))
        {
            Ptr<ObssPdAlgorithm> obssPdAlgorithm = m_obssPdAlgorithm.Create<ObssPdAlgorithm>();
            device->AggregateObject(obssPdAlgorithm);
            obssPdAlgorithm->ConnectWifiNetDevice(device);
        }
        devices.Add(device);
        NS_LOG_DEBUG("node=" << node << ", mob=" << node->GetObject<MobilityModel>());
        if (m_enableFlowControl)
        {
            Ptr<NetDeviceQueueInterface> ndqi;
            BooleanValue qosSupported;
            Ptr<WifiMacQueue> wmq;

            mac->GetAttributeFailSafe("QosSupported", qosSupported);
            if (qosSupported.Get())
            {
                ndqi = CreateObjectWithAttributes<NetDeviceQueueInterface>("NTxQueues",
                                                                           UintegerValue(4));
                for (auto& ac : {AC_BE, AC_BK, AC_VI, AC_VO})
                {
                    Ptr<QosTxop> qosTxop = mac->GetQosTxop(ac);
                    wmq = qosTxop->GetWifiMacQueue();
                    ndqi->GetTxQueue(static_cast<std::size_t>(ac))->ConnectQueueTraces(wmq);
                }
                ndqi->SetSelectQueueCallback(m_selectQueueCallback);
            }
            else
            {
                ndqi = CreateObject<NetDeviceQueueInterface>();

                wmq = mac->GetTxop()->GetWifiMacQueue();
                ndqi->GetTxQueue(0)->ConnectQueueTraces(wmq);
            }
            device->AggregateObject(ndqi);
        }
    }
    return devices;
}

NetDeviceContainer
WifiHelper::Install(const WifiPhyHelper& phyHelper,
                    const WifiMacHelper& macHelper,
                    NodeContainer c) const
{
    return Install(phyHelper, macHelper, c.Begin(), c.End());
}

NetDeviceContainer
WifiHelper::Install(const WifiPhyHelper& phy, const WifiMacHelper& mac, Ptr<Node> node) const
{
    return Install(phy, mac, NodeContainer(node));
}

NetDeviceContainer
WifiHelper::Install(const WifiPhyHelper& phy, const WifiMacHelper& mac, std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(phy, mac, NodeContainer(node));
}

void
WifiHelper::EnableLogComponents(LogLevel logLevel)
{
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnableAll(LOG_PREFIX_NODE);

    LogComponentEnable("AarfWifiManager", logLevel);
    LogComponentEnable("AarfcdWifiManager", logLevel);
    LogComponentEnable("AdhocWifiMac", logLevel);
    LogComponentEnable("AdvancedApEmlsrManager", logLevel);
    LogComponentEnable("AdvancedEmlsrManager", logLevel);
    LogComponentEnable("AmrrWifiManager", logLevel);
    LogComponentEnable("ApEmlsrManager", logLevel);
    LogComponentEnable("ApWifiMac", logLevel);
    LogComponentEnable("AparfWifiManager", logLevel);
    LogComponentEnable("ArfWifiManager", logLevel);
    LogComponentEnable("BlockAckAgreement", logLevel);
    LogComponentEnable("BlockAckManager", logLevel);
    LogComponentEnable("CaraWifiManager", logLevel);
    LogComponentEnable("ChannelAccessManager", logLevel);
    LogComponentEnable("ConstantObssPdAlgorithm", logLevel);
    LogComponentEnable("ConstantRateWifiManager", logLevel);
    LogComponentEnable("DefaultApEmlsrManager", logLevel);
    LogComponentEnable("DefaultEmlsrManager", logLevel);
    LogComponentEnable("DsssErrorRateModel", logLevel);
    LogComponentEnable("DsssPhy", logLevel);
    LogComponentEnable("DsssPpdu", logLevel);
    LogComponentEnable("EhtFrameExchangeManager", logLevel);
    LogComponentEnable("EhtPhy", logLevel);
    LogComponentEnable("EhtPpdu", logLevel);
    LogComponentEnable("EmlsrManager", logLevel);
    LogComponentEnable("ErpOfdmPhy", logLevel);
    LogComponentEnable("ErpOfdmPpdu", logLevel);
    LogComponentEnable("FrameExchangeManager", logLevel);
    LogComponentEnable("GcrManager", logLevel);
    LogComponentEnable("HeConfiguration", logLevel);
    LogComponentEnable("HeFrameExchangeManager", logLevel);
    LogComponentEnable("HePhy", logLevel);
    LogComponentEnable("HePpdu", logLevel);
    LogComponentEnable("HtConfiguration", logLevel);
    LogComponentEnable("HtFrameExchangeManager", logLevel);
    LogComponentEnable("HtPhy", logLevel);
    LogComponentEnable("HtPpdu", logLevel);
    LogComponentEnable("IdealWifiManager", logLevel);
    LogComponentEnable("InterferenceHelper", logLevel);
    LogComponentEnable("MacRxMiddle", logLevel);
    LogComponentEnable("MacTxMiddle", logLevel);
    LogComponentEnable("MinstrelHtWifiManager", logLevel);
    LogComponentEnable("MinstrelWifiManager", logLevel);
    LogComponentEnable("MpduAggregator", logLevel);
    LogComponentEnable("MsduAggregator", logLevel);
    LogComponentEnable("MultiUserScheduler", logLevel);
    LogComponentEnable("NistErrorRateModel", logLevel);
    LogComponentEnable("ObssPdAlgorithm", logLevel);
    LogComponentEnable("OfdmPhy", logLevel);
    LogComponentEnable("OfdmPpdu", logLevel);
    LogComponentEnable("OnoeWifiManager", logLevel);
    LogComponentEnable("OriginatorBlockAckAgreement", logLevel);
    LogComponentEnable("ParfWifiManager", logLevel);
    LogComponentEnable("PhyEntity", logLevel);
    LogComponentEnable("QosFrameExchangeManager", logLevel);
    LogComponentEnable("QosTxop", logLevel);
    LogComponentEnable("RecipientBlockAckAgreement", logLevel);
    LogComponentEnable("RrMultiUserScheduler", logLevel);
    LogComponentEnable("RraaWifiManager", logLevel);
    LogComponentEnable("RrpaaWifiManager", logLevel);
    LogComponentEnable("SimpleFrameCaptureModel", logLevel);
    LogComponentEnable("SpectrumWifiPhy", logLevel);
    LogComponentEnable("StaWifiMac", logLevel);
    LogComponentEnable("SupportedRates", logLevel);
    LogComponentEnable("TableBasedErrorRateModel", logLevel);
    LogComponentEnable("ThompsonSamplingWifiManager", logLevel);
    LogComponentEnable("ThresholdPreambleDetectionModel", logLevel);
    LogComponentEnable("Txop", logLevel);
    LogComponentEnable("VhtConfiguration", logLevel);
    LogComponentEnable("VhtFrameExchangeManager", logLevel);
    LogComponentEnable("VhtPhy", logLevel);
    LogComponentEnable("VhtPpdu", logLevel);
    LogComponentEnable("WifiAckManager", logLevel);
    LogComponentEnable("WifiAssocManager", logLevel);
    LogComponentEnable("WifiDefaultAckManager", logLevel);
    LogComponentEnable("WifiDefaultAssocManager", logLevel);
    LogComponentEnable("WifiDefaultGcrManager", logLevel);
    LogComponentEnable("WifiDefaultProtectionManager", logLevel);
    LogComponentEnable("WifiMac", logLevel);
    LogComponentEnable("WifiMacQueue", logLevel);
    LogComponentEnable("WifiMpdu", logLevel);
    LogComponentEnable("WifiNetDevice", logLevel);
    LogComponentEnable("WifiPhyStateHelper", logLevel);
    LogComponentEnable("WifiPhyOperatingChannel", logLevel);
    LogComponentEnable("WifiPhy", logLevel);
    LogComponentEnable("WifiPpdu", logLevel);
    LogComponentEnable("WifiProtectionManager", logLevel);
    LogComponentEnable("WifiPsdu", logLevel);
    LogComponentEnable("WifiRadioEnergyModel", logLevel);
    LogComponentEnable("WifiRemoteStationManager", logLevel);
    LogComponentEnable("WifiSpectrumPhyInterface", logLevel);
    LogComponentEnable("WifiSpectrumSignalParameters", logLevel);
    LogComponentEnable("WifiSpectrumValueHelper", logLevel);
    LogComponentEnable("WifiTxCurrentModel", logLevel);
    LogComponentEnable("WifiTxParameters", logLevel);
    LogComponentEnable("WifiTxTimer", logLevel);
    LogComponentEnable("YansErrorRateModel", logLevel);
    LogComponentEnable("YansWifiChannel", logLevel);
    LogComponentEnable("YansWifiPhy", logLevel);

    LogComponentEnable("Athstats", logLevel);
    LogComponentEnable("WifiHelper", logLevel);
    LogComponentEnable("SpectrumWifiHelper", logLevel);
    LogComponentEnable("YansWifiHelper", logLevel);
}

int64_t
WifiHelper::AssignStreams(NetDeviceContainer c, int64_t stream)
{
    int64_t currentStream = stream;
    Ptr<NetDevice> netDevice;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        netDevice = (*i);
        if (auto wifi = DynamicCast<WifiNetDevice>(netDevice))
        {
            // Handle any random numbers in the PHY objects.
            for (auto& phy : wifi->GetPhys())
            {
                currentStream += phy->AssignStreams(currentStream);
            }

            // Handle any random numbers in the station managers.
            for (auto& manager : wifi->GetRemoteStationManagers())
            {
                currentStream += manager->AssignStreams(currentStream);
            }

            // Handle any random numbers in the MAC objects.
            auto mac = wifi->GetMac();
            PointerValue ptr;
            if (!mac->GetQosSupported())
            {
                mac->GetAttribute("Txop", ptr);
                auto txop = ptr.Get<Txop>();
                currentStream += txop->AssignStreams(currentStream);
            }
            else
            {
                mac->GetAttribute("VO_Txop", ptr);
                auto vo_txop = ptr.Get<QosTxop>();
                currentStream += vo_txop->AssignStreams(currentStream);

                mac->GetAttribute("VI_Txop", ptr);
                auto vi_txop = ptr.Get<QosTxop>();
                currentStream += vi_txop->AssignStreams(currentStream);

                mac->GetAttribute("BE_Txop", ptr);
                auto be_txop = ptr.Get<QosTxop>();
                currentStream += be_txop->AssignStreams(currentStream);

                mac->GetAttribute("BK_Txop", ptr);
                auto bk_txop = ptr.Get<QosTxop>();
                currentStream += bk_txop->AssignStreams(currentStream);
            }

            // if an AP, handle any beacon jitter
            if (auto apMac = DynamicCast<ApWifiMac>(mac); apMac)
            {
                currentStream += apMac->AssignStreams(currentStream);
            }
            // if a STA, handle any probe request jitter
            if (auto staMac = DynamicCast<StaWifiMac>(mac); staMac)
            {
                currentStream += staMac->AssignStreams(currentStream);
            }
        }
    }
    return (currentStream - stream);
}

} // namespace ns3
