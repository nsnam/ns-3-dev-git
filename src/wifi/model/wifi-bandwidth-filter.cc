/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli "Federico II"
 * Copyright (c) 2022 University of Washington (port logic to WifiBandwidthFilter)
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
 */

#include "wifi-bandwidth-filter.h"

#include "spectrum-wifi-phy.h"
#include "wifi-psdu.h"
#include "wifi-spectrum-phy-interface.h"
#include "wifi-spectrum-signal-parameters.h"

#include <ns3/spectrum-transmit-filter.h>

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

    // WifiSpectrumPhyInterface (passed in) can be used to fetch SpectrumWifiPhy
    Ptr<const SpectrumWifiPhy> wifiPhy =
        DynamicCast<const WifiSpectrumPhyInterface>(receiverPhy)->GetSpectrumWifiPhy();

    NS_ASSERT_MSG(wifiPhy, "WifiPhy should be valid if WifiSpectrumSignalParameters was found");

    // The signal power is spread over a frequency interval that includes a guard
    // band on the left and a guard band on the right of the nominal TX band

    const uint16_t rxCenterFreq = wifiRxParams->ppdu->GetTxCenterFreq();
    const uint16_t rxWidth = wifiRxParams->ppdu->GetTxVector().GetChannelWidth();
    const uint16_t guardBandwidth = wifiPhy->GetGuardBandwidth(rxWidth);
    const uint16_t operatingFrequency = wifiPhy->GetOperatingChannel().GetFrequency();
    const uint16_t operatingChannelWidth = wifiPhy->GetOperatingChannel().GetWidth();

    const uint16_t rxMinFreq = rxCenterFreq - rxWidth / 2 - guardBandwidth;
    const uint16_t rxMaxFreq = rxCenterFreq + rxWidth / 2 + guardBandwidth;

    const uint16_t channelMinFreq = operatingFrequency - operatingChannelWidth / 2;
    const uint16_t channelMaxFreq = operatingFrequency + operatingChannelWidth / 2;

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
    auto filter = (rxMinFreq >= channelMaxFreq || rxMaxFreq <= channelMinFreq);
    NS_LOG_DEBUG("Returning " << filter);
    return filter;
}

} // namespace ns3
