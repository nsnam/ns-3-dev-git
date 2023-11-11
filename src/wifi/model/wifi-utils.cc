/*
 * Copyright (c) 2016
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-utils.h"

#include "ap-wifi-mac.h"
#include "ctrl-headers.h"
#include "gcr-manager.h"
#include "wifi-mac-header.h"
#include "wifi-mac-trailer.h"

#include "ns3/packet.h"

#include <cmath>

namespace ns3
{

const Time WIFI_TU = MicroSeconds(WIFI_TU_US);

double
DbToRatio(dB_u val)
{
    return std::pow(10.0, 0.1 * val);
}

Watt_u
DbmToW(dBm_u val)
{
    return std::pow(10.0, 0.1 * (val - 30.0));
}

dBm_u
WToDbm(Watt_u val)
{
    NS_ASSERT(val > 0.);
    return 10.0 * std::log10(val) + 30.0;
}

dB_u
RatioToDb(double ratio)
{
    return 10.0 * std::log10(ratio);
}

uint32_t
GetAckSize()
{
    static const uint32_t size = WifiMacHeader(WIFI_MAC_CTL_ACK).GetSize() + 4;

    return size;
}

uint32_t
GetBlockAckSize(BlockAckType type)
{
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_BACKRESP);
    CtrlBAckResponseHeader blockAck;
    blockAck.SetType(type);
    return hdr.GetSize() + blockAck.GetSerializedSize() + 4;
}

uint32_t
GetBlockAckRequestSize(BlockAckReqType type)
{
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_BACKREQ);
    CtrlBAckRequestHeader bar;
    bar.SetType(type);
    return hdr.GetSize() + bar.GetSerializedSize() + 4;
}

uint32_t
GetMuBarSize(std::list<BlockAckReqType> types)
{
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_TRIGGER);
    CtrlTriggerHeader trigger;
    trigger.SetType(TriggerFrameType::MU_BAR_TRIGGER);
    for (auto& t : types)
    {
        auto userInfo = trigger.AddUserInfoField();
        CtrlBAckRequestHeader bar;
        bar.SetType(t);
        userInfo.SetMuBarTriggerDepUserInfo(bar);
    }
    return hdr.GetSize() + trigger.GetSerializedSize() + 4;
}

uint32_t
GetRtsSize()
{
    static const uint32_t size = WifiMacHeader(WIFI_MAC_CTL_RTS).GetSize() + 4;

    return size;
}

uint32_t
GetCtsSize()
{
    static const uint32_t size = WifiMacHeader(WIFI_MAC_CTL_CTS).GetSize() + 4;

    return size;
}

bool
IsInWindow(uint16_t seq, uint16_t winstart, uint16_t winsize)
{
    return ((seq - winstart + 4096) % 4096) < winsize;
}

void
AddWifiMacTrailer(Ptr<Packet> packet)
{
    WifiMacTrailer fcs;
    packet->AddTrailer(fcs);
}

uint32_t
GetSize(Ptr<const Packet> packet, const WifiMacHeader* hdr, bool isAmpdu)
{
    uint32_t size;
    WifiMacTrailer fcs;
    if (isAmpdu)
    {
        size = packet->GetSize();
    }
    else
    {
        size = packet->GetSize() + hdr->GetSize() + fcs.GetSerializedSize();
    }
    return size;
}

bool
TidToLinkMappingValidForNegType1(const WifiTidLinkMapping& dlLinkMapping,
                                 const WifiTidLinkMapping& ulLinkMapping)
{
    if (dlLinkMapping.empty() && ulLinkMapping.empty())
    {
        // default mapping is valid
        return true;
    }

    if (dlLinkMapping.size() != 8 || ulLinkMapping.size() != 8)
    {
        // not all TIDs have been mapped
        return false;
    }

    const auto& linkSet = dlLinkMapping.cbegin()->second;

    for (const auto& linkMapping : {std::cref(dlLinkMapping), std::cref(ulLinkMapping)})
    {
        for (const auto& [tid, links] : linkMapping.get())
        {
            if (links != linkSet)
            {
                // distinct link sets
                return false;
            }
        }
    }

    return true;
}

bool
IsGroupcast(const Mac48Address& adr)
{
    return adr.IsGroup() && !adr.IsBroadcast();
}

bool
IsGcr(Ptr<WifiMac> mac, const WifiMacHeader& hdr)
{
    auto apMac = DynamicCast<ApWifiMac>(mac);
    return apMac && apMac->UseGcr(hdr);
}

Mac48Address
GetIndividuallyAddressedRecipient(Ptr<WifiMac> mac, const WifiMacHeader& hdr)
{
    const auto isGcr = IsGcr(mac, hdr);
    const auto addr1 = hdr.GetAddr1();
    if (!isGcr)
    {
        return addr1;
    }
    auto apMac = DynamicCast<ApWifiMac>(mac);
    return apMac->GetGcrManager()->GetIndividuallyAddressedRecipient(addr1);
}

} // namespace ns3
