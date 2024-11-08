/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */

#include "bridge-channel.h"

#include "ns3/log.h"

/**
 * @file
 * @ingroup bridge
 * ns3::BridgeChannel implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BridgeChannel");

NS_OBJECT_ENSURE_REGISTERED(BridgeChannel);

TypeId
BridgeChannel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BridgeChannel")
                            .SetParent<Channel>()
                            .SetGroupName("Bridge")
                            .AddConstructor<BridgeChannel>();
    return tid;
}

BridgeChannel::BridgeChannel()
    : Channel()
{
    NS_LOG_FUNCTION_NOARGS();
}

BridgeChannel::~BridgeChannel()
{
    NS_LOG_FUNCTION_NOARGS();

    for (auto iter = m_bridgedChannels.begin(); iter != m_bridgedChannels.end(); iter++)
    {
        *iter = nullptr;
    }
    m_bridgedChannels.clear();
}

void
BridgeChannel::AddChannel(Ptr<Channel> bridgedChannel)
{
    m_bridgedChannels.push_back(bridgedChannel);
}

std::size_t
BridgeChannel::GetNDevices() const
{
    uint32_t ndevices = 0;
    for (auto iter = m_bridgedChannels.begin(); iter != m_bridgedChannels.end(); iter++)
    {
        ndevices += (*iter)->GetNDevices();
    }
    return ndevices;
}

Ptr<NetDevice>
BridgeChannel::GetDevice(std::size_t i) const
{
    std::size_t ndevices = 0;
    for (auto iter = m_bridgedChannels.begin(); iter != m_bridgedChannels.end(); iter++)
    {
        if ((i - ndevices) < (*iter)->GetNDevices())
        {
            return (*iter)->GetDevice(i - ndevices);
        }
        ndevices += (*iter)->GetNDevices();
    }
    return nullptr;
}

} // namespace ns3
