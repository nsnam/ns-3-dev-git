/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_MLO_TEST_H
#define WIFI_MLO_TEST_H

#include "ns3/ap-wifi-mac.h"
#include "ns3/constant-rate-wifi-manager.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/mgt-headers.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-server.h"
#include "ns3/qos-utils.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/wifi-psdu.h"

#include <optional>
#include <vector>

using namespace ns3;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the implementation of WifiAssocManager::GetNextAffiliatedAp(), which
 * searches a given RNR element for APs affiliated to the same AP MLD as the
 * reporting AP that sent the frame containing the element.
 */
class GetRnrLinkInfoTest : public TestCase
{
  public:
    /**
     * Constructor
     */
    GetRnrLinkInfoTest();
    ~GetRnrLinkInfoTest() override = default;

  private:
    void DoRun() override;
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * Test the WifiMac::SwapLinks() method.
 */
class MldSwapLinksTest : public TestCase
{
    /**
     * Test WifiMac subclass used to access the SwapLinks method.
     */
    class TestWifiMac : public WifiMac
    {
      public:
        ~TestWifiMac() override = default;

        /// @return the object TypeId
        static TypeId GetTypeId();

        using WifiMac::GetLinks;
        using WifiMac::SwapLinks;

        bool CanForwardPacketsTo(Mac48Address to) const override
        {
            return true;
        }

      private:
        void DoCompleteConfig() override
        {
        }

        void Enqueue(Ptr<WifiMpdu> mpdu, Mac48Address to, Mac48Address from) override
        {
        }
    };

    /**
     * Test FrameExchangeManager subclass to access m_linkId
     */
    class TestFrameExchangeManager : public FrameExchangeManager
    {
      public:
        /// @return the link ID stored by this object
        uint8_t GetLinkId() const
        {
            return m_linkId;
        }
    };

    /**
     * Test RemoteStationManager subclass to access m_linkId
     */
    class TestRemoteStationManager : public ConstantRateWifiManager
    {
      public:
        /// @return the link ID stored by this object
        uint8_t GetLinkId() const
        {
            return m_linkId;
        }
    };

  public:
    MldSwapLinksTest();
    ~MldSwapLinksTest() override = default;

  protected:
    void DoRun() override;

  private:
    /**
     * Run a single test case.
     *
     * @param text string identifying the test case
     * @param nLinks the number of links of the MLD
     * @param links a set of pairs (from, to) each mapping a current link ID to the
     *              link ID it has to become (i.e., link 'from' becomes link 'to')
     * @param expected maps each link ID to the id of the PHY that is expected
     *                 to operate on that link after the swap
     */
    void RunOne(std::string text,
                std::size_t nLinks,
                const std::map<uint8_t, uint8_t>& links,
                const std::map<uint8_t, uint8_t>& expected);
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * Test that the AIDs that an AP MLD assigns to SLDs and MLDs are all unique.
 */
class AidAssignmentTest : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param linkIds A vector specifying the set of link IDs each STA will setup
     */
    AidAssignmentTest(const std::vector<std::set<uint8_t>>& linkIds);

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Set the SSID on the next station that needs to start the association procedure.
     * This method is triggered every time a STA completes its association.
     *
     * @param staMac the MAC of the STA that completed association
     */
    void SetSsid(Ptr<StaWifiMac> staMac, Mac48Address /* apAddr */);

    const std::vector<std::string> m_linkChannels;  //!< channels for all AP links
    const std::vector<std::set<uint8_t>> m_linkIds; //!< link IDs for all non-AP STAs/MLDs
    NetDeviceContainer m_staDevices;                //!< non-AP STAs/MLDs devices
    uint16_t m_expectedAid;                         //!< expected AID for current non-AP STA/MLD
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Base class for Multi-Link Operations tests
 *
 * Three spectrum channels are created, one for each band (2.4 GHz, 5 GHz and 6 GHz).
 * Each PHY object is attached to the spectrum channel corresponding to the PHY band
 * in which it is operating.
 */
class MultiLinkOperationsTestBase : public TestCase
{
  public:
    /**
     * Configuration parameters common to all subclasses
     */
    struct BaseParams
    {
        std::vector<std::string>
            staChannels; //!< the strings specifying the operating channels for the non-AP MLD
        std::vector<std::string>
            apChannels; //!< the strings specifying the operating channels for the AP MLD
        std::vector<uint8_t>
            fixedPhyBands; //!< list of IDs of non-AP MLD PHYs that cannot switch band
    };

    /**
     * Constructor
     *
     * @param name The name of the new TestCase created
     * @param nStations the number of stations to create
     * @param baseParams common configuration parameters
     */
    MultiLinkOperationsTestBase(const std::string& name,
                                uint8_t nStations,
                                const BaseParams& baseParams);
    ~MultiLinkOperationsTestBase() override = default;

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
     * Check that the expected Capabilities information elements are present in the given
     * management frame based on the band in which the given link is operating.
     *
     * @param mpdu the given management frame
     * @param mac the MAC transmitting the management frame
     * @param phyId the ID of the PHY transmitting the management frame
     */
    void CheckCapabilities(Ptr<WifiMpdu> mpdu, Ptr<WifiMac> mac, uint8_t phyId);

    /**
     * Function to trace packets received by the server application
     * @param nodeId the ID of the node that received the packet
     * @param p the packet
     * @param addr the address
     */
    virtual void L7Receive(uint8_t nodeId, Ptr<const Packet> p, const Address& addr);

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
                                           Time delay = Seconds(0),
                                           uint8_t priority = 0) const;

    void DoSetup() override;

    /// PHY band-indexed map of spectrum channels
    using ChannelMap = std::map<FrequencyRange, Ptr<MultiModelSpectrumChannel>>;

    /**
     * Uplink or Downlink direction
     */
    enum Direction
    {
        DL = 0,
        UL
    };

    /**
     * Check that the Address 1 and Address 2 fields of the given PSDU contain device MAC addresses.
     *
     * @param psdu the given PSDU
     * @param direction indicates direction for management frames (DL or UL)
     */
    void CheckAddresses(Ptr<const WifiPsdu> psdu,
                        std::optional<Direction> direction = std::nullopt);

    /// Information about transmitted frames
    struct FrameInfo
    {
        Time startTx;             ///< TX start time
        WifiConstPsduMap psduMap; ///< transmitted PSDU map
        WifiTxVector txVector;    ///< TXVECTOR
        uint8_t linkId;           ///< link ID
        uint8_t phyId;            ///< ID of the transmitting PHY
    };

    std::vector<FrameInfo> m_txPsdus;             ///< transmitted PSDUs
    const std::vector<std::string> m_staChannels; ///< strings specifying channels for STA
    const std::vector<std::string> m_apChannels;  ///< strings specifying channels for AP
    const std::vector<uint8_t> m_fixedPhyBands;   ///< links on non-AP MLD with fixed PHY band
    Ptr<ApWifiMac> m_apMac;                       ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs;       ///< STA wifi MACs
    uint8_t m_nStations;                          ///< number of stations to create
    uint16_t m_lastAid;                           ///< AID of last associated station
    Time m_duration{Seconds(1)};                  ///< simulation duration
    std::vector<std::size_t> m_rxPkts; ///< number of packets received at application layer
                                       ///< by each node (index is node ID)

    /**
     * Reset the given PHY helper, use the given strings to set the ChannelSettings
     * attribute of the PHY objects to create, and attach them to the given spectrum
     * channels appropriately.
     *
     * @param helper the given PHY helper
     * @param channels the strings specifying the operating channels to configure
     * @param channelMap the created spectrum channels
     */
    void SetChannels(SpectrumWifiPhyHelper& helper,
                     const std::vector<std::string>& channels,
                     const ChannelMap& channelMap);

    /**
     * Set the SSID on the next station that needs to start the association procedure.
     * This method is connected to the ApWifiMac's AssociatedSta trace source.
     * Start generating traffic (if needed) when all stations are associated.
     *
     * @param aid the AID assigned to the previous associated STA
     */
    void SetSsid(uint16_t aid, Mac48Address /* addr */);

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
 * @brief Multi-Link Discovery & Setup test.
 *
 * This test sets up an AP MLD and a non-AP MLD having a variable number of links.
 * The RF channels to set each link to are provided as input parameters through the test
 * case constructor, along with the identifiers (starting at 0) of the links that cannot
 * switch PHY band (if any). The links that are expected to be setup are also provided as input
 * parameters. This test verifies that the management frames exchanged during ML discovery
 * and ML setup contain the expected values and that the two MLDs setup the expected links.
 *
 * The negotiated TID-to-link mapping is tested by verifying that generated QoS data frames of
 * a given TID are transmitted on links which the TID is mapped to. Specifically, the following
 * operations are performed separately for each direction (downlink and uplink). A first TID
 * is searched such that it is not mapped on all the setup links. If no such TID is found, only
 * QoS frames of TID 0 are generated. Otherwise, we also search for a second TID that is mapped
 * to a link set that is disjoint with the link set to which the first TID is mapped. If such a
 * TID is found, QoS frames of both the first TID and the second TID are generated; otherwise,
 * only QoS frames of the first TID are generated. For each TID, a number of QoS frames equal
 * to the number of setup links is generated. For each TID, we check that the first N QoS frames,
 * where N is the size of the link set to which the TID is mapped, are transmitted concurrently,
 * while the following QoS frames are sent after the first QoS frame sent on the same link. We
 * also check that all the QoS frames are sent on a link belonging to the link set to which the
 * TID is mapped. If QoS frames of two TIDs are generated, we also check that the first N QoS
 * frames of a TID, where N is the size of the link set to which that TID is mapped, are sent
 * concurrently with the first M QoS frames of the other TID, where M is the size of the link
 * set to which the other TID is mapped.
 */
class MultiLinkSetupTest : public MultiLinkOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * @param baseParams common configuration parameters
     * @param scanType the scan type (active or passive)
     * @param setupLinks a list of links that are expected to be setup. In case one of the two
     *                   devices has a single link, the ID of the link on the MLD is indicated
     * @param apNegSupport TID-to-Link Mapping negotiation supported by the AP MLD (0, 1, or 3)
     * @param dlTidToLinkMapping DL TID-to-Link Mapping for EHT configuration of non-AP MLD
     * @param ulTidToLinkMapping UL TID-to-Link Mapping for EHT configuration of non-AP MLD
     * @param support160MHzOp whether non-AP MLDs support 160 MHz operations
     */
    MultiLinkSetupTest(const BaseParams& baseParams,
                       WifiScanType scanType,
                       const std::vector<uint8_t>& setupLinks,
                       WifiTidToLinkMappingNegSupport apNegSupport,
                       const std::string& dlTidToLinkMapping,
                       const std::string& ulTidToLinkMapping,
                       bool support160MHzOp = true);
    ~MultiLinkSetupTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;

  private:
    void StartTraffic() override;

    /**
     * Check correctness of Multi-Link Setup procedure.
     */
    void CheckMlSetup();

    /**
     * Check that links that are not setup on the non-AP MLD are disabled. Also, on the AP side,
     * check that the queue storing QoS data frames destined to the non-AP MLD has a mask for a
     * link if and only if the link has been setup by the non-AO MLD.
     */
    void CheckDisabledLinks();

    /**
     * Check correctness of the given Beacon frame.
     *
     * @param mpdu the given Beacon frame
     * @param linkId the ID of the link on which the Beacon frame was transmitted
     */
    void CheckBeacon(Ptr<WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Check correctness of the given Probe Response frame.
     *
     * @param mpdu the given Probe Response frame
     * @param linkId the ID of the link on which the Probe Response frame was transmitted
     */
    void CheckProbeResponse(Ptr<WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Check correctness of the given Association Request frame.
     *
     * @param mpdu the given Association Request frame
     * @param linkId the ID of the link on which the Association Request frame was transmitted
     */
    void CheckAssocRequest(Ptr<WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Check correctness of the given Association Response frame.
     *
     * @param mpdu the given Association Response frame
     * @param linkId the ID of the link on which the Association Response frame was transmitted
     */
    void CheckAssocResponse(Ptr<WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Check that QoS data frames are sent on links their TID is mapped to and with the
     * correct TX width.
     *
     * @param mpdu the given QoS data frame
     * @param txvector the TXVECTOR used to send the QoS data frame
     * @param linkId the ID of the link on which the QoS data frame was transmitted
     * @param index index of the QoS data frame in the vector of transmitted PSDUs
     */
    void CheckQosData(Ptr<WifiMpdu> mpdu,
                      const WifiTxVector& txvector,
                      uint8_t linkId,
                      std::size_t index);

    const std::vector<uint8_t> m_setupLinks; //!< IDs of the expected links to setup
    WifiScanType m_scanType;                 //!< the scan type (active or passive)
    std::size_t m_nProbeResp; //!< number of Probe Responses received by the non-AP MLD
    WifiTidToLinkMappingNegSupport
        m_apNegSupport;                //!< TID-to-Link Mapping negotiation supported by the AP MLD
    std::string m_dlTidLinkMappingStr; //!< DL TID-to-Link Mapping for non-AP MLD EHT configuration
    std::string m_ulTidLinkMappingStr; //!< UL TID-to-Link Mapping for non-AP MLD EHT configuration
    WifiTidLinkMapping m_dlTidLinkMapping; //!< expected DL TID-to-Link Mapping requested by non-AP
                                           //!< MLD and accepted by AP MLD
    WifiTidLinkMapping m_ulTidLinkMapping; //!< expected UL TID-to-Link Mapping requested by non-AP
                                           //!< MLD and accepted by AP MLD
    uint8_t m_dlTid1;                      //!< the TID of the first set of DL QoS data frames
    uint8_t m_ulTid1;                      //!< the TID of the first set of UL QoS data frames
    std::optional<uint8_t> m_dlTid2;       //!< the TID of the optional set of DL QoS data frames
    std::optional<uint8_t> m_ulTid2;       //!< the TID of the optional set of UL QoS data frames
    std::vector<std::size_t>
        m_qosFrames1; //!< indices of QoS frames of the first set in the vector of TX PSDUs
    std::vector<std::size_t>
        m_qosFrames2;       //!< indices of QoS frames of the optional set in the vector of TX PSDUs
    bool m_support160MHzOp; //!< whether non-AP MLDs support 160 MHz operations
};

/**
 * Tested traffic patterns.
 */
enum class WifiTrafficPattern : uint8_t
{
    STA_TO_STA = 0,
    STA_TO_AP,
    AP_TO_STA,
    AP_TO_BCAST,
    STA_TO_BCAST
};

/**
 * Block Ack agreement enabled/disabled
 */
enum class WifiBaEnabled : uint8_t
{
    NO = 0,
    YES
};

/**
 * Whether to send a BlockAckReq after a missed BlockAck
 */
enum class WifiUseBarAfterMissedBa : uint8_t
{
    NO = 0,
    YES
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test data transmission between two MLDs.
 *
 * This test sets up an AP MLD and two non-AP MLDs having a variable number of links.
 * The RF channels to set each link to are provided as input parameters through the test
 * case constructor, along with the identifiers (starting at 0) of the links that cannot
 * switch PHY band (if any). This test aims at veryfing the successful transmission of both
 * unicast QoS data frames (from one station to another, from one station to the AP, from
 * the AP to the station) and broadcast QoS data frames (from the AP or from one station).
 * In the scenarios in which the AP forwards frames (i.e., from one station to another and
 * from one station to broadcast) the client application generates only 4 packets, in order
 * to limit the probability of collisions. In the other scenarios, 8 packets are generated.
 * When BlockAck agreements are enabled, the maximum A-MSDU size is set such that two
 * packets can be aggregated in an A-MSDU. The MPDU with sequence number equal to 1 is
 * corrupted (once, by using a post reception error model) to test its successful
 * re-transmission, unless the traffic scenario is from the AP to broadcast (broadcast frames
 * are not retransmitted) or is a scenario where the AP forwards frame (to limit the
 * probability of collisions).
 *
 * When BlockAck agreements are enabled, we also corrupt a BlockAck frame, so as to simulate
 * the case of BlockAck timeout. Both the case where a BlockAckReq is sent and the case where
 * data frame are retransmitted are tested. Finally, when BlockAck agreements are enabled, we
 * also enable the concurrent transmission of data frames over two links and check that at
 * least one MPDU is concurrently transmitted over two links.
 */
class MultiLinkTxTest : public MultiLinkOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * @param baseParams common configuration parameters
     * @param trafficPattern the pattern of traffic to generate
     * @param baEnabled whether BA agreement is enabled or disabled
     * @param useBarAfterMissedBa whether a BAR or Data frames are sent after missed BlockAck
     * @param nMaxInflight the max number of links on which an MPDU can be simultaneously inflight
     *                     (unused if Block Ack agreements are not established)
     */
    MultiLinkTxTest(const BaseParams& baseParams,
                    WifiTrafficPattern trafficPattern,
                    WifiBaEnabled baEnabled,
                    WifiUseBarAfterMissedBa useBarAfterMissedBa,
                    uint8_t nMaxInflight);
    ~MultiLinkTxTest() override = default;

  protected:
    /**
     * Check the content of a received BlockAck frame when the max number of links on which
     * an MPDU can be inflight is one.
     *
     * @param psdu the PSDU containing the BlockAck
     * @param txVector the TXVECTOR used to transmit the BlockAck
     * @param linkId the ID of the link on which the BlockAck was transmitted
     */
    void CheckBlockAck(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId);

    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;
    void DoSetup() override;
    void DoRun() override;

  private:
    void StartTraffic() override;

    /// Receiver address-indexed map of list error models
    using RxErrorModelMap = std::unordered_map<Mac48Address, Ptr<ListErrorModel>, WifiAddressHash>;

    RxErrorModelMap m_errorModels;       ///< error rate models to corrupt packets
    std::list<uint64_t> m_uidList;       ///< list of UIDs of packets to corrupt
    bool m_dataCorrupted{false};         ///< whether second data frame has been already corrupted
    WifiTrafficPattern m_trafficPattern; ///< the pattern of traffic to generate
    bool m_baEnabled;                    ///< whether BA agreement is enabled or disabled
    bool m_useBarAfterMissedBa;          ///< whether to send BAR after missed BlockAck
    std::size_t m_nMaxInflight;          ///< max number of links on which an MPDU can be inflight
    std::size_t m_nPackets;              ///< number of application packets to generate
    std::size_t m_blockAckCount{0};      ///< transmitted BlockAck counter
    std::size_t m_blockAckReqCount{0};   ///< transmitted BlockAckReq counter
    std::map<uint16_t, std::size_t> m_inflightCount; ///< seqNo-indexed max number of simultaneous
                                                     ///< transmissions of a data frame
    Ptr<WifiMac> m_sourceMac; ///< MAC of the node sending application packets
};

/**
 * Tested MU traffic patterns.
 */
enum class WifiMuTrafficPattern : uint8_t
{
    DL_MU_BAR_BA_SEQUENCE = 0,
    DL_MU_MU_BAR,
    DL_MU_AGGR_MU_BAR,
    UL_MU
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test data transmission between MLDs using OFDMA MU transmissions
 *
 * This test sets up an AP MLD and two non-AP MLDs having a variable number of links.
 * The RF channels to set each link to are provided as input parameters through the test
 * case constructor, along with the identifiers (starting at 0) of the links that cannot
 * switch PHY band (if any). This test aims at veryfing the successful transmission of both
 * DL MU and UL MU frames. In the DL MU scenarios, the client applications installed on the
 * AP generate 8 packets addressed to each of the stations (plus 3 packets to trigger the
 * establishment of BlockAck agreements). In the UL MU scenario, client applications
 * installed on the stations generate 4 packets each (plus 3 packets to trigger the
 * establishment of BlockAck agreements).
 *
 * The maximum A-MSDU size is set such that two packets can be aggregated in an A-MSDU.
 * The MPDU with sequence number equal to 3 is corrupted (by using a post reception error
 * model) once and for a single station, to test its successful re-transmission.
 *
 * Also, we enable the concurrent transmission of data frames over two links and check that at
 * least one MPDU is concurrently transmitted over two links.
 */
class MultiLinkMuTxTest : public MultiLinkOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * @param baseParams common configuration parameters
     * @param muTrafficPattern the pattern of traffic to generate
     * @param useBarAfterMissedBa whether a BAR or Data frames are sent after missed BlockAck
     * @param nMaxInflight the max number of links on which an MPDU can be simultaneously inflight
     *                     (unused if Block Ack agreements are not established)
     */
    MultiLinkMuTxTest(const BaseParams& baseParams,
                      WifiMuTrafficPattern muTrafficPattern,
                      WifiUseBarAfterMissedBa useBarAfterMissedBa,
                      uint8_t nMaxInflight);
    ~MultiLinkMuTxTest() override = default;

  protected:
    /**
     * Check the content of a received BlockAck frame when the max number of links on which
     * an MPDU can be inflight is one.
     *
     * @param psdu the PSDU containing the BlockAck
     * @param txVector the TXVECTOR used to transmit the BlockAck
     * @param linkId the ID of the link on which the BlockAck was transmitted
     */
    void CheckBlockAck(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId);

    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;
    void DoSetup() override;
    void DoRun() override;

  private:
    void StartTraffic() override;

    /// Receiver address-indexed map of list error models
    using RxErrorModelMap = std::unordered_map<Mac48Address, Ptr<ListErrorModel>, WifiAddressHash>;

    /// A pair of a MAC address (the address of the receiver for DL frames and the address of
    /// the sender for UL frames) and a sequence number identifying a transmitted QoS data frame
    using AddrSeqNoPair = std::pair<Mac48Address, uint16_t>;

    RxErrorModelMap m_errorModels;                  ///< error rate models to corrupt packets
    std::list<uint64_t> m_uidList;                  ///< list of UIDs of packets to corrupt
    std::optional<Mac48Address> m_dataCorruptedSta; ///< MAC address of the station that received
                                                    ///< MPDU with SeqNo=2 corrupted
    bool m_waitFirstTf{true}; ///< whether we are waiting for the first Basic Trigger Frame
    WifiMuTrafficPattern m_muTrafficPattern; ///< the pattern of traffic to generate
    bool m_useBarAfterMissedBa;              ///< whether to send BAR after missed BlockAck
    std::size_t m_nMaxInflight; ///< max number of links on which an MPDU can be inflight
    std::vector<PacketSocketAddress> m_sockets; ///< packet socket addresses for STAs
    std::size_t m_nPackets;                     ///< number of application packets to generate
    std::size_t m_blockAckCount{0};             ///< transmitted BlockAck counter
    std::size_t m_tfCount{0};                   ///< transmitted Trigger Frame counter
    // std::size_t m_blockAckReqCount{0};     ///< transmitted BlockAckReq counter
    std::map<AddrSeqNoPair, std::size_t> m_inflightCount; ///< max number of simultaneous
                                                          ///< transmissions of each data frame
    Ptr<WifiMac> m_sourceMac; ///< MAC of the node sending application packets
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test release of sequence numbers upon CTS timeout in multi-link operations
 *
 * In this test, an AP MLD and a non-AP MLD setup 3 links. Usage of RTS/CTS protection is
 * enabled for frames whose length is at least 1000 bytes. The AP MLD receives a first set
 * of 4 packets from the upper layer and sends an RTS frame, which is corrupted at the
 * receiver, on a first link. When the RTS frame is transmitted, the AP MLD receives another
 * set of 4 packets, which are transmitted after a successful RTS/CTS exchange on a second
 * link. In the meantime, a new RTS/CTS exchange is successfully carried out (on the first
 * link or on the third link) to transmit the first set of 4 packets. When the transmission
 * of the first set of 4 packets starts, the AP MLD receives the third set of 4 packets from
 * the upper layer, which are transmitted after a successful RTS/CTS exchange.
 *
 * This test checks that sequence numbers are correctly assigned to all the MPDUs carrying data.
 */
class ReleaseSeqNoAfterCtsTimeoutTest : public MultiLinkOperationsTestBase
{
  public:
    ReleaseSeqNoAfterCtsTimeoutTest();
    ~ReleaseSeqNoAfterCtsTimeoutTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

  private:
    void StartTraffic() override;

    PacketSocketAddress m_sockAddr;   //!< packet socket address
    std::size_t m_nQosDataFrames;     //!< counter for transmitted QoS data frames
    Ptr<ListErrorModel> m_errorModel; //!< error rate model to corrupt first RTS frame
    bool m_rtsCorrupted;              //!< whether the first RTS frame has been corrupted
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test update of BA starting sequence number after ADDBA Response timeout in
 *        multi-link operations
 *
 * In this test, an AP MLD and a non-AP MLD setup 2 links. The AP MLD has a QoS data frame to
 * transmit to the non-AP MLD, which triggers the establishment of a BA agreement. When the ADDBA
 * Request frame is received by the non-AP MLD, transmissions of the non-AP MLD are blocked to put
 * the transmission of the ADDBA Response on hold. The goal is to mimic a delay in getting channel
 * access due to, e.g., other devices grabbing the medium. When the ADDBA Response timer at the AP
 * MLD expires, transmissions of the non-AP MLD are unblocked, so that the AP MLD sends the QoS data
 * frame (protected by RTS, but the test works without RTS as well, and using Normal acknowledgment)
 * on one link and the non-AP MLD sends the ADDBA Response on the other link. The transmission of
 * the QoS data frame is then corrupted. We verify that:
 *
 * - when the AP MLD receives the ADDBA Response, the BA starting sequence number is set to the
 *   sequence number of the QoS data frame which is inflight
 * - the QoS data frame is retransmitted and received by the non-AP MLD
 */
class StartSeqNoUpdateAfterAddBaTimeoutTest : public MultiLinkOperationsTestBase
{
  public:
    StartSeqNoUpdateAfterAddBaTimeoutTest();

  private:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;
    void StartTraffic() override;

    PacketSocketAddress m_sockAddr;      //!< packet socket address
    std::size_t m_nQosDataCount;         //!< counter for transmitted QoS data frames
    Ptr<ListErrorModel> m_staErrorModel; //!< error rate model to corrupt frames at the non-AP MLD
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi 11be MLD Test Suite
 */
class WifiMultiLinkOperationsTestSuite : public TestSuite
{
  public:
    WifiMultiLinkOperationsTestSuite();
};

#endif /* WIFI_MLO_TEST_H */
