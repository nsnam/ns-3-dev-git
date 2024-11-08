/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef DOT11S_STACK_INSTALLER_H
#define DOT11S_STACK_INSTALLER_H

#include "ns3/mesh-stack-installer.h"

namespace ns3
{

/**
 * @ingroup dot11s
 * @brief Helper class to allow easy installation of 802.11s stack.
 */
class Dot11sStack : public MeshStack
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Create a Dot11sStack() installer helper.
     */
    Dot11sStack();

    /**
     * Destroy a Dot11sStack() installer helper.
     */
    ~Dot11sStack() override;

    /**
     * Break any reference cycles in the installer helper.  Required for ns-3
     * Object support.
     */
    void DoDispose() override;

    /**
     * @brief Install an 802.11s stack.
     * @param mp The Ptr<MeshPointDevice> to use when setting up the PMP.
     * @return true if successful
     */
    bool InstallStack(Ptr<MeshPointDevice> mp) override;

    /**
     * @brief Iterate through the referenced devices and protocols and print
     * their statistics
     * @param mp The Ptr<MeshPointDevice> to use when setting up the PMP.
     * @param os The output stream
     */
    void Report(const Ptr<MeshPointDevice> mp, std::ostream&) override;

    /**
     * @brief Reset the statistics on the referenced devices and protocols.
     * @param mp The Ptr<MeshPointDevice> to use when setting up the PMP.
     */
    void ResetStats(const Ptr<MeshPointDevice> mp) override;

  private:
    Mac48Address m_root; ///< root
};

} // namespace ns3

#endif
