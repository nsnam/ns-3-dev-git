/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_EMLSR_TEST_H
#define WIFI_EMLSR_TEST_H

#include "ns3/adhoc-wifi-mac.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/error-model.h"
#include "ns3/header-serialization-test.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket-client.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-queue-scheduler.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-psdu.h"

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

using namespace ns3;
using namespace std::string_literals;

// forward declaration
namespace ns3
{
struct EmlsrMainPhySwitchTrace;
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Base class for EMLSR Operations tests
 *
 * This base class setups and configures one AP MLD, a variable number of non-AP MLDs with
 * EMLSR activated and a variable number of non-AP MLD with EMLSR deactivated. Every MLD has
 * three links, each operating on a distinct PHY band (2.4 GHz, 5 GHz and 6 GHz). Therefore,
 * it is expected that three links are setup by the non-AP MLD(s). The values for the Padding
 * Delay, the Transition Delay and the Transition Timeout are provided as argument to the
 * constructor of this class, along with the IDs of the links on which EMLSR mode must be
 * enabled for the non-AP MLDs (this information is used to set the EmlsrLinkSet attribute
 * of the DefaultEmlsrManager installed on the non-AP MLDs).
 */
class EmlsrOperationsTestBase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name The name of the new TestCase created
     */
    EmlsrOperationsTestBase(const std::string& name);
    ~EmlsrOperationsTestBase() override = default;

    /// Enumeration for traffic directions
    enum TrafficDirection : uint8_t
    {
        DOWNLINK = 0,
        UPLINK
    };

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
     * @param dir the traffic direction (downlink/uplink)
     * @param staId the index (starting at 0) of the non-AP MLD generating/receiving packets
     * @param count the number of packets to generate
     * @param pktSize the size of the packets to generate
     * @param priority user priority for generated packets
     * @return an application generating the given number packets of the given size from/to the
     *         AP MLD to/from the given non-AP MLD
     */
    Ptr<PacketSocketClient> GetApplication(TrafficDirection dir,
                                           std::size_t staId,
                                           std::size_t count,
                                           std::size_t pktSize,
                                           uint8_t priority = 0) const;

    /**
     * Check whether QoS data unicast transmissions addressed to the given destination on the
     * given link are blocked or unblocked for the given reason on the given device.
     *
     * @param mac the MAC of the given device
     * @param dest the MAC address of the given destination
     * @param linkId the ID of the given link
     * @param reason the reason for blocking transmissions to test
     * @param blocked whether transmissions are blocked for the given reason
     * @param description text indicating when this check is performed
     * @param testUnblockedForOtherReasons whether to test if transmissions are unblocked for
     *                                     all the reasons other than the one provided
     */
    void CheckBlockedLink(Ptr<WifiMac> mac,
                          Mac48Address dest,
                          uint8_t linkId,
                          WifiQueueBlockedReason reason,
                          bool blocked,
                          std::string description,
                          bool testUnblockedForOtherReasons = true);

    /**
     * Check whether the MediumSyncDelay timer is running on the given link of the given device.
     *
     * @param staMac the MAC of the given device
     * @param linkId the ID of the given link
     * @param isRunning whether the MediumSyncDelay timer is running
     * @param msg message to print in case the check failed
     */
    void CheckMsdTimerRunning(Ptr<StaWifiMac> staMac,
                              uint8_t linkId,
                              bool isRunning,
                              const std::string& msg);

    /**
     * Check whether aux PHYs of the given device are in sleep mode/awake.
     *
     * @param staMac the MAC of the given device
     * @param sleep whether aux PHYs should be in sleep mode
     */
    void CheckAuxPhysSleepMode(Ptr<StaWifiMac> staMac, bool sleep);

    /**
     * Callback connected to the EMLSR Manager MainPhySwitch trace source.
     *
     * @param index the index of the EMLSR client whose main PHY switch event is logged
     * @param info the information associated with the main PHY switch event
     */
    virtual void MainPhySwitchInfoCallback(std::size_t index, const EmlsrMainPhySwitchTrace& info);

    /**
     * Check information provided by the EMLSR Manager MainPhySwitch trace.
     *
     * @param index the ID of the EMLSR client this check refers to
     * @param reason the reason for main PHY to switch
     * @param fromLinkId the ID of the link the main PHY is moving from (if any)
     * @param toLinkId the ID of the link the main PHY is moving to
     * @param checkFromLinkId whether to check the given fromLinkId value
     * @param checkToLinkId whether to check the given toLinkId value
     */
    void CheckMainPhyTraceInfo(std::size_t index,
                               std::string_view reason,
                               const std::optional<uint8_t>& fromLinkId,
                               uint8_t toLinkId,
                               bool checkFromLinkId = true,
                               bool checkToLinkId = true);

    void DoSetup() override;

    /// Information about transmitted frames
    struct FrameInfo
    {
        Time startTx;             ///< TX start time
        WifiConstPsduMap psduMap; ///< transmitted PSDU map
        WifiTxVector txVector;    ///< TXVECTOR
        uint8_t linkId;           ///< link ID
        uint8_t phyId;            ///< ID of the transmitting PHY
    };

    /// array of strings defining the channels for the MLD links
    std::array<std::string, 3> m_channelsStr{"{2, 0, BAND_2_4GHZ, 0}"s,
                                             "{36, 0, BAND_5GHZ, 0}"s,
                                             "{1, 0, BAND_6GHZ, 0}"s};

    /// array of frequency ranges for MLD links
    const std::array<FrequencyRange, 3> m_freqRanges{WIFI_SPECTRUM_2_4_GHZ,
                                                     WIFI_SPECTRUM_5_GHZ,
                                                     WIFI_SPECTRUM_6_GHZ};

    uint32_t m_rngSeed{1};                    //!< RNG seed value
    uint64_t m_rngRun{1};                     //!< RNG run value
    int64_t m_streamNo{5};                    //!< RNG stream number
    uint8_t m_mainPhyId{0};                   //!< ID of the main PHY
    std::set<uint8_t> m_linksToEnableEmlsrOn; /**< IDs of the links on which EMLSR mode has to
                                                   be enabled */
    std::size_t m_nPhysPerEmlsrDevice{3};     //!< number of PHYs per EMLSR client
    std::size_t m_nEmlsrStations{1};          ///< number of stations to create that activate EMLSR
    std::size_t m_nNonEmlsrStations{0};       /**< number of stations to create that do not
                                                activate EMLSR */
    Time m_transitionTimeout{MicroSeconds(128)}; ///< Transition Timeout advertised by the AP MLD
    std::vector<Time> m_paddingDelay{
        {MicroSeconds(32)}}; ///< Padding Delay advertised by the non-AP MLD
    std::vector<Time> m_transitionDelay{
        {MicroSeconds(16)}};                ///< Transition Delay advertised by the non-AP MLD
    std::vector<uint8_t> m_establishBaDl{}; /**< the TIDs for which BA needs to be established
                                                 with the AP as originator */
    std::vector<uint8_t> m_establishBaUl{}; /**< the TIDs for which BA needs to be established
                                                 with the AP as recipient */
    bool m_putAuxPhyToSleep{false};   //!< whether aux PHYs are put to sleep during DL/UL TXOPs
    std::vector<FrameInfo> m_txPsdus; ///< transmitted PSDUs
    Ptr<ApWifiMac> m_apMac;           ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs;       ///< MACs of the non-AP MLDs
    std::vector<PacketSocketAddress> m_dlSockets; ///< packet socket address for DL traffic
    std::vector<PacketSocketAddress> m_ulSockets; ///< packet socket address for UL traffic
    uint16_t m_startAid{1};                       ///< first AID to allocate to stations
    uint16_t m_lastAid{0};                        ///< AID of last associated station
    Time m_duration{0};                           ///< simulation duration
    std::map<std::size_t, std::shared_ptr<EmlsrMainPhySwitchTrace>>
        m_traceInfo; ///< EMLSR client ID-indexed map of trace info from last main PHY switch

  private:
    /**
     * Callback connected to the ApWifiMac's AssociatedSta trace source.
     * Start generating traffic (if needed) when all stations are associated.
     *
     * @param aid the AID assigned to the previous associated STA
     */
    void StaAssociated(uint16_t aid, Mac48Address /* addr */);

    /**
     * Callback connected to the QosTxop's BaEstablished trace source of the AP's BE AC.
     *
     * @param recipient the recipient of the established Block Ack agreement
     * @param tid the TID
     */
    void BaEstablishedDl(Mac48Address recipient,
                         uint8_t tid,
                         std::optional<Mac48Address> /* gcrGroup */);

    /**
     * Callback connected to the QosTxop's BaEstablished trace source of a STA's BE AC.
     *
     * @param index the index of the STA which the callback is connected to
     * @param recipient the recipient of the established Block Ack agreement
     * @param tid the TID
     */
    void BaEstablishedUl(std::size_t index,
                         Mac48Address recipient,
                         uint8_t tid,
                         std::optional<Mac48Address> /* gcrGroup */);

    /**
     * Set the SSID on the next station that needs to start the association procedure, or start
     * traffic if no other station left.
     *
     * @param count the number of STAs that completed the association procedure
     */
    void SetSsid(std::size_t count);

    /**
     * Start the generation of traffic (needs to be overridden)
     */
    virtual void StartTraffic()
    {
    }
};

#endif /* WIFI_EMLSR_TEST_H */
