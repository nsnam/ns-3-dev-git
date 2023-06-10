/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/address-utils.h"
#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/mgt-headers.h"
#include "ns3/multi-link-element.h"
#include "ns3/reduced-neighbor-report.h"
#include "ns3/tid-to-link-mapping-element.h"
#include "ns3/wifi-phy-operating-channel.h"

#include <optional>
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiEhtInfoElemsTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test Multi-Link Element (Basic variant) serialization and deserialization
 */
class BasicMultiLinkElementTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     */
    BasicMultiLinkElementTest();
    ~BasicMultiLinkElementTest() override;

    /**
     * Get a Multi-Link Element including the given Common Info field and the
     * given Per-STA Profile Subelements
     *
     * \param commonInfo the given Common Info field
     * \param subelements the given set of Per-STA Profile Subelements
     * \return a Multi-Link Element
     */
    MultiLinkElement GetMultiLinkElement(
        const CommonInfoBasicMle& commonInfo,
        std::vector<MultiLinkElement::PerStaProfileSubelement> subelements);

  private:
    void DoRun() override;

    MgtAssocRequestHeader m_outerAssoc; //!< the frame containing the MLE
};

BasicMultiLinkElementTest::BasicMultiLinkElementTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of Basic variant Multi-Link elements")
{
}

BasicMultiLinkElementTest::~BasicMultiLinkElementTest()
{
}

MultiLinkElement
BasicMultiLinkElementTest::GetMultiLinkElement(
    const CommonInfoBasicMle& commonInfo,
    std::vector<MultiLinkElement::PerStaProfileSubelement> subelements)
{
    MultiLinkElement mle(MultiLinkElement::BASIC_VARIANT);
    mle.SetMldMacAddress(commonInfo.m_mldMacAddress);
    if (commonInfo.m_linkIdInfo.has_value())
    {
        mle.SetLinkIdInfo(*commonInfo.m_linkIdInfo);
    }
    if (commonInfo.m_bssParamsChangeCount.has_value())
    {
        mle.SetBssParamsChangeCount(*commonInfo.m_bssParamsChangeCount);
    }
    if (commonInfo.m_mediumSyncDelayInfo.has_value())
    {
        mle.SetMediumSyncDelayTimer(
            MicroSeconds(32 * commonInfo.m_mediumSyncDelayInfo->mediumSyncDuration));
        mle.SetMediumSyncOfdmEdThreshold(
            commonInfo.m_mediumSyncDelayInfo->mediumSyncOfdmEdThreshold - 72);
        mle.SetMediumSyncMaxNTxops(commonInfo.m_mediumSyncDelayInfo->mediumSyncMaxNTxops + 1);
    }
    if (commonInfo.m_emlCapabilities.has_value())
    {
        auto padding = commonInfo.m_emlCapabilities->emlsrPaddingDelay;
        mle.SetEmlsrPaddingDelay(MicroSeconds(padding == 0 ? 0 : (1 << (4 + padding))));
        auto transitionD = commonInfo.m_emlCapabilities->emlsrTransitionDelay;
        mle.SetEmlsrTransitionDelay(MicroSeconds(transitionD == 0 ? 0 : (1 << (3 + transitionD))));
        auto transitionT = commonInfo.m_emlCapabilities->transitionTimeout;
        mle.SetTransitionTimeout(MicroSeconds(transitionT == 0 ? 0 : (1 << (6 + transitionT))));
    }

    for (std::size_t i = 0; i < subelements.size(); ++i)
    {
        mle.AddPerStaProfileSubelement();
        mle.GetPerStaProfile(i) = std::move(subelements[i]);
    }

    return mle;
}

void
BasicMultiLinkElementTest::DoRun()
{
    CommonInfoBasicMle commonInfo = {
        .m_mldMacAddress = Mac48Address("01:23:45:67:89:ab"),
    };

    // Common Info with MLD MAC address
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {}));

    commonInfo.m_linkIdInfo = 3;

    // Adding Link ID Info
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {}));

    commonInfo.m_bssParamsChangeCount = 1;

    // Adding BSS Parameters Change Count
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {}));

    commonInfo.m_mediumSyncDelayInfo =
        CommonInfoBasicMle::MediumSyncDelayInfo{.mediumSyncDuration = 1,
                                                .mediumSyncOfdmEdThreshold = 4,
                                                .mediumSyncMaxNTxops = 5};

    // Adding Medium Sync Delay Information
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {}));

    commonInfo.m_emlCapabilities = CommonInfoBasicMle::EmlCapabilities{.emlsrSupport = 1,
                                                                       .emlsrPaddingDelay = 4,
                                                                       .emlsrTransitionDelay = 5,
                                                                       .transitionTimeout = 10};

    // Adding Medium Sync Delay Information
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {}));

    /**
     * To test the serialization/deserialization of Per-STA Profile subelements, we include
     * the Multi-Link Element in an Association Request frame
     */

    CapabilityInformation capabilities;
    capabilities.SetShortPreamble(true);
    capabilities.SetShortSlotTime(true);
    capabilities.SetEss();

    m_outerAssoc.SetListenInterval(0);
    m_outerAssoc.Capabilities() = capabilities;
    m_outerAssoc.Get<Ssid>() = Ssid("MySsid");

    AllSupportedRates rates;
    rates.AddSupportedRate(6e6);
    rates.AddSupportedRate(9e6);
    rates.AddSupportedRate(12e6);
    rates.AddSupportedRate(18e6);
    rates.AddSupportedRate(24e6);
    rates.AddSupportedRate(36e6);
    rates.AddSupportedRate(48e6);
    rates.AddSupportedRate(54e6);
    // extended rates
    rates.AddSupportedRate(1e6);
    rates.AddSupportedRate(2e6);

    m_outerAssoc.Get<SupportedRates>() = rates.rates;
    m_outerAssoc.Get<ExtendedSupportedRatesIE>() = rates.extendedRates;

    EhtCapabilities ehtCapabilities;
    for (auto maxMcs : {7, 9, 11, 13})
    {
        ehtCapabilities.SetSupportedRxEhtMcsAndNss(EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY,
                                                   maxMcs,
                                                   1);
        ehtCapabilities.SetSupportedTxEhtMcsAndNss(EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY,
                                                   maxMcs,
                                                   1);
    }

    m_outerAssoc.Get<HeCapabilities>().emplace();
    m_outerAssoc.Get<EhtCapabilities>() = ehtCapabilities;

    // The Association Request included in the first Per-STA Profile subelement is identical
    // to the containing frame, so that all the IEs are inherited and the Per-STA Profile
    // does not contain any Information Element.

    MultiLinkElement::PerStaProfileSubelement perStaProfile1(MultiLinkElement::BASIC_VARIANT);
    perStaProfile1.SetLinkId(3);
    perStaProfile1.SetCompleteProfile();
    perStaProfile1.SetAssocRequest(m_outerAssoc);

    /* Association Request included in the second Per-STA Profile subelement */
    MgtAssocRequestHeader assoc;
    assoc.Capabilities() = capabilities;
    // we simulate a "mistake" by adding an Ssid IE, which cannot be included in the
    // Per-STA Profile subelement. We will check that this Ssid is not serialized
    assoc.Get<Ssid>() = Ssid("OtherSsid");
    // another "mistake" of the same type, except that a TID-To-Link Mapping element
    // is not included in the containing frame
    assoc.Get<TidToLinkMapping>().emplace();
    // the SupportedRates IE is the same (hence not serialized) as in the containing frame,
    // while the ExtendedSupportedRatesIE is different (hence serialized)
    rates.AddSupportedRate(5.5e6);
    rates.AddSupportedRate(11e6);
    assoc.Get<SupportedRates>() = rates.rates;
    assoc.Get<ExtendedSupportedRatesIE>() = rates.extendedRates;
    // a VhtCapabilities IE is not present in the containing frame, hence it is serialized
    assoc.Get<VhtCapabilities>().emplace();
    // HeCapabilities IE is present in the containing frame and in the Per-STA Profile subelement,
    // hence it is not serialized
    assoc.Get<HeCapabilities>().emplace();
    // EhtCapabilities IE is present in the containing frame but not in the Per-STA Profile
    //  subelement, hence it is listed in a Non-Inheritance element

    MultiLinkElement::PerStaProfileSubelement perStaProfile2(MultiLinkElement::BASIC_VARIANT);
    perStaProfile2.SetLinkId(0);
    perStaProfile2.SetCompleteProfile();
    perStaProfile2.SetStaMacAddress(Mac48Address("ba:98:76:54:32:10"));
    perStaProfile2.SetAssocRequest(assoc);

    // The Association Request included in the third Per-STA Profile subelement has the
    // EHT Capabilities element (which is inherited and not serialized) but it does not have the
    // Ssid element, which is not listed in the Non-Inheritance element because it shall not
    // appear in a Per-STA Profile subelement.
    assoc.Get<Ssid>().reset();
    assoc.Get<EhtCapabilities>() = ehtCapabilities;

    auto perStaProfile3 = perStaProfile2;
    perStaProfile3.SetAssocRequest(assoc);

    // Adding MLE with two Per-STA Profile Subelements
    m_outerAssoc.Get<MultiLinkElement>() =
        GetMultiLinkElement(commonInfo, {perStaProfile1, perStaProfile2, perStaProfile3});

    // first, check that serialization/deserialization of the whole Association Request works
    TestHeaderSerialization(m_outerAssoc);

    // now, "manually" serialize and deserialize the header to check that the expected elements
    // have been serialized
    Buffer buffer;
    buffer.AddAtStart(m_outerAssoc.GetSerializedSize());
    m_outerAssoc.Serialize(buffer.Begin());

    auto i = buffer.Begin();
    i = CapabilityInformation().Deserialize(i);
    i.ReadLsbtohU16(); // Listen interval

    auto tmp = i;
    i = Ssid().DeserializeIfPresent(tmp);
    NS_TEST_EXPECT_MSG_GT(i.GetDistanceFrom(tmp), 0, "Ssid element not present");

    i = SupportedRates().DeserializeIfPresent(tmp = i);
    NS_TEST_EXPECT_MSG_GT(i.GetDistanceFrom(tmp), 0, "SupportedRates element not present");

    i = ExtendedSupportedRatesIE().DeserializeIfPresent(tmp = i);
    NS_TEST_EXPECT_MSG_GT(i.GetDistanceFrom(tmp),
                          0,
                          "ExtendedSupportedRatesIE element not present");

    i = HeCapabilities().DeserializeIfPresent(tmp = i);
    NS_TEST_EXPECT_MSG_GT(i.GetDistanceFrom(tmp), 0, "HeCapabilities element not present");

    // deserialize Multi-Link Element
    NS_TEST_EXPECT_MSG_EQ(i.ReadU8(), IE_EXTENSION, "IE_EXTENSION expected at the begin of MLE");
    i.ReadU8(); // length
    NS_TEST_EXPECT_MSG_EQ(i.ReadU8(),
                          IE_EXT_MULTI_LINK_ELEMENT,
                          "IE_EXT_MULTI_LINK_ELEMENT expected");

    uint16_t mlControl = i.ReadLsbtohU16();
    auto nBytes = CommonInfoBasicMle().Deserialize(i, mlControl >> 4);
    i.Next(nBytes);

    // first Per-STA Profile subelement
    NS_TEST_EXPECT_MSG_EQ(i.ReadU8(),
                          MultiLinkElement::PER_STA_PROFILE_SUBELEMENT_ID,
                          "PER_STA_PROFILE_SUBELEMENT_ID expected");
    i.ReadU8();        // length
    i.ReadLsbtohU16(); // STA Control field
    i.ReadU8();        // STA Info Length
    // no STA address
    i = CapabilityInformation().Deserialize(i);
    // no Information Element

    // second Per-STA Profile subelement
    NS_TEST_EXPECT_MSG_EQ(i.ReadU8(),
                          MultiLinkElement::PER_STA_PROFILE_SUBELEMENT_ID,
                          "PER_STA_PROFILE_SUBELEMENT_ID expected");
    i.ReadU8();        // length
    i.ReadLsbtohU16(); // STA Control field
    i.ReadU8();        // STA Info Length
    Mac48Address address;
    ReadFrom(i, address);
    i = CapabilityInformation().Deserialize(i);
    // no Listen interval
    // Ssid element not present (as mandated by specs)
    // SupportedRates not present because it is inherited

    i = ExtendedSupportedRatesIE().DeserializeIfPresent(tmp = i);
    NS_TEST_EXPECT_MSG_GT(i.GetDistanceFrom(tmp),
                          0,
                          "ExtendedSupportedRatesIE element not present");

    i = VhtCapabilities().DeserializeIfPresent(tmp = i);
    NS_TEST_EXPECT_MSG_GT(i.GetDistanceFrom(tmp), 0, "VhtCapabilities element not present");

    // HeCapabilities not present because it is inherited
    NonInheritance nonInheritance;
    i = nonInheritance.DeserializeIfPresent(tmp = i);
    NS_TEST_EXPECT_MSG_GT(i.GetDistanceFrom(tmp), 0, "Non-Inheritance element not present");
    NS_TEST_EXPECT_MSG_EQ(nonInheritance.IsPresent(IE_EXTENSION, IE_EXT_EHT_CAPABILITIES),
                          true,
                          "Non-Inheritance does not indicate EhtCapabilities");
    NS_TEST_EXPECT_MSG_EQ(nonInheritance.m_elemIdList.size(),
                          0,
                          "Unexpected size for Elem ID list of Non-Inheritance element");
    NS_TEST_EXPECT_MSG_EQ(nonInheritance.m_elemIdExtList.size(),
                          1,
                          "Unexpected size for Elem ID list of Non-Inheritance element");

    // third Per-STA Profile subelement
    NS_TEST_EXPECT_MSG_EQ(i.ReadU8(),
                          MultiLinkElement::PER_STA_PROFILE_SUBELEMENT_ID,
                          "PER_STA_PROFILE_SUBELEMENT_ID expected");
    i.ReadU8();        // length
    i.ReadLsbtohU16(); // STA Control field
    i.ReadU8();        // STA Info Length
    ReadFrom(i, address);
    i = CapabilityInformation().Deserialize(i);
    // no Listen interval
    // Ssid element not present (as mandated by specs)
    // SupportedRates not present because it is inherited

    i = ExtendedSupportedRatesIE().DeserializeIfPresent(tmp = i);
    NS_TEST_EXPECT_MSG_GT(i.GetDistanceFrom(tmp),
                          0,
                          "ExtendedSupportedRatesIE element not present");

    i = VhtCapabilities().DeserializeIfPresent(tmp = i);
    NS_TEST_EXPECT_MSG_GT(i.GetDistanceFrom(tmp), 0, "VhtCapabilities element not present");

    // HeCapabilities not present because it is inherited
    // EhtCapabilities not present because it is inherited

    // the Multi-Link Element is done, we shall now find the EHT Capabilities of the
    // containing Association Request frame
    ehtCapabilities = EhtCapabilities(true, m_outerAssoc.Get<HeCapabilities>().value());
    i = ehtCapabilities.DeserializeIfPresent(tmp = i);
    NS_TEST_EXPECT_MSG_GT(i.GetDistanceFrom(tmp), 0, "EhtCapabilities element not present");

    /**
     * Yet another test: use the Deserialize method of the management frame and check that
     * inherited Information Elements have been copied
     */
    MgtAssocRequestHeader frame;
    auto count = frame.Deserialize(buffer.Begin());

    NS_TEST_EXPECT_MSG_EQ(count, buffer.GetSize(), "Unexpected number of deserialized bytes");

    // containing frame
    NS_TEST_EXPECT_MSG_EQ(frame.Get<Ssid>().has_value(),
                          true,
                          "Containing frame should have SSID IE");
    NS_TEST_EXPECT_MSG_EQ(frame.Get<SupportedRates>().has_value(),
                          true,
                          "Containing frame should have Supported Rates IE");
    NS_TEST_EXPECT_MSG_EQ(frame.Get<ExtendedSupportedRatesIE>().has_value(),
                          true,
                          "Containing frame should have Extended Supported Rates IE");
    NS_TEST_EXPECT_MSG_EQ(frame.Get<HtCapabilities>().has_value(),
                          false,
                          "Containing frame should not have HT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(frame.Get<ExtendedCapabilities>().has_value(),
                          false,
                          "Containing frame should not have Extended Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(frame.Get<VhtCapabilities>().has_value(),
                          false,
                          "Containing frame should not have VHT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(frame.Get<HeCapabilities>().has_value(),
                          true,
                          "Containing frame should have HE Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(frame.Get<MultiLinkElement>().has_value(),
                          true,
                          "Containing frame should have Multi-Link Element IE");
    NS_TEST_EXPECT_MSG_EQ(frame.Get<EhtCapabilities>().has_value(),
                          true,
                          "Containing frame should have EHT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(frame.Get<TidToLinkMapping>().has_value(),
                          false,
                          "Containing frame should not have TID-to-Link Mapping IE");

    auto& mle = frame.Get<MultiLinkElement>().value();

    NS_TEST_EXPECT_MSG_EQ(mle.GetNPerStaProfileSubelements(),
                          3,
                          "Unexpected number of Per-STA Profile subelements");

    // frame in first Per-STA Profile subelement has inherited all the IEs but SSID and
    // Multi-Link Element IEs
    auto& perSta1 = mle.GetPerStaProfile(0);
    NS_TEST_EXPECT_MSG_EQ(perSta1.HasAssocRequest(),
                          true,
                          "First Per-STA Profile should contain an Association Request frame");
    auto& perSta1Frame =
        std::get<std::reference_wrapper<MgtAssocRequestHeader>>(perSta1.GetAssocRequest()).get();

    NS_TEST_EXPECT_MSG_EQ(perSta1Frame.Get<Ssid>().has_value(),
                          false,
                          "Frame in first Per-STA Profile should not have SSID IE");
    NS_TEST_EXPECT_MSG_EQ(perSta1Frame.Get<SupportedRates>().has_value(),
                          true,
                          "Frame in first Per-STA Profile should have Supported Rates IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta1Frame.Get<SupportedRates>() == frame.Get<SupportedRates>()),
        true,
        "Supported Rates IE not correctly inherited by frame in first Per-STA Profile");
    NS_TEST_EXPECT_MSG_EQ(perSta1Frame.Get<ExtendedSupportedRatesIE>().has_value(),
                          true,
                          "Frame in first Per-STA Profile should have Extended Supported Rates IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta1Frame.Get<ExtendedSupportedRatesIE>() == frame.Get<ExtendedSupportedRatesIE>()),
        true,
        "Extended Supported Rates IE not correctly inherited by frame in first Per-STA Profile");
    NS_TEST_EXPECT_MSG_EQ(perSta1Frame.Get<HtCapabilities>().has_value(),
                          false,
                          "Frame in first Per-STA Profile should not have HT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(
        perSta1Frame.Get<ExtendedCapabilities>().has_value(),
        false,
        "Frame in first Per-STA Profile should not have Extended Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(perSta1Frame.Get<VhtCapabilities>().has_value(),
                          false,
                          "Frame in first Per-STA Profile should not have VHT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(perSta1Frame.Get<HeCapabilities>().has_value(),
                          true,
                          "Frame in first Per-STA Profile should have HE Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta1Frame.Get<HeCapabilities>() == frame.Get<HeCapabilities>()),
        true,
        "HE Capabilities IE not correctly inherited by frame in first Per-STA Profile");
    NS_TEST_EXPECT_MSG_EQ(perSta1Frame.Get<MultiLinkElement>().has_value(),
                          false,
                          "Frame in first Per-STA Profile should not have Multi-Link Element IE");
    NS_TEST_EXPECT_MSG_EQ(perSta1Frame.Get<EhtCapabilities>().has_value(),
                          true,
                          "Frame in first Per-STA Profile should have EHT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta1Frame.Get<EhtCapabilities>() == frame.Get<EhtCapabilities>()),
        true,
        "EHT Capabilities IE not correctly inherited by frame in first Per-STA Profile");
    NS_TEST_EXPECT_MSG_EQ(perSta1Frame.Get<TidToLinkMapping>().has_value(),
                          false,
                          "Frame in first Per-STA Profile should not have TID-to-Link Mapping IE");

    // frame in second Per-STA Profile subelement includes VHT Capabilities IE and has inherited
    //  all the IEs but SSID IE, Multi-Link Element IE, Extended Supported Rates IE (different
    // than in containing frame) and EHT Capabilities IE (listed in Non-Inheritance IE).
    auto& perSta2 = mle.GetPerStaProfile(1);
    NS_TEST_EXPECT_MSG_EQ(perSta2.HasAssocRequest(),
                          true,
                          "Second Per-STA Profile should contain an Association Request frame");
    auto& perSta2Frame =
        std::get<std::reference_wrapper<MgtAssocRequestHeader>>(perSta2.GetAssocRequest()).get();

    NS_TEST_EXPECT_MSG_EQ(perSta2Frame.Get<Ssid>().has_value(),
                          false,
                          "Frame in second Per-STA Profile should not have SSID IE");
    NS_TEST_EXPECT_MSG_EQ(perSta2Frame.Get<SupportedRates>().has_value(),
                          true,
                          "Frame in second Per-STA Profile should have Supported Rates IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta2Frame.Get<SupportedRates>() == frame.Get<SupportedRates>()),
        true,
        "Supported Rates IE not correctly inherited by frame in second Per-STA Profile");
    NS_TEST_EXPECT_MSG_EQ(
        perSta2Frame.Get<ExtendedSupportedRatesIE>().has_value(),
        true,
        "Frame in second Per-STA Profile should have Extended Supported Rates IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta2Frame.Get<ExtendedSupportedRatesIE>() == frame.Get<ExtendedSupportedRatesIE>()),
        false,
        "Extended Supported Rates IE should have not been inherited by frame in second Per-STA "
        "Profile");
    NS_TEST_EXPECT_MSG_EQ(perSta2Frame.Get<HtCapabilities>().has_value(),
                          false,
                          "Frame in second Per-STA Profile should not have HT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(
        perSta2Frame.Get<ExtendedCapabilities>().has_value(),
        false,
        "Frame in second Per-STA Profile should not have Extended Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(perSta2Frame.Get<VhtCapabilities>().has_value(),
                          true,
                          "Frame in second Per-STA Profile should have VHT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(perSta2Frame.Get<HeCapabilities>().has_value(),
                          true,
                          "Frame in second Per-STA Profile should have HE Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta2Frame.Get<HeCapabilities>() == frame.Get<HeCapabilities>()),
        true,
        "HE Capabilities IE not correctly inherited by frame in second Per-STA Profile");
    NS_TEST_EXPECT_MSG_EQ(perSta2Frame.Get<MultiLinkElement>().has_value(),
                          false,
                          "Frame in second Per-STA Profile should not have Multi-Link Element IE");
    NS_TEST_EXPECT_MSG_EQ(
        perSta2Frame.Get<EhtCapabilities>().has_value(),
        false,
        "Frame in second Per-STA Profile should have not inherited EHT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(perSta2Frame.Get<TidToLinkMapping>().has_value(),
                          false,
                          "Frame in second Per-STA Profile should not have TID-to-Link Mapping IE");

    // frame in third Per-STA Profile subelement includes VHT Capabilities IE and has inherited
    //  all the IEs but SSID IE, Multi-Link Element IE and Extended Supported Rates IE (different
    // than in containing frame).
    auto& perSta3 = mle.GetPerStaProfile(2);
    NS_TEST_EXPECT_MSG_EQ(perSta3.HasAssocRequest(),
                          true,
                          "Third Per-STA Profile should contain an Association Request frame");
    auto& perSta3Frame =
        std::get<std::reference_wrapper<MgtAssocRequestHeader>>(perSta3.GetAssocRequest()).get();

    NS_TEST_EXPECT_MSG_EQ(perSta3Frame.Get<Ssid>().has_value(),
                          false,
                          "Frame in third Per-STA Profile should not have SSID IE");
    NS_TEST_EXPECT_MSG_EQ(perSta3Frame.Get<SupportedRates>().has_value(),
                          true,
                          "Frame in third Per-STA Profile should have Supported Rates IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta3Frame.Get<SupportedRates>() == frame.Get<SupportedRates>()),
        true,
        "Supported Rates IE not correctly inherited by frame in third Per-STA Profile");
    NS_TEST_EXPECT_MSG_EQ(perSta3Frame.Get<ExtendedSupportedRatesIE>().has_value(),
                          true,
                          "Frame in third Per-STA Profile should have Extended Supported Rates IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta3Frame.Get<ExtendedSupportedRatesIE>() == frame.Get<ExtendedSupportedRatesIE>()),
        false,
        "Extended Supported Rates IE should have not been inherited by frame in third Per-STA "
        "Profile");
    NS_TEST_EXPECT_MSG_EQ(perSta3Frame.Get<HtCapabilities>().has_value(),
                          false,
                          "Frame in third Per-STA Profile should not have HT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(
        perSta3Frame.Get<ExtendedCapabilities>().has_value(),
        false,
        "Frame in third Per-STA Profile should not have Extended Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(perSta3Frame.Get<VhtCapabilities>().has_value(),
                          true,
                          "Frame in third Per-STA Profile should have VHT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(perSta3Frame.Get<HeCapabilities>().has_value(),
                          true,
                          "Frame in third Per-STA Profile should have HE Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta3Frame.Get<HeCapabilities>() == frame.Get<HeCapabilities>()),
        true,
        "HE Capabilities IE not correctly inherited by frame in third Per-STA Profile");
    NS_TEST_EXPECT_MSG_EQ(perSta3Frame.Get<MultiLinkElement>().has_value(),
                          false,
                          "Frame in third Per-STA Profile should not have Multi-Link Element IE");
    NS_TEST_EXPECT_MSG_EQ(
        perSta3Frame.Get<EhtCapabilities>().has_value(),
        true,
        "Frame in third Per-STA Profile should have inherited EHT Capabilities IE");
    NS_TEST_EXPECT_MSG_EQ(
        (perSta3Frame.Get<EhtCapabilities>() == frame.Get<EhtCapabilities>()),
        true,
        "EHT Capabilities IE not correctly inherited by frame in third Per-STA Profile");
    NS_TEST_EXPECT_MSG_EQ(perSta3Frame.Get<TidToLinkMapping>().has_value(),
                          false,
                          "Frame in third Per-STA Profile should not have TID-to-Link Mapping IE");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test Reduced Neighbor Report serialization and deserialization
 */
class ReducedNeighborReportTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     */
    ReducedNeighborReportTest();
    ~ReducedNeighborReportTest() override;

    /// typedef for const iterator on the set of available channels
    using PhyOpChannelIt = WifiPhyOperatingChannel::ConstIterator;

    /**
     * Get a Reduced Neighbor Report element including the given operating channels
     *
     * \param channel2_4It a channel in the 2.4 GHz band
     * \param channel5It a channel in the 5 GHz band
     * \param channel6It a channel in the 6 GHz band
     * \return a Reduced Neighbor Report element
     */
    ReducedNeighborReport GetReducedNeighborReport(PhyOpChannelIt channel2_4It,
                                                   PhyOpChannelIt channel5It,
                                                   PhyOpChannelIt channel6It);

  private:
    void DoRun() override;
};

ReducedNeighborReportTest::ReducedNeighborReportTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of Reduced Neighbor Report elements")
{
}

ReducedNeighborReportTest::~ReducedNeighborReportTest()
{
}

ReducedNeighborReport
ReducedNeighborReportTest::GetReducedNeighborReport(PhyOpChannelIt channel2_4It,
                                                    PhyOpChannelIt channel5It,
                                                    PhyOpChannelIt channel6It)
{
    ReducedNeighborReport rnr;

    std::stringstream info;

    if (channel2_4It != WifiPhyOperatingChannel::m_frequencyChannels.cend())
    {
        WifiPhyOperatingChannel channel(channel2_4It);

        info << "{Ch=" << +channel.GetNumber() << ", Bw=" << channel.GetWidth() << ", 2.4 GHz} ";
        rnr.AddNbrApInfoField();
        std::size_t nbrId = rnr.GetNNbrApInfoFields() - 1;
        rnr.SetOperatingChannel(nbrId, channel);
        // Add a TBTT Information Field
        rnr.AddTbttInformationField(nbrId);
        rnr.SetBssid(nbrId, 0, Mac48Address("00:00:00:00:00:24"));
        rnr.SetShortSsid(nbrId, 0, 0);
        rnr.SetBssParameters(nbrId, 0, 10);
        rnr.SetPsd20MHz(nbrId, 0, 50);
        rnr.SetMldParameters(nbrId, 0, 0, 2, 3);
    }

    if (channel5It != WifiPhyOperatingChannel::m_frequencyChannels.cend())
    {
        WifiPhyOperatingChannel channel(channel5It);

        info << "{Ch=" << +channel.GetNumber() << ", Bw=" << channel.GetWidth() << ", 5 GHz} ";
        rnr.AddNbrApInfoField();
        std::size_t nbrId = rnr.GetNNbrApInfoFields() - 1;
        rnr.SetOperatingChannel(nbrId, channel);
        // Add a TBTT Information Field
        rnr.AddTbttInformationField(nbrId);
        rnr.SetBssid(nbrId, 0, Mac48Address("00:00:00:00:00:05"));
        rnr.SetShortSsid(nbrId, 0, 0);
        rnr.SetBssParameters(nbrId, 0, 20);
        rnr.SetPsd20MHz(nbrId, 0, 60);
        rnr.SetMldParameters(nbrId, 0, 0, 3, 4);
        // Add another TBTT Information Field
        rnr.AddTbttInformationField(nbrId);
        rnr.SetBssid(nbrId, 1, Mac48Address("00:00:00:00:01:05"));
        rnr.SetShortSsid(nbrId, 1, 0);
        rnr.SetBssParameters(nbrId, 1, 30);
        rnr.SetPsd20MHz(nbrId, 1, 70);
        rnr.SetMldParameters(nbrId, 1, 0, 4, 5);
    }

    if (channel6It != WifiPhyOperatingChannel::m_frequencyChannels.cend())
    {
        WifiPhyOperatingChannel channel(channel6It);

        info << "{Ch=" << +channel.GetNumber() << ", Bw=" << channel.GetWidth() << ", 6 GHz} ";
        rnr.AddNbrApInfoField();
        std::size_t nbrId = rnr.GetNNbrApInfoFields() - 1;
        rnr.SetOperatingChannel(nbrId, channel);
        // Add a TBTT Information Field
        rnr.AddTbttInformationField(nbrId);
        rnr.SetBssid(nbrId, 0, Mac48Address("00:00:00:00:00:06"));
        rnr.SetShortSsid(nbrId, 0, 0);
        rnr.SetBssParameters(nbrId, 0, 40);
        rnr.SetPsd20MHz(nbrId, 0, 80);
        rnr.SetMldParameters(nbrId, 0, 0, 5, 6);
    }

    NS_LOG_DEBUG(info.str());
    return rnr;
}

void
ReducedNeighborReportTest::DoRun()
{
    PhyOpChannelIt channel2_4It;
    PhyOpChannelIt channel5It;
    PhyOpChannelIt channel6It;
    channel2_4It = channel5It = channel6It = WifiPhyOperatingChannel::m_frequencyChannels.cbegin();

    // Test all available frequency channels
    while (channel2_4It != WifiPhyOperatingChannel::m_frequencyChannels.cend() ||
           channel5It != WifiPhyOperatingChannel::m_frequencyChannels.cend() ||
           channel6It != WifiPhyOperatingChannel::m_frequencyChannels.cend())
    {
        if (channel2_4It != WifiPhyOperatingChannel::m_frequencyChannels.cend())
        {
            channel2_4It = WifiPhyOperatingChannel::FindFirst(0,
                                                              0,
                                                              0,
                                                              WIFI_STANDARD_80211be,
                                                              WIFI_PHY_BAND_2_4GHZ,
                                                              channel2_4It);
        }
        if (channel5It != WifiPhyOperatingChannel::m_frequencyChannels.cend())
        {
            channel5It = WifiPhyOperatingChannel::FindFirst(0,
                                                            0,
                                                            0,
                                                            WIFI_STANDARD_80211be,
                                                            WIFI_PHY_BAND_5GHZ,
                                                            channel5It);
        }
        if (channel6It != WifiPhyOperatingChannel::m_frequencyChannels.cend())
        {
            channel6It = WifiPhyOperatingChannel::FindFirst(0,
                                                            0,
                                                            0,
                                                            WIFI_STANDARD_80211be,
                                                            WIFI_PHY_BAND_6GHZ,
                                                            channel6It);
        }

        TestHeaderSerialization(GetReducedNeighborReport(channel2_4It, channel5It, channel6It));

        // advance all channel iterators
        if (channel2_4It != WifiPhyOperatingChannel::m_frequencyChannels.cend())
        {
            channel2_4It++;
        }
        if (channel5It != WifiPhyOperatingChannel::m_frequencyChannels.cend())
        {
            channel5It++;
        }
        if (channel6It != WifiPhyOperatingChannel::m_frequencyChannels.cend())
        {
            channel6It++;
        }
    }
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test serialization and deserialization of EHT capabilities IE
 */
class WifiEhtCapabilitiesIeTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     * \param is2_4Ghz whether the PHY is operating in 2.4 GHz
     * \param channelWidth the supported channel width in MHz
     */
    WifiEhtCapabilitiesIeTest(bool is2_4Ghz, uint16_t channelWidth);
    ~WifiEhtCapabilitiesIeTest() override = default;

    /**
     * Generate the HE capabilities IE.
     *
     * \return the generated HE capabilities IE
     */
    HeCapabilities GetHeCapabilities() const;

    /**
     * Generate the EHT capabilities IE.
     *
     * \param maxMpduLength the maximum MPDU length in bytes
     * \param maxAmpduSize the maximum A-MPDU size in bytes
     * \param maxSupportedMcs the maximum EHT MCS supported by the PHY
     * \return the generated EHT capabilities IE
     */
    EhtCapabilities GetEhtCapabilities(uint16_t maxMpduLength,
                                       uint32_t maxAmpduSize,
                                       uint8_t maxSupportedMcs) const;

    /**
     * Serialize the EHT capabilities in a buffer.
     *
     * \param ehtCapabilities the EHT capabilities
     * \return the buffer in which the EHT capabilities has been serialized
     */
    Buffer SerializeIntoBuffer(const EhtCapabilities& ehtCapabilities);

    /**
     * Check that the given buffer contains the given value at the given position.
     *
     * \param buffer the given buffer
     * \param position the given position (starting at 0)
     * \param value the given value
     */
    void CheckSerializedByte(const Buffer& buffer, uint32_t position, uint8_t value);

    /**
     * Check the content of the EHT MAC Capabilities Information subfield.
     *
     * \param buffer the buffer containing the serialized EHT capabilities
     * \param expectedValueFirstByte the expected value for the first byte
     */
    void CheckEhtMacCapabilitiesInformation(const Buffer& buffer, uint8_t expectedValueFirstByte);

    /**
     * Check the content of the EHT PHY Capabilities Information subfield.
     *
     * \param buffer the buffer containing the serialized EHT capabilities
     * \param expectedValueSixthByte the expected value for the sixth byte
     */
    void CheckEhtPhyCapabilitiesInformation(const Buffer& buffer, uint8_t expectedValueSixthByte);

    /**
     * Check the content of the Supported EHT-MCS And NSS Set subfield.
     * \param maxSupportedMcs the maximum EHT MCS supported by the PHY
     *
     * \param buffer the buffer containing the serialized EHT capabilities
     */
    void CheckSupportedEhtMcsAndNssSet(const Buffer& buffer, uint8_t maxSupportedMcs);

  private:
    void DoRun() override;

    bool m_is2_4Ghz;         //!< whether the PHY is operating in 2.4 GHz
    uint16_t m_channelWidth; //!< Supported channel width by the PHY (in MHz)
};

WifiEhtCapabilitiesIeTest ::WifiEhtCapabilitiesIeTest(bool is2_4Ghz, uint16_t channelWidth)
    : HeaderSerializationTestCase{"Check serialization and deserialization of EHT capabilities IE"},
      m_is2_4Ghz{is2_4Ghz},
      m_channelWidth{channelWidth}
{
}

HeCapabilities
WifiEhtCapabilitiesIeTest::GetHeCapabilities() const
{
    HeCapabilities capabilities;
    uint8_t channelWidthSet = 0;
    if ((m_channelWidth >= 40) && m_is2_4Ghz)
    {
        channelWidthSet |= 0x01;
    }
    if ((m_channelWidth >= 80) && !m_is2_4Ghz)
    {
        channelWidthSet |= 0x02;
    }
    if ((m_channelWidth >= 160) && !m_is2_4Ghz)
    {
        channelWidthSet |= 0x04;
    }
    capabilities.SetChannelWidthSet(channelWidthSet);
    return capabilities;
}

EhtCapabilities
WifiEhtCapabilitiesIeTest::GetEhtCapabilities(uint16_t maxMpduLength,
                                              uint32_t maxAmpduSize,
                                              uint8_t maxSupportedMcs) const
{
    EhtCapabilities capabilities;

    if (m_is2_4Ghz)
    {
        capabilities.SetMaxMpduLength(maxMpduLength);
    }
    // round to the next power of two minus one
    maxAmpduSize = (1UL << static_cast<uint32_t>(std::ceil(std::log2(maxAmpduSize + 1)))) - 1;
    // The maximum A-MPDU length in EHT capabilities elements ranges from 2^23-1 to 2^24-1
    capabilities.SetMaxAmpduLength(std::min(std::max(maxAmpduSize, 8388607U), 16777215U));

    capabilities.m_phyCapabilities.supportTx1024And4096QamForRuSmallerThan242Tones =
        (maxSupportedMcs >= 12) ? 1 : 0;
    capabilities.m_phyCapabilities.supportRx1024And4096QamForRuSmallerThan242Tones =
        (maxSupportedMcs >= 12) ? 1 : 0;
    if (m_channelWidth == 20)
    {
        for (auto maxMcs : {7, 9, 11, 13})
        {
            capabilities.SetSupportedRxEhtMcsAndNss(EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY,
                                                    maxMcs,
                                                    maxMcs <= maxSupportedMcs ? 1 : 0);
            capabilities.SetSupportedTxEhtMcsAndNss(EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY,
                                                    maxMcs,
                                                    maxMcs <= maxSupportedMcs ? 2 : 0);
        }
    }
    else
    {
        for (auto maxMcs : {9, 11, 13})
        {
            capabilities.SetSupportedRxEhtMcsAndNss(
                EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_NOT_LARGER_THAN_80_MHZ,
                maxMcs,
                maxMcs <= maxSupportedMcs ? 3 : 0);
            capabilities.SetSupportedTxEhtMcsAndNss(
                EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_NOT_LARGER_THAN_80_MHZ,
                maxMcs,
                maxMcs <= maxSupportedMcs ? 4 : 0);
        }
    }
    if (m_channelWidth >= 160)
    {
        for (auto maxMcs : {9, 11, 13})
        {
            capabilities.SetSupportedRxEhtMcsAndNss(EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_160_MHZ,
                                                    maxMcs,
                                                    maxMcs <= maxSupportedMcs ? 2 : 0);
            capabilities.SetSupportedTxEhtMcsAndNss(EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_160_MHZ,
                                                    maxMcs,
                                                    maxMcs <= maxSupportedMcs ? 1 : 0);
        }
    }
    if (m_channelWidth == 320)
    {
        capabilities.m_phyCapabilities.support320MhzIn6Ghz = 1;
        for (auto maxMcs : {9, 11, 13})
        {
            capabilities.SetSupportedRxEhtMcsAndNss(EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_320_MHZ,
                                                    maxMcs,
                                                    maxMcs <= maxSupportedMcs ? 4 : 0);
            capabilities.SetSupportedTxEhtMcsAndNss(EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_320_MHZ,
                                                    maxMcs,
                                                    maxMcs <= maxSupportedMcs ? 3 : 0);
        }
    }
    else
    {
        capabilities.m_phyCapabilities.support320MhzIn6Ghz = 0;
    }

    return capabilities;
}

Buffer
WifiEhtCapabilitiesIeTest::SerializeIntoBuffer(const EhtCapabilities& ehtCapabilities)
{
    Buffer buffer;
    buffer.AddAtStart(ehtCapabilities.GetSerializedSize());
    ehtCapabilities.Serialize(buffer.Begin());
    return buffer;
}

void
WifiEhtCapabilitiesIeTest::CheckSerializedByte(const Buffer& buffer,
                                               uint32_t position,
                                               uint8_t value)
{
    Buffer::Iterator it = buffer.Begin();
    it.Next(position);
    uint8_t byte = it.ReadU8();
    NS_TEST_EXPECT_MSG_EQ(+byte, +value, "Unexpected byte at pos=" << position);
}

void
WifiEhtCapabilitiesIeTest::CheckEhtMacCapabilitiesInformation(const Buffer& buffer,
                                                              uint8_t expectedValueFirstByte)
{
    CheckSerializedByte(buffer, 3, expectedValueFirstByte);
    CheckSerializedByte(buffer, 4, 0x00);
}

void
WifiEhtCapabilitiesIeTest::CheckEhtPhyCapabilitiesInformation(const Buffer& buffer,
                                                              uint8_t expectedValueSixthByte)
{
    CheckSerializedByte(buffer, 5, (m_channelWidth == 320) ? 0x02 : 0x00);
    CheckSerializedByte(buffer, 6, 0x00);
    CheckSerializedByte(buffer, 7, 0x00);
    CheckSerializedByte(buffer, 8, 0x00);
    CheckSerializedByte(buffer, 9, 0x00);
    CheckSerializedByte(buffer, 10, expectedValueSixthByte);
    CheckSerializedByte(buffer, 11, 0x00);
    CheckSerializedByte(buffer, 12, 0x00);
    CheckSerializedByte(buffer, 13, 0x00);
}

void
WifiEhtCapabilitiesIeTest::CheckSupportedEhtMcsAndNssSet(const Buffer& buffer,
                                                         uint8_t maxSupportedMcs)
{
    if (m_channelWidth == 20)
    {
        CheckSerializedByte(buffer, 14, 0x21); // first byte of Supported EHT-MCS And NSS Set
        CheckSerializedByte(
            buffer,
            15,
            maxSupportedMcs >= 8 ? 0x21 : 0x00); // second byte of Supported EHT-MCS And NSS Set
        CheckSerializedByte(
            buffer,
            16,
            maxSupportedMcs >= 10 ? 0x21 : 0x00); // third byte of Supported EHT-MCS And NSS Set
        CheckSerializedByte(
            buffer,
            17,
            maxSupportedMcs >= 12 ? 0x21 : 0x00); // fourth byte of Supported EHT-MCS And NSS Set
    }
    else
    {
        CheckSerializedByte(buffer, 14, 0x43); // first byte of Supported EHT-MCS And NSS Set
        CheckSerializedByte(
            buffer,
            15,
            maxSupportedMcs >= 10 ? 0x43 : 0x00); // second byte of Supported EHT-MCS And NSS Set
        CheckSerializedByte(
            buffer,
            16,
            maxSupportedMcs >= 12 ? 0x43 : 0x00); // third byte of Supported EHT-MCS And NSS Set
    }
    if (m_channelWidth >= 160)
    {
        CheckSerializedByte(buffer, 17, 0x12); // first byte of EHT-MCS Map (BW = 160 MHz)
        CheckSerializedByte(
            buffer,
            18,
            maxSupportedMcs >= 10 ? 0x12 : 0x00); // second byte of EHT-MCS Map (BW = 160 MHz)
        CheckSerializedByte(
            buffer,
            19,
            maxSupportedMcs >= 12 ? 0x12 : 0x00); // third byte of EHT-MCS Map (BW = 160 MHz)
    }
    if (m_channelWidth == 320)
    {
        CheckSerializedByte(buffer, 20, 0x34); // first byte of EHT-MCS Map (BW = 320 MHz)
        CheckSerializedByte(
            buffer,
            21,
            maxSupportedMcs >= 10 ? 0x34 : 0x00); // second byte of EHT-MCS Map (BW = 320 MHz)
        CheckSerializedByte(
            buffer,
            22,
            maxSupportedMcs >= 12 ? 0x34 : 0x00); // third byte of EHT-MCS Map (BW = 320 MHz)
    }
}

void
WifiEhtCapabilitiesIeTest::DoRun()
{
    uint8_t maxMcs = 0;
    uint16_t expectedEhtMcsAndNssSetSize = 0;
    switch (m_channelWidth)
    {
    case 20:
        expectedEhtMcsAndNssSetSize = 4;
        break;
    case 40:
    case 80:
        expectedEhtMcsAndNssSetSize = 3;
        break;
    case 160:
        expectedEhtMcsAndNssSetSize = (2 * 3);
        break;
    case 320:
        expectedEhtMcsAndNssSetSize = (3 * 3);
        break;
    default:
        NS_ASSERT_MSG(false, "Invalid upper channel width " << m_channelWidth);
    }

    uint16_t expectedSize = 1 +                          // Element ID
                            1 +                          // Length
                            1 +                          // Element ID Extension
                            2 +                          // EHT MAC Capabilities Information
                            9 +                          // EHT PHY Capabilities Information
                            expectedEhtMcsAndNssSetSize; // Supported EHT-MCS And NSS Set

    auto mapType = m_channelWidth == 20 ? EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY
                                        : EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_NOT_LARGER_THAN_80_MHZ;

    {
        maxMcs = 11;
        HeCapabilities heCapabilities = GetHeCapabilities();
        EhtCapabilities ehtCapabilities = GetEhtCapabilities(3895, 65535, maxMcs);

        NS_ASSERT(ehtCapabilities.GetHighestSupportedRxMcs(mapType) == maxMcs);
        NS_ASSERT(ehtCapabilities.GetHighestSupportedTxMcs(mapType) == maxMcs);

        NS_TEST_EXPECT_MSG_EQ(ehtCapabilities.GetSerializedSize(),
                              expectedSize,
                              "Unexpected header size");

        Buffer buffer = SerializeIntoBuffer(ehtCapabilities);

        CheckEhtMacCapabilitiesInformation(buffer, 0x00);

        CheckEhtPhyCapabilitiesInformation(buffer, 0x00);

        CheckSupportedEhtMcsAndNssSet(buffer, maxMcs);

        TestHeaderSerialization(ehtCapabilities, m_is2_4Ghz, heCapabilities);
    }

    {
        maxMcs = 11;
        HeCapabilities heCapabilities = GetHeCapabilities();
        EhtCapabilities ehtCapabilities = GetEhtCapabilities(11454, 65535, maxMcs);

        NS_ASSERT(ehtCapabilities.GetHighestSupportedRxMcs(mapType) == maxMcs);
        NS_ASSERT(ehtCapabilities.GetHighestSupportedTxMcs(mapType) == maxMcs);

        NS_TEST_EXPECT_MSG_EQ(ehtCapabilities.GetSerializedSize(),
                              expectedSize,
                              "Unexpected header size");

        Buffer buffer = SerializeIntoBuffer(ehtCapabilities);

        CheckEhtMacCapabilitiesInformation(buffer, m_is2_4Ghz ? 0x80 : 0x00);

        CheckEhtPhyCapabilitiesInformation(buffer, 0x00);

        CheckSupportedEhtMcsAndNssSet(buffer, maxMcs);

        TestHeaderSerialization(ehtCapabilities, m_is2_4Ghz, heCapabilities);
    }

    {
        maxMcs = 13;
        HeCapabilities heCapabilities = GetHeCapabilities();
        EhtCapabilities ehtCapabilities = GetEhtCapabilities(3895, 65535, maxMcs);

        NS_ASSERT(ehtCapabilities.GetHighestSupportedRxMcs(mapType) == maxMcs);
        NS_ASSERT(ehtCapabilities.GetHighestSupportedTxMcs(mapType) == maxMcs);

        NS_TEST_EXPECT_MSG_EQ(ehtCapabilities.GetSerializedSize(),
                              expectedSize,
                              "Unexpected header size");

        Buffer buffer = SerializeIntoBuffer(ehtCapabilities);

        CheckEhtMacCapabilitiesInformation(buffer, 0x00);

        CheckEhtPhyCapabilitiesInformation(buffer, 0x06);

        CheckSupportedEhtMcsAndNssSet(buffer, maxMcs);

        TestHeaderSerialization(ehtCapabilities, m_is2_4Ghz, heCapabilities);
    }

    {
        maxMcs = 11;
        HeCapabilities heCapabilities = GetHeCapabilities();
        EhtCapabilities ehtCapabilities = GetEhtCapabilities(3895, 65535, maxMcs);

        NS_ASSERT(ehtCapabilities.GetHighestSupportedRxMcs(mapType) == maxMcs);
        NS_ASSERT(ehtCapabilities.GetHighestSupportedTxMcs(mapType) == maxMcs);

        std::vector<std::pair<uint8_t, uint8_t>> ppeThresholds;
        ppeThresholds.emplace_back(1, 2); // NSS1 242-tones RU
        ppeThresholds.emplace_back(2, 3); // NSS1 484-tones RU
        ppeThresholds.emplace_back(3, 4); // NSS2 242-tones RU
        ppeThresholds.emplace_back(4, 3); // NSS2 484-tones RU
        ppeThresholds.emplace_back(3, 2); // NSS3 242-tones RU
        ppeThresholds.emplace_back(2, 1); // NSS3 484-tones RU
        ehtCapabilities.SetPpeThresholds(2, 0x03, ppeThresholds);

        expectedSize += 6;

        NS_TEST_EXPECT_MSG_EQ(ehtCapabilities.GetSerializedSize(),
                              expectedSize,
                              "Unexpected header size");

        Buffer buffer = SerializeIntoBuffer(ehtCapabilities);

        CheckEhtMacCapabilitiesInformation(buffer, 0x00);

        CheckEhtPhyCapabilitiesInformation(buffer, 0x08);

        CheckSupportedEhtMcsAndNssSet(buffer, maxMcs);

        TestHeaderSerialization(ehtCapabilities, m_is2_4Ghz, heCapabilities);
    }
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test TID-To-Link Mapping information element serialization and deserialization
 */
class TidToLinkMappingElementTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     *
     * \tparam Args \deduced Template type parameter pack for the sequence of TID-Link mapping pairs
     * \param direction The direction for the TID-to-link mapping
     * \param args A sequence of TID-Link mapping pairs
     */
    template <typename... Args>
    TidToLinkMappingElementTest(TidLinkMapDir direction, Args&&... args);

    ~TidToLinkMappingElementTest() override = default;

  private:
    /**
     * Set the given TID-Link mapping and recursively call itself to set the remaining pairs.
     *
     * \tparam Args \deduced Template type parameter pack for the sequence of TID-Link mapping pairs
     * \param tid the TID
     * \param linkIds the IDs of the links on which the given TID is mapped
     * \param args A sequence of TID-Link mapping pairs
     */
    template <typename... Args>
    void SetLinkMapping(uint8_t tid, const std::list<uint8_t>& linkIds, Args&&... args);

    /**
     * Base case to stop the recursion performed by the templated version of this method.
     */
    void SetLinkMapping()
    {
    }

    void DoRun() override;

    TidToLinkMapping m_tidToLinkMapping; ///< TID-To-Link Mapping element
};

template <typename... Args>
TidToLinkMappingElementTest::TidToLinkMappingElementTest(TidLinkMapDir direction, Args&&... args)
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of TID-To-Link Mapping elements")
{
    m_tidToLinkMapping.m_control.direction = direction;
    m_tidToLinkMapping.m_control.defaultMapping = true;
    SetLinkMapping(args...);
}

template <typename... Args>
void
TidToLinkMappingElementTest::SetLinkMapping(uint8_t tid,
                                            const std::list<uint8_t>& linkIds,
                                            Args&&... args)
{
    m_tidToLinkMapping.m_control.defaultMapping = false;
    m_tidToLinkMapping.SetLinkMappingOfTid(tid, linkIds);
    SetLinkMapping(args...);
}

void
TidToLinkMappingElementTest::DoRun()
{
    TestHeaderSerialization(m_tidToLinkMapping);
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test EHT Operation information element serialization and deserialization
 */
class EhtOperationElementTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     *
     * \param params the EHT Operation Parameters field
     * \param rxMaxNss0_7 RX max NSS that supports EHT MCS 0-7
     * \param txMaxNss0_7 TX max NSS that supports EHT MCS 0-7
     * \param rxMaxNss8_9 RX max NSS that supports EHT MCS 8-9
     * \param txMaxNss8_9 TX max NSS that supports EHT MCS 8-9
     * \param rxMaxNss10_11 RX max NSS that supports EHT MCS 10-11
     * \param txMaxNss10_11 TX max NSS that supports EHT MCS 10-11
     * \param rxMaxNss12_13 RX max NSS that supports EHT MCS 12-13
     * \param txMaxNss12_13 TX max NSS that supports EHT MCS 12-13
     * \param opInfo the EHT Operation Information field
     */
    EhtOperationElementTest(const EhtOperation::EhtOpParams& params,
                            uint8_t rxMaxNss0_7,
                            uint8_t txMaxNss0_7,
                            uint8_t rxMaxNss8_9,
                            uint8_t txMaxNss8_9,
                            uint8_t rxMaxNss10_11,
                            uint8_t txMaxNss10_11,
                            uint8_t rxMaxNss12_13,
                            uint8_t txMaxNss12_13,
                            std::optional<EhtOperation::EhtOpInfo> opInfo);

    ~EhtOperationElementTest() override = default;

  private:
    void DoRun() override;

    EhtOperation m_ehtOperation; ///< EHT Operation element
};

EhtOperationElementTest::EhtOperationElementTest(const EhtOperation::EhtOpParams& params,
                                                 uint8_t rxMaxNss0_7,
                                                 uint8_t txMaxNss0_7,
                                                 uint8_t rxMaxNss8_9,
                                                 uint8_t txMaxNss8_9,
                                                 uint8_t rxMaxNss10_11,
                                                 uint8_t txMaxNss10_11,
                                                 uint8_t rxMaxNss12_13,
                                                 uint8_t txMaxNss12_13,
                                                 std::optional<EhtOperation::EhtOpInfo> opInfo)
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of EHT Operation elements")
{
    m_ehtOperation.m_params = params;
    m_ehtOperation.SetMaxRxNss(rxMaxNss0_7, 0, 7);
    m_ehtOperation.SetMaxTxNss(txMaxNss0_7, 0, 7);
    m_ehtOperation.SetMaxRxNss(rxMaxNss8_9, 8, 9);
    m_ehtOperation.SetMaxTxNss(txMaxNss8_9, 8, 9);
    m_ehtOperation.SetMaxRxNss(rxMaxNss10_11, 10, 11);
    m_ehtOperation.SetMaxTxNss(txMaxNss10_11, 10, 11);
    m_ehtOperation.SetMaxRxNss(rxMaxNss12_13, 12, 13);
    m_ehtOperation.SetMaxTxNss(txMaxNss12_13, 12, 13);
    m_ehtOperation.m_opInfo = opInfo;
}

void
EhtOperationElementTest::DoRun()
{
    TestHeaderSerialization(m_ehtOperation);
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi EHT Information Elements Test Suite
 */
class WifiEhtInfoElemsTestSuite : public TestSuite
{
  public:
    WifiEhtInfoElemsTestSuite();
};

WifiEhtInfoElemsTestSuite::WifiEhtInfoElemsTestSuite()
    : TestSuite("wifi-eht-info-elems", UNIT)
{
    AddTestCase(new BasicMultiLinkElementTest(), TestCase::QUICK);
    AddTestCase(new ReducedNeighborReportTest(), TestCase::QUICK);
    AddTestCase(new WifiEhtCapabilitiesIeTest(false, 20), TestCase::QUICK);
    AddTestCase(new WifiEhtCapabilitiesIeTest(true, 20), TestCase::QUICK);
    AddTestCase(new WifiEhtCapabilitiesIeTest(false, 80), TestCase::QUICK);
    AddTestCase(new WifiEhtCapabilitiesIeTest(true, 40), TestCase::QUICK);
    AddTestCase(new WifiEhtCapabilitiesIeTest(true, 80), TestCase::QUICK);
    AddTestCase(new WifiEhtCapabilitiesIeTest(false, 160), TestCase::QUICK);
    AddTestCase(new WifiEhtCapabilitiesIeTest(false, 320), TestCase::QUICK);
    AddTestCase(new TidToLinkMappingElementTest(TidLinkMapDir::DOWNLINK), TestCase::QUICK);
    AddTestCase(
        new TidToLinkMappingElementTest(TidLinkMapDir::UPLINK, 3, std::list<uint8_t>{0, 4, 6}),
        TestCase::QUICK);
    AddTestCase(new TidToLinkMappingElementTest(TidLinkMapDir::BOTH_DIRECTIONS,
                                                3,
                                                std::list<uint8_t>{0, 4, 6},
                                                6,
                                                std::list<uint8_t>{3, 7, 11, 14}),
                TestCase::QUICK);
    AddTestCase(new TidToLinkMappingElementTest(TidLinkMapDir::DOWNLINK,
                                                0,
                                                std::list<uint8_t>{0, 1, 2},
                                                1,
                                                std::list<uint8_t>{3, 4, 5},
                                                2,
                                                std::list<uint8_t>{6, 7},
                                                3,
                                                std::list<uint8_t>{8, 9, 10},
                                                4,
                                                std::list<uint8_t>{11, 12, 13},
                                                5,
                                                std::list<uint8_t>{14},
                                                6,
                                                std::list<uint8_t>{1, 3, 6},
                                                7,
                                                std::list<uint8_t>{11, 14}),
                TestCase::QUICK);
    AddTestCase(new EhtOperationElementTest({0, 0, 0, 0, 0}, 1, 2, 3, 4, 5, 6, 7, 8, std::nullopt),
                TestCase::QUICK);
    AddTestCase(new EhtOperationElementTest({1, 0, 0, 1, 0},
                                            1,
                                            2,
                                            3,
                                            4,
                                            5,
                                            6,
                                            7,
                                            8,
                                            EhtOperation::EhtOpInfo{{1}, 3, 5}),
                TestCase::QUICK);
    AddTestCase(new EhtOperationElementTest({1, 1, 1, 1, 2},
                                            1,
                                            2,
                                            3,
                                            4,
                                            5,
                                            6,
                                            7,
                                            8,
                                            EhtOperation::EhtOpInfo{{2}, 4, 6, 3000}),
                TestCase::QUICK);
}

static WifiEhtInfoElemsTestSuite g_wifiEhtInfoElemsTestSuite; ///< the test suite
