/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#ifndef PMP_REGRESSION_H
#define PMP_REGRESSION_H
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup dot11s-test
 *
 * @brief Peering Management Protocol regression test
 *
 * Initiate scenario with 2 stations. Procedure of opening peer link
 * is the following:
 * @verbatim
 * |----------->|  Beacon
 * |----------->|  Peer Link Open frame
 * |<-----------|  Peer Link Confirm frame
 * |<-----------|  Peer Link Open frame
 * |----------->|  Peer Link Confirm frame
 * |............|
 * |<---------->|  Other beacons
 * @endverbatim
 */
class PeerManagementProtocolRegressionTest : public TestCase
{
  public:
    PeerManagementProtocolRegressionTest();
    ~PeerManagementProtocolRegressionTest() override;

  private:
    /// @internal It is important to have pointers here
    NodeContainer* m_nodes;
    /// Simulation time
    Time m_time;

    /// Create nodes function
    void CreateNodes();
    /// Create devices function
    void CreateDevices();
    /// Check results function
    void CheckResults();
    void DoRun() override;
};
#endif /* PMP_REGRESSION_H */
