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

#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/mgt-headers.h"
#include "ns3/multi-link-element.h"
#include "ns3/reduced-neighbor-report.h"
#include "ns3/wifi-phy-operating-channel.h"

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

    WifiMacType m_frameType; //!< the type of frame possibly included in the MLE
};

BasicMultiLinkElementTest::BasicMultiLinkElementTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of Basic variant Multi-Link elements"),
      m_frameType(WIFI_MAC_MGT_ASSOCIATION_REQUEST)
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
    MultiLinkElement mle(MultiLinkElement::BASIC_VARIANT, m_frameType);
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
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {}), m_frameType);

    commonInfo.m_linkIdInfo = 3;

    // Adding Link ID Info
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {}), m_frameType);

    commonInfo.m_bssParamsChangeCount = 1;

    // Adding BSS Paraemeters Change Count
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {}), m_frameType);

    commonInfo.m_mediumSyncDelayInfo =
        CommonInfoBasicMle::MediumSyncDelayInfo{.mediumSyncDuration = 1,
                                                .mediumSyncOfdmEdThreshold = 4,
                                                .mediumSyncMaxNTxops = 5};

    // Adding Medium Sync Delay Information
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {}), m_frameType);

    SupportedRates rates;
    rates.AddSupportedRate(6);
    rates.AddSupportedRate(12);
    rates.AddSupportedRate(24);

    CapabilityInformation capabilities;
    capabilities.SetShortPreamble(true);
    capabilities.SetShortSlotTime(true);
    capabilities.SetEss();

    MgtAssocRequestHeader assoc;
    assoc.SetSsid(Ssid("MySsid"));
    assoc.SetSupportedRates(rates);
    assoc.SetCapabilities(capabilities);
    assoc.SetListenInterval(0);

    MultiLinkElement::PerStaProfileSubelement perStaProfile(MultiLinkElement::BASIC_VARIANT,
                                                            m_frameType);
    perStaProfile.SetLinkId(5);
    perStaProfile.SetCompleteProfile();
    perStaProfile.SetStaMacAddress(Mac48Address("ba:98:76:54:32:10"));
    perStaProfile.SetAssocRequest(assoc);

    // Adding Per-STA Profile Subelement
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {perStaProfile}), m_frameType);

    // Adding two Per-STA Profile Subelements
    TestHeaderSerialization(GetMultiLinkElement(commonInfo, {perStaProfile, perStaProfile}),
                            m_frameType);
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
}

static WifiEhtInfoElemsTestSuite g_wifiEhtInfoElemsTestSuite; ///< the test suite
