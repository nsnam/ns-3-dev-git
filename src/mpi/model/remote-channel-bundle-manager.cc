/*
 *  Copyright 2013. Lawrence Livermore National Security, LLC.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Steven Smith <smith84@llnl.gov>
 *
 */

/**
 * @file
 * @ingroup mpi
 * Implementation of class ns3::RemoteChannelBundleManager.
 */

#include "remote-channel-bundle-manager.h"

#include "null-message-simulator-impl.h"
#include "remote-channel-bundle.h"

#include "ns3/simulator.h"

namespace ns3
{

bool ns3::RemoteChannelBundleManager::g_initialized = false;
ns3::RemoteChannelBundleManager::RemoteChannelMap
    ns3::RemoteChannelBundleManager::g_remoteChannelBundles;

Ptr<RemoteChannelBundle>
RemoteChannelBundleManager::Find(uint32_t systemId)
{
    auto kv = g_remoteChannelBundles.find(systemId);

    if (kv == g_remoteChannelBundles.end())
    {
        return nullptr;
    }
    else
    {
        return kv->second;
    }
}

Ptr<RemoteChannelBundle>
RemoteChannelBundleManager::Add(uint32_t systemId)
{
    NS_ASSERT(!g_initialized);
    NS_ASSERT(g_remoteChannelBundles.find(systemId) == g_remoteChannelBundles.end());

    Ptr<RemoteChannelBundle> remoteChannelBundle = Create<RemoteChannelBundle>(systemId);

    g_remoteChannelBundles[systemId] = remoteChannelBundle;

    return remoteChannelBundle;
}

std::size_t
RemoteChannelBundleManager::Size()
{
    return g_remoteChannelBundles.size();
}

void
RemoteChannelBundleManager::InitializeNullMessageEvents()
{
    NS_ASSERT(!g_initialized);

    for (auto iter = g_remoteChannelBundles.begin(); iter != g_remoteChannelBundles.end(); ++iter)
    {
        Ptr<RemoteChannelBundle> bundle = iter->second;
        bundle->Send(bundle->GetDelay());

        NullMessageSimulatorImpl::GetInstance()->ScheduleNullMessageEvent(bundle);
    }

    g_initialized = true;
}

Time
RemoteChannelBundleManager::GetSafeTime()
{
    NS_ASSERT(g_initialized);

    Time safeTime = Simulator::GetMaximumSimulationTime();

    for (auto kv = g_remoteChannelBundles.begin(); kv != g_remoteChannelBundles.end(); ++kv)
    {
        safeTime = Min(safeTime, kv->second->GetGuaranteeTime());
    }

    return safeTime;
}

void
RemoteChannelBundleManager::Destroy()
{
    NS_ASSERT(g_initialized);

    g_remoteChannelBundles.clear();
    g_initialized = false;
}

} // namespace ns3
