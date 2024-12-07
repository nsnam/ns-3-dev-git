/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "yans-wifi-phy.h"

#include "interference-helper.h"
#include "yans-wifi-channel.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("YansWifiPhy");

NS_OBJECT_ENSURE_REGISTERED(YansWifiPhy);

TypeId
YansWifiPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::YansWifiPhy")
            .SetParent<WifiPhy>()
            .SetGroupName("Wifi")
            .AddConstructor<YansWifiPhy>()
            .AddTraceSource("SignalArrival",
                            "Trace start of all signal arrivals, including weak signals",
                            MakeTraceSourceAccessor(&YansWifiPhy::m_signalArrivalCb),
                            "ns3::YansWifiPhy::SignalArrivalCallback");
    return tid;
}

YansWifiPhy::YansWifiPhy()
{
    NS_LOG_FUNCTION(this);
}

void
YansWifiPhy::SetInterferenceHelper(const Ptr<InterferenceHelper> helper)
{
    WifiPhy::SetInterferenceHelper(helper);
    // add dummy band for Yans
    m_interference->AddBand({{{0, 0}},
                             {{
                                 Hz_u{0},
                                 Hz_u{0},
                             }}});
}

YansWifiPhy::~YansWifiPhy()
{
    NS_LOG_FUNCTION(this);
}

void
YansWifiPhy::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_channel = nullptr;
    WifiPhy::DoDispose();
}

Ptr<Channel>
YansWifiPhy::GetChannel() const
{
    return m_channel;
}

void
YansWifiPhy::SetChannel(const Ptr<YansWifiChannel> channel)
{
    NS_LOG_FUNCTION(this << channel);
    m_channel = channel;
    m_channel->Add(this);
}

void
YansWifiPhy::StartTx(Ptr<const WifiPpdu> ppdu)
{
    NS_LOG_FUNCTION(this << ppdu);
    NS_LOG_DEBUG("Start transmission: signal power before antenna gain="
                 << GetPower(ppdu->GetTxVector().GetTxPowerLevel()) << "dBm");
    m_signalTransmissionCb(ppdu, ppdu->GetTxVector());
    m_channel->Send(this, ppdu, GetTxPowerForTransmission(ppdu) + GetTxGain());
}

void
YansWifiPhy::TraceSignalArrival(Ptr<const WifiPpdu> ppdu, double rxPowerDbm, Time duration)
{
    NS_LOG_FUNCTION(this << ppdu);
    m_signalArrivalCb(ppdu, rxPowerDbm, ppdu->GetTxDuration());
}

MHz_u
YansWifiPhy::GetGuardBandwidth(MHz_u currentChannelWidth) const
{
    NS_ABORT_MSG("Guard bandwidth not relevant for Yans");
    return MHz_u{0};
}

std::tuple<dBr_u, dBr_u, dBr_u>
YansWifiPhy::GetTxMaskRejectionParams() const
{
    NS_ABORT_MSG("Tx mask rejection params not relevant for Yans");
    return std::make_tuple(dBr_u{0.0}, dBr_u{0.0}, dBr_u{0.0});
}

WifiSpectrumBandInfo
YansWifiPhy::GetBand(MHz_u /*bandWidth*/, uint8_t /*bandIndex*/)
{
    return {{{0, 0}},
            {{
                Hz_u{0},
                Hz_u{0},
            }}};
}

FrequencyRange
YansWifiPhy::GetCurrentFrequencyRange() const
{
    return WHOLE_WIFI_SPECTRUM;
}

WifiSpectrumBandFrequencies
YansWifiPhy::ConvertIndicesToFrequencies(const WifiSpectrumBandIndices& /*indices*/) const
{
    return {Hz_u{0}, Hz_u{0}};
}

void
YansWifiPhy::FinalizeChannelSwitch()
{
    NS_LOG_FUNCTION(this);
    NS_ABORT_MSG_IF(GetOperatingChannel().GetNSegments() > 1,
                    "operating channel made of non-contiguous segments cannot be used with Yans");
}

} // namespace ns3
