/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_DSO_TEST_H
#define WIFI_DSO_TEST_H

#include "ns3/ap-wifi-mac.h"
#include "ns3/error-model.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/nstime.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket-client.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/waveform-generator.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-types.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

using namespace ns3;

namespace ns3
{
enum class DsoTxopEvent;
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Base class for DSO tests
 *
 * This base class setups and configures one AP STA, a variable number of non-AP STAs with
 * DSO activated and a variable number of non-AP STAs with DSO deactivated.
 */
class DsoTestBase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name The name of the new TestCase created
     */
    explicit DsoTestBase(const std::string& name);

    /// Enumeration for traffic directions
    enum TrafficDirection : uint8_t
    {
        DOWNLINK = 0,
        UPLINK
    };

  protected:
    void DoSetup() override;

    /// Information about transmitted frames
    struct FrameInfo
    {
        Time startTx;             ///< TX start time
        WifiConstPsduMap psduMap; ///< transmitted PSDU map
        WifiTxVector txVector;    ///< TXVECTOR
    };

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
     * Start the generation of traffic (needs to be overridden)
     */
    virtual void StartTraffic();

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    virtual void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);

    /**
     * Callback invoked when a packet is received by an application
     *
     * @param p the packet
     * @param adr the address
     */
    virtual void Receive(Ptr<const Packet> p, const Address& adr);

    std::size_t m_nDsoStas{1};    ///< number of UHR non-AP STAs with DSO enabled
    std::size_t m_nNonDsoStas{0}; ///< number of UHR non-AP STAs with DSO disabled
    std::size_t m_nNonUhrStas{0}; ///< number of non-UHR non-AP STAs
    std::string m_apOpChannel;    ///< string representing the operating channel of the AP
    std::vector<std::string>
        m_stasOpChannel;            ///< string representing the operating channel per non-AP STAs
    std::string m_mode{"UhrMcs11"}; ///< the mode to configure on the constant rate manager

    Ptr<MultiModelSpectrumChannel> m_channel; ///< the spectrum channel
    Ptr<ApWifiMac> m_apMac;                   ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs;   ///< MACs of the non-AP STAs
    Time m_duration;                          ///< simulation duration
    std::vector<FrameInfo> m_txPsdus;         ///< transmitted PSDUs
    uint64_t m_receivedPackets{0};            ///< received packets

  private:
    std::vector<PacketSocketAddress> m_dlSockets; ///< packet socket address for DL traffic
    std::vector<PacketSocketAddress> m_ulSockets; ///< packet socket address for UL traffic
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the computation of the primary and DSO subbands.
 *
 * This test considers an AP STA and a non-AP STA with DSO activated.
 */
class DsoSubbandsTest : public DsoTestBase
{
  public:
    /**
     * Constructor
     *
     * @param apChannel The operating channel of the AP
     * @param stasChannel The operating channel of the STAs
     * @param expectedDsoSubbands The expected DSO subbands
     */
    DsoSubbandsTest(
        const std::string& apChannel,
        const std::string& stasChannel,
        const std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel>& expectedDsoSubbands);

  protected:
    void DoRun() override;

  private:
    std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel>
        m_expectedDsoSubbands; //!< expected DSO subbands
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the DSO frame exchange sequence.
 */
class DsoTxopTest : public DsoTestBase
{
  public:
    /**
     * Parameters for the DSO TXOP test
     */
    struct Params
    {
        std::string testName; //!< the name of the test
        std::size_t numDlMuPpdus{
            0}; //!< the number of DL MU PPDU to be transmitted by the AP during DSO TXOP
        std::size_t numUlMuPpdus{
            0}; //!< the number of UL MU PPDU to be transmitted by the STA during DSO TXOP
        Time switchingDelayToDso; //!< the delay to switch the channel to the DSO subband
        std::vector<Time> channelSwitchBackDelays; ///< channel switch back delay per non-AP STAs
        std::vector<Time> paddingDelays;           ///< padding delay per non-AP STAs
        bool nextTxopIsDso{
            false}; //!< whether the next TXOP following the DSO TXOP under test is also a DSO TXOP
        bool protectSingleExchange{false};        //!< whether single protection mechanism is used
        bool generateInterferenceAfterIcf{false}; //!< whether to generate interference in DSO
                                                  //!< subband after ICF transmission has started
        bool generateObssDuringDsoTxop{false};    //!< whether to generate OBSS during DSO TXOP
        std::set<std::size_t> corruptedIcfResponses{}; //!< STA ID that should corrupt the ICF
                                                       //!< response(s) in the first DSO TXOP
        bool corruptIcf{false}; //!< whether to corrupt the ICF in the first DSO TXOP
        DsoTxopEvent expectedTxopEndEventInDsoSubband{}; //!< the expected DSO TXOP termination
                                                         //!< event for operating in DSO subband
    };

    /**
     * Constructor
     *
     * @param params parameters for the DSO TXOP test
     */
    DsoTxopTest(const Params& params);
    ~DsoTxopTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;
    void StartTraffic() override;
    void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW) override;

  private:
    /**
     * Callback connected to the DSO Manager DSO TXOP event trace source.
     *
     * @param index the index of the DSO STA whose DSO TXOP event is logged
     * @param event the event that occurred
     */
    void DsoTxopEventCallback(std::size_t index, DsoTxopEvent event, uint8_t /*linkId*/);

    /**
     * Check that appropriate actions are taken when a DSO ICF is transmitted by the AP.
     *
     * @param psduMap the PSDU carrying the ICF
     * @param txVector the TXVECTOR used to send the ICF
     */
    void CheckIcf(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector);

    /**
     * Check that appropriate actions are taken when the STA transmits a PPDU containing
     * QoS NULL frames.
     *
     * @param psduMap the PSDU(s) carrying QoS NULL frames
     * @param txVector the TXVECTOR used to send the PPDU
     */
    void CheckQosNullFrames(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector);

    /**
     * Check that appropriate actions are taken when the AP transmits a PPDU containing
     * QoS data frames.
     *
     * @param psduMap the PSDU(s) carrying QoS data frames
     * @param txVector the TXVECTOR used to send the PPDU
     */
    void CheckQosFrames(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector);

    /**
     * Check that appropriate actions are taken when BlockAck frames are transmitted by STAs.
     *
     * @param psduMap the PSDU carrying BlockAck frames
     * @param txVector the TXVECTOR used to send the BlockAck frames
     */
    void CheckBlockAck(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector);

    /**
     * Get the delay until current TXOP completion.
     *
     * @param clientId the index of the STA to check
     * @param psduMap the PSDU carrying the last frame of the sequence
     * @param txVector the TXVECTOR used to send the last frame of the sequence
     * @return the delay until current TXOP completion
     */
    Time GetDelayUntilTxopCompletion(std::size_t clientId,
                                     const WifiConstPsduMap& psduMap,
                                     const WifiTxVector& txVector);

    /**
     * Schedule checks for the switch back to the primary subband.
     *
     * @param clientId the index of the STA to check
     * @param delay the delay after which the DSO STA is expected to initiate its switch back
     */
    void ScheduleChecksSwitchBack(std::size_t clientId, const Time& delay);

    /**
     * Schedule checks for the blocked DL transmissions to the DSO STAs.
     *
     * @param delay the delay after which the AP is expected to block its DL transmissions to the
     * DSO STAs
     * @param timeout the timeout to be added to the DSO STA switching duration after which the AP
     * is expected to unblock its DL transmissions to the DSO STAs
     */
    void ScheduleChecksBlockedDlTx(const Time& delay, const Time& timeout);
    /**
     * Status of the switch back to the primary subband.
     */
    enum class SwitchBackStatus : uint8_t
    {
        NOT_SWITCHING = 0,
        SWITCHING_BACK_TO_PRIMARY,
        SWITCHED_BACK_TO_PRIMARY,
    };

    /**
     * Check the switch back to the primary subband.
     *
     * @param staId the index of the STA to check
     * @param status the status to indicate whether the STA is still operating on the DSO subband or
     * switching/switched back to the primary subband
     */
    void CheckChannelSwitchingBack(std::size_t staId, SwitchBackStatus status);

    /**
     * Check that the DL transmissions to the DSO STA are blocked/unblocked.
     *
     * @param staId the index of the STA to check
     * @param blocked whether the DL transmissions to the DSO STA are blocked
     */
    void CheckBlockedDlTx(std::size_t staId, bool blocked);

    /**
     * Check that the simulation produced the expected results.
     */
    void CheckResults();

    /**
     * Generate OBSS frame in the DSO subband
     */
    void GenerateObssFrame();

    /**
     * Start interference function
     * @param duration the duration of the interference
     */
    void StartInterference(const Time& duration);

    /**
     * Stop interference function
     */
    void StopInterference();

    Params m_params; //!< the test parameters

    Ptr<SpectrumWifiPhy> m_obssPhy; ///< PHY of an OBSS STA

    Ptr<WaveformGenerator> m_interferer; ///< waveform generator for interference

    Ptr<ListErrorModel>
        m_apErrorModel; ///< error rate model to artificially corrupt frames sent to the AP
    std::vector<Ptr<ListErrorModel>>
        m_staErrorModels;      ///< error rate model to artificially corrupt frames sent to the STAs
    std::size_t m_countIcf{0}; //!< counter for ICF frames transmitted once traffic has started
    std::size_t m_countQoSframes{0}; //!< counter for QoS frames received once traffic has started
    std::size_t m_countBlockAck{
        0}; //!< counter for BlockAck frames received once traffic has started

    std::map<std::size_t, std::vector<DsoTxopEvent>>
        m_dsoTxopEventInfos{}; //!< map of DSO TXOP events per STA

    std::size_t m_idxStaInDsoSubband{
        1}; //!< index of the STA that is expected to switch to the DSO subband
    std::size_t m_idxStaInPrimarySubband{
        0}; //!< index of the STA that is expected to stay in the primary subband
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the DSO MU scheduler.
 *
 * This test considers an AP with a DSO scheduler and a variable number of non-AP STAs with DSO
 * activated. Downlink, uplink and both traffic directions are tested. For simplicity, a single
 * packet per STA and per direction is enqueued prior to a DL or UL MU transmission. The test also
 * allows to generate more packets for some STAs.
 *
 * It is verified that:
 * - the scheduler allocates RUs to the STAs according to the expected allocations;
 * - RU allocation stay unchanged over a same DSO TXOP;
 * - in case of DL + UL traffic, the scheduler alternates between DL and UL MU transmissions;
 * - the AP and the non-AP STAs transmit the expected number of packets within the duration of the
 * test.
 */
class DsoSchedulerTest : public DsoTestBase
{
  public:
    /**
     * Parameters for the DSO Scheduler test
     */
    struct Params
    {
        std::string testName;    //!< the name of the test
        std::string apOpChannel; ///< string representing the operating channel of the AP
        std::vector<std::string> dsoStasOpChannel; ///< string representing the operating channel
                                                   ///< per non-AP STAs that are DSO capable
        std::vector<std::size_t>
            numGeneratedDlPackets; //!< number of downlink packets to be generated per STA
        std::vector<std::size_t>
            numGeneratedUlPackets;    //!< number of uplink packets to be generated per STA
        std::size_t maxServedStas{4}; //!< maximum number of STAs that can be served by the
                                      //!< scheduler in a single TXOP (0 means no limit)
        std::vector<std::map<uint16_t, EhtRu::RuSpec>>
            expectedRuAllocations; //!< expected RU allocation indexed by STA ID for every TXOP
    };

    /**
     * Constructor
     *
     * @param params parameters for the DSO TXOP test
     */
    DsoSchedulerTest(const Params& params);
    ~DsoSchedulerTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;
    void StartTraffic() override;
    void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW) override;

  private:
    /**
     * Check RUs allocated by scheduler in transmitted Trigger Frame.
     *
     * @param psduMap the PSDU carrying the TF
     * @param txVector the TXVECTOR used to send the TF
     */
    void CheckTrigger(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector);

    /**
     * Check RUs allocated by scheduler in transmitted MU PPDU.
     *
     * @param psduMap the PSDU(s) carrying QoS data frames
     * @param txVector the TXVECTOR used to send the PPDU
     */
    void CheckMuPpdu(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector);

    /**
     * Check that the simulation produced the expected results.
     */
    void CheckResults();

    Params m_params;                          //!< the test parameters
    const bool m_enableUlOfdma;               //!< whether to enable UL OFDMA
    std::size_t m_txopId{0};                  //!< the TXOP ID used to identify the current TXOP
    std::vector<std::size_t> m_dlTxPackets{}; //!< number of transmitted DL packets per STA
    std::vector<std::size_t> m_ulTxPackets{}; //!< number of transmitted UL packets per STA
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi DSO Test Suite
 */
class WifiDsoTestSuite : public TestSuite
{
  public:
    WifiDsoTestSuite();
};

#endif /* WIFI_DSO_TEST_H */
