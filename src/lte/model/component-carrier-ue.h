/*
 * Copyright (c) 2015 Danilo Abrignani
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it>
 */

#ifndef COMPONENT_CARRIER_UE_H
#define COMPONENT_CARRIER_UE_H

#include "component-carrier.h"
#include "lte-phy.h"
#include "lte-ue-phy.h"

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"

namespace ns3
{

class LteUeMac;

/**
 * @ingroup lte
 *
 * ComponentCarrierUe Object, it defines a single Carrier for the Ue
 */
class ComponentCarrierUe : public ComponentCarrier
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    ComponentCarrierUe();

    ~ComponentCarrierUe() override;
    void DoDispose() override;

    /**
     * @return a pointer to the physical layer.
     */
    Ptr<LteUePhy> GetPhy() const;

    /**
     * @return a pointer to the MAC layer.
     */
    Ptr<LteUeMac> GetMac() const;

    /**
     * Set LteUePhy
     * @param s a pointer to the LteUePhy
     */
    void SetPhy(Ptr<LteUePhy> s);

    /**
     * Set the LteEnbMac
     * @param s a pointer to the LteEnbMac
     */
    void SetMac(Ptr<LteUeMac> s);

  protected:
    // inherited from Object
    void DoInitialize() override;

  private:
    Ptr<LteUePhy> m_phy; ///< the Phy instance of this eNodeB component carrier
    Ptr<LteUeMac> m_mac; ///< the MAC instance of this eNodeB component carrier
};

} // namespace ns3

#endif /* COMPONENT_CARRIER_UE_H */
