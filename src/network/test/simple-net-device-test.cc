/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/callback.h"
#include "ns3/data-rate.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/error-model.h"
#include "ns3/mac48-address.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/queue.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Regression test for @issueid{370}: a received packet with no matching
 *        protocol handler must be traced, not dropped.
 *
 * The receiver device is installed on a Node (so its receive callback is the
 * real Node::NonPromiscReceiveFromDevice), but no protocol handler is
 * registered for the chosen protocol number. A unicast packet addressed to the
 * receiver is sent; the device must honor the @c false return and fire the
 * MacRxDrop trace.
 */
class SimpleNetDeviceNoHandlerDropTestCase : public TestCase
{
  public:
    SimpleNetDeviceNoHandlerDropTestCase();

  private:
    void DoRun() override;

    /**
     * MacRxDrop trace sink.
     * @param p The dropped packet.
     */
    void DropEvent(Ptr<const Packet> p);

    uint32_t m_drops{0}; //!< Number of MacRxDrop events observed.
};

SimpleNetDeviceNoHandlerDropTestCase::SimpleNetDeviceNoHandlerDropTestCase()
    : TestCase("Packet dropped for missing protocol handler fires MacRxDrop (issue #370)")
{
}

void
SimpleNetDeviceNoHandlerDropTestCase::DropEvent(Ptr<const Packet> p)
{
    m_drops++;
}

void
SimpleNetDeviceNoHandlerDropTestCase::DoRun()
{
    auto a = CreateObject<Node>();
    auto b = CreateObject<Node>();

    auto sender = CreateObject<SimpleNetDevice>();
    auto receiver = CreateObject<SimpleNetDevice>();
    auto channel = CreateObject<SimpleChannel>();

    ObjectFactory queueFactory("ns3::DropTailQueue<Packet>");
    sender->SetQueue(queueFactory.Create<Queue<Packet>>());
    receiver->SetQueue(queueFactory.Create<Queue<Packet>>());

    // AddDevice() binds the receive callback to Node::NonPromiscReceiveFromDevice.
    // No RegisterProtocolHandler() is called on node b, so no handler matches.
    a->AddDevice(sender);
    b->AddDevice(receiver);

    sender->SetNode(a);
    sender->SetChannel(channel);
    sender->SetAddress(Mac48Address::Allocate());

    receiver->SetNode(b);
    receiver->SetChannel(channel);
    receiver->SetAddress(Mac48Address::Allocate());

    receiver->TraceConnectWithoutContext(
        "MacRxDrop",
        MakeCallback(&SimpleNetDeviceNoHandlerDropTestCase::DropEvent, this));

    // Unicast to the receiver's own address (classified PACKET_HOST) with a
    // protocol number that has no registered handler on node b.
    auto pkt = Create<Packet>(64);
    Simulator::Schedule(Seconds(0),
                        &SimpleNetDevice::Send,
                        sender,
                        pkt,
                        receiver->GetAddress(),
                        static_cast<uint16_t>(0x8888));

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_drops,
                          1,
                          "Packet dropped due to missing protocol handler was not traced "
                          "(issue #370): expected the MacRxDrop trace to fire once.");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Tests that a ReceiveErrorModel discards the packet and fires PhyRxDrop.
 *
 * With a receive error model that corrupts every packet, a frame handed to the
 * receiver must be dropped at the physical layer: the PhyRxDrop trace fires and
 * the packet is never delivered to the receive callback. The control case (no
 * error model) confirms the same frame is otherwise delivered.
 */
class SimpleNetDeviceErrorModelTestCase : public TestCase
{
  public:
    SimpleNetDeviceErrorModelTestCase();

  private:
    void DoRun() override;

    /**
     * PhyRxDrop trace sink.
     * @param p The dropped packet.
     */
    void PhyDrop(Ptr<const Packet> p);

    /**
     * Receive callback counting delivered packets.
     * @param d The receiving device.
     * @param p The received packet.
     * @param proto The protocol number.
     * @param from The source address.
     * @return Always true (packet accepted).
     */
    bool Rx(Ptr<NetDevice> d, Ptr<const Packet> p, uint16_t proto, const Address& from);

    uint32_t m_phyDrops{0};   //!< Number of PhyRxDrop events observed.
    uint32_t m_deliveries{0}; //!< Number of packets delivered to the callback.
};

SimpleNetDeviceErrorModelTestCase::SimpleNetDeviceErrorModelTestCase()
    : TestCase("ReceiveErrorModel drops a packet and fires PhyRxDrop")
{
}

void
SimpleNetDeviceErrorModelTestCase::PhyDrop(Ptr<const Packet> p)
{
    m_phyDrops++;
}

bool
SimpleNetDeviceErrorModelTestCase::Rx(Ptr<NetDevice> d,
                                      Ptr<const Packet> p,
                                      uint16_t proto,
                                      const Address& from)
{
    m_deliveries++;
    return true;
}

void
SimpleNetDeviceErrorModelTestCase::DoRun()
{
    for (bool corrupt : {false, true})
    {
        m_phyDrops = 0;
        m_deliveries = 0;

        auto sender = CreateObject<SimpleNetDevice>();
        auto receiver = CreateObject<SimpleNetDevice>();
        auto channel = CreateObject<SimpleChannel>();

        ObjectFactory queueFactory("ns3::DropTailQueue<Packet>");
        sender->SetQueue(queueFactory.Create<Queue<Packet>>());
        receiver->SetQueue(queueFactory.Create<Queue<Packet>>());

        // Devices need a Node so SimpleChannel can deliver in the node's context.
        // Node::AddDevice (rather than SetNode) is used so the node owns the
        // devices and disposes them on Simulator::Destroy, breaking the
        // device/channel reference cycle. The custom receive callback set
        // below still wins, as it is installed after AddDevice.
        CreateObject<Node>()->AddDevice(sender);
        CreateObject<Node>()->AddDevice(receiver);

        sender->SetChannel(channel);
        sender->SetAddress(Mac48Address::Allocate());
        receiver->SetChannel(channel);
        receiver->SetAddress(Mac48Address::Allocate());

        if (corrupt)
        {
            // Packet-level rate of 1.0 corrupts every packet deterministically.
            auto em = CreateObject<RateErrorModel>();
            em->SetAttribute("ErrorUnit", EnumValue(RateErrorModel::ERROR_UNIT_PACKET));
            em->SetAttribute("ErrorRate", DoubleValue(1.0));
            receiver->SetAttribute("ReceiveErrorModel", PointerValue(em));
        }

        receiver->SetReceiveCallback(MakeCallback(&SimpleNetDeviceErrorModelTestCase::Rx, this));
        receiver->TraceConnectWithoutContext(
            "PhyRxDrop",
            MakeCallback(&SimpleNetDeviceErrorModelTestCase::PhyDrop, this));

        auto pkt = Create<Packet>(64);
        Simulator::Schedule(Seconds(0),
                            &SimpleNetDevice::Send,
                            sender,
                            pkt,
                            receiver->GetAddress(),
                            static_cast<uint16_t>(0x0800));

        Simulator::Run();
        Simulator::Destroy();

        if (corrupt)
        {
            NS_TEST_ASSERT_MSG_EQ(m_phyDrops, 1, "Corrupt packet should fire PhyRxDrop once");
            NS_TEST_ASSERT_MSG_EQ(m_deliveries, 0, "Corrupt packet must not be delivered");
        }
        else
        {
            NS_TEST_ASSERT_MSG_EQ(m_phyDrops, 0, "Uncorrupted packet should not fire PhyRxDrop");
            NS_TEST_ASSERT_MSG_EQ(m_deliveries, 1, "Uncorrupted packet should be delivered once");
        }
    }
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Tests that the DataRate attribute governs the transmission time.
 *
 * A packet sent through a device configured with a finite DataRate is delivered
 * after packetSize / DataRate seconds (the channel delay is zero). Sending a
 * 125-byte (1000-bit) packet at 1 Mbps must complete at exactly 1 ms.
 */
class SimpleNetDeviceDataRateTestCase : public TestCase
{
  public:
    SimpleNetDeviceDataRateTestCase();

  private:
    void DoRun() override;

    /**
     * Receive callback recording the reception time.
     * @param d The receiving device.
     * @param p The received packet.
     * @param proto The protocol number.
     * @param from The source address.
     * @return Always true (packet accepted).
     */
    bool Rx(Ptr<NetDevice> d, Ptr<const Packet> p, uint16_t proto, const Address& from);

    Time m_rxTime{Time::Min()}; //!< Time the packet was received.
};

SimpleNetDeviceDataRateTestCase::SimpleNetDeviceDataRateTestCase()
    : TestCase("DataRate attribute governs the transmission time")
{
}

bool
SimpleNetDeviceDataRateTestCase::Rx(Ptr<NetDevice> d,
                                    Ptr<const Packet> p,
                                    uint16_t proto,
                                    const Address& from)
{
    m_rxTime = Simulator::Now();
    return true;
}

void
SimpleNetDeviceDataRateTestCase::DoRun()
{
    auto sender = CreateObject<SimpleNetDevice>();
    auto receiver = CreateObject<SimpleNetDevice>();
    auto channel = CreateObject<SimpleChannel>();

    ObjectFactory queueFactory("ns3::DropTailQueue<Packet>");
    sender->SetQueue(queueFactory.Create<Queue<Packet>>());
    receiver->SetQueue(queueFactory.Create<Queue<Packet>>());

    // Devices need a Node so SimpleChannel can deliver in the node's context.
    // Node::AddDevice (rather than SetNode) is used so the node owns the
    // devices and disposes them on Simulator::Destroy, breaking the
    // device/channel reference cycle. The custom receive callback set below
    // still wins, as it is installed after AddDevice.
    CreateObject<Node>()->AddDevice(sender);
    CreateObject<Node>()->AddDevice(receiver);

    // The transmission time is computed from the sending device's DataRate.
    sender->SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));

    sender->SetChannel(channel);
    sender->SetAddress(Mac48Address::Allocate());
    receiver->SetChannel(channel);
    receiver->SetAddress(Mac48Address::Allocate());

    receiver->SetReceiveCallback(MakeCallback(&SimpleNetDeviceDataRateTestCase::Rx, this));

    // 125 bytes = 1000 bits; at 1 Mbps the transmission takes exactly 1 ms.
    auto pkt = Create<Packet>(125);
    Simulator::Schedule(Seconds(0),
                        &SimpleNetDevice::Send,
                        sender,
                        pkt,
                        receiver->GetAddress(),
                        static_cast<uint16_t>(0x0800));

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_rxTime,
                          MilliSeconds(1),
                          "A 1000-bit packet at 1 Mbps should be received after 1 ms");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Test suite exercising SimpleNetDevice reception behavior.
 */
class SimpleNetDeviceTestSuite : public TestSuite
{
  public:
    SimpleNetDeviceTestSuite();
};

SimpleNetDeviceTestSuite::SimpleNetDeviceTestSuite()
    : TestSuite("simple-net-device", Type::UNIT)
{
    AddTestCase(new SimpleNetDeviceNoHandlerDropTestCase, TestCase::Duration::QUICK);
    AddTestCase(new SimpleNetDeviceErrorModelTestCase, TestCase::Duration::QUICK);
    AddTestCase(new SimpleNetDeviceDataRateTestCase, TestCase::Duration::QUICK);
}

static SimpleNetDeviceTestSuite g_simpleNetDeviceTestSuite; //!< Static variable for test init
