/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
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

#include "ns3/test.h"
#include "ns3/wifi-phy-operating-channel.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiRuAllocationTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the WifiPhyOperatingChannel::Get20MHzIndicesCoveringRu() method.
 */
class Wifi20MHzIndicesCoveringRuTest : public TestCase
{
  public:
    /**
     * Constructor
     */
    Wifi20MHzIndicesCoveringRuTest();
    ~Wifi20MHzIndicesCoveringRuTest() override = default;

    /**
     * Check that the indices of the 20 MHz channels covering the given RU as computed
     * by WifiPhyOperatingChannel::Get20MHzIndicesCoveringRu() are correct.
     *
     * \param primary20 the index of the primary20 channel to configure
     * \param ru the given RU
     * \param width the width in MHz of the channel to which the given RU refers to; normally,
     *              it is the width in MHz of the PPDU for which the RU is allocated
     * \param indices the expected indices
     */
    void RunOne(uint8_t primary20,
                HeRu::RuSpec ru,
                uint16_t width,
                const std::set<uint8_t>& indices);

  private:
    void DoRun() override;

    WifiPhyOperatingChannel m_channel; //!< operating channel
};

Wifi20MHzIndicesCoveringRuTest::Wifi20MHzIndicesCoveringRuTest()
    : TestCase("Check computation of the indices of the 20 MHz channels covering an RU")
{
}

void
Wifi20MHzIndicesCoveringRuTest::RunOne(uint8_t primary20,
                                       HeRu::RuSpec ru,
                                       uint16_t width,
                                       const std::set<uint8_t>& indices)
{
    auto printToStr = [](const std::set<uint8_t>& s) {
        std::stringstream ss;
        ss << "{";
        for (const auto& index : s)
        {
            ss << +index << " ";
        }
        ss << "}";
        return ss.str();
    };

    m_channel.SetPrimary20Index(primary20);

    auto actualIndices = m_channel.Get20MHzIndicesCoveringRu(ru, width);
    NS_TEST_ASSERT_MSG_EQ((actualIndices == indices),
                          true,
                          "Channel width=" << m_channel.GetWidth() << " MHz, PPDU width=" << width
                                           << " MHz, p20Index=" << +primary20 << " , RU=" << ru
                                           << ". Expected indices " << printToStr(indices)
                                           << " differs from actual " << printToStr(actualIndices));
}

void
Wifi20MHzIndicesCoveringRuTest::DoRun()
{
    /******************
     * 20 MHz channel *
     ******************/
    m_channel.SetDefault(20, WIFI_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);

    /* 20 MHz PPDU */
    {
        const uint16_t width = 20;
        const uint8_t p20Index = 0;

        // All the 9 26-tone RUs are covered by the unique 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {p20Index});
        }
        // All the 4 52-tone RUs are covered by the unique 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {p20Index});
        }
        // Both 106-tone RUs are covered by the unique 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {p20Index});
        }
        // The 242-tone RU is covered by the unique 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 1, true), width, {p20Index});
    }

    /******************
     * 40 MHz channel *
     ******************/
    m_channel.SetDefault(40, WIFI_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);

    /* 20 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 2; p20Index++)
    {
        const uint16_t width = 20;

        // All the 9 26-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {p20Index});
        }
        // All the 4 52-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {p20Index});
        }
        // Both 106-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {p20Index});
        }
        // The 242-tone RU is covered by the primary 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 1, true), width, {p20Index});
    }

    /* 40 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 2; p20Index++)
    {
        const uint16_t width = 40;

        // The first 9 26-tone RUs are covered by the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {0});
        }
        // The second 9 26-tone RUs are covered by the second 20 MHz channel
        for (std::size_t idx = 10; idx <= 18; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {1});
        }
        // The first 4 52-tone RUs are covered by the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {0});
        }
        // The second 4 52-tone RUs are covered by the second 20 MHz channel
        for (std::size_t idx = 5; idx <= 8; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {1});
        }
        // The first 2 106-tone RUs are covered by the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {0});
        }
        // The second 2 106-tone RUs are covered by the second 20 MHz channel
        for (std::size_t idx = 3; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {1});
        }
        // The first 242-tone RU is covered by the first 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 1, true), width, {0});
        // The second 242-tone RU is covered by the second 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 2, true), width, {1});
        // The 484-tone RU is covered by both 20 MHz channels
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_484_TONE, 1, true), width, {0, 1});
    }

    /******************
     * 80 MHz channel *
     ******************/
    m_channel.SetDefault(80, WIFI_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);

    /* 20 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 4; p20Index++)
    {
        const uint16_t width = 20;

        // All the 9 26-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {p20Index});
        }
        // All the 4 52-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {p20Index});
        }
        // Both 106-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {p20Index});
        }
        // The 242-tone RU is covered by the primary 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 1, true), width, {p20Index});
    }

    /* 40 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 4; p20Index++)
    {
        const uint16_t width = 40;
        // PPDU is transmitted on P40, which may be in the lower or higher 40 MHz
        const uint8_t p40Index = p20Index / 2;
        // RUs can be allocated in one (or both) of the two 20 MHz channels in P40
        const uint8_t ch20Index0 = p40Index * 2;
        const uint8_t ch20Index1 = p40Index * 2 + 1;

        // The first 9 26-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {ch20Index0});
        }
        // The second 9 26-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 10; idx <= 18; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {ch20Index1});
        }
        // The first 4 52-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {ch20Index0});
        }
        // The second 4 52-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 5; idx <= 8; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {ch20Index1});
        }
        // The first 2 106-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {ch20Index0});
        }
        // The second 2 106-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 3; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {ch20Index1});
        }
        // The first 242-tone RU is in the lower 20 MHz of the PPDU bandwidth
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 1, true), width, {ch20Index0});
        // The second 242-tone RU is in the higher 20 MHz of the PPDU bandwidth
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 2, true), width, {ch20Index1});
        // The 484-tone RU is covered by both 20 MHz channels
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_484_TONE, 1, true), width, {ch20Index0, ch20Index1});
    }

    /* 80 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 4; p20Index++)
    {
        const uint16_t width = 80;

        // The first 9 26-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {0});
        }
        // The second 9 26-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 10; idx <= 18; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {1});
        }
        // The center 26-tone RU is covered by the central 20 MHz channels
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, 19, true), width, {1, 2});
        // The following 9 26-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 20; idx <= 28; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {2});
        }
        // The last 9 26-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 29; idx <= 37; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {3});
        }
        // The first 4 52-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {0});
        }
        // The second 4 52-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 5; idx <= 8; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {1});
        }
        // The third 4 52-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 9; idx <= 12; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {2});
        }
        // The fourth 4 52-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 13; idx <= 16; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {3});
        }
        // The first 2 106-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {0});
        }
        // The second 2 106-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 3; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {1});
        }
        // The third 2 106-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 5; idx <= 6; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {2});
        }
        // The fourth 2 106-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 7; idx <= 8; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {3});
        }
        // The first 242-tone RU is in the first 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 1, true), width, {0});
        // The second 242-tone RU is in the second 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 2, true), width, {1});
        // The third 242-tone RU is in the third 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 3, true), width, {2});
        // The fourth 242-tone RU is in the fourth 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 4, true), width, {3});
        // The first 484-tone RU is covered by the first two 20 MHz channels
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_484_TONE, 1, true), width, {0, 1});
        // The second 484-tone RU is covered by the last two 20 MHz channels
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_484_TONE, 2, true), width, {2, 3});
        // The 996-tone RU is covered by all the 20 MHz channels
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_996_TONE, 1, true), width, {0, 1, 2, 3});
    }

    /******************
     * 160 MHz channel *
     ******************/
    m_channel.SetDefault(160, WIFI_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);

    /* 20 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 8; p20Index++)
    {
        const uint16_t width = 20;

        // All the 9 26-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {p20Index});
        }
        // All the 4 52-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {p20Index});
        }
        // Both 106-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {p20Index});
        }
        // The 242-tone RU is covered by the primary 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 1, true), width, {p20Index});
    }

    /* 40 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 8; p20Index++)
    {
        const uint16_t width = 40;
        // PPDU is transmitted on P40, which is one of the four 40 MHz channels
        const uint8_t p40Index = p20Index / 2;
        // RUs can be allocated in one (or both) of the two 20 MHz channels in P40
        const uint8_t ch20Index0 = p40Index * 2;
        const uint8_t ch20Index1 = p40Index * 2 + 1;

        // The first 9 26-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {ch20Index0});
        }
        // The second 9 26-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 10; idx <= 18; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {ch20Index1});
        }
        // The first 4 52-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {ch20Index0});
        }
        // The second 4 52-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 5; idx <= 8; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {ch20Index1});
        }
        // The first 2 106-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {ch20Index0});
        }
        // The second 2 106-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 3; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {ch20Index1});
        }
        // The first 242-tone RU is in the lower 20 MHz of the PPDU bandwidth
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 1, true), width, {ch20Index0});
        // The second 242-tone RU is in the higher 20 MHz of the PPDU bandwidth
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 2, true), width, {ch20Index1});
        // The 484-tone RU is covered by both 20 MHz channels
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_484_TONE, 1, true), width, {ch20Index0, ch20Index1});
    }

    /* 80 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 8; p20Index++)
    {
        const uint16_t width = 80;
        // PPDU is transmitted on P80, which is one of the two 80 MHz channels
        const uint8_t p80Index = p20Index / 4;
        // RUs can be allocated in one (or more) of the four 20 MHz channels in P80
        const uint8_t ch20Index0 = p80Index * 4;
        const uint8_t ch20Index1 = p80Index * 4 + 1;
        const uint8_t ch20Index2 = p80Index * 4 + 2;
        const uint8_t ch20Index3 = p80Index * 4 + 3;

        // The first 9 26-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {ch20Index0});
        }
        // The second 9 26-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 10; idx <= 18; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {ch20Index1});
        }
        // The center 26-tone RU is covered by the central 20 MHz channels
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, 19, true), width, {ch20Index1, ch20Index2});
        // The following 9 26-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 20; idx <= 28; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {ch20Index2});
        }
        // The last 9 26-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 29; idx <= 37; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_26_TONE, idx, true), width, {ch20Index3});
        }
        // The first 4 52-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {ch20Index0});
        }
        // The second 4 52-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 5; idx <= 8; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {ch20Index1});
        }
        // The third 4 52-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 9; idx <= 12; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {ch20Index2});
        }
        // The fourth 4 52-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 13; idx <= 16; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_52_TONE, idx, true), width, {ch20Index3});
        }
        // The first 2 106-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {ch20Index0});
        }
        // The second 2 106-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 3; idx <= 4; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {ch20Index1});
        }
        // The third 2 106-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 5; idx <= 6; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {ch20Index2});
        }
        // The fourth 2 106-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 7; idx <= 8; idx++)
        {
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_106_TONE, idx, true), width, {ch20Index3});
        }
        // The first 242-tone RU is in the first 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 1, true), width, {ch20Index0});
        // The second 242-tone RU is in the second 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 2, true), width, {ch20Index1});
        // The third 242-tone RU is in the third 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 3, true), width, {ch20Index2});
        // The fourth 242-tone RU is in the fourth 20 MHz channel
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 4, true), width, {ch20Index3});
        // The first 484-tone RU is covered by the first two 20 MHz channels
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_484_TONE, 1, true), width, {ch20Index0, ch20Index1});
        // The second 484-tone RU is covered by the last two 20 MHz channels
        RunOne(p20Index, HeRu::RuSpec(HeRu::RU_484_TONE, 2, true), width, {ch20Index2, ch20Index3});
        // The 996-tone RU is covered by all the 20 MHz channels
        RunOne(p20Index,
               HeRu::RuSpec(HeRu::RU_996_TONE, 1, true),
               width,
               {ch20Index0, ch20Index1, ch20Index2, ch20Index3});
    }

    /* 160 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 8; p20Index++)
    {
        const uint16_t width = 160;

        for (auto primary80MHz : {true, false})
        {
            // RUs can be allocated in one (or more) of the four 20 MHz channels in P80/S80
            // (depending on the primary80MHz flag)
            const uint8_t p80Index = (primary80MHz == (p20Index < 4)) ? 0 : 1;
            const uint8_t ch20Index0 = p80Index * 4;
            const uint8_t ch20Index1 = p80Index * 4 + 1;
            const uint8_t ch20Index2 = p80Index * 4 + 2;
            const uint8_t ch20Index3 = p80Index * 4 + 3;

            // The first 9 26-tone RUs are in the first 20 MHz channel
            for (std::size_t idx = 1; idx <= 9; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_26_TONE, idx, primary80MHz),
                       width,
                       {ch20Index0});
            }
            // The second 9 26-tone RUs are in the second 20 MHz channel
            for (std::size_t idx = 10; idx <= 18; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_26_TONE, idx, primary80MHz),
                       width,
                       {ch20Index1});
            }
            // The center 26-tone RU is covered by the central 20 MHz channels
            RunOne(p20Index,
                   HeRu::RuSpec(HeRu::RU_26_TONE, 19, primary80MHz),
                   width,
                   {ch20Index1, ch20Index2});
            // The following 9 26-tone RUs are in the third 20 MHz channel
            for (std::size_t idx = 20; idx <= 28; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_26_TONE, idx, primary80MHz),
                       width,
                       {ch20Index2});
            }
            // The last 9 26-tone RUs are in the fourth 20 MHz channel
            for (std::size_t idx = 29; idx <= 37; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_26_TONE, idx, primary80MHz),
                       width,
                       {ch20Index3});
            }
            // The first 4 52-tone RUs are in the first 20 MHz channel
            for (std::size_t idx = 1; idx <= 4; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_52_TONE, idx, primary80MHz),
                       width,
                       {ch20Index0});
            }
            // The second 4 52-tone RUs are in the second 20 MHz channel
            for (std::size_t idx = 5; idx <= 8; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_52_TONE, idx, primary80MHz),
                       width,
                       {ch20Index1});
            }
            // The third 4 52-tone RUs are in the third 20 MHz channel
            for (std::size_t idx = 9; idx <= 12; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_52_TONE, idx, primary80MHz),
                       width,
                       {ch20Index2});
            }
            // The fourth 4 52-tone RUs are in the fourth 20 MHz channel
            for (std::size_t idx = 13; idx <= 16; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_52_TONE, idx, primary80MHz),
                       width,
                       {ch20Index3});
            }
            // The first 2 106-tone RUs are in the first 20 MHz channel
            for (std::size_t idx = 1; idx <= 2; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_106_TONE, idx, primary80MHz),
                       width,
                       {ch20Index0});
            }
            // The second 2 106-tone RUs are in the second 20 MHz channel
            for (std::size_t idx = 3; idx <= 4; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_106_TONE, idx, primary80MHz),
                       width,
                       {ch20Index1});
            }
            // The third 2 106-tone RUs are in the third 20 MHz channel
            for (std::size_t idx = 5; idx <= 6; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_106_TONE, idx, primary80MHz),
                       width,
                       {ch20Index2});
            }
            // The fourth 2 106-tone RUs are in the fourth 20 MHz channel
            for (std::size_t idx = 7; idx <= 8; idx++)
            {
                RunOne(p20Index,
                       HeRu::RuSpec(HeRu::RU_106_TONE, idx, primary80MHz),
                       width,
                       {ch20Index3});
            }
            // The first 242-tone RU is in the first 20 MHz channel
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 1, primary80MHz), width, {ch20Index0});
            // The second 242-tone RU is in the second 20 MHz channel
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 2, primary80MHz), width, {ch20Index1});
            // The third 242-tone RU is in the third 20 MHz channel
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 3, primary80MHz), width, {ch20Index2});
            // The fourth 242-tone RU is in the fourth 20 MHz channel
            RunOne(p20Index, HeRu::RuSpec(HeRu::RU_242_TONE, 4, primary80MHz), width, {ch20Index3});
            // The first 484-tone RU is covered by the first two 20 MHz channels
            RunOne(p20Index,
                   HeRu::RuSpec(HeRu::RU_484_TONE, 1, primary80MHz),
                   width,
                   {ch20Index0, ch20Index1});
            // The second 484-tone RU is covered by the last two 20 MHz channels
            RunOne(p20Index,
                   HeRu::RuSpec(HeRu::RU_484_TONE, 2, primary80MHz),
                   width,
                   {ch20Index2, ch20Index3});
            // The 996-tone RU is covered by all the 20 MHz channels
            RunOne(p20Index,
                   HeRu::RuSpec(HeRu::RU_996_TONE, 1, primary80MHz),
                   width,
                   {ch20Index0, ch20Index1, ch20Index2, ch20Index3});
        }
        // The 2x996-tone RU is covered by all the eight 20 MHz channels
        RunOne(p20Index,
               HeRu::RuSpec(HeRu::RU_2x996_TONE, 1, true),
               width,
               {0, 1, 2, 3, 4, 5, 6, 7});
    }
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi primary channels test suite
 */
class WifiRuAllocationTestSuite : public TestSuite
{
  public:
    WifiRuAllocationTestSuite();
};

WifiRuAllocationTestSuite::WifiRuAllocationTestSuite()
    : TestSuite("wifi-ru-allocation", Type::UNIT)
{
    AddTestCase(new Wifi20MHzIndicesCoveringRuTest(), TestCase::Duration::QUICK);
}

static WifiRuAllocationTestSuite g_wifiRuAllocationTestSuite; ///< the test suite
