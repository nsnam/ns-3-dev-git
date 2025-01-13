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
 * Declaration of class ns3::RemoteChannelBundleManager.
 */

#ifndef NS3_REMOTE_CHANNEL_BUNDLE_MANAGER
#define NS3_REMOTE_CHANNEL_BUNDLE_MANAGER

#include "ns3/nstime.h"
#include "ns3/ptr.h"

#include <unordered_map>

namespace ns3
{

class RemoteChannelBundle;

/**
 * @ingroup mpi
 *
 * @brief Singleton for managing the RemoteChannelBundles for each process.
 *
 * Manages collective tasks associated with the bundle collection.
 */
class RemoteChannelBundleManager
{
  public:
    /**
     * Get the bundle corresponding to a remote rank.
     *
     * @param [in] systemId The remote system id.
     * @return The bundle for the specified system id.
     */
    static Ptr<RemoteChannelBundle> Find(uint32_t systemId);

    /**
     * Add RemoteChannelBundle from this task to MPI task
     * on other side of the link.
     * Can not be invoked after InitializeNullMessageEvents has been invoked.
     *
     * @param [in] systemId The remote system id.
     * @return The newly added bundle.
     */
    static Ptr<RemoteChannelBundle> Add(uint32_t systemId);

    /**
     * Get the number of ns-3 channels in this bundle
     * @return The number of channels.
     */
    static std::size_t Size();

    /**
     * Setup initial Null Message events for every RemoteChannelBundle.
     * All RemoteChannelBundles should be added before this method is invoked.
     */
    static void InitializeNullMessageEvents();

    /**
     * Get the safe time across all channels in this bundle.
     * @return The safe time.
     */
    static Time GetSafeTime();

    /** Destroy the singleton. */
    static void Destroy();

  private:
    /**
     * Private ctor to prevent creation outside of singleton pattern.
     */
    RemoteChannelBundleManager()
    {
    }

    /**
     * Private dtor to prevent destruction outside of singleton pattern.
     */
    ~RemoteChannelBundleManager()
    {
    }

    /**
     * Container for all remote channel bundles for this task.
     *
     */
    typedef std::unordered_map<uint32_t, Ptr<RemoteChannelBundle>> RemoteChannelMap;
    /** The remote channel bundles. */
    static RemoteChannelMap g_remoteChannelBundles;

    /**
     * Protect manager class from being initialized twice or incorrect
     * ordering of method calls.
     */
    static bool g_initialized;
};

} // namespace ns3

#endif
