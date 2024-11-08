/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#ifndef HELLO_REGRESSION_TEST_H
#define HELLO_REGRESSION_TEST_H

#include "ns3/ipv4-raw-socket-impl.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/test.h"

namespace ns3
{
namespace olsr
{
/**
 * @ingroup olsr-test
 * @ingroup tests
 *
 * @brief Trivial (still useful) test of OLSR operation
 *
 * This test creates 2 stations with point-to-point link and runs OLSR without any extra traffic.
 * It is expected that only HELLO messages will be sent.
 *
 * Expected trace (5 seconds):
   \verbatim
    1       2
    |------>|   HELLO (empty) src = 10.1.1.1
    |<------|   HELLO (empty) src = 10.1.1.2
    |------>|   HELLO (Link type: Asymmetric link, Neighbor address: 10.1.1.2) src = 10.1.1.1
    |<------|   HELLO (Link type: Asymmetric link, Neighbor address: 10.1.1.1) src = 10.1.1.2
    |------>|   HELLO (Link type: Symmetric link, Neighbor address: 10.1.1.2) src = 10.1.1.1
    |<------|   HELLO (Link type: Symmetric link, Neighbor address: 10.1.1.1) src = 10.1.1.2
   \endverbatim
 */
class HelloRegressionTest : public TestCase
{
  public:
    HelloRegressionTest();
    ~HelloRegressionTest() override;

  private:
    /// Total simulation time
    const Time m_time;
    /// Create & configure test network
    void CreateNodes();
    void DoRun() override;

    /**
     * Receive raw data on node A
     * @param socket receiving socket
     */
    void ReceivePktProbeA(Ptr<Socket> socket);
    /// Packet counter on node A
    uint8_t m_countA;
    /// Receiving socket on node A
    Ptr<Ipv4RawSocketImpl> m_rxSocketA;
    /**
     * Receive raw data on node B
     * @param socket receiving socket
     */
    void ReceivePktProbeB(Ptr<Socket> socket);
    /// Packet counter on node B
    uint8_t m_countB;
    /// Receiving socket on node B
    Ptr<Ipv4RawSocketImpl> m_rxSocketB;
};

} // namespace olsr
} // namespace ns3

#endif /* HELLO_REGRESSION_TEST_H */
