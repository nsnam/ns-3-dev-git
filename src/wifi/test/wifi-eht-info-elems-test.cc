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
}

static WifiEhtInfoElemsTestSuite g_wifiEhtInfoElemsTestSuite; ///< the test suite
