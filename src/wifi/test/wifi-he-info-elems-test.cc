/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/he-6ghz-band-capabilities.h"
#include "ns3/he-operation.h"
#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiHeInfoElemsTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test HE Operation information element serialization and deserialization
 */
class HeOperationElementTest : public HeaderSerializationTestCase
{
  public:
    HeOperationElementTest();

  private:
    void DoRun() override;
};

HeOperationElementTest::HeOperationElementTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of HE Operation elements")
{
}

void
HeOperationElementTest::DoRun()
{
    HeOperation heOperation;

    heOperation.m_heOpParams.m_defaultPeDuration = 6;
    heOperation.m_heOpParams.m_twtRequired = 1;
    heOperation.m_heOpParams.m_txopDurRtsThresh = 1000;
    heOperation.m_heOpParams.m_erSuDisable = 1;

    heOperation.m_bssColorInfo.m_bssColor = 44;
    heOperation.m_bssColorInfo.m_bssColorDisabled = 1;

    heOperation.SetMaxHeMcsPerNss(8, 7);
    heOperation.SetMaxHeMcsPerNss(6, 8);
    heOperation.SetMaxHeMcsPerNss(4, 9);
    heOperation.SetMaxHeMcsPerNss(2, 10);
    heOperation.SetMaxHeMcsPerNss(1, 11);

    TestHeaderSerialization(heOperation);

    heOperation.m_6GHzOpInfo = HeOperation::OpInfo6GHz{};
    heOperation.m_6GHzOpInfo->m_primCh = 191;
    heOperation.m_6GHzOpInfo->m_chWid = 2;
    heOperation.m_6GHzOpInfo->m_dupBeacon = 1;
    heOperation.m_6GHzOpInfo->m_regInfo = 6;
    heOperation.m_6GHzOpInfo->m_chCntrFreqSeg0 = 157;
    heOperation.m_6GHzOpInfo->m_chCntrFreqSeg1 = 201;
    heOperation.m_6GHzOpInfo->m_minRate = 211;

    TestHeaderSerialization(heOperation);
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test HE 6 GHz Band Capabilities information element serialization and deserialization
 */
class He6GhzBandCapabilitiesTest : public HeaderSerializationTestCase
{
  public:
    He6GhzBandCapabilitiesTest();

  private:
    void DoRun() override;
};

He6GhzBandCapabilitiesTest::He6GhzBandCapabilitiesTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of HE 6 GHz Band Capabilities elements")
{
}

void
He6GhzBandCapabilitiesTest::DoRun()
{
    He6GhzBandCapabilities he6GhzBandCapabilities;

    he6GhzBandCapabilities.m_capabilitiesInfo.m_minMpduStartSpacing = 5;
    he6GhzBandCapabilities.SetMaxAmpduLength((static_cast<uint32_t>(1) << 18) - 1);
    he6GhzBandCapabilities.SetMaxMpduLength(11454);
    he6GhzBandCapabilities.m_capabilitiesInfo.m_smPowerSave = 3;
    he6GhzBandCapabilities.m_capabilitiesInfo.m_rdResponder = 1;
    he6GhzBandCapabilities.m_capabilitiesInfo.m_rxAntennaPatternConsistency = 1;
    he6GhzBandCapabilities.m_capabilitiesInfo.m_txAntennaPatternConsistency = 1;

    TestHeaderSerialization(he6GhzBandCapabilities);
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi HE Information Elements Test Suite
 */
class WifiHeInfoElemsTestSuite : public TestSuite
{
  public:
    WifiHeInfoElemsTestSuite();
};

WifiHeInfoElemsTestSuite::WifiHeInfoElemsTestSuite()
    : TestSuite("wifi-he-info-elems", Type::UNIT)
{
    AddTestCase(new HeOperationElementTest, TestCase::Duration::QUICK);
    AddTestCase(new He6GhzBandCapabilitiesTest, TestCase::Duration::QUICK);
}

static WifiHeInfoElemsTestSuite g_wifiHeInfoElemsTestSuite; ///< the test suite
