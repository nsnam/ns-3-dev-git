/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Davide Magrin <davide@magr.in>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/minstrel-ht-wifi-manager.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-list.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/pointer.h"
#include "ns3/power-save-manager.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/socket.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/tim.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ns3-constants.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-static-setup-helper.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <iterator>
#include <list>
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PowerSaveTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test TIM Information element serialization and deserialization
 */
class TimInformationElementTest : public HeaderSerializationTestCase
{
  public:
    /**
     * @brief Constructor
     */
    TimInformationElementTest();

    void DoRun() override;
    /**
     * Reset the passed TIM to have the provided parameters.
     *
     * @param tim the TIM element to set
     * @param dtimCount the DTIM count value
     * @param dtimPeriod the DTIM period value
     * @param multicastPending whether group addressed frames are queued
     * @param aidValues the AID values to set
     */
    void SetTim(Tim& tim,
                uint8_t dtimCount,
                uint8_t dtimPeriod,
                bool multicastPending,
                const std::list<uint16_t>& aidValues);

    /**
     * Test that the Bitmap Control and the Partial Virtual Bitmap
     * fields of the provided TIM match the passed bufferContents.
     *
     * @param tim the provided TIM
     * @param bufferContents the expected content of the buffer
     */
    void CheckSerializationAgainstBuffer(Tim& tim, const std::vector<uint8_t>& bufferContents);

    /**
     * Test that the GetAidSet() method return the expected set of AID values.
     *
     * @param tim the TIM element
     * @param aid the AID value passed to GetAidSet()
     * @param expectedSet the expected set of AID values returned by GetAidSet()
     */
    void CheckAidSet(const Tim& tim, uint16_t aid, const std::set<uint16_t>& expectedSet);
};

TimInformationElementTest::TimInformationElementTest()
    : HeaderSerializationTestCase("Test for the TIM Information Element implementation")
{
}

void
TimInformationElementTest::SetTim(Tim& tim,
                                  uint8_t dtimCount,
                                  uint8_t dtimPeriod,
                                  bool multicastPending,
                                  const std::list<uint16_t>& aidValues)
{
    tim = Tim();
    tim.m_dtimCount = dtimCount;
    tim.m_dtimPeriod = dtimPeriod;
    tim.m_hasMulticastPending = multicastPending;
    tim.AddAid(aidValues.begin(), aidValues.end());
}

void
TimInformationElementTest::CheckSerializationAgainstBuffer(
    Tim& tim,
    const std::vector<uint8_t>& bufferContents)
{
    // Serialize the TIM
    Buffer buffer;
    buffer.AddAtStart(tim.GetSerializedSize());
    tim.Serialize(buffer.Begin());

    // Check the two Buffer instances
    Buffer::Iterator bufferIterator = buffer.Begin();
    for (uint32_t j = 0; j < buffer.GetSize(); j++)
    {
        // We skip the first four bytes, since they contain known information
        if (j > 3)
        {
            NS_TEST_EXPECT_MSG_EQ(bufferIterator.ReadU8(),
                                  bufferContents.at(j - 4),
                                  "Serialization is different than provided known serialization");
        }
        else
        {
            // Advance the serialized buffer, which also contains
            // the Element ID, Length, DTIM Count, DTIM Period fields
            bufferIterator.ReadU8();
        }
    }
}

void
TimInformationElementTest::CheckAidSet(const Tim& tim,
                                       uint16_t aid,
                                       const std::set<uint16_t>& expectedSet)
{
    auto ret = tim.GetAidSet(aid);

    {
        std::vector<uint16_t> diff;

        // Expected set minus returned set provides expected elements that are not returned
        std::set_difference(expectedSet.cbegin(),
                            expectedSet.cend(),
                            ret.cbegin(),
                            ret.cend(),
                            std::back_inserter(diff));

        std::stringstream ss;
        std::copy(diff.cbegin(), diff.cend(), std::ostream_iterator<uint16_t>(ss, " "));

        NS_TEST_EXPECT_MSG_EQ(diff.size(),
                              0,
                              "Expected elements not returned by GetAidSet(): " << ss.str());
    }
    {
        std::vector<uint16_t> diff;

        // Returned set minus expected set provides returned elements that are not expected
        std::set_difference(ret.cbegin(),
                            ret.cend(),
                            expectedSet.cbegin(),
                            expectedSet.cend(),
                            std::back_inserter(diff));

        std::stringstream ss;
        std::copy(diff.cbegin(), diff.cend(), std::ostream_iterator<uint16_t>(ss, " "));

        NS_TEST_EXPECT_MSG_EQ(diff.size(),
                              0,
                              "Returned elements not expected by GetAidSet(): " << ss.str());
    }
}

void
TimInformationElementTest::DoRun()
{
    Tim tim;

    // The first three examples from 802.11-2020, Annex L
    //
    // 1. No group addressed MSDUs, but there is traffic for STAs with AID 2 and AID 7
    SetTim(tim, 0, 3, false, {2, 7});
    TestHeaderSerialization(tim);
    CheckSerializationAgainstBuffer(tim, {0b00000000, 0b10000100});
    CheckAidSet(tim, 0, {2, 7});
    CheckAidSet(tim, 1, {2, 7});
    CheckAidSet(tim, 2, {7});
    CheckAidSet(tim, 7, {});
    //
    // 2. There are group addressed MSDUs, DTIM count = 0, the nodes
    // with AID 2, 7, 22, and 24 have data buffered in the AP
    SetTim(tim, 0, 3, true, {2, 7, 22, 24});
    TestHeaderSerialization(tim);
    CheckSerializationAgainstBuffer(tim,
                                    {
                                        0b00000001,
                                        // NOTE The following byte is different from the example
                                        // in the standard. This is because the example sets the
                                        // AID 0 bit in the partial virtual bitmap to 1. Our code
                                        // and the example code provided in the Annex, instead, do
                                        // not set this bit. Relevant Note from 802.11-2020,
                                        // Section 9.4.2.5.1: "The bit numbered 0 in the traffic
                                        // indication virtual bitmap need not be included in the
                                        // Partial Virtual Bitmap field even if that bit is set."
                                        0b10000100,
                                        0b00000000,
                                        0b01000000,
                                        0b00000001,
                                    });
    CheckAidSet(tim, 0, {2, 7, 22, 24});
    CheckAidSet(tim, 2, {7, 22, 24});
    CheckAidSet(tim, 7, {22, 24});
    CheckAidSet(tim, 22, {24});
    CheckAidSet(tim, 24, {});
    //
    // 3. There are group addressed MSDUs, DTIM count = 0, only the node
    // with AID 24 has data buffered in the AP
    SetTim(tim, 0, 3, true, {24});
    TestHeaderSerialization(tim);
    CheckSerializationAgainstBuffer(tim, {0b00000011, 0b00000000, 0b00000001});

    // Other arbitrary examples just to make sure
    // Serialization -> Deserialization -> Serialization works
    SetTim(tim, 0, 3, false, {2000});
    TestHeaderSerialization(tim);
    SetTim(tim, 1, 3, true, {1, 134});
    TestHeaderSerialization(tim);
    SetTim(tim, 1, 3, false, {1, 2});
    TestHeaderSerialization(tim);

    // Edge cases
    //
    // What if there is group addressed data only?
    //
    // In this case, we should still have an empty byte in the Partial Virtual Bitmap.
    // From 802.11-2020: in the event that all bits other than bit 0 in the traffic indication
    // virtual bitmap are 0, the Partial Virtual Bitmap field is encoded as a single octet
    // equal to 0, the Bitmap Offset subfield is 0, and the Length field is 4.
    SetTim(tim, 0, 3, true, {});
    TestHeaderSerialization(tim);
    CheckSerializationAgainstBuffer(tim, {0b00000001, 0b00000000});
    NS_TEST_EXPECT_MSG_EQ(tim.GetSerializedSize() - 2, 4, "Unexpected TIM Length");
    //
    // What if there is no group addressed data and no unicast data?
    //
    // From 802.11-2020: When the TIM is carried in a non-S1G PPDU, in the event that all bits
    // other than bit 0 in the traffic indication virtual bitmap are 0, the Partial Virtual Bitmap
    // field is encoded as a single octet equal to 0, the Bitmap Offset subfield is 0, and the
    // Length field is 4.
    SetTim(tim, 0, 3, false, {});
    TestHeaderSerialization(tim);
    CheckSerializationAgainstBuffer(tim, {0b00000000, 0b00000000});
    NS_TEST_EXPECT_MSG_EQ(tim.GetSerializedSize() - 2, 4, "Unexpected TIM Length");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test rate manager that differs from MinstrelHT in that it allows to arbitrarily drop
 * frames by overriding the DoGetMpdusToDropOnTxFailure method.
 */
class PowerSaveTestRateManager : public MinstrelHtWifiManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::PowerSaveTestRateManager")
                                .SetParent<MinstrelHtWifiManager>()
                                .SetGroupName("Wifi")
                                .AddConstructor<PowerSaveTestRateManager>();
        return tid;
    }

    bool m_drop{false}; ///< whether to drop the given MPDUs

  private:
    std::list<Ptr<WifiMpdu>> DoGetMpdusToDropOnTxFailure(WifiRemoteStation* station,
                                                         Ptr<WifiPsdu> psdu) override
    {
        if (!m_drop)
        {
            return {};
        }

        std::list<Ptr<WifiMpdu>> mpdusToDrop;

        for (const auto& mpdu : *PeekPointer(psdu))
        {
            mpdusToDrop.push_back(mpdu);
        }

        return mpdusToDrop;
    }
};

NS_OBJECT_ENSURE_REGISTERED(PowerSaveTestRateManager);

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test transmission of broadcast and unicast frames with STAs in Power Save mode
 *
 * The test scenario comprises an AP MLD with two links, a non-AP STA (operating on link 0) and a
 * non-AP MLD with two links. The non-AP MLD negotiates a TID-to-Link Mapping such that (both in
 * DL and UL) TID 0 is mapped on both links and TID 2 is mapped on link 1 only. The sequence of
 * events is depicted in the diagram below. Note that, for most of the time, the non-AP STA
 * affiliated with the non-AP MLD and operating on link 0 is in active mode, while the non-AP STA
 * affiliated with the non-AP MLD and operating on link 1 is in power save mode.
 *
 * The standard supported by the non-AP STA is a parameter of this test, while the non-AP MLD
 * supports the 802.11be standard.
 *
   \verbatim
               (1)            (2)                   (3)                           (4) non-AP STA
                                                                                      is set to go
            ┌──────┐                       ┌──────┐   Broadcast  ┌─────────┐  ┌──────┐to PS mode
            │Beacon│   non-AP STA performs │Beacon│ frame queued │Broadcast│  │Beacon││      ┌───┐
   [link 0] │ DTIM │     association       │ TIM  │    at AP  │  │QoS Data │  │ DTIM ││      │Ack│
   ─────────┴──────┴───────────────────────┴──────┴───────────▼──┴─────────┴──┴──────┴▼┬────┬┴───┴─
                                                              │Not buffered            │Data│
                         non-AP MLD performs                  │(no STA in PS mode)     │Null│
                ┌──────┐ association and sends Data ┌──────┐  │  ┌─────────┐  ┌──────┐ └────┘
                │Beacon│ Null for other non-AP STA  │Beacon│  │  │Broadcast│  │Beacon│       ┌───┐
   [link 1]     │ DTIM │ to switch to active mode   │ TIM  │  │  │QoS Data │  │ DTIM │       │Ack│
   ─────────────┴──────┴────────────────────────────┴──────┴──▼──┴─────────┴──┴──────┴▲┬────┬┴───┴─
                                                                                      ││Data│
                                                                                      ││Null│
                                                                                      │└────┘
                                                                              non-AP STA affiliated
                                                                          with non-AP MLD on link 1
                                                                            is set to go to PS mode
   (CONTINUED)

                             (5)          (6)            (7)         (8)               (9)
                      QoS data frame with    another broadcast frame       2 frames for non-AP
      1 broadcast      TID 4 queued for |        |queued at AP               STA are queued at
      frame queued     ┌──────┐ non-AP ┌──────┐ ┌─────────┐ ┌────────────────┐ AP and buffered
        at AP │        │Beacon│    MLD │Beacon│ │Broadcast│ │ BA agreement + │ │
   [link 0]   │        │ TIM  │        │ DTIM │ │QoS Data │ │unicast QoS Data│ │
   ───────────▼────────┴──────┴────────┴▼─────┴─┴▼────────┴─┴────────────────┴─▼───────────────────
              │                         │        │
   non-AP STA -sleep--│        │------│ │        │        │----------------------------------------
   non-AP STA │                         │        │
   affiliated ---sleep--------│        │------------------------│                                │-
   on link 1  │                  No DTIM,        │
              │Buffered because  frames still    │
              │STAs in PS mode   buffered        │
              │                ┌──────┐ │        │               ┌──────┐ ┌─────────┐ ┌─────────┐
              │                │Beacon│ │        │               │Beacon│ │Broadcast│ │Broadcast│
   [link 1]   │                │ TIM  │ │        │               │ DTIM │ │QoS Data │ │QoS Data │
   ───────────▼────────────────┴──────┴─▼────────▼───────────────┴──────┴─┴─────────┴─┴─────────┴──



   (CONTINUED)
                       (10)                      (11)        (12)               (13)

            ┌──────┐       ┌────────┐                               ┌──────┐       ┌────────┐
            │Beacon│       │QoS Data│                  PS-Poll      │Beacon│       │QoS Data│
   [link 0] │ TIM  │       │ to STA │                  dropped      │ DTIM │       │ to STA │
   ─────────┴──────┴─┬────┬┴────────┴┬───┬─┬────X───┬────X──────────┴──────┴─┬────┬┴────────┴┬───┬─
                     │ PS │          │ACK│ │ PS │...│ PS │                   │ PS │          │ACK│
                     │Poll│          └───┘ │Poll│   │Poll│                   │Poll│          └───┘
                     └────┘                └────┘   └────┘                   └────┘
   non-AP STA                                             │--sleep-│                              │-
   non-AP STA
   affiliated --------sleep-------------------------------│        │-------------------------------
   on link 1
                                                           ┌──────┐ drop broadcast
                                                           │Beacon│ frame queued for
   [link 1]                                                │ TIM  │ TX on link 0
   ────────────────────────────────────────────────────────┴──────┴────────────────────────────────


   (CONTINUED)

                                (14)            (15)          (16)                    (17)
     2 QoS Data frames with                   2 QoS Data frames
       TID 0 for non-AP MLD                    with TID 2 for
       are queued at AP but                    non-AP MLD are
     not buffered as STA on  ┌────────┬────────┐ queued and ┌──────┐
   link 0 is in active mode  │QoS Data│QoS Data│  buffered  │Beacon│
   [link 0]           │      │ to MLD │ to MLD │     │      │ TIM  │
   ───────────────────▼──────┴────────┴────────┴┬──┬────────┴──────┴───────────────────────────────
                      │ Block Ack               │BA│ │
                      │ agreement               └──┘ │
                      │ establishment                │
   non-AP STA ------------------sleep----------------------│        │------------------------------
   non-AP STA         │                              │
   affiliated       │--------sleep-----------------------------------│
   on link 1          │                              │
            ┌──────┐  │                              │                ┌──────┐       ┌────────┐
            │Beacon│  │                              │                │Beacon│       │QoS Data│
   [link 1] │ DTIM │  │                              │                │ TIM  │       │ to MLD │
   ─────────┴──────┴──▼──────────────────────────────▼────────────────┴──────┴─┬────┬┴────────X────
                                                                               │ PS │
                                                                               │Poll│
                                                                               └────┘


   (CONTINUED)
                             (17)                                          (18)
                                                    2 QoS Data frames
                                                    are queued at
                                                    non-AP STA         ADDBA
                                                          │       ┌───┐Resp      ┌───┐      ┌───┐
   [link 0]                                               │       │Ack│timer     │Ack│      │Ack│
   ───────────────────────────────────────────────────────▼┬─────┬┴───┴────┬────┬┴───┴┬────┬┴───┴──
                                                           │ADDBA│         │QoS │     │QoS │
                                                           │ Req │         │Data│     │Data│
                                                           └─────┘         └────┘     └────┘
   non-AP STA ------------------sleep---------------------│            │--│                      │-
   non-AP STA
   affiliated                                             │--------sleep---------------------------
   on link 1
                    ┌────────┐             ┌────────┐
                    │QoS Data│             │QoS Data│
   [link 1]         │ to MLD │             │ to MLD │
   ───────────┬────┬┴────────┴┬───┬──┬────┬┴────────┴┬───┬─────────────────────────────────────────
              │ PS │          │ACK│  │ PS │          │ACK│
              │Poll│          └───┘  │Poll│          └───┘
              └────┘                 └────┘



   (CONTINUED)
                      (19)                        (20)                          (21)
                                    2 QoS Data frames
            ┌──────┐        ┌─────┐ with TID 0 are queued   ┌─────┐
            │Beacon│        │ADDBA│ at non-AP MLD  ┌───┐    │ADDBA│
   [link 0] │ DTIM │        │Resp │     |          │Ack│    │Resp │
   ─────────┴──────┴──┬────┬┴─────┴┬───┬▼───┬─────┬┴───┴────┴─────┴┬───┬───────────────────────────
                      │ PS │       │Ack│|   │ADDBA│                │Ack│
                      │Poll│       └───┘|   │ Req │                └───┘
                      └────┘            |   └─────┘
   non-AP STA                           │------------------sleep-----------------------------------
   non-AP STA                           |
   affiliated --------sleep-------------|   backoff   |------sleep-----|                 |--sleep--
   on link 1                            |                                   can also
                                        |                                   occur on  ┌──┐
   [link 1]                             |                                   link 0    │BA│
   ─────────────────────────────────────▼──────────────────────────────────┬────┬────┬┴──┴─────────
                                                                           │QoS │QoS │
                                                                           │Data│Data│
                                                                           └────┴────┘



   (CONTINUED)
                       (22)                      (23)
    a QoS Data frame       frame dropped   a QoS Data frame with
    with TID 0 for non-AP  at AckTimeout,  TID 0 for non-AP MLD
    MLD is queued at AP    non-AP STA on   is queued and buffered
    MLD but not  ┌────┐    link 0 set to   at AP  ┌──────┐                         ┌────┐
     buffered  | │QoS │    go to  ┌───┐    MLD    │Beacon│        ┌───┐            │QoS │
   [link 0]    | │Data│    PS mode│Ack│          |│ TIM  │        │BAR│            │Data│
   ────────────▼─┴────X─────┬────┬┴───┴──────────▼┴──────┴──┬────┬┴───┴┬──┬──┬────┬┴────┴┬───┬─────
               |            │Data│               |          │ PS │     │BA│  │ PS │      │Ack│
               |            │Null│               |          │Poll│     └──┘  │Poll│      └───┘
               |            └────┘               |          └────┘           └────┘
   non-AP STA ------------sleep------------------|        |----------------------------------------
   non-AP STA  |                                 |
   affiliated  |                       |--sleep--|                                            |----
   on link 0   |                                 |
   non-AP STA  |                                 |
   affiliated ---------------sleep------|        |-------sleep-------------------------------------
   on link 1   |                         ┌──────┐|
               |                         │Beacon│|
   [link 1]    |                         │ DTIM │|
   ────────────▼─────────────────────────┴──────┴▼─────────────────────────────────────────────────

   \endverbatim
 */
class WifiPowerSaveModeTest : public TestCase
{
  public:
    /**
     * Constructor
     * @param standard the standard supported by the non-AP STA
     */
    WifiPowerSaveModeTest(WifiStandard standard);

    /**
     * Callback invoked when the PHY state helper reports a change in PHY state
     *
     * @param nodeId the ID of the node
     * @param phyId the ID of the PHY that changed state
     * @param stateStart the start time of the period elapsed in the given PHY state
     * @param stateDuration the duration of the period elapsed in the given PHY state
     * @param state the given PHY state
     */
    void NotifyStateChange(uint32_t nodeId,
                           uint8_t phyId,
                           Time stateStart,
                           Time stateDuration,
                           WifiPhyState state);

    /**
     * Notify that the given MPDU was enqueued at the AP.
     *
     * @param mpdu the given MPDU
     */
    void NotifyMpduEnqueued(Ptr<const WifiMpdu> mpdu);

    /**
     * Callback invoked when PHY receives a PSDU to transmit
     *
     * @param mac the MAC transmitting the PSDUs
     * @param phyId the ID of the PHY transmitting the PSDUs
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPower the tx power
     */
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  Watt_u txPower);

  protected:
    /**
     * Function to trace packets received by the server application
     * @param nodeId the ID of the node that received the packet
     * @param p the packet
     * @param addr the address
     */
    void L7Receive(uint8_t nodeId, Ptr<const Packet> p, const Address& addr);

    /**
     * @param sockAddr the packet socket address identifying local outgoing interface
     *                 and remote address
     * @param count the number of packets to generate
     * @param pktSize the size of the packets to generate
     * @param delay the delay with which traffic generation starts
     * @param priority user priority for generated packets
     * @return an application generating the given number packets of the given size destined
     *         to the given packet socket address
     */
    Ptr<PacketSocketClient> GetApplication(const PacketSocketAddress& sockAddr,
                                           std::size_t count,
                                           std::size_t pktSize,
                                           Time delay = Time{0},
                                           uint8_t priority = 0) const;

    /// Actions and checks to perform upon the transmission of each frame
    struct Events
    {
        /**
         * Constructor.
         *
         * @param type the frame MAC header type
         * @param f function to perform actions and checks
         */
        Events(WifiMacType type,
               std::function<void(Ptr<const WifiPsdu>, const Time&, uint8_t)>&& f = {})
            : hdrType(type),
              func(f)
        {
        }

        WifiMacType hdrType; ///< MAC header type of frame being transmitted
        std::function<void(Ptr<const WifiPsdu>, const Time&, uint8_t)>
            func; ///< function to perform actions and checks
    };

    /// Set the list of events to expect in this test run.
    void SetEvents();

  private:
    void DoSetup() override;
    void DoRun() override;

    WifiStandard m_standard;                        //!< the wifi standard for AP and STA
    Ptr<ApWifiMac> m_apMac;                         ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs;         ///< STA wifi MACs
    std::list<Events> m_events;                     //!< list of events for a test run
    std::size_t m_processedEvents{0};               //!< number of processed events
    Time m_duration{Seconds(5)};                    ///< simulation duration
    std::array<std::size_t, 3> m_rxPkts{{0, 0, 0}}; ///< number of packets received at application
                                                    ///< layer by each node (index is node ID)
    std::vector<double> m_jitter{0.2, 0.6};   ///< jitter for the initial Beacon frames on the links
    Ptr<ListErrorModel> m_apErrorModel;       ///< error model to install on the AP MLD
    Ptr<ListErrorModel> m_nonApStaErrorModel; ///< error model to install on the non-AP STA
    Ptr<ListErrorModel> m_nonApMldErrorModel; ///< error model to install on the non-AP MLD
};

WifiPowerSaveModeTest::WifiPowerSaveModeTest(WifiStandard standard)
    : TestCase("Test operations in powersave mode"),
      m_standard(standard),
      m_apErrorModel(CreateObject<ListErrorModel>()),
      m_nonApStaErrorModel(CreateObject<ListErrorModel>()),
      m_nonApMldErrorModel(CreateObject<ListErrorModel>())
{
}

Ptr<PacketSocketClient>
WifiPowerSaveModeTest::GetApplication(const PacketSocketAddress& sockAddr,
                                      std::size_t count,
                                      std::size_t pktSize,
                                      Time delay,
                                      uint8_t priority) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetAttribute("Priority", UintegerValue(priority));
    client->SetRemote(sockAddr);
    client->SetStartTime(delay);
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
WifiPowerSaveModeTest::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);
    int64_t streamNumber = 100;

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNodes(2);

    // create MLDs
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::PowerSaveTestRateManager");

    auto channel = CreateObject<MultiModelSpectrumChannel>();
    SpectrumWifiPhyHelper phy(2);
    phy.SetChannel(channel);
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.Set(0, "ChannelSettings", StringValue("{36, 0, BAND_5GHZ, 0}"));
    phy.Set(1, "ChannelSettings", StringValue("{120, 0, BAND_5GHZ, 0}"));

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac",
                "QosSupported",
                BooleanValue(true),
                "BeaconDtimPeriod",
                UintegerValue(2),
                "FrameRetryLimit",
                UintegerValue(5), // ensure PS-Poll is retransmitted 5 times
                "Ssid",
                SsidValue(Ssid("power-save-ssid")));

    auto apDev = wifi.Install(phy, mac, wifiApNode);

    // Force non-AP MLD to perform association on link 1, while non-AP STA performs association
    // on link 0, so that there is no risk of collision among association frames
    phy.Set(0, "ChannelSettings", StringValue("{60, 0, BAND_5GHZ, 0}"));

    mac.SetType("ns3::StaWifiMac",
                "QosSupported",
                BooleanValue(true),
                "Ssid",
                SsidValue(Ssid("power-save-ssid")));
    mac.SetPowerSaveManager("ns3::DefaultPowerSaveManager");

    // TID 0 is mapped on both links, TID 2 is mapped on link 1 only
    wifi.ConfigEhtOptions("TidToLinkMappingDl", StringValue("0 0,1; 2 1"));
    wifi.ConfigEhtOptions("TidToLinkMappingUl", StringValue("0 0,1; 2 1"));

    auto staDev = wifi.Install(phy, mac, wifiStaNodes.Get(0));

    // create SLD
    wifi.SetStandard(m_standard);
    phy = SpectrumWifiPhyHelper{};
    phy.SetChannel(channel);
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.Set(0, "ChannelSettings", StringValue("{36, 0, BAND_5GHZ, 0}"));

    staDev.Add(wifi.Install(phy, mac, wifiStaNodes.Get(1)));

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(NetDeviceContainer(staDev, apDev), streamNumber);

    auto positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, 1.0, 0.0));
    MobilityHelper mobility;
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDev.Get(0))->GetMac());
    m_staMacs.push_back(
        DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(staDev.Get(0))->GetMac()));
    m_staMacs.push_back(
        DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(staDev.Get(1))->GetMac()));

    // define the jitter for the initial Beacon frames on the two links, so that Beacon frames
    // on the two links are spaced enough, as expected by the checks in this test
    auto beaconJitter = CreateObject<DeterministicRandomVariable>();
    beaconJitter->SetValueArray(m_jitter);
    m_apMac->SetAttribute("BeaconJitter", PointerValue(beaconJitter));

    // Trace PSDUs passed to the PHY on all devices
    for (const auto& mac : std::list<Ptr<WifiMac>>{m_apMac, m_staMacs[0], m_staMacs[1]})
    {
        for (uint8_t phyId = 0; phyId < mac->GetDevice()->GetNPhys(); ++phyId)
        {
            mac->GetDevice()->GetPhy(phyId)->TraceConnectWithoutContext(
                "PhyTxPsduBegin",
                MakeCallback(&WifiPowerSaveModeTest::Transmit, this).Bind(mac, phyId));
        }
    }

    // install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);

    // install a packet socket server on all nodes
    for (auto nodeIt = NodeList::Begin(); nodeIt != NodeList::End(); ++nodeIt)
    {
        PacketSocketAddress srvAddr;
        auto device = DynamicCast<WifiNetDevice>((*nodeIt)->GetDevice(0));
        NS_TEST_ASSERT_MSG_NE(device, nullptr, "Expected a WifiNetDevice");
        srvAddr.SetSingleDevice(device->GetIfIndex());
        srvAddr.SetProtocol(1);

        auto server = CreateObject<PacketSocketServer>();
        server->SetLocal(srvAddr);
        (*nodeIt)->AddApplication(server);
        server->SetStartTime(Seconds(0)); // now
        server->SetStopTime(m_duration);
    }

    for (std::size_t nodeId = 0; nodeId < NodeList::GetNNodes(); nodeId++)
    {
        Config::ConnectWithoutContext(
            "/NodeList/" + std::to_string(nodeId) +
                "/ApplicationList/*/$ns3::PacketSocketServer/Rx",
            MakeCallback(&WifiPowerSaveModeTest::L7Receive, this).Bind(nodeId));
    }

    // Trace PHY state changes at the non-AP STAs
    for (uint32_t id = 0; id < 2; ++id)
    {
        for (uint8_t phyId = 0; phyId < m_staMacs[id]->GetDevice()->GetNPhys(); ++phyId)
        {
            m_staMacs[id]->GetDevice()->GetPhy(phyId)->GetState()->TraceConnectWithoutContext(
                "State",
                MakeCallback(&WifiPowerSaveModeTest::NotifyStateChange, this).Bind(id, phyId));
        }
    }

    // Trace MPDUs enqueued at the AP
    Config::ConnectWithoutContext(
        "/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_Txop/Queue/Enqueue",
        MakeCallback(&WifiPowerSaveModeTest::NotifyMpduEnqueued, this));
    Config::ConnectWithoutContext(
        "/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/BK_Txop/Queue/Enqueue",
        MakeCallback(&WifiPowerSaveModeTest::NotifyMpduEnqueued, this));

    // Install post reception error models
    m_apMac->GetWifiPhy(0)->SetPostReceptionErrorModel(m_apErrorModel);
    m_apMac->GetWifiPhy(1)->SetPostReceptionErrorModel(m_apErrorModel);
    m_staMacs[0]->GetWifiPhy(0)->SetPostReceptionErrorModel(m_nonApMldErrorModel);
    m_staMacs[0]->GetWifiPhy(1)->SetPostReceptionErrorModel(m_nonApMldErrorModel);
    m_staMacs[1]->GetWifiPhy(0)->SetPostReceptionErrorModel(m_nonApStaErrorModel);

    SetEvents();
}

void
WifiPowerSaveModeTest::Transmit(Ptr<WifiMac> mac,
                                uint8_t phyId,
                                WifiConstPsduMap psduMap,
                                WifiTxVector txVector,
                                Watt_u txPower)
{
    const auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link found for PHY ID " << +phyId);
    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psduMap.begin()->second->GetHeader(0);
    const auto txDuration =
        WifiPhy::CalculateTxDuration(psdu, txVector, mac->GetWifiPhy(*linkId)->GetPhyBand());

    const auto checkFrame =
        (hdr.IsBeacon() || (m_apMac->IsAssociated(m_staMacs[0]->GetAddress()).has_value() &&
                            m_apMac->IsAssociated(m_staMacs[1]->GetAddress()).has_value()));

    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        if (checkFrame && !m_events.empty())
        {
            ss << "PSDU #" << ++m_processedEvents;
        }

        ss << std::setprecision(10) << " Link ID " << +linkId.value() << " Phy ID " << +phyId
           << " TX duration=" << txDuration.As(Time::MS) << " #MPDUs " << psdu->GetNMpdus();
        for (auto it = psdu->begin(); it != psdu->end(); ++it)
        {
            ss << "\n" << **it;
        }
        NS_LOG_INFO(ss.str());
    }
    NS_LOG_INFO("TXVECTOR = " << txVector << "\n");

    if (!checkFrame)
    {
        return;
    }

    if (!m_events.empty())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(WifiMacHeader(m_events.front().hdrType).GetTypeString(),
                              hdr.GetTypeString(),
                              "Unexpected MAC header type for frame transmitted at time "
                                  << Simulator::Now().As(Time::US));
        // perform actions/checks, if any
        if (m_events.front().func)
        {
            m_events.front().func(psdu, txDuration, *linkId);
        }

        m_events.pop_front();
    }
}

void
WifiPowerSaveModeTest::L7Receive(uint8_t nodeId, Ptr<const Packet> p, const Address& addr)
{
    NS_LOG_INFO("Packet received by NODE " << +nodeId << "\n");
    m_rxPkts[nodeId]++;
}

void
WifiPowerSaveModeTest::DoRun()
{
    Simulator::Stop(Seconds(1));
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

    Simulator::Destroy();
}

void
WifiPowerSaveModeTest::NotifyStateChange(uint32_t nodeId,
                                         uint8_t phyId,
                                         Time stateStart,
                                         Time stateDuration,
                                         WifiPhyState state)
{
    if (state == WifiPhyState::SLEEP)
    {
        NS_LOG_DEBUG("PHY " << +phyId << " of STA " << nodeId << " was in sleep mode from "
                            << stateStart.As(Time::S) << " to "
                            << (stateStart + stateDuration).As(Time::S) << "\n");
    }
}

void
WifiPowerSaveModeTest::NotifyMpduEnqueued(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_DEBUG("QUEUED " << *mpdu << "\n");
}

void
WifiPowerSaveModeTest::SetEvents()
{
    constexpr auto DTIM = true;
    constexpr auto NO_DTIM = false;
    constexpr auto MCAST_PENDING = true;
    constexpr auto MCAST_NOT_PENDING = false;
    constexpr std::size_t NON_AP_MLD = 0;
    constexpr std::size_t NON_AP_STA = 1;
    constexpr auto SLEEP = true;
    constexpr auto AWAKE = false;

    // lambda to check the TIM element contained in the given Beacon frame
    auto checkTim = [this](Ptr<WifiMpdu> mpdu,
                           bool isDtim,
                           bool hasMulticastPending,
                           const std::map<uint16_t, bool>& aidIsSetMap) {
        const auto now = Simulator::Now();
        NS_TEST_ASSERT_MSG_EQ(mpdu->GetHeader().IsBeacon(),
                              true,
                              "Expected PSDU transmitted at " << now << " to be a Beacon frame");
        MgtBeaconHeader beacon;
        mpdu->GetPacket()->PeekHeader(beacon);
        auto& tim = beacon.Get<Tim>();
        NS_TEST_ASSERT_MSG_EQ(tim.has_value(),
                              true,
                              "Expected beacon in PSDU transmitted at "
                                  << now << " to contain a TIM element");
        NS_TEST_EXPECT_MSG_EQ((tim->m_dtimCount == 0),
                              isDtim,
                              "Unexpected DTIM count value for PSDU transmitted at " << now);
        NS_TEST_EXPECT_MSG_EQ(tim->m_hasMulticastPending,
                              hasMulticastPending,
                              "Unexpected multicast pending bit value for PSDU transmitted at "
                                  << now);
        for (const auto& [aid, isSet] : aidIsSetMap)
        {
            NS_TEST_EXPECT_MSG_EQ(tim->HasAid(aid),
                                  isSet,
                                  "Unexpected AID " << aid << " bit value for PSDU transmitted at "
                                                    << now);
        }
    };

    // lambda to check that a non-AP STA affiliated with a given device and operating on the
    // given link is in the given PM mode and in the awake or sleep state
    auto checkSleep = [this](std::size_t staId,
                             uint8_t linkId,
                             WifiPowerManagementMode pmMode,
                             bool sleep) {
        NS_TEST_EXPECT_MSG_EQ(
            m_apMac->GetWifiRemoteStationManager(linkId)->IsInPsMode(
                m_staMacs[staId]->GetAddress()),
            (pmMode == WifiPowerManagementMode::WIFI_PM_POWERSAVE),
            "[PSDU #" << m_processedEvents << "] "
                      << "Unexpected power management mode stored at the AP MLD for the non-AP STA"
                      << (staId == 0 ? " affiliated with the non-AP MLD and operating on link " +
                                           std::to_string(linkId)
                                     : ""));
        NS_TEST_EXPECT_MSG_EQ(
            +m_staMacs[staId]->GetPmMode(linkId),
            +pmMode,
            "[PSDU #" << m_processedEvents << "] "
                      << "Unexpected power management mode for non-AP STA"
                      << (staId == 0 ? " affiliated with the non-AP MLD and operating on link " +
                                           std::to_string(linkId)
                                     : ""));
        NS_TEST_EXPECT_MSG_EQ(
            m_staMacs[staId]->GetWifiPhy(linkId)->IsStateSleep(),
            sleep,
            "[" << Simulator::Now() << "] "
                << "Unexpected sleep state for the PHY of the non-AP STA"
                << (staId == 0 ? " affiliated with the non-AP MLD and operating on link " +
                                     std::to_string(linkId)
                               : ""));
    };

    // lambda to drop all MPDUs in the given PSDU
    auto dropPsdu = [](Ptr<const WifiPsdu> psdu, Ptr<ListErrorModel> model) {
        std::list<uint64_t> uids;
        for (const auto& mpdu : *PeekPointer(psdu))
        {
            uids.push_back(mpdu->GetPacket()->GetUid());
        }
        model->SetList(uids);
    };

    /**
     * (1) The AP MLD sends a Beacon frame on each of its two links. Both Beacon frames contain
     * a DTIM and the multicast pending bit is not set. After receiving these Beacon frames,
     * the non-AP STA and the non-AP MLD start the association procedure.
     */
    m_events.emplace_back(WIFI_MAC_MGT_BEACON,
                          [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
                              checkTim(*psdu->begin(), DTIM, MCAST_NOT_PENDING, {});
                          });

    m_events.emplace_back(WIFI_MAC_MGT_BEACON,
                          [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
                              checkTim(*psdu->begin(), DTIM, MCAST_NOT_PENDING, {});
                          });

    /**
     * (2) Frames (other than Beacon frames) exchanged before non-AP STA and non-AP MLD are
     * considered associated by the AP MLD are ignored. The following frames are the Data Null/Ack
     * exchange for the non-AP STA affiliated with the non-AP MLD to switch to active mode.
     */
    m_events.emplace_back(
        WIFI_MAC_DATA_NULL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            // check that non-AP STA and non-AP MLD are associated and received an AID of 2
            // or 3 (AID 1 is reserved by the AP MLD for groupcast traffic on the other
            // link)
            NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->IsAssociated(),
                                  true,
                                  "Expected non-AP MLD to be associated");
            const auto nonApMldAid = m_staMacs[0]->GetAssociationId();
            NS_TEST_EXPECT_MSG_EQ((nonApMldAid == 2 || nonApMldAid == 3),
                                  true,
                                  "Unexpected AID for non-AP MLD (" << nonApMldAid << ")");
            NS_TEST_EXPECT_MSG_EQ(m_staMacs[1]->IsAssociated(),
                                  true,
                                  "Expected non-AP STA to be associated");
            const auto nonApStaAid = m_staMacs[1]->GetAssociationId();
            NS_TEST_EXPECT_MSG_EQ((nonApStaAid == 2 || nonApStaAid == 3),
                                  true,
                                  "Unexpected AID for non-AP STA (" << nonApStaAid << ")");
            // check that the Power Management flag is set to 0
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  false,
                                  "Expected the Power Management flag not to be set");
        });
    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            // check that all non-AP STAs affiliated with the AP MLD are in active
            // mode after the reception of the Ack in response to the Data Null
            // frame
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            });
        });

    /**
     * (3) After two other Beacon frames (each sent on a distinct link), a broadcast frame is
     * queued at the AP MLD, which immediately transmits it on both links, given that no STA
     * is in power save mode.
     */

    PacketSocketAddress broadcastAddr;
    broadcastAddr.SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
    broadcastAddr.SetPhysicalAddress(Mac48Address::GetBroadcast());
    broadcastAddr.SetProtocol(1);

    const auto delay = MilliSeconds(5); // about a TXOP duration

    m_events.emplace_back(WIFI_MAC_MGT_BEACON,
                          [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
                              checkTim(*psdu->begin(),
                                       NO_DTIM,
                                       MCAST_NOT_PENDING,
                                       /* AID bits */ {{1, false}, {2, false}, {3, false}});
                          });

    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            checkTim(*psdu->begin(),
                     NO_DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {2, false}, {3, false}});
            // queue a broadcast frame
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(broadcastAddr, 1, 1000, delay));
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  Mac48Address::GetBroadcast(),
                                  "Expected a broadcast frame");
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[1],
                                      1,
                                      "Non-AP MLD expected to receive a broadcast frame");
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[2],
                                      (linkId == 0 ? 1 : 0),
                                      "Non-AP STA expected to receive a broadcast frame");
            });
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  Mac48Address::GetBroadcast(),
                                  "Expected a broadcast frame");
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[1],
                                      2,
                                      "Non-AP MLD expected to receive another broadcast frame");
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[2],
                                      1,
                                      "Non-AP STA expected to receive a broadcast frame");
            });
        });

    /**
     * (4) After two other Beacon frames (each sent on a distinct link), the non-AP STA
     * (operating on link 0) and the non-AP STA affiliated with the non-AP MLD and operating
     * on link 1 switch to power save mode by sending a Data Null frame with the PM flag set to 1.
     * After receiving the Ack response, the non-AP STAs put their PHYs to sleep.
     * Also, after transmitting the last Ack response, a broadcast frame is queued at the AP
     * MLD, but it is buffered because there are STAs in power save mode on both links.
     */
    m_events.emplace_back(WIFI_MAC_MGT_BEACON,
                          [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
                              checkTim(*psdu->begin(),
                                       DTIM,
                                       MCAST_NOT_PENDING,
                                       /* AID bits */ {{1, false}, {2, false}, {3, false}});
                          });

    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            checkTim(*psdu->begin(),
                     DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {2, false}, {3, false}});
            // switch non-AP STAs to Power Save mode
            Simulator::Schedule(delay, [=, this]() {
                m_staMacs[0]->SetPowerSaveMode({true, 1});
                m_staMacs[1]->SetPowerSaveMode({true, 0});
            });
        });

    m_events.emplace_back(
        WIFI_MAC_DATA_NULL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            // check that the Power Management flag is set to 1
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  true,
                                  "Expected the Power Management flag to be set");
        });
    m_events.emplace_back(
        WIFI_MAC_DATA_NULL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            // check that the Power Management flag is set to 1
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  true,
                                  "Expected the Power Management flag to be set");
        });

    m_events.emplace_back(WIFI_MAC_CTL_ACK);

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            // check that all non-AP STAs are in the expected power management mode and their PHYs
            // are in the expected sleep state after the reception of the Ack in response to the
            // last Data Null frame
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });

            // queue one broadcast frame at the AP MLD when Ack transmission is completed
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                m_apMac->GetDevice()->GetNode()->AddApplication(
                    GetApplication(broadcastAddr, 1, 1000, delay));
            });
        });

    /**
     * (5) The next two Beacon frames (on the two links) do not contain a DTIM, thus the broadcast
     * frames are still buffered. Check that all non-AP STAs operating on the link on which the
     * Beacon frame is transmitted are awake to listen to the Beacon frame and that the non-AP STAs
     * in power save mode return to sleep after the Beacon frame.
     */
    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            checkTim(*psdu->begin(),
                     NO_DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {2, false}, {3, false}});
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 1);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 0);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });
    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            checkTim(*psdu->begin(),
                     NO_DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {2, false}, {3, false}});
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 1);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 0);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    /**
     * (6) The next Beacon frame includes a DTIM and the AID bits in the virtual bitmap
     * corresponding to both AID 0 (buffered group addressed frames on the same link) and AID 1
     * (buffered group addressed frames on the other link) are both set. Therefore, after receiving
     * the Beacon frame, the non-AP STAs operating on the link on which the Beacon frame was
     * received stay awake to receive the broadcast frames. When the transmission of the Beacon
     * frame starts, a QoS data frame with TID 4 for the non-AP MLD is queued at the AP MLD.
     */
    PacketSocketAddress nonApMldAddr;
    nonApMldAddr.SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
    nonApMldAddr.SetPhysicalAddress(m_staMacs[0]->GetDevice()->GetAddress());
    nonApMldAddr.SetProtocol(1);

    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            checkTim(*psdu->begin(),
                     DTIM,
                     MCAST_PENDING,
                     /* AID bits */ {{1, true}, {2, false}, {3, false}});
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 1);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 0);

            // queue a unicast frame with TID 4 at the AP MLD addressed to the non-AP MLD
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(nonApMldAddr, 1, 1000, Time{0}, 4));

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 1);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 0);
            });
        });

    /**
     * (7) After the Beacon frame including a DTIM, the broadcast frame is sent with the More Data
     * flag set to 0, because at the time the frame is sent no other broadcast frame is queued.
     * Right after starting its transmission, another broadcast frame is queued. The non-AP STA
     * operating on the same link goes to sleep after the reception of the broadcast frame because
     * the More Data flag is set to 0. Given that a broadcast frame with the More Data flag set to
     * 0 is sent, the AP blocks the transmission of group addressed frames and transmits the unicast
     * data frame for the non-AP MLD, which has an affiliated STA in active mode on link 0.
     * The jitter between the initial Beacon frames sent on the two links has been set large enough
     * to have these frames transmitted on the same link as the first Beacon frame before a Beacon
     * frame is transmitted on the other link.
     */
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  Mac48Address::GetBroadcast(),
                                  "Expected a broadcast frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  false,
                                  "Expected the More Data flag not to be set");

            // queue another broadcast frame at the AP MLD, which is not transmitted after this
            // frame because this frame has the More Data flag set to zero and STAs may go to sleep
            // after receiving this broadcast frame
            m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(broadcastAddr, 1, 1000));

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[1],
                                      3,
                                      "Non-AP MLD expected to receive a broadcast frame");
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[2],
                                      2,
                                      "Non-AP STA expected to receive a broadcast frame");
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    // Block Ack agreement establishment for TID 4
    m_events.emplace_back(WIFI_MAC_MGT_ACTION);
    m_events.emplace_back(WIFI_MAC_CTL_ACK);
    m_events.emplace_back(WIFI_MAC_CTL_END);
    m_events.emplace_back(WIFI_MAC_MGT_ACTION);
    m_events.emplace_back(WIFI_MAC_CTL_ACK);
    m_events.emplace_back(WIFI_MAC_CTL_END);

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetHeader(0).GetAddr1(),
                m_staMacs[NON_AP_MLD]->GetFrameExchangeManager(linkId)->GetAddress(),
                "Expected a unicast frame addressed to the STA affiliated with non-AP MLD on link "
                    << +linkId);
        });

    m_events.emplace_back(WIFI_MAC_CTL_ACK);
    m_events.emplace_back(WIFI_MAC_CTL_END);

    /**
     * (8) Next, a Beacon frame is transmitted on the other link. It includes a DTIM and the AID bit
     * in the virtual bitmap corresponding to AID 0 (buffered group addressed frames on the same
     * link) and to AID 1 (buffered group addressed frames on the other link) are both set, because
     * the second broadcast frame has not been transmitted on link 0. Again, after receiving the
     * Beacon frame, the non-AP STAs operating on the link on which the Beacon frame was received
     * stay awake to receive the broadcast frames. Also, two unicast frames addressed to the non-AP
     * STA are queued at the AP MLD, but they are buffered because the non-AP STA is in power save
     * mode.
     */
    PacketSocketAddress nonApStaAddr;
    nonApStaAddr.SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
    nonApStaAddr.SetPhysicalAddress(m_staMacs[1]->GetDevice()->GetAddress());
    nonApStaAddr.SetProtocol(1);

    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            checkTim(*psdu->begin(),
                     DTIM,
                     MCAST_PENDING,
                     /* AID bits */ {{1, true}, {2, false}, {3, false}});
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 1);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 0);
            // queue two unicast frames at the AP MLD addressed to the non-AP STA
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(nonApStaAddr, 2, 1000, Time{0}));

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 1);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 0);
            });
        });

    /**
     * (9) We check that the first broadcast frame has the More Data flag set to 1 and the non-AP
     * STAs operating on the same link stay awake after its reception. The second broadcast frame
     * has the More Data flag set to 0 and the non-AP STAs operating on the same link go to sleep
     * after its reception.
     */

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  Mac48Address::GetBroadcast(),
                                  "Expected a broadcast frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  true,
                                  "Expected the More Data flag to be set");

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[1],
                                      5,
                                      "Non-AP MLD expected to receive a broadcast frame");
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[2],
                                      2,
                                      "Non-AP STA expected to receive a broadcast frame");
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 1);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, linkId != 0);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  Mac48Address::GetBroadcast(),
                                  "Expected a broadcast frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  false,
                                  "Expected the More Data flag not to be set");

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[1],
                                      6,
                                      "Non-AP MLD expected to receive a broadcast frame");
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[2],
                                      2,
                                      "Non-AP STA expected to receive a broadcast frame");
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    /**
     * (10) We assume that the Beacon frame on link 0 is sent first. This Beacon frame has the AID
     * bit in the virtual bitmap corresponding to the AID of the non-AP STA set to 1. The non-AP STA
     * is awake before the transmission of the Beacon frame and stays awake afterwards. The non-AP
     * STA sends a PS Poll frame, receives a QoS data frame from the AP (having the More Data flag
     * set to 1) and transmits an Ack. Then, it stays awake to send another PS Poll frame.
     */
    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_ASSERT_MSG_EQ(
                +linkId,
                0,
                "Beacon on link 0 should be sent first; try to modify the Beacon jitter");

            const auto aidNonApMld = m_staMacs[0]->GetAssociationId();
            const auto aidNonApSta = m_staMacs[1]->GetAssociationId();
            checkTim(*psdu->begin(),
                     NO_DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {aidNonApMld, false}, {aidNonApSta, true}});
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_PSPOLL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[1]->GetAddress(),
                                  "Unexpected transmitter address for PS Poll frame");
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  m_staMacs[1]->GetAddress(),
                                  "Unexpected receiver address for QoS Data frame after PS Poll");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  true,
                                  "Expected the More Data flag to be set");

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[2],
                                      3,
                                      "Non-AP STA expected to receive a unicast frame");
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                // non-AP stays awake to send another PS Poll frame
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            });
        });

    /**
     * (11) The second PS Poll frame is discarded multiple times by the AP MLD for a post reception
     * error. We configure the remote station manager on the non-AP STA to drop the PS Poll after
     * the 5th unsuccessful transmission.
     */

    m_events.emplace_back(
        WIFI_MAC_CTL_PSPOLL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[1]->GetAddress(),
                                  "Unexpected transmitter address for PS Poll frame");
            dropPsdu(psdu, m_apErrorModel);
        });

    m_events.emplace_back(WIFI_MAC_CTL_PSPOLL);
    m_events.emplace_back(WIFI_MAC_CTL_PSPOLL);
    m_events.emplace_back(WIFI_MAC_CTL_PSPOLL);

    m_events.emplace_back(
        WIFI_MAC_CTL_PSPOLL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            auto rsm =
                DynamicCast<PowerSaveTestRateManager>(m_staMacs[1]->GetWifiRemoteStationManager(0));
            NS_TEST_ASSERT_MSG_NE(rsm, nullptr, "Unexpected type of remote station manager");
            rsm->m_drop = true;

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                // reset list of UIDs to drop at the AP MLD
                m_apErrorModel->SetList({});
            });
        });

    /**
     * (12) The next transmitted frame is the Beacon frame on link 1. The non-AP STA turned to the
     * sleep state after giving up sending the PS Poll frame (which was dropped) and is still in
     * the sleep state. Drop the broadcast frame that is still buffered for transmission on link 0,
     * so as to avoid a possible collision after the next Beacon frame including a DTIM sent on link
     * 0 between the AP (trying to send the broadcast frame) and the non-AP STA (trying to send a
     * PS-Poll frame)
     */

    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 1, "Expected a Beacon frame on link 1");

            checkTim(*psdu->begin(),
                     NO_DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {2, false}, {3, false}});
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            // the non-AP STA is in sleep state after giving up sending the PS Poll frame
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);

            // reset the configuration on the remote station manager of the non-AP STA
            auto rsm =
                DynamicCast<PowerSaveTestRateManager>(m_staMacs[1]->GetWifiRemoteStationManager(0));
            NS_TEST_ASSERT_MSG_NE(rsm, nullptr, "Unexpected type of remote station manager");
            rsm->m_drop = false;

            // drop queued broadcast frame
            WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE,
                                         WifiRcvAddr::BROADCAST,
                                         m_apMac->GetFrameExchangeManager(0)->GetAddress(),
                                         0);
            auto mpdu = m_apMac->GetTxopQueue(AC_BE)->PeekByQueueId(queueId);
            NS_TEST_ASSERT_MSG_NE(mpdu, nullptr, "Broadcast data frame not found");
            m_apMac->GetTxopQueue(AC_BE)->Remove(mpdu);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    /**
     * (13) The next Beacon frame is sent again on link 0. This Beacon frame has again the AID bit
     * in the virtual bitmap corresponding to the AID of the non-AP STA set to 1. The non-AP STA is
     * awake before the transmission of the Beacon frame and stays awake afterwards. The non-AP STA
     * sends a PS Poll frame, receives a QoS data frame from the AP (having the More Data flag set
     * to 0) and transmits an Ack. Then, it goes to sleep because no other QoS data frame is
     * expected to be received from the AP.
     */
    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected a Beacon frame on link 0");

            const auto aidNonApMld = m_staMacs[0]->GetAssociationId();
            const auto aidNonApSta = m_staMacs[1]->GetAssociationId();
            checkTim(*psdu->begin(),
                     DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {aidNonApMld, false}, {aidNonApSta, true}});
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_PSPOLL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[1]->GetAddress(),
                                  "Unexpected transmitter address for PS Poll frame");
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  m_staMacs[1]->GetAddress(),
                                  "Unexpected receiver address for QoS Data frame after PS Poll");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  false,
                                  "Expected the More Data flag not to be set");

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[2],
                                      4,
                                      "Non-AP STA expected to receive a unicast frame");
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                // non-AP STAs goes to sleep
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    /**
     * (14) The next Beacon frame is sent on link 1. Non-AP STAs operating on link 1 wake up to
     * listen to the Beacon and go to sleep afterwards. After the transmission of the Beacon frame,
     * two unicast data frames with TID 0 are queued at the AP MLD and addressed to the non-AP MLD.
     * Given that TID 0 is mapped on both links and, specifically, is mapped onto link 0, on which
     * an affiliated non-AP STA is operating in active mode, the unicast frames are not buffered and
     * are transmitted on link 0 (aggregated in an A-MPDU) after the frame exchanges to establish
     * the Block Ack agreement in the DL direction. These MPDUs have both the Power Management flag
     * and the More Data flag not set, because the More Data flag is used by the AP MLD when
     * transmitting frames to a non-AP STA in PS mode.
     */
    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 1, "Expected a Beacon frame on link 1");

            checkTim(*psdu->begin(),
                     DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {2, false}, {3, false}});
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, false);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, true);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, true);
                // queue two unicast frames with TID 0 at the AP MLD addressed to the non-AP MLD
                m_apMac->GetDevice()->GetNode()->AddApplication(
                    GetApplication(nonApMldAddr, 2, 1000, Time{0}));
            });
        });

    m_events.emplace_back(
        WIFI_MAC_MGT_ACTION,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected an Action frame on link 0");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected receiver address for first Action frame");

            {
                WifiActionHeader actionHdr;
                psdu->GetPayload(0)->PeekHeader(actionHdr);
                NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetCategory(),
                                      +WifiActionHeader::BLOCK_ACK,
                                      "Expected an Action frame of BLOCK_ACK category");
                NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetAction().blockAck,
                                      +WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST,
                                      "Expected an ADDBA Request frame");
            }

            // non-AP STAs in power save mode are sleeping
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
        });

    m_events.emplace_back(WIFI_MAC_CTL_ACK);

    m_events.emplace_back(
        WIFI_MAC_MGT_ACTION,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected an Action frame on link 0");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected transmitter address for second Action frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  false,
                                  "Expected the More Data flag not to be set");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  false,
                                  "Expected the Power Management flag not to be set");
            {
                WifiActionHeader actionHdr;
                psdu->GetPayload(0)->PeekHeader(actionHdr);
                NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetCategory(),
                                      +WifiActionHeader::BLOCK_ACK,
                                      "Expected an Action frame of BLOCK_ACK category");
                NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetAction().blockAck,
                                      +WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE,
                                      "Expected an ADDBA Response frame");
            }

            // non-AP STAs in power save mode are sleeping
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
        });

    m_events.emplace_back(WIFI_MAC_CTL_ACK);

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected a QoS Data frame on link 0");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr1(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected receiver address for unicast QoS Data frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_apMac->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected transmitter address for unicast QoS Data frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(), 2, "Expected an A-MPDU to be sent");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  false,
                                  "Expected the More Data flag of the first MPDU not to be set");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(1).IsMoreData(),
                                  false,
                                  "Expected the More Data flag of the second MPDU not to be set");
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetHeader(0).IsPowerManagement(),
                false,
                "Expected the Power Management flag of the first MPDU not to be set");
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetHeader(1).IsPowerManagement(),
                false,
                "Expected the Power Management flag of the second MPDU not to be set");
            NS_TEST_EXPECT_MSG_EQ(+psdu->GetHeader(0).GetQosTid(),
                                  0,
                                  "Unexpected TID value for the first MPDU");
            NS_TEST_EXPECT_MSG_EQ(+psdu->GetHeader(1).GetQosTid(),
                                  0,
                                  "Unexpected TID value for the second MPDU");

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[1],
                                      8,
                                      "Non-AP MLD expected to receive two more (unicast) frames");
            });
        });

    /**
     * (15) The non-AP STA affiliated with the non-AP MLD and operating on link 0 transmits a
     * BlockAck response. Afterwards, two unicast Data frames with TID 2 are queued at the AP MLD
     * and addressed to the non-AP MLD. These frames are buffered because TID 2 is only mapped to
     * link 1, on which an affiliated non-AP STA in PS mode is operating.
     */
    m_events.emplace_back(
        WIFI_MAC_CTL_BACKRESP,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected a BlockAck response on link 0");

            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                // queue two unicast frames with TID 2 at the AP MLD addressed to the non-AP MLD
                m_apMac->GetDevice()->GetNode()->AddApplication(
                    GetApplication(nonApMldAddr, 2, 1000, Time{0}, 2));
            });
        });

    /**
     * (16) The next frame is a Beacon sent on link 0. The AID bit in the virtual bitmap
     * corresponding to the AID of the non-AP MLD is set to 1 because there are buffered units. The
     * non-AP STA affiliated with the non-AP MLD and operating on link 0 is in active mode, hence it
     * is not supposed to send a PS Poll frame.
     */
    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected a Beacon frame on link 0");

            const auto aidNonApMld = m_staMacs[0]->GetAssociationId();
            const auto aidNonApSta = m_staMacs[1]->GetAssociationId();
            checkTim(*psdu->begin(),
                     NO_DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {aidNonApMld, true}, {aidNonApSta, false}});

            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    /**
     * (17) The next frame is thus a Beacon sent on link 1. The AID bit in the virtual bitmap
     * corresponding to the AID of the non-AP MLD is still set to 1 because there are buffered
     * units. The non-AP STA affiliated with the non-AP MLD and operating on link 1 in PS mode
     * stays awake to send a PS Poll frame. The AP MLD sends the first buffered unit, which is
     * corrupted at the receiver. After the timeout, the non-AP STA affiliated with the non-AP MLD
     * and operating on link 1 sends again a PS Poll, receives a buffered unit and sends the Ack
     * response (repeated twice because the two buffered units are sent one at a time).
     * After the last Ack response, two QoS Data frames are queued at the non-AP STA and addressed
     * to the AP MLD.
     */
    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 1, "Expected a Beacon frame on link 1");

            const auto aidNonApMld = m_staMacs[0]->GetAssociationId();
            const auto aidNonApSta = m_staMacs[1]->GetAssociationId();
            checkTim(*psdu->begin(),
                     NO_DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {aidNonApMld, true}, {aidNonApSta, false}});

            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_PSPOLL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(1)->GetAddress(),
                                  "Unexpected transmitter address for PS Poll frame");
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  m_staMacs[0]->GetFrameExchangeManager(1)->GetAddress(),
                                  "Unexpected receiver address for QoS Data frame after PS Poll");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  true,
                                  "Expected the More Data flag to be set");

            // corrupt the reception of the frame at the non-AP STA
            dropPsdu(psdu, m_nonApMldErrorModel);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                // reset error list, so that the frame is corrupted only once
                m_nonApMldErrorModel->SetList({});
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_PSPOLL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(1)->GetAddress(),
                                  "Unexpected transmitter address for PS Poll frame");
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  m_staMacs[0]->GetFrameExchangeManager(1)->GetAddress(),
                                  "Unexpected receiver address for QoS Data frame after PS Poll");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  true,
                                  "Expected the More Data flag to be set");

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[1],
                                      9,
                                      "Non-AP MLD expected to receive another unicast frame");
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                // non-AP STA on link 1 stays awake
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_PSPOLL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(1)->GetAddress(),
                                  "Unexpected transmitter address for PS Poll frame");
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  m_staMacs[0]->GetFrameExchangeManager(1)->GetAddress(),
                                  "Unexpected receiver address for QoS Data frame after PS Poll");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  false,
                                  "Expected the More Data flag not to be set");

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[1],
                                      10,
                                      "Non-AP MLD expected to receive another unicast frame");
            });
        });

    PacketSocketAddress apMldAddr;
    apMldAddr.SetSingleDevice(m_staMacs[1]->GetDevice()->GetIfIndex());
    apMldAddr.SetPhysicalAddress(m_apMac->GetDevice()->GetAddress());
    apMldAddr.SetProtocol(1);

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                // non-AP STA on link 1 goes to sleep
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                // queue two unicast frames at the non-AP STA addressed to the AP MLD
                m_staMacs[1]->GetDevice()->GetNode()->AddApplication(
                    GetApplication(apMldAddr, 2, 1000, Time{0}));
            });
        });

    /**
     * (18) Two QoS data frames have been queued at the non-AP STA, which is in sleep state. The
     * non-AP STA does not wait until the next Beacon, but it is immediately awaken to start a TXOP.
     * Actually, the queued frames trigger the establishment of a Block Ack agreement in the uplink
     * direction; the non-AP STA sends an ADDBA Request, which is acknowledged by the AP. Then, the
     * AP buffers the ADDBA Response, because the non-AP STA is in power save mode; the non-AP STA
     * has blocked transmission of QoS data frames (while waiting for ADDBA Response) and thus
     * releases the channel, generates backoff and restarts channel access. When the non-AP STA is
     * granted channel access again, it still has nothing to transmit and this time it does not
     * restart channel access and goes to sleep. When the ADDBA Response timer expires, the
     * transmission of the QoS data frames is unblocked, the non-AP STA wakes up and transmits the
     * two QoS data frames using normal acknowledgment; afterwards, the non-AP STA goes back to
     * sleep.
     */
    if (m_standard >= WIFI_STANDARD_80211n)
    {
        m_events.emplace_back(
            WIFI_MAC_MGT_ACTION,
            [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
                NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected an Action frame on link 0");

                NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                      m_staMacs[1]->GetFrameExchangeManager(0)->GetAddress(),
                                      "Unexpected transmitter address for Action frame");
                NS_TEST_EXPECT_MSG_EQ(
                    psdu->GetHeader(0).IsPowerManagement(),
                    true,
                    "Expected the Power Management flag of the ADDBA Request to be set");

                WifiActionHeader actionHdr;
                psdu->GetPayload(0)->PeekHeader(actionHdr);
                NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetCategory(),
                                      +WifiActionHeader::BLOCK_ACK,
                                      "Expected an Action frame of BLOCK_ACK category");
                NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetAction().blockAck,
                                      +WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST,
                                      "Expected an ADDBA Request frame");

                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                // non-AP STA woke up to transmit this frame
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            });

        m_events.emplace_back(
            WIFI_MAC_CTL_ACK,
            [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
                Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                    // check sleep state after the backoff counter has reached zero (the non-AP STA
                    // releases the channel and does not restart channel access)
                    auto acBe = m_staMacs[1]->GetQosTxop(AC_BE);
                    auto backoffEnd =
                        m_staMacs[1]->GetChannelAccessManager()->GetBackoffEndFor(acBe);
                    Simulator::Schedule(backoffEnd - Simulator::Now() + TimeStep(1), [=] {
                        checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                        checkSleep(NON_AP_MLD,
                                   1,
                                   WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                   SLEEP);
                        // non-AP STA goes to sleep because no QoS data frames can be sent
                        checkSleep(NON_AP_STA,
                                   0,
                                   WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                   SLEEP);
                    });
                });
            });
    }

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[1]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected transmitted address for QoS Data frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  true,
                                  "Expected the Power Management flag to be set");

            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            // non-AP STA woke up to transmit this frame
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[0],
                                      1,
                                      "AP MLD expected to receive the first unicast frame");
            });
        });

    m_events.emplace_back(WIFI_MAC_CTL_ACK);

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[1]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected transmitted address for QoS Data frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  true,
                                  "Expected the Power Management flag to be set");

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[0],
                                      2,
                                      "AP MLD expected to receive the second unicast frame");
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                // non-AP STA goes to sleep because no queued frames
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    /**
     * (19) The next Beacon frame sent on link 0 indicates that the AP has a buffered unit (the
     * ADDBA Response) for the non-AP STA, which sends a PS-Poll to request the AP to transmit it.
     */
    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            const auto aidNonApMld = m_staMacs[0]->GetAssociationId();
            const auto aidNonApSta = m_staMacs[1]->GetAssociationId();
            const auto is11n = (m_standard >= WIFI_STANDARD_80211n);

            checkTim(*psdu->begin(),
                     DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {aidNonApMld, false}, {aidNonApSta, is11n}});

            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                // non-AP STA stays awake to transmit the PS-Poll
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, !is11n);

                if (m_standard < WIFI_STANDARD_80211n)
                {
                    // queue two unicast frames with TID 0 at the non-AP MLD addressed to the AP MLD
                    m_staMacs[0]->GetDevice()->GetNode()->AddApplication(
                        GetApplication(apMldAddr, 2, 1000, Time{0}));
                }
            });
        });

    if (m_standard >= WIFI_STANDARD_80211n)
    {
        m_events.emplace_back(
            WIFI_MAC_CTL_PSPOLL,
            [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
                NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                      m_staMacs[1]->GetFrameExchangeManager(0)->GetAddress(),
                                      "Unexpected transmitter address for PS Poll frame");
            });

        m_events.emplace_back(
            WIFI_MAC_MGT_ACTION,
            [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
                NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                      m_staMacs[1]->GetFrameExchangeManager(0)->GetAddress(),
                                      "Unexpected receiver address for Action frame");

                WifiActionHeader actionHdr;
                psdu->GetPayload(0)->PeekHeader(actionHdr);
                NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetCategory(),
                                      +WifiActionHeader::BLOCK_ACK,
                                      "Expected an Action frame of BLOCK_ACK category");
                NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetAction().blockAck,
                                      +WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE,
                                      "Expected an ADDBA Response frame");

                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                // non-AP STA stays awake to receive this frame
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            });

        apMldAddr.SetSingleDevice(m_staMacs[0]->GetDevice()->GetIfIndex());

        m_events.emplace_back(
            WIFI_MAC_CTL_ACK,
            [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
                Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                    checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                    checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                    // non-AP STA goes to sleep because no queued frames
                    checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                    // queue two unicast frames with TID 0 at the non-AP MLD addressed to the AP MLD
                    m_staMacs[0]->GetDevice()->GetNode()->AddApplication(
                        GetApplication(apMldAddr, 2, 1000, Time{0}));
                });
            });
    }

    /**
     * (20) Frames with TID 0 can be transmitted on both links, hence the non-AP MLD requests
     * channel access on both links. This implies that the non-AP STA affiliated with the non-AP MLD
     * and operating on link 1 awakens and generates a new backoff value. The backoff counter of the
     * non-AP STA affiliated with the non-AP MLD and operating on link 0 has likely reached zero
     * already, hence the management frames to establish the BA agreement are sent on link 0.
     * During such frame exchange, the backoff counter on link 1 reaches zero, channel access is
     * granted but no frame is available for transmission (queues are blocked while BA is being
     * established), hence the non-AP STA affiliated with the non-AP MLD and operating on link 1
     * is put to sleep.
     * When the non-AP MLD completes the transmission of the Ack in response to the ADDBA Response
     * frame, transmissions are unblocked and channel access is requested on both links. Hence, the
     * non-AP STA affiliated with the non-AP MLD and operating on link 1 awakens and generates a
     * new backoff value.
     */
    m_events.emplace_back(
        WIFI_MAC_MGT_ACTION,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(
                +linkId,
                0,
                "Expected an Action frame on link 0, try changing RNG seed and run values");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected transmitter address for Action frame");
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetHeader(0).IsPowerManagement(),
                false,
                "Expected the Power Management flag of the ADDBA Request not to be set");

            WifiActionHeader actionHdr;
            psdu->GetPayload(0)->PeekHeader(actionHdr);
            NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetCategory(),
                                  +WifiActionHeader::BLOCK_ACK,
                                  "Expected an Action frame of BLOCK_ACK category");
            NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetAction().blockAck,
                                  +WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST,
                                  "Expected an ADDBA Request frame");

            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            // non-AP STA on link 1 awakened upon channel access request
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
        });

    m_events.emplace_back(WIFI_MAC_CTL_ACK);

    m_events.emplace_back(
        WIFI_MAC_MGT_ACTION,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected an Action frame on link 0");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr1(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected receiver address for Action frame");

            WifiActionHeader actionHdr;
            psdu->GetPayload(0)->PeekHeader(actionHdr);
            NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetCategory(),
                                  +WifiActionHeader::BLOCK_ACK,
                                  "Expected an Action frame of BLOCK_ACK category");
            NS_TEST_EXPECT_MSG_EQ(+actionHdr.GetAction().blockAck,
                                  +WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE,
                                  "Expected an ADDBA Response frame");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            // we assume that, when the Ack transmission starts, the backoff on link 1 has
            // reached zero and the non-AP STA operating on link 1 has been put to sleep
            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);

            // when the Ack transmission ends, transmissions of QoS Data frames are unblocked,
            // channel access is requested and the non-AP STA operating on link 1 awakens
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    /**
     * (21) UL QoS Data frames can now be sent in an A-MPDU on either link 0 or 1 (depends on the
     * generated backoff values)
     */
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr1(),
                                  m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected transmitter address for unicast QoS Data frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected receiver address for unicast QoS Data frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(), 2, "Expected an A-MPDU to be sent");
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetHeader(0).IsPowerManagement(),
                (linkId == 1),
                "Unexpected value for the Power Management flag of the first MPDU");
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetHeader(1).IsPowerManagement(),
                (linkId == 1),
                "Unexpected value for the Power Management flag of the second MPDU");
            NS_TEST_EXPECT_MSG_EQ(+psdu->GetHeader(0).GetQosTid(),
                                  0,
                                  "Unexpected TID value for the first MPDU");
            NS_TEST_EXPECT_MSG_EQ(+psdu->GetHeader(1).GetQosTid(),
                                  0,
                                  "Unexpected TID value for the second MPDU");

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[0],
                                      4,
                                      "AP MLD expected to receive two more (unicast) frames");
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_BACKRESP,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_ACTIVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                // queue a unicast frame with TID 0 at the AP MLD addressed to the non-AP MLD
                m_apMac->GetDevice()->GetNode()->AddApplication(
                    GetApplication(nonApMldAddr, 1, 1000, Time{0}));
            });
        });

    /**
     * (22) The AP MLD transmits the QoS data with TID 0 for the non-AP MLD on link 0, but we force
     * a receive error at the non-AP MLD. The frame retry limit on the AP is set to 1, so that the
     * QoS data frame is dropped when the Ack timeout occurs; consequently, a BlockAckReq is
     * enqueued. The non-AP MLD is then configured to switch the affiliated non-AP STA operating
     * on link 0, too, to powersave mode. The affiliated non-AP STA operating on link 0 is put to
     * sleep after receiving the Ack in response to the sent Data Null frame.
     */
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected receiver address for QoS Data frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  false,
                                  "Expected the More Data flag not to be set");

            // corrupt the reception of the PSDU at the non-AP MLD
            dropPsdu(psdu, m_nonApMldErrorModel);

            // set the frame retry limit on the AP to 1, so that the QoS data frame is dropped
            // when the Ack timeout occurs
            m_apMac->SetAttribute("FrameRetryLimit", UintegerValue(1));
            auto rsm =
                DynamicCast<PowerSaveTestRateManager>(m_apMac->GetWifiRemoteStationManager(linkId));
            NS_TEST_ASSERT_MSG_NE(rsm, nullptr, "Unexpected type of remote station manager");
            rsm->m_drop = true;

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                // reset error list
                m_nonApMldErrorModel->SetList({});
                // switch affiliated non-AP STA on link 0 to powersave mode
                m_staMacs[0]->SetPowerSaveMode({true, 0});
            });
        });

    m_events.emplace_back(
        WIFI_MAC_DATA_NULL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected transmitter address for Data Null frame");
            // check that the Power Management flag is set to 1
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  true,
                                  "Expected the Power Management flag to be set");
            // when AckTimeout occurred, the QoS data frame was dropped and a BlockAckReq
            // was enqueued at the AP MLD
            NS_TEST_EXPECT_MSG_EQ(
                m_apMac->GetTxopQueue(AC_BE)->GetNPackets(),
                1,
                "Expected only one frame (BlockAckReq) to be queued at the AP MLD");
            auto mpdu = m_apMac->GetTxopQueue(AC_BE)->Peek();
            NS_TEST_ASSERT_MSG_NE(mpdu, nullptr, "Expected a frame to be queued at the AP MLD");
            NS_TEST_EXPECT_MSG_EQ(std::string(mpdu->GetHeader().GetTypeString()),
                                  "CTL_BACKREQ",
                                  "Expected a BlockAckReq to be queued at the non-AP MLD");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            // check that all non-AP STAs affiliated with the AP MLD are in powersave mode and
            // in the sleep state after the reception of the Ack
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    /**
     * (23) When the Beacon frame is sent on link 1, another QoS data with TID 0 for the non-AP MLD
     * is queued at the AP MLD, which is now buffered because both STAs affiliated with the non-AP
     * MLD are in powersave mode. The next Beacon frame, on link 0, indicates that frames are
     * buffered for the non-AP MLD, which sends a two PS-Poll frames: the AP sends the BlockAckReq
     * (ack'ed by a BlockAck frame) in response to the first PS-Poll and the QoS Data frame (ack'ed
     * by a Normal Ack) in response to the second PS-Poll frame.
     */

    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 1, "Expected a Beacon frame on link 1");

            checkTim(*psdu->begin(),
                     DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {2, false}, {3, false}});

            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                // affiliated non-AP STA on link 0 is put to sleep again
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);

                // queue another unicast frames with TID 0 at the AP MLD for the non-AP MLD
                m_apMac->GetDevice()->GetNode()->AddApplication(
                    GetApplication(nonApMldAddr, 1, 1000, Time{0}));
            });
        });

    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected a Beacon frame on link 0");
            const auto aidNonApMld = m_staMacs[0]->GetAssociationId();
            const auto aidNonApSta = m_staMacs[1]->GetAssociationId();

            checkTim(*psdu->begin(),
                     NO_DTIM,
                     MCAST_NOT_PENDING,
                     /* AID bits */ {{1, false}, {aidNonApMld, true}, {aidNonApSta, false}});

            checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
            checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);

            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, AWAKE);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_PSPOLL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected a PS-Poll frame on link 0");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected transmitter address for PS Poll frame");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_BACKREQ,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected a BlockAckReq frame on link 0");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  true,
                                  "Expected the More Data flag to be set");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_BACKRESP,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected a BlockAck frame on link 0");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected transmitter address for PS Poll frame");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_PSPOLL,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Expected a PS-Poll frame on link 0");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected transmitter address for PS Poll frame");
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr1(),
                                  m_staMacs[0]->GetFrameExchangeManager(0)->GetAddress(),
                                  "Unexpected receiver address for QoS Data frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsMoreData(),
                                  false,
                                  "Expected the More Data flag not to be set");
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_rxPkts[1],
                                      11,
                                      "Non-AP MLD expected to receive another unicast frame");
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=](Ptr<const WifiPsdu> psdu, const Time& txDuration, uint8_t linkId) {
            // all non-AP STAs are in the sleep state after Normal Ack
            Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=]() {
                checkSleep(NON_AP_MLD, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_MLD, 1, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
                checkSleep(NON_AP_STA, 0, WifiPowerManagementMode::WIFI_PM_POWERSAVE, SLEEP);
            });
        });
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test transmission of broadcast and unicast frames with STAs in Power Save mode
 *
 * This test considers an AP MLD having two links and two devices: a device under test (DUT), which
 * is a 2-link non-AP MLD, and a single-link non-AP STA. The DUT can have EMLSR mode enabled or
 * disabled on both links and static setup helper can be used or not. Both STAs affiliated with the
 * DUT are switched to powersave mode after completing ML setup or enabling EMLSR mode.
 *
 * It is verified that:
 *
 * - upon completion of association (EML OMN) procedure, both STAs affiliated with the DUT are in PS
 *   mode and their PHYs are in sleep state (except, in case static setup helper is not used, the
 *   PHY of the STA operating on the link that is not used for association, which is put to sleep
 *   after receiving the first Beacon frame)
 * - while the AP MLD sends a PPDU to the other non-AP STA, a packet is enqueued at the DUT; the
 *   PHYs of both STAs affiliated with the DUT are awaken from sleep and the state of the PHY
 *   operating on the same link as the other non-AP STA is CCA_BUSY
 * - when a non-null PSM timeout is configured, the PHY is kept in awake state for such interval
 *   after releasing (and not requesting again) the channel and after receiving a Beacon frame that
 *   does not indicate that the AP has pending frames for the DUT
 * - if channel access is requested (to transmit a frame) during a PSM timeout, the latter is
 *   cancelled
 * - when a non-null Listen Advance is configured, the STA wakes up such interval prior to the TBTT
 *
   \verbatim

                        1 frame queued
                            at DUT
                              │                   channel access
                        ┌───────────┐             granted, no TX,    1 frame queued
            ┌──────┐    │QoS Data to│           no channel request       at DUT
   [link 0] │Beacon│    │ other STA │                   │                  │
   ─────────┴──────┴────┴─────▼─────┴┬───┬─────────────────────────────────▼───────────────────────
                              │CCA   │ACK│              │                  │
   non-AP STA      |---sleep--│BUSY  └───┘              │------sleep-------│             │---------
   on link 0                  │                                            │
   non-AP STA -----sleep------│                      │----│                │                     │-
   on link 1                  │                                            │
                              │                  PSM       Listen          PSM               PSM
                              │           ┌───┐timeout    Advance┌──────┐timeout      ┌───┐timeout
   [link 1]                   │           │ACK│------│    │------│Beacon│---X         │ACK│------│
   ───────────────────────────▼─┬────────┬┴───┴──────────────────┴──────┴──▼┬────────┬┴───┴────────
                                │QoS Data│                                  │QoS Data│
                                │from DUT│                                  │from DUT│
                                └────────┘                                  └────────┘

   \endverbatim
 */
class WifiPsModeAttributesTest : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param enableEmlsr whether to enable EMLSR on the DUT
     * @param staticSetup whether static setup helper is used to configure association
     */
    WifiPsModeAttributesTest(bool enableEmlsr, bool staticSetup);

    /**
     * Callback invoked when PHY receives a PSDU to transmit
     *
     * @param mac the MAC transmitting the PSDUs
     * @param phyId the ID of the PHY transmitting the PSDUs
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPower the tx power
     */
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  Watt_u txPower);

  protected:
    /**
     * @param dir the traffic direction (downlink/uplink)
     * @param staId the index of the non-AP MLD generating/receiving packets
     * @param count the number of packets to generate
     * @param pktSize the size of the packets to generate
     * @param priority user priority for generated packets
     * @return an application generating the given number packets of the given size from/to the
     *         AP MLD to/from the given non-AP MLD
     */
    Ptr<PacketSocketClient> GetApplication(WifiDirection dir,
                                           std::size_t staId,
                                           std::size_t count,
                                           std::size_t pktSize,
                                           uint8_t priority = 0) const;

    /// Actions and checks to perform upon the transmission of each frame
    struct Events
    {
        /**
         * Constructor.
         *
         * @param type the frame MAC header type
         * @param f function to perform actions and checks
         */
        Events(WifiMacType type,
               std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, linkId_t)>&& f = {})
            : hdrType(type),
              func(f)
        {
        }

        WifiMacType hdrType; ///< MAC header type of frame being transmitted
        std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, linkId_t)>
            func; ///< function to perform actions and checks
    };

    /// Insert elements in the list of expected events (transmitted frames)
    void InsertEvents();

    void DoSetup() override;
    void DoRun() override;

  private:
    /// number of links for ML devices
    static constexpr std::size_t N_LINKS = 2;
    /// frequency channels for the two links
    const std::array<std::string, N_LINKS> m_channels{"{42, 80, BAND_5GHZ, 0}",
                                                      "{23, 80, BAND_6GHZ, 0}"};
    /// channel used to initialize the PHY of the DUT operating on the link that shall not be
    /// used for ML setup
    const std::string m_otherChannel{"{100, 20, BAND_5GHZ, 0}"};
    /// jitter for the initial Beacon frame sent on the link used by the DUT for association
    static constexpr double DUT_BEACON_JITTER = 0.6;
    /// jitter for the initial Beacon frame sent on the link used by the other STA for association
    static constexpr double STA_BEACON_JITTER = 0.2;
    /// the time a STA listens for a Beacon is the percentage of the Beacon interval given by the
    /// sum of the beacon jitter and this surplus
    static constexpr double WAIT_BEACON_SURPLUS = 0.1;

    const Time m_psmTimeout{MicroSeconds(100)};   ///< the PSM timeout
    const Time m_listenAdvance{MicroSeconds(50)}; ///< the value for the ListenAdvance attribute
    const Time m_switchDelay{MicroSeconds(64)};   ///< channel switch, transition and padding delay
    bool m_enableEmlsr;                           ///< whether to enable EMLSR on the DUT
    bool m_staticSetup; ///< whether static setup helper is used to configure association
    static constexpr std::size_t DUT_INDEX = 0;   ///< DUT index
    static constexpr std::size_t STA_INDEX = 1;   ///< index of the other non-AP STA
    Ptr<ApWifiMac> m_apMac;                       ///< AP wifi MAC
    std::array<Ptr<StaWifiMac>, 2> m_staMacs;     ///< wifi MACs of the STAs
    std::vector<PacketSocketAddress> m_dlSockets; ///< packet socket addresses for DL traffic
    std::vector<PacketSocketAddress> m_ulSockets; ///< packet socket addresses for UL traffic
    std::list<Events> m_events;                   ///< list of events for a test run
    bool m_checkTxPsdus{false};                   ///< whether to check transmitted PSDUs against
                                                  ///< the inserted events
    Time m_duration{Seconds(0.5)};                ///< simulation duration
};

WifiPsModeAttributesTest::WifiPsModeAttributesTest(bool enableEmlsr, bool staticSetup)
    : TestCase("Test that medium is sensed busy after waking up from sleep mode (enableEmlsr=" +
               std::to_string(+enableEmlsr) + ", staticSetup=" + std::to_string(+staticSetup) +
               ")"),
      m_enableEmlsr(enableEmlsr),
      m_staticSetup(staticSetup)
{
}

Ptr<PacketSocketClient>
WifiPsModeAttributesTest::GetApplication(WifiDirection dir,
                                         std::size_t staId,
                                         std::size_t count,
                                         std::size_t pktSize,
                                         uint8_t priority) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetAttribute("Priority", UintegerValue(priority));
    client->SetRemote(dir == WifiDirection::DOWNLINK ? m_dlSockets.at(staId)
                                                     : m_ulSockets.at(staId));
    client->SetStartTime(Time{0});
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
WifiPsModeAttributesTest::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 100;
    SsidValue ssidValue(Ssid("power-save-ssid"));

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNodes(2);

    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(m_switchDelay));
    Config::SetDefault("ns3::EmlsrManager::EmlsrPaddingDelay", TimeValue(m_switchDelay));
    Config::SetDefault("ns3::EmlsrManager::EmlsrTransitionDelay", TimeValue(m_switchDelay));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager");
    wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(m_enableEmlsr));

    // create a spectrum channel for each link
    std::array<Ptr<MultiModelSpectrumChannel>, N_LINKS> spectrumChannels;
    for (std::size_t id = 0; id < spectrumChannels.size(); ++id)
    {
        spectrumChannels[id] = CreateObject<MultiModelSpectrumChannel>();
    }

    // AP MLD
    SpectrumWifiPhyHelper phy(m_channels.size());
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    for (std::size_t id = 0; id < m_channels.size(); ++id)
    {
        phy.Set(id, "ChannelSettings", StringValue(m_channels[id]));
        const auto phyBand = WifiChannelConfig::FromString(m_channels[id]).front().band;
        const auto freqRange = GetFrequencyRange(phyBand);
        phy.AddChannel(spectrumChannels[id], freqRange);
        // do not create multiple spectrum interfaces for STR devices
        phy.AddPhyToFreqRangeMapping(id, freqRange);
    }

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac",
                "BeaconInterval",
                TimeValue(DEFAULT_BEACON_INTERVAL()),
                "Ssid",
                ssidValue);

    auto apDev = wifi.Install(phy, mac, wifiApNode);

    // other non-AP STA
    wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(false));

    SpectrumWifiPhyHelper staPhy;
    staPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    staPhy.Set("ChannelSettings", StringValue(m_channels[0]));
    staPhy.SetChannel(spectrumChannels[0]);

    mac.SetType("ns3::StaWifiMac",
                "Ssid",
                ssidValue,
                "WaitBeaconTimeout",
                TimeValue((STA_BEACON_JITTER + WAIT_BEACON_SURPLUS) * DEFAULT_BEACON_INTERVAL()));

    auto staDevs = wifi.Install(staPhy, mac, wifiStaNodes.Get(STA_INDEX));

    // DUT
    wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(m_enableEmlsr));

    if (!m_staticSetup)
    {
        // channel on link 0 does not match the AP's channel, hence ML setup is guaranteed to occur
        // on link 1, thus preventing potential collisions with the other non-AP STA, which only
        // operates on link 0
        phy.Set(0, "ChannelSettings", StringValue(m_otherChannel));
    }

    if (m_enableEmlsr)
    {
        phy.ResetPhyToFreqRangeMapping();
        mac.SetType(
            "ns3::StaWifiMac",
            "Ssid",
            ssidValue,
            "WaitBeaconTimeout",
            TimeValue((DUT_BEACON_JITTER + WAIT_BEACON_SURPLUS) * DEFAULT_BEACON_INTERVAL()));
        mac.SetEmlsrManager("ns3::AdvancedEmlsrManager",
                            "EmlsrLinkSet",
                            StringValue("0,1"),
                            "MainPhyId",
                            UintegerValue(1));
    }

    // set power save mode on both STAs of the DUT
    mac.SetPowerSaveManager("ns3::DefaultPowerSaveManager",
                            "PowerSaveMode",
                            StringValue("0 true, 1 true"));

    staDevs.Add(wifi.Install(phy, mac, wifiStaNodes.Get(DUT_INDEX)));

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(NetDeviceContainer(staDevs, apDev), streamNumber);

    auto positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, 1.0, 0.0));
    MobilityHelper mobility;
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDev.Get(0))->GetMac());
    m_staMacs[DUT_INDEX] = DynamicCast<StaWifiMac>(
        DynamicCast<WifiNetDevice>(wifiStaNodes.Get(DUT_INDEX)->GetDevice(0))->GetMac());
    m_staMacs[STA_INDEX] = DynamicCast<StaWifiMac>(
        DynamicCast<WifiNetDevice>(wifiStaNodes.Get(STA_INDEX)->GetDevice(0))->GetMac());

    // define the jitter for the initial Beacon frames on the two links, so that Beacon frames are
    // sent on the two links at different moments and the other non-AP STA starts association before
    // the DUT
    auto beaconJitter = CreateObject<DeterministicRandomVariable>();
    beaconJitter->SetValueArray({STA_BEACON_JITTER, DUT_BEACON_JITTER});
    m_apMac->SetAttribute("BeaconJitter", PointerValue(beaconJitter));

    if (m_staticSetup)
    {
        /* static setup of association and EMLSR mode (if needed) */
        WifiStaticSetupHelper::SetStaticAssociation(m_apMac->GetDevice(), staDevs);
        if (m_enableEmlsr)
        {
            WifiStaticSetupHelper::SetStaticEmlsr(m_apMac->GetDevice(), staDevs);
        }
    }

    // Trace PSDUs passed to the PHY on all devices
    for (const auto& mac : std::list<Ptr<WifiMac>>{m_apMac, m_staMacs[0], m_staMacs[1]})
    {
        for (uint8_t phyId = 0; phyId < mac->GetDevice()->GetNPhys(); ++phyId)
        {
            mac->GetDevice()->GetPhy(phyId)->TraceConnectWithoutContext(
                "PhyTxPsduBegin",
                MakeCallback(&WifiPsModeAttributesTest::Transmit, this).Bind(mac, phyId));
        }
    }

    // install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);

    // install a packet socket server on all nodes
    for (auto nodeIt = NodeList::Begin(); nodeIt != NodeList::End(); ++nodeIt)
    {
        PacketSocketAddress srvAddr;
        auto device = DynamicCast<WifiNetDevice>((*nodeIt)->GetDevice(0));
        NS_TEST_ASSERT_MSG_NE(device, nullptr, "Expected a WifiNetDevice");
        srvAddr.SetSingleDevice(device->GetIfIndex());
        srvAddr.SetProtocol(1);

        auto server = CreateObject<PacketSocketServer>();
        server->SetLocal(srvAddr);
        (*nodeIt)->AddApplication(server);
        server->SetStartTime(Time{0}); // now
        server->SetStopTime(m_duration);
    }

    // set DL and UL packet sockets
    for (const auto& staMac : m_staMacs)
    {
        m_dlSockets.emplace_back();
        m_dlSockets.back().SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
        m_dlSockets.back().SetPhysicalAddress(staMac->GetDevice()->GetAddress());
        m_dlSockets.back().SetProtocol(1);

        m_ulSockets.emplace_back();
        m_ulSockets.back().SetSingleDevice(staMac->GetDevice()->GetIfIndex());
        m_ulSockets.back().SetPhysicalAddress(m_apMac->GetDevice()->GetAddress());
        m_ulSockets.back().SetProtocol(1);
    }
}

void
WifiPsModeAttributesTest::Transmit(Ptr<WifiMac> mac,
                                   uint8_t phyId,
                                   WifiConstPsduMap psduMap,
                                   WifiTxVector txVector,
                                   Watt_u txPower)
{
    const auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link found for PHY ID " << +phyId);

    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        ss << std::setprecision(10) << " Link ID " << +linkId.value() << " Phy ID " << +phyId
           << " #MPDUs " << psdu->GetNMpdus();
        for (auto it = psdu->begin(); it != psdu->end(); ++it)
        {
            ss << "\n" << **it;
        }
        NS_LOG_INFO(ss.str());
    }
    NS_LOG_INFO("TXVECTOR = " << txVector << "\n");

    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);

    if (!m_checkTxPsdus)
    {
        return;
    }

    if (!m_events.empty())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(WifiMacHeader(m_events.front().hdrType).GetTypeString(),
                              hdr.GetTypeString(),
                              "Unexpected MAC header type for frame transmitted at time "
                                  << Simulator::Now().As(Time::US));
        // perform actions/checks, if any
        if (m_events.front().func)
        {
            m_events.front().func(psdu, txVector, linkId.value());
        }

        m_events.pop_front();
    }
}

void
WifiPsModeAttributesTest::InsertEvents()
{
    m_checkTxPsdus = true;

    // lambda to statically establish a BlockAck agreement and generate a packet for the given STA
    auto generatePacket = [this](WifiDirection dir, std::size_t staId) {
        Ptr<WifiMac> mac;
        if (dir == WifiDirection::DOWNLINK)
        {
            mac = m_apMac;
            WifiStaticSetupHelper::SetStaticBlockAckPostInit(m_apMac->GetDevice(),
                                                             m_staMacs[staId]->GetDevice(),
                                                             0);
        }
        else
        {
            mac = m_staMacs[staId];
            WifiStaticSetupHelper::SetStaticBlockAckPostInit(m_staMacs[staId]->GetDevice(),
                                                             m_apMac->GetDevice(),
                                                             0);
        }

        mac->GetDevice()->GetNode()->AddApplication(GetApplication(dir, staId, 1, 1000));
    };

    // lambda to check PM mode and PHY state of a STA affiliated with the DUT
    auto checkPmModeAndPhyState =
        [this](linkId_t linkId, WifiPowerManagementMode pmMode, WifiPhyState state) {
            const auto now = Simulator::Now();
            // check PM mode
            NS_TEST_EXPECT_MSG_EQ(m_staMacs[DUT_INDEX]->GetPmMode(linkId),
                                  pmMode,
                                  "Unexpected PM mode for STA affiliated with the DUT and "
                                  "operating on link "
                                      << +linkId << "(non-AP MLD side) at time " << now);
            NS_TEST_EXPECT_MSG_EQ(m_apMac->GetWifiRemoteStationManager(linkId)->IsInPsMode(
                                      m_staMacs[DUT_INDEX]->GetAddress()),
                                  pmMode == WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                  "Unexpected PM mode for STA affiliated with the DUT and "
                                  "operating on link "
                                      << +linkId << "(AP MLD side) at time " << now);
            // check PHY state
            NS_TEST_EXPECT_MSG_EQ(m_staMacs[DUT_INDEX]->GetWifiPhy(linkId)->GetState()->GetState(),
                                  state,
                                  "Unexpected state of DUT PHY on link " << +linkId << " at time "
                                                                         << now);
        };

    if (!m_staticSetup)
    {
        if (m_enableEmlsr)
        {
            // EML OMN request
            m_events.emplace_back(WIFI_MAC_MGT_ACTION);
            m_events.emplace_back(WIFI_MAC_CTL_ACK);
            m_events.emplace_back(WIFI_MAC_CTL_END);
            // EML OMN response
            m_events.emplace_back(WIFI_MAC_CTL_TRIGGER);
            m_events.emplace_back(WIFI_MAC_CTL_CTS);
            m_events.emplace_back(WIFI_MAC_MGT_ACTION);
            m_events.emplace_back(WIFI_MAC_CTL_ACK);
            m_events.emplace_back(
                WIFI_MAC_CTL_END,
                [this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
                    // EML OMN exchange has switched all STAs affiliated with the EMLSR client to
                    // active mode, hence we need to switch them to power save mode (one at a time
                    // to have full control on the sequence of transmitted frames)
                    m_staMacs[DUT_INDEX]->GetPowerSaveManager()->SetPowerSaveMode({{0, true}});
                });
            // Data null frame to switch STA on link 0 (auxiliary link) to power save mode
            m_events.emplace_back(WIFI_MAC_CTL_RTS);
            m_events.emplace_back(WIFI_MAC_CTL_CTS);
            m_events.emplace_back(WIFI_MAC_DATA_NULL);
            m_events.emplace_back(
                WIFI_MAC_CTL_ACK,
                [this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
                    // by default, aux PHYs switch link, thus the next Data null frame will also
                    // be sent on a link on which the aux PHY is operating
                    m_staMacs[DUT_INDEX]->GetPowerSaveManager()->SetPowerSaveMode({{1, true}});
                });
            m_events.emplace_back(WIFI_MAC_CTL_RTS);
            m_events.emplace_back(WIFI_MAC_CTL_CTS);
        }

        m_events.emplace_back(WIFI_MAC_DATA_NULL);
        m_events.emplace_back(WIFI_MAC_CTL_ACK);
    }

    // if static setup helper is not used, the STA affiliated with the DUT and operating on the link
    // that is not used for association (link 0) has not yet received a Beacon frame, hence it has
    // not been put to sleep mode. Therefore, wait until a Beacon frame is received on link 0 before
    // generating a packet addressed to the other non-AP STA

    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Beacon frame sent on unexpected link");
            generatePacket(WifiDirection::DOWNLINK, STA_INDEX);
        });

    // data frame addressed to the other non-AP STA (on link 0)
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 0, "Downlink QoS data frame sent on unexpected link");
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetAddr1(),
                m_staMacs[STA_INDEX]->GetFrameExchangeManager(SINGLE_LINK_OP_ID)->GetAddress(),
                "Downlink QoS data frame not addressed to the other non-AP STA");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Downlink QoS data frame not transmitted by the AP MLD");

            // both STAs affiliated with the DUT must be in power save mode and sleep state
            for (const auto id : m_staMacs[DUT_INDEX]->GetSetupLinkIds())
            {
                checkPmModeAndPhyState(id,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       WifiPhyState::SLEEP);
            }

            // schedule the generation of an uplink packet from the DUT when PHY header ends
            Simulator::Schedule(WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector),
                                [=] { generatePacket(WifiDirection::UPLINK, DUT_INDEX); });
        });

    // first data frame transmitted by the DUT (on link 1)
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId,
                                  1,
                                  "First uplink QoS data frame sent on unexpected link");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr1(),
                                  m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "First uplink QoS data frame not addressed to the AP MLD");
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetAddr2(),
                m_staMacs[DUT_INDEX]->GetFrameExchangeManager(linkId)->GetAddress(),
                "First uplink QoS data frame not transmitted by the DUT");

            // both STAs affiliated with the DUT must be still in power save mode and both PHYs of
            // the DUT must be now in awake state: the one operating on link 0 must be in CCA_BUSY
            // state, the one operating on link 1 must be in idle state
            for (const auto id : m_staMacs[DUT_INDEX]->GetSetupLinkIds())
            {
                checkPmModeAndPhyState(id,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       id == 0 ? WifiPhyState::CCA_BUSY : WifiPhyState::IDLE);
            }

            // Apply the PsmTimeout and ListenAdvance values
            auto psManager = m_staMacs[DUT_INDEX]->GetPowerSaveManager();
            psManager->SetAttribute("PsmTimeout", TimeValue(m_psmTimeout));
            psManager->SetAttribute("ListenAdvance", TimeValue(m_listenAdvance));
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId,
                                  0,
                                  "Expected Ack on link 0 to be transmitted before link 1");

            // the STA operating on link 0 goes to sleep when channel access is no longer requested
            // because the DUT has no frames to transmit (it is checked that it is in sleep mode
            // when the next beacon on the other link is transmitted -- see below). Here, we
            // schedule checks to verify that this STA is in sleep state a ListenAdvance interval
            // prior to the next TBTT on this link
            const auto timeUntilNextTbtt =
                m_staMacs[DUT_INDEX]->GetPowerSaveManager()->GetTimeUntilNextTbtt(linkId);
            NS_TEST_ASSERT_MSG_EQ(timeUntilNextTbtt.has_value(),
                                  true,
                                  "Expected next TBTT on link 0 to be known");
            Simulator::Schedule(*timeUntilNextTbtt - m_listenAdvance - TimeStep(1), [=] {
                checkPmModeAndPhyState(linkId,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       WifiPhyState::SLEEP);
            });
            Simulator::Schedule(*timeUntilNextTbtt - m_listenAdvance + TimeStep(1), [=] {
                checkPmModeAndPhyState(linkId,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       WifiPhyState::IDLE);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId,
                                  1,
                                  "Expected Ack on link 1 to be transmitted after link 0");
            const auto band = m_apMac->GetWifiPhy(linkId)->GetPhyBand();
            const auto txDuration = WifiPhy::CalculateTxDuration(psdu, txVector, band);

            // the STA operating on link 1 goes to sleep after the PSM timeout
            Simulator::Schedule(txDuration + m_psmTimeout - TimeStep(1), [=] {
                checkPmModeAndPhyState(linkId,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       WifiPhyState::IDLE);
            });
            Simulator::Schedule(txDuration + m_psmTimeout + TimeStep(1), [=] {
                checkPmModeAndPhyState(linkId,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       WifiPhyState::SLEEP);
            });
            // the STA operating on link 1 stays in sleep state until a ListenAdvance interval prior
            // to the next TBTT
            const auto timeUntilNextTbtt =
                m_staMacs[DUT_INDEX]->GetPowerSaveManager()->GetTimeUntilNextTbtt(linkId);
            NS_TEST_ASSERT_MSG_EQ(timeUntilNextTbtt.has_value(),
                                  true,
                                  "Expected next TBTT on link 1 to be known");
            Simulator::Schedule(*timeUntilNextTbtt - m_listenAdvance - TimeStep(1), [=] {
                checkPmModeAndPhyState(linkId,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       WifiPhyState::SLEEP);
            });
            Simulator::Schedule(*timeUntilNextTbtt - m_listenAdvance + TimeStep(1), [=] {
                checkPmModeAndPhyState(linkId,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       WifiPhyState::IDLE);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_MGT_BEACON,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId, 1, "Beacon frame sent on unexpected link");

            // the STA operating on link 1 is in awake state
            checkPmModeAndPhyState(linkId,
                                   WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                   WifiPhyState::IDLE);
            // the STA operating on link 0 is in sleep state
            checkPmModeAndPhyState(0,
                                   WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                   WifiPhyState::SLEEP);

            const auto band = m_apMac->GetWifiPhy(linkId)->GetPhyBand();
            const auto txDuration = WifiPhy::CalculateTxDuration(psdu, txVector, band);

            // in the middle of the PSM timeout interval following the Beacon frame, generate a
            // packet at the DUT, to check that the PSM timeout is cancelled
            auto delay = txDuration + m_psmTimeout / 2;
            Simulator::Schedule(delay, [=] {
                checkPmModeAndPhyState(linkId,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       WifiPhyState::IDLE);
                generatePacket(WifiDirection::UPLINK, DUT_INDEX);
            });
            // check that both STAs are awake after enqueuing the packet
            delay += TimeStep(1);
            Simulator::Schedule(delay, [=, this] {
                for (const auto id : m_staMacs[DUT_INDEX]->GetSetupLinkIds())
                {
                    NS_TEST_EXPECT_MSG_EQ(m_staMacs[DUT_INDEX]->GetWifiPhy(id)->IsStateSleep(),
                                          false,
                                          "Unexpected state for DUT STA on link " << +id);
                }
            });
            // check that STA on link 1 is still awake after the PSM timeout, i.e., the
            // PSM timeout has been cancelled
            delay += m_psmTimeout / 2;
            Simulator::Schedule(delay, [=, this] {
                NS_TEST_EXPECT_MSG_EQ(m_staMacs[DUT_INDEX]->GetWifiPhy(linkId)->IsStateSleep(),
                                      false,
                                      "Unexpected state for DUT STA on link " << +linkId);
            });
        });

    // second data frame transmitted by the DUT (on link 1, because the STA on link 0 was awaken
    // from sleep state, hence it has to wait for AIFS plus backoff starting from the time it woke
    // up)
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId,
                                  1,
                                  "Second uplink QoS data frame sent on unexpected link");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr1(),
                                  m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Second uplink QoS data frame not addressed to the AP MLD");
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetAddr2(),
                m_staMacs[DUT_INDEX]->GetFrameExchangeManager(linkId)->GetAddress(),
                "Second uplink QoS data frame not transmitted by the DUT");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId,
                                  1,
                                  "Expected Ack on link 1 after second uplink QoS data frame");
            const auto band = m_apMac->GetWifiPhy(linkId)->GetPhyBand();
            const auto txDuration = WifiPhy::CalculateTxDuration(psdu, txVector, band);

            // the STA operating on link 1 goes to sleep after the PSM timeout
            Simulator::Schedule(txDuration + m_psmTimeout - TimeStep(1), [=] {
                checkPmModeAndPhyState(linkId,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       WifiPhyState::IDLE);
            });
            Simulator::Schedule(txDuration + m_psmTimeout + TimeStep(1), [=] {
                checkPmModeAndPhyState(linkId,
                                       WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                       WifiPhyState::SLEEP);
            });
        });
}

void
WifiPsModeAttributesTest::DoRun()
{
    if (m_staticSetup)
    {
        InsertEvents();
    }
    else
    {
        // if static setup is not used, wait until the DUT receives the Association Response and
        // start inserting events right after the Ack is sent
        Callback<void, Mac48Address> cb([this](Mac48Address) {
            Simulator::Schedule(m_staMacs[DUT_INDEX]->GetWifiPhy()->GetSifs() + TimeStep(1),
                                &WifiPsModeAttributesTest::InsertEvents,
                                this);
        });
        m_staMacs[DUT_INDEX]->TraceConnectWithoutContext("Assoc", cb);
    }

    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Power Save Test Suite
 */
class PowerSaveTestSuite : public TestSuite
{
  public:
    PowerSaveTestSuite();
};

PowerSaveTestSuite::PowerSaveTestSuite()
    : TestSuite("wifi-power-save", Type::UNIT)
{
    AddTestCase(new TimInformationElementTest, TestCase::Duration::QUICK);

    for (const auto standard :
         {WIFI_STANDARD_80211a, WIFI_STANDARD_80211n, WIFI_STANDARD_80211ac, WIFI_STANDARD_80211ax})
    {
        AddTestCase(new WifiPowerSaveModeTest(standard), TestCase::Duration::QUICK);
    }

    for (const auto enableEmlsr : {false, true})
    {
        for (const auto staticSetup : {false, true})
        {
            AddTestCase(new WifiPsModeAttributesTest(enableEmlsr, staticSetup),
                        TestCase::Duration::QUICK);
        }
    }
}

static PowerSaveTestSuite g_powerSaveTestSuite; ///< the test suite
