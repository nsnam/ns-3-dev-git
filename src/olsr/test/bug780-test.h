/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#ifndef BUG780_TEST_H
#define BUG780_TEST_H

#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/test.h"

namespace ns3
{

class Socket;

namespace olsr
{

/**
 * @ingroup olsr-test
 * @ingroup tests
 *
 * See \bugid{780}
 */
class Bug780Test : public TestCase
{
  public:
    Bug780Test();
    ~Bug780Test() override;

  private:
    /// Total simulation time
    const Time m_time;
    /// Create & configure test network
    void CreateNodes();
    void DoRun() override;
    /// Send one ping
    void SendPing();
    /**
     * Receive echo reply
     * @param socket the socket
     */
    void Receive(Ptr<Socket> socket);
    /// Socket
    Ptr<Socket> m_socket;
    /// Sequence number
    uint16_t m_seq;
    /// Received ECHO Reply counter
    uint16_t m_recvCount;
};

} // namespace olsr
} // namespace ns3

#endif /* BUG780_TEST_H */
