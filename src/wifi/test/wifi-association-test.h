/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_ASSOC_TEST_H
#define WIFI_ASSOC_TEST_H

#include "ns3/ap-wifi-mac.h"
#include "ns3/log.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-client.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/wifi-assoc-manager.h"

#include <list>
#include <map>
#include <set>
#include <sstream>

using namespace ns3;

namespace
{
/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * Test Association Manager allowing users to explicitly configure the best AP to associate with.
 */
class WifiTestAssocManager : public WifiAssocManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    void NotifyChannelSwitched(linkId_t linkId) override;
    bool Compare(const StaWifiMac::ApInfo& lhs, const StaWifiMac::ApInfo& rhs) const override;

    /**
     * Set the best AP to associate with. For all the APs other than the best AP, CanBeReturned()
     * returns false, therefore the station can only associate with the given AP, if present in
     * the AP info list held by this manager.
     *
     * @param bssid the address of the best AP (possibly affiliated with an AP MLD)
     * @param setupLinks the list of links to setup, in case of ML setup
     */
    void SetBestAp(Mac48Address bssid,
                   const std::list<StaWifiMac::ApInfo::SetupLinksInfo>& setupLinks);

  protected:
    bool CanBeInserted(const StaWifiMac::ApInfo& apInfo) const override;
    bool CanBeReturned(const StaWifiMac::ApInfo& apInfo) const override;

  private:
    void DoStartScanning() override;

    Mac48Address m_bestAp; ///< the MAC address of the AP to associate with
    std::list<StaWifiMac::ApInfo::SetupLinksInfo> m_setupLinks; ///< the list of links to setup
};

} // namespace

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Base class for association tests
 *
 * Setup one non-AP STA and two APs with a configurable number of links.
 */
class WifiAssociationTestBase : public TestCase
{
  public:
    /// Configuration parameters common to all subclasses
    struct BaseParams
    {
        std::string ap1Channels; ///< Semicolon separated (no spaces) list of channel settings for
                                 ///< the links of AP 1
        std::string ap2Channels; ///< Semicolon separated (no spaces) list of channel settings for
                                 ///< the links of AP 2
        std::string staChannels; ///< Semicolon separated (no spaces) list of channel settings for
                                 ///< the links of STA
    };

    /**
     * Constructor
     *
     * @param name The name of the new TestCase created
     * @param baseParams common configuration parameters
     */
    WifiAssociationTestBase(const std::string& name, const BaseParams& baseParams);
    ~WifiAssociationTestBase() override = default;

  protected:
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
     * @param apId the AP ID (1 or 2)
     * @param count the number of packets to generate
     * @param pktSize the size of the packets to generate
     * @param priority user priority for generated packets
     * @return an application generating the given number packets from/to the given AP to/from the
     *         STA
     */
    Ptr<PacketSocketClient> GetApplication(WifiDirection dir,
                                           std::size_t apId,
                                           std::size_t count,
                                           std::size_t pktSize,
                                           uint8_t priority = 0) const;

    /**
     * Callback invoked when a STA completes association with an AP.
     *
     * @param bssid the AP address
     */
    virtual void AssocSeenBySta(Mac48Address bssid)
    {
    }

    /**
     * Callback invoked when a STA transitions to unassociated state.
     *
     * @param bssid the address of the AP the STA was associated with
     */
    virtual void DisassocSeenBySta(Mac48Address bssid)
    {
    }

    /**
     * Callback invoked when the given AP detects that the given STA has completed association.
     *
     * @param apMac the MAC of the given AP
     * @param aid the AID of the associated STA
     * @param staAddress the address of the associated STA
     */
    virtual void AssocSeenByAp(Ptr<ApWifiMac> apMac, uint16_t aid, Mac48Address staAddress)
    {
    }

    /**
     * Callback invoked when the given AP detects that the given STA has disassociated.
     *
     * @param apMac the MAC of the given AP
     * @param aid the AID of the STA that disassociated
     * @param staAddress the address of the STA that disassociated
     */
    virtual void DisassocSeenByAp(Ptr<ApWifiMac> apMac, uint16_t aid, Mac48Address staAddress)
    {
    }

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

    std::list<Events> m_events; //!< list of events for a test run

    BaseParams m_baseParams;                                      ///< configuration parameters
    std::string m_assocMgrTypeId{"ns3::WifiDefaultAssocManager"}; ///< Association Manager type ID
    Ptr<ApWifiMac> m_ap1Mac;                                      ///< AP 1 wifi MAC
    Ptr<ApWifiMac> m_ap2Mac;                                      ///< AP 2 wifi MAC
    Ptr<StaWifiMac> m_staMac;                                     ///< STA wifi MAC
    Time m_duration{Seconds(0.8)};                                ///< simulation duration
    std::map<WifiDirection, PacketSocketAddress> m_ap1Sockets;    ///< packet socket addresses for
                                                                  ///< DL/UL traffic from AP1
    std::map<WifiDirection, PacketSocketAddress> m_ap2Sockets;    ///< packet socket addresses for
                                                                  ///< DL/UL traffic from AP2
    bool m_checkTxPsdus{false}; ///< whether TX PSDUs should be checked against the list of events
    std::set<WifiMacType>
        m_framesToSkip; ///< frames that must not be checked against the list of events

  private:
    /**
     * Start the generation of traffic (needs to be overridden)
     */
    virtual void StartTraffic()
    {
    }
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test association, disassociation and reassociation in various scenarios
 *
 * Example scenario for uplink traffic (STA sends Disassociation frame before roaming to another AP)
 *
 *                       request
 *      ┌───┬───┬───┐ disassociation         ┌─────┐                             ┌───┬───┬───┐
 *      │QoS│   │QoS│     | ┌────────┐       │Assoc│               ┌───┐         │QoS│   │QoS│
 * STA  │ 1 │...│ n │     | │Disassoc│       │ Req │               │Ack│         │ 1 │...│ n │
 * ─────┴───X───┴───┴─────v─┴────────┴───────┴─────┴───────────────┴───┴─^───────┴───┴───┴───┴─────
 *                          |---optional--|                              | BA agreement
 *                   ┌──┐             ┌───┐                               establishment
 * AP 1              │BA│             │Ack│                            between STA and AP 2
 * ──────────────────┴──X─────────────┴───┴────────────────────────────────────────────────────────
 *                                                          ┌─────┐
 *                                                  ┌───┐   │Assoc│                           ┌──┐
 * AP 2                                             │Ack│   │ Resp│                           │BA│
 * ─────────────────────────────────────────────────┴───┴───┴─────┴───────────────────────────┴──┴─
 *
 *
 * After each association, it is checked that:
 *
 * - the station manager of the AP operating on a given link reports that the STA is associated
 *   if and only if that link is expected to be setup
 * - the station manager of the AP operating on a given link returns the address of the STA
 *   affiliated to the non-AP MLD and operating on that link if and only if the STA performs ML
 *   setup (instead of legacy association) and that link is expected to be setup
 * - the STA has setup the expected links and the BSSIDs are correct
 *
 * After the first association, a first set of (DL or UL) packets are generated, transmitted to/by
 * the first AP. The first two MPDUs in the transmitted A-MPDU are corrupted; the receiver sends the
 * acknowledgment, which however is corrupted and is not received.
 * Disassociation is then requested (before BlockAck timeout, if a Disassociation frame is sent, or
 * after BlockAckTimeout, otherwise) so that a BlockAckReq is queued when the STA actually
 * disassociates (tested).
 * On both the STA side (when disassociation is requested, if a Disassociation frame is not sent,
 * or when the Disassociation frame is acked, otherwise) and the AP side (when the Disassociation
 * frame is received, if the STA sends a Disassociation frame, or when the Reassociation frame is
 * received, if the STA does not send a Disassociation frame and associates with the same AP again),
 * it is checked that:
 *
 * - transmissions of unicast frames (but management frames) on all links setup by the STA are
 *   blocked with DISASSOCIATION reason
 * - [STA side only] QoS data frames are still queued
 * - [STA side only] no BAR (having link addresses) is queued anymore if ML setup was performed or
 *   STA notified disassociation to the AP
 * - Block Ack agreement has been destroyed if and only if STA notified disassociation to the AP
 *
 * After the second association, it is checked that:
 *
 * - a new Block Ack agreement is established if STA roams to a different AP or if STA destroyed the
 *   previous agreement (which occurs when STA sends a Disassociation frame) or legacy association
 *   is performed and STA setups a different link than before
 * - STA has not dropped the BlockAckReq frame if and only if STA has not sent a Disassociation
 *   frame (otherwise all control frames are dropped), STA established legacy association and
 *   reassociates with the same AP (and same link)
 * - if STA has a queued BlockAckReq frame, such a frame is transmitted first, followed by a
 *   BlockAck response; afterwards, STA retransmits the two MPDUs that are indicated as not received
 *   in the BlockAck response. Otherwise, all the QoS data frames are retransmitted.
 */
class WifiDisassocAndReassocTest : public WifiAssociationTestBase
{
  public:
    /// Setup Links info for AP info objects
    using SetupLinksInfo = std::pair<linkId_t /* STA link ID */, linkId_t /* AP link ID */>;

    /// Configuration parameters specific to this test
    struct Params
    {
        WifiAssocType assocType{};   ///< the association type (legacy or ML setup)
        std::size_t firstApLinkId{}; ///< the ID of the link (as seen by the AP) that is used to
                                     ///< send the first Association Request
        std::list<SetupLinksInfo> firstSetupLinks; ///< links to setup with the first AP
        std::size_t secondApId{}; ///< the ID (1 or 2) of the AP (possibly affiliated with an AP
                                  ///< MLD) that is the recipient of the second Association Request
        std::size_t secondApLinkId{}; ///< the ID of the link (as seen by the AP) that is used to
                                      ///< send the second Association Request
        std::list<SetupLinksInfo> secondSetupLinks; ///< links to setup with the second AP
        WifiDirection trafficDir{};                 ///< traffic direction
        bool sendDisassoc{}; ///< whether the STA notifies disassociation to the AP
    };

    /**
     * Constructor
     *
     * @param baseParams common configuration parameters
     * @param params parameters specific for this test
     */
    WifiDisassocAndReassocTest(const BaseParams& baseParams, const Params& params);

  protected:
    void AssocSeenBySta(Mac48Address bssid) override;
    void AssocSeenByAp(Ptr<ApWifiMac> apMac, uint16_t aid, Mac48Address staAddress) override;

    /// Insert elements in the list of expected events (transmitted frames)
    void InsertEvents();

    /// Take actions connected to roaming
    void StartRoaming();

    /**
     * Check that the AP which the STA is expected to be associated with stores the expected info
     * about the STA in its remote station managers.
     *
     * @param firstAssoc true/false if the STA associated for the first/second time
     */
    void CheckRemoteStaInfo(bool firstAssoc);

    /**
     * Check that the STA stores the expected info about the AP it is expected to be associated
     * with.
     *
     * @param firstAssoc true/false if the STA associated for the first/second time
     */
    void CheckRemoteApInfo(bool firstAssoc);

    /**
     * Check that the expected operations are performed by the local device after disassociation.
     *
     * @param local the MAC of the local device
     */
    void CheckAfterDisassoc(Ptr<WifiMac> local);

  private:
    void DoSetup() override;
    void DoRun() override;

    const std::size_t m_firstApId{1}; ///< the ID of the AP (possibly affiliated with an AP MLD)
                                      ///< that is the recipient of the first Assoc Request
    Params m_params;                  ///< parameters specific for this test
    const std::size_t m_nPackets{5};  ///< number of packets generated in a burst
    Time m_assocTime;                 ///< time at which last association took place
    linkId_t m_baLinkId{
        SINGLE_LINK_OP_ID};               ///< ID of the link on which the BA is sent before roaming
    std::size_t m_assocWithAp1Count{0};   ///< number of associations with AP 1 (seen by STA)
    std::size_t m_assocWithAp2Count{0};   ///< number of associations with AP 2 (seen by STA)
    std::size_t m_assocSeenByAp1Count{0}; ///< number of associations with AP 1 (seen by AP 1)
    std::size_t m_assocSeenByAp2Count{0}; ///< number of associations with AP 2 (seen by AP 2)
    Ptr<ListErrorModel> m_staErrorModel;  ///< error rate model to install on the STA
    Ptr<ListErrorModel> m_ap1ErrorModel;  ///< error rate model to install on the first AP
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi 11be MLD Test Suite
 */
class WifiAssociationTestSuite : public TestSuite
{
  public:
    WifiAssociationTestSuite();
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param params the base params
 * @returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const WifiAssociationTestBase::BaseParams& params)
{
    return (os << "BaseParams{AP 1: " << params.ap1Channels << ", AP 2: " << params.ap2Channels
               << ", STA: " << params.staChannels << "}");
}

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param params the WifiDisassocAndReassocTest params
 * @returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const WifiDisassocAndReassocTest::Params& params)
{
    os << "Params{" << params.trafficDir << ", "
       << (params.assocType == WifiAssocType::ML_SETUP ? "ML setup" : "Legacy assoc") << ", "
       << (params.sendDisassoc ? "Send" : "Do NOT send") << " Disassoc frame";
    os << " [1st] AP link ID: " << params.firstApLinkId << ", Setup links: ";
    for (const auto& [staLinkId, apLinkID] : params.firstSetupLinks)
    {
        os << "(" << +staLinkId << "," << +apLinkID << ")";
    }
    os << " [2nd] AP ID: " << params.secondApId << ", AP link ID: " << params.secondApLinkId
       << ", Setup links: ";
    for (const auto& [staLinkId, apLinkID] : params.secondSetupLinks)
    {
        os << "(" << +staLinkId << "," << +apLinkID << ")";
    }
    os << "}";

    return os;
}

#endif /* WIFI_ASSOC_TEST_H */
