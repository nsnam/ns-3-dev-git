/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "adhoc-wifi-mac.h"

#include "qos-txop.h"

#include "ns3/eht-capabilities.h"
#include "ns3/he-capabilities.h"
#include "ns3/ht-capabilities.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/vht-capabilities.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AdhocWifiMac");

NS_OBJECT_ENSURE_REGISTERED(AdhocWifiMac);

TypeId
AdhocWifiMac::GetTypeId()
{
    static TypeId tid = TypeId("ns3::AdhocWifiMac")
                            .SetParent<WifiMac>()
                            .SetGroupName("Wifi")
                            .AddConstructor<AdhocWifiMac>();
    return tid;
}

AdhocWifiMac::AdhocWifiMac()
{
    NS_LOG_FUNCTION(this);
    // Let the lower layers know that we are acting in an IBSS
    SetTypeOfStation(ADHOC_STA);
}

AdhocWifiMac::~AdhocWifiMac()
{
    NS_LOG_FUNCTION(this);
}

void
AdhocWifiMac::DoCompleteConfig()
{
    NS_LOG_FUNCTION(this);
}

bool
AdhocWifiMac::CanForwardPacketsTo(Mac48Address to) const
{
    return true;
}

void
AdhocWifiMac::Enqueue(Ptr<WifiMpdu> mpdu, Mac48Address to, Mac48Address from)
{
    NS_LOG_FUNCTION(this << *mpdu << to << from);

    if (GetWifiRemoteStationManager()->IsBrandNew(to))
    {
        // In ad hoc mode, we assume that every destination supports all the rates we support.
        if (GetHtSupported(SINGLE_LINK_OP_ID))
        {
            GetWifiRemoteStationManager()->AddAllSupportedMcs(to);
            GetWifiRemoteStationManager()->AddStationHtCapabilities(
                to,
                GetHtCapabilities(SINGLE_LINK_OP_ID));
        }
        if (GetVhtSupported(SINGLE_LINK_OP_ID))
        {
            GetWifiRemoteStationManager()->AddStationVhtCapabilities(
                to,
                GetVhtCapabilities(SINGLE_LINK_OP_ID));
        }
        if (GetHeSupported())
        {
            GetWifiRemoteStationManager()->AddStationHeCapabilities(
                to,
                GetHeCapabilities(SINGLE_LINK_OP_ID));
            if (Is6GhzBand(SINGLE_LINK_OP_ID))
            {
                GetWifiRemoteStationManager()->AddStationHe6GhzCapabilities(
                    to,
                    GetHe6GhzBandCapabilities(SINGLE_LINK_OP_ID));
            }
        }
        if (GetEhtSupported())
        {
            GetWifiRemoteStationManager()->AddStationEhtCapabilities(
                to,
                GetEhtCapabilities(SINGLE_LINK_OP_ID));
        }
        GetWifiRemoteStationManager()->AddAllSupportedModes(to);
        GetWifiRemoteStationManager()->RecordDisassociated(to);
    }

    auto& hdr = mpdu->GetHeader();

    hdr.SetAddr1(to);
    hdr.SetAddr2(GetAddress());
    hdr.SetAddr3(GetBssid(SINGLE_LINK_OP_ID));
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();

    auto txop = hdr.IsQosData() ? StaticCast<Txop>(GetQosTxop(hdr.GetQosTid())) : GetTxop();
    NS_ASSERT(txop);
    txop->Queue(mpdu);
}

void
AdhocWifiMac::SetLinkUpCallback(Callback<void> linkUp)
{
    NS_LOG_FUNCTION(this << &linkUp);
    WifiMac::SetLinkUpCallback(linkUp);

    // The approach taken here is that, from the point of view of a STA
    // in IBSS mode, the link is always up, so we immediately invoke the
    // callback if one is set
    linkUp();
}

void
AdhocWifiMac::Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << +linkId);
    const WifiMacHeader* hdr = &mpdu->GetHeader();
    NS_ASSERT(!hdr->IsCtl());
    Mac48Address from = hdr->GetAddr2();
    Mac48Address to = hdr->GetAddr1();
    if (GetWifiRemoteStationManager()->IsBrandNew(from))
    {
        // In ad hoc mode, we assume that every destination supports all the rates we support.
        if (GetHtSupported(SINGLE_LINK_OP_ID))
        {
            GetWifiRemoteStationManager()->AddAllSupportedMcs(from);
            GetWifiRemoteStationManager()->AddStationHtCapabilities(
                from,
                GetHtCapabilities(SINGLE_LINK_OP_ID));
        }
        if (GetVhtSupported(SINGLE_LINK_OP_ID))
        {
            GetWifiRemoteStationManager()->AddStationVhtCapabilities(
                from,
                GetVhtCapabilities(SINGLE_LINK_OP_ID));
        }
        if (GetHeSupported())
        {
            GetWifiRemoteStationManager()->AddStationHeCapabilities(
                from,
                GetHeCapabilities(SINGLE_LINK_OP_ID));
        }
        if (GetEhtSupported())
        {
            GetWifiRemoteStationManager()->AddStationEhtCapabilities(
                from,
                GetEhtCapabilities(SINGLE_LINK_OP_ID));
        }
        GetWifiRemoteStationManager()->AddAllSupportedModes(from);
        GetWifiRemoteStationManager()->RecordDisassociated(from);
    }
    if (hdr->IsData())
    {
        if (hdr->IsQosData() && hdr->IsQosAmsdu())
        {
            NS_LOG_DEBUG("Received A-MSDU from" << from);
            DeaggregateAmsduAndForward(mpdu);
        }
        else
        {
            ForwardUp(mpdu->GetPacket(), from, to);
        }
        return;
    }

    // Invoke the receive handler of our parent class to deal with any other frames
    WifiMac::Receive(mpdu, linkId);
}

} // namespace ns3
