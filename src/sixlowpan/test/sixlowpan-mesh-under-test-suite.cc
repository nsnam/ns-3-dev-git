/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#include "mock-net-device.h"

#include "ns3/boolean.h"
#include "ns3/iana-ieee802-numbers.h"
#include "ns3/mac16-address.h"
#include "ns3/mac48-address.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/sixlowpan-header.h"
#include "ns3/sixlowpan-mesh-under-routing.h"
#include "ns3/sixlowpan-net-device.h"
#include "ns3/sixlowpan-simple-flooding.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

#include <vector>

using namespace ns3;

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify the FIFO drop policy of the duplicate-detection cache.
 *
 * The base-class cache keeps at most MeshCacheLength entries per
 * originator. Once that capacity is reached, the oldest entry is
 * dropped on each new insertion (FIFO drop policy).
 */
class DuplicateCacheFifoDropTestCase : public TestCase
{
  public:
    DuplicateCacheFifoDropTestCase()
        : TestCase("Duplicate cache drops oldest entry when full")
    {
    }

  private:
    void DoRun() override
    {
        Ptr<SixLowPanSimpleFlooding> policy = CreateObject<SixLowPanSimpleFlooding>();
        policy->SetAttribute("MeshCacheLength", UintegerValue(3));

        Mac16Address orig("00:01");

        policy->RecordPacket(orig, 1);
        policy->RecordPacket(orig, 2);
        policy->RecordPacket(orig, 3);

        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(orig, 1), true, "seq 1 should be cached");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(orig, 2), true, "seq 2 should be cached");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(orig, 3), true, "seq 3 should be cached");

        // Insert seq 4 -- the oldest entry (seq 1) is dropped.
        policy->RecordPacket(orig, 4);

        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(orig, 1),
                              false,
                              "seq 1 should have been dropped (FIFO)");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(orig, 4), true, "seq 4 should be cached");
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify per-originator cache partitioning under overflow.
 *
 * Each originator has its own FIFO. Overflowing one originator's cache
 * must drop only that originator's oldest entry; it must never drop an
 * entry belonging to a different originator.
 */
class DuplicateCachePerOriginatorTestCase : public TestCase
{
  public:
    DuplicateCachePerOriginatorTestCase()
        : TestCase("Overflowing one originator does not drop another's entries")
    {
    }

  private:
    void DoRun() override
    {
        Ptr<SixLowPanSimpleFlooding> policy = CreateObject<SixLowPanSimpleFlooding>();
        policy->SetAttribute("MeshCacheLength", UintegerValue(2));

        Mac16Address origA("00:01");
        Mac16Address origB("00:02");

        // Fill origB to capacity, then push origA one past capacity.
        policy->RecordPacket(origB, 1);
        policy->RecordPacket(origB, 2);
        policy->RecordPacket(origA, 1);
        policy->RecordPacket(origA, 2);
        policy->RecordPacket(origA, 3); // origA overflows: its oldest (seq 1) is dropped

        // The overflow dropped only origA's oldest entry.
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(origA, 1), false, "origA seq 1 dropped (FIFO)");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(origA, 2), true, "origA seq 2 still cached");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(origA, 3), true, "origA seq 3 cached");

        // origB's entries are untouched by origA's overflow.
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(origB, 1), true, "origB seq 1 untouched");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(origB, 2), true, "origB seq 2 untouched");
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify the default policy schedules and invokes the forward callback.
 */
class SimpleFloodingForwardsTestCase : public TestCase
{
  public:
    SimpleFloodingForwardsTestCase()
        : TestCase("SimpleFlooding invokes the forward callback once")
    {
    }

  private:
    /**
     * @brief Callback used to capture forward invocations.
     * @param packet The forwarded packet.
     */
    void RecordForward(Ptr<Packet> packet)
    {
        m_forwardedPacket = packet;
        m_forwardCount++;
    }

    void DoRun() override
    {
        Ptr<SixLowPanSimpleFlooding> policy = CreateObject<SixLowPanSimpleFlooding>();
        policy->AssignStreams(1);

        Ptr<Packet> packet = Create<Packet>(64);
        Mac16Address orig("00:01");

        SixLowPanMeshUnderRouting::ForwardCallback cb =
            MakeCallback(&SimpleFloodingForwardsTestCase::RecordForward, this);

        policy->OnPacketForward(packet, orig, /*seqNo=*/1, /*hopsLeft=*/5, cb);

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_forwardCount, 1, "Forward callback should fire exactly once");
        NS_TEST_ASSERT_MSG_EQ(m_forwardedPacket, packet, "Same packet pointer should be forwarded");
    }

    void DoTeardown() override
    {
        Simulator::Destroy();
    }

    Ptr<Packet> m_forwardedPacket; ///< Packet captured at the forward callback.
    int m_forwardCount{0};         ///< Number of times the forward callback was invoked.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Base for device-level mesh-under tests.
 *
 * Builds a SixLowPanNetDevice over a MockNetDevice, injects mesh frames
 * through the underlying device and captures both the frames the device
 * re-broadcasts and the packets it delivers locally. Subclasses exercise
 * the device public API end to end, verifying that the policy
 * delegation preserves the historical device behavior.
 */
class MeshUnderDeviceTestCase : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param name The test case name.
     */
    MeshUnderDeviceTestCase(std::string name)
        : TestCase(name)
    {
    }

  protected:
    /**
     * @brief Build the node, the underlying MockNetDevice and the 6LoWPAN device.
     *
     * The underlying device gets the MAC address 00:00:00:00:00:01, so the
     * node's 16-bit mesh address is 00:01.
     *
     * @param useMesh The device's UseMeshUnder attribute (mesh-under membership).
     * @param forwardMesh The device's ForwardMesh attribute (relaying).
     */
    void SetupDevice(bool useMesh = true, bool forwardMesh = true)
    {
        Ptr<Node> node = CreateObject<Node>();

        m_mock = CreateObject<MockNetDevice>();
        node->AddDevice(m_mock);
        m_mock->SetNode(node);
        m_mock->SetAddress(Mac48Address("00:00:00:00:00:01"));
        m_mock->SetMtu(150);
        m_mock->SetSendCallback(MakeCallback(&MeshUnderDeviceTestCase::CaptureSend, this));

        m_dev = CreateObject<SixLowPanNetDevice>();
        node->AddDevice(m_dev);
        m_dev->SetNetDevice(m_mock);
        m_dev->SetAttribute("UseMeshUnder", BooleanValue(useMesh));
        m_dev->SetAttribute("ForwardMesh", BooleanValue(forwardMesh));
        m_dev->SetReceiveCallback(MakeCallback(&MeshUnderDeviceTestCase::CaptureUp, this));
    }

    /**
     * @brief Build a mesh frame (MESH + BC0 + uncompressed IPv6 payload).
     *
     * @param originator The MESH originator address.
     * @param finalDst The MESH final destination address.
     * @param hopsLeft The MESH hops-left value.
     * @param seqNo The BC0 sequence number.
     * @return The assembled frame.
     */
    static Ptr<Packet> BuildMeshFrame(Mac16Address originator,
                                      Mac16Address finalDst,
                                      uint8_t hopsLeft,
                                      uint8_t seqNo)
    {
        Ptr<Packet> packet = Create<Packet>(40);

        SixLowPanIpv6 uncompressedHdr;
        packet->AddHeader(uncompressedHdr);

        SixLowPanBc0 bc0Hdr;
        bc0Hdr.SetSequenceNumber(seqNo);
        packet->AddHeader(bc0Hdr);

        SixLowPanMesh meshHdr;
        meshHdr.SetOriginator(originator);
        meshHdr.SetFinalDst(finalDst);
        meshHdr.SetHopsLeft(hopsLeft);
        packet->AddHeader(meshHdr);

        return packet;
    }

    /**
     * @brief Schedule the injection of a frame, as if received from the channel.
     *
     * The event is scheduled with the node's context, as a channel would.
     *
     * @param when The injection time.
     * @param packet The frame to inject.
     */
    void ScheduleInject(Time when, Ptr<Packet> packet)
    {
        Simulator::ScheduleWithContext(m_mock->GetNode()->GetId(),
                                       when,
                                       &MockNetDevice::Receive,
                                       m_mock,
                                       packet,
                                       uint16_t{iana::ieee802numbers::LoWPAN},
                                       m_mock->GetBroadcast(),
                                       Address(Mac48Address("00:00:00:00:00:02")),
                                       NetDevice::PACKET_BROADCAST);
    }

    /**
     * @brief Capture a frame the device re-broadcasts.
     * @param device The sending device.
     * @param packet The transmitted frame.
     * @param protocol The protocol number.
     * @param source The source address.
     * @param destination The destination address.
     * @param packetType The packet type.
     * @return true
     */
    bool CaptureSend(Ptr<NetDevice> device,
                     Ptr<const Packet> packet,
                     uint16_t protocol,
                     const Address& source,
                     const Address& destination,
                     NetDevice::PacketType packetType)
    {
        m_sentFrames.push_back(packet->Copy());
        return true;
    }

    /**
     * @brief Capture a locally delivered packet.
     * @param device The receiving device.
     * @param packet The delivered packet.
     * @param protocol The protocol number.
     * @param source The source address.
     * @return true
     */
    bool CaptureUp(Ptr<NetDevice> device,
                   Ptr<const Packet> packet,
                   uint16_t protocol,
                   const Address& source)
    {
        m_deliveredCount++;
        return true;
    }

    void DoTeardown() override
    {
        m_dev = nullptr;
        m_mock = nullptr;
        Simulator::Destroy();
    }

    Ptr<MockNetDevice> m_mock;             ///< Underlying (mock) device.
    Ptr<SixLowPanNetDevice> m_dev;         ///< Device under test.
    std::vector<Ptr<Packet>> m_sentFrames; ///< Frames re-broadcast by the device.
    uint32_t m_deliveredCount{0};          ///< Packets delivered locally.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief The device forwards a new mesh frame once and drops duplicates.
 */
class MeshUnderDeviceForwardTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceForwardTestCase()
        : MeshUnderDeviceTestCase("Device forwards new mesh frames once, with decremented hops")
    {
    }

  private:
    void DoRun() override
    {
        SetupDevice();

        Mac16Address originator("00:02");
        Mac16Address finalDst("00:03"); // Another node: to be forwarded, not delivered.

        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(originator, finalDst, /*hopsLeft=*/5, /*seqNo=*/1));
        // The same (originator, sequence) pair again: a duplicate.
        ScheduleInject(MilliSeconds(2),
                       BuildMeshFrame(originator, finalDst, /*hopsLeft=*/5, /*seqNo=*/1));

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(), 1, "One forward for two copies (dedup)");
        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount, 0, "Frame for another node must not be delivered");

        if (!m_sentFrames.empty())
        {
            SixLowPanMesh meshHdr;
            SixLowPanBc0 bc0Hdr;
            m_sentFrames[0]->RemoveHeader(meshHdr);
            m_sentFrames[0]->RemoveHeader(bc0Hdr);
            NS_TEST_ASSERT_MSG_EQ(meshHdr.GetHopsLeft(), 4, "Hops left must be decremented");
            NS_TEST_ASSERT_MSG_EQ(meshHdr.GetOriginator(),
                                  Address(originator),
                                  "Originator preserved");
            NS_TEST_ASSERT_MSG_EQ(meshHdr.GetFinalDst(), Address(finalDst), "Final dst preserved");
            NS_TEST_ASSERT_MSG_EQ(bc0Hdr.GetSequenceNumber(), 1, "Sequence number preserved");
        }
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief A mesh frame destined to the node is delivered locally.
 *
 * A node in the mesh (UseMeshUnder enabled) must decode and deliver a mesh
 * frame addressed to it, and must not re-broadcast a unicast frame it
 * is the destination of.
 */
class MeshUnderDeviceDeliveryTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceDeliveryTestCase()
        : MeshUnderDeviceTestCase("Mesh frame for this node is delivered")
    {
    }

  private:
    void DoRun() override
    {
        SetupDevice();

        Mac16Address originator("00:02");
        Mac16Address finalDst("00:01"); // This node.

        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(originator, finalDst, /*hopsLeft=*/5, /*seqNo=*/1));

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount, 1, "Frame for this node must be delivered");
        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(), 0, "Unicast frame for this node: no forward");
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Hop limit and broadcast handling on the receive path.
 */
class MeshUnderDeviceHopLimitTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceHopLimitTestCase()
        : MeshUnderDeviceTestCase("Hop limit is honored; broadcast is forwarded and delivered")
    {
    }

  private:
    void DoRun() override
    {
        SetupDevice();

        Mac16Address originator("00:02");

        // Exhausted hop count: must not be forwarded.
        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(originator,
                                      Mac16Address("00:03"),
                                      /*hopsLeft=*/0,
                                      /*seqNo=*/1));
        // Broadcast: must be forwarded and delivered.
        ScheduleInject(MilliSeconds(2),
                       BuildMeshFrame(originator,
                                      Mac16Address("ff:ff"),
                                      /*hopsLeft=*/3,
                                      /*seqNo=*/2));

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(), 1, "Only the broadcast frame is forwarded");
        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount, 1, "The broadcast frame is also delivered");

        if (!m_sentFrames.empty())
        {
            SixLowPanMesh meshHdr;
            m_sentFrames[0]->RemoveHeader(meshHdr);
            NS_TEST_ASSERT_MSG_EQ(meshHdr.GetHopsLeft(), 2, "Hops left must be decremented");
        }
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief A node outside the mesh receiving a mesh frame reports and drops it.
 *
 * With UseMeshUnder disabled, a received mesh-under frame is a network
 * misconfiguration: it must be dropped (DROP_MESH_NOT_ENABLED), not
 * delivered and not forwarded.
 */
class MeshUnderDeviceMisconfigTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceMisconfigTestCase()
        : MeshUnderDeviceTestCase("Mesh frame without UseMeshUnder is reported and dropped")
    {
    }

  private:
    /**
     * @brief Capture drop-trace invocations.
     * @param reason The drop reason.
     * @param packet The dropped packet.
     * @param device The dropping device.
     * @param ifindex The interface index.
     */
    void CaptureDrop(SixLowPanNetDevice::DropReason reason,
                     Ptr<const Packet> packet,
                     Ptr<SixLowPanNetDevice> device,
                     uint32_t ifindex)
    {
        m_dropCount++;
        m_lastDropReason = reason;
    }

    void DoRun() override
    {
        SetupDevice(/*useMesh=*/false);
        m_dev->TraceConnectWithoutContext(
            "Drop",
            MakeCallback(&MeshUnderDeviceMisconfigTestCase::CaptureDrop, this));

        // A mesh frame addressed to this very node: it must still be dropped.
        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(Mac16Address("00:02"),
                                      Mac16Address("00:01"),
                                      /*hopsLeft=*/5,
                                      /*seqNo=*/1));

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount,
                              0,
                              "Frame must not be delivered without UseMeshUnder");
        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(),
                              0,
                              "Frame must not be forwarded without UseMeshUnder");
        NS_TEST_ASSERT_MSG_EQ(m_dropCount, 1, "The drop must be reported on the Drop trace");
        NS_TEST_ASSERT_MSG_EQ(m_lastDropReason,
                              SixLowPanNetDevice::DROP_MESH_NOT_ENABLED,
                              "The drop reason must be DROP_MESH_NOT_ENABLED");
    }

    uint32_t m_dropCount{0}; ///< Number of drop-trace invocations.
    SixLowPanNetDevice::DropReason m_lastDropReason{
        SixLowPanNetDevice::DROP_FRAGMENT_TIMEOUT}; ///< Last reported drop reason.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief A node in the mesh with ForwardMesh disabled receives but does not relay.
 */
class MeshUnderDeviceNoRelayTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceNoRelayTestCase()
        : MeshUnderDeviceTestCase("Node with ForwardMesh disabled delivers but does not relay")
    {
    }

  private:
    void DoRun() override
    {
        SetupDevice(/*useMesh=*/true, /*forwardMesh=*/false);

        Mac16Address originator("00:02");

        // A frame for another node: without ForwardMesh it goes nowhere.
        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(originator,
                                      Mac16Address("00:03"),
                                      /*hopsLeft=*/5,
                                      /*seqNo=*/1));
        // A broadcast frame: delivered locally, but still not relayed.
        ScheduleInject(MilliSeconds(2),
                       BuildMeshFrame(originator,
                                      Mac16Address("ff:ff"),
                                      /*hopsLeft=*/5,
                                      /*seqNo=*/2));

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(), 0, "No relaying with ForwardMesh disabled");
        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount, 1, "The broadcast frame is still delivered");
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Three-node chain: the middle node decides whether the far node is reached.
 *
 * A frame from node A (00:02) to node C (00:03) arrives at the middle
 * node B (00:01) only; whatever B transmits reaches C. When B is part
 * of the mesh and forwards, C receives the packet. When B is outside
 * the mesh, C receives nothing.
 */
class MeshUnderChainRelayTestCase : public MeshUnderDeviceTestCase
{
  public:
    /**
     * @brief Constructor.
     * @param middleInMesh Whether the middle node takes part in the mesh.
     */
    MeshUnderChainRelayTestCase(bool middleInMesh)
        : MeshUnderDeviceTestCase(middleInMesh
                                      ? "3-node chain: middle node relays to the far node"
                                      : "3-node chain: middle node outside the mesh, no relay"),
          m_middleInMesh(middleInMesh)
    {
    }

  private:
    /**
     * @brief Deliver whatever the middle node transmits to the far node,
     *        as a channel would.
     * @param device The sending device.
     * @param packet The transmitted frame.
     * @param protocol The protocol number.
     * @param source The source address.
     * @param destination The destination address.
     * @param packetType The packet type.
     * @return true
     */
    bool RelayToFarNode(Ptr<NetDevice> device,
                        Ptr<const Packet> packet,
                        uint16_t protocol,
                        const Address& source,
                        const Address& destination,
                        NetDevice::PacketType packetType)
    {
        Simulator::ScheduleWithContext(m_mockFar->GetNode()->GetId(),
                                       MicroSeconds(100),
                                       &MockNetDevice::Receive,
                                       m_mockFar,
                                       packet->Copy(),
                                       protocol,
                                       m_mockFar->GetBroadcast(),
                                       source,
                                       NetDevice::PACKET_BROADCAST);
        return true;
    }

    /**
     * @brief Count packets delivered on the far node.
     * @param device The receiving device.
     * @param packet The delivered packet.
     * @param protocol The protocol number.
     * @param source The source address.
     * @return true
     */
    bool CaptureUpFar(Ptr<NetDevice> device,
                      Ptr<const Packet> packet,
                      uint16_t protocol,
                      const Address& source)
    {
        m_deliveredFar++;
        return true;
    }

    void DoRun() override
    {
        // Middle node (B, 00:01): the base fixture device.
        SetupDevice(/*useMesh=*/m_middleInMesh, /*forwardMesh=*/true);
        m_mock->SetSendCallback(MakeCallback(&MeshUnderChainRelayTestCase::RelayToFarNode, this));

        // Far node (C, 00:03).
        Ptr<Node> nodeFar = CreateObject<Node>();
        m_mockFar = CreateObject<MockNetDevice>();
        nodeFar->AddDevice(m_mockFar);
        m_mockFar->SetNode(nodeFar);
        m_mockFar->SetAddress(Mac48Address("00:00:00:00:00:03"));
        m_mockFar->SetMtu(150);
        m_devFar = CreateObject<SixLowPanNetDevice>();
        nodeFar->AddDevice(m_devFar);
        m_devFar->SetNetDevice(m_mockFar);
        m_devFar->SetAttribute("UseMeshUnder", BooleanValue(true));
        m_devFar->SetReceiveCallback(
            MakeCallback(&MeshUnderChainRelayTestCase::CaptureUpFar, this));

        // A frame from A (00:02) to C (00:03) arrives at B only.
        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(Mac16Address("00:02"),
                                      Mac16Address("00:03"),
                                      /*hopsLeft=*/5,
                                      /*seqNo=*/1));

        Simulator::Stop(MilliSeconds(100));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_deliveredFar,
                              (m_middleInMesh ? 1 : 0),
                              "Far node delivery depends on the middle node relaying");
        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount, 0, "The frame is not for the middle node");
    }

    void DoTeardown() override
    {
        m_devFar = nullptr;
        m_mockFar = nullptr;
        MeshUnderDeviceTestCase::DoTeardown();
    }

    bool m_middleInMesh;              ///< Whether the middle node takes part in the mesh.
    Ptr<MockNetDevice> m_mockFar;     ///< Far node underlying (mock) device.
    Ptr<SixLowPanNetDevice> m_devFar; ///< Far node device under test.
    uint32_t m_deliveredFar{0};       ///< Packets delivered on the far node.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief The device default policy and stream assignment match the
 *        historical behavior.
 */
class MeshUnderDeviceStreamsTestCase : public TestCase
{
  public:
    MeshUnderDeviceStreamsTestCase()
        : TestCase("Default policy exists at construction; AssignStreams returns 2")
    {
    }

  private:
    void DoRun() override
    {
        Ptr<SixLowPanNetDevice> dev = CreateObject<SixLowPanNetDevice>();

        // The MeshUnderRouting attribute default provides a policy at
        // construction, so stream assignment is effective during setup.
        Ptr<SixLowPanMeshUnderRouting> policy = dev->GetMeshUnderRouting();
        NS_TEST_ASSERT_MSG_NE(policy, nullptr, "Default policy must exist at construction");
        NS_TEST_ASSERT_MSG_NE(DynamicCast<SixLowPanSimpleFlooding>(policy),
                              nullptr,
                              "Default policy must be SixLowPanSimpleFlooding");

        // The historical implementation assigned two streams (device RNG and
        // forwarding jitter).
        NS_TEST_ASSERT_MSG_EQ(dev->AssignStreams(7), 2, "Two streams, as before the refactoring");
    }

    void DoTeardown() override
    {
        Simulator::Destroy();
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief 6LoWPAN mesh-under forwarding test suite.
 */
class SixLowPanMeshUnderTestSuite : public TestSuite
{
  public:
    SixLowPanMeshUnderTestSuite()
        : TestSuite("sixlowpan-mesh-under", Type::UNIT)
    {
        AddTestCase(new DuplicateCacheFifoDropTestCase, Duration::QUICK);
        AddTestCase(new DuplicateCachePerOriginatorTestCase, Duration::QUICK);
        AddTestCase(new SimpleFloodingForwardsTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceForwardTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceDeliveryTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceHopLimitTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceMisconfigTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceNoRelayTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderChainRelayTestCase(true), Duration::QUICK);
        AddTestCase(new MeshUnderChainRelayTestCase(false), Duration::QUICK);
        AddTestCase(new MeshUnderDeviceStreamsTestCase, Duration::QUICK);
    }
};

/// Static suite registration.
static SixLowPanMeshUnderTestSuite g_sixLowPanMeshUnderTestSuite;
