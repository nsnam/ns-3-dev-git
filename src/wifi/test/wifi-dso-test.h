/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_DSO_TEST_H
#define WIFI_DSO_TEST_H

#include "ns3/ap-wifi-mac.h"
#include "ns3/nstime.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-types.h"

#include <string>
#include <vector>

using namespace ns3;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Base class for DSO tests
 *
 * This base class setups and configures one AP STA, a variable number of non-AP STAs with
 * DSO activated and a variable number of non-AP STAs with DSO deactivated.
 */
class DsoTestBase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name The name of the new TestCase created
     */
    explicit DsoTestBase(const std::string& name);

  protected:
    void DoSetup() override;

    std::size_t m_nDsoStas{1};    ///< number of UHR non-AP STAs with DSO enabled
    std::size_t m_nNonDsoStas{0}; ///< number of UHR non-AP STAs with DSO disabled
    std::size_t m_nNonUhrStas{0}; ///< number of non-UHR non-AP STAs
    std::string m_apOpChannel;    ///< string representing the operating channel of the AP
    std::string m_stasOpChannel;  ///< string representing the operating channel of the non-AP STAs
    std::vector<FrequencyRange> m_stasFreqRanges; ///< the frequency ranges covered by each spectrum
                                                  ///< PHY interface for DSO STAs
    Ptr<ApWifiMac> m_apMac;                       ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs;       ///< MACs of the non-AP STAs
    Time m_duration;                              ///< simulation duration
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the computation of the primary and DSO subbands.
 *
 * This test considers an AP STA and a non-AP STA with DSO activated.
 */
class DsoSubbandsTest : public DsoTestBase
{
  public:
    /**
     * Constructor
     *
     * @param apChannel The operating channel of the AP
     * @param stasChannel The operating channel of the STAs
     * @param expectedDsoSubbands The expected DSO subbands
     */
    DsoSubbandsTest(
        const std::string& apChannel,
        const std::string& stasChannel,
        const std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel>& expectedDsoSubbands);

  protected:
    void DoRun() override;

  private:
    std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel>
        m_expectedDsoSubbands; //!< expected DSO subbands
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi DSO Test Suite
 */
class WifiDsoTestSuite : public TestSuite
{
  public:
    WifiDsoTestSuite();
};

#endif /* WIFI_DSO_TEST_H */
