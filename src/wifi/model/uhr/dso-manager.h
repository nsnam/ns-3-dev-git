/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef DSO_MANAGER_H
#define DSO_MANAGER_H

#include "ns3/object.h"

namespace ns3
{

class UhrFrameExchangeManager;
class StaWifiMac;

/**
 * @ingroup wifi
 *
 * DsoManager is an abstract base class defining the API that UHR non-AP STA STAs with DSO activated
 * can use to handle the DSO operations.
 */
class DsoManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    DsoManager();
    ~DsoManager() override;

    /**
     * Set the wifi MAC. Note that it must be the MAC of an UHR non-AP STA.
     *
     * @param mac the wifi MAC
     */
    void SetWifiMac(Ptr<StaWifiMac> mac);

  protected:
    void DoDispose() override;

    /**
     * @return the MAC of the non-AP MLD managed by this DSO Manager.
     */
    Ptr<StaWifiMac> GetStaMac() const;

    /**
     * @param linkId the ID of the given link
     * @return the UHR FrameExchangeManager attached to the non-AP STA operating on the given link
     */
    Ptr<UhrFrameExchangeManager> GetUhrFem(uint8_t linkId) const;

  private:
    Ptr<StaWifiMac> m_staMac; //!< the MAC of the managed non-AP MLD
};

} // namespace ns3

#endif /* DSO_MANAGER_H */
