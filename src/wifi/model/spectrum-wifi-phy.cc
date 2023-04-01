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
#include "ns3/he-phy.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-channel.h"
#include "ns3/wifi-net-device.h"

#include <algorithm>

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
                BooleanValue(false),
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
            .AddTraceSource("SignalArrival",
                            "Signal arrival",
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
    WifiPhy::DoDispose();
}

void
SpectrumWifiPhy::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    WifiPhy::DoInitialize();
}

void
SpectrumWifiPhy::UpdateInterferenceHelperBands()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(!m_spectrumPhyInterfaces.empty());
    uint16_t channelWidth = GetChannelWidth();
    WifiSpectrumPhyInterface::WifiSpectrumBands bands{};
    if (channelWidth < 20)
    {
        bands.push_back(GetBand(channelWidth));
    }
    else
    {
        for (uint16_t bw = 160; bw >= 20; bw = bw / 2)
        {
            for (uint32_t i = 0; i < (channelWidth / bw); ++i)
            {
                bands.push_back(GetBand(bw, i));
            }
        }
    }
    WifiSpectrumPhyInterface::WifiSpectrumBands allBands{bands};
    if (GetStandard() >= WIFI_STANDARD_80211ax)
    {
        auto&& ruBands = HePhy::GetRuBands(this, channelWidth, GetGuardBandwidth(channelWidth));
        for (const auto& bandRuPair : ruBands)
        {
            allBands.push_back(bandRuPair.first);
        }
        m_currentSpectrumPhyInterface->SetRuBands(std::move(ruBands));
    }

    m_currentSpectrumPhyInterface->SetBands(std::move(bands));

    if (m_interference->HasBands())
    {
        m_interference->UpdateBands(allBands, GetCurrentFrequencyRange());
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
SpectrumWifiPhy::ResetSpectrumModelIfNeeded()
{
    NS_LOG_FUNCTION(this);

    // We have to reset the spectrum model because we changed RF channel. Consequently,
    // we also have to add the spectrum interface to the spectrum channel again because
    // MultiModelSpectrumChannel keeps spectrum interfaces in a map indexed by the RX
    // spectrum model UID (which has changed after channel switching).
    // Both SingleModelSpectrumChannel and MultiModelSpectrumChannel ensure not to keep
    // duplicated spectrum interfaces (the latter removes the spectrum interface and adds
    // it again in the entry associated with the new RX spectrum model UID)

    const auto channelWidth = GetChannelWidth();
    const auto centerFrequency = GetFrequency();
    if (m_currentSpectrumPhyInterface->GetCenterFrequency() == centerFrequency)
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
        return;
    }

    // Replace existing spectrum model with new one
    m_currentSpectrumPhyInterface->SetRxSpectrumModel(centerFrequency,
                                                      channelWidth,
                                                      GetSubcarrierSpacing(),
                                                      GetGuardBandwidth(channelWidth));

    m_currentSpectrumPhyInterface->GetChannel()->AddRx(m_currentSpectrumPhyInterface);

    UpdateInterferenceHelperBands();
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
    ResetSpectrumModelIfNeeded();

    if (IsInitialized())
    {
        SwitchMaybeToCcaBusy(nullptr);
    }
}

bool
SpectrumWifiPhy::CanStartRx(Ptr<const WifiPpdu> ppdu, uint16_t txWidth) const
{
    return GetLatestPhyEntity()->CanStartRx(ppdu, txWidth);
}

void
SpectrumWifiPhy::StartRx(Ptr<SpectrumSignalParameters> rxParams,
                         Ptr<const WifiSpectrumPhyInterface> interface)
{
    NS_LOG_FUNCTION(this << rxParams << interface);
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
    const auto channelWidth = interface ? interface->GetChannelWidth() : GetChannelWidth();
    const auto& bands =
        interface ? interface->GetBands() : m_currentSpectrumPhyInterface->GetBands();
    double totalRxPowerW = 0;
    RxPowerWattPerChannelBand rxPowerW;

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
        rxPowerPerBandW *= DbToRatio(GetRxGain());
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
        const auto& ruBands =
            interface ? interface->GetRuBands() : m_currentSpectrumPhyInterface->GetRuBands();
        NS_ASSERT(!ruBands.empty());
        for (const auto& [band, ru] : ruBands)
        {
            double rxPowerPerBandW =
                WifiSpectrumValueHelper::GetBandPowerW(receivedSignalPsd, band.indices);
            NS_LOG_DEBUG("Signal power received (watts) before antenna gain for RU with type "
                         << ru.GetRuType() << " and index " << ru.GetIndex() << " -> ("
                         << band.indices.first << "; " << band.indices.second
                         << "): " << rxPowerPerBandW);
            rxPowerPerBandW *= DbToRatio(GetRxGain());
            NS_LOG_DEBUG("Signal power received after antenna gain for RU with type "
                         << ru.GetRuType() << " and index " << ru.GetIndex() << " -> ("
                         << band.indices.first << "; " << band.indices.second << "): "
                         << rxPowerPerBandW << " W (" << WToDbm(rxPowerPerBandW) << " dBm)");
            rxPowerW.insert({band, rxPowerPerBandW});
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

    if (m_trackSignalsInactiveInterfaces && interface &&
        (interface != m_currentSpectrumPhyInterface))
    {
        NS_LOG_INFO("Received Wi-Fi signal from a non-active PHY interface");
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
SpectrumWifiPhy::GetBand(uint16_t bandWidth, uint8_t bandIndex /* = 0 */)
{
    auto channelWidth = GetChannelWidth();
    auto subcarrierSpacing = GetSubcarrierSpacing();
    auto numBandsInChannel = static_cast<size_t>(channelWidth * 1e6 / subcarrierSpacing);
    auto numBandsInBand = static_cast<size_t>(bandWidth * 1e6 / subcarrierSpacing);
    if (numBandsInBand % 2 == 0)
    {
        numBandsInChannel += 1; // symmetry around center frequency
    }
    NS_ABORT_IF(!m_currentSpectrumPhyInterface);
    auto rxSpectrumModel = m_currentSpectrumPhyInterface->GetRxSpectrumModel();
    size_t totalNumBands = rxSpectrumModel->GetNumBands();
    NS_ASSERT_MSG((numBandsInChannel % 2 == 1) && (totalNumBands % 2 == 1),
                  "Should have odd number of bands");
    NS_ASSERT_MSG((bandIndex * bandWidth) < channelWidth, "Band index is out of bound");
    NS_ASSERT(totalNumBands >= numBandsInChannel);
    auto startIndex = ((totalNumBands - numBandsInChannel) / 2) + (bandIndex * numBandsInBand);
    auto stopIndex = startIndex + numBandsInBand - 1;
    auto frequencies = ConvertIndicesToFrequencies({startIndex, stopIndex});
    auto freqRange = m_currentSpectrumPhyInterface->GetFrequencyRange();
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

WifiSpectrumBandFrequencies
SpectrumWifiPhy::ConvertIndicesToFrequencies(const WifiSpectrumBandIndices& indices) const
{
    NS_ABORT_IF(!m_currentSpectrumPhyInterface);
    auto rxSpectrumModel = m_currentSpectrumPhyInterface->GetRxSpectrumModel();
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

} // namespace ns3
