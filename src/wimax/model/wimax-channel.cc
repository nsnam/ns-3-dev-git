/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#include "wimax-channel.h"

#include "wimax-phy.h"

#include "ns3/assert.h"
#include "ns3/net-device.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WimaxChannel");

NS_OBJECT_ENSURE_REGISTERED(WimaxChannel);

TypeId
WimaxChannel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WimaxChannel").SetParent<Channel>().SetGroupName("Wimax");
    return tid;
}

WimaxChannel::WimaxChannel()
{
}

WimaxChannel::~WimaxChannel()
{
}

void
WimaxChannel::Attach(Ptr<WimaxPhy> phy)
{
    DoAttach(phy);
}

std::size_t
WimaxChannel::GetNDevices() const
{
    return DoGetNDevices();
}

Ptr<NetDevice>
WimaxChannel::GetDevice(std::size_t index) const
{
    return DoGetDevice(index);
}

} // namespace ns3
