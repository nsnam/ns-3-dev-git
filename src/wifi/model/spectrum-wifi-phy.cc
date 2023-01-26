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
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *
 * Ported from yans-wifi-phy.cc by several contributors starting
 * with Nicola Baldo and Dean Armstrong
 */

#include "spectrum-wifi-phy.h"

#include "interference-helper.h"
#include "wifi-psdu.h"
#include "wifi-spectrum-phy-interface.h"
#include "wifi-spectrum-signal-parameters.h"
#include "wifi-utils.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/wifi-net-device.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SpectrumWifiPhy");

NS_OBJECT_ENSURE_REGISTERED(SpectrumWifiPhy);

TypeId
SpectrumWifiPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SpectrumWifiPhy")
            .SetParent<WifiPhy>()
            .SetGroupName("Wifi")
            .AddConstructor<SpectrumWifiPhy>()
            .AddAttribute("DisableWifiReception",
                          "Prevent Wi-Fi frame sync from ever happening",
                          BooleanValue(false),
                          MakeBooleanAccessor(&SpectrumWifiPhy::m_disableWifiReception),
                          MakeBooleanChecker())
            .AddAttribute(
                "TxMaskInnerBandMinimumRejection",
                "Minimum rejection (dBr) for the inner band of the transmit spectrum mask",
                DoubleValue(-20.0),
                MakeDoubleAccessor(&SpectrumWifiPhy::m_txMaskInnerBandMinimumRejection),
                MakeDoubleChecker<double>())
            .AddAttribute(
                "TxMaskOuterBandMinimumRejection",
                "Minimum rejection (dBr) for the outer band of the transmit spectrum mask",
                DoubleValue(-28.0),
                MakeDoubleAccessor(&SpectrumWifiPhy::m_txMaskOuterBandMinimumRejection),
                MakeDoubleChecker<double>())
            .AddAttribute(
                "TxMaskOuterBandMaximumRejection",
                "Maximum rejection (dBr) for the outer band of the transmit spectrum mask",
                DoubleValue(-40.0),
                MakeDoubleAccessor(&SpectrumWifiPhy::m_txMaskOuterBandMaximumRejection),
                MakeDoubleChecker<double>())
            .AddTraceSource("SignalArrival",
                            "Signal arrival",
                            MakeTraceSourceAccessor(&SpectrumWifiPhy::m_signalCb),
                            "ns3::SpectrumWifiPhy::SignalArrivalCallback");
    return tid;
}

SpectrumWifiPhy::SpectrumWifiPhy()
{
    NS_LOG_FUNCTION(this);
}

SpectrumWifiPhy::~SpectrumWifiPhy()
{
    NS_LOG_FUNCTION(this);
}

void
SpectrumWifiPhy::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_channel = nullptr;
    m_wifiSpectrumPhyInterface = nullptr;
    m_antenna = nullptr;
    m_rxSpectrumModel = nullptr;
    m_ruBands.clear();
    WifiPhy::DoDispose();
}

void
SpectrumWifiPhy::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    WifiPhy::DoInitialize();
    // This connection is deferred until frequency and channel width are set
    if (m_channel && m_wifiSpectrumPhyInterface)
    {
        m_channel->AddRx(m_wifiSpectrumPhyInterface);
    }
    else
    {
        NS_FATAL_ERROR("SpectrumWifiPhy misses channel and WifiSpectrumPhyInterface objects at "
                       "initialization time");
    }
}

Ptr<const SpectrumModel>
SpectrumWifiPhy::GetRxSpectrumModel()
{
    NS_LOG_FUNCTION(this);
    if (m_rxSpectrumModel)
    {
        return m_rxSpectrumModel;
    }
    else
    {
        if (GetFrequency() == 0)
        {
            NS_LOG_DEBUG("Frequency is not set; returning 0");
            return nullptr;
        }
        else
        {
            uint16_t channelWidth = GetChannelWidth();
            NS_LOG_DEBUG("Creating spectrum model from frequency/width pair of ("
                         << GetFrequency() << ", " << channelWidth << ")");
            m_rxSpectrumModel =
                WifiSpectrumValueHelper::GetSpectrumModel(GetFrequency(),
                                                          channelWidth,
                                                          GetBandBandwidth(),
                                                          GetGuardBandwidth(channelWidth));
            UpdateInterferenceHelperBands();
        }
    }
    return m_rxSpectrumModel;
}

void
SpectrumWifiPhy::UpdateInterferenceHelperBands()
{
    NS_LOG_FUNCTION(this);
    uint16_t channelWidth = GetChannelWidth();
    m_interference->RemoveBands();
    if (channelWidth < 20)
    {
        WifiSpectrumBand band = GetBand(channelWidth);
        m_interference->AddBand(band);
    }
    else
    {
        for (uint16_t bw = 160; bw >= 20; bw = bw / 2)
        {
            for (uint32_t i = 0; i < (channelWidth / bw); ++i)
            {
                m_interference->AddBand(GetBand(bw, i));
            }
        }
    }
    if (GetStandard() >= WIFI_STANDARD_80211ax)
    {
        // For a given RU type, some RUs over a channel occupy the same tones as
        // the corresponding RUs over a subchannel, while some others not. For instance,
        // the first nine 26-tone RUs over an 80 MHz channel occupy the same tones as
        // the first nine 26-tone RUs over the lowest 40 MHz subchannel. Therefore, we
        // need to store all the bands in a set (which removes duplicates) and then
        // pass the elements in the set to AddBand (to which we cannot pass duplicates)
        if (m_ruBands[channelWidth].empty())
        {
            for (uint16_t bw = 160; bw >= 20; bw = bw / 2)
            {
                for (uint32_t i = 0; i < (channelWidth / bw); ++i)
                {
                    for (uint32_t type = 0; type < 7; type++)
                    {
                        HeRu::RuType ruType = static_cast<HeRu::RuType>(type);
                        std::size_t nRus = HeRu::GetNRus(bw, ruType);
                        for (std::size_t phyIndex = 1; phyIndex <= nRus; phyIndex++)
                        {
                            HeRu::SubcarrierGroup group =
                                HeRu::GetSubcarrierGroup(bw, ruType, phyIndex);
                            HeRu::SubcarrierRange range =
                                std::make_pair(group.front().first, group.back().second);
                            WifiSpectrumBand band =
                                ConvertHeRuSubcarriers(bw,
                                                       GetGuardBandwidth(channelWidth),
                                                       range,
                                                       i);
                            std::size_t index =
                                (bw == 160 && phyIndex > nRus / 2 ? phyIndex - nRus / 2 : phyIndex);
                            bool primary80IsLower80 =
                                (GetOperatingChannel().GetPrimaryChannelIndex(20) < bw / 40);
                            bool primary80 = (bw < 160 || ruType == HeRu::RU_2x996_TONE ||
                                              (primary80IsLower80 && phyIndex <= nRus / 2) ||
                                              (!primary80IsLower80 && phyIndex > nRus / 2));
                            HeRu::RuSpec ru(ruType, index, primary80);
                            NS_ABORT_IF(ru.GetPhyIndex(bw,
                                                       GetOperatingChannel().GetPrimaryChannelIndex(
                                                           20)) != phyIndex);
                            m_ruBands[channelWidth].insert({band, ru});
                        }
                    }
                }
            }
        }
        for (const auto& bandRuPair : m_ruBands[channelWidth])
        {
            m_interference->AddBand(bandRuPair.first);
        }
    }
}

Ptr<Channel>
SpectrumWifiPhy::GetChannel() const
{
    return m_channel;
}

void
SpectrumWifiPhy::SetChannel(const Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
}

void
SpectrumWifiPhy::ResetSpectrumModel()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(IsInitialized(), "Executing method before run-time");
    uint16_t channelWidth = GetChannelWidth();
    NS_LOG_DEBUG("Run-time change of spectrum model from frequency/width pair of ("
                 << GetFrequency() << ", " << channelWidth << ")");
    // Replace existing spectrum model with new one, and must call AddRx ()
    // on the SpectrumChannel to provide this new spectrum model to it
    m_rxSpectrumModel = WifiSpectrumValueHelper::GetSpectrumModel(GetFrequency(),
                                                                  channelWidth,
                                                                  GetBandBandwidth(),
                                                                  GetGuardBandwidth(channelWidth));
    m_channel->AddRx(m_wifiSpectrumPhyInterface);
    UpdateInterferenceHelperBands();
}

void
SpectrumWifiPhy::DoChannelSwitch()
{
    NS_LOG_FUNCTION(this);
    WifiPhy::DoChannelSwitch();
    if (IsInitialized())
    {
        ResetSpectrumModel();
    }
}

bool
SpectrumWifiPhy::CanStartRx(Ptr<const WifiPpdu> ppdu, uint16_t txWidth) const
{
    return GetLatestPhyEntity()->CanStartRx(ppdu, txWidth);
}

void
SpectrumWifiPhy::StartRx(Ptr<SpectrumSignalParameters> rxParams)
{
    NS_LOG_FUNCTION(this << rxParams);
    Time rxDuration = rxParams->duration;
    Ptr<SpectrumValue> receivedSignalPsd = rxParams->psd;
    NS_LOG_DEBUG("Received signal with PSD " << *receivedSignalPsd << " and duration "
                                             << rxDuration.As(Time::NS));
    uint32_t senderNodeId = 0;
    if (rxParams->txPhy)
    {
        senderNodeId = rxParams->txPhy->GetDevice()->GetNode()->GetId();
    }
    NS_LOG_DEBUG("Received signal from " << senderNodeId << " with unfiltered power "
                                         << WToDbm(Integral(*receivedSignalPsd)) << " dBm");

    // Integrate over our receive bandwidth (i.e., all that the receive
    // spectral mask representing our filtering allows) to find the
    // total energy apparent to the "demodulator".
    // This is done per 20 MHz channel band.
    uint16_t channelWidth = GetChannelWidth();
    double totalRxPowerW = 0;
    RxPowerWattPerChannelBand rxPowerW;

    if ((channelWidth == 5) || (channelWidth == 10))
    {
        WifiSpectrumBand filteredBand = GetBand(channelWidth);
        double rxPowerPerBandW =
            WifiSpectrumValueHelper::GetBandPowerW(receivedSignalPsd, filteredBand);
        NS_LOG_DEBUG("Signal power received (watts) before antenna gain: " << rxPowerPerBandW);
        rxPowerPerBandW *= DbToRatio(GetRxGain());
        totalRxPowerW += rxPowerPerBandW;
        rxPowerW.insert({filteredBand, rxPowerPerBandW});
        NS_LOG_DEBUG("Signal power received after antenna gain for "
                     << channelWidth << " MHz channel: " << rxPowerPerBandW << " W ("
                     << WToDbm(rxPowerPerBandW) << " dBm)");
    }

    // 20 MHz is handled apart since the totalRxPowerW is computed through it
    for (uint16_t bw = 160; bw > 20; bw = bw / 2)
    {
        for (uint32_t i = 0; i < (channelWidth / bw); i++)
        {
            NS_ASSERT(channelWidth >= bw);
            WifiSpectrumBand filteredBand = GetBand(bw, i);
            double rxPowerPerBandW =
                WifiSpectrumValueHelper::GetBandPowerW(receivedSignalPsd, filteredBand);
            NS_LOG_DEBUG("Signal power received (watts) before antenna gain for "
                         << bw << " MHz channel band " << +i << ": " << rxPowerPerBandW);
            rxPowerPerBandW *= DbToRatio(GetRxGain());
            rxPowerW.insert({filteredBand, rxPowerPerBandW});
            NS_LOG_DEBUG("Signal power received after antenna gain for "
                         << bw << " MHz channel band " << +i << ": " << rxPowerPerBandW << " W ("
                         << WToDbm(rxPowerPerBandW) << " dBm)");
        }
    }

    for (uint32_t i = 0; i < (channelWidth / 20); i++)
    {
        WifiSpectrumBand filteredBand = GetBand(20, i);
        double rxPowerPerBandW =
            WifiSpectrumValueHelper::GetBandPowerW(receivedSignalPsd, filteredBand);
        NS_LOG_DEBUG("Signal power received (watts) before antenna gain for 20 MHz channel band "
                     << +i << ": " << rxPowerPerBandW);
        rxPowerPerBandW *= DbToRatio(GetRxGain());
        totalRxPowerW += rxPowerPerBandW;
        rxPowerW.insert({filteredBand, rxPowerPerBandW});
        NS_LOG_DEBUG("Signal power received after antenna gain for 20 MHz channel band "
                     << +i << ": " << rxPowerPerBandW << " W (" << WToDbm(rxPowerPerBandW)
                     << " dBm)");
    }

    if (GetStandard() >= WIFI_STANDARD_80211ax)
    {
        NS_ASSERT(!m_ruBands[channelWidth].empty());
        for (const auto& bandRuPair : m_ruBands[channelWidth])
        {
            double rxPowerPerBandW =
                WifiSpectrumValueHelper::GetBandPowerW(receivedSignalPsd, bandRuPair.first);
            NS_LOG_DEBUG("Signal power received (watts) before antenna gain for RU with type "
                         << bandRuPair.second.GetRuType() << " and index "
                         << bandRuPair.second.GetIndex() << " -> (" << bandRuPair.first.first
                         << "; " << bandRuPair.first.second << "): " << rxPowerPerBandW);
            rxPowerPerBandW *= DbToRatio(GetRxGain());
            NS_LOG_DEBUG("Signal power received after antenna gain for RU with type "
                         << bandRuPair.second.GetRuType() << " and index "
                         << bandRuPair.second.GetIndex() << " -> (" << bandRuPair.first.first
                         << "; " << bandRuPair.first.second << "): " << rxPowerPerBandW << " W ("
                         << WToDbm(rxPowerPerBandW) << " dBm)");
            rxPowerW.insert({bandRuPair.first, rxPowerPerBandW});
        }
    }

    NS_LOG_DEBUG("Total signal power received after antenna gain: "
                 << totalRxPowerW << " W (" << WToDbm(totalRxPowerW) << " dBm)");

    Ptr<WifiSpectrumSignalParameters> wifiRxParams =
        DynamicCast<WifiSpectrumSignalParameters>(rxParams);

    // Log the signal arrival to the trace source
    m_signalCb(bool(wifiRxParams), senderNodeId, WToDbm(totalRxPowerW), rxDuration);

    if (!wifiRxParams)
    {
        NS_LOG_INFO("Received non Wi-Fi signal");
        m_interference->AddForeignSignal(rxDuration, rxPowerW);
        SwitchMaybeToCcaBusy(nullptr);
        return;
    }
    if (wifiRxParams && m_disableWifiReception)
    {
        NS_LOG_INFO("Received Wi-Fi signal but blocked from syncing");
        m_interference->AddForeignSignal(rxDuration, rxPowerW);
        SwitchMaybeToCcaBusy(nullptr);
        return;
    }
    // Do no further processing if signal is too weak
    // Current implementation assumes constant RX power over the PPDU duration
    // Compare received TX power per MHz to normalized RX sensitivity
    const auto txWidth = wifiRxParams->txWidth;
    const auto& ppdu = GetRxPpduFromTxPpdu(wifiRxParams->ppdu);
    const auto& txVector = ppdu->GetTxVector();
    if (totalRxPowerW < DbmToW(GetRxSensitivity()) * (txWidth / 20.0))
    {
        NS_LOG_INFO("Received signal too weak to process: " << WToDbm(totalRxPowerW) << " dBm");
        m_interference->Add(ppdu, txVector, rxDuration, rxPowerW);
        SwitchMaybeToCcaBusy(nullptr);
        return;
    }

    if (wifiRxParams->txPhy)
    {
        if (!CanStartRx(ppdu, txWidth))
        {
            NS_LOG_INFO("Cannot start reception of the PPDU, consider it as interference");
            m_interference->Add(ppdu, txVector, rxDuration, rxPowerW);
            SwitchMaybeToCcaBusy(ppdu);
            return;
        }
    }

    NS_LOG_INFO("Received Wi-Fi signal");
    StartReceivePreamble(ppdu, rxPowerW, rxDuration);
}

Ptr<const WifiPpdu>
SpectrumWifiPhy::GetRxPpduFromTxPpdu(Ptr<const WifiPpdu> ppdu)
{
    return GetPhyEntityForPpdu(ppdu)->GetRxPpduFromTxPpdu(ppdu);
}

Ptr<Object>
SpectrumWifiPhy::GetAntenna() const
{
    return m_antenna;
}

void
SpectrumWifiPhy::SetAntenna(const Ptr<AntennaModel> a)
{
    NS_LOG_FUNCTION(this << a);
    m_antenna = a;
}

void
SpectrumWifiPhy::CreateWifiSpectrumPhyInterface(Ptr<NetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    m_wifiSpectrumPhyInterface = CreateObject<WifiSpectrumPhyInterface>();
    m_wifiSpectrumPhyInterface->SetSpectrumWifiPhy(this);
    m_wifiSpectrumPhyInterface->SetDevice(device);
}

void
SpectrumWifiPhy::StartTx(Ptr<const WifiPpdu> ppdu)
{
    NS_LOG_FUNCTION(this << ppdu);
    GetPhyEntity(ppdu->GetModulation())->StartTx(ppdu);
}

void
SpectrumWifiPhy::Transmit(Ptr<WifiSpectrumSignalParameters> txParams)
{
    NS_LOG_FUNCTION(this << txParams);

    // Finish configuration
    NS_ASSERT_MSG(m_wifiSpectrumPhyInterface,
                  "SpectrumPhy() is not set; maybe forgot to call CreateWifiSpectrumPhyInterface?");
    txParams->txPhy = m_wifiSpectrumPhyInterface->GetObject<SpectrumPhy>();
    txParams->txAntenna = m_antenna;

    m_channel->StartTx(txParams);
}

uint32_t
SpectrumWifiPhy::GetBandBandwidth() const
{
    uint32_t bandBandwidth = 0;
    switch (GetStandard())
    {
    case WIFI_STANDARD_80211a:
    case WIFI_STANDARD_80211g:
    case WIFI_STANDARD_80211b:
    case WIFI_STANDARD_80211n:
    case WIFI_STANDARD_80211ac:
        // Use OFDM subcarrier width of 312.5 KHz as band granularity
        bandBandwidth = 312500;
        break;
    case WIFI_STANDARD_80211p:
        if (GetChannelWidth() == 5)
        {
            // Use OFDM subcarrier width of 78.125 KHz as band granularity
            bandBandwidth = 78125;
        }
        else
        {
            // Use OFDM subcarrier width of 156.25 KHz as band granularity
            bandBandwidth = 156250;
        }
        break;
    case WIFI_STANDARD_80211ax:
    case WIFI_STANDARD_80211be:
        // Use OFDM subcarrier width of 78.125 KHz as band granularity
        bandBandwidth = 78125;
        break;
    default:
        NS_FATAL_ERROR("Standard unknown: " << GetStandard());
        break;
    }
    return bandBandwidth;
}

uint16_t
SpectrumWifiPhy::GetGuardBandwidth(uint16_t currentChannelWidth) const
{
    uint16_t guardBandwidth = 0;
    if (currentChannelWidth == 22)
    {
        // handle case of DSSS transmission
        guardBandwidth = 10;
    }
    else
    {
        // In order to properly model out of band transmissions for OFDM, the guard
        // band has been configured so as to expand the modeled spectrum up to the
        // outermost referenced point in "Transmit spectrum mask" sections' PSDs of
        // each PHY specification of 802.11-2016 standard. It thus ultimately corresponds
        // to the currently considered channel bandwidth (which can be different from
        // supported channel width).
        guardBandwidth = currentChannelWidth;
    }
    return guardBandwidth;
}

WifiSpectrumBand
SpectrumWifiPhy::GetBand(uint16_t bandWidth, uint8_t bandIndex)
{
    uint16_t channelWidth = GetChannelWidth();
    uint32_t bandBandwidth = GetBandBandwidth();
    size_t numBandsInChannel = static_cast<size_t>(channelWidth * 1e6 / bandBandwidth);
    size_t numBandsInBand = static_cast<size_t>(bandWidth * 1e6 / bandBandwidth);
    if (numBandsInBand % 2 == 0)
    {
        numBandsInChannel += 1; // symmetry around center frequency
    }
    size_t totalNumBands = GetRxSpectrumModel()->GetNumBands();
    NS_ASSERT_MSG((numBandsInChannel % 2 == 1) && (totalNumBands % 2 == 1),
                  "Should have odd number of bands");
    NS_ASSERT_MSG((bandIndex * bandWidth) < channelWidth, "Band index is out of bound");
    WifiSpectrumBand band;
    band.first = ((totalNumBands - numBandsInChannel) / 2) + (bandIndex * numBandsInBand);
    band.second = band.first + numBandsInBand - 1;
    if (band.first >= totalNumBands / 2)
    {
        // step past DC
        band.first += 1;
    }
    return band;
}

WifiSpectrumBand
SpectrumWifiPhy::ConvertHeRuSubcarriers(uint16_t bandWidth,
                                        uint16_t guardBandwidth,
                                        HeRu::SubcarrierRange range,
                                        uint8_t bandIndex) const
{
    WifiSpectrumBand convertedSubcarriers;
    uint32_t nGuardBands =
        static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / GetBandBandwidth()) + 0.5);
    uint32_t centerFrequencyIndex = 0;
    switch (bandWidth)
    {
    case 20:
        centerFrequencyIndex = (nGuardBands / 2) + 6 + 122;
        break;
    case 40:
        centerFrequencyIndex = (nGuardBands / 2) + 12 + 244;
        break;
    case 80:
        centerFrequencyIndex = (nGuardBands / 2) + 12 + 500;
        break;
    case 160:
        centerFrequencyIndex = (nGuardBands / 2) + 12 + 1012;
        break;
    default:
        NS_FATAL_ERROR("ChannelWidth " << bandWidth << " unsupported");
        break;
    }

    size_t numBandsInBand = static_cast<size_t>(bandWidth * 1e6 / GetBandBandwidth());
    centerFrequencyIndex += numBandsInBand * bandIndex;

    convertedSubcarriers.first = centerFrequencyIndex + range.first;
    convertedSubcarriers.second = centerFrequencyIndex + range.second;
    return convertedSubcarriers;
}

std::tuple<double, double, double>
SpectrumWifiPhy::GetTxMaskRejectionParams() const
{
    return std::make_tuple(m_txMaskInnerBandMinimumRejection,
                           m_txMaskOuterBandMinimumRejection,
                           m_txMaskOuterBandMaximumRejection);
}

} // namespace ns3
