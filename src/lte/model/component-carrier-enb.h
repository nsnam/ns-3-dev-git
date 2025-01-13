/*
 * Copyright (c) 2015 Danilo Abrignani
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it>
 */

#ifndef COMPONENT_CARRIER_ENB_H
#define COMPONENT_CARRIER_ENB_H

#include "component-carrier.h"
#include "lte-enb-phy.h"

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"

namespace ns3
{

class LteEnbMac;
class FfMacScheduler;
class LteFfrAlgorithm;

/**
 * @ingroup lte
 *
 * Defines a single carrier for enb, and contains pointers to LteEnbPhy,
 * LteEnbMac, LteFfrAlgorithm, and FfMacScheduler objects.
 *
 */
class ComponentCarrierEnb : public ComponentCarrierBaseStation
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    ComponentCarrierEnb();

    ~ComponentCarrierEnb() override;
    void DoDispose() override;

    /**
     * @return a pointer to the physical layer.
     */
    Ptr<LteEnbPhy> GetPhy();

    /**
     * @return a pointer to the MAC layer.
     */
    Ptr<LteEnbMac> GetMac();

    /**
     * @return a pointer to the Ffr Algorithm.
     */
    Ptr<LteFfrAlgorithm> GetFfrAlgorithm();

    /**
     * @return a pointer to the Mac Scheduler.
     */
    Ptr<FfMacScheduler> GetFfMacScheduler();

    /**
     * Set the LteEnbPhy
     * @param s a pointer to the LteEnbPhy
     */
    void SetPhy(Ptr<LteEnbPhy> s);
    /**
     * Set the LteEnbMac
     * @param s a pointer to the LteEnbMac
     */
    void SetMac(Ptr<LteEnbMac> s);

    /**
     * Set the FfMacScheduler Algorithm
     * @param s a pointer to the FfMacScheduler
     */
    void SetFfMacScheduler(Ptr<FfMacScheduler> s);

    /**
     * Set the LteFfrAlgorithm
     * @param s a pointer to the LteFfrAlgorithm
     */
    void SetFfrAlgorithm(Ptr<LteFfrAlgorithm> s);

  protected:
    void DoInitialize() override;

  private:
    Ptr<LteEnbPhy> m_phy;            ///< the Phy instance of this eNodeB component carrier
    Ptr<LteEnbMac> m_mac;            ///< the MAC instance of this eNodeB component carrier
    Ptr<FfMacScheduler> m_scheduler; ///< the scheduler instance of this eNodeB component carrier
    Ptr<LteFfrAlgorithm>
        m_ffrAlgorithm; ///< the FFR algorithm instance of this eNodeB component carrier
};

} // namespace ns3

#endif /* COMPONENT_CARRIER_H */
