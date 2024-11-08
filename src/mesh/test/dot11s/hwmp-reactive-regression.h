/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */
#include "ns3/ipv4-interface-container.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/pcap-file.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup dot11s-test
 *
 * @brief test for multihop path establishing and path error
 * procedures
 * Initiate scenario with 6 stations. Procedure of opening peer link
 * is the following: (PMP routines are not shown)
 * @verbatim
 *      0    1    2    3    4    5
 *      |    |    |    |    |<---|--->  ARP request (2.002s)
 *      |....|....|....|....|....|      ARP requests (continued)
 *      |<---|--->|    |    |    |      ARP request
 *  <---|--->|    |    |    |    |      PREQ       } This order is broken
 *  <---|--->|    |    |    |    |      ARP request} due to BroadcastDca
 *      |<---|--->|    |    |    |      PREQ 2.00468s)
 *      |....|....|....|....|....|      ARP request
 *      |    |    |    |<---|--->|      PREQ (2.00621s)
 *      |    |    |    |    |<---|      PREP
 *      |....|....|....|....|....|      PREP (continued)
 *      |<---|    |    |    |    |      PREP (2.00808s)
 *      |--->|    |    |    |    |      ARP reply (2.0084s)
 *      |....|....|....|....|....|      ARP replies
 *      |    |    |    |    |--->|      ARP reply (2.01049s)
 *      |    |    |    |    |<---|      Data (2.01059s)
 *      |....|....|....|....|....|      Data (continued)
 *      |<---|    |    |    |    |      Data
 *  <---|--->|    |    |    |    |      ARP request (2.02076s)
 *      |....|....|....|....|....|      ARP requests (continued)
 *      |    |    |    |    |<---|--->  ARP request
 *      |    |    |    |    |<---|      ARP reply (2.02281s)
 *      |....|....|....|....|....|      ARP replies (continued)
 *      |<---|    |    |    |    |      ARP reply
 *      |--->|    |    |    |    |      Data
 * At 5s, station number 3 disappears, and PERR is forwarded from 2 to 0
 * and from 4 to 5, and station 5 starts path discovery procedure
 * again:
 *      |    |<---|         |--->|      PERR (one due to beacon loss and one due to TX error)
 *      |<---|    |         |    |      PERR
 *      |    |    |         |<---|--->  PREQ
 *      |    |    |     <---|--->|      PREQ
 *      |....|....|.........|....|      Repeated attempts of PREQ
 * @endverbatim
 */

class HwmpReactiveRegressionTest : public TestCase
{
  public:
    HwmpReactiveRegressionTest();
    ~HwmpReactiveRegressionTest() override;

    void DoRun() override;
    /// Check results function
    void CheckResults();

  private:
    /// @internal It is important to have pointers here
    NodeContainer* m_nodes;
    /// Simulation time
    Time m_time;
    Ipv4InterfaceContainer m_interfaces; ///< interfaces

    /// Create nodes function
    void CreateNodes();
    /// Create devices function
    void CreateDevices();
    /// Install application function
    void InstallApplications();
    /// Reset position function
    void ResetPosition();

    /// Server-side socket
    Ptr<Socket> m_serverSocket;
    /// Client-side socket
    Ptr<Socket> m_clientSocket;

    /// sent packets counter
    uint32_t m_sentPktsCounter;

    /**
     * Send data
     * @param socket the sending socket
     */
    void SendData(Ptr<Socket> socket);

    /**
     * @brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * @param socket the socket the packet was received to.
     */
    void HandleReadServer(Ptr<Socket> socket);

    /**
     * @brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * @param socket the socket the packet was received to.
     */
    void HandleReadClient(Ptr<Socket> socket);
};
