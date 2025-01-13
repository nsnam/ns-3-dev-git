/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli "Federico II"
 * Copyright (c) 2022 University of Washington (port logic to WifiBandwidthFilter)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "wifi-bandwidth-filter.h"

#include "spectrum-wifi-phy.h"
#include "wifi-psdu.h"
#include "wifi-spectrum-phy-interface.h"
#include "wifi-spectrum-signal-parameters.h"

#include "ns3/boolean.h"
#include "ns3/spectrum-transmit-filter.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiBandwidthFilter");

NS_OBJECT_ENSURE_REGISTERED(WifiBandwidthFilter);

WifiBandwidthFilter::WifiBandwidthFilter()
{
    NS_LOG_FUNCTION(this);
}

TypeId
WifiBandwidthFilter::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiBandwidthFilter")
                            .SetParent<SpectrumTransmitFilter>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiBandwidthFilter>();
    return tid;
}

bool
WifiBandwidthFilter::DoFilter(Ptr<const SpectrumSignalParameters> params,
                              Ptr<const SpectrumPhy> receiverPhy)
{
    NS_LOG_FUNCTION(this << params);

    auto wifiRxParams = DynamicCast<const WifiSpectrumSignalParameters>(params);
    if (!wifiRxParams)
    {
        NS_LOG_DEBUG("Received a non Wi-Fi signal: do not filter");
        return false;
    }

    auto interface = DynamicCast<const WifiSpectrumPhyInterface>(receiverPhy);
    if (!interface)
    {
        NS_LOG_DEBUG("Sending a Wi-Fi signal to a non Wi-Fi device; do not filter");
        return false;
    }

    auto wifiPhy = interface->GetSpectrumWifiPhy();
    NS_ASSERT_MSG(wifiPhy,
                  "WifiPhy should be valid if WifiSpectrumSignalParameters was found and sending "
                  "to a WifiSpectrumPhyInterface");

    BooleanValue trackSignalsInactiveInterfaces;
    wifiPhy->GetAttribute("TrackSignalsFromInactiveInterfaces", trackSignalsInactiveInterfaces);

    NS_ASSERT_MSG(trackSignalsInactiveInterfaces.Get() ||
                      (interface == wifiPhy->GetCurrentInterface()),
                  "DoFilter should not be called for an inactive interface if "
                  "SpectrumWifiPhy::TrackSignalsFromInactiveInterfaces attribute is not enabled");

    NS_ASSERT((interface != wifiPhy->GetCurrentInterface()) ||
              (wifiPhy->GetOperatingChannel().GetTotalWidth() == interface->GetChannelWidth()));
    NS_ASSERT(
        (interface != wifiPhy->GetCurrentInterface()) ||
        (wifiPhy->GetOperatingChannel().GetFrequencies() == interface->GetCenterFrequencies()));

    // The signal power is spread over a frequency interval that includes a guard
    // band on the left and a guard band on the right of the nominal TX band
    const auto rxCenterFreqs = wifiRxParams->ppdu->GetTxCenterFreqs();
    // all segments have the same width
    const auto rxWidth =
        (wifiRxParams->ppdu->GetTxVector().GetChannelWidth() / rxCenterFreqs.size());
    const auto guardBandwidth = wifiPhy->GetGuardBandwidth(rxWidth);
    bool filter = true;
    for (auto rxCenterFreq : rxCenterFreqs)
    {
        const auto rxMinFreq = rxCenterFreq - rxWidth / 2 - guardBandwidth;
        const auto rxMaxFreq = rxCenterFreq + rxWidth / 2 + guardBandwidth;
        const auto operatingFrequencies = interface->GetCenterFrequencies();
        const auto operatingChannelWidth =
            interface->GetChannelWidth() / operatingFrequencies.size();
        for (auto operatingFrequency : operatingFrequencies)
        {
            const auto channelMinFreq = operatingFrequency - operatingChannelWidth / 2;
            const auto channelMaxFreq = operatingFrequency + operatingChannelWidth / 2;
            /**
             * The PPDU can be ignored if the two bands do not overlap.
             *
             * First non-overlapping case:
             *
             *                                        ┌─────────┬─────────┬─────────┐
             *                                PPDU    │  Guard  │ Nominal │  Guard  │
             *                                        │  Band   │   Band  │  Band   │
             *                                        └─────────┴─────────┴─────────┘
             *                                    rxMinFreq                     rxMaxFreq
             *
             * channelMinFreq                channelMaxFreq
             *         ┌──────────────────────────────┐
             *         │         Operating            │
             *         │           Channel            │
             *         └──────────────────────────────┘
             *
             * Second non-overlapping case:
             *
             *         ┌─────────┬─────────┬─────────┐
             * PPDU    │  Guard  │ Nominal │  Guard  │
             *         │  Band   │   Band  │  Band   │
             *         └─────────┴─────────┴─────────┘
             *     rxMinFreq                     rxMaxFreq
             *
             *                               channelMinFreq                channelMaxFreq
             *                                       ┌──────────────────────────────┐
             *                                       │         Operating            │
             *                                       │           Channel            │
             *                                       └──────────────────────────────┘
             */
            filter &= ((rxMinFreq >= channelMaxFreq) || (rxMaxFreq <= channelMinFreq));
        }
    }
    NS_LOG_DEBUG("Returning " << filter);
    return filter;
}

int64_t
WifiBandwidthFilter::DoAssignStreams(int64_t stream)
{
    return 0;
}

} // namespace ns3
