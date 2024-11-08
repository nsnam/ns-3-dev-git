/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef FLAME_INSTALLER_H
#define FLAME_INSTALLER_H

#include "ns3/mesh-stack-installer.h"

namespace ns3
{

/**
 * @ingroup flame
 *
 * @brief Helper class used to install FLAME mesh stack (actually single
 * protocol in this stack)
 */
class FlameStack : public MeshStack
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Construct a FlameStack helper class.
     */
    FlameStack();

    /**
     * Destroy a FlameStack helper class.
     */
    ~FlameStack() override;

    /**
     * Break any reference cycles in the installer helper.  Required for ns-3
     * Object support.
     */
    void DoDispose() override;

    /**
     * @brief Install a flame stack on the given MeshPointDevice
     * @param mp The Ptr<MeshPointDevice> to use.
     * @return true if successful
     */
    bool InstallStack(Ptr<MeshPointDevice> mp) override;

    /**
     * @brief Print flame protocol statistics.
     * @param mp The Ptr<MeshPointDevice> to use.
     * @param os The output stream
     */
    void Report(const Ptr<MeshPointDevice> mp, std::ostream&) override;

    /**
     * @brief Reset the statistics.
     * @param mp The Ptr<MeshPointDevice> to use.
     */
    void ResetStats(const Ptr<MeshPointDevice> mp) override;
};

} // namespace ns3

#endif /* FLAME_INSTALLER_H */
