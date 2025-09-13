/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_COEX_TEST_BASE_H
#define WIFI_COEX_TEST_BASE_H

#include "ns3/ap-wifi-mac.h"
#include "ns3/coex-arbitrator.h"
#include "ns3/mock-event-generator.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/wifi-coex-manager.h"
#include "ns3/wifi-psdu.h"

using namespace ns3;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Base class for coex tests
 *
 * This base class performs the setup of an AP and a non-AP STA with a WifiCoexArbitrator and a
 * WifiCoexManager installed. A mock coex event generator can also be installed. A function (which
 * can be overridden) is connected to the CoexEvent trace source of the coex arbitrator.
 */
class WifiCoexTestBase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name The name of the new TestCase created
     */
    WifiCoexTestBase(const std::string& name);
    ~WifiCoexTestBase() override = default;

  protected:
    /**
     * Set the attributes of the mock event generator to the given values. If a mock event generator
     * has not been created yet, one is created and installed.
     *
     * @param interEventTime the time between two consecutive coex events
     * @param startDelay the delay between the notification and the start of a coex event
     * @param duration the duration of a coex event
     */
    void SetMockEventGen(const Time& interEventTime, const Time& startDelay, const Time& duration);

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * @param mac the MAC transmitting the PSDUs
     * @param phyId the ID of the PHY transmitting the PSDUs
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    virtual void Transmit(Ptr<WifiMac> mac,
                          uint8_t phyId,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW);

    /**
     * @param dir the traffic direction (uplink/downlink)
     * @param count the number of packets to generate
     * @param pktSize the size of the packets to generate
     * @param priority user priority for generated packets
     * @return an application generating the given number packets from/to the given AP to/from the
     *         STA
     */
    Ptr<PacketSocketClient> GetApplication(WifiDirection dir,
                                           std::size_t count,
                                           std::size_t pktSize,
                                           uint8_t priority = 0) const;

    /**
     * Function to trace packets received by the server applications
     * @param nodeId the ID of the node that received the packet
     * @param p the packet
     * @param addr the address
     */
    virtual void L7Receive(uint8_t nodeId, Ptr<const Packet> p, const Address& addr);

    /**
     * Callback connected to the CoexEvent trace of the coex arbitrator.
     *
     * @param coexEvent the coex event notified to the coex arbitrator
     */
    virtual void CoexEventCallback(const coex::Event& coexEvent);

    void DoSetup() override;

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

    std::list<Events> m_events; ///< list of events for a test run

    std::string m_channels{
        "{36,0,BAND_5GHZ,0}"}; ///< Semicolon separated (no spaces) list of channel settings for the
                               ///< links of both the AP and the STA
    std::string m_dataMode{"UhrMcs0"}; ///< data mode used by the constant rate manager
    std::string m_arbitratorName{"ns3::coex::WifiArbitrator"}; ///< arbitrator TID name
    std::string m_managerName{"ns3::WifiCoexManager"};         ///< coex manager TID name
    Ptr<ApWifiMac> m_apMac;                                    ///< AP wifi MAC
    Ptr<StaWifiMac> m_staMac;                                  ///< STA wifi MAC
    Time m_duration;                                           ///< simulation duration
    PacketSocketAddress m_ulSocket;               ///< packet socket addresses for UL traffic
    PacketSocketAddress m_dlSocket;               ///< packet socket addresses for DL traffic
    std::array<std::size_t, 2> m_rxPkts{{0, 0}};  ///< number of packets received at application
                                                  ///< layer by AP and STA (index is node ID)
    Ptr<WifiCoexManager> m_coexManager;           ///< the wifi Coex Manager
    Ptr<coex::Arbitrator> m_coexArbitrator;       ///< the Coex Arbitrator
    Ptr<coex::MockEventGenerator> m_mockEventGen; ///< the mock event generator
};

#endif /* WIFI_COEX_TEST_BASE_H */
