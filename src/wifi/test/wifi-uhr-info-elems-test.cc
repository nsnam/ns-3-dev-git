/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uhr-capabilities.h"
#include "ns3/uhr-operation.h"

#include <optional>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiUhrInfoElemsTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test UHR Capabilities information element serialization and deserialization
 */
class UhrCapabilitiesElementTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Parameters for UhrCapabilitiesElementTest
     */
    struct TestParams
    {
        std::string name;                                    ///< Test case name
        UhrCapabilities::UhrMacCapabilities macCapabilities; ///< UHR MAC Capabilities field
        UhrCapabilities::UhrPhyCapabilities phyCapabilities; ///< UHR PHY Capabilities field
    };

    /**
     * Constructor
     *
     * @param params Test parameters
     */
    UhrCapabilitiesElementTest(const TestParams& params);

  private:
    void DoRun() override;

    UhrCapabilities m_uhrCapabilities; ///< UHR Capabilities element
};

UhrCapabilitiesElementTest::UhrCapabilitiesElementTest(const TestParams& params)
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of UHR Capabilities elements: " + params.name)
{
    m_uhrCapabilities.m_macCapabilities = params.macCapabilities;
    m_uhrCapabilities.m_phyCapabilities = params.phyCapabilities;
}

void
UhrCapabilitiesElementTest::DoRun()
{
    TestHeaderSerialization(m_uhrCapabilities);
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test UHR Operation information element serialization and deserialization
 */
class UhrOperationElementTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Parameters for UhrOperationElementTest
     */
    struct TestParams
    {
        std::string name;                   ///< Test case name
        UhrOperation::UhrOpParams opParams; ///< UHR Operation Parameters field
        uint8_t rxMaxNss0_7{};              ///< RX max NSS that supports EHT MCS 0-7
        uint8_t txMaxNss0_7{};              ///< TX max NSS that supports EHT MCS 0-7
        uint8_t rxMaxNss8_9{};              ///< RX max NSS that supports EHT MCS 8-9
        uint8_t txMaxNss8_9{};              ///< TX max NSS that supports EHT MCS 8-9
        uint8_t rxMaxNss10_11{};            ///< RX max NSS that supports EHT MCS 10-11
        uint8_t txMaxNss10_11{};            ///< TX max NSS that supports EHT MCS 10-11
        uint8_t rxMaxNss12_13{};            ///< RX max NSS that supports EHT MCS 12-13
        uint8_t txMaxNss12_13{};            ///< TX max NSS that supports EHT MCS 12-13
        std::optional<UhrOperation::NpcaOpParams>
            npcaParams; ///< Optional NPCA Operation Parameters field
        std::optional<UhrOperation::PEdcaOpParams>
            pEdcaParams; ///< Optional P-EDCA Operation Parameters field
        std::optional<UhrOperation::DbeOpParams>
            dbeParams; ///< Optional DBE Operation Parameters field
    };

    /**
     * Constructor
     *
     * @param params Test parameters
     */
    UhrOperationElementTest(const TestParams& params);

  private:
    void DoRun() override;

    UhrOperation m_uhrOperation; ///< UHR Operation element
};

UhrOperationElementTest::UhrOperationElementTest(const TestParams& params)
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of UHR Operation elements: " + params.name)
{
    m_uhrOperation.m_params = params.opParams;
    m_uhrOperation.SetMaxRxNss(params.rxMaxNss0_7, 0, 7);
    m_uhrOperation.SetMaxTxNss(params.txMaxNss0_7, 0, 7);
    m_uhrOperation.SetMaxRxNss(params.rxMaxNss8_9, 8, 9);
    m_uhrOperation.SetMaxTxNss(params.txMaxNss8_9, 8, 9);
    m_uhrOperation.SetMaxRxNss(params.rxMaxNss10_11, 10, 11);
    m_uhrOperation.SetMaxTxNss(params.txMaxNss10_11, 10, 11);
    m_uhrOperation.SetMaxRxNss(params.rxMaxNss12_13, 12, 13);
    m_uhrOperation.SetMaxTxNss(params.txMaxNss12_13, 12, 13);
    m_uhrOperation.m_npcaParams = params.npcaParams;
    m_uhrOperation.m_pEdcaParams = params.pEdcaParams;
    m_uhrOperation.m_dbeParams = params.dbeParams;
}

void
UhrOperationElementTest::DoRun()
{
    TestHeaderSerialization(m_uhrOperation);
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi UHR Information Elements Test Suite
 */
class WifiUhrInfoElemsTestSuite : public TestSuite
{
  public:
    WifiUhrInfoElemsTestSuite();
};

WifiUhrInfoElemsTestSuite::WifiUhrInfoElemsTestSuite()
    : TestSuite("wifi-uhr-info-elems", Type::UNIT)
{
    AddTestCase(new UhrCapabilitiesElementTest({
                    .name = "DSO feature advertisement",
                    .macCapabilities =
                        UhrCapabilities::UhrMacCapabilities{
                            .dsoSupport = 1,
                        },
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new UhrOperationElementTest({
                    .name = "all features disabled",
                    .rxMaxNss0_7 = 1,
                    .txMaxNss0_7 = 2,
                    .rxMaxNss8_9 = 3,
                    .txMaxNss8_9 = 4,
                    .rxMaxNss10_11 = 5,
                    .txMaxNss10_11 = 6,
                    .rxMaxNss12_13 = 7,
                    .txMaxNss12_13 = 8,
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new UhrOperationElementTest({
                    .name = "NPCA enabled, without Disabled Subchannel Bitmap",
                    .rxMaxNss0_7 = 1,
                    .txMaxNss0_7 = 2,
                    .rxMaxNss8_9 = 3,
                    .txMaxNss8_9 = 4,
                    .rxMaxNss10_11 = 5,
                    .txMaxNss10_11 = 6,
                    .rxMaxNss12_13 = 7,
                    .txMaxNss12_13 = 8,
                    .npcaParams =
                        UhrOperation::NpcaOpParams{
                            .primaryChan = 6,
                            .minDurationThresh = 5,
                            .switchDelay = 20,
                            .switchBackDelay = 30,
                            .initialQsrc = 2,
                            .moplen = 1,
                            .disSubchanBmPresent = 0,
                        },
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new UhrOperationElementTest({
                    .name = "NPCA enabled, with Disabled Subchannel Bitmap",
                    .rxMaxNss0_7 = 1,
                    .txMaxNss0_7 = 2,
                    .rxMaxNss8_9 = 3,
                    .txMaxNss8_9 = 4,
                    .rxMaxNss10_11 = 5,
                    .txMaxNss10_11 = 6,
                    .rxMaxNss12_13 = 7,
                    .txMaxNss12_13 = 8,
                    .npcaParams =
                        UhrOperation::NpcaOpParams{
                            .primaryChan = 6,
                            .minDurationThresh = 5,
                            .switchDelay = 20,
                            .switchBackDelay = 30,
                            .initialQsrc = 2,
                            .moplen = 1,
                            .disSubchanBmPresent = 1,
                            .disabledSubchBm = 0x0FFF,
                        },
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new UhrOperationElementTest({
                    .name = "P-EDCA enabled",
                    .rxMaxNss0_7 = 1,
                    .txMaxNss0_7 = 2,
                    .rxMaxNss8_9 = 3,
                    .txMaxNss8_9 = 4,
                    .rxMaxNss10_11 = 5,
                    .txMaxNss10_11 = 6,
                    .rxMaxNss12_13 = 7,
                    .txMaxNss12_13 = 8,
                    .pEdcaParams =
                        UhrOperation::PEdcaOpParams{
                            .eCWmin = 4,
                            .eCWmax = 10,
                            .aifsn = 3,
                            .cwDs = 2,
                            .psrcThreshold = 1,
                            .qsrcThreshold = 0,
                        },
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new UhrOperationElementTest({
                    .name = "DBE enabled, without DBE Operation Parameters",
                    .rxMaxNss0_7 = 1,
                    .txMaxNss0_7 = 2,
                    .rxMaxNss8_9 = 3,
                    .txMaxNss8_9 = 4,
                    .rxMaxNss10_11 = 5,
                    .txMaxNss10_11 = 6,
                    .rxMaxNss12_13 = 7,
                    .txMaxNss12_13 = 8,
                    .dbeParams =
                        UhrOperation::DbeOpParams{
                            .dBeBandwidth = 5,
                            .dbeDisabledSubchannelBitmap = 0x00FF,
                        },
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new UhrOperationElementTest({
                    .name = "All supported features enabled",
                    .rxMaxNss0_7 = 1,
                    .txMaxNss0_7 = 2,
                    .rxMaxNss8_9 = 3,
                    .txMaxNss8_9 = 4,
                    .rxMaxNss10_11 = 5,
                    .txMaxNss10_11 = 6,
                    .rxMaxNss12_13 = 7,
                    .txMaxNss12_13 = 8,
                    .npcaParams =
                        UhrOperation::NpcaOpParams{
                            .primaryChan = 6,
                            .minDurationThresh = 5,
                            .switchDelay = 20,
                            .switchBackDelay = 30,
                            .initialQsrc = 2,
                            .moplen = 1,
                            .disSubchanBmPresent = 1,
                            .disabledSubchBm = 0x0F0F,
                        },
                    .pEdcaParams =
                        UhrOperation::PEdcaOpParams{
                            .eCWmin = 2,
                            .eCWmax = 8,
                            .aifsn = 1,
                            .cwDs = 3,
                            .psrcThreshold = 1,
                            .qsrcThreshold = 2,
                        },
                    .dbeParams =
                        UhrOperation::DbeOpParams{
                            .dBeBandwidth = 4,
                            .dbeDisabledSubchannelBitmap = 0x000F,
                        },
                }),
                TestCase::Duration::QUICK);
}

static WifiUhrInfoElemsTestSuite g_wifiUhrInfoElemsTestSuite; ///< the test suite
