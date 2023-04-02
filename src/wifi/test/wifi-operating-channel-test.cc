/*
 * Copyright (c) 2023
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/test.h"
#include "ns3/wifi-phy-operating-channel.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiOperatingChannelTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the WifiPhyOperatingChannel::Set() method.
 */
class SetWifiOperatingChannelTest : public TestCase
{
  public:
    /**
     * Constructor
     */
    SetWifiOperatingChannelTest();
    ~SetWifiOperatingChannelTest() override = default;

  private:
    void DoRun() override;

    /**
     * Run one function.
     * \param runInfo the string that indicates info about the test case to run
     * \param segments the info about each frequency segment to set for the operating channel
     * \param standard the 802.11 standard to consider for the test
     * \param band the PHY band to consider for the test
     * \param expectExceptionThrown flag to indicate whether an exception is expected to be thrown
     * \param expectedWidth the expected width type of the operating channel
     * \param expectedSegments the info about the expected frequency segments of the operating
     * channel
     */
    void RunOne(const std::string& runInfo,
                const std::vector<FrequencyChannelInfo>& segments,
                WifiStandard standard,
                WifiPhyBand band,
                bool expectExceptionThrown,
                WifiChannelWidthType expectedWidth = WifiChannelWidthType::UNKNOWN,
                const std::vector<FrequencyChannelInfo>& expectedSegments = {});

    WifiPhyOperatingChannel m_channel; //!< operating channel
};

SetWifiOperatingChannelTest::SetWifiOperatingChannelTest()
    : TestCase("Check configuration of the operating channel")
{
}

void
SetWifiOperatingChannelTest::RunOne(const std::string& runInfo,
                                    const std::vector<FrequencyChannelInfo>& segments,
                                    WifiStandard standard,
                                    WifiPhyBand band,
                                    bool expectExceptionThrown,
                                    WifiChannelWidthType expectedWidth,
                                    const std::vector<FrequencyChannelInfo>& expectedSegments)
{
    NS_LOG_FUNCTION(this << runInfo);

    bool exceptionThrown = false;
    try
    {
        m_channel.Set(segments, standard);
    }
    catch (const std::runtime_error&)
    {
        exceptionThrown = true;
    }
    NS_TEST_ASSERT_MSG_EQ(exceptionThrown,
                          expectExceptionThrown,
                          "Exception thrown mismatch for run: " << runInfo);
    if (!exceptionThrown)
    {
        NS_TEST_ASSERT_MSG_EQ(
            m_channel.GetWidthType(),
            expectedWidth,
            "Operating channel has an incorrect channel width type for run: " << runInfo);
        NS_TEST_ASSERT_MSG_EQ(m_channel.GetNSegments(),
                              expectedSegments.size(),
                              "Incorrect number of frequency segments for run: " << runInfo);
        for (std::size_t i = 0; i < m_channel.GetNSegments(); ++i)
        {
            const auto& frequencyChannelInfo = expectedSegments.at(i);
            NS_TEST_ASSERT_MSG_EQ(m_channel.GetNumber(i),
                                  frequencyChannelInfo.number,
                                  "Operating channel has an incorrect channel number at segment "
                                      << i << " for run: " << runInfo);
            NS_TEST_ASSERT_MSG_EQ(m_channel.GetFrequency(i),
                                  frequencyChannelInfo.frequency,
                                  "Operating channel has an incorrect center frequency at segment "
                                      << i << " for run: " << runInfo);
            NS_TEST_ASSERT_MSG_EQ(m_channel.GetWidth(i),
                                  frequencyChannelInfo.width,
                                  "Operating channel has an incorrect channel width at segment "
                                      << i << " for run: " << runInfo);
            NS_TEST_ASSERT_MSG_EQ(m_channel.GetPhyBand(),
                                  frequencyChannelInfo.band,
                                  "Operating channel has an incorrect band for run: " << runInfo);
        }
    }
}

void
SetWifiOperatingChannelTest::DoRun()
{
    RunOne("dummy channel with all inputs unset",
           {{}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           true);

    RunOne("default 20 MHz OFDM channel operating on channel 36",
           {{36, 0, 20, WIFI_PHY_BAND_5GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           false,
           WifiChannelWidthType::CW_20MHZ,
           {{36, 5180, 20, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM}});

    RunOne("default 40 MHz OFDM channel operating on channel 38",
           {{38, 0, 40, WIFI_PHY_BAND_5GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           false,
           WifiChannelWidthType::CW_40MHZ,
           {{38, 5190, 40, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM}});

    RunOne("default 80 MHz OFDM channel operating on channel 42",
           {{42, 0, 80, WIFI_PHY_BAND_5GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           false,
           WifiChannelWidthType::CW_80MHZ,
           {{42, 5210, 80, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM}});

    RunOne("default 160 MHz (contiguous) OFDM channel operating on channel 50",
           {{50, 0, 160, WIFI_PHY_BAND_5GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           false,
           WifiChannelWidthType::CW_160MHZ,
           {{50, 5250, 160, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM}});

    RunOne("valid 80+80 MHz (non-contiguous) OFDM channel operating on channels 42 and 106",
           {{42, 0, 80, WIFI_PHY_BAND_5GHZ}, {106, 0, 80, WIFI_PHY_BAND_5GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           false,
           WifiChannelWidthType::CW_80_PLUS_80MHZ,
           {{42, 5210, 80, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
            {106, 5530, 80, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM}});

    RunOne("invalid 80+80 MHz (non-contiguous) OFDM channel higher channel not being 80 MHz",
           {{42, 0, 80, WIFI_PHY_BAND_5GHZ}, {102, 0, 80, WIFI_PHY_BAND_5GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           true);

    RunOne("invalid 80+80 MHz (non-contiguous) OFDM channel lower channel not being 80 MHz",
           {{36, 0, 20, WIFI_PHY_BAND_5GHZ}, {106, 0, 80, WIFI_PHY_BAND_5GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           true);

    RunOne("invalid 80+80 MHz (non-contiguous) OFDM channel with both segments configured on the "
           "same channel",
           {{42, 0, 80, WIFI_PHY_BAND_5GHZ}, {42, 0, 80, WIFI_PHY_BAND_5GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           true);

    RunOne("invalid 80+80 MHz (non-contiguous) OFDM channel with segments configured to be "
           "contiguous (lower before higher)",
           {{42, 0, 80, WIFI_PHY_BAND_5GHZ}, {58, 0, 80, WIFI_PHY_BAND_5GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           true);

    RunOne("invalid 80+80 MHz (non-contiguous) OFDM channel with segments configured to be "
           "contiguous (higher before lower)",
           {{58, 0, 80, WIFI_PHY_BAND_5GHZ}, {42, 0, 80, WIFI_PHY_BAND_5GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           true);

    RunOne("invalid 80+80 MHz (non-contiguous) OFDM channel with each segments configured on a "
           "different band",
           {{42, 0, 80, WIFI_PHY_BAND_5GHZ}, {215, 0, 80, WIFI_PHY_BAND_6GHZ}},
           WIFI_STANDARD_UNSPECIFIED,
           WIFI_PHY_BAND_UNSPECIFIED,
           true);
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi operating channel test suite
 */
class WifiOperatingChannelTestSuite : public TestSuite
{
  public:
    WifiOperatingChannelTestSuite();
};

WifiOperatingChannelTestSuite::WifiOperatingChannelTestSuite()
    : TestSuite("wifi-operating-channel", UNIT)
{
    AddTestCase(new SetWifiOperatingChannelTest(), TestCase::QUICK);
}

static WifiOperatingChannelTestSuite g_wifiOperatingChannelTestSuite; ///< the test suite
