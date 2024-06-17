/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Pavel Boyko <boyko@iitp.ru>
 */

#include "mesh-wifi-beacon.h"

#include "ns3/nstime.h"
#include "ns3/wifi-mac-header.h"

namespace ns3
{

MeshWifiBeacon::MeshWifiBeacon(Ssid ssid, AllSupportedRates rates, uint64_t us)
{
    m_header.Get<Ssid>() = ssid;
    m_header.Get<SupportedRates>() = rates.rates;
    m_header.Get<ExtendedSupportedRatesIE>() = rates.extendedRates;
    m_header.SetBeaconIntervalUs(us);
}

void
MeshWifiBeacon::AddInformationElement(Ptr<WifiInformationElement> ie)
{
    m_elements.AddInformationElement(ie);
}

Time
MeshWifiBeacon::GetBeaconInterval() const
{
    return MicroSeconds(m_header.GetBeaconIntervalUs());
}

Ptr<Packet>
MeshWifiBeacon::CreatePacket()
{
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(m_elements);
    packet->AddHeader(BeaconHeader());
    return packet;
}

WifiMacHeader
MeshWifiBeacon::CreateHeader(Mac48Address address, Mac48Address mpAddress)
{
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_MGT_BEACON);
    hdr.SetAddr1(Mac48Address::GetBroadcast());
    hdr.SetAddr2(address);
    hdr.SetAddr3(mpAddress);
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();
    return hdr;
}

} // namespace ns3
