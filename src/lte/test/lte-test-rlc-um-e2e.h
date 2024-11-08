/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef LTE_TEST_RLC_UM_E2E_H
#define LTE_TEST_RLC_UM_E2E_H

#include "ns3/ptr.h"
#include "ns3/test.h"

namespace ns3
{
class Packet;
}

using namespace ns3;

/**
 * @ingroup lte-test
 *
 * @brief Test suite for RlcUmE2eTestCase
 */
class LteRlcUmE2eTestSuite : public TestSuite
{
  public:
    LteRlcUmE2eTestSuite();
};

/**
 * @ingroup lte-test
 *
 * @brief Test end-to-end flow when RLC UM is being used.
 */
class LteRlcUmE2eTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     * @param seed the random variable seed
     * @param losses the error rate
     */
    LteRlcUmE2eTestCase(std::string name, uint32_t seed, double losses);
    LteRlcUmE2eTestCase();
    ~LteRlcUmE2eTestCase() override;

  private:
    void DoRun() override;

    /**
     * DL drop event
     * @param p the packet
     */
    void DlDropEvent(Ptr<const Packet> p);
    /**
     * UL drop event
     * @param p the packet
     */
    void UlDropEvent(Ptr<const Packet> p);

    uint32_t m_dlDrops; ///< number of Dl drops
    uint32_t m_ulDrops; ///< number of UL drops

    uint32_t m_seed; ///< random number seed
    double m_losses; ///< error rate
};

#endif // LTE_TEST_RLC_UM_E2E_H
