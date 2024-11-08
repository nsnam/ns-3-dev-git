/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef MESH_STACK_INSTALLER_H
#define MESH_STACK_INSTALLER_H
#include "ns3/mesh-point-device.h"

namespace ns3
{
/**
 * @ingroup mesh
 *
 * @brief Prototype for class, which helps to install MAC-layer
 * routing stack to ns3::MeshPointDevice
 *
 * You need to create a MeshPointDevice and attach all
 * interfaces to it, than call Install method
 */
class MeshStack : public Object
{
  public:
    /// @brief Register this type.
    /// @return The TypeId.
    static TypeId GetTypeId();

    /**
     * @brief Installs mesh stack. needed by helper only
     * @param mp the mesh point device
     * @returns true if successful
     */
    virtual bool InstallStack(Ptr<MeshPointDevice> mp) = 0;
    /**
     * Report statistics of a given mesh point
     * @param mp the mesh point device
     * @param os the output stream
     */
    virtual void Report(const Ptr<MeshPointDevice> mp, std::ostream& os) = 0;
    /**
     * Reset statistics of a given mesh point
     * @param mp the mesh point device
     */
    virtual void ResetStats(const Ptr<MeshPointDevice> mp) = 0;
};
} // namespace ns3
#endif
