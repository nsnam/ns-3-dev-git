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
#include "wifi-net-device.h"
#include "wifi-psdu.h"
#include "wifi-spectrum-phy-interface.h"
#include "wifi-spectrum-signal-parameters.h"
#include "wifi-utils.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/he-phy.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-channel.h"

#include <algorithm>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_PHY_NS_LOG_APPEND_CONTEXT(Ptr(this))

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
                "TrackSignalsFromInactiveInterfaces",
                "Enable or disable tracking signals coming from inactive spectrum PHY interfaces",
                BooleanValue(true),
                MakeBooleanAccessor(&SpectrumWifiPhy::m_trackSignalsInactiveInterfaces),
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
            .AddTraceSource(
                "SignalArrival",
                "Trace start of all signal arrivals, including weak and foreign signals",
                MakeTraceSourceAccessor(&SpectrumWifiPhy::m_signalCb),
                "ns3::SpectrumWifiPhy::SignalArrivalCallback");
    return tid;
}

SpectrumWifiPhy::SpectrumWifiPhy()
    : m_spectrumPhyInterfaces{},
      m_currentSpectrumPhyInterface{nullptr}
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
    m_spectrumPhyInterfaces.clear();
    m_currentSpectrumPhyInterface = nullptr;
    m_antenna = nullptr;
    m_channelSwitchedCallback.Nullify();
    WifiPhy::DoDispose();
}

void
SpectrumWifiPhy::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    WifiPhy::DoInitialize();
}

WifiSpectrumBands
SpectrumWifiPhy::ComputeBands(Ptr<WifiSpectrumPhyInterface> spectrumPhyInterface)
{
    NS_LOG_FUNCTION(this << spectrumPhyInterface);
    WifiSpectrumBands bands{};
    const auto channelWidth = spectrumPhyInterface->GetChannelWidth();
    if (channelWidth < 20)
    {
        bands.push_back(GetBandForInterface(spectrumPhyInterface, channelWidth));
    }
    else
    {
        for (uint16_t bw = channelWidth; bw >= 20; bw = bw / 2)
        {
            for (uint32_t i = 0; i < (channelWidth / bw); ++i)
            {
                bands.push_back(GetBandForInterface(spectrumPhyInterface, bw, i));
            }
        }
    }
    return bands;
}

HeRuBands
SpectrumWifiPhy::GetHeRuBands(Ptr<WifiSpectrumPhyInterface> spectrumPhyInterface,
                              uint16_t guardBandwidth)
{
    HeRuBands heRuBands{};
    const auto channelWidth = spectrumPhyInterface->GetChannelWidth();
    for (uint16_t bw = channelWidth; bw >= 20; bw = bw / 2)
    {
        for (uint32_t i = 0; i < (channelWidth / bw); ++i)
        {
            for (uint32_t type = 0; type < 7; type++)
            {
                auto ruType = static_cast<HeRu::RuType>(type);
                std::size_t nRus = HeRu::GetNRus(bw, ruType);
                for (std::size_t phyIndex = 1; phyIndex <= nRus; phyIndex++)
                {
                    HeRu::SubcarrierGroup group = HeRu::GetSubcarrierGroup(bw, ruType, phyIndex);
                    HeRu::SubcarrierRange subcarrierRange =
                        std::make_pair(group.front().first, group.back().second);
                    const auto bandIndices = HePhy::ConvertHeRuSubcarriers(bw,
                                                                           guardBandwidth,
                                                                           GetSubcarrierSpacing(),
                                                                           subcarrierRange,
                                                                           i);
                    const auto bandFrequencies =
                        ConvertIndicesToFrequenciesForInterface(spectrumPhyInterface, bandIndices);
                    WifiSpectrumBandInfo band = {bandIndices, bandFrequencies};
                    std::size_t index =
                        (bw == 160 && phyIndex > nRus / 2 ? phyIndex - nRus / 2 : phyIndex);
                    const auto p20Index = GetOperatingChannel().GetPrimaryChannelIndex(20);
                    bool primary80IsLower80 = (p20Index < bw / 40);
                    bool primary80 = (bw < 160 || ruType == HeRu::RU_2x996_TONE ||
                                      (primary80IsLower80 && phyIndex <= nRus / 2) ||
                                      (!primary80IsLower80 && phyIndex > nRus / 2));
                    HeRu::RuSpec ru(ruType, index, primary80);
                    NS_ABORT_IF(ru.GetPhyIndex(bw, p20Index) != phyIndex);
                    heRuBands.insert({band, ru});
                }
            }
        }
    }
    return heRuBands;
}

void
SpectrumWifiPhy::UpdateInterferenceHelperBands(Ptr<WifiSpectrumPhyInterface> spectrumPhyInterface)
{
    NS_LOG_FUNCTION(this << spectrumPhyInterface);
    auto&& bands = ComputeBands(spectrumPhyInterface);
    WifiSpectrumBands allBands{bands};
    if (GetStandard() >= WIFI_STANDARD_80211ax)
    {
        const auto channelWidth = spectrumPhyInterface->GetChannelWidth();
        auto&& heRuBands = GetHeRuBands(spectrumPhyInterface, GetGuardBandwidth(channelWidth));
        for (const auto& bandRuPair : heRuBands)
        {
            allBands.push_back(bandRuPair.first);
        }
        spectrumPhyInterface->SetHeRuBands(std::move(heRuBands));
    }

    spectrumPhyInterface->SetBands(std::move(bands));

    if (m_interference->HasBands())
    {
        m_interference->UpdateBands(allBands, spectrumPhyInterface->GetFrequencyRange());
    }
    else
    {
        for (const auto& band : allBands)
        {
            m_interference->AddBand(band);
        }
    }
}

Ptr<Channel>
SpectrumWifiPhy::GetChannel() const
{
    NS_ABORT_IF(!m_currentSpectrumPhyInterface);
    return m_currentSpectrumPhyInterface->GetChannel();
}

void
SpectrumWifiPhy::AddChannel(const Ptr<SpectrumChannel> channel, const FrequencyRange& freqRange)
{
    NS_LOG_FUNCTION(this << channel << freqRange);

    const auto foundOverlappingChannel =
        std::any_of(m_spectrumPhyInterfaces.cbegin(),
                    m_spectrumPhyInterfaces.cend(),
                    [freqRange, channel](const auto& item) {
                        const auto spectrumRange = item.first;
                        const auto noOverlap =
                            ((freqRange.minFrequency >= spectrumRange.maxFrequency) ||
                             (freqRange.maxFrequency <= spectrumRange.minFrequency));
                        return (!noOverlap);
                    });
    NS_ABORT_MSG_IF(foundOverlappingChannel,
                    "Added a wifi spectrum channel that overlaps with another existing wifi "
                    "spectrum channel");

    auto wifiSpectrumPhyInterface = CreateObject<WifiSpectrumPhyInterface>(freqRange);
    wifiSpectrumPhyInterface->SetSpectrumWifiPhy(this);
    wifiSpectrumPhyInterface->SetChannel(channel);
    if (GetDevice())
    {
        wifiSpectrumPhyInterface->SetDevice(GetDevice());
    }
    m_spectrumPhyInterfaces.emplace(freqRange, wifiSpectrumPhyInterface);
}

void
SpectrumWifiPhy::ResetSpectrumModel(Ptr<WifiSpectrumPhyInterface> spectrumPhyInterface,
                                    uint16_t centerFrequency,
                                    uint16_t channelWidth)
{
    NS_LOG_FUNCTION(this << spectrumPhyInterface << centerFrequency << channelWidth);

    // We have to reset the spectrum model because we changed RF channel. Consequently,
    // we also have to add the spectrum interface to the spectrum channel again because
    // MultiModelSpectrumChannel keeps spectrum interfaces in a map indexed by the RX
    // spectrum model UID (which has changed after channel switching).
    // Both SingleModelSpectrumChannel and MultiModelSpectrumChannel ensure not to keep
    // duplicated spectrum interfaces (the latter removes the spectrum interface and adds
    // it again in the entry associated with the new RX spectrum model UID)

    // Replace existing spectrum model with new one
    spectrumPhyInterface->SetRxSpectrumModel(centerFrequency,
                                             channelWidth,
                                             GetSubcarrierSpacing(),
                                             GetGuardBandwidth(channelWidth));

    spectrumPhyInterface->GetChannel()->AddRx(spectrumPhyInterface);

    UpdateInterferenceHelperBands(spectrumPhyInterface);
}

void
SpectrumWifiPhy::DoChannelSwitch()
{
    NS_LOG_FUNCTION(this);
    const auto frequencyBefore = GetOperatingChannel().IsSet() ? GetFrequency() : 0;
    const auto widthBefore = GetOperatingChannel().IsSet() ? GetChannelWidth() : 0;
    WifiPhy::DoChannelSwitch();
    const auto frequencyAfter = GetFrequency();
    const auto widthAfter = GetChannelWidth();
    if ((frequencyBefore == frequencyAfter) && (widthBefore == widthAfter))
    {
        NS_LOG_DEBUG("Same RF channel as before, do nothing");
        if (IsInitialized())
        {
            SwitchMaybeToCcaBusy(nullptr);
        }
        return;
    }

    auto newSpectrumPhyInterface = GetInterfaceCoveringChannelBand(frequencyAfter, widthAfter);
    const auto interfaceChanged = (newSpectrumPhyInterface != m_currentSpectrumPhyInterface);

    NS_ABORT_MSG_IF(!newSpectrumPhyInterface,
                    "No spectrum channel covers frequency range ["
                        << frequencyAfter - (widthAfter / 2) << " MHz - "
                        << frequencyAfter + (widthAfter / 2) << " MHz]");

    if (interfaceChanged)
    {
        NS_LOG_DEBUG("Switch to existing RF interface with frequency/width pair of ("
                     << frequencyAfter << ", " << widthAfter << ")");
        if (m_currentSpectrumPhyInterface && !m_trackSignalsInactiveInterfaces)
        {
            m_currentSpectrumPhyInterface->GetChannel()->RemoveRx(m_currentSpectrumPhyInterface);
        }
    }

    m_currentSpectrumPhyInterface = newSpectrumPhyInterface;

    auto reset = true;
    if (m_currentSpectrumPhyInterface->GetCenterFrequency() == frequencyAfter)
    {
        // Center frequency has not changed for that interface, hence we do not need to
        // reset the spectrum model nor update any band stored in the interference helper
        if (!m_trackSignalsInactiveInterfaces)
        {
            // If we are not tracking signals from inactive interface,
            // this means the spectrum interface has been disconnected
            // from the spectrum channel and has to be connected back
            m_currentSpectrumPhyInterface->GetChannel()->AddRx(m_currentSpectrumPhyInterface);
        }
        reset = false;
    }

    if (reset)
    {
        ResetSpectrumModel(m_currentSpectrumPhyInterface, frequencyAfter, widthAfter);
    }

    if (IsInitialized())
    {
        SwitchMaybeToCcaBusy(nullptr);
    }

    Simulator::ScheduleNow(&SpectrumWifiPhy::NotifyChannelSwitched, this);
}

void
SpectrumWifiPhy::NotifyChannelSwitched()
{
    if (!m_channelSwitchedCallback.IsNull())
    {
        m_channelSwitchedCallback();
    }
}

void
SpectrumWifiPhy::ConfigureInterface(uint16_t frequency, uint16_t width)
{
    NS_LOG_FUNCTION(this << frequency << width);

    if (!m_trackSignalsInactiveInterfaces)
    {
        NS_LOG_DEBUG("Tracking of signals on inactive interfaces is not enabled");
        return;
    }

    auto spectrumPhyInterface = GetInterfaceCoveringChannelBand(frequency, width);

    NS_ABORT_MSG_IF(!spectrumPhyInterface,
                    "No spectrum channel covers frequency range ["
                        << frequency - (width / 2) << " MHz - " << frequency + (width / 2)
                        << " MHz]");

    NS_ABORT_MSG_IF(spectrumPhyInterface == m_currentSpectrumPhyInterface,
                    "This method should not be called for the current interface");

    if ((frequency == spectrumPhyInterface->GetCenterFrequency()) &&
        (width == spectrumPhyInterface->GetChannelWidth()))
    {
        NS_LOG_DEBUG("Same RF channel as before on that interface, do nothing");
        return;
    }

    ResetSpectrumModel(spectrumPhyInterface, frequency, width);
}

bool
SpectrumWifiPhy::CanStartRx(Ptr<const WifiPpdu> ppdu) const
{
    return GetLatestPhyEntity()->CanStartRx(ppdu);
}

void
SpectrumWifiPhy::StartRx(Ptr<SpectrumSignalParameters> rxParams,
                         Ptr<const WifiSpectrumPhyInterface> interface)
{
    NS_LOG_FUNCTION(this << rxParams << interface);
    Time rxDuration = rxParams->duration;
    Ptr<SpectrumValue> receivedSignalPsd = rxParams->psd;
    if (interface)
    {
        NS_ASSERT_MSG(receivedSignalPsd->GetValuesN() ==
                          interface->GetRxSpectrumModel()->GetNumBands(),
                      "Use multi model spectrum channel if PHYs have different spectrum models!");
    }
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
    const auto channelWidth = interface ? interface->GetChannelWidth() : GetChannelWidth();
    const auto& bands =
        interface ? interface->GetBands() : m_currentSpectrumPhyInterface->GetBands();
    double totalRxPowerW = 0;
    RxPowerWattPerChannelBand rxPowerW;

    const auto rxGainRatio = DbToRatio(GetRxGain());

    std::size_t index = 0;
    uint16_t prevBw = 0;
    for (const auto& band : bands)
    {
        uint16_t bw = (band.frequencies.second - band.frequencies.first) / 1e6;
        NS_ASSERT(bw <= channelWidth);
        index = ((bw != prevBw) ? 0 : (index + 1));
        double rxPowerPerBandW =
            WifiSpectrumValueHelper::GetBandPowerW(receivedSignalPsd, band.indices);
        NS_LOG_DEBUG("Signal power received (watts) before antenna gain for "
                     << bw << " MHz channel band " << index << ": " << band);
        rxPowerPerBandW *= rxGainRatio;
        rxPowerW.insert({band, rxPowerPerBandW});
        NS_LOG_DEBUG("Signal power received after antenna gain for "
                     << bw << " MHz channel band " << index << ": " << rxPowerPerBandW << " W ("
                     << WToDbm(rxPowerPerBandW) << " dBm)");
        if (bw <= 20)
        {
            totalRxPowerW += rxPowerPerBandW;
        }
        prevBw = bw;
    }

    if (GetStandard() >= WIFI_STANDARD_80211ax)
    {
        const auto& heRuBands =
            interface ? interface->GetHeRuBands() : m_currentSpectrumPhyInterface->GetHeRuBands();
        NS_ASSERT(!heRuBands.empty());
        for (const auto& [band, ru] : heRuBands)
        {
            double rxPowerPerBandW =
                WifiSpectrumValueHelper::GetBandPowerW(receivedSignalPsd, band.indices);
            rxPowerPerBandW *= rxGainRatio;
            rxPowerW.insert({band, rxPowerPerBandW});
        }
    }

    NS_LOG_DEBUG("Total signal power received after antenna gain: "
                 << totalRxPowerW << " W (" << WToDbm(totalRxPowerW) << " dBm)");

    Ptr<WifiSpectrumSignalParameters> wifiRxParams =
        DynamicCast<WifiSpectrumSignalParameters>(rxParams);

    // Log the signal arrival to the trace source
    m_signalCb(rxParams, senderNodeId, WToDbm(totalRxPowerW), rxDuration);

    if (!wifiRxParams)
    {
        NS_LOG_INFO("Received non Wi-Fi signal");
        m_interference->AddForeignSignal(rxDuration,
                                         rxPowerW,
                                         interface ? interface->GetFrequencyRange()
                                                   : GetCurrentFrequencyRange());
        SwitchMaybeToCcaBusy(nullptr);
        return;
    }

    if (wifiRxParams && m_disableWifiReception)
    {
        NS_LOG_INFO("Received Wi-Fi signal but blocked from syncing");
        NS_ASSERT(interface);
        m_interference->AddForeignSignal(rxDuration, rxPowerW, interface->GetFrequencyRange());
        SwitchMaybeToCcaBusy(nullptr);
        return;
    }

    if (m_trackSignalsInactiveInterfaces && interface &&
        (interface != m_currentSpectrumPhyInterface))
    {
        NS_LOG_INFO("Received Wi-Fi signal from a non-active PHY interface "
                    << interface->GetFrequencyRange());
        m_interference->AddForeignSignal(rxDuration, rxPowerW, interface->GetFrequencyRange());
        SwitchMaybeToCcaBusy(nullptr);
        return;
    }

    // Do no further processing if signal is too weak
    // Current implementation assumes constant RX power over the PPDU duration
    // Compare received TX power per MHz to normalized RX sensitivity
    const auto ppdu = GetRxPpduFromTxPpdu(wifiRxParams->ppdu);
    if (totalRxPowerW < DbmToW(GetRxSensitivity()) * (ppdu->GetTxChannelWidth() / 20.0))
    {
        NS_LOG_INFO("Received signal too weak to process: " << WToDbm(totalRxPowerW) << " dBm");
        m_interference->Add(ppdu, rxDuration, rxPowerW, GetCurrentFrequencyRange());
        SwitchMaybeToCcaBusy(nullptr);
        return;
    }

    if (wifiRxParams->txPhy)
    {
        if (!CanStartRx(ppdu))
        {
            NS_LOG_INFO("Cannot start reception of the PPDU, consider it as interference");
            m_interference->Add(ppdu, rxDuration, rxPowerW, GetCurrentFrequencyRange());
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

Ptr<AntennaModel>
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
SpectrumWifiPhy::SetDevice(const Ptr<WifiNetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    WifiPhy::SetDevice(device);
    for (auto& spectrumPhyInterface : m_spectrumPhyInterfaces)
    {
        spectrumPhyInterface.second->SetDevice(device);
    }
}

void
SpectrumWifiPhy::StartTx(Ptr<const WifiPpdu> ppdu)
{
    NS_LOG_FUNCTION(this << ppdu);
    m_signalTransmissionCb(ppdu, ppdu->GetTxVector());
    GetPhyEntity(ppdu->GetModulation())->StartTx(ppdu);
}

void
SpectrumWifiPhy::Transmit(Ptr<WifiSpectrumSignalParameters> txParams)
{
    NS_LOG_FUNCTION(this << txParams);
    NS_ABORT_IF(!m_currentSpectrumPhyInterface);
    m_currentSpectrumPhyInterface->StartTx(txParams);
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

WifiSpectrumBandInfo
SpectrumWifiPhy::GetBandForInterface(Ptr<WifiSpectrumPhyInterface> spectrumPhyInterface,
                                     uint16_t bandWidth,
                                     uint8_t bandIndex /* = 0 */)
{
    const auto subcarrierSpacing = GetSubcarrierSpacing();
    const auto channelWidth = spectrumPhyInterface->GetChannelWidth();
    const auto numBandsInBand = static_cast<size_t>(bandWidth * 1e6 / subcarrierSpacing);
    auto numBandsInChannel = static_cast<size_t>(channelWidth * 1e6 / subcarrierSpacing);
    if (numBandsInBand % 2 == 0)
    {
        numBandsInChannel += 1; // symmetry around center frequency
    }
    auto rxSpectrumModel = spectrumPhyInterface->GetRxSpectrumModel();
    size_t totalNumBands = rxSpectrumModel->GetNumBands();
    NS_ASSERT_MSG((numBandsInChannel % 2 == 1) && (totalNumBands % 2 == 1),
                  "Should have odd number of bands");
    NS_ASSERT_MSG((bandIndex * bandWidth) < channelWidth, "Band index is out of bound");
    NS_ASSERT(totalNumBands >= numBandsInChannel);
    auto startIndex = ((totalNumBands - numBandsInChannel) / 2) + (bandIndex * numBandsInBand);
    auto stopIndex = startIndex + numBandsInBand - 1;
    auto frequencies =
        ConvertIndicesToFrequenciesForInterface(spectrumPhyInterface, {startIndex, stopIndex});
    auto freqRange = spectrumPhyInterface->GetFrequencyRange();
    NS_ASSERT(frequencies.first >= (freqRange.minFrequency * 1e6));
    NS_ASSERT(frequencies.second <= (freqRange.maxFrequency * 1e6));
    NS_ASSERT((frequencies.second - frequencies.first) == (bandWidth * 1e6));
    if (startIndex >= totalNumBands / 2)
    {
        // step past DC
        startIndex += 1;
    }
    return {{startIndex, stopIndex}, frequencies};
}

WifiSpectrumBandInfo
SpectrumWifiPhy::GetBand(uint16_t bandWidth, uint8_t bandIndex /* = 0 */)
{
    NS_ABORT_IF(!m_currentSpectrumPhyInterface);
    return GetBandForInterface(m_currentSpectrumPhyInterface, bandWidth, bandIndex);
}

WifiSpectrumBandFrequencies
SpectrumWifiPhy::ConvertIndicesToFrequencies(const WifiSpectrumBandIndices& indices) const
{
    NS_ABORT_IF(!m_currentSpectrumPhyInterface);
    return ConvertIndicesToFrequenciesForInterface(m_currentSpectrumPhyInterface, indices);
}

WifiSpectrumBandFrequencies
SpectrumWifiPhy::ConvertIndicesToFrequenciesForInterface(
    Ptr<WifiSpectrumPhyInterface> spectrumPhyInterface,
    const WifiSpectrumBandIndices& indices) const
{
    NS_ABORT_IF(!spectrumPhyInterface);
    auto rxSpectrumModel = spectrumPhyInterface->GetRxSpectrumModel();
    auto startGuardBand = rxSpectrumModel->Begin();
    auto startChannel = std::next(startGuardBand, indices.first);
    auto endChannel = std::next(startGuardBand, indices.second + 1);
    auto lowFreq = static_cast<uint64_t>(startChannel->fc);
    auto highFreq = static_cast<uint64_t>(endChannel->fc);
    return {lowFreq, highFreq};
}

std::tuple<double, double, double>
SpectrumWifiPhy::GetTxMaskRejectionParams() const
{
    return std::make_tuple(m_txMaskInnerBandMinimumRejection,
                           m_txMaskOuterBandMinimumRejection,
                           m_txMaskOuterBandMaximumRejection);
}

FrequencyRange
SpectrumWifiPhy::GetCurrentFrequencyRange() const
{
    NS_ABORT_IF(!m_currentSpectrumPhyInterface);
    return m_currentSpectrumPhyInterface->GetFrequencyRange();
}

const std::map<FrequencyRange, Ptr<WifiSpectrumPhyInterface>>&
SpectrumWifiPhy::GetSpectrumPhyInterfaces() const
{
    return m_spectrumPhyInterfaces;
}

Ptr<WifiSpectrumPhyInterface>
SpectrumWifiPhy::GetInterfaceCoveringChannelBand(uint16_t frequency, uint16_t width) const
{
    const auto lowFreq = frequency - (width / 2);
    const auto highFreq = frequency + (width / 2);
    const auto it = std::find_if(m_spectrumPhyInterfaces.cbegin(),
                                 m_spectrumPhyInterfaces.cend(),
                                 [lowFreq, highFreq](const auto& item) {
                                     return ((lowFreq >= item.first.minFrequency) &&
                                             (highFreq <= item.first.maxFrequency));
                                 });
    if (it == std::end(m_spectrumPhyInterfaces))
    {
        return nullptr;
    }
    return it->second;
}

Ptr<WifiSpectrumPhyInterface>
SpectrumWifiPhy::GetCurrentInterface() const
{
    return m_currentSpectrumPhyInterface;
}

void
SpectrumWifiPhy::SetChannelSwitchedCallback(Callback<void> callback)
{
    m_channelSwitchedCallback = callback;
}

} // namespace ns3
