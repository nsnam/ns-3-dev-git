/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#ifndef DEVICE_COEX_MANAGER_H
#define DEVICE_COEX_MANAGER_H

#include "ns3/object.h"

namespace ns3
{
namespace coex
{

class Arbitrator;

/**
 * @ingroup coex
 *
 * Base class for a Coex Manager. Devices that can handle notifications from a Coex Arbitrator
 * have to define their own Coex Manager subclass.
 */
class DeviceManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    ~DeviceManager() override;

    /**
     * Set the Coex Arbitrator for the node.
     *
     * @param coexArbitrator the Coex Arbitrator for the node
     */
    void SetCoexArbitrator(Ptr<Arbitrator> coexArbitrator);

    /**
     * @return the Coex Arbitrator for the node
     */
    Ptr<Arbitrator> GetCoexArbitrator() const;

  protected:
    void DoDispose() override;

    Ptr<Arbitrator> m_coexArbitrator; ///< the Coex Arbitrator for the node
};

} // namespace coex
} // namespace ns3

#endif /* DEVICE_COEX_MANAGER_H */
