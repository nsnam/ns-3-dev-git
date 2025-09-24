/*
 * Copyright (c) 2010 Hajime Tazaki
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */
/*
 * This is the test code for ipv4-raw-socket-impl.cc.
 */

#include "ns3/arp-l3-protocol.h"
#include "ns3/boolean.h"
#include "ns3/core-module.h"
#include "ns3/error-model.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv4-raw-socket-factory.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/test.h"
#ifdef __WIN32__
#include "ns3/win32-internet.h"
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <limits>
#include <string>
#include <sys/types.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ipv4-raw-fragmentation-test");

/**
 * @ingroup internet-test
 *
 * @brief IPv4 Raw Socket Fragmentation Test
 * Configuration : Two sockets. 1 sender, 1 receiver
 *                 Sender socket sends packets in two epochs.
 *                 In the first epoch, it sends payload of sizes {500, 1000, 5000, 10000,
 *                 20000, 40000, 60000}.
 *                 In the second epoch, it sends payload of sizes {5000, 10000, 20000,
 *                 40000, 60000}. These are payloads of size greater than the MTU of
 *                 the link.
 *                 In the first epoch, there is no loss model.
 *                 In the second epoch, the second fragment of the packet sent is dropped.
 *
 * Expected behaviour: In the first epoch: Receiver should receive only 1 packet and the size of
 *                     the payload received should be equal to the size of the payload sent.
 *                     No packets should have been dropped.
 *                     In the second epoch: Receiver should not receive any packet. Only 1 packet
 *                     should have been dropped at the physical layer.
 */
class Ipv4RawFragmentationTest : public TestCase
{
  public:
    Ipv4RawFragmentationTest();
    void DoRun() override;

  private:
    /**
     * @brief Send data to the receiver
     * @param s The sending socket.
     * @param sz Size of the packet.
     */
    void SendPacket(Ptr<Socket> s, uint32_t sz);

    /**
     * @brief Receive callback
     * @param s The receiving socket.
     */
    void RxCallback(Ptr<Socket> s);

    /**
     * @brief Packet dropped callback at Phy Rx.
     * @param p Dropped packet.
     */
    void PhyRxDropCallback(Ptr<const Packet> p);

    uint32_t m_rxCount{0};          //!< Received packet count
    uint32_t m_rxPayloadSize{0};    //!< Received packet size
    uint32_t m_droppedFragments{0}; //!< Number of dropped fragments
};

Ipv4RawFragmentationTest::Ipv4RawFragmentationTest()
    : TestCase("IPv4 raw fragmentation, fragment‑loss test")
{
}

void
Ipv4RawFragmentationTest::RxCallback(Ptr<Socket> s)
{
    Ptr<Packet> p = s->Recv();
    Ipv4Header h;
    if (p->PeekHeader(h))
    {
        p->RemoveHeader(h);
    }
    m_rxCount++;
    m_rxPayloadSize = p->GetSize();
    NS_LOG_DEBUG("Receiver got " << m_rxPayloadSize << " bytes");
}

void
Ipv4RawFragmentationTest::PhyRxDropCallback(Ptr<const Packet> p)
{
    ++m_droppedFragments;
}

void
Ipv4RawFragmentationTest::SendPacket(Ptr<Socket> socket, uint32_t size)
{
    Address realTo = InetSocketAddress(Ipv4Address("10.0.1.1"), 0);
    Ptr<Packet> p = Create<Packet>(size);
    socket->SendTo(p, 0, realTo);
    NS_LOG_DEBUG("Sender sent " << p->GetSize() << " bytes to 10.0.1.1");
}

void
Ipv4RawFragmentationTest::DoRun()
{
    Ptr<Node> rxNode = CreateObject<Node>();
    Ptr<Node> txNode = CreateObject<Node>();

    NodeContainer nodes(rxNode, txNode);

    SimpleNetDeviceHelper helperChannel;
    helperChannel.SetNetDevicePointToPointMode(true);
    NetDeviceContainer net = helperChannel.Install(nodes);

    net.Get(1)->SetMtu(1000);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ptr<Ipv4> ipv4;
    uint32_t netdev_idx;
    Ipv4InterfaceAddress ipv4Addr;

    ipv4 = rxNode->GetObject<Ipv4>();
    netdev_idx = ipv4->AddInterface(net.Get(0));
    ipv4Addr = Ipv4InterfaceAddress(Ipv4Address("10.0.1.1"), Ipv4Mask(0xffff0000U));
    ipv4->AddAddress(netdev_idx, ipv4Addr);
    ipv4->SetUp(netdev_idx);

    ipv4 = txNode->GetObject<Ipv4>();
    netdev_idx = ipv4->AddInterface(net.Get(1));
    ipv4Addr = Ipv4InterfaceAddress(Ipv4Address("10.0.1.2"), Ipv4Mask(0xffff0000U));
    ipv4->AddAddress(netdev_idx, ipv4Addr);
    ipv4->SetUp(netdev_idx);

    net.Get(0)->TraceConnectWithoutContext(
        "PhyRxDrop",
        MakeCallback(&Ipv4RawFragmentationTest::PhyRxDropCallback, this));

    Ptr<SocketFactory> rxSocketFactory = rxNode->GetObject<Ipv4RawSocketFactory>();
    Ptr<Socket> rxSocket = rxSocketFactory->CreateSocket();
    rxSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 0));
    rxSocket->SetRecvCallback(MakeCallback(&Ipv4RawFragmentationTest::RxCallback, this));

    Ptr<SocketFactory> txSocketFactory = txNode->GetObject<Ipv4RawSocketFactory>();
    Ptr<Socket> txSocket = txSocketFactory->CreateSocket();

    // Contains both Fragmentable and Non-fragmentable payload sizes
    std::vector<uint32_t> payloadSizes = {500, 1000, 5000, 10000, 20000, 40000, 60000};

    for (auto sentPayloadSize : payloadSizes)
    {
        m_rxCount = 0;
        m_droppedFragments = 0;
        m_rxPayloadSize = 0;
        Simulator::Schedule(Seconds(1.0),
                            &Ipv4RawFragmentationTest::SendPacket,
                            this,
                            txSocket,
                            sentPayloadSize);
        Simulator::Run();

        NS_TEST_EXPECT_MSG_EQ(m_rxCount, 1, "Packet should be reassembled and be delivered");
        NS_TEST_EXPECT_MSG_EQ(m_rxPayloadSize,
                              sentPayloadSize,
                              "Reassembled payload size must match the payload size sent");
        NS_TEST_EXPECT_MSG_EQ(m_droppedFragments, 0, "No fragment must have been dropped");
    }

    std::vector<uint32_t> fragmentablePayloadSizes = {5000, 10000, 20000, 40000, 60000};

    for (auto sentPayloadSize : fragmentablePayloadSizes)
    {
        m_rxCount = 0;
        m_droppedFragments = 0;
        Ptr<ReceiveListErrorModel> errModel = CreateObject<ReceiveListErrorModel>();
        errModel->SetList({1});

        net.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(errModel));

        Simulator::Schedule(Seconds(1.0),
                            &Ipv4RawFragmentationTest::SendPacket,
                            this,
                            txSocket,
                            sentPayloadSize);
        Simulator::Run();

        NS_TEST_EXPECT_MSG_EQ(m_rxCount, 0, "Packet must not have been delivered");

        NS_TEST_EXPECT_MSG_EQ(m_droppedFragments, 1, "One fragment must have been dropped");
    }
    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief IPv4 RAW Socket Test
 */
class Ipv4RawSocketImplTest : public TestCase
{
    Ptr<Packet> m_receivedPacket;  //!< Received packet (1).
    Ptr<Packet> m_receivedPacket2; //!< Received packet (2).

    /**
     * @brief Send data.
     * @param socket The sending socket.
     * @param to Destination address.
     */
    void DoSendData(Ptr<Socket> socket, std::string to);
    /**
     * @brief Send data.
     * @param socket The sending socket.
     * @param to Destination address.
     */
    void SendData(Ptr<Socket> socket, std::string to);
    /**
     * @brief Send data.
     * @param socket The sending socket.
     * @param to Destination address.
     */
    void DoSendData_IpHdr(Ptr<Socket> socket, std::string to);
    /**
     * @brief Send data.
     * @param socket The sending socket.
     * @param to Destination address.
     */
    void SendData_IpHdr(Ptr<Socket> socket, std::string to);

  public:
    void DoRun() override;
    Ipv4RawSocketImplTest();

    /**
     * @brief Receive data.
     * @param socket The receiving socket.
     * @param packet The received packet.
     * @param from The sender.
     */
    void ReceivePacket(Ptr<Socket> socket, Ptr<Packet> packet, const Address& from);
    /**
     * @brief Receive data.
     * @param socket The receiving socket.
     * @param packet The received packet.
     * @param from The sender.
     */
    void ReceivePacket2(Ptr<Socket> socket, Ptr<Packet> packet, const Address& from);
    /**
     * @brief Receive data.
     * @param socket The receiving socket.
     */
    void ReceivePkt(Ptr<Socket> socket);
    /**
     * @brief Receive data.
     * @param socket The receiving socket.
     */
    void ReceivePkt2(Ptr<Socket> socket);
};

Ipv4RawSocketImplTest::Ipv4RawSocketImplTest()
    : TestCase("IPv4 Raw socket implementation")
{
}

void
Ipv4RawSocketImplTest::ReceivePacket(Ptr<Socket> socket, Ptr<Packet> packet, const Address& from)
{
    m_receivedPacket = packet;
}

void
Ipv4RawSocketImplTest::ReceivePacket2(Ptr<Socket> socket, Ptr<Packet> packet, const Address& from)
{
    m_receivedPacket2 = packet;
}

void
Ipv4RawSocketImplTest::ReceivePkt(Ptr<Socket> socket)
{
    uint32_t availableData;
    availableData = socket->GetRxAvailable();
    m_receivedPacket = socket->Recv(2, MSG_PEEK);
    NS_TEST_ASSERT_MSG_EQ(m_receivedPacket->GetSize(), 2, "ReceivedPacket size is not equal to 2");
    m_receivedPacket = socket->Recv(std::numeric_limits<uint32_t>::max(), 0);
    NS_TEST_ASSERT_MSG_EQ(availableData,
                          m_receivedPacket->GetSize(),
                          "Received packet size is not equal to Rx buffer size");
}

void
Ipv4RawSocketImplTest::ReceivePkt2(Ptr<Socket> socket)
{
    uint32_t availableData;
    availableData = socket->GetRxAvailable();
    m_receivedPacket2 = socket->Recv(2, MSG_PEEK);
    NS_TEST_ASSERT_MSG_EQ(m_receivedPacket2->GetSize(), 2, "ReceivedPacket size is not equal to 2");
    m_receivedPacket2 = socket->Recv(std::numeric_limits<uint32_t>::max(), 0);
    NS_TEST_ASSERT_MSG_EQ(availableData,
                          m_receivedPacket2->GetSize(),
                          "Received packet size is not equal to Rx buffer size");
}

void
Ipv4RawSocketImplTest::DoSendData(Ptr<Socket> socket, std::string to)
{
    Address realTo = InetSocketAddress(Ipv4Address(to.c_str()), 0);
    NS_TEST_EXPECT_MSG_EQ(socket->SendTo(Create<Packet>(123), 0, realTo), 123, to);
}

void
Ipv4RawSocketImplTest::SendData(Ptr<Socket> socket, std::string to)
{
    m_receivedPacket = Create<Packet>();
    m_receivedPacket2 = Create<Packet>();
    Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                   Seconds(0),
                                   &Ipv4RawSocketImplTest::DoSendData,
                                   this,
                                   socket,
                                   to);
    Simulator::Run();
}

void
Ipv4RawSocketImplTest::DoSendData_IpHdr(Ptr<Socket> socket, std::string to)
{
    Address realTo = InetSocketAddress(Ipv4Address(to.c_str()), 0);
    socket->SetAttribute("IpHeaderInclude", BooleanValue(true));
    Ptr<Packet> p = Create<Packet>(123);
    Ipv4Header ipHeader;
    ipHeader.SetSource(Ipv4Address("10.0.0.2"));
    ipHeader.SetDestination(Ipv4Address(to.c_str()));
    ipHeader.SetProtocol(0);
    ipHeader.SetPayloadSize(p->GetSize());
    ipHeader.SetTtl(255);
    p->AddHeader(ipHeader);

    NS_TEST_EXPECT_MSG_EQ(socket->SendTo(p, 0, realTo), 143, to);
    socket->SetAttribute("IpHeaderInclude", BooleanValue(false));
}

void
Ipv4RawSocketImplTest::SendData_IpHdr(Ptr<Socket> socket, std::string to)
{
    m_receivedPacket = Create<Packet>();
    m_receivedPacket2 = Create<Packet>();
    Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                   Seconds(0),
                                   &Ipv4RawSocketImplTest::DoSendData_IpHdr,
                                   this,
                                   socket,
                                   to);
    Simulator::Run();
}

void
Ipv4RawSocketImplTest::DoRun()
{
    // Create topology

    // Receiver Node
    Ptr<Node> rxNode = CreateObject<Node>();
    // Sender Node
    Ptr<Node> txNode = CreateObject<Node>();

    NodeContainer nodes(rxNode, txNode);

    SimpleNetDeviceHelper helperChannel1;
    helperChannel1.SetNetDevicePointToPointMode(true);
    NetDeviceContainer net1 = helperChannel1.Install(nodes);

    SimpleNetDeviceHelper helperChannel2;
    helperChannel2.SetNetDevicePointToPointMode(true);
    NetDeviceContainer net2 = helperChannel2.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ptr<Ipv4> ipv4;
    uint32_t netdev_idx;
    Ipv4InterfaceAddress ipv4Addr;

    // Receiver Node
    ipv4 = rxNode->GetObject<Ipv4>();
    netdev_idx = ipv4->AddInterface(net1.Get(0));
    ipv4Addr = Ipv4InterfaceAddress(Ipv4Address("10.0.0.1"), Ipv4Mask(0xffff0000U));
    ipv4->AddAddress(netdev_idx, ipv4Addr);
    ipv4->SetUp(netdev_idx);

    netdev_idx = ipv4->AddInterface(net2.Get(0));
    ipv4Addr = Ipv4InterfaceAddress(Ipv4Address("10.0.1.1"), Ipv4Mask(0xffff0000U));
    ipv4->AddAddress(netdev_idx, ipv4Addr);
    ipv4->SetUp(netdev_idx);

    // Sender Node
    ipv4 = txNode->GetObject<Ipv4>();
    netdev_idx = ipv4->AddInterface(net1.Get(1));
    ipv4Addr = Ipv4InterfaceAddress(Ipv4Address("10.0.0.2"), Ipv4Mask(0xffff0000U));
    ipv4->AddAddress(netdev_idx, ipv4Addr);
    ipv4->SetUp(netdev_idx);

    netdev_idx = ipv4->AddInterface(net2.Get(1));
    ipv4Addr = Ipv4InterfaceAddress(Ipv4Address("10.0.1.2"), Ipv4Mask(0xffff0000U));
    ipv4->AddAddress(netdev_idx, ipv4Addr);
    ipv4->SetUp(netdev_idx);

    // Create the IPv4 Raw sockets
    Ptr<SocketFactory> rxSocketFactory = rxNode->GetObject<Ipv4RawSocketFactory>();
    Ptr<Socket> rxSocket = rxSocketFactory->CreateSocket();
    NS_TEST_EXPECT_MSG_EQ(rxSocket->Bind(InetSocketAddress(Ipv4Address("0.0.0.0"), 0)),
                          0,
                          "trivial");
    rxSocket->SetRecvCallback(MakeCallback(&Ipv4RawSocketImplTest::ReceivePkt, this));

    Ptr<Socket> rxSocket2 = rxSocketFactory->CreateSocket();
    rxSocket2->SetRecvCallback(MakeCallback(&Ipv4RawSocketImplTest::ReceivePkt2, this));
    NS_TEST_EXPECT_MSG_EQ(rxSocket2->Bind(InetSocketAddress(Ipv4Address("10.0.1.1"), 0)),
                          0,
                          "trivial");

    Ptr<SocketFactory> txSocketFactory = txNode->GetObject<Ipv4RawSocketFactory>();
    Ptr<Socket> txSocket = txSocketFactory->CreateSocket();

    // ------ Now the tests ------------

    // Unicast test
    SendData(txSocket, "10.0.0.1");
    NS_TEST_EXPECT_MSG_EQ(m_receivedPacket->GetSize(), 143, "recv: 10.0.0.1");
    NS_TEST_EXPECT_MSG_EQ(m_receivedPacket2->GetSize(),
                          0,
                          "second interface should not receive it");

    m_receivedPacket->RemoveAllByteTags();
    m_receivedPacket2->RemoveAllByteTags();

    // Unicast w/ header test
    SendData_IpHdr(txSocket, "10.0.0.1");
    NS_TEST_EXPECT_MSG_EQ(m_receivedPacket->GetSize(), 143, "recv(hdrincl): 10.0.0.1");
    NS_TEST_EXPECT_MSG_EQ(m_receivedPacket2->GetSize(),
                          0,
                          "second interface should not receive it");

    m_receivedPacket->RemoveAllByteTags();
    m_receivedPacket2->RemoveAllByteTags();

#if 0
  // Simple broadcast test

  SendData (txSocket, "255.255.255.255");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket->GetSize (), 143, "recv: 255.255.255.255");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket2->GetSize (), 0, "second socket should not receive it (it is bound specifically to the second interface's address");

  m_receivedPacket->RemoveAllByteTags ();
  m_receivedPacket2->RemoveAllByteTags ();
#endif

    // Simple Link-local multicast test

    txSocket->Bind(InetSocketAddress(Ipv4Address("10.0.0.2"), 0));
    SendData(txSocket, "224.0.0.9");
    NS_TEST_EXPECT_MSG_EQ(m_receivedPacket->GetSize(), 143, "recv: 224.0.0.9");
    NS_TEST_EXPECT_MSG_EQ(m_receivedPacket2->GetSize(),
                          0,
                          "second socket should not receive it (it is bound specifically to the "
                          "second interface's address");

    m_receivedPacket->RemoveAllByteTags();
    m_receivedPacket2->RemoveAllByteTags();

#if 0
  // Broadcast test with multiple receiving sockets

  // When receiving broadcast packets, all sockets sockets bound to
  // the address/port should receive a copy of the same packet -- if
  // the socket address matches.
  rxSocket2->Dispose ();
  rxSocket2 = rxSocketFactory->CreateSocket ();
  rxSocket2->SetRecvCallback (MakeCallback (&Ipv4RawSocketImplTest::ReceivePkt2, this));
  NS_TEST_EXPECT_MSG_EQ (rxSocket2->Bind (InetSocketAddress (Ipv4Address ("0.0.0.0"), 0)), 0, "trivial");

  SendData (txSocket, "255.255.255.255");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket->GetSize (), 143, "recv: 255.255.255.255");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket2->GetSize (), 143, "recv: 255.255.255.255");
#endif

    m_receivedPacket = nullptr;
    m_receivedPacket2 = nullptr;

    // Simple getpeername tests

    Address peerAddress;
    int err = txSocket->GetPeerName(peerAddress);
    NS_TEST_EXPECT_MSG_EQ(err, -1, "socket GetPeerName() should fail when socket is not connected");
    NS_TEST_EXPECT_MSG_EQ(txSocket->GetErrno(),
                          Socket::ERROR_NOTCONN,
                          "socket error code should be ERROR_NOTCONN");

    InetSocketAddress peer("10.0.0.1", 1234);
    err = txSocket->Connect(peer);
    NS_TEST_EXPECT_MSG_EQ(err, 0, "socket Connect() should succeed");

    err = txSocket->GetPeerName(peerAddress);
    NS_TEST_EXPECT_MSG_EQ(err, 0, "socket GetPeerName() should succeed when socket is connected");
    peer.SetPort(0);
    NS_TEST_EXPECT_MSG_EQ(peerAddress,
                          peer,
                          "address from socket GetPeerName() should equal the connected address");

    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief IPv4 RAW Socket TestSuite
 */
class Ipv4RawTestSuite : public TestSuite
{
  public:
    Ipv4RawTestSuite()
        : TestSuite("ipv4-raw", Type::UNIT)
    {
        AddTestCase(new Ipv4RawSocketImplTest, TestCase::Duration::QUICK);
        AddTestCase(new Ipv4RawFragmentationTest, TestCase::Duration::QUICK);
    }
};

static Ipv4RawTestSuite g_ipv4rawTestSuite; //!< Static variable for test initialization
