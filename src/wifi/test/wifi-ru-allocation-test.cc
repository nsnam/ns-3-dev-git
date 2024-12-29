/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/test.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-ru.h"
#include "ns3/wifi-utils.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiRuAllocationTest");

namespace
{
/**
 * Create an HE or an EHT RU Specification.
 * If a primary80 is provided, an HE RU Specification is created, other it is an EHT RU
 * Specification.
 *
 * @param ruType the RU type
 * @param index the RU index (starting at 1)
 * @param primaryOrLow80MHz whether the RU is allocated in the primary 80MHz channel or in the low
 * 80 MHz if the RU is allocated in the secondary 160 MHz
 * @param primary160MHz whether the RU is allocated in the primary 160MHz channel (only for EHT)
 * @return the created RU Specification.
 */
WifiRu::RuSpec
MakeRuSpec(RuType ruType,
           std::size_t index,
           bool primaryOrLow80MHz,
           std::optional<bool> primary160MHz = std::nullopt)
{
    if (!primary160MHz)
    {
        return HeRu::RuSpec{ruType, index, primaryOrLow80MHz};
    }
    return EhtRu::RuSpec{ruType, index, *primary160MHz, primaryOrLow80MHz};
}

} // namespace

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the WifiPhyOperatingChannel::Get20MHzIndicesCoveringRu() method.
 */
class Wifi20MHzIndicesCoveringRuTest : public TestCase
{
  public:
    /**
     * Constructor
     * @param standard the standard to use for the test
     */
    Wifi20MHzIndicesCoveringRuTest(WifiStandard standard);
    ~Wifi20MHzIndicesCoveringRuTest() override = default;

    /**
     * Check that the indices of the 20 MHz channels covering the given RU as computed
     * by WifiPhyOperatingChannel::Get20MHzIndicesCoveringRu() are correct.
     *
     * @param primary20 the index of the primary20 channel to configure
     * @param ru the given RU
     * @param width the width of the channel to which the given RU refers to; normally, it is the
     * width of the PPDU for which the RU is allocated
     * @param indices the expected indices
     */
    void RunOne(uint8_t primary20,
                WifiRu::RuSpec ru,
                MHz_u width,
                const std::set<uint8_t>& indices);

  private:
    void DoRun() override;

    WifiStandard m_standard;           ///< The standard to use for the test
    WifiPhyOperatingChannel m_channel; //!< operating channel
};

Wifi20MHzIndicesCoveringRuTest::Wifi20MHzIndicesCoveringRuTest(WifiStandard standard)
    : TestCase("Check computation of the indices of the 20 MHz channels covering an RU for " +
               std::string((standard == WIFI_STANDARD_80211ax) ? "11ax" : "11be")),
      m_standard(standard)
{
}

void
Wifi20MHzIndicesCoveringRuTest::RunOne(uint8_t primary20,
                                       WifiRu::RuSpec ru,
                                       MHz_u width,
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
    NS_TEST_EXPECT_MSG_EQ((actualIndices == indices),
                          true,
                          "Channel width=" << m_channel.GetWidth() << ", PPDU width=" << width
                                           << ", p20Index=" << +primary20 << " , RU=" << ru
                                           << ". Expected indices " << printToStr(indices)
                                           << " differs from actual " << printToStr(actualIndices));
}

void
Wifi20MHzIndicesCoveringRuTest::DoRun()
{
    const auto p80{true};
    const auto p160{(m_standard == WIFI_STANDARD_80211be) ? std::optional(true) : std::nullopt};

    /******************
     * 20 MHz channel *
     ******************/
    m_channel.SetDefault(MHz_u{20}, m_standard, WIFI_PHY_BAND_6GHZ);

    /* 20 MHz PPDU */
    {
        const MHz_u width{20};
        const uint8_t p20Index = 0;

        // All the 9 26-tone RUs are covered by the unique 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {p20Index});
        }
        // All the 4 52-tone RUs are covered by the unique 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {p20Index});
        }
        // Both 106-tone RUs are covered by the unique 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {p20Index});
        }
        // The 242-tone RU is covered by the unique 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160), width, {p20Index});
    }

    /******************
     * 40 MHz channel *
     ******************/
    m_channel.SetDefault(MHz_u{40}, m_standard, WIFI_PHY_BAND_6GHZ);

    /* 20 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 2; p20Index++)
    {
        const MHz_u width{20};

        // All the 9 26-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {p20Index});
        }
        // All the 4 52-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {p20Index});
        }
        // Both 106-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {p20Index});
        }
        // The 242-tone RU is covered by the primary 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160), width, {p20Index});
    }

    /* 40 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 2; p20Index++)
    {
        const MHz_u width{40};

        // The first 9 26-tone RUs are covered by the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {0});
        }
        // The second 9 26-tone RUs are covered by the second 20 MHz channel
        for (std::size_t idx = 10; idx <= 18; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {1});
        }
        // The first 4 52-tone RUs are covered by the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {0});
        }
        // The second 4 52-tone RUs are covered by the second 20 MHz channel
        for (std::size_t idx = 5; idx <= 8; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {1});
        }
        // The first 2 106-tone RUs are covered by the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {0});
        }
        // The second 2 106-tone RUs are covered by the second 20 MHz channel
        for (std::size_t idx = 3; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {1});
        }
        // The first 242-tone RU is covered by the first 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160), width, {0});
        // The second 242-tone RU is covered by the second 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 2, p80, p160), width, {1});
        // The 484-tone RU is covered by both 20 MHz channels
        RunOne(p20Index, MakeRuSpec(RuType::RU_484_TONE, 1, p80, p160), width, {0, 1});
    }

    /******************
     * 80 MHz channel *
     ******************/
    m_channel.SetDefault(MHz_u{80}, m_standard, WIFI_PHY_BAND_6GHZ);

    /* 20 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 4; p20Index++)
    {
        const MHz_u width{20};

        // All the 9 26-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {p20Index});
        }
        // All the 4 52-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {p20Index});
        }
        // Both 106-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {p20Index});
        }
        // The 242-tone RU is covered by the primary 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160), width, {p20Index});
    }

    /* 40 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 4; p20Index++)
    {
        const MHz_u width{40};
        // PPDU is transmitted on P40, which may be in the lower or higher 40 MHz
        const uint8_t p40Index = p20Index / 2;
        // RUs can be allocated in one (or both) of the two 20 MHz channels in P40
        const uint8_t ch20Index0 = p40Index * 2;
        const uint8_t ch20Index1 = p40Index * 2 + 1;

        // The first 9 26-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {ch20Index0});
        }
        // The second 9 26-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 10; idx <= 18; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {ch20Index1});
        }
        // The first 4 52-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {ch20Index0});
        }
        // The second 4 52-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 5; idx <= 8; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {ch20Index1});
        }
        // The first 2 106-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {ch20Index0});
        }
        // The second 2 106-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 3; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {ch20Index1});
        }
        // The first 242-tone RU is in the lower 20 MHz of the PPDU bandwidth
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160), width, {ch20Index0});
        // The second 242-tone RU is in the higher 20 MHz of the PPDU bandwidth
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 2, p80, p160), width, {ch20Index1});
        // The 484-tone RU is covered by both 20 MHz channels
        RunOne(p20Index,
               MakeRuSpec(RuType::RU_484_TONE, 1, p80, p160),
               width,
               {ch20Index0, ch20Index1});
    }

    /* 80 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 4; p20Index++)
    {
        const MHz_u width{80};

        // The first 9 26-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {0});
        }
        // The second 9 26-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 10; idx <= 18; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {1});
        }
        if (m_standard == WIFI_STANDARD_80211ax)
        {
            // The center 26-tone RU is covered by the central 20 MHz channels
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, 19, p80, p160), width, {1, 2});
        }
        // The following 9 26-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 20; idx <= 28; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {2});
        }
        // The last 9 26-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 29; idx <= 37; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {3});
        }
        // The first 4 52-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {0});
        }
        // The second 4 52-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 5; idx <= 8; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {1});
        }
        // The third 4 52-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 9; idx <= 12; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {2});
        }
        // The fourth 4 52-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 13; idx <= 16; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {3});
        }
        // The first 2 106-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {0});
        }
        // The second 2 106-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 3; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {1});
        }
        // The third 2 106-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 5; idx <= 6; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {2});
        }
        // The fourth 2 106-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 7; idx <= 8; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {3});
        }
        // The first 242-tone RU is in the first 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160), width, {0});
        // The second 242-tone RU is in the second 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 2, p80, p160), width, {1});
        // The third 242-tone RU is in the third 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 3, p80, p160), width, {2});
        // The fourth 242-tone RU is in the fourth 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 4, p80, p160), width, {3});
        // The first 484-tone RU is covered by the first two 20 MHz channels
        RunOne(p20Index, MakeRuSpec(RuType::RU_484_TONE, 1, p80, p160), width, {0, 1});
        // The second 484-tone RU is covered by the last two 20 MHz channels
        RunOne(p20Index, MakeRuSpec(RuType::RU_484_TONE, 2, p80, p160), width, {2, 3});
        // The 996-tone RU is covered by all the 20 MHz channels
        RunOne(p20Index, MakeRuSpec(RuType::RU_996_TONE, 1, p80, p160), width, {0, 1, 2, 3});
    }

    /******************
     * 160 MHz channel *
     ******************/
    m_channel.SetDefault(MHz_u{160}, m_standard, WIFI_PHY_BAND_6GHZ);

    /* 20 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 8; p20Index++)
    {
        const MHz_u width{20};

        // All the 9 26-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {p20Index});
        }
        // All the 4 52-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {p20Index});
        }
        // Both 106-tone RUs are covered by the primary 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {p20Index});
        }
        // The 242-tone RU is covered by the primary 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160), width, {p20Index});
    }

    /* 40 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 8; p20Index++)
    {
        const MHz_u width{40};
        // PPDU is transmitted on P40, which is one of the four 40 MHz channels
        const uint8_t p40Index = p20Index / 2;
        // RUs can be allocated in one (or both) of the two 20 MHz channels in P40
        const uint8_t ch20Index0 = p40Index * 2;
        const uint8_t ch20Index1 = p40Index * 2 + 1;

        // The first 9 26-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 9; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {ch20Index0});
        }
        // The second 9 26-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 10; idx <= 18; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {ch20Index1});
        }
        // The first 4 52-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {ch20Index0});
        }
        // The second 4 52-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 5; idx <= 8; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {ch20Index1});
        }
        // The first 2 106-tone RUs are in the lower 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {ch20Index0});
        }
        // The second 2 106-tone RUs are in the higher 20 MHz of the PPDU bandwidth
        for (std::size_t idx = 3; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {ch20Index1});
        }
        // The first 242-tone RU is in the lower 20 MHz of the PPDU bandwidth
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160), width, {ch20Index0});
        // The second 242-tone RU is in the higher 20 MHz of the PPDU bandwidth
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 2, p80, p160), width, {ch20Index1});
        // The 484-tone RU is covered by both 20 MHz channels
        RunOne(p20Index,
               MakeRuSpec(RuType::RU_484_TONE, 1, p80, p160),
               width,
               {ch20Index0, ch20Index1});
    }

    /* 80 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 8; p20Index++)
    {
        const MHz_u width{80};
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
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {ch20Index0});
        }
        // The second 9 26-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 10; idx <= 18; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {ch20Index1});
        }
        if (m_standard == WIFI_STANDARD_80211ax)
        {
            // The center 26-tone RU is covered by the central 20 MHz channels
            RunOne(p20Index,
                   MakeRuSpec(RuType::RU_26_TONE, 19, p80, p160),
                   width,
                   {ch20Index1, ch20Index2});
        }
        // The following 9 26-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 20; idx <= 28; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {ch20Index2});
        }
        // The last 9 26-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 29; idx <= 37; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_26_TONE, idx, p80, p160), width, {ch20Index3});
        }
        // The first 4 52-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {ch20Index0});
        }
        // The second 4 52-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 5; idx <= 8; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {ch20Index1});
        }
        // The third 4 52-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 9; idx <= 12; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {ch20Index2});
        }
        // The fourth 4 52-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 13; idx <= 16; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_52_TONE, idx, p80, p160), width, {ch20Index3});
        }
        // The first 2 106-tone RUs are in the first 20 MHz channel
        for (std::size_t idx = 1; idx <= 2; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {ch20Index0});
        }
        // The second 2 106-tone RUs are in the second 20 MHz channel
        for (std::size_t idx = 3; idx <= 4; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {ch20Index1});
        }
        // The third 2 106-tone RUs are in the third 20 MHz channel
        for (std::size_t idx = 5; idx <= 6; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {ch20Index2});
        }
        // The fourth 2 106-tone RUs are in the fourth 20 MHz channel
        for (std::size_t idx = 7; idx <= 8; idx++)
        {
            RunOne(p20Index, MakeRuSpec(RuType::RU_106_TONE, idx, p80, p160), width, {ch20Index3});
        }
        // The first 242-tone RU is in the first 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160), width, {ch20Index0});
        // The second 242-tone RU is in the second 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 2, p80, p160), width, {ch20Index1});
        // The third 242-tone RU is in the third 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 3, p80, p160), width, {ch20Index2});
        // The fourth 242-tone RU is in the fourth 20 MHz channel
        RunOne(p20Index, MakeRuSpec(RuType::RU_242_TONE, 4, p80, p160), width, {ch20Index3});
        // The first 484-tone RU is covered by the first two 20 MHz channels
        RunOne(p20Index,
               MakeRuSpec(RuType::RU_484_TONE, 1, p80, p160),
               width,
               {ch20Index0, ch20Index1});
        // The second 484-tone RU is covered by the last two 20 MHz channels
        RunOne(p20Index,
               MakeRuSpec(RuType::RU_484_TONE, 2, p80, p160),
               width,
               {ch20Index2, ch20Index3});
        // The 996-tone RU is covered by all the 20 MHz channels
        RunOne(p20Index,
               MakeRuSpec(RuType::RU_996_TONE, 1, p80, p160),
               width,
               {ch20Index0, ch20Index1, ch20Index2, ch20Index3});
    }

    /* 160 MHz PPDU */
    for (uint8_t p20Index = 0; p20Index < 8; p20Index++)
    {
        const MHz_u width{160};

        for (auto primary80 : {true, false})
        {
            // RUs can be allocated in one (or more) of the four 20 MHz channels in P80/S80
            // (depending on the primary80 flag)
            const uint8_t p80Index = (primary80 == (p20Index < 4)) ? 0 : 1;
            const uint8_t ch20Index0 = p80Index * 4;
            const uint8_t ch20Index1 = p80Index * 4 + 1;
            const uint8_t ch20Index2 = p80Index * 4 + 2;
            const uint8_t ch20Index3 = p80Index * 4 + 3;

            // The first 9 26-tone RUs are in the first 20 MHz channel
            std::size_t startIdx = 1;
            std::size_t stopIdx = startIdx + 8;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_26_TONE, idx, primary80, p160),
                       width,
                       {ch20Index0});
            }
            // The second 9 26-tone RUs are in the second 20 MHz channel
            startIdx = stopIdx + 1;
            stopIdx = startIdx + 8;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_26_TONE, idx, primary80, p160),
                       width,
                       {ch20Index1});
            }
            if (m_standard == WIFI_STANDARD_80211ax)
            {
                // The center 26-tone RU is covered by the central 20 MHz channels
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_26_TONE, 19, primary80, p160),
                       width,
                       {ch20Index1, ch20Index2});
            }
            // The following 9 26-tone RUs are in the third 20 MHz channel
            startIdx = stopIdx + 2;
            stopIdx = startIdx + 8;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_26_TONE, idx, primary80, p160),
                       width,
                       {ch20Index2});
            }
            // The last 9 26-tone RUs are in the fourth 20 MHz channel
            startIdx = stopIdx + 1;
            stopIdx = startIdx + 8;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_26_TONE, idx, primary80, p160),
                       width,
                       {ch20Index3});
            }
            // The first 4 52-tone RUs are in the first 20 MHz channel
            startIdx = 1;
            stopIdx = startIdx + 3;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_52_TONE, idx, primary80, p160),
                       width,
                       {ch20Index0});
            }
            // The second 4 52-tone RUs are in the second 20 MHz channel
            startIdx = stopIdx + 1;
            stopIdx = startIdx + 3;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_52_TONE, idx, primary80, p160),
                       width,
                       {ch20Index1});
            }
            // The third 4 52-tone RUs are in the third 20 MHz channel
            startIdx = stopIdx + 1;
            stopIdx = startIdx + 3;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_52_TONE, idx, primary80, p160),
                       width,
                       {ch20Index2});
            }
            // The fourth 4 52-tone RUs are in the fourth 20 MHz channel
            startIdx = stopIdx + 1;
            stopIdx = startIdx + 3;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_52_TONE, idx, primary80, p160),
                       width,
                       {ch20Index3});
            }
            // The first 2 106-tone RUs are in the first 20 MHz channel
            startIdx = 1;
            stopIdx = startIdx + 1;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_106_TONE, idx, primary80, p160),
                       width,
                       {ch20Index0});
            }
            // The second 2 106-tone RUs are in the second 20 MHz channel
            startIdx = stopIdx + 1;
            stopIdx = startIdx + 1;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_106_TONE, idx, primary80, p160),
                       width,
                       {ch20Index1});
            }
            // The third 2 106-tone RUs are in the third 20 MHz channel
            startIdx = stopIdx + 1;
            stopIdx = startIdx + 1;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_106_TONE, idx, primary80, p160),
                       width,
                       {ch20Index2});
            }
            // The fourth 2 106-tone RUs are in the fourth 20 MHz channel
            startIdx = stopIdx + 1;
            stopIdx = startIdx + 1;
            for (std::size_t idx = startIdx; idx <= stopIdx; idx++)
            {
                RunOne(p20Index,
                       MakeRuSpec(RuType::RU_106_TONE, idx, primary80, p160),
                       width,
                       {ch20Index3});
            }
            // The first 242-tone RU is in the first 20 MHz channel
            auto idx = 1;
            RunOne(p20Index,
                   MakeRuSpec(RuType::RU_242_TONE, idx++, primary80, p160),
                   width,
                   {ch20Index0});
            // The second 242-tone RU is in the second 20 MHz channel
            RunOne(p20Index,
                   MakeRuSpec(RuType::RU_242_TONE, idx++, primary80, p160),
                   width,
                   {ch20Index1});
            // The third 242-tone RU is in the third 20 MHz channel
            RunOne(p20Index,
                   MakeRuSpec(RuType::RU_242_TONE, idx++, primary80, p160),
                   width,
                   {ch20Index2});
            // The fourth 242-tone RU is in the fourth 20 MHz channel
            RunOne(p20Index,
                   MakeRuSpec(RuType::RU_242_TONE, idx, primary80, p160),
                   width,
                   {ch20Index3});
            // The first 484-tone RU is covered by the first two 20 MHz channels
            idx = 1;
            RunOne(p20Index,
                   MakeRuSpec(RuType::RU_484_TONE, idx++, primary80, p160),
                   width,
                   {ch20Index0, ch20Index1});
            // The second 484-tone RU is covered by the last two 20 MHz channels
            RunOne(p20Index,
                   MakeRuSpec(RuType::RU_484_TONE, idx, primary80, p160),
                   width,
                   {ch20Index2, ch20Index3});
            // The 996-tone RU is covered by all the 20 MHz channels
            idx = 1;
            RunOne(p20Index,
                   MakeRuSpec(RuType::RU_996_TONE, idx, primary80, p160),
                   width,
                   {ch20Index0, ch20Index1, ch20Index2, ch20Index3});
        }
        // The 2x996-tone RU is covered by all the eight 20 MHz channels
        RunOne(p20Index,
               MakeRuSpec(RuType::RU_2x996_TONE, 1, p80),
               width,
               {0, 1, 2, 3, 4, 5, 6, 7});
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the WifiRu::GetNRus() method.
 */
class WifiNumRusInChannelTest : public TestCase
{
  public:
    /**
     * Constructor
     * @param modClass the modulation class to distinguish 802.11ax and 802.11be
     */
    WifiNumRusInChannelTest(WifiModulationClass modClass);

  private:
    void DoRun() override;

    /**
     * Check the number of RUs for a given channel width as computed
     * by GetNRus() is correct.
     *
     * @param rutype the given RU type
     * @param width the width of the channel to check
     * @param size the expected size
     */
    void RunOne(RuType rutype, MHz_u width, std::size_t size);

    WifiModulationClass m_modClass; ///< the modulation class to consider for the test
};

WifiNumRusInChannelTest::WifiNumRusInChannelTest(WifiModulationClass modClass)
    : TestCase{"Check returned set of RUs of a given type for a given channel width for " +
               std::string((modClass == WIFI_MOD_CLASS_HE) ? "HE" : "EHT")},
      m_modClass{modClass}
{
}

void
WifiNumRusInChannelTest::RunOne(RuType rutype, MHz_u width, std::size_t size)
{
    const auto numRus = WifiRu::GetNRus(width, rutype, m_modClass);
    NS_TEST_EXPECT_MSG_EQ(numRus,
                          size,
                          "Channel width=" << width << ", RU type=" << rutype << ". Expected size "
                                           << size << " differs from computed size " << numRus);
}

void
WifiNumRusInChannelTest::DoRun()
{
    /******************
     * 20 MHz channel *
     ******************/
    {
        const MHz_u width{20};

        // 9x 26-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_26_TONE, width, 9);

        // 4x 52-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_52_TONE, width, 4);

        // 2x 106-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_106_TONE, width, 2);

        // 1x 242-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_242_TONE, width, 1);

        // no 484-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_484_TONE, width, 0);

        // no 996-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_996_TONE, width, 0);

        // no 2x996-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_2x996_TONE, width, 0);

        // no 4x996-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_4x996_TONE, width, 0);
    }

    /******************
     * 40 MHz channel *
     ******************/
    {
        const MHz_u width{40};

        // 18x 26-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_26_TONE, width, 18);

        // 8x 52-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_52_TONE, width, 8);

        // 4x 106-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_106_TONE, width, 4);

        // 2x 242-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_242_TONE, width, 2);

        // 1x 484-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_484_TONE, width, 1);

        // no 996-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_996_TONE, width, 0);

        // no 2x996-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_2x996_TONE, width, 0);

        // no 4x996-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_4x996_TONE, width, 0);
    }

    /******************
     * 80 MHz channel *
     ******************/
    {
        const MHz_u width{80};

        // 37x (defined) 26-tone RUs are in 80 MHz channels (1 less for EHT)
        RunOne(RuType::RU_26_TONE, width, (m_modClass == WIFI_MOD_CLASS_HE) ? 37 : 36);

        // 16x 52-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_52_TONE, width, 16);

        // 8x 106-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_106_TONE, width, 8);

        // 4x 242-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_242_TONE, width, 4);

        // 2x 484-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_484_TONE, width, 2);

        // 1x 996-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_996_TONE, width, 1);

        // no 2x996-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_2x996_TONE, width, 0);

        // no 4x996-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_4x996_TONE, width, 0);
    }

    /******************
     * 160 MHz channel *
     ******************/
    {
        const MHz_u width{160};

        // 74x (defined) 26-tone RUs are in 160 MHz channels (2 less for EHT)
        RunOne(RuType::RU_26_TONE, width, (m_modClass == WIFI_MOD_CLASS_HE) ? 74 : 72);

        // 32x 52-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_52_TONE, width, 32);

        // 16x 106-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_106_TONE, width, 16);

        // 8x 242-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_242_TONE, width, 8);

        // 4x 484-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_484_TONE, width, 4);

        // 2x 996-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_996_TONE, width, 2);

        // 1x 2x996-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_2x996_TONE, width, 1);

        // no 4x996-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_4x996_TONE, width, 0);
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the WifiRu::GetRusOfType() method.
 */
class WifiRusOfTypeInChannelTest : public TestCase
{
  public:
    /**
     * Constructor
     * @param modClass the modulation class to distinguish 802.11ax and 802.11be
     */
    WifiRusOfTypeInChannelTest(WifiModulationClass modClass);

  private:
    void DoRun() override;

    /**
     * Check the set of RUs returned by GetRusOfType() is correct.
     *
     * @param rutype the given RU type
     * @param width the width of the channel to check
     * @param expectedRus the expected set of RUs
     */
    void RunOne(RuType rutype, MHz_u width, const std::vector<WifiRu::RuSpec>& expectedRus);

    WifiModulationClass m_modClass; ///< the modulation class to consider for the test
};

WifiRusOfTypeInChannelTest::WifiRusOfTypeInChannelTest(WifiModulationClass modClass)
    : TestCase{"Check returned RUs of a given type for a given channel width for " +
               std::string((modClass == WIFI_MOD_CLASS_HE) ? "HE" : "EHT")},
      m_modClass{modClass}
{
}

void
WifiRusOfTypeInChannelTest::RunOne(RuType rutype,
                                   MHz_u width,
                                   const std::vector<WifiRu::RuSpec>& expectedRus)
{
    auto printToStr = [](const std::vector<WifiRu::RuSpec>& v) {
        std::stringstream ss;
        ss << "{";
        for (const auto& ru : v)
        {
            ss << ru << " ";
        }
        ss << "}";
        return ss.str();
    };

    const auto actualRus = WifiRu::GetRusOfType(width, rutype, m_modClass);
    NS_TEST_EXPECT_MSG_EQ((actualRus == expectedRus),
                          true,
                          "Channel width=" << width << ", RU type=" << rutype << ". Expected RUs "
                                           << printToStr(expectedRus) << " differs from actual RUs "
                                           << printToStr(actualRus));
}

void
WifiRusOfTypeInChannelTest::DoRun()
{
    const auto p80{true};
    const auto s80{false};
    const auto p160{(m_modClass == WIFI_MOD_CLASS_HE) ? std::nullopt : std::optional(true)};

    /******************
     * 20 MHz channel *
     ******************/
    {
        const MHz_u width{20};

        // 9x 26-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_26_TONE,
               width,
               {MakeRuSpec(RuType::RU_26_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 4, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 6, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 7, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 8, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 9, p80, p160)});

        // 4x 52-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_52_TONE,
               width,
               {MakeRuSpec(RuType::RU_52_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 4, p80, p160)});

        // 2x 106-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_106_TONE,
               width,
               {MakeRuSpec(RuType::RU_106_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 2, p80, p160)});

        // 1x 242-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_242_TONE, width, {MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160)});

        // no 484-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_484_TONE, width, {});

        // no 996-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_996_TONE, width, {});

        // no 2x996-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_2x996_TONE, width, {});

        // no 4x996-tone RUs are in 20 MHz channels
        RunOne(RuType::RU_4x996_TONE, width, {});
    }

    /******************
     * 40 MHz channel *
     ******************/
    {
        const MHz_u width{40};

        // 18x 26-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_26_TONE,
               width,
               {MakeRuSpec(RuType::RU_26_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 4, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 6, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 7, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 8, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 9, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 10, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 11, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 12, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 13, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 14, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 15, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 16, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 17, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 18, p80, p160)});

        // 8x 52-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_52_TONE,
               width,
               {MakeRuSpec(RuType::RU_52_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 4, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 5, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 6, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 7, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 8, p80, p160)});

        // 4x 106-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_106_TONE,
               width,
               {MakeRuSpec(RuType::RU_106_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 4, p80, p160)});

        // 2x 242-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_242_TONE,
               width,
               {MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 2, p80, p160)});

        // 1x 484-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_484_TONE, width, {MakeRuSpec(RuType::RU_484_TONE, 1, p80, p160)});

        // no 996-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_996_TONE, width, {});

        // no 2x996-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_2x996_TONE, width, {});

        // no 4x996-tone RUs are in 40 MHz channels
        RunOne(RuType::RU_4x996_TONE, width, {});
    }

    /******************
     * 80 MHz channel *
     ******************/
    {
        const MHz_u width{80};

        // 37x 26-tone RUs are in 80 MHz channels (1 less for EHT)
        std::vector<WifiRu::RuSpec> expectedRus{MakeRuSpec(RuType::RU_26_TONE, 1, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 2, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 3, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 4, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 6, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 7, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 8, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 9, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 10, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 11, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 12, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 13, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 14, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 15, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 16, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 17, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 18, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 20, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 21, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 22, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 23, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 24, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 25, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 26, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 27, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 28, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 29, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 30, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 31, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 32, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 33, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 34, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 35, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 36, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 37, p80, p160)};
        if (m_modClass == WIFI_MOD_CLASS_HE)
        {
            // RU 19 is undefined for EHT
            const auto it = std::next(expectedRus.cbegin(), 18);
            expectedRus.insert(it, MakeRuSpec(RuType::RU_26_TONE, 19, p80, p160));
        }
        RunOne(RuType::RU_26_TONE, width, expectedRus);

        // 16x 52-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_52_TONE,
               width,
               {MakeRuSpec(RuType::RU_52_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 4, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 5, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 6, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 7, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 8, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 9, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 10, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 11, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 12, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 13, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 14, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 15, p80, p160),
                MakeRuSpec(RuType::RU_52_TONE, 16, p80, p160)});

        // 8x 106-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_106_TONE,
               width,
               {MakeRuSpec(RuType::RU_106_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 4, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 5, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 6, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 7, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 8, p80, p160)});

        // 4x 242-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_242_TONE,
               width,
               {MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 4, p80, p160)});

        // 2x 484-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_484_TONE,
               width,
               {MakeRuSpec(RuType::RU_484_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_484_TONE, 2, p80, p160)});

        // 1x 996-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_996_TONE, width, {MakeRuSpec(RuType::RU_996_TONE, 1, p80, p160)});

        // no 2x996-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_2x996_TONE, width, {});

        // no 4x996-tone RUs are in 80 MHz channels
        RunOne(RuType::RU_4x996_TONE, width, {});
    }

    /******************
     * 160 MHz channel *
     ******************/
    {
        const MHz_u width{160};

        // 74x 26-tone RUs are in 160 MHz channels (2 less for EHT)
        std::vector<WifiRu::RuSpec> expectedRus{MakeRuSpec(RuType::RU_26_TONE, 1, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 2, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 3, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 4, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 6, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 7, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 8, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 9, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 10, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 11, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 12, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 13, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 14, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 15, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 16, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 17, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 18, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 20, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 21, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 22, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 23, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 24, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 25, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 26, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 27, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 28, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 29, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 30, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 31, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 32, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 33, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 34, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 35, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 36, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 37, p80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 1, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 2, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 3, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 4, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 5, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 6, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 7, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 8, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 9, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 10, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 11, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 12, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 13, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 14, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 15, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 16, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 17, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 18, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 20, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 21, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 22, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 23, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 24, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 25, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 26, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 27, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 28, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 29, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 30, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 31, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 32, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 33, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 34, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 35, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 36, s80, p160),
                                                MakeRuSpec(RuType::RU_26_TONE, 37, s80, p160)};
        if (m_modClass == WIFI_MOD_CLASS_HE)
        {
            // RU 19 and RU 56 are undefined for EHT
            auto it = std::next(expectedRus.cbegin(), 18);
            expectedRus.insert(it, MakeRuSpec(RuType::RU_26_TONE, 19, p80));
            it = std::next(expectedRus.cbegin(), 55);
            expectedRus.insert(it, MakeRuSpec(RuType::RU_26_TONE, 19, s80));
        }
        RunOne(RuType::RU_26_TONE, width, expectedRus);

        // 32x 52-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_52_TONE, width, {MakeRuSpec(RuType::RU_52_TONE, 1, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 2, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 3, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 4, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 5, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 6, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 7, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 8, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 9, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 10, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 11, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 12, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 13, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 14, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 15, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 16, p80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 1, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 2, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 3, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 4, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 5, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 6, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 7, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 8, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 9, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 10, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 11, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 12, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 13, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 14, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 15, s80, p160),
                                           MakeRuSpec(RuType::RU_52_TONE, 16, s80, p160)});

        // 16x 106-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_106_TONE,
               width,
               {MakeRuSpec(RuType::RU_106_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 4, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 5, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 6, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 7, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 8, p80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 1, s80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 2, s80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 3, s80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 4, s80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 5, s80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 6, s80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 7, s80, p160),
                MakeRuSpec(RuType::RU_106_TONE, 8, s80, p160)});

        // 8x 242-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_242_TONE,
               width,
               {MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 4, p80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 1, s80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 2, s80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 3, s80, p160),
                MakeRuSpec(RuType::RU_242_TONE, 4, s80, p160)});

        // 4x 484-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_484_TONE,
               width,
               {MakeRuSpec(RuType::RU_484_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_484_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_484_TONE, 1, s80, p160),
                MakeRuSpec(RuType::RU_484_TONE, 2, s80, p160)});

        // 2x 996-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_996_TONE,
               width,
               {MakeRuSpec(RuType::RU_996_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_996_TONE, 1, s80, p160)});

        // 1x 2x996-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_2x996_TONE, width, {MakeRuSpec(RuType::RU_2x996_TONE, 1, p80, p160)});

        // no 4x996-tone RUs are in 160 MHz channels
        RunOne(RuType::RU_4x996_TONE, width, {});
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the WifiRu::GetCentral26TonesRus() method.
 */
class WifiCentral26TonesRusInChannelTest : public TestCase
{
  public:
    /**
     * Constructor
     * @param modClass the modulation class to distinguish 802.11ax and 802.11be
     */
    WifiCentral26TonesRusInChannelTest(WifiModulationClass modClass);

  private:
    void DoRun() override;

    /**
     * Check the set of 26-tone RUs returned by GetCentral26TonesRus() is correct.
     *
     * @param rutype the given RU type
     * @param width the width of the channel to check
     * @param expectedCentral26TonesRus the expected set of 26-tone RUs
     */
    void RunOne(RuType rutype,
                MHz_u width,
                const std::vector<WifiRu::RuSpec>& expectedCentral26TonesRus);

    WifiModulationClass m_modClass; ///< the modulation class to consider for the test
};

WifiCentral26TonesRusInChannelTest::WifiCentral26TonesRusInChannelTest(WifiModulationClass modClass)
    : TestCase{"Check returned set of 26-tone RUs in a given RU type and a given channel width "
               "for " +
               std::string((modClass == WIFI_MOD_CLASS_HE) ? "HE" : "EHT")},
      m_modClass{modClass}
{
}

void
WifiCentral26TonesRusInChannelTest::RunOne(RuType rutype,
                                           MHz_u width,
                                           const std::vector<WifiRu::RuSpec>& expectedRus)
{
    auto printToStr = [](const std::vector<WifiRu::RuSpec>& v) {
        std::stringstream ss;
        ss << "{";
        for (const auto& ru : v)
        {
            ss << ru << " ";
        }
        ss << "}";
        return ss.str();
    };

    const auto actualRus = WifiRu::GetCentral26TonesRus(width, rutype, m_modClass);
    NS_TEST_EXPECT_MSG_EQ((actualRus == expectedRus),
                          true,
                          "Channel width=" << width << ", RU type=" << rutype
                                           << ". Expected 26-tone RUs " << printToStr(expectedRus)
                                           << " differs from actual 26-tone RUs "
                                           << printToStr(actualRus));
}

void
WifiCentral26TonesRusInChannelTest::DoRun()
{
    const auto p80{true};
    const auto s80{false};
    const auto p160{(m_modClass == WIFI_MOD_CLASS_HE) ? std::nullopt : std::optional(true)};

    /******************
     * 20 MHz channel *
     ******************/
    {
        const MHz_u width{20};

        // returned set should be empty for 26-tones RUs
        RunOne(RuType::RU_26_TONE, width, {});

        // there is room for 1 center 26-tone RU when 52-tone RUs are used over 20 MHz
        RunOne(RuType::RU_52_TONE, width, {MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160)});

        // there is room for 1 center 26-tone RU when 106-tone RUs are used over 20 MHz
        RunOne(RuType::RU_106_TONE, width, {MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160)});

        // there is no room for center 26-tone RUs when 242-tone RUs are used over 20 MHz
        RunOne(RuType::RU_242_TONE, width, {});
    }

    /******************
     * 40 MHz channel *
     ******************/
    {
        const MHz_u width{40};

        // returned set should be empty for 26-tones RUs
        RunOne(RuType::RU_26_TONE, width, {});

        // there is room for 2 center 26-tone RUs when 52-tone RUs are used over 40 MHz
        RunOne(RuType::RU_52_TONE,
               width,
               {MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 14, p80, p160)});

        // there is room for 2 center 26-tone RUs when 106-tone RUs are used over 40 MHz
        RunOne(RuType::RU_106_TONE,
               width,
               {MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 14, p80, p160)});

        // there is no room for center 26-tone RUs when 242-tone RUs are used over 40 MHz
        RunOne(RuType::RU_242_TONE, width, {});

        // there is no room for center 26-tone RUs when 484-tone RUs are used over 40 MHz
        RunOne(RuType::RU_484_TONE, width, {});
    }

    /******************
     * 80 MHz channel *
     ******************/
    {
        const MHz_u width{80};

        // returned set should be empty for 26-tones RUs
        RunOne(RuType::RU_26_TONE, width, {});

        // there is room for 5 (1 less for EHT) center 26-tone RUs when
        // 52-tone/106-tone/242-tone/484-tone RUs are used over 80 MHz
        std::vector<WifiRu::RuSpec> expectedCentral26TonesRus{
            MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160),
            MakeRuSpec(RuType::RU_26_TONE, 14, p80, p160),
            MakeRuSpec(RuType::RU_26_TONE, 24, p80, p160),
            MakeRuSpec(RuType::RU_26_TONE, 33, p80, p160)};
        std::vector<WifiRu::RuSpec> extraCentral26TonesRus{};
        if (m_modClass == WIFI_MOD_CLASS_HE) // RU 19 is undefined for EHT
        {
            HeRu::RuSpec central26TonesRu{RuType::RU_26_TONE, 19, p80};
            extraCentral26TonesRus.emplace_back(central26TonesRu);
            const auto it = std::next(expectedCentral26TonesRus.cbegin(), 2);
            expectedCentral26TonesRus.insert(it, central26TonesRu);
        }

        RunOne(RuType::RU_52_TONE, width, expectedCentral26TonesRus);
        RunOne(RuType::RU_106_TONE, width, expectedCentral26TonesRus);

        RunOne(RuType::RU_242_TONE, width, extraCentral26TonesRus);
        RunOne(RuType::RU_484_TONE, width, extraCentral26TonesRus);

        // there is no room for center 26-tone RUs when 996-tone RUs are used over 80 MHz
        RunOne(RuType::RU_996_TONE, width, {});
    }

    /******************
     * 160 MHz channel *
     ******************/
    {
        const MHz_u width{160};

        // returned set should be empty for 26-tones RUs
        RunOne(RuType::RU_26_TONE, width, {});

        // there is room for 10 (2 less for EHT) center 26-tone RUs when
        // 52-tone/106-tone/242-tone/484-tone RUs are used over 80 MHz
        std::vector<WifiRu::RuSpec> expectedCentral26TonesRus{
            MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160),
            MakeRuSpec(RuType::RU_26_TONE, 14, p80, p160),
            MakeRuSpec(RuType::RU_26_TONE, 24, p80, p160),
            MakeRuSpec(RuType::RU_26_TONE, 33, p80, p160),
            MakeRuSpec(RuType::RU_26_TONE, 5, s80, p160),
            MakeRuSpec(RuType::RU_26_TONE, 14, s80, p160),
            MakeRuSpec(RuType::RU_26_TONE, 24, s80, p160),
            MakeRuSpec(RuType::RU_26_TONE, 33, s80, p160)};
        std::vector<WifiRu::RuSpec> extraCentral26TonesRus{};
        if (m_modClass == WIFI_MOD_CLASS_HE) // RU 19 and RU 56 are undefined for EHT
        {
            {
                HeRu::RuSpec central26TonesRu{RuType::RU_26_TONE, 19, p80};
                extraCentral26TonesRus.emplace_back(central26TonesRu);
                const auto it = std::next(expectedCentral26TonesRus.cbegin(), 2);
                expectedCentral26TonesRus.insert(it, central26TonesRu);
            }
            {
                HeRu::RuSpec central26TonesRu{RuType::RU_26_TONE, 19, s80};
                extraCentral26TonesRus.emplace_back(central26TonesRu);
                const auto it = std::next(expectedCentral26TonesRus.cbegin(), 7);
                expectedCentral26TonesRus.insert(it, central26TonesRu);
            }
        }

        RunOne(RuType::RU_52_TONE, width, expectedCentral26TonesRus);
        RunOne(RuType::RU_106_TONE, width, expectedCentral26TonesRus);

        RunOne(RuType::RU_242_TONE, width, extraCentral26TonesRus);
        RunOne(RuType::RU_484_TONE, width, extraCentral26TonesRus);

        // there is no room for center 26-tone RUs when 996-tone/2x996-tone RUs are used over 160
        // MHz
        RunOne(RuType::RU_996_TONE, width, {});
        RunOne(RuType::RU_2x996_TONE, width, {});
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the WifiRu::GetEqualSizedRusForStations() method.
 */
class WifiEqualSizedRusTest : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param modClass the modulation class to distinguish 802.11ax and 802.11be
     */
    WifiEqualSizedRusTest(WifiModulationClass modClass);

    /**
     * Check the maximization of the number of candidate stations that can be assigned
     * a RU subject to the constraint that all the stations must be assigned a RU
     * of the same size as computed by GetEqualSizedRusForStations() is correct.
     *
     * @param width the width of the channel to check
     * @param numStas the number of candidate stations we want to assign a RU
     * @param expectedNumStas the expected number of candidate stations that can be assigned a RU
     * @param expectedNumCentral26TonesRus the expected number of additional 26-tone RUs that can be
     * allocated @param expectedRuType the expected RU type
     */
    void RunOne(MHz_u width,
                std::size_t numStas,
                std::size_t expectedNumStas,
                std::size_t expectedNumCentral26TonesRus,
                RuType expectedRuType);

  private:
    void DoRun() override;

    WifiModulationClass m_modClass; ///< the modulation class to consider for the test
};

WifiEqualSizedRusTest::WifiEqualSizedRusTest(WifiModulationClass modClass)
    : TestCase{"Check computation of the number of equal sized RUs for a given channel width for " +
               std::string((modClass == WIFI_MOD_CLASS_HE) ? "HE" : "EHT")},
      m_modClass{modClass}
{
}

void
WifiEqualSizedRusTest::RunOne(MHz_u width,
                              std::size_t numStas,
                              std::size_t expectedNumStas,
                              std::size_t expectedNumCentral26TonesRus,
                              RuType expectedRuType)
{
    std::size_t actualNumStas{numStas};
    std::size_t actualNumCentral26TonesRus{0};
    const auto actualRuType = WifiRu::GetEqualSizedRusForStations(width,
                                                                  actualNumStas,
                                                                  actualNumCentral26TonesRus,
                                                                  m_modClass);
    NS_TEST_EXPECT_MSG_EQ(
        actualNumStas,
        expectedNumStas,
        "Channel width=" << width << " MHz. Expected number of candidate stations "
                         << expectedNumStas << " differs from actual number of candidate stations "
                         << actualNumStas);
    NS_TEST_EXPECT_MSG_EQ(actualNumCentral26TonesRus,
                          expectedNumCentral26TonesRus,
                          "Channel width="
                              << width << " MHz. Expected number of additional 26-tone RUs "
                              << expectedNumStas
                              << " differs from actual number of additional 26-tone RUs "
                              << actualNumStas);
    NS_TEST_EXPECT_MSG_EQ(actualRuType,
                          expectedRuType,
                          "Channel width=" << width << " MHz. Expected RU type " << expectedRuType
                                           << " differs from actual RU type " << actualRuType);
}

void
WifiEqualSizedRusTest::DoRun()
{
    /******************
     * 20 MHz channel *
     ******************/
    {
        const MHz_u width{20};

        // 1 STA using 242-tone RU and no center 26-tone RU available over a 20 MHz channel can be
        // allocated for 1 candidate station
        RunOne(width, 1, 1, 0, RuType::RU_242_TONE);

        // 2 STAs using 106-tone RUs and 1 center 26-tone RU available over a 20 MHz channel can be
        // allocated for 2 candidate stations
        RunOne(width, 2, 2, 1, RuType::RU_106_TONE);

        // 2 STAs using 106-tone RUs and 1 center 26-tone RU available over a 20 MHz channel can be
        // allocated for 3 candidate stations
        RunOne(width, 3, 2, 1, RuType::RU_106_TONE);

        // 4 STAs using 52-tone RUs and 1 center 26-tone RU available over a 20 MHz channel can be
        // allocated for 4 candidate stations
        RunOne(width, 4, 4, 1, RuType::RU_52_TONE);

        // 4 STAs using 52-tone RUs and 1 center 26-tone RU available over a 20 MHz channel can be
        // allocated for 5 candidate stations
        RunOne(width, 5, 4, 1, RuType::RU_52_TONE);

        // 4 STAs using 52-tone RUs and 1 center 26-tone RU available over a 20 MHz channel can be
        // allocated for 6 candidate stations
        RunOne(width, 6, 4, 1, RuType::RU_52_TONE);

        // 4 STAs using 52-tone RUs and 1 center 26-tone RU available over a 20 MHz channel can be
        // allocated for 7 candidate stations
        RunOne(width, 7, 4, 1, RuType::RU_52_TONE);

        // 4 STAs using 52-tone RUs and 1 center 26-tone RU available over a 20 MHz channel can be
        // allocated for 8 candidate stations
        RunOne(width, 8, 4, 1, RuType::RU_52_TONE);

        // 9 STAs using 26-tone RUs and no center 26-tone RU available over a 20 MHz channel can be
        // allocated for 9 candidate stations
        RunOne(width, 9, 9, 0, RuType::RU_26_TONE);

        // 9 STAs using 26-tone RUs over a 20 MHz channel can be allocated for 10 candidate stations
        RunOne(width, 10, 9, 0, RuType::RU_26_TONE);
    }

    /******************
     * 40 MHz channel *
     ******************/
    {
        const MHz_u width{40};

        // 1 STA using 484-tone RU and no center 26-tone RU available over a 40 MHz channel can be
        // allocated for 1 candidate station
        RunOne(width, 1, 1, 0, RuType::RU_484_TONE);

        // 2 STAs using 242-tone RUs and no center 26-tone RU available over a 40 MHz channel can be
        // allocated for 2 candidate stations
        RunOne(width, 2, 2, 0, RuType::RU_242_TONE);

        // 2 STAs using 242-tone RUs and no center 26-tone RU available over a 40 MHz channel can be
        // allocated for 3 candidate stations
        RunOne(width, 3, 2, 0, RuType::RU_242_TONE);

        // 4 STAs using 106-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 4 candidate stations
        RunOne(width, 4, 4, 2, RuType::RU_106_TONE);

        // 4 STAs using 106-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 5 candidate stations
        RunOne(width, 5, 4, 2, RuType::RU_106_TONE);

        // 4 STAs using 106-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 6 candidate stations
        RunOne(width, 6, 4, 2, RuType::RU_106_TONE);

        // 4 STAs using 106-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 7 candidate stations
        RunOne(width, 7, 4, 2, RuType::RU_106_TONE);

        // 8 STAs using 52-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 8 candidate stations
        RunOne(width, 8, 8, 2, RuType::RU_52_TONE);

        // 8 STAs using 52-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 9 candidate stations
        RunOne(width, 9, 8, 2, RuType::RU_52_TONE);

        // 8 STAs using 52-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 10 candidate stations
        RunOne(width, 10, 8, 2, RuType::RU_52_TONE);

        // 8 STAs using 52-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 11 candidate stations
        RunOne(width, 11, 8, 2, RuType::RU_52_TONE);

        // 8 STAs using 52-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 12 candidate stations
        RunOne(width, 12, 8, 2, RuType::RU_52_TONE);

        // 8 STAs using 52-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 13 candidate stations
        RunOne(width, 13, 8, 2, RuType::RU_52_TONE);

        // 8 STAs using 52-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 14 candidate stations
        RunOne(width, 14, 8, 2, RuType::RU_52_TONE);

        // 8 STAs using 52-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 15 candidate stations
        RunOne(width, 15, 8, 2, RuType::RU_52_TONE);

        // 8 STAs using 52-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 16 candidate stations
        RunOne(width, 16, 8, 2, RuType::RU_52_TONE);

        // 8 STAs using 52-tone RUs and 2 center 26-tone RUs available over a 40 MHz channel can be
        // allocated for 17 candidate stations
        RunOne(width, 17, 8, 2, RuType::RU_52_TONE);

        // 18 STAs using 26-tone RUs over a 40 MHz channel can be allocated for 18 candidate
        // stations
        RunOne(width, 18, 18, 0, RuType::RU_26_TONE);

        // 18 STAs using 26-tone RUs over a 40 MHz channel can be allocated for 19 candidate
        // stations
        RunOne(width, 19, 18, 0, RuType::RU_26_TONE);
    }

    /******************
     * 80 MHz channel *
     ******************/
    {
        const MHz_u width{80};

        // 1 STA using 996-tone RU and no center 26-tone RU available over a 80 MHz channel can be
        // allocated for 11 candidate stations for 1 candidate station
        RunOne(width, 1, 1, 0, RuType::RU_996_TONE);

        // 2 STAs using 484-tone RUs and 1 center 26-tone RU (HE only) available over a 80 MHz
        // channel can be allocated for 2 candidate stations
        RunOne(width, 2, 2, (m_modClass == WIFI_MOD_CLASS_HE) ? 1 : 0, RuType::RU_484_TONE);

        // 2 STAs using 484-tone RUs and 1 center 26-tone RU (HE only) available over a 80 MHz
        // channel can be allocated for 3 candidate stations
        RunOne(width, 3, 2, (m_modClass == WIFI_MOD_CLASS_HE) ? 1 : 0, RuType::RU_484_TONE);

        // 4 STAs using 242-tone RUs and 1 center 26-tone RU (HE only) available over a 80 MHz
        // channel can be allocated for 4 candidate stations
        RunOne(width, 4, 4, (m_modClass == WIFI_MOD_CLASS_HE) ? 1 : 0, RuType::RU_242_TONE);

        // 4 STAs using 242-tone RUs and 1 center 26-tone RU (HE only) available over a 80 MHz
        // channel can be allocated for 5 candidate stations
        RunOne(width, 5, 4, (m_modClass == WIFI_MOD_CLASS_HE) ? 1 : 0, RuType::RU_242_TONE);

        // 4 STAs using 242-tone RUs and 1 center 26-tone RU (HE only) available over a 80 MHz
        // channel can be allocated for 6 candidate stations
        RunOne(width, 6, 4, (m_modClass == WIFI_MOD_CLASS_HE) ? 1 : 0, RuType::RU_242_TONE);

        // 4 STAs using 242-tone RUs and 1 center 26-tone RU (HE only) available over a 80 MHz
        // channel can be allocated for 7 candidate stations
        RunOne(width, 7, 4, (m_modClass == WIFI_MOD_CLASS_HE) ? 1 : 0, RuType::RU_242_TONE);

        // 8 STAs using 106-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 8 candidate stations
        RunOne(width, 8, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_106_TONE);

        // 8 STAs using 106-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 9 candidate stations
        RunOne(width, 9, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_106_TONE);

        // 8 STAs using 106-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 10 candidate stations
        RunOne(width, 10, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_106_TONE);

        // 8 STAs using 106-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations
        RunOne(width, 11, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_106_TONE);

        // 8 STAs using 106-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 12 candidate stations
        RunOne(width, 12, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_106_TONE);

        // 8 STAs using 106-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 13 candidate stations
        RunOne(width, 13, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_106_TONE);

        // 8 STAs using 106-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 14 candidate stations
        RunOne(width, 14, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_106_TONE);

        // 8 STAs using 106-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 15 candidate stations
        RunOne(width, 15, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_106_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 16 candidate stations
        RunOne(width, 16, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 17 candidate stations
        RunOne(width, 17, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 18 candidate stations
        RunOne(width, 18, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 19 candidate stations
        RunOne(width, 19, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 20 candidate stations
        RunOne(width, 20, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 21 candidate stations
        RunOne(width, 21, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 22 candidate stations
        RunOne(width, 22, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 23 candidate stations
        RunOne(width, 23, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 24 candidate stations
        RunOne(width, 24, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 25 candidate stations
        RunOne(width, 25, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 26 candidate stations
        RunOne(width, 26, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 27 candidate stations
        RunOne(width, 27, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 28 candidate stations
        RunOne(width, 28, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 29 candidate stations
        RunOne(width, 29, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 30 candidate stations
        RunOne(width, 30, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 31 candidate stations
        RunOne(width, 31, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 31 candidate stations
        RunOne(width, 31, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 32 candidate stations
        RunOne(width, 32, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 33 candidate stations
        RunOne(width, 33, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 34 candidate stations
        RunOne(width, 34, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs using 52-tone RUs and 5 center 26-tone RUs (1 less for EHT) available over a 80
        // MHz channel can be allocated for 11 candidate stations for 35 candidate stations
        RunOne(width, 35, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 4, RuType::RU_52_TONE);

        // 16 STAs (36 for EHT) using 52-tone RUs (26-tone RUs for EHT) and 5 center 26-tone RUs
        // (HE only) available over a 80 MHz channel can be allocated for 11 candidate stations for
        // 36 candidate stations
        RunOne(width,
               36,
               (m_modClass == WIFI_MOD_CLASS_HE) ? 16 : 36,
               (m_modClass == WIFI_MOD_CLASS_HE) ? 5 : 0,
               (m_modClass == WIFI_MOD_CLASS_HE) ? RuType::RU_52_TONE : RuType::RU_26_TONE);

        // 37 STAs (36 for EHT) using 26-tone RUs over a 80 MHz channel can be allocated for 37
        // candidate stations
        RunOne(width, 37, (m_modClass == WIFI_MOD_CLASS_HE) ? 37 : 36, 0, RuType::RU_26_TONE);

        // 37 STAs (36 for EHT) using 26-tone RUs over a 80 MHz channel can be allocated for 38
        // candidate stations
        RunOne(width, 38, (m_modClass == WIFI_MOD_CLASS_HE) ? 37 : 36, 0, RuType::RU_26_TONE);
    }

    /******************
     * 160 MHz channel *
     ******************/
    {
        const MHz_u width{160};

        // 1 STA using 2x996-tone RU and no center 26-tone RU available over a 160 MHz channel can
        // be allocated for 1 candidate station
        RunOne(width, 1, 1, 0, RuType::RU_2x996_TONE);

        // 2 STAs using 996-tone RUs and no center 26-tone RU available over a 160 MHz channel can
        // be allocated for 2 candidate stations
        RunOne(width, 2, 2, 0, RuType::RU_996_TONE);

        // 2 STAs using 996-tone RUs and no center 26-tone RU available over a 160 MHz channel can
        // be allocated for 3 candidate stations
        RunOne(width, 3, 2, 0, RuType::RU_996_TONE);

        // 4 STAs using 484-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 4 candidate stations
        RunOne(width, 4, 4, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_484_TONE);

        // 4 STAs using 484-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 5 candidate stations
        RunOne(width, 5, 4, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_484_TONE);

        // 4 STAs using 484-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 6 candidate stations
        RunOne(width, 6, 4, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_484_TONE);

        // 4 STAs using 484-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 7 candidate stations
        RunOne(width, 7, 4, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_484_TONE);

        // 8 STAs using 242-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 8 candidate stations
        RunOne(width, 8, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_242_TONE);

        // 8 STAs using 242-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 9 candidate stations
        RunOne(width, 9, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_242_TONE);

        // 8 STAs using 242-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 10 candidate stations
        RunOne(width, 10, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_242_TONE);

        // 8 STAs using 242-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 11 candidate stations
        RunOne(width, 11, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_242_TONE);

        // 8 STAs using 242-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 12 candidate stations
        RunOne(width, 12, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_242_TONE);

        // 8 STAs using 242-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 13 candidate stations
        RunOne(width, 13, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_242_TONE);

        // 8 STAs using 242-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 14 candidate stations
        RunOne(width, 14, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_242_TONE);

        // 8 STAs using 242-tone RUs and 2 center 26-tone RUs (HE only) available over a 160 MHz
        // channel can be allocated for 15 candidate stations
        RunOne(width, 15, 8, (m_modClass == WIFI_MOD_CLASS_HE) ? 2 : 0, RuType::RU_242_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 16 candidate stations
        RunOne(width, 16, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 17 candidate stations
        RunOne(width, 17, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 18 candidate stations
        RunOne(width, 18, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 19 candidate stations
        RunOne(width, 19, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 20 candidate stations
        RunOne(width, 20, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 21 candidate stations
        RunOne(width, 21, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 22 candidate stations
        RunOne(width, 22, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 23 candidate stations
        RunOne(width, 23, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 24 candidate stations
        RunOne(width, 24, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 25 candidate stations
        RunOne(width, 25, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 26 candidate stations
        RunOne(width, 26, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 27 candidate stations
        RunOne(width, 27, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 28 candidate stations
        RunOne(width, 28, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 29 candidate stations
        RunOne(width, 29, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 30 candidate stations
        RunOne(width, 30, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 16 STAs using 106-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a
        // 160 MHz channel can be allocated for 31 candidate stations
        RunOne(width, 31, 16, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_106_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 32 candidate stations
        RunOne(width, 32, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 33 candidate stations
        RunOne(width, 33, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 34 candidate stations
        RunOne(width, 34, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 35 candidate stations
        RunOne(width, 35, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 36 candidate stations
        RunOne(width, 36, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 37 candidate stations
        RunOne(width, 37, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 38 candidate stations
        RunOne(width, 38, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 39 candidate stations
        RunOne(width, 39, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 40 candidate stations
        RunOne(width, 40, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 41 candidate stations
        RunOne(width, 41, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 42 candidate stations
        RunOne(width, 42, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 43 candidate stations
        RunOne(width, 43, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 44 candidate stations
        RunOne(width, 44, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 45 candidate stations
        RunOne(width, 45, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 46 candidate stations
        RunOne(width, 46, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 47 candidate stations
        RunOne(width, 47, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 48 candidate stations
        RunOne(width, 48, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 49 candidate stations
        RunOne(width, 49, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 50 candidate stations
        RunOne(width, 50, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 51 candidate stations
        RunOne(width, 51, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 52 candidate stations
        RunOne(width, 52, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 53 candidate stations
        RunOne(width, 53, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 54 candidate stations
        RunOne(width, 54, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 55 candidate stations
        RunOne(width, 55, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 56 candidate stations
        RunOne(width, 56, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 57 candidate stations
        RunOne(width, 57, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 58 candidate stations
        RunOne(width, 58, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 59 candidate stations
        RunOne(width, 59, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 60 candidate stations
        RunOne(width, 60, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 61 candidate stations
        RunOne(width, 61, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 62 candidate stations
        RunOne(width, 62, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 63 candidate stations
        RunOne(width, 63, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 64 candidate stations
        RunOne(width, 64, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 65 candidate stations
        RunOne(width, 65, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 66 candidate stations
        RunOne(width, 66, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 67 candidate stations
        RunOne(width, 67, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 68 candidate stations
        RunOne(width, 68, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 69 candidate stations
        RunOne(width, 69, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 70 candidate stations
        RunOne(width, 70, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs using 52-tone RUs and 10 center 26-tone RUs (2 less for EHT) available over a 160
        // MHz channel can be allocated for 71 candidate stations
        RunOne(width, 71, 32, (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 8, RuType::RU_52_TONE);

        // 32 STAs (72 for EHT) using 52-tone RUs (26-tone RUs for EHT) and 10 center 26-tone RUs
        // (HE only) available over a 160 MHz channel can be allocated for 72 candidate stations
        RunOne(width,
               72,
               (m_modClass == WIFI_MOD_CLASS_HE) ? 32 : 72,
               (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 0,
               (m_modClass == WIFI_MOD_CLASS_HE) ? RuType::RU_52_TONE : RuType::RU_26_TONE);

        // 32 STAs (72 for EHT) using 52-tone RUs (26-tone RUs for EHT) and 10 center 26-tone RUs
        // (HE only) available over a 160 MHz channel can be allocated for 73 candidate stations
        RunOne(width,
               73,
               (m_modClass == WIFI_MOD_CLASS_HE) ? 32 : 72,
               (m_modClass == WIFI_MOD_CLASS_HE) ? 10 : 0,
               (m_modClass == WIFI_MOD_CLASS_HE) ? RuType::RU_52_TONE : RuType::RU_26_TONE);

        // 74 STAs (72 for EHT) using 26-tone RUs over a 160 MHz channel can be allocated for 74
        // candidate stations
        RunOne(width, 74, (m_modClass == WIFI_MOD_CLASS_HE) ? 74 : 72, 0, RuType::RU_26_TONE);

        // 74 STAs (72 for EHT) using 26-tone RUs over a 160 MHz channel can be allocated for 75
        // candidate stations
        RunOne(width, 75, (m_modClass == WIFI_MOD_CLASS_HE) ? 74 : 72, 0, RuType::RU_26_TONE);
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the WifiRu::GetSubcarrierGroup() method.
 */
class WifiSubcarrierGroupsTest : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param modClass the modulation class to distinguish 802.11ax and 802.11be
     */
    WifiSubcarrierGroupsTest(WifiModulationClass modClass);

    /**
     * Check the subcarrier group as returned
     * by GetSubcarrierGroup() is correct.
     *
     * @param width the width of the channel to check
     * @param ruType the RU type to check
     * @param phyIndex the PHY index to check
     * @param expectedSubcarrierGroup the expected subcarrier range
     */
    void RunOne(MHz_u width,
                RuType ruType,
                std::size_t phyIndex,
                const SubcarrierGroup& expectedSubcarrierGroup);

  private:
    void DoRun() override;

    WifiModulationClass m_modClass; ///< the modulation class to consider for the test
};

WifiSubcarrierGroupsTest::WifiSubcarrierGroupsTest(WifiModulationClass modClass)
    : TestCase{"Check computation of the subcarrier groups for " +
               std::string((modClass == WIFI_MOD_CLASS_HE) ? "HE" : "EHT")},
      m_modClass{modClass}
{
}

void
WifiSubcarrierGroupsTest::RunOne(MHz_u width,
                                 RuType ruType,
                                 std::size_t phyIndex,
                                 const SubcarrierGroup& expectedSubcarrierGroup)
{
    auto printToStr = [](const SubcarrierGroup& groups) {
        std::stringstream ss;
        ss << "{ ";
        for (const auto& group : groups)
        {
            ss << "(" << group.first << ", " << group.second << ") ";
        }
        ss << "}";
        return ss.str();
    };

    const auto actualSubcarrierGroup =
        WifiRu::GetSubcarrierGroup(width, ruType, phyIndex, m_modClass);
    NS_TEST_EXPECT_MSG_EQ((actualSubcarrierGroup == expectedSubcarrierGroup),
                          true,
                          "Channel width=" << width << ", RU type=" << ruType << ", PHY index="
                                           << phyIndex << ". Expected subcarrier groups "
                                           << printToStr(expectedSubcarrierGroup)
                                           << " differs from actual subcarrier groups "
                                           << printToStr(actualSubcarrierGroup));
}

void
WifiSubcarrierGroupsTest::DoRun()
{
    const std::map<BwTonesPair, std::vector<SubcarrierGroup>> expectedHeRuSubcarrierGroups = {
        {{MHz_u{20}, RuType::RU_26_TONE},
         {/* 1 */ {{-121, -96}},
          /* 2 */ {{-95, -70}},
          /* 3 */ {{-68, -43}},
          /* 4 */ {{-42, -17}},
          /* 5 */ {{-16, -4}, {4, 16}},
          /* 6 */ {{17, 42}},
          /* 7 */ {{43, 68}},
          /* 8 */ {{70, 95}},
          /* 9 */ {{96, 121}}}},
        {{MHz_u{20}, RuType::RU_52_TONE},
         {/* 1 */ {{-121, -70}},
          /* 2 */ {{-68, -17}},
          /* 3 */ {{17, 68}},
          /* 4 */ {{70, 121}}}},
        {{MHz_u{20}, RuType::RU_106_TONE},
         {/* 1 */ {{-122, -17}},
          /* 2 */ {{17, 122}}}},
        {{MHz_u{20}, RuType::RU_242_TONE}, {/* 1 */ {{-122, -2}, {2, 122}}}},
        {{MHz_u{40}, RuType::RU_26_TONE},
         {/* 1 */ {{-243, -218}},
          /* 2 */ {{-217, -192}},
          /* 3 */ {{-189, -164}},
          /* 4 */ {{-163, -138}},
          /* 5 */ {{-136, -111}},
          /* 6 */ {{-109, -84}},
          /* 7 */ {{-83, -58}},
          /* 8 */ {{-55, -30}},
          /* 9 */ {{-29, -4}},
          /* 10 */ {{4, 29}},
          /* 11 */ {{30, 55}},
          /* 12 */ {{58, 83}},
          /* 13 */ {{84, 109}},
          /* 14 */ {{111, 136}},
          /* 15 */ {{138, 163}},
          /* 16 */ {{164, 189}},
          /* 17 */ {{192, 217}},
          /* 18 */ {{218, 243}}}},
        {{MHz_u{40}, RuType::RU_52_TONE},
         {/* 1 */ {{-243, -192}},
          /* 2 */ {{-189, -138}},
          /* 3 */ {{-109, -58}},
          /* 4 */ {{-55, -4}},
          /* 5 */ {{4, 55}},
          /* 6 */ {{58, 109}},
          /* 7 */ {{138, 189}},
          /* 8 */ {{192, 243}}}},
        {{MHz_u{40}, RuType::RU_106_TONE},
         {/* 1 */ {{-243, -138}},
          /* 2 */ {{-109, -4}},
          /* 3 */ {{4, 109}},
          /* 4 */ {{138, 243}}}},
        {{MHz_u{40}, RuType::RU_242_TONE},
         {/* 1 */ {{-244, -3}},
          /* 2 */ {{3, 244}}}},
        {{MHz_u{40}, RuType::RU_484_TONE}, {/* 1 */ {{-244, -3}, {3, 244}}}},
        {{MHz_u{80}, RuType::RU_26_TONE},
         {/* 1 */ {{-499, -474}},
          /* 2 */ {{-473, -448}},
          /* 3 */ {{-445, -420}},
          /* 4 */ {{-419, -394}},
          /* 5 */ {{-392, -367}},
          /* 6 */ {{-365, -340}},
          /* 7 */ {{-339, -314}},
          /* 8 */ {{-311, -286}},
          /* 9 */ {{-285, -260}},
          /* 10 */ {{-257, -232}},
          /* 11 */ {{-231, -206}},
          /* 12 */ {{-203, -178}},
          /* 13 */ {{-177, -152}},
          /* 14 */ {{-150, -125}},
          /* 15 */ {{-123, -98}},
          /* 16 */ {{-97, -72}},
          /* 17 */ {{-69, -44}},
          /* 18 */ {{-43, -18}},
          /* 19 */ {{-16, -4}, {4, 16}},
          /* 20 */ {{18, 43}},
          /* 21 */ {{44, 69}},
          /* 22 */ {{72, 97}},
          /* 23 */ {{98, 123}},
          /* 24 */ {{125, 150}},
          /* 25 */ {{152, 177}},
          /* 26 */ {{178, 203}},
          /* 27 */ {{206, 231}},
          /* 28 */ {{232, 257}},
          /* 29 */ {{260, 285}},
          /* 30 */ {{286, 311}},
          /* 31 */ {{314, 339}},
          /* 32 */ {{340, 365}},
          /* 33 */ {{367, 392}},
          /* 34 */ {{394, 419}},
          /* 35 */ {{420, 445}},
          /* 36 */ {{448, 473}},
          /* 37 */ {{474, 499}}}},
        {{MHz_u{80}, RuType::RU_52_TONE},
         {/* 1 */ {{-499, -448}},
          /* 2 */ {{-445, -394}},
          /* 3 */ {{-365, -314}},
          /* 4 */ {{-311, -260}},
          /* 5 */ {{-257, -206}},
          /* 6 */ {{-203, -152}},
          /* 7 */ {{-123, -72}},
          /* 8 */ {{-69, -18}},
          /* 9 */ {{18, 69}},
          /* 10 */ {{72, 123}},
          /* 11 */ {{152, 203}},
          /* 12 */ {{206, 257}},
          /* 13 */ {{260, 311}},
          /* 14 */ {{314, 365}},
          /* 15 */ {{394, 445}},
          /* 16 */ {{448, 499}}}},
        {{MHz_u{80}, RuType::RU_106_TONE},
         {/* 1 */ {{-499, -394}},
          /* 2 */ {{-365, -260}},
          /* 3 */ {{-257, -152}},
          /* 4 */ {{-123, -18}},
          /* 5 */ {{18, 123}},
          /* 6 */ {{152, 257}},
          /* 7 */ {{260, 365}},
          /* 8 */ {{394, 499}}}},
        {{MHz_u{80}, RuType::RU_242_TONE},
         {/* 1 */ {{-500, -259}},
          /* 2 */ {{-258, -17}},
          /* 3 */ {{17, 258}},
          /* 4 */ {{259, 500}}}},
        {{MHz_u{80}, RuType::RU_484_TONE},
         {/* 1 */ {{-500, -17}},
          /* 2 */ {{17, 500}}}},
        {{MHz_u{80}, RuType::RU_996_TONE}, {/* 1 */ {{-500, -3}, {3, 500}}}},
        {{MHz_u{160}, RuType::RU_26_TONE},
         {/* 1 */ {{-1011, -986}},
          /* 2 */ {{-985, -960}},
          /* 3 */ {{-957, -932}},
          /* 4 */ {{-931, -906}},
          /* 5 */ {{-904, -879}},
          /* 6 */ {{-877, -852}},
          /* 7 */ {{-851, -826}},
          /* 8 */ {{-823, -798}},
          /* 9 */ {{-797, -772}},
          /* 10 */ {{-769, -744}},
          /* 11 */ {{-743, -718}},
          /* 12 */ {{-715, -690}},
          /* 13 */ {{-689, -664}},
          /* 14 */ {{-662, -637}},
          /* 15 */ {{-635, -610}},
          /* 16 */ {{-609, -584}},
          /* 17 */ {{-581, -556}},
          /* 18 */ {{-555, -530}},
          /* 19 */ {{{-528, -516}, {-508, -496}}},
          /* 20 */ {{-494, -469}},
          /* 21 */ {{-468, -443}},
          /* 22 */ {{-440, -415}},
          /* 23 */ {{-414, -389}},
          /* 24 */ {{-387, -362}},
          /* 25 */ {{-360, -335}},
          /* 26 */ {{-334, -309}},
          /* 27 */ {{-306, -281}},
          /* 28 */ {{-280, -255}},
          /* 29 */ {{-252, -227}},
          /* 30 */ {{-226, -201}},
          /* 31 */ {{-198, -173}},
          /* 32 */ {{-172, -147}},
          /* 33 */ {{-145, -120}},
          /* 34 */ {{-118, -93}},
          /* 35 */ {{-92, -67}},
          /* 36 */ {{-64, -39}},
          /* 37 */ {{-38, -13}},
          /* 38 */ {{13, 38}},
          /* 39 */ {{39, 64}},
          /* 40 */ {{67, 92}},
          /* 41 */ {{93, 118}},
          /* 42 */ {{120, 145}},
          /* 43 */ {{147, 172}},
          /* 44 */ {{173, 198}},
          /* 45 */ {{201, 226}},
          /* 46 */ {{227, 252}},
          /* 47 */ {{255, 280}},
          /* 48 */ {{281, 306}},
          /* 49 */ {{309, 334}},
          /* 50 */ {{335, 360}},
          /* 51 */ {{362, 387}},
          /* 52 */ {{389, 414}},
          /* 53 */ {{415, 440}},
          /* 54 */ {{443, 468}},
          /* 55 */ {{469, 494}},
          /* 56 */ {{496, 508}, {516, 528}},
          /* 57 */ {{530, 555}},
          /* 58 */ {{556, 581}},
          /* 59 */ {{584, 609}},
          /* 60 */ {{610, 635}},
          /* 61 */ {{637, 662}},
          /* 62 */ {{664, 689}},
          /* 63 */ {{690, 715}},
          /* 64 */ {{718, 743}},
          /* 65 */ {{744, 769}},
          /* 66 */ {{772, 797}},
          /* 67 */ {{798, 823}},
          /* 68 */ {{826, 851}},
          /* 69 */ {{852, 877}},
          /* 70 */ {{879, 904}},
          /* 71 */ {{906, 931}},
          /* 72 */ {{932, 957}},
          /* 73 */ {{960, 985}},
          /* 74 */ {{986, 1011}}}},
        {{MHz_u{160}, RuType::RU_52_TONE},
         {/* 1 */ {{-1011, -960}},
          /* 2 */ {{-957, -906}},
          /* 3 */ {{-877, -826}},
          /* 4 */ {{-823, -772}},
          /* 5 */ {{-769, -718}},
          /* 6 */ {{-715, -664}},
          /* 7 */ {{-635, -584}},
          /* 8 */ {{-581, -530}},
          /* 9 */ {{-494, -443}},
          /* 10 */ {{-440, -389}},
          /* 11 */ {{-360, -309}},
          /* 12 */ {{-306, -255}},
          /* 13 */ {{-252, -201}},
          /* 14 */ {{-198, -147}},
          /* 15 */ {{-118, -67}},
          /* 16 */ {{-64, -13}},
          /* 17 */ {{13, 64}},
          /* 18 */ {{67, 118}},
          /* 19 */ {{147, 198}},
          /* 20 */ {{201, 252}},
          /* 21 */ {{255, 306}},
          /* 22 */ {{309, 360}},
          /* 23 */ {{389, 440}},
          /* 24 */ {{443, 494}},
          /* 25 */ {{530, 581}},
          /* 26 */ {{584, 635}},
          /* 27 */ {{664, 715}},
          /* 28 */ {{718, 769}},
          /* 29 */ {{772, 823}},
          /* 30 */ {{826, 877}},
          /* 31 */ {{906, 957}},
          /* 32 */ {{960, 1011}}}},
        {{MHz_u{160}, RuType::RU_106_TONE},
         {/* 1 */ {{-1011, -906}},
          /* 2 */ {{-877, -772}},
          /* 3 */ {{-769, -664}},
          /* 4 */ {{-635, -530}},
          /* 5 */ {{-494, -389}},
          /* 6 */ {{-360, -255}},
          /* 7 */ {{-252, -147}},
          /* 8 */ {{-118, -13}},
          /* 9 */ {{13, 118}},
          /* 10 */ {{147, 252}},
          /* 11 */ {{255, 360}},
          /* 12 */ {{389, 494}},
          /* 13 */ {{530, 635}},
          /* 14 */ {{664, 769}},
          /* 15 */ {{772, 877}},
          /* 16 */ {{906, 1011}}}},
        {{MHz_u{160}, RuType::RU_242_TONE},
         {/* 1 */ {{-1012, -771}},
          /* 2 */ {{-770, -529}},
          /* 3 */ {{-495, -254}},
          /* 4 */ {{-253, -12}},
          /* 5 */ {{12, 253}},
          /* 6 */ {{254, 495}},
          /* 7 */ {{529, 770}},
          /* 8 */ {{771, 1012}}}},
        {{MHz_u{160}, RuType::RU_484_TONE},
         {/* 1 */ {{-1012, -529}},
          /* 2 */ {{-495, -12}},
          /* 3 */ {{12, 495}},
          /* 4 */ {{529, 1012}}}},
        {{MHz_u{160}, RuType::RU_996_TONE},
         {/* 1 */ {{-1012, -515}, {-509, -12}},
          /* 2 */ {{12, 509}, {515, 1012}}}},
        {{MHz_u{160}, RuType::RU_2x996_TONE},
         {/* 1 */ {{-1012, -515}, {-509, -12}, {12, 509}, {515, 1012}}}},
    };

    const std::map<BwTonesPair, std::vector<SubcarrierGroup>> expectedEhtRuSubcarrierGroups = {
        {{MHz_u{20}, RuType::RU_26_TONE},
         {/* 1 */ {{-121, -96}},
          /* 2 */ {{-95, -70}},
          /* 3 */ {{-68, -43}},
          /* 4 */ {{-42, -17}},
          /* 5 */ {{-16, -4}, {4, 16}},
          /* 6 */ {{17, 42}},
          /* 7 */ {{43, 68}},
          /* 8 */ {{70, 95}},
          /* 9 */ {{96, 121}}}},
        {{MHz_u{20}, RuType::RU_52_TONE},
         {/* 1 */ {{-121, -70}},
          /* 2 */ {{-68, -17}},
          /* 3 */ {{17, 68}},
          /* 4 */ {{70, 121}}}},
        {{MHz_u{20}, RuType::RU_106_TONE},
         {/* 1 */ {{-122, -17}},
          /* 2 */ {{17, 122}}}},
        {{MHz_u{20}, RuType::RU_242_TONE}, {/* 1 */ {{-122, -2}, {2, 122}}}},
        {{MHz_u{40}, RuType::RU_26_TONE},
         {/* 1 */ {{-243, -218}},
          /* 2 */ {{-217, -192}},
          /* 3 */ {{-189, -164}},
          /* 4 */ {{-163, -138}},
          /* 5 */ {{-136, -111}},
          /* 6 */ {{-109, -84}},
          /* 7 */ {{-83, -58}},
          /* 8 */ {{-55, -30}},
          /* 9 */ {{-29, -4}},
          /* 10 */ {{4, 29}},
          /* 11 */ {{30, 55}},
          /* 12 */ {{58, 83}},
          /* 13 */ {{84, 109}},
          /* 14 */ {{111, 136}},
          /* 15 */ {{138, 163}},
          /* 16 */ {{164, 189}},
          /* 17 */ {{192, 217}},
          /* 18 */ {{218, 243}}}},
        {{MHz_u{40}, RuType::RU_52_TONE},
         {/* 1 */ {{-243, -192}},
          /* 2 */ {{-189, -138}},
          /* 3 */ {{-109, -58}},
          /* 4 */ {{-55, -4}},
          /* 5 */ {{4, 55}},
          /* 6 */ {{58, 109}},
          /* 7 */ {{138, 189}},
          /* 8 */ {{192, 243}}}},
        {{MHz_u{40}, RuType::RU_106_TONE},
         {/* 1 */ {{-243, -138}},
          /* 2 */ {{-109, -4}},
          /* 3 */ {{4, 109}},
          /* 4 */ {{138, 243}}}},
        {{MHz_u{40}, RuType::RU_242_TONE},
         {/* 1 */ {{-244, -3}},
          /* 2 */ {{3, 244}}}},
        {{MHz_u{40}, RuType::RU_484_TONE}, {/* 1 */ {{-244, -3}, {3, 244}}}},
        {{MHz_u{80}, RuType::RU_26_TONE},
         {/* 1 */ {{-499, -474}},
          /* 2 */ {{-473, -448}},
          /* 3 */ {{-445, -420}},
          /* 4 */ {{-419, -394}},
          /* 5 */ {{-392, -367}},
          /* 6 */ {{-365, -340}},
          /* 7 */ {{-339, -314}},
          /* 8 */ {{-311, -286}},
          /* 9 */ {{-285, -260}},
          /* 10 */ {{-252, -227}},
          /* 11 */ {{-226, -201}},
          /* 12 */ {{-198, -173}},
          /* 13 */ {{-172, -147}},
          /* 14 */ {{-145, -120}},
          /* 15 */ {{-118, -93}},
          /* 16 */ {{-92, -67}},
          /* 17 */ {{-64, -39}},
          /* 18 */ {{-38, -13}},
          /* 19 not defined */ {},
          /* 20 */ {{13, 38}},
          /* 21 */ {{39, 64}},
          /* 22 */ {{67, 92}},
          /* 23 */ {{93, 118}},
          /* 24 */ {{120, 145}},
          /* 25 */ {{147, 172}},
          /* 26 */ {{173, 198}},
          /* 27 */ {{201, 226}},
          /* 28 */ {{227, 252}},
          /* 29 */ {{260, 285}},
          /* 30 */ {{286, 311}},
          /* 31 */ {{314, 339}},
          /* 32 */ {{340, 365}},
          /* 33 */ {{367, 392}},
          /* 34 */ {{394, 419}},
          /* 35 */ {{420, 445}},
          /* 36 */ {{448, 473}},
          /* 37 */ {{474, 499}}}},
        {{MHz_u{80}, RuType::RU_52_TONE},
         {/* 1 */ {{-499, -448}},
          /* 2 */ {{-445, -394}},
          /* 3 */ {{-365, -314}},
          /* 4 */ {{-311, -260}},
          /* 5 */ {{-252, -201}},
          /* 6 */ {{-198, -147}},
          /* 7 */ {{-118, -67}},
          /* 8 */ {{-64, -13}},
          /* 9 */ {{13, 64}},
          /* 10 */ {{67, 118}},
          /* 11 */ {{147, 198}},
          /* 12 */ {{201, 252}},
          /* 13 */ {{260, 311}},
          /* 14 */ {{314, 365}},
          /* 15 */ {{394, 445}},
          /* 16 */ {{448, 499}}}},
        {{MHz_u{80}, RuType::RU_106_TONE},
         {/* 1 */ {{-499, -394}},
          /* 2 */ {{-365, -260}},
          /* 3 */ {{-252, -147}},
          /* 4 */ {{-118, -13}},
          /* 5 */ {{13, 118}},
          /* 6 */ {{147, 252}},
          /* 7 */ {{260, 365}},
          /* 8 */ {{394, 499}}}},
        {{MHz_u{80}, RuType::RU_242_TONE},
         {/* 1 */ {{-500, -259}},
          /* 2 */ {{-253, -12}},
          /* 3 */ {{12, 253}},
          /* 4 */ {{259, 500}}}},
        {{MHz_u{80}, RuType::RU_484_TONE},
         {/* 1 */ {{-500, -259}, {-253, -12}},
          /* 2 */ {{12, 253}, {259, 500}}}},
        {{MHz_u{80}, RuType::RU_996_TONE}, {/* 1 */ {{-500, -3}, {3, 500}}}},
        {{MHz_u{160}, RuType::RU_26_TONE},
         {/* 1 */ {{-1011, -986}},
          /* 2 */ {{-985, -960}},
          /* 3 */ {{-957, -932}},
          /* 4 */ {{-931, -906}},
          /* 5 */ {{-904, -879}},
          /* 6 */ {{-877, -852}},
          /* 7 */ {{-851, -826}},
          /* 8 */ {{-823, -798}},
          /* 9 */ {{-797, -772}},
          /* 10 */ {{-764, -739}},
          /* 11 */ {{-738, -713}},
          /* 12 */ {{-710, -685}},
          /* 13 */ {{-684, -659}},
          /* 14 */ {{-657, -632}},
          /* 15 */ {{-630, -605}},
          /* 16 */ {{-604, -579}},
          /* 17 */ {{-576, -551}},
          /* 18 */ {{-550, -525}},
          /* 19 not defined */ {},
          /* 20 */ {{-499, -474}},
          /* 21 */ {{-473, -448}},
          /* 22 */ {{-445, -420}},
          /* 23 */ {{-419, -394}},
          /* 24 */ {{-392, -367}},
          /* 25 */ {{-365, -340}},
          /* 26 */ {{-339, -314}},
          /* 27 */ {{-311, -286}},
          /* 28 */ {{-285, -260}},
          /* 29 */ {{-252, -227}},
          /* 30 */ {{-226, -201}},
          /* 31 */ {{-198, -173}},
          /* 32 */ {{-172, -147}},
          /* 33 */ {{-145, -120}},
          /* 34 */ {{-118, -93}},
          /* 35 */ {{-92, -67}},
          /* 36 */ {{-64, -39}},
          /* 37 */ {{-38, -13}},
          /* 38 */ {{13, 38}},
          /* 39 */ {{39, 64}},
          /* 40 */ {{67, 92}},
          /* 41 */ {{93, 118}},
          /* 42 */ {{120, 145}},
          /* 43 */ {{147, 172}},
          /* 44 */ {{173, 198}},
          /* 45 */ {{201, 226}},
          /* 46 */ {{227, 252}},
          /* 47 */ {{260, 285}},
          /* 48 */ {{286, 311}},
          /* 49 */ {{314, 339}},
          /* 50 */ {{340, 365}},
          /* 51 */ {{367, 392}},
          /* 52 */ {{394, 419}},
          /* 53 */ {{420, 445}},
          /* 54 */ {{448, 473}},
          /* 55 */ {{474, 499}},
          /* 56 not defined */ {},
          /* 57 */ {{525, 550}},
          /* 58 */ {{551, 576}},
          /* 59 */ {{579, 604}},
          /* 60 */ {{605, 630}},
          /* 61 */ {{632, 657}},
          /* 62 */ {{659, 684}},
          /* 63 */ {{685, 710}},
          /* 64 */ {{713, 738}},
          /* 65 */ {{739, 764}},
          /* 66 */ {{772, 797}},
          /* 67 */ {{798, 823}},
          /* 68 */ {{826, 851}},
          /* 69 */ {{852, 877}},
          /* 70 */ {{879, 904}},
          /* 71 */ {{906, 931}},
          /* 72 */ {{932, 957}},
          /* 73 */ {{960, 985}},
          /* 74 */ {{986, 1011}}}},
        {{MHz_u{160}, RuType::RU_52_TONE},
         {/* 1 */ {{-1011, -960}},
          /* 2 */ {{-957, -906}},
          /* 3 */ {{-877, -826}},
          /* 4 */ {{-823, -772}},
          /* 5 */ {{-764, -713}},
          /* 6 */ {{-710, -659}},
          /* 7 */ {{-630, -579}},
          /* 8 */ {{-576, -525}},
          /* 9 */ {{-499, -448}},
          /* 10 */ {{-445, -394}},
          /* 11 */ {{-365, -314}},
          /* 12 */ {{-311, -260}},
          /* 13 */ {{-252, -201}},
          /* 14 */ {{-198, -147}},
          /* 15 */ {{-118, -67}},
          /* 16 */ {{-64, -13}},
          /* 17 */ {{13, 64}},
          /* 18 */ {{67, 118}},
          /* 19 */ {{147, 198}},
          /* 20 */ {{201, 252}},
          /* 21 */ {{260, 311}},
          /* 22 */ {{314, 365}},
          /* 23 */ {{394, 445}},
          /* 24 */ {{448, 499}},
          /* 25 */ {{525, 576}},
          /* 26 */ {{579, 630}},
          /* 27 */ {{659, 710}},
          /* 28 */ {{713, 764}},
          /* 29 */ {{772, 823}},
          /* 30 */ {{826, 877}},
          /* 31 */ {{906, 957}},
          /* 32 */ {{960, 1011}}}},
        {{MHz_u{160}, RuType::RU_106_TONE},
         {/* 1 */ {{-1011, -906}},
          /* 2 */ {{-877, -772}},
          /* 3 */ {{-764, -659}},
          /* 4 */ {{-630, -525}},
          /* 5 */ {{-499, -394}},
          /* 6 */ {{-365, -260}},
          /* 7 */ {{-252, -147}},
          /* 8 */ {{-118, -13}},
          /* 9 */ {{13, 118}},
          /* 10 */ {{147, 252}},
          /* 11 */ {{260, 365}},
          /* 12 */ {{394, 499}},
          /* 13 */ {{525, 630}},
          /* 14 */ {{659, 764}},
          /* 15 */ {{772, 877}},
          /* 16 */ {{906, 1011}}}},
        {{MHz_u{160}, RuType::RU_242_TONE},
         {/* 1 */ {{-1012, -771}},
          /* 2 */ {{-765, -524}},
          /* 3 */ {{-500, -259}},
          /* 4 */ {{-253, -12}},
          /* 5 */ {{12, 253}},
          /* 6 */ {{259, 500}},
          /* 7 */ {{524, 765}},
          /* 8 */ {{771, 1012}}}},
        {{MHz_u{160}, RuType::RU_484_TONE},
         {/* 1 */ {{-1012, -771}, {-765, -524}},
          /* 2 */ {{-500, -259}, {-253, -12}},
          /* 3 */ {{12, 253}, {259, 500}},
          /* 4 */ {{524, 765}, {771, 1012}}}},
        {{MHz_u{160}, RuType::RU_996_TONE},
         {/* 1 */ {{-1012, -515}, {-509, -12}},
          /* 2 */ {{12, 509}, {515, 1012}}}},
        {{MHz_u{160}, RuType::RU_2x996_TONE},
         {/* 1 */ {{-1012, -515}, {-509, -12}, {12, 509}, {515, 1012}}}},
        {{MHz_u{320}, RuType::RU_26_TONE},
         {/* 1 */ {{-2035, -2010}},
          /* 2 */ {{-2009, -1984}},
          /* 3 */ {{-1981, -1956}},
          /* 4 */ {{-1955, -1930}},
          /* 5 */ {{-1928, -1903}},
          /* 6 */ {{-1901, -1876}},
          /* 7 */ {{-1875, -1850}},
          /* 8 */ {{-1847, -1822}},
          /* 9 */ {{-1821, -1796}},
          /* 10 */ {{-1788, -1763}},
          /* 11 */ {{-1762, -1737}},
          /* 12 */ {{-1734, -1709}},
          /* 13 */ {{-1708, -1683}},
          /* 14 */ {{-1681, -1656}},
          /* 15 */ {{-1654, -1629}},
          /* 16 */ {{-1628, -1603}},
          /* 17 */ {{-1600, -1575}},
          /* 18 */ {{-1574, -1549}},
          /* 19 not defined */ {},
          /* 20 */ {{-1523, -1498}},
          /* 21 */ {{-1497, -1472}},
          /* 22 */ {{-1469, -1444}},
          /* 23 */ {{-1443, -1418}},
          /* 24 */ {{-1416, -1391}},
          /* 25 */ {{-1389, -1364}},
          /* 26 */ {{-1363, -1338}},
          /* 27 */ {{-1335, -1310}},
          /* 28 */ {{-1309, -1284}},
          /* 29 */ {{-1276, -1251}},
          /* 30 */ {{-1250, -1225}},
          /* 31 */ {{-1222, -1197}},
          /* 32 */ {{-1196, -1171}},
          /* 33 */ {{-1169, -1144}},
          /* 34 */ {{-1142, -1117}},
          /* 35 */ {{-1116, -1091}},
          /* 36 */ {{-1088, -1063}},
          /* 37 */ {{-1062, -1037}},
          /* 38 */ {{-1011, -986}},
          /* 39 */ {{-985, -960}},
          /* 40 */ {{-957, -932}},
          /* 41 */ {{-931, -906}},
          /* 42 */ {{-904, -879}},
          /* 43 */ {{-877, -852}},
          /* 44 */ {{-851, -826}},
          /* 45 */ {{-823, -798}},
          /* 46 */ {{-797, -772}},
          /* 47 */ {{-764, -739}},
          /* 48 */ {{-738, -713}},
          /* 49 */ {{-710, -685}},
          /* 50 */ {{-684, -659}},
          /* 51 */ {{-657, -632}},
          /* 52 */ {{-630, -605}},
          /* 53 */ {{-604, -579}},
          /* 54 */ {{-576, -551}},
          /* 55 */ {{-550, -525}},
          /* 56 not defined */ {},
          /* 57 */ {{-499, -474}},
          /* 58 */ {{-473, -448}},
          /* 59 */ {{-445, -420}},
          /* 60 */ {{-419, -394}},
          /* 61 */ {{-392, -367}},
          /* 62 */ {{-365, -340}},
          /* 63 */ {{-339, -314}},
          /* 64 */ {{-311, -286}},
          /* 65 */ {{-285, -260}},
          /* 66 */ {{-252, -227}},
          /* 67 */ {{-226, -201}},
          /* 68 */ {{-198, -173}},
          /* 69 */ {{-172, -147}},
          /* 70 */ {{-145, -120}},
          /* 71 */ {{-118, -93}},
          /* 72 */ {{-92, -67}},
          /* 73 */ {{-64, -39}},
          /* 74 */ {{-38, -13}},
          /* 75 */ {{13, 38}},
          /* 76 */ {{39, 64}},
          /* 77 */ {{67, 92}},
          /* 78 */ {{93, 118}},
          /* 79 */ {{120, 145}},
          /* 80 */ {{147, 172}},
          /* 81 */ {{173, 198}},
          /* 82 */ {{201, 226}},
          /* 83 */ {{227, 252}},
          /* 84 */ {{260, 285}},
          /* 85 */ {{286, 311}},
          /* 86 */ {{314, 339}},
          /* 87 */ {{340, 365}},
          /* 88 */ {{367, 392}},
          /* 89 */ {{394, 419}},
          /* 90 */ {{420, 445}},
          /* 91 */ {{448, 473}},
          /* 92 */ {{474, 499}},
          /* 93 not defined */ {},
          /* 94 */ {{525, 550}},
          /* 95 */ {{551, 576}},
          /* 96 */ {{579, 604}},
          /* 97 */ {{605, 630}},
          /* 98 */ {{632, 657}},
          /* 99 */ {{659, 684}},
          /* 100 */ {{685, 710}},
          /* 101 */ {{713, 738}},
          /* 102 */ {{739, 764}},
          /* 103 */ {{772, 797}},
          /* 104 */ {{798, 823}},
          /* 105 */ {{826, 851}},
          /* 106 */ {{852, 877}},
          /* 107 */ {{879, 904}},
          /* 108 */ {{906, 931}},
          /* 109 */ {{932, 957}},
          /* 110 */ {{960, 985}},
          /* 111 */ {{986, 1011}},
          /* 112 */ {{1037, 1062}},
          /* 113 */ {{1063, 1088}},
          /* 114 */ {{1091, 1116}},
          /* 115 */ {{1117, 1142}},
          /* 116 */ {{1144, 1169}},
          /* 117 */ {{1171, 1196}},
          /* 118 */ {{1197, 1222}},
          /* 119 */ {{1225, 1250}},
          /* 120 */ {{1251, 1276}},
          /* 121 */ {{1284, 1309}},
          /* 122 */ {{1310, 1335}},
          /* 123 */ {{1338, 1363}},
          /* 124 */ {{1364, 1389}},
          /* 125 */ {{1391, 1416}},
          /* 126 */ {{1418, 1443}},
          /* 127 */ {{1444, 1469}},
          /* 128 */ {{1472, 1497}},
          /* 129 */ {{1498, 1523}},
          /* 130 not defined */ {},
          /* 131 */ {{1549, 1574}},
          /* 132 */ {{1575, 1600}},
          /* 133 */ {{1603, 1628}},
          /* 134 */ {{1629, 1654}},
          /* 135 */ {{1656, 1681}},
          /* 136 */ {{1683, 1708}},
          /* 137 */ {{1709, 1734}},
          /* 138 */ {{1737, 1762}},
          /* 139 */ {{1763, 1788}},
          /* 140 */ {{1796, 1821}},
          /* 141 */ {{1822, 1847}},
          /* 142 */ {{1850, 1875}},
          /* 143 */ {{1876, 1901}},
          /* 144 */ {{1903, 1928}},
          /* 145 */ {{1930, 1955}},
          /* 146 */ {{1956, 1981}},
          /* 147 */ {{1984, 2009}},
          /* 148 */ {{2010, 2035}}}},
        {{MHz_u{320}, RuType::RU_52_TONE},
         {/* 1 */ {{-2035, -1984}},
          /* 2 */ {{-1981, -1930}},
          /* 3 */ {{-1901, -1850}},
          /* 4 */ {{-1847, -1796}},
          /* 5 */ {{-1788, -1737}},
          /* 6 */ {{-1734, -1683}},
          /* 7 */ {{-1654, -1603}},
          /* 8 */ {{-1600, -1549}},
          /* 9 */ {{-1523, -1472}},
          /* 10 */ {{-1469, -1418}},
          /* 11 */ {{-1389, -1338}},
          /* 12 */ {{-1335, -1284}},
          /* 13 */ {{-1276, -1225}},
          /* 14 */ {{-1222, -1171}},
          /* 15 */ {{-1142, -1091}},
          /* 16 */ {{-1088, -1037}},
          /* 17 */ {{-1011, -960}},
          /* 18 */ {{-957, -906}},
          /* 19 */ {{-877, -826}},
          /* 20 */ {{-823, -772}},
          /* 21 */ {{-764, -713}},
          /* 22 */ {{-710, -659}},
          /* 23 */ {{-630, -579}},
          /* 24 */ {{-576, -525}},
          /* 25 */ {{-499, -448}},
          /* 26 */ {{-445, -394}},
          /* 27 */ {{-365, -314}},
          /* 28 */ {{-311, -260}},
          /* 29 */ {{-252, -201}},
          /* 30 */ {{-198, -147}},
          /* 31 */ {{-118, -67}},
          /* 32 */ {{-64, -13}},
          /* 33 */ {{13, 64}},
          /* 34 */ {{67, 118}},
          /* 35 */ {{147, 198}},
          /* 36 */ {{201, 252}},
          /* 37 */ {{260, 311}},
          /* 38 */ {{314, 365}},
          /* 39 */ {{394, 445}},
          /* 40 */ {{448, 499}},
          /* 41 */ {{525, 576}},
          /* 42 */ {{579, 630}},
          /* 43 */ {{659, 710}},
          /* 44 */ {{713, 764}},
          /* 45 */ {{772, 823}},
          /* 46 */ {{826, 877}},
          /* 47 */ {{906, 957}},
          /* 48 */ {{960, 1011}},
          /* 49 */ {{1037, 1088}},
          /* 50 */ {{1091, 1142}},
          /* 51 */ {{1171, 1222}},
          /* 52 */ {{1225, 1276}},
          /* 53 */ {{1284, 1335}},
          /* 54 */ {{1338, 1389}},
          /* 55 */ {{1418, 1469}},
          /* 56 */ {{1472, 1523}},
          /* 57 */ {{1549, 1600}},
          /* 58 */ {{1603, 1654}},
          /* 59 */ {{1683, 1734}},
          /* 60 */ {{1737, 1788}},
          /* 61 */ {{1796, 1847}},
          /* 62 */ {{1850, 1901}},
          /* 63 */ {{1930, 1981}},
          /* 64 */ {{1984, 2035}}}},
        {{MHz_u{320}, RuType::RU_106_TONE},
         {/* 1 */ {{-2035, -1930}},
          /* 2 */ {{-1901, -1796}},
          /* 3 */ {{-1788, -1683}},
          /* 4 */ {{-1654, -1549}},
          /* 5 */ {{-1523, -1418}},
          /* 6 */ {{-1389, -1284}},
          /* 7 */ {{-1276, -1171}},
          /* 8 */ {{-1142, -1037}},
          /* 9 */ {{-1011, -906}},
          /* 10 */ {{-877, -772}},
          /* 11 */ {{-764, -659}},
          /* 12 */ {{-630, -525}},
          /* 13 */ {{-499, -394}},
          /* 14 */ {{-365, -260}},
          /* 15 */ {{-252, -147}},
          /* 16 */ {{-118, -13}},
          /* 17 */ {{13, 118}},
          /* 18 */ {{147, 252}},
          /* 19 */ {{260, 365}},
          /* 20 */ {{394, 499}},
          /* 21 */ {{525, 630}},
          /* 22 */ {{659, 764}},
          /* 23 */ {{772, 877}},
          /* 24 */ {{906, 1011}},
          /* 25 */ {{1037, 1142}},
          /* 26 */ {{1171, 1276}},
          /* 27 */ {{1284, 1389}},
          /* 28 */ {{1418, 1523}},
          /* 29 */ {{1549, 1654}},
          /* 30 */ {{1683, 1788}},
          /* 31 */ {{1796, 1901}},
          /* 32 */ {{1930, 2035}}}},
        {{MHz_u{320}, RuType::RU_242_TONE},
         {/* 1 */ {{-2036, -1795}},
          /* 2 */ {{-1789, -1548}},
          /* 3 */ {{-1524, -1283}},
          /* 4 */ {{-1277, -1036}},
          /* 5 */ {{-1012, -771}},
          /* 6 */ {{-765, -524}},
          /* 7 */ {{-500, -259}},
          /* 8 */ {{-253, -12}},
          /* 9 */ {{12, 253}},
          /* 10 */ {{259, 500}},
          /* 11 */ {{524, 765}},
          /* 12 */ {{771, 1012}},
          /* 13 */ {{1036, 1277}},
          /* 14 */ {{1283, 1524}},
          /* 15 */ {{1548, 1789}},
          /* 16 */ {{1795, 2036}}}},
        {{MHz_u{320}, RuType::RU_484_TONE},
         {/* 1 */ {{-2036, -1795}, {-1789, -1548}},
          /* 2 */ {{-1524, -1283}, {-1277, -1036}},
          /* 3 */ {{-1012, -771}, {-765, -524}},
          /* 4 */ {{-500, -259}, {-253, -12}},
          /* 5 */ {{12, 253}, {259, 500}},
          /* 6 */ {{524, 765}, {771, 1012}},
          /* 7 */ {{1036, 1277}, {1283, 1524}},
          /* 8 */ {{1548, 1789}, {1795, 2036}}}},
        {{MHz_u{320}, RuType::RU_996_TONE},
         {/* 1 */ {{-2036, -1539}, {-1533, -1036}},
          /* 2 */ {{-1012, -515}, {-509, -12}},
          /* 3 */ {{12, 509}, {515, 1012}},
          /* 4 */ {{1036, 1533}, {1539, 2036}}}},
        {{MHz_u{320}, RuType::RU_2x996_TONE},
         {/* 1 */ {{-2036, -1539}, {-1533, -1036}, {-1012, -515}, {-509, -12}},
          /* 2 */ {{12, 509}, {515, 1012}, {1036, 1533}, {1539, 2036}}}},
        {{MHz_u{320}, RuType::RU_4x996_TONE},
         {/* 1 */ {{-2036, -1539},
                   {-1533, -1036},
                   {-1012, -515},
                   {-509, -12},
                   {12, 509},
                   {515, 1012},
                   {1036, 1533},
                   {1539, 2036}}}},
    };

    const auto& expectedRuSubcarrierGroups = (m_modClass == WIFI_MOD_CLASS_HE)
                                                 ? expectedHeRuSubcarrierGroups
                                                 : expectedEhtRuSubcarrierGroups;
    for (const auto& [bwTonesPair, ruSubcarrierGroups] : expectedRuSubcarrierGroups)
    {
        std::size_t phyIndex = 1;
        for (const auto& subcarrierGroups : ruSubcarrierGroups)
        {
            RunOne(bwTonesPair.first, bwTonesPair.second, phyIndex++, subcarrierGroups);
        }
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the methods to convert PHY indices to 80MHz indices with primary flags.
 */
class WifiRuPhyIdxTo80MHzIdxAndFlagsTest : public TestCase
{
  public:
    /**
     * Constructor
     * @param modClass the modulation class to distinguish 802.11ax and 802.11be
     */
    WifiRuPhyIdxTo80MHzIdxAndFlagsTest(WifiModulationClass modClass);

    /**
     * Check converted PHY indices to 80MHz indices with primary flag are correct.
     *
     * @param primary20 the index of the primary20 channel to configure
     * @param bw the bandwidth
     * @param ruType the RU type
     * @param phyIndex the PHY index of the RU
     * @param expectedP160 the expected P160 flag
     * @param expectedP80OrLower80 the expected P80OrLower80 flag
     * @param expected80MHzIdx the expected index within the 80 MHz segment
     */
    void RunOne(uint8_t primary20,
                MHz_u bw,
                RuType ruType,
                std::size_t phyIndex,
                bool expectedP160,
                bool expectedP80OrLower80,
                std::size_t expected80MHzIdx);

  private:
    void DoRun() override;

    WifiModulationClass m_modClass; ///< the modulation class to consider for the test
};

WifiRuPhyIdxTo80MHzIdxAndFlagsTest::WifiRuPhyIdxTo80MHzIdxAndFlagsTest(WifiModulationClass modClass)
    : TestCase{"Check conversion from PHY indices to 80MHz indices with primary flag for " +
               std::string((modClass == WIFI_MOD_CLASS_HE) ? "HE" : "EHT")},
      m_modClass{modClass}
{
}

void
WifiRuPhyIdxTo80MHzIdxAndFlagsTest::RunOne(uint8_t primary20,
                                           MHz_u bw,
                                           RuType ruType,
                                           std::size_t phyIndex,
                                           bool expectedP160,
                                           bool expectedP80OrLower80,
                                           std::size_t expected80MHzIdx)
{
    auto primary80OrLower80{true};
    auto primary160{true};
    std::size_t idx80MHz{1};
    if (m_modClass == WIFI_MOD_CLASS_HE)
    {
        idx80MHz = HeRu::GetIndexIn80MHzSegment(bw, ruType, phyIndex);
        primary80OrLower80 = HeRu::GetPrimary80MHzFlag(bw, ruType, phyIndex, primary20);
    }
    else
    {
        idx80MHz = EhtRu::GetIndexIn80MHzSegment(bw, ruType, phyIndex);
        const auto& [p160, p80OrLower80] = EhtRu::GetPrimaryFlags(bw, ruType, phyIndex, primary20);
        primary160 = p160;
        primary80OrLower80 = p80OrLower80;
    }
    NS_TEST_EXPECT_MSG_EQ(idx80MHz,
                          expected80MHzIdx,
                          "BW=" << bw << ", p20Index=" << +primary20 << " , ruType=" << ruType
                                << " , phyIndex=" << phyIndex << ". Expected 80MHz index "
                                << expected80MHzIdx << " differs from actual " << idx80MHz);
    NS_TEST_EXPECT_MSG_EQ(primary160,
                          expectedP160,
                          "BW=" << bw << ", p20Index=" << +primary20 << " , ruType=" << ruType
                                << " , phyIndex=" << phyIndex << ". Expected P160 flag "
                                << expectedP160 << " differs from actual " << primary160);
    NS_TEST_EXPECT_MSG_EQ(primary80OrLower80,
                          expectedP80OrLower80,
                          "BW=" << bw << ", p20Index=" << +primary20 << " , ruType=" << ruType
                                << " , phyIndex=" << phyIndex << ". Expected P80OrLower80 flag "
                                << expectedP80OrLower80 << " differs from actual "
                                << primary80OrLower80);
}

void
WifiRuPhyIdxTo80MHzIdxAndFlagsTest::DoRun()
{
    const auto p160{true};
    const auto s160{false};
    const auto p80OrLower80{true};
    const auto s80OrHigher80{false};

    // consider maximum bandwidth for the test: 160 MHz for HE and 320 MHz otherwise (EHT)
    uint8_t p20IdxMax = (m_modClass == WIFI_MOD_CLASS_HE) ? 8 : 16;

    /* 20 MHz */
    {
        const MHz_u bw{20};

        for (uint8_t p20Index = 0; p20Index < p20IdxMax; p20Index++)
        {
            std::size_t numRusPer20MHz = 9;
            std::size_t startPhyIdx = p20Index * numRusPer20MHz;
            // All the 26-tone RUs in 20 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer20MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_26_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer20MHz = 4;
            startPhyIdx = p20Index * numRusPer20MHz;
            // All the 52-tone RUs in 20 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer20MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_52_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer20MHz = 2;
            startPhyIdx = p20Index * numRusPer20MHz;
            // Both the 106-tone RUs in 20 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer20MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_106_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer20MHz = 1;
            startPhyIdx = p20Index * numRusPer20MHz;
            // The 242-tone RUs in 20 MHz PPDUs is always in P80 (hence index within 80 MHz segment
            // equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer20MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_242_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }
        }
    }

    /* 40 MHz */
    {
        const MHz_u bw{40};

        for (uint8_t p20Index = 0; p20Index < p20IdxMax; p20Index++)
        {
            std::size_t numRusPer40MHz = 18;
            std::size_t startPhyIdx = (p20Index / 2) * numRusPer40MHz;
            // All the 26-tone RUs in 40 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer40MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_26_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer40MHz = 8;
            startPhyIdx = (p20Index / 2) * numRusPer40MHz;
            // All the 52-tone RUs in 40 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer40MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_52_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer40MHz = 4;
            startPhyIdx = (p20Index / 2) * numRusPer40MHz;
            // All the 106-tone RUs in 40 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer40MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_106_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer40MHz = 2;
            startPhyIdx = (p20Index / 2) * numRusPer40MHz;
            // Both the 242-tone RUs in 40 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer40MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_242_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer40MHz = 1;
            startPhyIdx = (p20Index / 2) * numRusPer40MHz;
            // The 484-tone RUs in 40 MHz PPDUs is always in P80 (hence index within 80 MHz segment
            // equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer40MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_484_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }
        }
    }

    /* 80 MHz */
    {
        const MHz_u bw{80};

        for (uint8_t p20Index = 0; p20Index < p20IdxMax; p20Index++)
        {
            std::size_t numRusPer80MHz = 37;
            std::size_t startPhyIdx = (p20Index / 4) * numRusPer80MHz;
            // All the 26-tone RUs in 80 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                if (phyIdx == 19)
                {
                    // Undefined RU
                    continue;
                }
                RunOne(p20Index, bw, RuType::RU_26_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer80MHz = 16;
            startPhyIdx = (p20Index / 4) * numRusPer80MHz;
            // All the 52-tone RUs in 80 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_52_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer80MHz = 8;
            startPhyIdx = (p20Index / 4) * numRusPer80MHz;
            // All the 106-tone RUs in 80 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_106_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer80MHz = 4;
            startPhyIdx = (p20Index / 4) * numRusPer80MHz;
            // All the 242-tone RUs in 80 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_242_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer80MHz = 2;
            startPhyIdx = (p20Index / 4) * numRusPer80MHz;
            // Both The 484-tone RUs in 80 MHz PPDUs are always in P80 (hence index within 80 MHz
            // segment equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_484_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }

            numRusPer80MHz = 1;
            startPhyIdx = (p20Index / 4) * numRusPer80MHz;
            // The 996-tone RUs in 80 MHz PPDUs is always in P80 (hence index within 80 MHz segment
            // equals PHY index)
            for (std::size_t phyIdx = startPhyIdx; phyIdx <= startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_996_TONE, phyIdx, p160, p80OrLower80, phyIdx);
            }
        }
    }

    /* 160 MHz */
    {
        const MHz_u bw{160};

        for (uint8_t p20Index = 0; p20Index < p20IdxMax; p20Index++)
        {
            const uint8_t p80Index = p20Index / 4;
            const uint8_t s80Index = (p80Index % 2 == 0) ? (p80Index + 1) : (p80Index - 1);

            // 26-tone RUs in P80
            std::size_t numRusPer80MHz = 37;
            std::size_t numRusPer160MHz = 2 * numRusPer80MHz;
            std::size_t startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            std::size_t idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                if ((m_modClass != WIFI_MOD_CLASS_HE) && (idxIn80MHz == 19))
                {
                    // Undefined RU
                    continue;
                }
                RunOne(p20Index, bw, RuType::RU_26_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 26-tone RUs in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                if ((m_modClass != WIFI_MOD_CLASS_HE) && (idxIn80MHz == 19))
                {
                    // Undefined RU
                    continue;
                }
                RunOne(p20Index, bw, RuType::RU_26_TONE, phyIdx, p160, s80OrHigher80, idxIn80MHz++);
            }

            // 52-tone RUs in P80
            numRusPer80MHz = 16;
            numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_52_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 52-tone RUs in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_52_TONE, phyIdx, p160, s80OrHigher80, idxIn80MHz++);
            }

            // 106-tone RUs in P80
            numRusPer80MHz = 8;
            numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_106_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 106-tone RUs in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_106_TONE,
                       phyIdx,
                       p160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 242-tone RUs in P80
            numRusPer80MHz = 4;
            numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_242_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 242-tone RUs in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_242_TONE,
                       phyIdx,
                       p160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 484-tone RUs in P80
            numRusPer80MHz = 2;
            numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_484_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 484-tone RUs in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_484_TONE,
                       phyIdx,
                       p160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 996-tone RU in P80
            numRusPer80MHz = 1;
            numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_996_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 996-tone RU in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer160MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_996_TONE,
                       phyIdx,
                       p160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 2x996-tone RU
            RunOne(p20Index, bw, RuType::RU_2x996_TONE, 1, p160, p80OrLower80, 1);
        }
    }

    /* 320 MHz */
    if (m_modClass == WIFI_MOD_CLASS_EHT)
    {
        const MHz_u bw{320};

        for (uint8_t p20Index = 0; p20Index < p20IdxMax; p20Index++)
        {
            const uint8_t p160Index = p20Index / 8;
            const uint8_t s160Index = (p160Index % 2 == 0) ? (p160Index + 1) : (p160Index - 1);
            const uint8_t p80Index = p20Index / 4;
            const uint8_t s80Index = (p80Index % 2 == 0) ? (p80Index + 1) : (p80Index - 1);

            // 26-tone RUs in P80
            std::size_t numRusPer80MHz = 37;
            std::size_t numRusPer320MHz = 4 * numRusPer80MHz;
            std::size_t startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            std::size_t idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                if ((m_modClass != WIFI_MOD_CLASS_HE) && (idxIn80MHz == 19))
                {
                    // Undefined RU
                    continue;
                }
                RunOne(p20Index, bw, RuType::RU_26_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 26-tone RUs in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                if ((m_modClass != WIFI_MOD_CLASS_HE) && (idxIn80MHz == 19))
                {
                    // Undefined RU
                    continue;
                }
                RunOne(p20Index, bw, RuType::RU_26_TONE, phyIdx, p160, s80OrHigher80, idxIn80MHz++);
            }

            // 26-tone RUs in S160
            std::size_t numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((s160Index * numRusPer160MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            // lower 80 MHz
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                if ((m_modClass != WIFI_MOD_CLASS_HE) && (idxIn80MHz == 19))
                {
                    // Undefined RU
                    continue;
                }
                RunOne(p20Index, bw, RuType::RU_26_TONE, phyIdx, s160, p80OrLower80, idxIn80MHz++);
            }
            // higher 80 MHz
            startPhyIdx += numRusPer80MHz;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                if ((m_modClass != WIFI_MOD_CLASS_HE) && (idxIn80MHz == 19))
                {
                    // Undefined RU
                    continue;
                }
                RunOne(p20Index, bw, RuType::RU_26_TONE, phyIdx, s160, s80OrHigher80, idxIn80MHz++);
            }

            // 52-tone RUs in P80
            numRusPer80MHz = 16;
            numRusPer320MHz = 4 * numRusPer80MHz;
            startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_52_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 52-tone RUs in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_52_TONE, phyIdx, p160, s80OrHigher80, idxIn80MHz++);
            }

            // 52-tone RUs in S160
            numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((s160Index * numRusPer160MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            // lower 80 MHz
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_52_TONE, phyIdx, s160, p80OrLower80, idxIn80MHz++);
            }
            // higher 80 MHz
            startPhyIdx += numRusPer80MHz;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_52_TONE, phyIdx, s160, s80OrHigher80, idxIn80MHz++);
            }

            // 106-tone RUs in P80
            numRusPer80MHz = 8;
            numRusPer320MHz = 4 * numRusPer80MHz;
            startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_106_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 106-tone RUs in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_106_TONE,
                       phyIdx,
                       p160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 106-tone RUs in S160
            numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((s160Index * numRusPer160MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            // lower 80 MHz
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_106_TONE, phyIdx, s160, p80OrLower80, idxIn80MHz++);
            }
            // higher 80 MHz
            startPhyIdx += numRusPer80MHz;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_106_TONE,
                       phyIdx,
                       s160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 242-tone RUs in P80
            numRusPer80MHz = 4;
            numRusPer320MHz = 4 * numRusPer80MHz;
            startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_242_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 242-tone RUs in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_242_TONE,
                       phyIdx,
                       p160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 242-tone RUs in S160
            numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((s160Index * numRusPer160MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            // lower 80 MHz
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_242_TONE, phyIdx, s160, p80OrLower80, idxIn80MHz++);
            }
            // higher 80 MHz
            startPhyIdx += numRusPer80MHz;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_242_TONE,
                       phyIdx,
                       s160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 484-tone RUs in P80
            numRusPer80MHz = 2;
            numRusPer320MHz = 4 * numRusPer80MHz;
            startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_484_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 484-tone RUs in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_484_TONE,
                       phyIdx,
                       p160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 484-tone RUs in S160
            numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((s160Index * numRusPer160MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            // lower 80 MHz
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_484_TONE, phyIdx, s160, p80OrLower80, idxIn80MHz++);
            }
            // higher 80 MHz
            startPhyIdx += numRusPer80MHz;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_484_TONE,
                       phyIdx,
                       s160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 996-tone RU in P80
            numRusPer80MHz = 1;
            numRusPer320MHz = 4 * numRusPer80MHz;
            startPhyIdx = ((p80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_996_TONE, phyIdx, p160, p80OrLower80, idxIn80MHz++);
            }

            // 996-tone RU in S80
            startPhyIdx = ((s80Index * numRusPer80MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_996_TONE,
                       phyIdx,
                       p160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 996-tone RUs in S160
            numRusPer160MHz = 2 * numRusPer80MHz;
            startPhyIdx = ((s160Index * numRusPer160MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            // lower 80 MHz
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index, bw, RuType::RU_996_TONE, phyIdx, s160, p80OrLower80, idxIn80MHz++);
            }
            // higher 80 MHz
            startPhyIdx += numRusPer80MHz;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer80MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_996_TONE,
                       phyIdx,
                       s160,
                       s80OrHigher80,
                       idxIn80MHz++);
            }

            // 2x996-tone RU in P160
            numRusPer160MHz = 1;
            numRusPer320MHz = 2 * numRusPer160MHz;
            startPhyIdx = ((p160Index * numRusPer160MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer160MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_2x996_TONE,
                       phyIdx,
                       p160,
                       p80OrLower80,
                       idxIn80MHz++);
            }

            // 996-tone RU in S80
            startPhyIdx = ((s160Index * numRusPer160MHz) % numRusPer320MHz) + 1;
            idxIn80MHz = 1;
            for (std::size_t phyIdx = startPhyIdx; phyIdx < startPhyIdx + numRusPer160MHz; phyIdx++)
            {
                RunOne(p20Index,
                       bw,
                       RuType::RU_2x996_TONE,
                       phyIdx,
                       s160,
                       p80OrLower80,
                       idxIn80MHz++);
            }

            // 4x996-tone RU
            RunOne(p20Index, bw, RuType::RU_4x996_TONE, 1, p160, p80OrLower80, 1);
        }
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the WifiRu::DoesOverlap() method.
 */
class WifiRuOverlappingTest : public TestCase
{
  public:
    /**
     * Constructor
     * @param modClass the modulation class to distinguish 802.11ax and 802.11be
     */
    WifiRuOverlappingTest(WifiModulationClass modClass);

    /**
     * Check the result of DoesOverlap() is correct.
     *
     * @param bw the bandwidth of the PPDU
     * @param ru the given RU allocation
     * @param rus the given set of RUs
     * @param overlapExpected whether it is expected the given RU overlaps with the given set of RUs
     */
    void RunOne(MHz_u bw,
                WifiRu::RuSpec ru,
                const std::vector<WifiRu::RuSpec>& rus,
                bool overlapExpected);

  private:
    void DoRun() override;

    WifiModulationClass m_modClass; ///< the modulation class to consider for the test
};

WifiRuOverlappingTest::WifiRuOverlappingTest(WifiModulationClass modClass)
    : TestCase{"Check RUs overlapping for " +
               std::string((modClass == WIFI_MOD_CLASS_HE) ? "HE" : "EHT")},
      m_modClass{modClass}
{
}

void
WifiRuOverlappingTest::RunOne(MHz_u bw,
                              WifiRu::RuSpec ru,
                              const std::vector<WifiRu::RuSpec>& rus,
                              bool overlapExpected)
{
    auto printToStr = [](const std::vector<WifiRu::RuSpec>& v) {
        std::stringstream ss;
        ss << "{";
        for (const auto& ru : v)
        {
            ss << ru << " ";
        }
        ss << "}";
        return ss.str();
    };

    const auto overlap = WifiRu::DoesOverlap(bw, ru, rus);
    NS_TEST_EXPECT_MSG_EQ(overlap,
                          overlapExpected,
                          "BW=" << bw << ", ru=" << ru << " , rus=" << printToStr(rus)
                                << ". Expected overlap " << overlapExpected
                                << " differs from actual " << overlap);
}

void
WifiRuOverlappingTest::DoRun()
{
    const auto p80{true};
    const auto s80{false};
    const auto p160{(m_modClass == WIFI_MOD_CLASS_HE) ? std::nullopt : std::optional(true)};
    const auto s160{(m_modClass == WIFI_MOD_CLASS_HE) ? std::nullopt : std::optional(false)};

    /* 20 MHz PPDU */
    {
        const MHz_u bw{20};

        auto ru = MakeRuSpec(RuType::RU_242_TONE, 1, p80, p160);

        // 242-tones RU should overlap with 26-tones RUs in same 80 MHz segment
        RunOne(bw,
               ru,
               {MakeRuSpec(RuType::RU_26_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 2, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 3, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 4, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 5, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 6, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 7, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 8, p80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 9, p80, p160)},
               true);

        // 242-tones RU should not overlap with 26-tones RUs in same 160 MHz segment but different
        // 80 MHz segment
        RunOne(bw,
               ru,
               {MakeRuSpec(RuType::RU_26_TONE, 1, s80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 2, s80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 3, s80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 4, s80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 5, s80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 6, s80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 7, s80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 8, s80, p160),
                MakeRuSpec(RuType::RU_26_TONE, 9, s80, p160)},
               false);

        // 242-tones RU should not overlap with 26-tones RUs in different 160 MHz segment
        if (m_modClass != WIFI_MOD_CLASS_HE)
        {
            RunOne(bw,
                   ru,
                   {MakeRuSpec(RuType::RU_26_TONE, 1, p80, s160),
                    MakeRuSpec(RuType::RU_26_TONE, 2, p80, s160),
                    MakeRuSpec(RuType::RU_26_TONE, 3, p80, s160),
                    MakeRuSpec(RuType::RU_26_TONE, 4, p80, s160),
                    MakeRuSpec(RuType::RU_26_TONE, 5, p80, s160),
                    MakeRuSpec(RuType::RU_26_TONE, 6, p80, s160),
                    MakeRuSpec(RuType::RU_26_TONE, 7, p80, s160),
                    MakeRuSpec(RuType::RU_26_TONE, 8, p80, s160),
                    MakeRuSpec(RuType::RU_26_TONE, 9, p80, s160)},
                   false);
        }
    }

    /* 80 MHz PPDU */
    {
        const MHz_u bw{80};

        auto ru = MakeRuSpec(RuType::RU_106_TONE, 1, p80, p160);

        // 106-tones RU should overlap with 484-tones RUs in same 80 MHz segment
        RunOne(bw,
               ru,
               {MakeRuSpec(RuType::RU_484_TONE, 1, p80, p160),
                MakeRuSpec(RuType::RU_484_TONE, 2, p80, p160)},
               true);

        // 106-tones RU should not overlap with 484-tones RUs in same 160 MHz segment but different
        // 80 MHz segment
        RunOne(bw,
               ru,
               {MakeRuSpec(RuType::RU_484_TONE, 1, s80, p160),
                MakeRuSpec(RuType::RU_484_TONE, 2, s80, p160)},
               false);

        // 106-tones RU should not overlap with 484-tones RUs in different 160 MHz segment
        if (m_modClass != WIFI_MOD_CLASS_HE)
        {
            RunOne(bw,
                   ru,
                   {MakeRuSpec(RuType::RU_484_TONE, 1, s80, s160),
                    MakeRuSpec(RuType::RU_484_TONE, 2, s80, s160)},
                   false);
        }
    }

    /* 160 MHz PPDU */
    {
        const MHz_u bw{160};

        auto ru = MakeRuSpec(RuType::RU_996_TONE, 1, s80, p160);

        // 996-tones RU should overlap with 2x996 RU in same 160 MHz segment
        RunOne(bw, ru, {MakeRuSpec(RuType::RU_2x996_TONE, 1, p80, p160)}, true);

        // 996-tones RU should not overlap with 2x996 RU in different 160 MHz segment
        if (m_modClass != WIFI_MOD_CLASS_HE)
        {
            RunOne(bw, ru, {MakeRuSpec(RuType::RU_2x996_TONE, 1, p80, s160)}, false);
        }
    }

    // TODO: these tests can be further extended with more combinations
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi primary channels test suite
 */
class WifiRuAllocationTestSuite : public TestSuite
{
  public:
    WifiRuAllocationTestSuite();
};

WifiRuAllocationTestSuite::WifiRuAllocationTestSuite()
    : TestSuite("wifi-ru-allocation", Type::UNIT)
{
    AddTestCase(new Wifi20MHzIndicesCoveringRuTest(WIFI_STANDARD_80211ax),
                TestCase::Duration::QUICK);
    AddTestCase(new Wifi20MHzIndicesCoveringRuTest(WIFI_STANDARD_80211be),
                TestCase::Duration::QUICK);
    AddTestCase(new WifiNumRusInChannelTest(WIFI_MOD_CLASS_HE), TestCase::Duration::QUICK);
    AddTestCase(new WifiNumRusInChannelTest(WIFI_MOD_CLASS_EHT), TestCase::Duration::QUICK);
    AddTestCase(new WifiRusOfTypeInChannelTest(WIFI_MOD_CLASS_HE), TestCase::Duration::QUICK);
    AddTestCase(new WifiRusOfTypeInChannelTest(WIFI_MOD_CLASS_EHT), TestCase::Duration::QUICK);
    AddTestCase(new WifiCentral26TonesRusInChannelTest(WIFI_MOD_CLASS_HE),
                TestCase::Duration::QUICK);
    AddTestCase(new WifiCentral26TonesRusInChannelTest(WIFI_MOD_CLASS_EHT),
                TestCase::Duration::QUICK);
    AddTestCase(new WifiEqualSizedRusTest(WIFI_MOD_CLASS_HE), TestCase::Duration::QUICK);
    AddTestCase(new WifiEqualSizedRusTest(WIFI_MOD_CLASS_EHT), TestCase::Duration::QUICK);
    AddTestCase(new WifiSubcarrierGroupsTest(WIFI_MOD_CLASS_HE), TestCase::Duration::QUICK);
    AddTestCase(new WifiSubcarrierGroupsTest(WIFI_MOD_CLASS_EHT), TestCase::Duration::QUICK);
    AddTestCase(new WifiRuPhyIdxTo80MHzIdxAndFlagsTest(WIFI_MOD_CLASS_HE),
                TestCase::Duration::QUICK);
    AddTestCase(new WifiRuPhyIdxTo80MHzIdxAndFlagsTest(WIFI_MOD_CLASS_EHT),
                TestCase::Duration::QUICK);
    AddTestCase(new WifiRuOverlappingTest(WIFI_MOD_CLASS_HE), TestCase::Duration::QUICK);
    AddTestCase(new WifiRuOverlappingTest(WIFI_MOD_CLASS_EHT), TestCase::Duration::QUICK);
}

static WifiRuAllocationTestSuite g_wifiRuAllocationTestSuite; ///< the test suite
