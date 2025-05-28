/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#ifndef COEX_ARBITRATOR_H
#define COEX_ARBITRATOR_H

#include "coex-types.h"

#include "ns3/object.h"

#include <map>

namespace ns3
{
class Node;

namespace coex
{
class DeviceManager;

/**
 * @defgroup coex Framework to handle coexistence events
 *
 * @brief an arbitrator for Coex Events
 *
 */

/**
 * @ingroup coex
 * @brief Coex Arbitrator base class
 *
 * A Coex Arbitrator handles requests for using a transmitting resource (antenna) shared among
 * multiple NetDevices.
 */
class Arbitrator : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    Arbitrator();
    ~Arbitrator() override;

    /**
     * Set the Coex Manager for the given Radio Access Technology (RAT).
     *
     * @param rat the given RAT
     * @param coexManager the Coex Manager
     */
    void SetDeviceCoexManager(Rat rat, Ptr<DeviceManager> coexManager);

    /**
     * Get the Coex Manager for the given RAT.
     *
     * @param rat the given RAT
     * @return the Coex Manager for the given RAT
     */
    Ptr<DeviceManager> GetDeviceCoexManager(Rat rat) const;

  protected:
    void NotifyNewAggregate() override;
    void DoDispose() override;

  private:
    Ptr<Node> m_node; ///< the node which this arbitrator is aggregated to
    std::map<Rat, Ptr<DeviceManager>>
        m_devCoexManagers; ///< a map of CoexManagers for the NetDevices on this node, indexed by
                           ///< the RAT corresponding to the NetDevice
};

} // namespace coex
} // namespace ns3

#endif /* COEX_ARBITRATOR_H */
