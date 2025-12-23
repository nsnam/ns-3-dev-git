/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/simulator.h"
#include "ns3/smd-bss-transition-parameters.h"
#include "ns3/smd-information.h"
#include "ns3/uhr-capabilities.h"
#include "ns3/uhr-mode-change.h"
#include "ns3/uhr-operation.h"
#include "ns3/wifi-standard-constants.h"

#include <optional>
#include <utility>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiUhrInfoElemsTest");

namespace
{
/**
 * Get a SMD BSS Transition Parameters ST Info field for an ST preparation request with the given
 * parameters.
 *
 * @param listenInterval the Listen Interval field value
 * @param reqDlSnNotTransferred the Request DL SN Not Transferred field value
 * @param reqUlSnNotTransferred the Request UL SN Not Transferred field value
 * @param scsIds the SCS IDs to include in the SCS List field, or an empty vector if the SCS List
 * field should not be present
 * @return the SMD BSS Transition Parameters ST Info field
 */
SmdBssTransitionParameters::StPrepReqStInfo
GetStPrepReqStInfo(uint16_t listenInterval = 0,
                   bool reqDlSnNotTransferred = false,
                   bool reqUlSnNotTransferred = false,
                   const std::vector<uint8_t>& scsIds = {})
{
    SmdBssTransitionParameters::StPrepReqStInfo stInfo{};
    stInfo.m_listenInterval = listenInterval;
    stInfo.m_stControl.m_reqDlSnNotTransferred = reqDlSnNotTransferred ? 1 : 0;
    stInfo.m_stControl.m_reqUlSnNotTransferred = reqUlSnNotTransferred ? 1 : 0;
    if (!scsIds.empty())
    {
        stInfo.m_scsList = SmdBssTransitionParameters::ScsList{.m_scsIds = scsIds};
    }
    return stInfo;
}

/**
 * Get a SMD BSS Transition Parameters ST Info field for an ST preparation response with the given
 * parameters.
 *
 * @param dlExtBaParams a vector of pairs of Block Ack Parameter Set and ADDBA Extension Parameter
 * Set field values for DL BA Info, or an empty vector if the DL BA Info field should not be present
 * @param ulExtBaParams a vector of pairs of Block Ack Parameter Set and ADDBA Extension Parameter
 * Set field values for UL BA Info, or an empty vector if the UL BA Info field should not be present
 * @param scsIds the SCS IDs to include in the SCS List field, or an empty vector if the SCS List
 * field should not be present
 * @return the SMD BSS Transition Parameters ST Info field
 */
SmdBssTransitionParameters::StPrepRespStInfo
GetStPrepRespStInfo(const std::vector<std::pair<BlockAckParameterSet, AddbaExtension::ExtParamSet>>&
                        dlExtBaParams = {},
                    const std::vector<std::pair<BlockAckParameterSet, AddbaExtension::ExtParamSet>>&
                        ulExtBaParams = {},
                    const std::vector<uint8_t>& scsIds = {})
{
    SmdBssTransitionParameters::StPrepRespStInfo stInfo{};
    if (!dlExtBaParams.empty())
    {
        stInfo.m_dlBaInfo = SmdBssTransitionParameters::StPrepRespStInfo::BaInfo{};
    }
    if (!ulExtBaParams.empty())
    {
        stInfo.m_ulBaInfo = SmdBssTransitionParameters::StPrepRespStInfo::BaInfo{};
    }
    for (const auto& [baParameterSet, extParamSet] : dlExtBaParams)
    {
        stInfo.m_dlBaInfo->Set(baParameterSet.m_tid,
                               SmdBssTransitionParameters::StPrepRespStInfo::ExtBaParamsInfo{
                                   .m_baParameterSet = baParameterSet,
                                   .m_addBaExtParameterSet = extParamSet});
    }
    for (const auto& [baParameterSet, extParamSet] : ulExtBaParams)
    {
        stInfo.m_ulBaInfo->Set(baParameterSet.m_tid,
                               SmdBssTransitionParameters::StPrepRespStInfo::ExtBaParamsInfo{
                                   .m_baParameterSet = baParameterSet,
                                   .m_addBaExtParameterSet = extParamSet});
    }
    if (!scsIds.empty())
    {
        stInfo.m_scsList = SmdBssTransitionParameters::ScsList{.m_scsIds = scsIds};
    }
    return stInfo;
}

/**
 * Get a SMD BSS Execution Request ST Info field for an ST execution request with the given
 * parameters.
 *
 * @param dlTidBitmap the DL TID Bitmap field value, or std::nullopt if the DL TID Bitmap field
 * should not be present
 * @param dlExtBaParams a vector of pairs of Block Ack Parameter Set and ADDBA Extension Parameter
 * Set field values for DL BA Info, or an empty vector if the DL BA Info field should not be present
 * @return the SMD BSS Transition Parameters ST Info field
 */
SmdBssTransitionParameters::StExecReqStInfo
GetStExecReqStInfo(std::optional<uint8_t> dlTidBitmap = std::nullopt,
                   const std::vector<std::pair<BlockAckParameterSet, AddbaExtension::ExtParamSet>>&
                       dlExtBaParams = {})
{
    SmdBssTransitionParameters::StExecReqStInfo stInfo{};
    stInfo.m_dlTidBitmap = dlTidBitmap;
    if (!dlExtBaParams.empty())
    {
        stInfo.m_dlBaInfo = SmdBssTransitionParameters::StPrepRespStInfo::BaInfo{};
        for (const auto& [baParameterSet, extParamSet] : dlExtBaParams)
        {
            stInfo.m_dlBaInfo->Set(baParameterSet.m_tid,
                                   SmdBssTransitionParameters::StPrepRespStInfo::ExtBaParamsInfo{
                                       .m_baParameterSet = baParameterSet,
                                       .m_addBaExtParameterSet = extParamSet});
        }
    }
    return stInfo;
}

/**
 * Get a SMD BSS Execution Response ST Info field for an ST execution response with the given
 * parameters.
 *
 * @param nomMaxDlDrainPeriodDur the Nominal Maximum DL Draining Period Duration field value, or
 * std::nullopt if the Nominal Maximum DL Draining Period Duration field should not be present
 * @param latestUlSnPerTid a map of TID to Latest UL SN field values for the Latest UL SN List,
 * or an empty map if the Latest UL SN List field should not be present
 * @return the SMD BSS Transition Parameters ST Info field
 */
SmdBssTransitionParameters::StExecRespStInfo
GetStExecRespStInfo(std::optional<uint16_t> nomMaxDlDrainPeriodDur = std::nullopt,
                    const std::map<uint8_t, uint16_t>& latestUlSnPerTid = {})
{
    SmdBssTransitionParameters::StExecRespStInfo stInfo{};
    stInfo.m_nomMaxDlDrainPeriodDur = nomMaxDlDrainPeriodDur;
    if (!latestUlSnPerTid.empty())
    {
        stInfo.m_latestUlSnList = SmdBssTransitionParameters::StExecRespStInfo::LatestUlSnList{};
        for (const auto& [tid, sn] : latestUlSnPerTid)
        {
            stInfo.m_latestUlSnList->Set(tid, sn);
        }
    }
    return stInfo;
}

} // namespace

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
        std::optional<UhrCapabilities::DbeCapabilityParameters>
            dbeCapabilityParams; ///< DBE Capability Parameters field
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
    m_uhrCapabilities.m_dbeCapabilityParams = params.dbeCapabilityParams;
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
        std::optional<UhrOperation::DuoOpParams>
            duoParams; ///< Optional DUO Operation Parameters field
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
    m_uhrOperation.m_duoParams = params.duoParams;
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
 * @brief Test UHR Mode Change information element serialization and deserialization
 */
class UhrModeChangeElementTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Parameters for UhrModeChangeElementTest
     */
    struct TestParams
    {
        std::string name;                                 ///< Test case name
        std::vector<UhrModeChange::ModeTuple> modeTuples; ///< Mode Tuples
    };

    /**
     * Constructor
     *
     * @param params Test parameters
     */
    UhrModeChangeElementTest(const TestParams& params);

  private:
    void DoRun() override;

    UhrModeChange m_uhrModeChange; ///< UHR Mode Change element
};

UhrModeChangeElementTest::UhrModeChangeElementTest(const TestParams& params)
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of UHR Mode Change elements: " + params.name)
{
    m_uhrModeChange.m_modeTuples = params.modeTuples;
}

void
UhrModeChangeElementTest::DoRun()
{
    auto deserIe = TestHeaderSerialization(m_uhrModeChange);
    NS_TEST_EXPECT_MSG_EQ(deserIe.m_modeTuples.size(),
                          m_uhrModeChange.m_modeTuples.size(),
                          "Number of Mode Tuples differs after serialization and deserialization");
    for (std::size_t i = 0; i < m_uhrModeChange.m_modeTuples.size(); ++i)
    {
        const auto& origModeTuple = m_uhrModeChange.m_modeTuples[i];
        const auto& deserModeTuple = deserIe.m_modeTuples[i];
        NS_TEST_EXPECT_MSG_EQ(+deserModeTuple.modeId,
                              +origModeTuple.modeId,
                              "Mode ID field of Mode Tuple "
                                  << i << " differs after serialization and deserialization");
        NS_TEST_EXPECT_MSG_EQ(+deserModeTuple.modeEnable,
                              +origModeTuple.modeEnable,
                              "Mode Enable field of Mode Tuple "
                                  << i << " differs after serialization and deserialization");
        NS_TEST_EXPECT_MSG_EQ(+deserModeTuple.modeUpdate,
                              +origModeTuple.modeUpdate,
                              "Mode Update field of Mode Tuple "
                                  << i << " differs after serialization and deserialization");
        const auto expectHasModeLengthAndParams =
            (origModeTuple.modeEnable == 1) &&
            ((origModeTuple.modeId == UhrMode::DPS) || (origModeTuple.modeId == UhrMode::NPCA) ||
             (origModeTuple.modeId == UhrMode::DSO) || (origModeTuple.modeId == UhrMode::EMLSR));
        NS_TEST_EXPECT_MSG_EQ(deserModeTuple.modeLength.has_value(),
                              expectHasModeLengthAndParams,
                              "Mode Length field presence of Mode Tuple "
                                  << i << " is expected to be "
                                  << (expectHasModeLengthAndParams ? "present" : "not present"));
        NS_TEST_EXPECT_MSG_EQ(deserModeTuple.modeParams.has_value(),
                              expectHasModeLengthAndParams,
                              "Mode Specific Parameters field presence of Mode Tuple "
                                  << i << " is expected to be "
                                  << (expectHasModeLengthAndParams ? "present" : "not present"));
        if (!expectHasModeLengthAndParams)
        {
            continue;
        }
        const auto& origModeParams = origModeTuple.modeParams.value();
        const auto& deserModeParams = deserModeTuple.modeParams.value();
        NS_TEST_EXPECT_MSG_EQ(deserModeParams.index(),
                              origModeParams.index(),
                              "Mode Specific Parameters type of Mode Tuple "
                                  << i << " differs after serialization and deserialization");
        switch (origModeTuple.modeId)
        {
        case UhrMode::DPS: {
            const auto& origDpsParams = std::get<UhrModeChange::DpsParams>(origModeParams);
            const auto& deserDpsParams = std::get<UhrModeChange::DpsParams>(deserModeParams);
            NS_TEST_EXPECT_MSG_EQ(
                deserDpsParams.paddingDelay,
                origDpsParams.paddingDelay,
                "Padding Delay field of DPS Mode Specific Parameters in Mode Tuple "
                    << i << " differs after serialization and deserialization");
            NS_TEST_EXPECT_MSG_EQ(
                deserDpsParams.transitionDelay,
                origDpsParams.transitionDelay,
                "Transition Delay field of DPS Mode Specific Parameters in Mode Tuple "
                    << i << " differs after serialization and deserialization");
            NS_TEST_EXPECT_MSG_EQ(
                deserDpsParams.icfRequired,
                origDpsParams.icfRequired,
                "ICF Required field of DPS Mode Specific Parameters in Mode Tuple "
                    << i << " differs after serialization and deserialization");
            break;
        }
        case UhrMode::EMLSR: {
            const auto& origEmlsrParams = std::get<UhrModeChange::EmlsrParams>(origModeParams);
            const auto& deserEmlsrParams = std::get<UhrModeChange::EmlsrParams>(deserModeParams);
            NS_TEST_EXPECT_MSG_EQ(
                deserEmlsrParams.paddingDelay,
                origEmlsrParams.paddingDelay,
                "Padding Delay field of EMLSR Mode Specific Parameters in Mode Tuple "
                    << i << " differs after serialization and deserialization");
            NS_TEST_EXPECT_MSG_EQ(
                deserEmlsrParams.transitionDelay,
                origEmlsrParams.transitionDelay,
                "Transition Delay field of EMLSR Mode Specific Parameters in Mode Tuple "
                    << i << " differs after serialization and deserialization");
            NS_TEST_EXPECT_MSG_EQ(
                deserEmlsrParams.inDevCoexAct,
                origEmlsrParams.inDevCoexAct,
                "In-device Coexistence Activity field of EMLSR Mode Specific Parameters in "
                "Mode Tuple "
                    << i << " differs after serialization and deserialization");
            break;
        }
        case UhrMode::NPCA: {
            const auto& origNpcaParams = std::get<UhrModeChange::NpcaParams>(origModeParams);
            const auto& deserNpcaParams = std::get<UhrModeChange::NpcaParams>(deserModeParams);
            NS_TEST_EXPECT_MSG_EQ(
                deserNpcaParams.switchingDelay,
                origNpcaParams.switchingDelay,
                "Switching Delay field of NPCA Mode Specific Parameters in Mode Tuple "
                    << i << " differs after serialization and deserialization");
            NS_TEST_EXPECT_MSG_EQ(
                deserNpcaParams.switchBackDelay,
                origNpcaParams.switchBackDelay,
                "Switch Back Delay field of NPCA Mode Specific Parameters in Mode Tuple "
                    << i << " differs after serialization and deserialization");
            break;
        }
        case UhrMode::DSO: {
            const auto& origDsoParams = std::get<UhrModeChange::DsoParams>(origModeParams);
            const auto& deserDsoParams = std::get<UhrModeChange::DsoParams>(deserModeParams);
            NS_TEST_EXPECT_MSG_EQ(
                deserDsoParams.switchBackDelay,
                origDsoParams.switchBackDelay,
                "Switch Back Delay field of DSO Mode Specific Parameters in Mode Tuple "
                    << i << " differs after serialization and deserialization");
            NS_TEST_EXPECT_MSG_EQ(
                deserDsoParams.paddingDelay,
                origDsoParams.paddingDelay,
                "Padding Delay field of DSO Mode Specific Parameters in Mode Tuple "
                    << i << " differs after serialization and deserialization");
            NS_TEST_EXPECT_MSG_EQ(
                deserDsoParams.preferred80MHzSubband,
                origDsoParams.preferred80MHzSubband,
                "Preferred 80 MHz Subband field of DSO Mode Specific Parameters in Mode Tuple "
                    << i << " differs after serialization and deserialization");
            break;
        }
        default:
            NS_ASSERT_MSG(
                false,
                "Unexpected Mode Specific Parameters type index: " << deserModeParams.index());
            break;
        }
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test SMD Information element serialization and deserialization
 */
class SmdInformationElementTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Parameters for SmdInformationElementTest
     */
    struct TestParams
    {
        std::string name;                                ///< Test case name
        Mac48Address smdIdentifier;                      ///< SMD Identifier field
        SmdInformation::SmdCapabilities smdCapabilities; ///< SMD Capabilities field
        Time preparationTimeout;                         ///< Preparation timeout field
    };

    /**
     * Constructor
     *
     * @param params Test parameters
     */
    SmdInformationElementTest(const TestParams& params);

  private:
    void DoRun() override;

    SmdInformation m_smdInformation; ///< SMD Information element
};

SmdInformationElementTest::SmdInformationElementTest(const TestParams& params)
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of SMD Information elements: " + params.name)
{
    m_smdInformation.m_smdIdentifier = params.smdIdentifier;
    m_smdInformation.m_smdCapabilities = params.smdCapabilities;
    m_smdInformation.SetPrepTimeout(params.preparationTimeout);
}

void
SmdInformationElementTest::DoRun()
{
    auto deserIe = TestHeaderSerialization(m_smdInformation);
    NS_TEST_EXPECT_MSG_EQ(deserIe.m_smdIdentifier,
                          m_smdInformation.m_smdIdentifier,
                          "SMD Identifier field differs after serialization and deserialization");
    NS_TEST_EXPECT_MSG_EQ(
        +deserIe.m_smdCapabilities.dlDataForwarding,
        +m_smdInformation.m_smdCapabilities.dlDataForwarding,
        "DL Data Forwarding field differs after serialization and deserialization");
    NS_TEST_EXPECT_MSG_EQ(+deserIe.m_smdCapabilities.maxNumOfPrepTargetApMlds,
                          +m_smdInformation.m_smdCapabilities.maxNumOfPrepTargetApMlds,
                          "Max Number of Prepared Target AP MLDs field differs after serialization "
                          "and deserialization");
    NS_TEST_EXPECT_MSG_EQ(+deserIe.m_smdCapabilities.smdType,
                          +m_smdInformation.m_smdCapabilities.smdType,
                          "SMD Type field differs after serialization and deserialization");
    NS_TEST_EXPECT_MSG_EQ(+deserIe.m_smdCapabilities.ptkMode,
                          +m_smdInformation.m_smdCapabilities.ptkMode,
                          "PTK Mode field differs after serialization and deserialization");
    NS_TEST_EXPECT_MSG_EQ(deserIe.GetPrepTimeout(),
                          m_smdInformation.GetPrepTimeout(),
                          "Preparation timeout field differs after serialization and "
                          "deserialization");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test SMD BSS Transition Parameters element serialization and deserialization
 */
class SmdBssTransitionParametersElementTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Parameters for SmdBssTransitionParametersElementTest
     */
    struct TestParams
    {
        std::string name;                          ///< Test case name
        SmdBssTransitionParameters::StInfo stInfo; ///< ST Info field
    };

    /**
     * Constructor
     *
     * @param params Test parameters
     */
    SmdBssTransitionParametersElementTest(const TestParams& params);

  private:
    void DoRun() override;

    TestParams m_params; ///< Test parameters
};

SmdBssTransitionParametersElementTest::SmdBssTransitionParametersElementTest(
    const TestParams& params)
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of SMD BSS Transition Parameters elements: " +
          params.name),
      m_params{params}
{
}

void
SmdBssTransitionParametersElementTest::DoRun()
{
    auto smdBssTransitionParameters = SmdBssTransitionParameters(m_params.stInfo);
    auto deserHdr = TestHeaderSerialization(smdBssTransitionParameters, m_params.stInfo);

    NS_TEST_EXPECT_MSG_EQ(deserHdr.m_stInfo.index(),
                          smdBssTransitionParameters.m_stInfo.index(),
                          "ST Info field type differs after serialization and deserialization");

    switch (smdBssTransitionParameters.m_stInfo.index())
    {
    case 1: // StPrepReqStInfo
    {
        auto& origStInfo = std::get<SmdBssTransitionParameters::StPrepReqStInfo>(
            smdBssTransitionParameters.m_stInfo);
        auto& deserStInfo =
            std::get<SmdBssTransitionParameters::StPrepReqStInfo>(deserHdr.m_stInfo);
        NS_TEST_EXPECT_MSG_EQ(
            deserStInfo.m_listenInterval,
            origStInfo.m_listenInterval,
            "Listen Interval field differs after serialization and deserialization");
        NS_TEST_EXPECT_MSG_EQ(
            deserStInfo.m_stControl.m_reqDlSnNotTransferred,
            origStInfo.m_stControl.m_reqDlSnNotTransferred,
            "Request DL SN Not Transferred field differs after serialization and deserialization");
        NS_TEST_EXPECT_MSG_EQ(
            deserStInfo.m_stControl.m_reqUlSnNotTransferred,
            origStInfo.m_stControl.m_reqUlSnNotTransferred,
            "Request UL SN Not Transferred field differs after serialization and deserialization");
        NS_TEST_ASSERT_MSG_EQ(deserStInfo.m_scsList.has_value(),
                              origStInfo.m_scsList.has_value(),
                              "SCS List presence differs after serialization and deserialization");
        if (origStInfo.m_scsList.has_value())
        {
            NS_TEST_ASSERT_MSG_EQ(deserStInfo.m_scsList->m_scsIds.size(),
                                  origStInfo.m_scsList->m_scsIds.size(),
                                  "Number Of SCS IDs differs after serialization and "
                                  "deserialization");
            for (std::size_t i = 0; i < origStInfo.m_scsList->m_scsIds.size(); ++i)
            {
                NS_TEST_EXPECT_MSG_EQ(
                    +deserStInfo.m_scsList->m_scsIds[i],
                    +origStInfo.m_scsList->m_scsIds[i],
                    "SCS ID at index " << i << " differs after serialization and deserialization");
            }
        }
        break;
    }
    case 2: // StPrepRespStInfo
    {
        auto& origStInfo = std::get<SmdBssTransitionParameters::StPrepRespStInfo>(
            smdBssTransitionParameters.m_stInfo);
        auto& deserStInfo =
            std::get<SmdBssTransitionParameters::StPrepRespStInfo>(deserHdr.m_stInfo);
        NS_TEST_ASSERT_MSG_EQ(
            deserStInfo.m_dlBaInfo.has_value(),
            origStInfo.m_dlBaInfo.has_value(),
            "DL BA Info presence differs after serialization and deserialization");
        if (origStInfo.m_dlBaInfo.has_value())
        {
            NS_TEST_ASSERT_MSG_EQ(+deserStInfo.m_dlBaInfo->GetTidBitmap(),
                                  +origStInfo.m_dlBaInfo->GetTidBitmap(),
                                  "DL BA Info TID Bitmap field differs after serialization and "
                                  "deserialization");
            const auto& deserDlExtBaParamsInfos = deserStInfo.m_dlBaInfo->GetExtBaParamsInfos();
            const auto& origDlExtBaParamsInfos = origStInfo.m_dlBaInfo->GetExtBaParamsInfos();
            NS_TEST_ASSERT_MSG_EQ(deserDlExtBaParamsInfos.size(),
                                  origDlExtBaParamsInfos.size(),
                                  "DL BA Info Extended BA Parameters Infos size differs after "
                                  "serialization and deserialization");
            for (const auto& [tid, info] : origDlExtBaParamsInfos)
            {
                NS_TEST_EXPECT_MSG_EQ(
                    deserDlExtBaParamsInfos.at(tid).m_baParameterSet,
                    info.m_baParameterSet,
                    "DL BA Info Extended BA Parameters Infos BA Parameter Set entry differs for "
                    "TID "
                        << +tid);
                NS_TEST_EXPECT_MSG_EQ(
                    deserDlExtBaParamsInfos.at(tid).m_addBaExtParameterSet,
                    info.m_addBaExtParameterSet,
                    "DL BA Info Extended BA Parameters Infos Additional BA Extended Parameter Set "
                    "entry differs for TID "
                        << +tid);
            }
        }
        NS_TEST_ASSERT_MSG_EQ(
            deserStInfo.m_ulBaInfo.has_value(),
            origStInfo.m_ulBaInfo.has_value(),
            "UL BA Info presence differs after serialization and deserialization");
        if (origStInfo.m_ulBaInfo.has_value())
        {
            NS_TEST_ASSERT_MSG_EQ(+deserStInfo.m_ulBaInfo->GetTidBitmap(),
                                  +origStInfo.m_ulBaInfo->GetTidBitmap(),
                                  "UL BA Info TID Bitmap field differs after serialization and "
                                  "deserialization");
            const auto& deserUlExtBaParamsInfos = deserStInfo.m_ulBaInfo->GetExtBaParamsInfos();
            const auto& origUlExtBaParamsInfos = origStInfo.m_ulBaInfo->GetExtBaParamsInfos();
            NS_TEST_ASSERT_MSG_EQ(deserUlExtBaParamsInfos.size(),
                                  origUlExtBaParamsInfos.size(),
                                  "UL BA Info Extended BA Parameters Infos size differs after "
                                  "serialization and deserialization");
            for (const auto& [tid, info] : origUlExtBaParamsInfos)
            {
                NS_TEST_EXPECT_MSG_EQ(
                    deserUlExtBaParamsInfos.at(tid).m_baParameterSet,
                    info.m_baParameterSet,
                    "UL BA Info Extended BA Parameters Infos BA Parameter Set entry differs for "
                    "TID "
                        << +tid);
                NS_TEST_EXPECT_MSG_EQ(
                    deserUlExtBaParamsInfos.at(tid).m_addBaExtParameterSet,
                    info.m_addBaExtParameterSet,
                    "UL BA Info Extended BA Parameters Infos Additional BA Extended Parameter Set "
                    "entry differs for TID "
                        << +tid);
            }
        }
        NS_TEST_ASSERT_MSG_EQ(deserStInfo.m_scsList.has_value(),
                              origStInfo.m_scsList.has_value(),
                              "SCS List presence differs after serialization and deserialization");
        if (origStInfo.m_scsList.has_value())
        {
            NS_TEST_ASSERT_MSG_EQ(deserStInfo.m_scsList->m_scsIds.size(),
                                  origStInfo.m_scsList->m_scsIds.size(),
                                  "Number Of SCS IDs differs after serialization and "
                                  "deserialization");
            for (std::size_t i = 0; i < origStInfo.m_scsList->m_scsIds.size(); ++i)
            {
                NS_TEST_EXPECT_MSG_EQ(
                    +deserStInfo.m_scsList->m_scsIds[i],
                    +origStInfo.m_scsList->m_scsIds[i],
                    "SCS ID at index " << i << " differs after serialization and deserialization");
            }
        }
        break;
    }
    case 3: // StExecReqStInfo
    {
        auto& origStInfo = std::get<SmdBssTransitionParameters::StExecReqStInfo>(
            smdBssTransitionParameters.m_stInfo);
        auto& deserStInfo =
            std::get<SmdBssTransitionParameters::StExecReqStInfo>(deserHdr.m_stInfo);
        NS_TEST_ASSERT_MSG_EQ(
            deserStInfo.m_dlTidBitmap.has_value(),
            origStInfo.m_dlTidBitmap.has_value(),
            "DL TID Bitmap presence differs after serialization and deserialization");
        if (origStInfo.m_dlTidBitmap.has_value())
        {
            NS_TEST_EXPECT_MSG_EQ(
                +*deserStInfo.m_dlTidBitmap,
                +*origStInfo.m_dlTidBitmap,
                "DL TID Bitmap field differs after serialization and deserialization");
        }
        NS_TEST_ASSERT_MSG_EQ(
            deserStInfo.m_dlBaInfo.has_value(),
            origStInfo.m_dlBaInfo.has_value(),
            "DL BA Info presence differs after serialization and deserialization");
        if (origStInfo.m_dlBaInfo.has_value())
        {
            NS_TEST_ASSERT_MSG_EQ(+deserStInfo.m_dlBaInfo->GetTidBitmap(),
                                  +origStInfo.m_dlBaInfo->GetTidBitmap(),
                                  "DL BA Info TID Bitmap field differs after serialization and "
                                  "deserialization");
            const auto& deserDlExtBaParamsInfos = deserStInfo.m_dlBaInfo->GetExtBaParamsInfos();
            const auto& origDlExtBaParamsInfos = origStInfo.m_dlBaInfo->GetExtBaParamsInfos();
            NS_TEST_ASSERT_MSG_EQ(deserDlExtBaParamsInfos.size(),
                                  origDlExtBaParamsInfos.size(),
                                  "DL BA Info Extended BA Parameters Infos size differs after "
                                  "serialization and deserialization");
            for (const auto& [tid, info] : origDlExtBaParamsInfos)
            {
                NS_TEST_EXPECT_MSG_EQ(
                    deserDlExtBaParamsInfos.at(tid).m_baParameterSet,
                    info.m_baParameterSet,
                    "DL BA Info Extended BA Parameters Infos BA Parameter Set entry differs for "
                    "TID "
                        << +tid);
                NS_TEST_EXPECT_MSG_EQ(
                    deserDlExtBaParamsInfos.at(tid).m_addBaExtParameterSet,
                    info.m_addBaExtParameterSet,
                    "DL BA Info Extended BA Parameters Infos Additional BA Extended Parameter Set "
                    "entry differs for TID "
                        << +tid);
            }
        }
        break;
    }
    case 4: // StExecRespStInfo
    {
        auto& origStInfo = std::get<SmdBssTransitionParameters::StExecRespStInfo>(
            smdBssTransitionParameters.m_stInfo);
        auto& deserStInfo =
            std::get<SmdBssTransitionParameters::StExecRespStInfo>(deserHdr.m_stInfo);
        NS_TEST_ASSERT_MSG_EQ(deserStInfo.m_nomMaxDlDrainPeriodDur.has_value(),
                              origStInfo.m_nomMaxDlDrainPeriodDur.has_value(),
                              "Nominal Maximum DL Draining Period Duration presence differs "
                              "after serialization and deserialization");
        if (origStInfo.m_nomMaxDlDrainPeriodDur.has_value())
        {
            NS_TEST_EXPECT_MSG_EQ(*deserStInfo.m_nomMaxDlDrainPeriodDur,
                                  *origStInfo.m_nomMaxDlDrainPeriodDur,
                                  "Nominal Maximum DL Draining Period Duration field differs "
                                  "after serialization and deserialization");
        }
        NS_TEST_ASSERT_MSG_EQ(
            deserStInfo.m_latestUlSnList.has_value(),
            origStInfo.m_latestUlSnList.has_value(),
            "Latest UL SN List presence differs after serialization and deserialization");
        if (origStInfo.m_latestUlSnList.has_value())
        {
            NS_TEST_EXPECT_MSG_EQ(
                +deserStInfo.m_latestUlSnList->GetTidBitmap(),
                +origStInfo.m_latestUlSnList->GetTidBitmap(),
                "TID Bitmap field differs after serialization and deserialization");
            const auto& deserLatestUlSnPerTid = deserStInfo.m_latestUlSnList->GetLatestUlSnPerTid();
            const auto& origLatestUlSnPerTid = origStInfo.m_latestUlSnList->GetLatestUlSnPerTid();
            NS_TEST_ASSERT_MSG_EQ(
                deserLatestUlSnPerTid.size(),
                origLatestUlSnPerTid.size(),
                "Latest UL SN per TID size differs after serialization and deserialization");
            for (const auto& [tid, sn] : origLatestUlSnPerTid)
            {
                NS_TEST_EXPECT_MSG_EQ(deserLatestUlSnPerTid.at(tid),
                                      sn,
                                      "Latest UL SN for TID "
                                          << tid
                                          << " after "
                                             "serialization and deserialization");
            }
        }
        break;
    }
    default:
        NS_ASSERT_MSG(false,
                      "Invalid ST Info type index " << smdBssTransitionParameters.m_stInfo.index());
        break;
    }
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
                    .name = "MAC and PHY capabilities without DBE",
                    .macCapabilities =
                        UhrCapabilities::UhrMacCapabilities{
                            .dsoSupport = 1,
                        },
                    .phyCapabilities =
                        UhrCapabilities::UhrPhyCapabilities{
                            .maxNssRxForSoundingNdpLe80Mhz = 1,
                            .maxNssRxForSoundingNdp320Mhz = 1,
                            .partialBwUlMuMimoSupport = 1,
                            .mcs15Support = 1,
                            .ueqmTxSupportAndMaxNssTx = 2,
                            .ueqmRxSupportAndMaxNssRx = 1,
                            .coSrMode1Support = 1,
                            .druDbw80InPbw160MhzSupport = 1,
                            .druRruHybridModeSupport = 1,
                        },
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new UhrCapabilitiesElementTest({
                    .name = "UHR MAC Capabilities with all features enabled",
                    .macCapabilities =
                        UhrCapabilities::UhrMacCapabilities{
                            .dsoSupport = 1,
                            .dbeSupport = 1,
                        },
                    .phyCapabilities =
                        UhrCapabilities::UhrPhyCapabilities{
                            .maxNssRxForSoundingNdpLe80Mhz = 1,
                            .maxNssRxForSoundingNdp320Mhz = 1,
                            .partialBwUlMuMimoSupport = 1,
                            .mcs15Support = 1,
                            .ueqmTxSupportAndMaxNssTx = 2,
                            .ueqmRxSupportAndMaxNssRx = 1,
                            .coSrMode1Support = 1,
                            .druDbw80InPbw160MhzSupport = 1,
                            .druRruHybridModeSupport = 1,
                        },
                    .dbeCapabilityParams =
                        UhrCapabilities::DbeCapabilityParameters{
                            .maxSupportedDbeBandwidth = 4,
                            .dbeCapabilities160MhzPresent = 1,
                            .dbeCapabilities320MhzPresent = 1,
                            .maxDbeBandwidthSwitchTimePeriod = 100,
                            .dbeModeChangeInterval = 15,
                            .dbeCapabilities160Mhz =
                                UhrCapabilities::DbeBandwidthCapabilities{
                                    .ehtMcsMap = {0x01, 0x02, 0x03},
                                    .nSoundingDimensions = 2,
                                    .nonOfdmaUlMuMimo = 1,
                                    .muBeamformer = 1,
                                    .beamformeeSs = 4,
                                },
                            .dbeCapabilities320Mhz =
                                UhrCapabilities::DbeBandwidthCapabilities{
                                    .ehtMcsMap = {0x11, 0x22, 0x33},
                                    .nSoundingDimensions = 3,
                                    .nonOfdmaUlMuMimo = 0,
                                    .muBeamformer = 1,
                                    .beamformeeSs = 5,
                                },
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
                            .primaryChan = 106,
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
                            .eCWmin = 2,
                            .eCWmax = 3,
                            .aifsn = 2,
                            .cwDs = 2,
                            .psrcThreshold = 4,
                            .qsrcThreshold = 3,
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
                            .dBeBandwidth = 2,
                            .disSubchanBmPresent = 0,
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
                            .eCWmax = 3,
                            .aifsn = 1,
                            .cwDs = 2,
                            .psrcThreshold = 1,
                            .qsrcThreshold = 2,
                        },
                    .dbeParams =
                        UhrOperation::DbeOpParams{
                            .dBeBandwidth = 4,
                            .disSubchanBmPresent = 1,
                            .disabledSubchBm = 0x000F,
                        },
                    .duoParams =
                        UhrOperation::DuoOpParams{
                            .maxStandaloneDuoBsrp = 31,
                        },
                }),
                TestCase::Duration::QUICK);
    AddTestCase(new UhrModeChangeElementTest({
                    .name = "UHR Mode Change to enable DPS",
                    .modeTuples =
                        {
                            UhrModeChange::ModeTuple{
                                .modeId = UhrMode::DPS,
                                .modeEnable = 1,
                                .modeLength = WIFI_UHR_MODE_CHANGE_DPS_PARAMS_SIZE_B,
                                .modeParams =
                                    UhrModeChange::DpsParams{
                                        .paddingDelay = 10,
                                        .transitionDelay = 20,
                                        .icfRequired = 1,
                                    },
                            },
                        },
                }),
                TestCase::Duration::QUICK);
    AddTestCase(new UhrModeChangeElementTest({
                    .name = "UHR Mode Change to disable NPCA",
                    .modeTuples =
                        {
                            UhrModeChange::ModeTuple{
                                .modeId = UhrMode::NPCA,
                                .modeEnable = 0,
                            },
                        },
                }),
                TestCase::Duration::QUICK);
    AddTestCase(new UhrModeChangeElementTest({
                    .name = "UHR Mode Change to disable EMLSR",
                    .modeTuples =
                        {
                            UhrModeChange::ModeTuple{
                                .modeId = UhrMode::EMLSR,
                                .modeEnable = 0,
                                .modeLength = WIFI_UHR_MODE_CHANGE_EMLSR_PARAMS_SIZE_B,
                                .modeParams = // Even though parameters should be ignored when
                                              // disabling a mode, this verifies that the presence
                                              // of parameters in this case does not cause issues
                                              // with serialization and deserialization
                                UhrModeChange::EmlsrParams{},
                            },
                        },
                }),
                TestCase::Duration::QUICK);
    AddTestCase(new UhrModeChangeElementTest({
                    .name = "UHR Mode Change to update NPCA",
                    .modeTuples =
                        {
                            UhrModeChange::ModeTuple{
                                .modeId = UhrMode::NPCA,
                                .modeEnable = 1,
                                .modeUpdate = 1,
                                .modeLength = WIFI_UHR_MODE_CHANGE_NPCA_PARAMS_SIZE_B,
                                .modeParams =
                                    UhrModeChange::NpcaParams{
                                        .switchingDelay = 20,
                                        .switchBackDelay = 30,
                                    },
                            },
                        },
                }),
                TestCase::Duration::QUICK);
    AddTestCase(new UhrModeChangeElementTest({
                    .name = "UHR Mode Change to enable DSO, to update EMLSR and to disable DPS",
                    .modeTuples =
                        {
                            UhrModeChange::ModeTuple{
                                .modeId = UhrMode::DSO,
                                .modeEnable = 1,
                                .modeLength = WIFI_UHR_MODE_CHANGE_DSO_PARAMS_SIZE_B,
                                .modeParams =
                                    UhrModeChange::DsoParams{
                                        .paddingDelay = 15,
                                        .switchBackDelay = 25,
                                        .preferred80MHzSubband = 1,
                                    },
                            },
                            UhrModeChange::ModeTuple{
                                .modeId = UhrMode::EMLSR,
                                .modeEnable = 1,
                                .modeUpdate = 1,
                                .modeLength = WIFI_UHR_MODE_CHANGE_EMLSR_PARAMS_SIZE_B,
                                .modeParams =
                                    UhrModeChange::EmlsrParams{
                                        .linkBm = 0x0037,
                                        .paddingDelay = 5,
                                        .transitionDelay = 15,
                                        .inDevCoexAct = 1,
                                    },
                            },
                            UhrModeChange::ModeTuple{
                                .modeId = UhrMode::DPS,
                                .modeEnable = 0,
                            },
                        },
                }),
                TestCase::Duration::QUICK);
    AddTestCase(new SmdInformationElementTest({
                    .name = "SMD Information element test",
                    .smdIdentifier = Mac48Address("AA:BB:CC:DD:EE:FF"),
                    .smdCapabilities =
                        SmdInformation::SmdCapabilities{
                            .dlDataForwarding = 1,
                            .maxNumOfPrepTargetApMlds = 2,
                            .neighboringApProbingSupport = 1,
                        },
                    .preparationTimeout = 200 * 64 * WIFI_TU,
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Req indicating both DL and UL SNs transferred",
                    .stInfo = GetStPrepReqStInfo(100, false, false),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Req indicating DL SN not transferred",
                    .stInfo = GetStPrepReqStInfo(200, true, false),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Req indicating UL SN not transferred",
                    .stInfo = GetStPrepReqStInfo(300, false, true),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Req indicating both DL and UL SNs not transferred",
                    .stInfo = GetStPrepReqStInfo(400, true, true),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Req with single SCS ID",
                    .stInfo = GetStPrepReqStInfo(500, false, false, {1}),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Req with multiple SCS IDs in decreasing priority order",
                    .stInfo = GetStPrepReqStInfo(600, true, true, {7, 3, 1}),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Resp without optional fields",
                    .stInfo = GetStPrepRespStInfo(),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Resp with DL BA Agreements",
                    .stInfo = GetStPrepRespStInfo({
                        {BlockAckParameterSet{.m_tid = 1, .m_bufferSize = 64},
                         AddbaExtension::ExtParamSet{}},
                        {BlockAckParameterSet{.m_tid = 4, .m_bufferSize = 256},
                         AddbaExtension::ExtParamSet{}},
                        {BlockAckParameterSet{.m_tid = 7, .m_bufferSize = 512},
                         AddbaExtension::ExtParamSet{}},
                    }),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Resp with UL BA Agreements",
                    .stInfo = GetStPrepRespStInfo(
                        {},
                        {
                            {BlockAckParameterSet{},
                             AddbaExtension::ExtParamSet{.extBufferSize = 1 /* 1024 */}},
                            {BlockAckParameterSet{.m_tid = 3, .m_bufferSize = 511},
                             AddbaExtension::ExtParamSet{}},
                            {BlockAckParameterSet{.m_tid = 5, .m_bufferSize = 255},
                             AddbaExtension::ExtParamSet{}},
                        }),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Resp with both DL and UL BA Agreements",
                    .stInfo = GetStPrepRespStInfo(
                        {
                            {BlockAckParameterSet{.m_tid = 2, .m_bufferSize = 64},
                             AddbaExtension::ExtParamSet{}},
                            {BlockAckParameterSet{.m_tid = 4, .m_bufferSize = 256},
                             AddbaExtension::ExtParamSet{}},
                        },
                        {{BlockAckParameterSet{.m_tid = 4, .m_bufferSize = 0},
                          AddbaExtension::ExtParamSet{.extBufferSize = 1 /* 1024 */}},
                         {BlockAckParameterSet{.m_tid = 7, .m_bufferSize = 512},
                          AddbaExtension::ExtParamSet{}}}),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Resp with accepted SCS IDs",
                    .stInfo = GetStPrepRespStInfo({}, {}, {7, 3, 1}),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Prep Resp with BA Agreements and accepted SCS IDs",
                    .stInfo = GetStPrepRespStInfo(
                        {
                            {BlockAckParameterSet{.m_tid = 2, .m_bufferSize = 64},
                             AddbaExtension::ExtParamSet{}},
                        },
                        {
                            {BlockAckParameterSet{.m_tid = 5, .m_bufferSize = 128},
                             AddbaExtension::ExtParamSet{}},
                        },
                        {4, 2}),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Exec Req without optional fields",
                    .stInfo = GetStExecReqStInfo(),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Exec Req with DL TID Bitmap",
                    .stInfo = GetStExecReqStInfo(0xAB),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(
        new SmdBssTransitionParametersElementTest({
            .name = "ST Exec Req with DL BA Info",
            .stInfo = GetStExecReqStInfo(std::nullopt,
                                         {
                                             {BlockAckParameterSet{.m_tid = 1, .m_bufferSize = 64},
                                              AddbaExtension::ExtParamSet{}},
                                             {BlockAckParameterSet{.m_tid = 5, .m_bufferSize = 256},
                                              AddbaExtension::ExtParamSet{}},
                                         }),
        }),
        TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Exec Req with DL TID Bitmap and DL BA Info",
                    .stInfo = GetStExecReqStInfo(
                        0x12,
                        {
                            {BlockAckParameterSet{.m_tid = 2, .m_bufferSize = 128},
                             AddbaExtension::ExtParamSet{}},
                            {BlockAckParameterSet{.m_tid = 4, .m_bufferSize = 0},
                             AddbaExtension::ExtParamSet{.extBufferSize = 1 /* 1024 */}},
                        }),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Exec Resp without optional fields",
                    .stInfo = GetStExecRespStInfo(),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Exec Resp with Nominal Maximum DL Draining Period Duration",
                    .stInfo = GetStExecRespStInfo(500),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Exec Resp with Latest UL SN List",
                    .stInfo = GetStExecRespStInfo(std::nullopt, {{0, 10}, {3, 20}, {6, 30}}),
                }),
                TestCase::Duration::QUICK);

    AddTestCase(new SmdBssTransitionParametersElementTest({
                    .name = "ST Exec Resp with Nominal Maximum DL Draining Period Duration and "
                            "Latest UL SN List",
                    .stInfo = GetStExecRespStInfo(250, {{1, 100}, {7, 4095}}),
                }),
                TestCase::Duration::QUICK);
}

static WifiUhrInfoElemsTestSuite g_wifiUhrInfoElemsTestSuite; ///< the test suite
