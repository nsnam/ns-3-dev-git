/*
 * Copyright (c) 2017 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 */

#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/wifi-phy-band.h"
#include "ns3/wifi-phy-common.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-standards.h"

#include <cmath>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiTransmitMaskTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test checks if Wifi spectrum values for OFDM are generated properly.
 * Different test cases are configured by defining different standards and bandwidth.
 */
class WifiOfdmMaskSlopesTestCase : public TestCase
{
  public:
    /**
     * typedef for a pair of sub-band index and relative power value
     */
    typedef std::pair<uint32_t, dBr_u> IndexPowerPair;

    /**
     * typedef for a vector of pairs of sub-band index and relative power value
     */
    typedef std::vector<IndexPowerPair> IndexPowerVect;

    /**
     * Constructor
     *
     * @param name test reference name
     * @param standard selected standard
     * @param band selected PHY band
     * @param channelWidth total channel width
     * @param centerFrequencies the center frequency per contiguous segment
     * @param maskRefs vector of expected power values and corresponding indexes of generated PSD
     *                     (only start and stop indexes/values given)
     * @param tolerance tolerance
     * @param precision precision (in decimals)
     * @param puncturedSubchannels bitmap indicating whether a 20 MHz subchannel is punctured or not
     * (only for 802.11ax and later)
     */
    WifiOfdmMaskSlopesTestCase(const std::string& name,
                               WifiStandard standard,
                               WifiPhyBand band,
                               MHz_u channelWidth,
                               const std::vector<MHz_u>& centerFrequencies,
                               const IndexPowerVect& maskRefs,
                               dB_u tolerance,
                               std::size_t precision,
                               const std::vector<bool>& puncturedSubchannels = std::vector<bool>{});
    ~WifiOfdmMaskSlopesTestCase() override = default;

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Interpolate PSD values for indexes between provided start and stop and append to provided
     * vector.
     *
     * @param vect vector of sub-band index and relative power value pairs to which interpolated
     values should be appended
     * @param start pair of sub-band index and relative power value for interval start
     * @param stop pair of sub-band index and relative power value for interval stop
    */
    void InterpolateAndAppendValues(IndexPowerVect& vect,
                                    IndexPowerPair start,
                                    IndexPowerPair stop) const;

    WifiStandard m_standard;          ///< the wifi standard to test
    WifiPhyBand m_band;               ///< the wifi PHY band to test
    MHz_u m_channelWidth;             ///< the total channel width to test
    std::vector<MHz_u> m_centerFreqs; ///< the center frequency per contiguous segment to test
    std::vector<bool>
        m_puncturedSubchannels; ///< bitmap indicating whether a 20 MHz subchannel is punctured or
                                ///< not (only used for 802.11ax and later)
    Ptr<SpectrumValue> m_actualSpectrum; ///< actual spectrum value
    IndexPowerVect m_expectedPsd;        ///< expected power values
    dB_u m_tolerance;                    ///< tolerance
    std::size_t m_precision;             ///< precision for double calculations (in decimals)
};

WifiOfdmMaskSlopesTestCase::WifiOfdmMaskSlopesTestCase(
    const std::string& name,
    WifiStandard standard,
    WifiPhyBand band,
    MHz_u channelWidth,
    const std::vector<MHz_u>& centerFrequencies,
    const IndexPowerVect& maskRefs,
    dB_u tolerance,
    std::size_t precision,
    const std::vector<bool>& puncturedSubchannels)
    : TestCase(std::string("SpectrumValue ") + name),
      m_standard{standard},
      m_band{band},
      m_channelWidth{channelWidth},
      m_centerFreqs{centerFrequencies},
      m_puncturedSubchannels{puncturedSubchannels},
      m_actualSpectrum{},
      m_expectedPsd{maskRefs},
      m_tolerance{tolerance},
      m_precision{precision}
{
    NS_LOG_FUNCTION(this << name << standard << band << channelWidth << tolerance << precision
                         << puncturedSubchannels.size());
}

void
WifiOfdmMaskSlopesTestCase::DoSetup()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(!m_centerFreqs.empty());
    NS_ASSERT(m_expectedPsd.size() % 2 == 0); // start/stop pairs expected

    dBr_u outerBandMaximumRejection{0.0};
    switch (m_band)
    {
    default:
    case WIFI_PHY_BAND_5GHZ:
        outerBandMaximumRejection = dBr_u{-40};
        break;
    case WIFI_PHY_BAND_2_4GHZ:
        outerBandMaximumRejection = (m_standard >= WIFI_STANDARD_80211n) ? dBr_u{-45} : dBr_u{-40};
        break;
    case WIFI_PHY_BAND_6GHZ:
        outerBandMaximumRejection = dBr_u{-40};
        break;
    }

    Watt_u refTxPower{1}; // have to work in dBr when comparing though
    switch (m_standard)
    {
    case WIFI_STANDARD_80211p:
        NS_ASSERT(m_band == WIFI_PHY_BAND_5GHZ);
        NS_ASSERT((m_channelWidth == MHz_u{5}) || (m_channelWidth == MHz_u{10}));
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(m_centerFreqs.front(),
                                                                      m_channelWidth,
                                                                      refTxPower,
                                                                      m_channelWidth,
                                                                      dBr_u{-20.0},
                                                                      dBr_u{-28.0},
                                                                      outerBandMaximumRejection);
        break;

    case WIFI_STANDARD_80211g:
        NS_ASSERT(m_band == WIFI_PHY_BAND_2_4GHZ);
        NS_ASSERT(m_channelWidth == MHz_u{20});
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(m_centerFreqs.front(),
                                                                      m_channelWidth,
                                                                      refTxPower,
                                                                      m_channelWidth,
                                                                      dBr_u{-20.0},
                                                                      dBr_u{-28.0},
                                                                      outerBandMaximumRejection);
        break;

    case WIFI_STANDARD_80211a:
        NS_ASSERT(m_band == WIFI_PHY_BAND_5GHZ);
        NS_ASSERT(m_channelWidth == MHz_u{20});
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(m_centerFreqs.front(),
                                                                      m_channelWidth,
                                                                      refTxPower,
                                                                      m_channelWidth,
                                                                      dBr_u{-20.0},
                                                                      dBr_u{-28.0},
                                                                      outerBandMaximumRejection);
        break;

    case WIFI_STANDARD_80211n:
        NS_ASSERT(m_channelWidth == MHz_u{20} || m_channelWidth == MHz_u{40});
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity(m_centerFreqs,
                                                                        m_channelWidth,
                                                                        refTxPower,
                                                                        m_channelWidth,
                                                                        dBr_u{-20.0},
                                                                        dBr_u{-28.0},
                                                                        outerBandMaximumRejection);
        break;

    case WIFI_STANDARD_80211ac:
        NS_ASSERT(m_band == WIFI_PHY_BAND_5GHZ);
        NS_ASSERT(m_channelWidth == MHz_u{20} || m_channelWidth == MHz_u{40} ||
                  m_channelWidth == MHz_u{80} || m_channelWidth == MHz_u{160});
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity(m_centerFreqs,
                                                                        m_channelWidth,
                                                                        refTxPower,
                                                                        m_channelWidth,
                                                                        dBr_u{-20.0},
                                                                        dBr_u{-28.0},
                                                                        outerBandMaximumRejection);
        break;

    case WIFI_STANDARD_80211ax:
        NS_ASSERT((m_band != WIFI_PHY_BAND_2_4GHZ) ||
                  (m_channelWidth < MHz_u{80})); // not enough space in 2.4 GHz bands
        NS_ASSERT(m_channelWidth == MHz_u{20} || m_channelWidth == MHz_u{40} ||
                  m_channelWidth == MHz_u{80} || m_channelWidth == MHz_u{160});
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(m_centerFreqs,
                                                                        m_channelWidth,
                                                                        refTxPower,
                                                                        m_channelWidth,
                                                                        dBr_u{-20.0},
                                                                        dBr_u{-28.0},
                                                                        outerBandMaximumRejection,
                                                                        m_puncturedSubchannels);
        break;

    default:
        NS_FATAL_ERROR("Standard unknown or non-OFDM");
        break;
    }

    NS_LOG_INFO("Build expected PSD");
    IndexPowerVect builtPsd;
    for (uint32_t i = 0; i < m_expectedPsd.size(); i += 2)
    {
        InterpolateAndAppendValues(builtPsd, m_expectedPsd[i], m_expectedPsd[i + 1]);
    }
    m_expectedPsd = builtPsd;
}

void
WifiOfdmMaskSlopesTestCase::InterpolateAndAppendValues(IndexPowerVect& vect,
                                                       IndexPowerPair start,
                                                       IndexPowerPair stop) const
{
    NS_LOG_FUNCTION(start.first << start.second << stop.first << stop.second);
    NS_ASSERT(start.first <= stop.first);

    if (start.first == stop.first) // only one point, no need to interpolate
    {
        NS_ASSERT(start.second == stop.second);
        vect.push_back(start);
        NS_LOG_LOGIC("Append (" << start.first << ", " << stop.second << ")");
        return;
    }

    const auto slope = (stop.second - start.second) / (stop.first - start.first);
    for (uint32_t i = start.first; i <= stop.first; i++)
    {
        const auto delta{i - start.first};
        dB_u val{start.second + slope * delta};
        const auto multiplier = std::round(std::pow(10.0, static_cast<double>(m_precision)));
        val = dB_u{std::floor(val * multiplier + 0.5) / multiplier};
        vect.emplace_back(i, val);
        NS_LOG_LOGIC("Append (" << i << ", " << val << ")");
    }

    NS_ASSERT(vect.back().first == stop.first &&
              TestDoubleIsEqual(vect.back().second, stop.second, m_tolerance));
}

void
WifiOfdmMaskSlopesTestCase::DoRun()
{
    NS_LOG_FUNCTION(this);
    dBr_u currentPower{0.0}; // have to work in dBr so as to compare with expected slopes
    Watt_u maxPower{(*m_actualSpectrum)[0]};
    for (auto&& vit = m_actualSpectrum->ConstValuesBegin();
         vit != m_actualSpectrum->ConstValuesEnd();
         ++vit)
    {
        maxPower = std::max(maxPower, Watt_u{*vit});
    }

    NS_LOG_INFO("Compare expected PSD");
    for (const auto& [subcarrier, expectedValue] : m_expectedPsd)
    {
        currentPower = dBr_u{10.0 * std::log10((*m_actualSpectrum)[subcarrier] / maxPower)};
        NS_LOG_LOGIC("For " << subcarrier << ", expected: " << expectedValue
                            << " vs obtained: " << currentPower);
        NS_TEST_EXPECT_MSG_EQ_TOL(currentPower,
                                  expectedValue,
                                  m_tolerance,
                                  "Spectrum value mismatch for subcarrier " << subcarrier);
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test suite for checking the consistency of different OFDM-based transmit masks.
 */
class WifiTransmitMaskTestSuite : public TestSuite
{
  public:
    WifiTransmitMaskTestSuite();
};

static WifiTransmitMaskTestSuite g_WifiTransmitMaskTestSuite;

WifiTransmitMaskTestSuite::WifiTransmitMaskTestSuite()
    : TestSuite("wifi-transmit-mask", Type::UNIT)
{
    // LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);
    // LogComponentEnable ("WifiTransmitMaskTest", logLevel);
    // LogComponentEnable ("WifiSpectrumValueHelper", logLevel);

    NS_LOG_INFO("Creating WifiTransmitMaskTestSuite");

    WifiOfdmMaskSlopesTestCase::IndexPowerVect maskSlopes;
    dB_u tol{10e-2};
    double prec = 10; // in decimals

    // ============================================================================================
    // 11p 5MHz
    NS_LOG_FUNCTION("Check slopes for 11p 5MHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),     // Outer band left (start)
        std::make_pair(31, dBr_u{-28.375}),  // Outer band left (stop)
        std::make_pair(32, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(60, dBr_u{-20.276}),  // Middle band left (stop)
        std::make_pair(61, dBr_u{-20.0}),    // Flat junction band left (start)
        std::make_pair(63, dBr_u{-20.0}),    // Flat junction band left (stop)
        std::make_pair(64, dBr_u{-20.0}),    // Inner band left (start)
        std::make_pair(69, dBr_u{-3.333}),   // Inner band left (stop)
        std::make_pair(123, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(128, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(129, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(131, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(132, dBr_u{-20.276}), // Middle band right (start)
        std::make_pair(160, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(161, dBr_u{-28.375}), // Outer band right (start)
        std::make_pair(192, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11p 5MHz",
                                               WIFI_STANDARD_80211p,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{5},
                                               {MHz_u{5860}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11p 10MHz
    NS_LOG_FUNCTION("Check slopes for 11p 10MHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),     // Outer band left (start)
        std::make_pair(31, dBr_u{-28.375}),  // Outer band left (stop)
        std::make_pair(32, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(60, dBr_u{-20.276}),  // Middle band left (stop)
        std::make_pair(61, dBr_u{-20.0}),    // Flat junction band left (start)
        std::make_pair(63, dBr_u{-20.0}),    // Flat junction band left (stop)
        std::make_pair(64, dBr_u{-20.0}),    // Inner band left (start)
        std::make_pair(69, dBr_u{-3.333}),   // Inner band left (stop)
        std::make_pair(123, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(128, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(129, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(131, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(132, dBr_u{-20.276}), // Middle band right (start)
        std::make_pair(160, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(161, dBr_u{-28.375}), // Outer band right (start)
        std::make_pair(192, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11p 10MHz",
                                               WIFI_STANDARD_80211p,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{10},
                                               {MHz_u{5860}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11a
    NS_LOG_FUNCTION("Check slopes for 11a");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),     // Outer band left (start)
        std::make_pair(31, dBr_u{-28.375}),  // Outer band left (stop)
        std::make_pair(32, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(60, dBr_u{-20.276}),  // Middle band left (stop)
        std::make_pair(61, dBr_u{-20.0}),    // Flat junction band left (start)
        std::make_pair(63, dBr_u{-20.0}),    // Flat junction band left (stop)
        std::make_pair(64, dBr_u{-20.0}),    // Inner band left (start)
        std::make_pair(69, dBr_u{-3.333}),   // Inner band left (stop)
        std::make_pair(123, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(128, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(129, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(131, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(132, dBr_u{-20.276}), // Middle band right (start)
        std::make_pair(160, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(161, dBr_u{-28.375}), // Outer band right (start)
        std::make_pair(192, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11a",
                                               WIFI_STANDARD_80211a,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{20},
                                               {MHz_u{5180}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11g
    NS_LOG_FUNCTION("Check slopes for 11g");
    // same slopes as 11a
    AddTestCase(new WifiOfdmMaskSlopesTestCase("11g",
                                               WIFI_STANDARD_80211g,
                                               WIFI_PHY_BAND_2_4GHZ,
                                               MHz_u{20},
                                               {MHz_u{2412}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11n 20MHz @ 2.4GHz
    NS_LOG_FUNCTION("Check slopes for 11n 20MHz @ 2.4GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-45.000}),   // Outer band left (start)
        std::make_pair(31, dBr_u{-28.531}),  // Outer band left (stop)
        std::make_pair(32, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(60, dBr_u{-20.276}),  // Middle band left (stop)
        std::make_pair(61, dBr_u{-20.0}),    // Flat junction band left (start)
        std::make_pair(61, dBr_u{-20.0}),    // Flat junction band left (stop)
        std::make_pair(62, dBr_u{-20.0}),    // Inner band left (start)
        std::make_pair(67, dBr_u{-3.333}),   // Inner band left (stop)
        std::make_pair(125, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(130, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(131, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(131, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(132, dBr_u{-20.276}), // Middle band right (start)
        std::make_pair(160, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(161, dBr_u{-28.531}), // Outer band right (start)
        std::make_pair(192, dBr_u{-45.000}), // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11n_2.4GHz 20MHz",
                                               WIFI_STANDARD_80211n,
                                               WIFI_PHY_BAND_2_4GHZ,
                                               MHz_u{20},
                                               {MHz_u{2412}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11n 20MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11n 20MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),     // Outer band left (start)
        std::make_pair(31, dBr_u{-28.375}),  // Outer band left (stop)
        std::make_pair(32, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(60, dBr_u{-20.276}),  // Middle band left (stop)
        std::make_pair(61, dBr_u{-20.0}),    // Flat junction band left (start)
        std::make_pair(61, dBr_u{-20.0}),    // Flat junction band left (stop)
        std::make_pair(62, dBr_u{-20.0}),    // Inner band left (start)
        std::make_pair(67, dBr_u{-3.333}),   // Inner band left (stop)
        std::make_pair(125, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(130, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(131, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(131, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(132, dBr_u{-20.276}), // Middle band right (start)
        std::make_pair(160, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(161, dBr_u{-28.375}), // Outer band right (start)
        std::make_pair(192, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11n_5GHz 20MHz",
                                               WIFI_STANDARD_80211n,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{20},
                                               {MHz_u{5180}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11n 40MHz @ 2.4GHz
    NS_LOG_FUNCTION("Check slopes for 11n 40MHz @ 2.4GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-45.000}),   // Outer band left (start)
        std::make_pair(63, dBr_u{-28.266}),  // Outer band left (stop)
        std::make_pair(64, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(124, dBr_u{-20.131}), // Middle band left (stop)
        std::make_pair(125, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(125, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(126, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(131, dBr_u{-3.333}),  // Inner band left (stop)
        std::make_pair(253, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(258, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(259, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(259, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(260, dBr_u{-20.131}), // Middle band right (start)
        std::make_pair(320, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(321, dBr_u{-28.266}), // Outer band right (start)
        std::make_pair(384, dBr_u{-45.000}), // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11n_2.4GHz 40MHz",
                                               WIFI_STANDARD_80211n,
                                               WIFI_PHY_BAND_2_4GHZ,
                                               MHz_u{40},
                                               {MHz_u{2422}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11n 20MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11n 40MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),     // Outer band left (start)
        std::make_pair(63, dBr_u{-28.188}),  // Outer band left (stop)
        std::make_pair(64, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(124, dBr_u{-20.131}), // Middle band left (stop)
        std::make_pair(125, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(125, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(126, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(131, dBr_u{-3.333}),  // Inner band left (stop)
        std::make_pair(253, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(258, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(259, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(259, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(260, dBr_u{-20.131}), // Middle band right (start)
        std::make_pair(320, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(321, dBr_u{-28.188}), // Outer band right (start)
        std::make_pair(384, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11n_5GHz 40MHz",
                                               WIFI_STANDARD_80211n,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{40},
                                               {MHz_u{5190}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ac 20MHz
    NS_LOG_FUNCTION("Check slopes for 11ac 20MHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),     // Outer band left (start)
        std::make_pair(31, dBr_u{-28.375}),  // Outer band left (stop)
        std::make_pair(32, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(60, dBr_u{-20.276}),  // Middle band left (stop)
        std::make_pair(61, dBr_u{-20.0}),    // Flat junction band left (start)
        std::make_pair(61, dBr_u{-20.0}),    // Flat junction band left (stop)
        std::make_pair(62, dBr_u{-20.0}),    // Inner band left (start)
        std::make_pair(67, dBr_u{-3.333}),   // Inner band left (stop)
        std::make_pair(125, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(130, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(131, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(131, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(132, dBr_u{-20.276}), // Middle band right (start)
        std::make_pair(160, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(161, dBr_u{-28.375}), // Outer band right (start)
        std::make_pair(192, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ac 20MHz",
                                               WIFI_STANDARD_80211ac,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{20},
                                               {MHz_u{5180}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ac 20MHz
    NS_LOG_FUNCTION("Check slopes for 11ac 40MHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),     // Outer band left (start)
        std::make_pair(63, dBr_u{-28.188}),  // Outer band left (stop)
        std::make_pair(64, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(124, dBr_u{-20.131}), // Middle band left (stop)
        std::make_pair(125, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(125, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(126, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(131, dBr_u{-3.333}),  // Inner band left (stop)
        std::make_pair(253, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(258, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(259, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(259, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(260, dBr_u{-20.131}), // Middle band right (start)
        std::make_pair(320, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(321, dBr_u{-28.188}), // Outer band right (start)
        std::make_pair(384, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ac 40MHz",
                                               WIFI_STANDARD_80211ac,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{40},
                                               {MHz_u{5190}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ac 80MHz
    NS_LOG_FUNCTION("Check slopes for 11ac 80MHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),     // Outer band left (start)
        std::make_pair(127, dBr_u{-28.094}), // Outer band left (stop)
        std::make_pair(128, dBr_u{-28.000}), // Middle band left (start)
        std::make_pair(252, dBr_u{-20.064}), // Middle band left (stop)
        std::make_pair(253, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(253, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(254, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(259, dBr_u{-3.333}),  // Inner band left (stop)
        std::make_pair(509, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(514, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(515, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(515, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(516, dBr_u{-20.064}), // Middle band right (start)
        std::make_pair(640, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(641, dBr_u{-28.094}), // Outer band right (start)
        std::make_pair(768, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ac 80MHz",
                                               WIFI_STANDARD_80211ac,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{80},
                                               {MHz_u{5210}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ac 20MHz
    NS_LOG_FUNCTION("Check slopes for 11ac 160MHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(255, dBr_u{-28.047}),  // Outer band left (stop)
        std::make_pair(256, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(508, dBr_u{-20.032}),  // Middle band left (stop)
        std::make_pair(509, dBr_u{-20.0}),    // Flat junction band left (start)
        std::make_pair(509, dBr_u{-20.0}),    // Flat junction band left (stop)
        std::make_pair(510, dBr_u{-20.0}),    // Inner band left (start)
        std::make_pair(515, dBr_u{-3.333}),   // Inner band left (stop)
        std::make_pair(1021, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(1026, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(1027, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(1027, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(1028, dBr_u{-20.032}), // Middle band right (start)
        std::make_pair(1280, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(1281, dBr_u{-28.047}), // Outer band right (start)
        std::make_pair(1536, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ac 160MHz",
                                               WIFI_STANDARD_80211ac,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{160},
                                               {MHz_u{5250}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ac 80+80MHz
    NS_LOG_FUNCTION("Check slopes for 11ac 80+80MHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),     // Outer band left (start)
        std::make_pair(127, dBr_u{-28.094}), // Outer band left (stop)
        std::make_pair(128, dBr_u{-28.000}), // Middle band left (start)
        std::make_pair(252, dBr_u{-20.064}), // Middle band left (stop)
        std::make_pair(253, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(253, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(254, dBr_u{-20.0}),   // Inner band left for first segment (start)
        std::make_pair(259, dBr_u{-3.333}),  // Inner band left for first segment (stop)
        std::make_pair(509, dBr_u{-3.333}),  // Inner band right for first segment (start)
        std::make_pair(514, dBr_u{-20.0}),   // Inner band right for first segment (stop)
        std::make_pair(515, dBr_u{-20.0}),   // Flat junction band right for first segment (start)
        std::make_pair(515, dBr_u{-20.0}),   // Flat junction band right for first segment (stop)
        std::make_pair(516, dBr_u{-20.01}),  // start linear sum region left (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(516, dBr_u{-20.01}),  // start linear sum region left (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(639, dBr_u{-24.99}),  // stop linear sum region left (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(639, dBr_u{-24.99}),  // stop linear sum region left (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(640, dBr_u{-25.0}),   // middle linear sum region (no interpolation possible,
                                             // so provide 2 times the same point)
        std::make_pair(640, dBr_u{-25.0}),   // middle linear sum region (no interpolation possible,
                                             // so provide 2 times the same point)
        std::make_pair(641, dBr_u{-24.99}),  // start linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(641, dBr_u{-24.99}),  // start linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(764, dBr_u{-20.01}),  // stop linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(764, dBr_u{-20.01}),  // stop linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(765, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(765, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(766, dBr_u{-20.0}),   // Inner band left for second segment (start)
        std::make_pair(771, dBr_u{-3.333}),  // Inner band left for second segment (stop)
        std::make_pair(1021, dBr_u{-3.333}), // Inner band right for second segment (start)
        std::make_pair(1026, dBr_u{-20.0}),  // Inner band right for second segment (stop)
        std::make_pair(1027, dBr_u{-20.0}),  // Flat junction band right (start)
        std::make_pair(1027, dBr_u{-20.0}),  // Flat junction band right (stop)
        std::make_pair(1028, dBr_u{-20.016}), // Middle band right (start)
        std::make_pair(1152, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(1153, dBr_u{-28.023}), // Outer band right (start)
        std::make_pair(1280, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ac 80+80MHz",
                                               WIFI_STANDARD_80211ac,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{160},
                                               {MHz_u{5530}, MHz_u{5690}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 20MHz @ 2.4GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 20MHz @ 2.4GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-45.000}),   // Outer band left (start)
        std::make_pair(127, dBr_u{-28.133}), // Outer band left (stop)
        std::make_pair(128, dBr_u{-28.000}), // Middle band left (start)
        std::make_pair(252, dBr_u{-20.064}), // Middle band left (stop)
        std::make_pair(253, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(255, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(256, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(261, dBr_u{-3.333}),  // Inner band left (stop)
        std::make_pair(262, dBr_u{0.0}),     // allocated band left (start)
        std::make_pair(382, dBr_u{0.0}),     // allocated band left (stop)
        std::make_pair(383, dBr_u{-20.0}),   // DC band (start)
        std::make_pair(385, dBr_u{-20.0}),   // DC band (stop)
        std::make_pair(386, dBr_u{0.0}),     // allocated band right (start)
        std::make_pair(506, dBr_u{0.0}),     // allocated band right (stop)
        std::make_pair(507, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(512, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(513, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(515, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(516, dBr_u{-20.064}), // Middle band right (start)
        std::make_pair(640, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(641, dBr_u{-28.133}), // Outer band right (start)
        std::make_pair(768, dBr_u{-45.000}), // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_2.4GHz 20MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_2_4GHZ,
                                               MHz_u{20},
                                               {MHz_u{2412}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 20MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 20MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),     // Outer band left (start)
        std::make_pair(127, dBr_u{-28.094}), // Outer band left (stop)
        std::make_pair(128, dBr_u{-28.000}), // Middle band left (start)
        std::make_pair(252, dBr_u{-20.064}), // Middle band left (stop)
        std::make_pair(253, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(255, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(256, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(261, dBr_u{-3.333}),  // Inner band left (stop)
        std::make_pair(262, dBr_u{0.0}),     // allocated band left (start)
        std::make_pair(382, dBr_u{0.0}),     // allocated band left (stop)
        std::make_pair(383, dBr_u{-20.0}),   // DC band (start)
        std::make_pair(385, dBr_u{-20.0}),   // DC band (stop)
        std::make_pair(386, dBr_u{0.0}),     // allocated band right (start)
        std::make_pair(506, dBr_u{0.0}),     // allocated band right (stop)
        std::make_pair(507, dBr_u{-3.333}),  // Inner band right (start)
        std::make_pair(512, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(513, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(515, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(516, dBr_u{-20.064}), // Middle band right (start)
        std::make_pair(640, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(641, dBr_u{-28.094}), // Outer band right (start)
        std::make_pair(768, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 20MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{20},
                                               {MHz_u{5180}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 40MHz @ 2.4GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 40MHz @ 2.4GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-45.000}),    // Outer band left (start)
        std::make_pair(255, dBr_u{-28.066}),  // Outer band left (stop)
        std::make_pair(256, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(505, dBr_u{-20.032}),  // Middle band left (stop)
        std::make_pair(506, dBr_u{-20.0}),    // Flat junction band left (start)
        std::make_pair(510, dBr_u{-20.0}),    // Flat junction band left (stop)
        std::make_pair(511, dBr_u{-20.0}),    // Inner band left (start)
        std::make_pair(523, dBr_u{-1.538}),   // Inner band left (stop)
        std::make_pair(524, dBr_u{0.0}),      // allocated band left (start)
        std::make_pair(765, dBr_u{0.0}),      // allocated band left (stop)
        std::make_pair(766, dBr_u{-20.0}),    // DC band (start)
        std::make_pair(770, dBr_u{-20.0}),    // DC band (stop)
        std::make_pair(771, dBr_u{0.0}),      // allocated band right (start)
        std::make_pair(1012, dBr_u{0.0}),     // allocated band right (stop)
        std::make_pair(1013, dBr_u{-1.538}),  // Inner band right (start)
        std::make_pair(1025, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(1026, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(1030, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(1031, dBr_u{-20.032}), // Middle band right (start)
        std::make_pair(1280, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(1281, dBr_u{-28.066}), // Outer band right (start)
        std::make_pair(1536, dBr_u{-45.000}), // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_2.4GHz 40MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_2_4GHZ,
                                               MHz_u{40},
                                               {MHz_u{2422}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 40MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 40MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(255, dBr_u{-28.047}),  // Outer band left (stop)
        std::make_pair(256, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(505, dBr_u{-20.032}),  // Middle band left (stop)
        std::make_pair(506, dBr_u{-20.0}),    // Flat junction band left (start)
        std::make_pair(510, dBr_u{-20.0}),    // Flat junction band left (stop)
        std::make_pair(511, dBr_u{-20.0}),    // Inner band left (start)
        std::make_pair(523, dBr_u{-1.538}),   // Inner band left (stop)
        std::make_pair(524, dBr_u{0.0}),      // allocated band left (start)
        std::make_pair(765, dBr_u{0.0}),      // allocated band left (stop)
        std::make_pair(766, dBr_u{-20.0}),    // DC band (start)
        std::make_pair(770, dBr_u{-20.0}),    // DC band (stop)
        std::make_pair(771, dBr_u{0.0}),      // allocated band right (start)
        std::make_pair(1012, dBr_u{0.0}),     // allocated band right (stop)
        std::make_pair(1013, dBr_u{-1.538}),  // Inner band right (start)
        std::make_pair(1025, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(1026, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(1030, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(1031, dBr_u{-20.032}), // Middle band right (start)
        std::make_pair(1280, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(1281, dBr_u{-28.047}), // Outer band right (start)
        std::make_pair(1536, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 40MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{40},
                                               {MHz_u{5190}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(511, dBr_u{-28.023}),  // Outer band left (stop)
        std::make_pair(512, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(1017, dBr_u{-20.016}), // Middle band left (stop)
        std::make_pair(1018, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(1022, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(1023, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(1035, dBr_u{-1.538}),  // Inner band left (stop)
        std::make_pair(1036, dBr_u{0.0}),     // allocated band left (start)
        std::make_pair(1533, dBr_u{0.0}),     // allocated band left (stop)
        std::make_pair(1534, dBr_u{-20.0}),   // DC band (start)
        std::make_pair(1538, dBr_u{-20.0}),   // DC band (stop)
        std::make_pair(1539, dBr_u{0.0}),     // allocated band right (start)
        std::make_pair(2036, dBr_u{0.0}),     // allocated band right (stop)
        std::make_pair(2037, dBr_u{-1.538}),  // Inner band right (start)
        std::make_pair(2049, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(2050, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(2054, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(2055, dBr_u{-20.016}), // Middle band right (start)
        std::make_pair(2560, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(2561, dBr_u{-28.023}), // Outer band right (start)
        std::make_pair(3072, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{80},
                                               {MHz_u{5210}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 160MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 160MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(1023, dBr_u{-28.012}), // Outer band left (stop)
        std::make_pair(1024, dBr_u{-28.000}), // Middle band left (start)
        std::make_pair(2041, dBr_u{-20.008}), // Middle band left (stop)
        std::make_pair(2042, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(2046, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(2047, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(2059, dBr_u{-1.538}),  // Inner band left (stop)
        std::make_pair(2060, dBr_u{0.0}),     // first 80 MHz allocated band left (start)
        std::make_pair(2557, dBr_u{0.0}),     // first 80 MHz allocated band left (stop)
        std::make_pair(2558, dBr_u{-20.0}),   // first 80 MHz DC band (start)
        std::make_pair(2562, dBr_u{-20.0}),   // first 80 MHz DC band (stop)
        std::make_pair(2563, dBr_u{0.0}),     // first 80 MHz allocated band right (start)
        std::make_pair(3060, dBr_u{0.0}),     // first 80 MHz allocated band right (stop)
        std::make_pair(3061, dBr_u{-20.0}),   // gap between 80 MHz bands (start)
        std::make_pair(3083, dBr_u{-20.0}),   // gap between 80 MHz bands (start)
        std::make_pair(3084, dBr_u{0.0}),     // second 80 MHz allocated band left (start)
        std::make_pair(3581, dBr_u{0.0}),     // second 80 MHz allocated band left (stop)
        std::make_pair(3582, dBr_u{-20.0}),   // second 80 MHz DC band (start)
        std::make_pair(3586, dBr_u{-20.0}),   // second 80 MHz DC band (stop)
        std::make_pair(3587, dBr_u{0.0}),     // second 80 MHz allocated band right (start)
        std::make_pair(4084, dBr_u{0.0}),     // second 80 MHz allocated band right (stop)
        std::make_pair(4085, dBr_u{-1.538}),  // Inner band right (start)
        std::make_pair(4097, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(4098, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(4102, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(4103, dBr_u{-20.008}), // Middle band right (start)
        std::make_pair(5120, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(5121, dBr_u{-28.012}), // Outer band right (start)
        std::make_pair(6144, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 160MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{160},
                                               {MHz_u{5250}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80+80MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 80+80MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(511, dBr_u{-28.023}),  // Outer band left (stop)
        std::make_pair(512, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(1017, dBr_u{-20.016}), // Middle band left (stop)
        std::make_pair(1018, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(1022, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(1023, dBr_u{-20.0}),   // Inner band left for first segment (start)
        std::make_pair(1035, dBr_u{-1.538}),  // Inner band left for first segment (stop)
        std::make_pair(1036, dBr_u{0.0}),     // allocated band left for first segment (start)
        std::make_pair(1533, dBr_u{0.0}),     // allocated band left for first segment (stop)
        std::make_pair(1534, dBr_u{-20.0}),   // DC band for first segment (start)
        std::make_pair(1538, dBr_u{-20.0}),   // DC band for first segment (stop)
        std::make_pair(1539, dBr_u{0.0}),     // allocated band right for first segment (start)
        std::make_pair(2036, dBr_u{0.0}),     // allocated band right for first segment (stop)
        std::make_pair(2037, dBr_u{-1.538}),  // Inner band right for first segment (start)
        std::make_pair(2049, dBr_u{-20.0}),   // Inner band right for first segment (stop)
        std::make_pair(2050, dBr_u{-20.0}),   // Flat junction band right for first segment (start)
        std::make_pair(2054, dBr_u{-20.0}),   // Flat junction band right for first segment (stop)
        std::make_pair(2055, dBr_u{-20.01}),  // start linear sum region left (no interpolation
                                              // possible, so provide 2 times the same point)
        std::make_pair(2055, dBr_u{-20.01}),  // start linear sum region left (no interpolation
                                              // possible, so provide 2 times the same point)
        std::make_pair(2559, dBr_u{-24.99}),  // stop linear sum region left (no interpolation
                                              // possible, so provide 2 times the same point)
        std::make_pair(2559, dBr_u{-24.99}),  // stop linear sum region left (no interpolation
                                              // possible, so provide 2 times the same point)
        std::make_pair(2560, dBr_u{-25.0}),  // middle linear sum region (no interpolation possible,
                                             // so provide 2 times the same point)
        std::make_pair(2560, dBr_u{-25.0}),  // middle linear sum region (no interpolation possible,
                                             // so provide 2 times the same point)
        std::make_pair(2561, dBr_u{-24.99}), // start linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(2561, dBr_u{-24.99}), // start linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(3065, dBr_u{-20.01}), // stop linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(3065, dBr_u{-20.01}), // stop linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(3066, dBr_u{-20.0}),  // Flat junction band left (start)
        std::make_pair(3070, dBr_u{-20.0}),  // Flat junction band left (stop)
        std::make_pair(3071, dBr_u{-20.0}),  // Inner band left for second segment (start)
        std::make_pair(3083, dBr_u{-1.538}), // Inner band left for second segment (stop)
        std::make_pair(3084, dBr_u{0.0}),    // allocated band left for second segment (start)
        std::make_pair(3581, dBr_u{0.0}),    // allocated band left for second segment (stop)
        std::make_pair(3582, dBr_u{-20.0}),  // DC band for second segment (start)
        std::make_pair(3586, dBr_u{-20.0}),  // DC band for second segment (stop)
        std::make_pair(3587, dBr_u{0.0}),    // allocated band right for second segment (start)
        std::make_pair(4084, dBr_u{0.0}),    // allocated band right for second segment (stop)
        std::make_pair(4085, dBr_u{-1.538}), // Inner band right for second segment (start)
        std::make_pair(4097, dBr_u{-20.0}),  // Inner band right for second segment (stop)
        std::make_pair(4098, dBr_u{-20.0}),  // Flat junction band right (start)
        std::make_pair(4102, dBr_u{-20.0}),  // Flat junction band right (stop)
        std::make_pair(4103, dBr_u{-20.016}), // Middle band right (start)
        std::make_pair(4608, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(4609, dBr_u{-28.023}), // Outer band right (start)
        std::make_pair(5120, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80+80MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{160},
                                               {MHz_u{5530}, MHz_u{5690}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80+80MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 80+80MHz @ 5GHz with larger frequency separation "
                    "between the two PSDs");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(511, dBr_u{-28.023}),  // Outer band left (stop)
        std::make_pair(512, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(1017, dBr_u{-20.016}), // Middle band left (stop)
        std::make_pair(1018, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(1022, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(1023, dBr_u{-20.0}),   // Inner band left for first segment (start)
        std::make_pair(1035, dBr_u{-1.538}),  // Inner band left for first segment (stop)
        std::make_pair(1036, dBr_u{0.0}),     // allocated band left for first segment (start)
        std::make_pair(1533, dBr_u{0.0}),     // allocated band left for first segment (stop)
        std::make_pair(1534, dBr_u{-20.0}),   // DC band for first segment (start)
        std::make_pair(1538, dBr_u{-20.0}),   // DC band for first segment (stop)
        std::make_pair(1539, dBr_u{0.0}),     // allocated band right for first segment (start)
        std::make_pair(2036, dBr_u{0.0}),     // allocated band right for first segment (stop)
        std::make_pair(2037, dBr_u{-1.538}),  // Inner band right for first segment (start)
        std::make_pair(2049, dBr_u{-20.0}),   // Inner band right for first segment (stop)
        std::make_pair(2050, dBr_u{-20.0}),   // Flat junction band right for first segment (start)
        std::make_pair(2054, dBr_u{-20.0}),   // Flat junction band right for first segment (stop)
        std::make_pair(2055, dBr_u{-20.01}),  // start linear sum region left (no interpolation
                                              // possible, so provide 2 times the same point)
        std::make_pair(2055, dBr_u{-20.01}),  // start linear sum region left (no interpolation
                                              // possible, so provide 2 times the same point)
        std::make_pair(3583, dBr_u{-24.99}),  // stop linear sum region left (no interpolation
                                              // possible, so provide 2 times the same point)
        std::make_pair(3583, dBr_u{-24.99}),  // stop linear sum region left (no interpolation
                                              // possible, so provide 2 times the same point)
        std::make_pair(3584, dBr_u{-25.0}),  // middle linear sum region (no interpolation possible,
                                             // so provide 2 times the same point)
        std::make_pair(3584, dBr_u{-25.0}),  // middle linear sum region (no interpolation possible,
                                             // so provide 2 times the same point)
        std::make_pair(3585, dBr_u{-24.99}), // start linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(3585, dBr_u{-24.99}), // start linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(5113, dBr_u{-20.01}), // stop linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(5113, dBr_u{-20.01}), // stop linear sum region right (no interpolation
                                             // possible, so provide 2 times the same point)
        std::make_pair(5114, dBr_u{-20.0}),  // Flat junction band left (start)
        std::make_pair(5118, dBr_u{-20.0}),  // Flat junction band left (stop)
        std::make_pair(5119, dBr_u{-20.0}),  // Inner band left for second segment (start)
        std::make_pair(5131, dBr_u{-1.538}), // Inner band left for second segment (stop)
        std::make_pair(5132, dBr_u{0.0}),    // allocated band left for second segment (start)
        std::make_pair(5629, dBr_u{0.0}),    // allocated band left for second segment (stop)
        std::make_pair(5630, dBr_u{-20.0}),  // DC band for second segment (start)
        std::make_pair(5634, dBr_u{-20.0}),  // DC band for second segment (stop)
        std::make_pair(5635, dBr_u{0.0}),    // allocated band right for second segment (start)
        std::make_pair(6132, dBr_u{0.0}),    // allocated band right for second segment (stop)
        std::make_pair(6133, dBr_u{-1.538}), // Inner band right for second segment (start)
        std::make_pair(6145, dBr_u{-20.0}),  // Inner band right for second segment (stop)
        std::make_pair(6146, dBr_u{-20.0}),  // Flat junction band right (start)
        std::make_pair(6150, dBr_u{-20.0}),  // Flat junction band right (stop)
        std::make_pair(6151, dBr_u{-20.016}), // Middle band right (start)
        std::make_pair(6656, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(6657, dBr_u{-28.023}), // Outer band right (start)
        std::make_pair(7168, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80+80MHz large separation",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{160},
                                               {MHz_u{5210}, MHz_u{5530}},
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 5GHz - first 20 MHz subchannel punctured
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 5GHz with first 20 MHz subchannel punctured");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(511, dBr_u{-28.023}),  // Outer band left (stop)
        std::make_pair(512, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(1017, dBr_u{-20.016}), // Middle band left (stop)
        std::make_pair(1018, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(1022, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(1023, dBr_u{-20.0}),   // punctured band (start)
        std::make_pair(1272, dBr_u{-20.0}),   // punctured band (stop)
        std::make_pair(1273, dBr_u{-20.0}),   // punctured band increasing slope (start)
        std::make_pair(1279, dBr_u{0.0}),     // punctured band increasing slope (stop)
        std::make_pair(1280, dBr_u{0.0}),     // allocated band left (start)
        std::make_pair(1533, dBr_u{0.0}),     // allocated band left (stop)
        std::make_pair(1534, dBr_u{-20.0}),   // DC band (start)
        std::make_pair(1538, dBr_u{-20.0}),   // DC band (stop)
        std::make_pair(1539, dBr_u{0.0}),     // allocated band right (start)
        std::make_pair(2036, dBr_u{0.0}),     // allocated band right (stop)
        std::make_pair(2037, dBr_u{-1.538}),  // Inner band right (start)
        std::make_pair(2049, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(2050, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(2054, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(2055, dBr_u{-20.016}), // Middle band right (start)
        std::make_pair(2560, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(2561, dBr_u{-28.023}), // Outer band right (start)
        std::make_pair(3072, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80MHz first subchannel punctured",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{80},
                                               {MHz_u{5210}},
                                               maskSlopes,
                                               tol,
                                               prec,
                                               {true, false, false, false}),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 5GHz - second 20 MHz subchannel punctured
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 5GHz with second 20 MHz subchannel punctured");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(511, dBr_u{-28.023}),  // Outer band left (stop)
        std::make_pair(512, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(1017, dBr_u{-20.016}), // Middle band left (stop)
        std::make_pair(1018, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(1022, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(1023, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(1035, dBr_u{-1.538}),  // Inner band left (stop)
        std::make_pair(1036, dBr_u{0.0}),     // allocated band left (start)
        std::make_pair(1279, dBr_u{0.0}),     // allocated band left (stop)
        std::make_pair(1280, dBr_u{0.0}),     // punctured band decreasing slope (start)
        std::make_pair(1286, dBr_u{-20.0}),   // punctured band decreasing slope (stop)
        std::make_pair(1287, dBr_u{-20.0}),   // punctured band (start)
        std::make_pair(1528, dBr_u{-20.0}),   // punctured band (stop)
        std::make_pair(1529, dBr_u{-20.0}),   // punctured band increasing slope (start)
        std::make_pair(1533, dBr_u{-6.667}),  // punctured band increasing slope (stop)
        std::make_pair(1534, dBr_u{-20.0}),   // DC band (start)
        std::make_pair(1538, dBr_u{-20.0}),   // DC band (stop)
        std::make_pair(1539, dBr_u{0.0}),     // allocated band right (start)
        std::make_pair(2036, dBr_u{0.0}),     // allocated band right (stop)
        std::make_pair(2037, dBr_u{-1.538}),  // Inner band right (start)
        std::make_pair(2049, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(2050, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(2054, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(2055, dBr_u{-20.016}), // Middle band right (start)
        std::make_pair(2560, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(2561, dBr_u{-28.023}), // Outer band right (start)
        std::make_pair(3072, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80MHz second subchannel punctured",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{80},
                                               {MHz_u{5210}},
                                               maskSlopes,
                                               tol,
                                               prec,
                                               {false, true, false, false}),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 5GHz - third 20 MHz subchannel punctured
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 5GHz with third 20 MHz subchannel punctured");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(511, dBr_u{-28.023}),  // Outer band left (stop)
        std::make_pair(512, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(1017, dBr_u{-20.016}), // Middle band left (stop)
        std::make_pair(1018, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(1022, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(1023, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(1035, dBr_u{-1.538}),  // Inner band left (stop)
        std::make_pair(1036, dBr_u{0.0}),     // allocated band left (start)
        std::make_pair(1533, dBr_u{0.0}),     // allocated band left (stop)
        std::make_pair(1534, dBr_u{-20.0}),   // DC band (start)
        std::make_pair(1535, dBr_u{-20.0}),   // DC band (stop)
        std::make_pair(1539, dBr_u{-10.0}),   // punctured band decreasing slope (start)
        std::make_pair(1542, dBr_u{-20.0}),   // punctured band decreasing slope (stop)
        std::make_pair(1543, dBr_u{-20.0}),   // punctured band (start)
        std::make_pair(1784, dBr_u{-20.0}),   // punctured band (stop)
        std::make_pair(1785, dBr_u{-20.0}),   // punctured band increasing slope (start)
        std::make_pair(1791, dBr_u{0.0}),     // punctured band increasing slope (stop)
        std::make_pair(1792, dBr_u{0.0}),     // allocated band right (start)
        std::make_pair(2036, dBr_u{0.0}),     // allocated band right (stop)
        std::make_pair(2037, dBr_u{-1.538}),  // Inner band right (start)
        std::make_pair(2049, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(2050, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(2054, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(2055, dBr_u{-20.016}), // Middle band right (start)
        std::make_pair(2560, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(2561, dBr_u{-28.023}), // Outer band right (start)
        std::make_pair(3072, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80MHz third subchannel punctured",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{80},
                                               {MHz_u{5210}},
                                               maskSlopes,
                                               tol,
                                               prec,
                                               {false, false, true, false}),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 5GHz - last 20 MHz subchannel punctured
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 5GHz with last 20 MHz subchannel punctured");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(511, dBr_u{-28.023}),  // Outer band left (stop)
        std::make_pair(512, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(1017, dBr_u{-20.016}), // Middle band left (stop)
        std::make_pair(1018, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(1022, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(1023, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(1035, dBr_u{-1.538}),  // Inner band left (stop)
        std::make_pair(1036, dBr_u{0.0}),     // allocated band left (start)
        std::make_pair(1533, dBr_u{0.0}),     // allocated band left (stop)
        std::make_pair(1534, dBr_u{-20.0}),   // DC band (start)
        std::make_pair(1538, dBr_u{-20.0}),   // DC band (stop)
        std::make_pair(1539, dBr_u{0.0}),     // allocated band right (start)
        std::make_pair(1791, dBr_u{0.0}),     // allocated band right (stop)
        std::make_pair(1792, dBr_u{0.0}),     // punctured band decreasing slope (start)
        std::make_pair(1798, dBr_u{-20.0}),   // punctured band decreasing slope (stop)
        std::make_pair(1799, dBr_u{-20.0}),   // punctured band (start)
        std::make_pair(2049, dBr_u{-20.0}),   // punctured band (stop)
        std::make_pair(2050, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(2054, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(2055, dBr_u{-20.016}), // Middle band right (start)
        std::make_pair(2560, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(2561, dBr_u{-28.023}), // Outer band right (start)
        std::make_pair(3072, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80MHz last subchannel punctured",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               MHz_u{80},
                                               {MHz_u{5210}},
                                               maskSlopes,
                                               tol,
                                               prec,
                                               {false, false, false, true}),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 160MHz @ 5GHz - first two 20 MHz subchannels punctured
    NS_LOG_FUNCTION(
        "Check slopes for 11ax 160MHz @ 5GHz with two first 20 MHz subchannels punctured");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),       // Outer band left (start)
        std::make_pair(1023, dBr_u{-28.012}),  // Outer band left (stop)
        std::make_pair(1024, dBr_u{-28.000}),  // Middle band left (start)
        std::make_pair(2041, dBr_u{-20.008}),  // Middle band left (stop)
        std::make_pair(2042, dBr_u{-20.0}),    // Flat junction band left (start)
        std::make_pair(2046, dBr_u{-20.0}),    // Flat junction band left (stop)
        std::make_pair(2047, dBr_u{-20.0}),    // punctured band (start)
        std::make_pair(2552, dBr_u{-20.0}),    // punctured band (stop)
        std::make_pair(2553, dBr_u{-20.0}),    // punctured band increasing slope (start)
        std::make_pair(2557, dBr_u{-6.66667}), // punctured band increasing slope (stop)
        std::make_pair(2558, dBr_u{-20.0}),    // first 80 MHz DC band (start)
        std::make_pair(2562, dBr_u{-20.0}),    // first 80 MHz DC band (stop)
        std::make_pair(2563, dBr_u{0.0}),      // first 80 MHz allocated band right (start)
        std::make_pair(3060, dBr_u{0.0}),      // first 80 MHz allocated band right (stop)
        std::make_pair(3061, dBr_u{-20.0}),    // gap between 80 MHz bands (start)
        std::make_pair(3083, dBr_u{-20.0}),    // gap between 80 MHz bands (start)
        std::make_pair(3084, dBr_u{0.0}),      // second 80 MHz allocated band left (start)
        std::make_pair(3581, dBr_u{0.0}),      // second 80 MHz allocated band left (stop)
        std::make_pair(3582, dBr_u{-20.0}),    // second 80 MHz DC band (start)
        std::make_pair(3586, dBr_u{-20.0}),    // second 80 MHz DC band (stop)
        std::make_pair(3587, dBr_u{0.0}),      // second 80 MHz allocated band right (start)
        std::make_pair(4084, dBr_u{0.0}),      // second 80 MHz allocated band right (stop)
        std::make_pair(4085, dBr_u{-1.538}),   // Inner band right (start)
        std::make_pair(4097, dBr_u{-20.0}),    // Inner band right (stop)
        std::make_pair(4098, dBr_u{-20.0}),    // Flat junction band right (start)
        std::make_pair(4102, dBr_u{-20.0}),    // Flat junction band right (stop)
        std::make_pair(4103, dBr_u{-20.008}),  // Middle band right (start)
        std::make_pair(5120, dBr_u{-28.000}),  // Middle band right (stop)
        std::make_pair(5121, dBr_u{-28.012}),  // Outer band right (start)
        std::make_pair(6144, dBr_u{-40.0}),    // Outer band right (stop)
    };

    AddTestCase(
        new WifiOfdmMaskSlopesTestCase("11ax_5GHz 160MHz first subchannels punctured",
                                       WIFI_STANDARD_80211ax,
                                       WIFI_PHY_BAND_5GHZ,
                                       MHz_u{160},
                                       {MHz_u{5250}},
                                       maskSlopes,
                                       tol,
                                       prec,
                                       {true, true, false, false, false, false, false, false}),
        TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 160MHz @ 5GHz - third and fourth 20 MHz subchannels punctured
    NS_LOG_FUNCTION(
        "Check slopes for 11ax 160MHz @ 5GHz with third and fourth 20 MHz subchannels punctured");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(1023, dBr_u{-28.012}), // Outer band left (stop)
        std::make_pair(1024, dBr_u{-28.000}), // Middle band left (start)
        std::make_pair(2041, dBr_u{-20.008}), // Middle band left (stop)
        std::make_pair(2042, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(2046, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(2047, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(2059, dBr_u{-1.538}),  // Inner band left (stop)
        std::make_pair(2060, dBr_u{0.0}),     // first 80 MHz allocated band left (start)
        std::make_pair(2557, dBr_u{0.0}),     // first 80 MHz allocated band left (stop)
        std::make_pair(2558, dBr_u{-20.0}),   // first 80 MHz DC band (start)
        std::make_pair(2562, dBr_u{-20.0}),   // first 80 MHz DC band (stop)
        std::make_pair(2563, dBr_u{-10.0}),   // punctured band decreasing slope (start)
        std::make_pair(2566, dBr_u{-20.0}),   // punctured band decreasing slope (stop)
        std::make_pair(2567, dBr_u{-20.0}),   // punctured band (start)
        std::make_pair(3060, dBr_u{-20.0}),   // punctured band (stop)
        std::make_pair(3061, dBr_u{-20.0}),   // gap between 80 MHz bands (start)
        std::make_pair(3083, dBr_u{-20.0}),   // gap between 80 MHz bands (start)
        std::make_pair(3084, dBr_u{0.0}),     // second 80 MHz allocated band left (start)
        std::make_pair(3581, dBr_u{0.0}),     // second 80 MHz allocated band left (stop)
        std::make_pair(3582, dBr_u{-20.0}),   // second 80 MHz DC band (start)
        std::make_pair(3586, dBr_u{-20.0}),   // second 80 MHz DC band (stop)
        std::make_pair(3587, dBr_u{0.0}),     // second 80 MHz allocated band right (start)
        std::make_pair(4084, dBr_u{0.0}),     // second 80 MHz allocated band right (stop)
        std::make_pair(4085, dBr_u{-1.538}),  // Inner band right (start)
        std::make_pair(4097, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(4098, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(4102, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(4103, dBr_u{-20.008}), // Middle band right (start)
        std::make_pair(5120, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(5121, dBr_u{-28.012}), // Outer band right (start)
        std::make_pair(6144, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(
        new WifiOfdmMaskSlopesTestCase("11ax_5GHz 160MHz third and fourth subchannels punctured",
                                       WIFI_STANDARD_80211ax,
                                       WIFI_PHY_BAND_5GHZ,
                                       MHz_u{160},
                                       {MHz_u{5250}},
                                       maskSlopes,
                                       tol,
                                       prec,
                                       {false, false, true, true, false, false, false, false}),
        TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 160MHz @ 5GHz - fifth and sixth 20 MHz subchannels punctured
    NS_LOG_FUNCTION(
        "Check slopes for 11ax 160MHz @ 5GHz with fifth and sixth 20 MHz subchannels punctured");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(1023, dBr_u{-28.012}), // Outer band left (stop)
        std::make_pair(1024, dBr_u{-28.000}), // Middle band left (start)
        std::make_pair(2041, dBr_u{-20.008}), // Middle band left (stop)
        std::make_pair(2042, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(2046, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(2047, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(2059, dBr_u{-1.538}),  // Inner band left (stop)
        std::make_pair(2060, dBr_u{0.0}),     // first 80 MHz allocated band left (start)
        std::make_pair(2557, dBr_u{0.0}),     // first 80 MHz allocated band left (stop)
        std::make_pair(2558, dBr_u{-20.0}),   // first 80 MHz DC band (start)
        std::make_pair(2562, dBr_u{-20.0}),   // first 80 MHz DC band (stop)
        std::make_pair(2563, dBr_u{0.0}),     // first 80 MHz allocated band right (start)
        std::make_pair(3060, dBr_u{0.0}),     // first 80 MHz allocated band right (stop)
        std::make_pair(3061, dBr_u{-20.0}),   // gap between 80 MHz bands (start)
        std::make_pair(3083, dBr_u{-20.0}),   // gap between 80 MHz bands (start)
        std::make_pair(3084, dBr_u{-20.0}),   // punctured band (start)
        std::make_pair(3576, dBr_u{-20.0}),   // punctured band (stop)
        std::make_pair(3577, dBr_u{-20.0}),   // punctured band increasing slope (start)
        std::make_pair(3581, dBr_u{-6.667}),  // punctured band increasing slope (stop)
        std::make_pair(3582, dBr_u{-20.0}),   // second 80 MHz DC band (start)
        std::make_pair(3586, dBr_u{-20.0}),   // second 80 MHz DC band (stop)
        std::make_pair(3587, dBr_u{0.0}),     // second 80 MHz allocated band right (start)
        std::make_pair(4084, dBr_u{0.0}),     // second 80 MHz allocated band right (stop)
        std::make_pair(4085, dBr_u{-1.538}),  // Inner band right (start)
        std::make_pair(4097, dBr_u{-20.0}),   // Inner band right (stop)
        std::make_pair(4098, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(4102, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(4103, dBr_u{-20.008}), // Middle band right (start)
        std::make_pair(5120, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(5121, dBr_u{-28.012}), // Outer band right (start)
        std::make_pair(6144, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(
        new WifiOfdmMaskSlopesTestCase("11ax_5GHz 160MHz fifth and sixth subchannels punctured",
                                       WIFI_STANDARD_80211ax,
                                       WIFI_PHY_BAND_5GHZ,
                                       MHz_u{160},
                                       {MHz_u{5250}},
                                       maskSlopes,
                                       tol,
                                       prec,
                                       {false, false, false, false, true, true, false, false}),
        TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 160MHz @ 5GHz - last two 20 MHz subchannels punctured
    NS_LOG_FUNCTION(
        "Check slopes for 11ax 160MHz @ 5GHz with two last 20 MHz subchannels punctured");
    maskSlopes = {
        std::make_pair(0, dBr_u{-40.0}),      // Outer band left (start)
        std::make_pair(1023, dBr_u{-28.012}), // Outer band left (stop)
        std::make_pair(1024, dBr_u{-28.000}), // Middle band left (start)
        std::make_pair(2041, dBr_u{-20.008}), // Middle band left (stop)
        std::make_pair(2042, dBr_u{-20.0}),   // Flat junction band left (start)
        std::make_pair(2046, dBr_u{-20.0}),   // Flat junction band left (stop)
        std::make_pair(2047, dBr_u{-20.0}),   // Inner band left (start)
        std::make_pair(2059, dBr_u{-1.538}),  // Inner band left (stop)
        std::make_pair(2060, dBr_u{0.0}),     // first 80 MHz allocated band left (start)
        std::make_pair(2557, dBr_u{0.0}),     // first 80 MHz allocated band left (stop)
        std::make_pair(2558, dBr_u{-20.0}),   // first 80 MHz DC band (start)
        std::make_pair(2562, dBr_u{-20.0}),   // first 80 MHz DC band (stop)
        std::make_pair(2563, dBr_u{0.0}),     // first 80 MHz allocated band right (start)
        std::make_pair(3060, dBr_u{0.0}),     // first 80 MHz allocated band right (stop)
        std::make_pair(3061, dBr_u{-20.0}),   // gap between 80 MHz bands (start)
        std::make_pair(3083, dBr_u{-20.0}),   // gap between 80 MHz bands (start)
        std::make_pair(3084, dBr_u{0.0}),     // second 80 MHz allocated band left (start)
        std::make_pair(3581, dBr_u{0.0}),     // second 80 MHz allocated band left (stop)
        std::make_pair(3582, dBr_u{-20.0}),   // second 80 MHz DC band (start)
        std::make_pair(3586, dBr_u{-20.0}),   // second 80 MHz DC band (stop)
        std::make_pair(3587, dBr_u{-10.0}),   // punctured band decreasing slope (start)
        std::make_pair(3590, dBr_u{-20.0}),   // punctured band decreasing slope (stop)
        std::make_pair(3591, dBr_u{-20.0}),   // punctured band (start)
        std::make_pair(4097, dBr_u{-20.0}),   // punctured band (stop)
        std::make_pair(4098, dBr_u{-20.0}),   // Flat junction band right (start)
        std::make_pair(4102, dBr_u{-20.0}),   // Flat junction band right (stop)
        std::make_pair(4103, dBr_u{-20.008}), // Middle band right (start)
        std::make_pair(5120, dBr_u{-28.000}), // Middle band right (stop)
        std::make_pair(5121, dBr_u{-28.012}), // Outer band right (start)
        std::make_pair(6144, dBr_u{-40.0}),   // Outer band right (stop)
    };

    AddTestCase(
        new WifiOfdmMaskSlopesTestCase("11ax_5GHz 160MHz last subchannels punctured",
                                       WIFI_STANDARD_80211ax,
                                       WIFI_PHY_BAND_5GHZ,
                                       MHz_u{160},
                                       {MHz_u{5250}},
                                       maskSlopes,
                                       tol,
                                       prec,
                                       {false, false, false, false, false, false, true, true}),
        TestCase::Duration::QUICK);
}
