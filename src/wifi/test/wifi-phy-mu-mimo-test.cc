/*
 * Copyright (c) 2022 DERONNE SOFTWARE ENGINEERING
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

#include "ns3/he-phy.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/wifi-tx-vector.h"

#include <list>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPhyMuMimoTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief DL MU TX-VECTOR test
 */
class TestDlMuTxVector : public TestCase
{
  public:
    TestDlMuTxVector();

  private:
    void DoRun() override;

    /**
     * Build a TXVECTOR for DL MU with the given bandwidth and user information.
     *
     * \param bw the channel width of the PPDU in MHz
     * \param userInfos the list of HE MU specific user transmission parameters
     *
     * \return the configured MU TXVECTOR
     */
    static WifiTxVector BuildTxVector(uint16_t bw, std::list<HeMuUserInfo> userInfos);
};

TestDlMuTxVector::TestDlMuTxVector()
    : TestCase("Check for valid combinations of MU TX-VECTOR")
{
}

WifiTxVector
TestDlMuTxVector::BuildTxVector(uint16_t bw, std::list<HeMuUserInfo> userInfos)
{
    WifiTxVector txVector;
    txVector.SetPreambleType(WIFI_PREAMBLE_HE_MU);
    txVector.SetChannelWidth(bw);
    std::list<uint16_t> staIds;
    uint16_t staId = 1;
    for (const auto& userInfo : userInfos)
    {
        txVector.SetHeMuUserInfo(staId, userInfo);
        staIds.push_back(staId++);
    }
    return txVector;
}

void
TestDlMuTxVector::DoRun()
{
    // Verify TxVector is OFDMA
    std::list<HeMuUserInfo> userInfos;
    userInfos.push_back({{HeRu::RU_106_TONE, 1, true}, 11, 1});
    userInfos.push_back({{HeRu::RU_106_TONE, 2, true}, 10, 2});
    WifiTxVector txVector = BuildTxVector(20, userInfos);
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlOfdma(),
                          true,
                          "TX-VECTOR should indicate an OFDMA transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlMuMimo(),
                          false,
                          "TX-VECTOR should not indicate a MU-MIMO transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsSigBCompression(),
                          false,
                          "TX-VECTOR should not indicate a SIG-B compression");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsValid(),
                          true,
                          "TX-VECTOR should indicate all checks are passed");
    userInfos.clear();

    // Verify TxVector is a full BW MU-MIMO
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 11, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 10, 2});
    txVector = BuildTxVector(20, userInfos);
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlOfdma(),
                          false,
                          "TX-VECTOR should indicate a MU-MIMO transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlMuMimo(),
                          true,
                          "TX-VECTOR should not indicate an OFDMA transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsSigBCompression(),
                          true,
                          "TX-VECTOR should indicate a SIG-B compression");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsValid(),
                          true,
                          "TX-VECTOR should indicate all checks are passed");
    userInfos.clear();

    // Verify TxVector is not valid if there are more than 8 STAs using the same RU
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 11, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 10, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 9, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 8, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 7, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 6, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 5, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 4, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 3, 1});
    txVector = BuildTxVector(20, userInfos);
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlOfdma(),
                          false,
                          "TX-VECTOR should indicate a MU-MIMO transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlMuMimo(),
                          true,
                          "TX-VECTOR should not indicate an OFDMA transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsSigBCompression(),
                          true,
                          "TX-VECTOR should indicate a SIG-B compression");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsValid(),
                          false,
                          "TX-VECTOR should not indicate all checks are passed");

    // Verify TxVector is not valid if the total number of antennas in a full BW MU-MIMO is above 8
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 11, 2});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 10, 2});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 9, 3});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 8, 3});
    txVector = BuildTxVector(20, userInfos);
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlOfdma(),
                          false,
                          "TX-VECTOR should indicate a MU-MIMO transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlMuMimo(),
                          true,
                          "TX-VECTOR should not indicate an OFDMA transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsSigBCompression(),
                          true,
                          "TX-VECTOR should indicate a SIG-B compression");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsValid(),
                          false,
                          "TX-VECTOR should not indicate all checks are passed");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi PHY MU-MIMO Test Suite
 */
class WifiPhyMuMimoTestSuite : public TestSuite
{
  public:
    WifiPhyMuMimoTestSuite();
};

WifiPhyMuMimoTestSuite::WifiPhyMuMimoTestSuite()
    : TestSuite("wifi-phy-mu-mimo", UNIT)
{
    AddTestCase(new TestDlMuTxVector, TestCase::QUICK);
}

static WifiPhyMuMimoTestSuite WifiPhyMuMimoTestSuite; ///< the test suite
