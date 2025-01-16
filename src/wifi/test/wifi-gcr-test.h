/*
 * Copyright (c) 2023 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_GCR_TEST_H
#define WIFI_GCR_TEST_H

#include "ns3/ap-wifi-mac.h"
#include "ns3/error-model.h"
#include "ns3/ht-phy.h"
#include "ns3/packet-socket-client.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/wifi-default-gcr-manager.h"
#include "ns3/wifi-mac-helper.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

using namespace ns3;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Base class for GCR tests
 *
 * It considers an AP and multiple STAs (with different capabilities) using either GCR-UR or GCR-BA.
 * The AP generates either multicast packets only or alternatively multicast and unicast packets.
 *
 * The test eventually corrupts some MPDUs based on a provided list of groupcast MPDUs in a given
 * PSDU (indices are starting from 1) that should not be successfully received by a given STA or by
 * all STA (s). It may also corrupts specific frames, such as RTS/CTS or action frames that are used
 * to establish or teardown Block Ack agreements. The latter is needed is needed for GCR-BA or for
 * GCR-UR when A-MPDU is used.
 *
 * It is checked that:
 *
 * - When no GCR-capable STA is present, GCR service is not used
 * - When the GCR service is used, groupcast frames are transmitted using HT, VHT or HE modulation
 * class, depending on the supported modulations by member STAs
 * - When the GCR is used, groupcast MPDUs carry an A-MSDU made of a single A-MSDU subframe,
 * regardless of the A-MSDU size settings
 * - When the GCR service is used, the expected protection mechanism is being used prior to the
 * transmission of the groupcast packet
 * - When the GCR service is used and RTS/CTS protection is selected, the receiver address of RTS
 * frames corresponds to the MAC address of one of the STA of the group
 * - When GCR-BA or GCR-UR with agreement is used, the expected amount of ADDBA request and response
 * frames has been received and they all contain the GCR group address
 * - when Block Ack agreement timeout is used, the expected amount of DELBA frames has been received
 * and they all contain the GCR group address
 * - The expected buffer size is being selected for the GCR Block Ack agreement, depending on what
 * is supported by each member
 */
class GcrTestBase : public TestCase
{
  public:
    /// Information about GCR STAs
    struct StaInfo
    {
        bool gcrCapable{false};                           ///< flag whether the STA is GCR capable
        WifiStandard standard{WIFI_STANDARD_UNSPECIFIED}; ///< standard configured for the STA
        MHz_u maxChannelWidth{20};    ///< maximum channel width supported by the STA
        uint8_t maxNumStreams{1};     ///< maximum number of spatial streams supported by the STA
        Time minGi{NanoSeconds(800)}; ///< minimum guard interval duration supported by the STA
    };

    /// Common parameters for GCR tests
    struct GcrParameters
    {
        std::vector<StaInfo> stas{};         ///< information about STAs
        uint16_t numGroupcastPackets{0};     ///< number of groupcast packets to generate
        uint16_t numUnicastPackets{0};       ///< number of unicast packets to generate
        uint32_t packetSize{1000};           ///< the size in bytes of the packets to generate
        uint16_t maxNumMpdusInPsdu{0};       ///< maximum number of MPDUs in PSDUs
        Time startGroupcast{Seconds(1.0)};   ///< time to start groupcast packets generation
        Time startUnicast{};                 ///< time to start unicast packets generation
        Time maxLifetime{MilliSeconds(500)}; ///< the maximum MSDU lifetime
        uint32_t rtsThreshold{};             ///< the RTS threshold in bytes
        GroupcastProtectionMode gcrProtectionMode{
            GroupcastProtectionMode::RTS_CTS}; ///< the protection mode to use
        std::map<uint8_t, std::map<uint8_t, std::set<uint8_t>>>
            mpdusToCorruptPerPsdu{}; ///< list of MPDUs (starting from 1) to corrupt per PSDU
                                     ///< (starting from 1)
        std::set<uint8_t> rtsFramesToCorrupt{}; ///< list of RTS frames (starting from 1) to corrupt
        std::set<uint8_t> ctsFramesToCorrupt{}; ///< list of CTS frames (starting from 1) to corrupt
        std::set<uint8_t>
            addbaReqsToCorrupt{}; ///< list of GCR ADDBA requests (starting from 1) to corrupt
        std::set<uint8_t>
            addbaRespsToCorrupt{}; ///< list of GCR ADDBA responses (starting from 1) to corrupt
        std::set<uint8_t> expectedDroppedGroupcastMpdus{}; ///< list of groupcast MPDUs that are
                                                           ///< expected to be dropped because of
                                                           ///< lifetime expiry (starting from 1)
        uint16_t baInactivityTimeout{0}; ///< max time (blocks of 1024 microseconds) allowed for
                                         ///< block ack inactivity
        Time txopLimit{};                ///< the TXOP limit duration
        Time duration{
            Seconds(2)}; ///< the duration of the simulation for the test run (2 seconds by default)
    };

    /**
     * Constructor
     *
     * @param testName the name of the test
     * @param params the common GCR parameters for the test to run
     */
    GcrTestBase(const std::string& testName, const GcrParameters& params);
    ~GcrTestBase() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Check results at the end of the test run.
     */
    virtual void CheckResults();

    /**
     * Configure the GCR manager for the test.
     *
     * @param macHelper the wifi mac helper
     */
    virtual void ConfigureGcrManager(WifiMacHelper& macHelper) = 0;

    /**
     * Callback invoked when a packet is generated by the packet socket client.
     *
     * @param context the context
     * @param p the packet
     * @param adr the address
     */
    virtual void PacketGenerated(std::string context, Ptr<const Packet> p, const Address& adr);

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * @param context the context
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    virtual void Transmit(std::string context,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW);

    /**
     * Callback invoked when a packet is successfully received by the PHY.
     *
     * @param context the context
     * @param p the packet
     * @param snr the SNR (in linear scale)
     * @param mode the WiFi mode
     * @param preamble the preamble
     */
    virtual void PhyRx(std::string context,
                       Ptr<const Packet> p,
                       double snr,
                       WifiMode mode,
                       WifiPreamble preamble);

    /**
     * Callback invoked when packet is received by the packet socket server.
     *
     * @param context the context
     * @param p the packet
     * @param adr the address
     */
    virtual void Receive(std::string context, Ptr<const Packet> p, const Address& adr) = 0;

    /**
     * Callback invoked when a TXOP is terminated.
     *
     * @param startTime the time TXOP started
     * @param duration the duration of the TXOP
     * @param linkId the ID of the link that gained TXOP
     */
    virtual void NotifyTxopTerminated(Time startTime, Time duration, uint8_t linkId);

    /**
     * Function to indicate whether A-MPDU or S-MPDU is currently being used.
     *
     * @return true if A-MPDU or S-MPDU is currently being used, false otherwise
     */
    virtual bool IsUsingAmpduOrSmpdu() const;

    std::string m_testName; ///< name of the test
    GcrParameters m_params; ///< parameters for the test to run
    bool m_expectGcrUsed;   ///< flag whether GCR is expected to be used during the test
    uint16_t m_expectedMaxNumMpdusInPsdu; ///< expected maximum number of MPDUs in PSDUs

    Ptr<ApWifiMac> m_apWifiMac;                 ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_stasWifiMac; ///< STAs wifi MAC
    Ptr<ListErrorModel> m_apErrorModel; ///< error rate model to corrupt frames sent to the AP
    std::vector<Ptr<ListErrorModel>>
        m_errorModels;                         ///< error rate models to corrupt packets (per STA)
    Ptr<PacketSocketClient> m_groupcastClient; ///< the packet socket client

    uint16_t m_packets; ///< Number of generated groupcast packets by the application
    std::vector<uint16_t>
        m_phyRxPerSta;  ///< count number of PSDUs successfully received by PHY of each STA
    uint8_t m_nTxApRts; ///< number of RTS frames sent by the AP
    uint8_t m_nTxApCts; ///< number of CTS-to-self frames sent by the AP
    std::vector<uint8_t> m_txCtsPerSta; ///< count number of CTS responses frames sent by each STA
    uint8_t m_totalTx;                  ///< total number of groupcast frames transmitted by the AP
    std::vector<std::vector<uint16_t>>
        m_rxGroupcastPerSta; ///< count groupcast packets received by the packet socket server of
                             ///< each STA and store TX attempt number for each received packet
    std::vector<uint16_t> m_rxUnicastPerSta; ///< count unicast packets received by the packet
                                             ///< socket server of each STA

    uint8_t m_nTxGroupcastInCurrentTxop; ///< number of groupcast frames transmitted by the AP
                                         ///< (including retries) in the current TXOP
    uint8_t
        m_nTxRtsInCurrentTxop; ///< number of RTS frames transmitted by the AP in the current TXOP
    uint8_t m_nTxCtsInCurrentTxop; ///< number of CTS-to-self frames transmitted by the AP in the
                                   ///< current TXOP

    uint8_t m_nTxAddbaReq;     ///< number of transmitted ADDBA Request frames
    uint8_t m_nTxAddbaResp;    ///< number of transmitted ADDBA Response frames
    uint8_t m_nTxDelba;        ///< number of transmitted DELBA frames
    uint8_t m_nTxGcrAddbaReq;  ///< number of transmitted GCR ADDBA Request frames
    uint8_t m_nTxGcrAddbaResp; ///< number of transmitted GCR ADDBA Response frames
    uint8_t m_nTxGcrDelba;     ///< number of transmitted GCR DELBA frames
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the implementation of GCR-UR.
 *
 * GCR-UR tests consider an AP and multiple STAs (with different capabilities) using GCR-UR with up
 * to 7 retries. Besides what is verified in the base class, it is checked that:
 *
 * - When the GCR-UR service is used, each groupcast packet is retransmitted 7 times, in different
 * TXOPs
 * - When the GCR-UR service is used, all retransmissions of an MPDU have the Retry field in their
 * Frame Control fields set to 1
 * - When the GCR-UR service is used, either the initial packet or one of its retransmission is
 * successfully received, unless all packets are corrupted
 * - When the GCR-UR service is used and MPDU aggregation is enabled, it is checked each STA
 * receives the expected amount of MPDUs
 * - When the GCR-UR service is used and MPDU aggregation is enabled, it is checked received MPDUs
 * are forwarded up in the expected order and the recipient window is correctly flushed
 */
class GcrUrTest : public GcrTestBase
{
  public:
    /// Parameters for GCR-UR tests
    struct GcrUrParameters
    {
        uint8_t nGcrRetries{7}; ///< number of solicited retries to use for GCR-UR
        uint8_t expectedSkippedRetries{
            0}; ///< the number of skipped retries because of lifetime expiry
        std::optional<uint16_t>
            packetsPauzeAggregation{}; ///< the amount of generated packets after which MPDU
                                       ///< aggregation should not be used by limiting the queue to
                                       ///< a single packet. If not specified, MPDU aggregation is
                                       ///< not paused
        std::optional<uint16_t>
            packetsResumeAggregation{}; ///< the amount of generated packets after which MPDU
                                        ///< aggregation should be used again by refilling the queue
                                        ///< with more packets. If not specified, MPDU aggregation
                                        ///< is not resumed
    };

    /**
     * Constructor
     *
     * @param testName the name of the test
     * @param commonParams the common GCR parameters for the test to run
     * @param gcrUrParams the GCR-UR parameters for the test to run
     */
    GcrUrTest(const std::string& testName,
              const GcrParameters& commonParams,
              const GcrUrParameters& gcrUrParams);
    ~GcrUrTest() override = default;

  private:
    void ConfigureGcrManager(WifiMacHelper& macHelper) override;
    void CheckResults() override;
    void PacketGenerated(std::string context, Ptr<const Packet> p, const Address& adr) override;
    void Transmit(std::string context,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;
    void Receive(std::string context, Ptr<const Packet> p, const Address& adr) override;
    bool IsUsingAmpduOrSmpdu() const override;

    GcrUrParameters m_gcrUrParams; ///< GCR-UR parameters for the test to run

    std::vector<uint8_t>
        m_totalTxGroupcasts; ///< total number of groupcast frames transmitted by the AP
                             ///< (including retries) per original groupcast frame

    Ptr<WifiMpdu> m_currentMpdu; ///< current MPDU
    uint64_t m_currentUid;       ///< current UID
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the implementation of GCR Block Ack.
 *
 * GCR-BA tests consider an AP and multiple STAs (with different capabilities) using GCR-BA.
 * During tests, besides frames that can be corrupted by the base class, transmitted MPDUs
 * can be corrupted, either for all STAs or for a particular STA. These tests eventually corrupt
 * Block Ack Requests and Block Acks frames.
 *
 * Besides what is verified in the base class, it is checked that:
 * - The expected amount of packets has been forwarded up to upper layer
 * - When the GCR-BA service is used, the expected amount of Block Ack request and Block Acks frames
 * have been received and they all contain the GCR group address
 * - MPDUs are properly discarded when their lifetime expires, and TX window as well as receiver
 * scoreboard are properly advanced if this occurs
 * - When the GCR-BA service is used, the exchange of GCR Block Ack Request and GCR Block Acks
 * frames might be spread over multiple TXOPs
 * - A-MPDU is only used if TXOP limit duration permits it
 */
class GcrBaTest : public GcrTestBase
{
  public:
    /// Parameters for GCR-BA tests
    struct GcrBaParameters
    {
        std::set<uint8_t> barsToCorrupt{}; ///< list of GCR BARs (starting from 1) to corrupt
        std::set<uint8_t>
            blockAcksToCorrupt{}; ///< list of GCR Block ACKs (starting from 1) to corrupt
        std::vector<uint8_t>
            expectedNTxBarsPerTxop{}; ///< the expected number of BAR frames transmitted by the AP
                                      ///< per TXOP (only takes into account TXOPs with BARs
                                      ///< transmitted)
    };

    /**
     * Constructor
     *
     * @param testName the name of the test
     * @param commonParams the common GCR parameters for the test to run
     * @param gcrBaParams the GCR-BA parameters for the test to run
     */
    GcrBaTest(const std::string& testName,
              const GcrParameters& commonParams,
              const GcrBaParameters& gcrBaParams);
    ~GcrBaTest() override = default;

  private:
    void ConfigureGcrManager(WifiMacHelper& macHelper) override;
    void CheckResults() override;
    void PacketGenerated(std::string context, Ptr<const Packet> p, const Address& adr) override;
    void Transmit(std::string context,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;
    void Receive(std::string context, Ptr<const Packet> p, const Address& adr) override;
    void NotifyTxopTerminated(Time startTime, Time duration, uint8_t linkId) override;

    GcrBaParameters m_gcrBaParams; ///< GCR-BA parameters for the test to run

    uint8_t m_nTxGcrBar;      ///< number of GCR Block Ack Request frames sent by the AP
    uint8_t m_nTxGcrBlockAck; ///< number of GCR Block Ack Response frames sent to the AP
    uint8_t m_nTxBlockAck;    ///< number of Block Ack Response frames sent to the AP
    uint16_t m_firstTxSeq;    ///< sequence number of the first in-flight groupcast MPDU
    int m_lastTxSeq;          ///< sequence number of the last in-flight groupcast MPDU

    std::vector<uint8_t>
        m_nTxGcrBarsPerTxop; ///< number of GCR BAR frames transmitted by the AP per TXOP (only
                             ///< takes into account TXOPs with BARs transmitted)
    uint8_t m_nTxGcrBarsInCurrentTxop; ///< number of GCR BAR frames transmitted by the AP in the
                                       ///< current TXOP
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi GCR Test Suite
 */
class WifiGcrTestSuite : public TestSuite
{
  public:
    WifiGcrTestSuite();
};

#endif /* WIFI_GCR_TEST_H */
