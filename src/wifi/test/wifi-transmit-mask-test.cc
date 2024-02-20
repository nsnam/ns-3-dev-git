/*
 * Copyright (c) 2017 Orange Labs
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
 * Author: Rediet <getachew.redieteab@orange.com>
 */

#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/wifi-phy-band.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-standards.h"

#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiTransmitMaskTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test checks if Wifi spectrum values for OFDM are generated properly.
 * Different test cases are configured by defining different standards and bandwidth.
 */
class WifiOfdmMaskSlopesTestCase : public TestCase
{
  public:
    /**
     * typedef for a pair of sub-band index and relative power value (dBr)
     */
    typedef std::pair<uint32_t, double> IndexPowerPair;

    /**
     * typedef for a vector of pairs of sub-band index and relative power value (dBr)
     */
    typedef std::vector<IndexPowerPair> IndexPowerVect;

    /**
     * Constructor
     *
     * \param name test reference name
     * \param standard selected standard
     * \param band selected PHY band
     * \param channelWidth channel width (in MHz)
     * \param maskRefs vector of expected power values and corresponding indexes of generated PSD
     *                     (only start and stop indexes/values given)
     * \param tolerance tolerance (in dB)
     * \param precision precision (in decimals)
     * \param puncturedSubchannels bitmap indicating whether a 20 MHz subchannel is punctured or not
     * (only for 802.11ax and later)
     */
    WifiOfdmMaskSlopesTestCase(const std::string& name,
                               WifiStandard standard,
                               WifiPhyBand band,
                               uint16_t channelWidth,
                               const IndexPowerVect& maskRefs,
                               double tolerance,
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
     * \param vect vector of sub-band index and relative power value pairs to which interpolated
     values should be appended
     * \param start pair of sub-band index and relative power value (dBr) for interval start
     * \param stop pair of sub-band index and relative power value (dBr) for interval stop
    */
    void InterpolateAndAppendValues(IndexPowerVect& vect,
                                    IndexPowerPair start,
                                    IndexPowerPair stop) const;

    WifiStandard m_standard; ///< the wifi standard to use for the test
    WifiPhyBand m_band;      ///< the wifi PHY band to use for the test
    uint16_t m_channelWidth; ///< the channel width in MHz to use for the test
    std::vector<bool>
        m_puncturedSubchannels; ///< bitmap indicating whether a 20 MHz subchannel is punctured or
                                ///< not (only used for 802.11ax and later)
    Ptr<SpectrumValue> m_actualSpectrum; ///< actual spectrum value
    IndexPowerVect m_expectedPsd;        ///< expected power values
    double m_tolerance;                  ///< tolerance (in dB)
    std::size_t m_precision;             ///< precision for double calculations (in decimals)
};

WifiOfdmMaskSlopesTestCase::WifiOfdmMaskSlopesTestCase(
    const std::string& name,
    WifiStandard standard,
    WifiPhyBand band,
    uint16_t channelWidth,
    const IndexPowerVect& maskRefs,
    double tolerance,
    std::size_t precision,
    const std::vector<bool>& puncturedSubchannels)
    : TestCase(std::string("SpectrumValue ") + name),
      m_standard{standard},
      m_band{band},
      m_channelWidth{channelWidth},
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
    NS_ASSERT(m_expectedPsd.size() % 2 == 0); // start/stop pairs expected

    uint16_t freq = 0;
    double outerBandMaximumRejection = 0.0;
    switch (m_band)
    {
    default:
    case WIFI_PHY_BAND_5GHZ:
        freq = 5170 + (m_channelWidth / 2); // so as to have 5180/5190/5210/5250 for 20/40/80/160
        outerBandMaximumRejection = -40;    // in dBr
        break;
    case WIFI_PHY_BAND_2_4GHZ:
        freq = 2402 + (m_channelWidth / 2); // so as to have 2412/2422 for 20/40
        outerBandMaximumRejection = (m_standard >= WIFI_STANDARD_80211n) ? -45 : -40; // in dBr
        break;
    case WIFI_PHY_BAND_6GHZ:
        freq = 5945 + (m_channelWidth / 2); // so as to have 5945/5955/5975/6015 for 20/40/80/160
        outerBandMaximumRejection = -40;    // in dBr
        break;
    }

    double refTxPowerW = 1; // have to work in dBr when comparing though
    switch (m_standard)
    {
    case WIFI_STANDARD_80211p:
        NS_ASSERT(m_band == WIFI_PHY_BAND_5GHZ);
        NS_ASSERT((m_channelWidth == 5) || (m_channelWidth == 10));
        freq = 5860;
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(freq,
                                                                      m_channelWidth,
                                                                      refTxPowerW,
                                                                      m_channelWidth,
                                                                      -20.0,
                                                                      -28.0,
                                                                      outerBandMaximumRejection);
        break;

    case WIFI_STANDARD_80211g:
        NS_ASSERT(m_band == WIFI_PHY_BAND_2_4GHZ);
        NS_ASSERT(m_channelWidth == 20);
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(freq,
                                                                      m_channelWidth,
                                                                      refTxPowerW,
                                                                      m_channelWidth,
                                                                      -20.0,
                                                                      -28.0,
                                                                      outerBandMaximumRejection);
        break;

    case WIFI_STANDARD_80211a:
        NS_ASSERT(m_band == WIFI_PHY_BAND_5GHZ);
        NS_ASSERT(m_channelWidth == 20);
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(freq,
                                                                      m_channelWidth,
                                                                      refTxPowerW,
                                                                      m_channelWidth,
                                                                      -20.0,
                                                                      -28.0,
                                                                      outerBandMaximumRejection);
        break;

    case WIFI_STANDARD_80211n:
        NS_ASSERT(m_channelWidth == 20 || m_channelWidth == 40);
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity(freq,
                                                                        m_channelWidth,
                                                                        refTxPowerW,
                                                                        m_channelWidth,
                                                                        -20.0,
                                                                        -28.0,
                                                                        outerBandMaximumRejection);
        break;

    case WIFI_STANDARD_80211ac:
        NS_ASSERT(m_band == WIFI_PHY_BAND_5GHZ);
        NS_ASSERT(m_channelWidth == 20 || m_channelWidth == 40 || m_channelWidth == 80 ||
                  m_channelWidth == 160);
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity(freq,
                                                                        m_channelWidth,
                                                                        refTxPowerW,
                                                                        m_channelWidth,
                                                                        -20.0,
                                                                        -28.0,
                                                                        outerBandMaximumRejection);
        break;

    case WIFI_STANDARD_80211ax:
        NS_ASSERT((m_band != WIFI_PHY_BAND_2_4GHZ) ||
                  (m_channelWidth != 160)); // not enough space in 2.4 GHz bands
        NS_ASSERT(m_channelWidth == 20 || m_channelWidth == 40 || m_channelWidth == 80 ||
                  m_channelWidth == 160);
        m_actualSpectrum =
            WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(freq,
                                                                        m_channelWidth,
                                                                        refTxPowerW,
                                                                        m_channelWidth,
                                                                        -20.0,
                                                                        -28.0,
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

    double slope = (stop.second - start.second) / (stop.first - start.first);
    for (uint32_t i = start.first; i <= stop.first; i++)
    {
        double val = start.second + slope * (i - start.first);
        double multiplier = std::round(std::pow(10.0, static_cast<double>(m_precision)));
        val = std::floor(val * multiplier + 0.5) / multiplier;
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
    double currentPowerDbr = 0.0; // have to work in dBr so as to compare with expected slopes
    double maxPowerW = (*m_actualSpectrum)[0];
    for (auto&& vit = m_actualSpectrum->ConstValuesBegin();
         vit != m_actualSpectrum->ConstValuesEnd();
         ++vit)
    {
        maxPowerW = std::max(maxPowerW, *vit);
    }

    NS_LOG_INFO("Compare expected PSD");
    for (const auto& [subcarrier, expectedValue] : m_expectedPsd)
    {
        currentPowerDbr = 10.0 * std::log10((*m_actualSpectrum)[subcarrier] / maxPowerW);
        NS_LOG_LOGIC("For " << subcarrier << ", expected: " << expectedValue
                            << " vs obtained: " << currentPowerDbr);
        NS_TEST_EXPECT_MSG_EQ_TOL(currentPowerDbr,
                                  expectedValue,
                                  m_tolerance,
                                  "Spectrum value mismatch for subcarrier " << subcarrier);
    }
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test suite for checking the consistency of different OFDM-based transmit masks.
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
    double tol = 10e-2; // in dB
    double prec = 10;   // in decimals

    // ============================================================================================
    // 11p 5MHz
    NS_LOG_FUNCTION("Check slopes for 11p 5MHz");
    maskSlopes = {
        std::make_pair(0, -40.0),     // Outer band left (start)
        std::make_pair(31, -28.375),  // Outer band left (stop)
        std::make_pair(32, -28.000),  // Middle band left (start)
        std::make_pair(60, -20.276),  // Middle band left (stop)
        std::make_pair(61, -20.0),    // Flat junction band left (start)
        std::make_pair(63, -20.0),    // Flat junction band left (stop)
        std::make_pair(64, -20.0),    // Inner band left (start)
        std::make_pair(69, -3.333),   // Inner band left (stop)
        std::make_pair(123, -3.333),  // Inner band right (start)
        std::make_pair(128, -20.0),   // Inner band right (stop)
        std::make_pair(129, -20.0),   // Flat junction band right (start)
        std::make_pair(131, -20.0),   // Flat junction band right (stop)
        std::make_pair(132, -20.276), // Middle band right (start)
        std::make_pair(160, -28.000), // Middle band right (stop)
        std::make_pair(161, -28.375), // Outer band right (start)
        std::make_pair(192, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11p 5MHz",
                                               WIFI_STANDARD_80211p,
                                               WIFI_PHY_BAND_5GHZ,
                                               5,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11p 10MHz
    NS_LOG_FUNCTION("Check slopes for 11p 10MHz");
    maskSlopes = {
        std::make_pair(0, -40.0),     // Outer band left (start)
        std::make_pair(31, -28.375),  // Outer band left (stop)
        std::make_pair(32, -28.000),  // Middle band left (start)
        std::make_pair(60, -20.276),  // Middle band left (stop)
        std::make_pair(61, -20.0),    // Flat junction band left (start)
        std::make_pair(63, -20.0),    // Flat junction band left (stop)
        std::make_pair(64, -20.0),    // Inner band left (start)
        std::make_pair(69, -3.333),   // Inner band left (stop)
        std::make_pair(123, -3.333),  // Inner band right (start)
        std::make_pair(128, -20.0),   // Inner band right (stop)
        std::make_pair(129, -20.0),   // Flat junction band right (start)
        std::make_pair(131, -20.0),   // Flat junction band right (stop)
        std::make_pair(132, -20.276), // Middle band right (start)
        std::make_pair(160, -28.000), // Middle band right (stop)
        std::make_pair(161, -28.375), // Outer band right (start)
        std::make_pair(192, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11p 10MHz",
                                               WIFI_STANDARD_80211p,
                                               WIFI_PHY_BAND_5GHZ,
                                               10,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11a
    NS_LOG_FUNCTION("Check slopes for 11a");
    maskSlopes = {
        std::make_pair(0, -40.0),     // Outer band left (start)
        std::make_pair(31, -28.375),  // Outer band left (stop)
        std::make_pair(32, -28.000),  // Middle band left (start)
        std::make_pair(60, -20.276),  // Middle band left (stop)
        std::make_pair(61, -20.0),    // Flat junction band left (start)
        std::make_pair(63, -20.0),    // Flat junction band left (stop)
        std::make_pair(64, -20.0),    // Inner band left (start)
        std::make_pair(69, -3.333),   // Inner band left (stop)
        std::make_pair(123, -3.333),  // Inner band right (start)
        std::make_pair(128, -20.0),   // Inner band right (stop)
        std::make_pair(129, -20.0),   // Flat junction band right (start)
        std::make_pair(131, -20.0),   // Flat junction band right (stop)
        std::make_pair(132, -20.276), // Middle band right (start)
        std::make_pair(160, -28.000), // Middle band right (stop)
        std::make_pair(161, -28.375), // Outer band right (start)
        std::make_pair(192, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11a",
                                               WIFI_STANDARD_80211a,
                                               WIFI_PHY_BAND_5GHZ,
                                               20,
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
                                               20,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11n 20MHz @ 2.4GHz
    NS_LOG_FUNCTION("Check slopes for 11n 20MHz @ 2.4GHz");
    maskSlopes = {
        std::make_pair(0, -45.000),   // Outer band left (start)
        std::make_pair(31, -28.531),  // Outer band left (stop)
        std::make_pair(32, -28.000),  // Middle band left (start)
        std::make_pair(60, -20.276),  // Middle band left (stop)
        std::make_pair(61, -20.0),    // Flat junction band left (start)
        std::make_pair(61, -20.0),    // Flat junction band left (stop)
        std::make_pair(62, -20.0),    // Inner band left (start)
        std::make_pair(67, -3.333),   // Inner band left (stop)
        std::make_pair(125, -3.333),  // Inner band right (start)
        std::make_pair(130, -20.0),   // Inner band right (stop)
        std::make_pair(131, -20.0),   // Flat junction band right (start)
        std::make_pair(131, -20.0),   // Flat junction band right (stop)
        std::make_pair(132, -20.276), // Middle band right (start)
        std::make_pair(160, -28.000), // Middle band right (stop)
        std::make_pair(161, -28.531), // Outer band right (start)
        std::make_pair(192, -45.000), // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11n_2.4GHz 20MHz",
                                               WIFI_STANDARD_80211n,
                                               WIFI_PHY_BAND_2_4GHZ,
                                               20,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11n 20MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11n 20MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, -40.0),     // Outer band left (start)
        std::make_pair(31, -28.375),  // Outer band left (stop)
        std::make_pair(32, -28.000),  // Middle band left (start)
        std::make_pair(60, -20.276),  // Middle band left (stop)
        std::make_pair(61, -20.0),    // Flat junction band left (start)
        std::make_pair(61, -20.0),    // Flat junction band left (stop)
        std::make_pair(62, -20.0),    // Inner band left (start)
        std::make_pair(67, -3.333),   // Inner band left (stop)
        std::make_pair(125, -3.333),  // Inner band right (start)
        std::make_pair(130, -20.0),   // Inner band right (stop)
        std::make_pair(131, -20.0),   // Flat junction band right (start)
        std::make_pair(131, -20.0),   // Flat junction band right (stop)
        std::make_pair(132, -20.276), // Middle band right (start)
        std::make_pair(160, -28.000), // Middle band right (stop)
        std::make_pair(161, -28.375), // Outer band right (start)
        std::make_pair(192, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11n_5GHz 20MHz",
                                               WIFI_STANDARD_80211n,
                                               WIFI_PHY_BAND_5GHZ,
                                               20,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11n 40MHz @ 2.4GHz
    NS_LOG_FUNCTION("Check slopes for 11n 40MHz @ 2.4GHz");
    maskSlopes = {
        std::make_pair(0, -45.000),   // Outer band left (start)
        std::make_pair(63, -28.266),  // Outer band left (stop)
        std::make_pair(64, -28.000),  // Middle band left (start)
        std::make_pair(124, -20.131), // Middle band left (stop)
        std::make_pair(125, -20.0),   // Flat junction band left (start)
        std::make_pair(125, -20.0),   // Flat junction band left (stop)
        std::make_pair(126, -20.0),   // Inner band left (start)
        std::make_pair(131, -3.333),  // Inner band left (stop)
        std::make_pair(253, -3.333),  // Inner band right (start)
        std::make_pair(258, -20.0),   // Inner band right (stop)
        std::make_pair(259, -20.0),   // Flat junction band right (start)
        std::make_pair(259, -20.0),   // Flat junction band right (stop)
        std::make_pair(260, -20.131), // Middle band right (start)
        std::make_pair(320, -28.000), // Middle band right (stop)
        std::make_pair(321, -28.266), // Outer band right (start)
        std::make_pair(384, -45.000), // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11n_2.4GHz 40MHz",
                                               WIFI_STANDARD_80211n,
                                               WIFI_PHY_BAND_2_4GHZ,
                                               40,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11n 20MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11n 40MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, -40.0),     // Outer band left (start)
        std::make_pair(63, -28.188),  // Outer band left (stop)
        std::make_pair(64, -28.000),  // Middle band left (start)
        std::make_pair(124, -20.131), // Middle band left (stop)
        std::make_pair(125, -20.0),   // Flat junction band left (start)
        std::make_pair(125, -20.0),   // Flat junction band left (stop)
        std::make_pair(126, -20.0),   // Inner band left (start)
        std::make_pair(131, -3.333),  // Inner band left (stop)
        std::make_pair(253, -3.333),  // Inner band right (start)
        std::make_pair(258, -20.0),   // Inner band right (stop)
        std::make_pair(259, -20.0),   // Flat junction band right (start)
        std::make_pair(259, -20.0),   // Flat junction band right (stop)
        std::make_pair(260, -20.131), // Middle band right (start)
        std::make_pair(320, -28.000), // Middle band right (stop)
        std::make_pair(321, -28.188), // Outer band right (start)
        std::make_pair(384, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11n_5GHz 40MHz",
                                               WIFI_STANDARD_80211n,
                                               WIFI_PHY_BAND_5GHZ,
                                               40,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ac 20MHz
    NS_LOG_FUNCTION("Check slopes for 11ac 20MHz");
    maskSlopes = {
        std::make_pair(0, -40.0),     // Outer band left (start)
        std::make_pair(31, -28.375),  // Outer band left (stop)
        std::make_pair(32, -28.000),  // Middle band left (start)
        std::make_pair(60, -20.276),  // Middle band left (stop)
        std::make_pair(61, -20.0),    // Flat junction band left (start)
        std::make_pair(61, -20.0),    // Flat junction band left (stop)
        std::make_pair(62, -20.0),    // Inner band left (start)
        std::make_pair(67, -3.333),   // Inner band left (stop)
        std::make_pair(125, -3.333),  // Inner band right (start)
        std::make_pair(130, -20.0),   // Inner band right (stop)
        std::make_pair(131, -20.0),   // Flat junction band right (start)
        std::make_pair(131, -20.0),   // Flat junction band right (stop)
        std::make_pair(132, -20.276), // Middle band right (start)
        std::make_pair(160, -28.000), // Middle band right (stop)
        std::make_pair(161, -28.375), // Outer band right (start)
        std::make_pair(192, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ac 20MHz",
                                               WIFI_STANDARD_80211ac,
                                               WIFI_PHY_BAND_5GHZ,
                                               20,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ac 20MHz
    NS_LOG_FUNCTION("Check slopes for 11ac 40MHz");
    maskSlopes = {
        std::make_pair(0, -40.0),     // Outer band left (start)
        std::make_pair(63, -28.188),  // Outer band left (stop)
        std::make_pair(64, -28.000),  // Middle band left (start)
        std::make_pair(124, -20.131), // Middle band left (stop)
        std::make_pair(125, -20.0),   // Flat junction band left (start)
        std::make_pair(125, -20.0),   // Flat junction band left (stop)
        std::make_pair(126, -20.0),   // Inner band left (start)
        std::make_pair(131, -3.333),  // Inner band left (stop)
        std::make_pair(253, -3.333),  // Inner band right (start)
        std::make_pair(258, -20.0),   // Inner band right (stop)
        std::make_pair(259, -20.0),   // Flat junction band right (start)
        std::make_pair(259, -20.0),   // Flat junction band right (stop)
        std::make_pair(260, -20.131), // Middle band right (start)
        std::make_pair(320, -28.000), // Middle band right (stop)
        std::make_pair(321, -28.188), // Outer band right (start)
        std::make_pair(384, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ac 40MHz",
                                               WIFI_STANDARD_80211ac,
                                               WIFI_PHY_BAND_5GHZ,
                                               40,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ac 80MHz
    NS_LOG_FUNCTION("Check slopes for 11ac 80MHz");
    maskSlopes = {
        std::make_pair(0, -40.0),     // Outer band left (start)
        std::make_pair(127, -28.094), // Outer band left (stop)
        std::make_pair(128, -28.000), // Middle band left (start)
        std::make_pair(252, -20.064), // Middle band left (stop)
        std::make_pair(253, -20.0),   // Flat junction band left (start)
        std::make_pair(253, -20.0),   // Flat junction band left (stop)
        std::make_pair(254, -20.0),   // Inner band left (start)
        std::make_pair(259, -3.333),  // Inner band left (stop)
        std::make_pair(509, -3.333),  // Inner band right (start)
        std::make_pair(514, -20.0),   // Inner band right (stop)
        std::make_pair(515, -20.0),   // Flat junction band right (start)
        std::make_pair(515, -20.0),   // Flat junction band right (stop)
        std::make_pair(516, -20.064), // Middle band right (start)
        std::make_pair(640, -28.000), // Middle band right (stop)
        std::make_pair(641, -28.094), // Outer band right (start)
        std::make_pair(768, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ac 80MHz",
                                               WIFI_STANDARD_80211ac,
                                               WIFI_PHY_BAND_5GHZ,
                                               80,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ac 20MHz
    NS_LOG_FUNCTION("Check slopes for 11ac 160MHz");
    maskSlopes = {
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(255, -28.047),  // Outer band left (stop)
        std::make_pair(256, -28.000),  // Middle band left (start)
        std::make_pair(508, -20.032),  // Middle band left (stop)
        std::make_pair(509, -20.0),    // Flat junction band left (start)
        std::make_pair(509, -20.0),    // Flat junction band left (stop)
        std::make_pair(510, -20.0),    // Inner band left (start)
        std::make_pair(515, -3.333),   // Inner band left (stop)
        std::make_pair(1021, -3.333),  // Inner band right (start)
        std::make_pair(1026, -20.0),   // Inner band right (stop)
        std::make_pair(1027, -20.0),   // Flat junction band right (start)
        std::make_pair(1027, -20.0),   // Flat junction band right (stop)
        std::make_pair(1028, -20.032), // Middle band right (start)
        std::make_pair(1280, -28.000), // Middle band right (stop)
        std::make_pair(1281, -28.047), // Outer band right (start)
        std::make_pair(1536, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ac 160MHz",
                                               WIFI_STANDARD_80211ac,
                                               WIFI_PHY_BAND_5GHZ,
                                               160,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 20MHz @ 2.4GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 20MHz @ 2.4GHz");
    maskSlopes = {
        std::make_pair(0, -45.000),   // Outer band left (start)
        std::make_pair(127, -28.133), // Outer band left (stop)
        std::make_pair(128, -28.000), // Middle band left (start)
        std::make_pair(252, -20.064), // Middle band left (stop)
        std::make_pair(253, -20.0),   // Flat junction band left (start)
        std::make_pair(255, -20.0),   // Flat junction band left (stop)
        std::make_pair(256, -20.0),   // Inner band left (start)
        std::make_pair(261, -3.333),  // Inner band left (stop)
        std::make_pair(262, 0.0),     // allocated band left (start)
        std::make_pair(382, 0.0),     // allocated band left (stop)
        std::make_pair(383, -20.0),   // DC band (start)
        std::make_pair(385, -20.0),   // DC band (stop)
        std::make_pair(386, 0.0),     // allocated band right (start)
        std::make_pair(506, 0.0),     // allocated band right (stop)
        std::make_pair(507, -3.333),  // Inner band right (start)
        std::make_pair(512, -20.0),   // Inner band right (stop)
        std::make_pair(513, -20.0),   // Flat junction band right (start)
        std::make_pair(515, -20.0),   // Flat junction band right (stop)
        std::make_pair(516, -20.064), // Middle band right (start)
        std::make_pair(640, -28.000), // Middle band right (stop)
        std::make_pair(641, -28.133), // Outer band right (start)
        std::make_pair(768, -45.000), // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_2.4GHz 20MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_2_4GHZ,
                                               20,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 20MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 20MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, -40.0),     // Outer band left (start)
        std::make_pair(127, -28.094), // Outer band left (stop)
        std::make_pair(128, -28.000), // Middle band left (start)
        std::make_pair(252, -20.064), // Middle band left (stop)
        std::make_pair(253, -20.0),   // Flat junction band left (start)
        std::make_pair(255, -20.0),   // Flat junction band left (stop)
        std::make_pair(256, -20.0),   // Inner band left (start)
        std::make_pair(261, -3.333),  // Inner band left (stop)
        std::make_pair(262, 0.0),     // allocated band left (start)
        std::make_pair(382, 0.0),     // allocated band left (stop)
        std::make_pair(383, -20.0),   // DC band (start)
        std::make_pair(385, -20.0),   // DC band (stop)
        std::make_pair(386, 0.0),     // allocated band right (start)
        std::make_pair(506, 0.0),     // allocated band right (stop)
        std::make_pair(507, -3.333),  // Inner band right (start)
        std::make_pair(512, -20.0),   // Inner band right (stop)
        std::make_pair(513, -20.0),   // Flat junction band right (start)
        std::make_pair(515, -20.0),   // Flat junction band right (stop)
        std::make_pair(516, -20.064), // Middle band right (start)
        std::make_pair(640, -28.000), // Middle band right (stop)
        std::make_pair(641, -28.094), // Outer band right (start)
        std::make_pair(768, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 20MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               20,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 40MHz @ 2.4GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 40MHz @ 2.4GHz");
    maskSlopes = {
        std::make_pair(0, -45.000),    // Outer band left (start)
        std::make_pair(255, -28.066),  // Outer band left (stop)
        std::make_pair(256, -28.000),  // Middle band left (start)
        std::make_pair(505, -20.032),  // Middle band left (stop)
        std::make_pair(506, -20.0),    // Flat junction band left (start)
        std::make_pair(510, -20.0),    // Flat junction band left (stop)
        std::make_pair(511, -20.0),    // Inner band left (start)
        std::make_pair(523, -1.538),   // Inner band left (stop)
        std::make_pair(524, 0.0),      // allocated band left (start)
        std::make_pair(765, 0.0),      // allocated band left (stop)
        std::make_pair(766, -20.0),    // DC band (start)
        std::make_pair(770, -20.0),    // DC band (stop)
        std::make_pair(771, 0.0),      // allocated band right (start)
        std::make_pair(1012, 0.0),     // allocated band right (stop)
        std::make_pair(1013, -1.538),  // Inner band right (start)
        std::make_pair(1025, -20.0),   // Inner band right (stop)
        std::make_pair(1026, -20.0),   // Flat junction band right (start)
        std::make_pair(1030, -20.0),   // Flat junction band right (stop)
        std::make_pair(1031, -20.032), // Middle band right (start)
        std::make_pair(1280, -28.000), // Middle band right (stop)
        std::make_pair(1281, -28.066), // Outer band right (start)
        std::make_pair(1536, -45.000), // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_2.4GHz 40MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_2_4GHZ,
                                               40,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 40MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 40MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(255, -28.047),  // Outer band left (stop)
        std::make_pair(256, -28.000),  // Middle band left (start)
        std::make_pair(505, -20.032),  // Middle band left (stop)
        std::make_pair(506, -20.0),    // Flat junction band left (start)
        std::make_pair(510, -20.0),    // Flat junction band left (stop)
        std::make_pair(511, -20.0),    // Inner band left (start)
        std::make_pair(523, -1.538),   // Inner band left (stop)
        std::make_pair(524, 0.0),      // allocated band left (start)
        std::make_pair(765, 0.0),      // allocated band left (stop)
        std::make_pair(766, -20.0),    // DC band (start)
        std::make_pair(770, -20.0),    // DC band (stop)
        std::make_pair(771, 0.0),      // allocated band right (start)
        std::make_pair(1012, 0.0),     // allocated band right (stop)
        std::make_pair(1013, -1.538),  // Inner band right (start)
        std::make_pair(1025, -20.0),   // Inner band right (stop)
        std::make_pair(1026, -20.0),   // Flat junction band right (start)
        std::make_pair(1030, -20.0),   // Flat junction band right (stop)
        std::make_pair(1031, -20.032), // Middle band right (start)
        std::make_pair(1280, -28.000), // Middle band right (stop)
        std::make_pair(1281, -28.047), // Outer band right (start)
        std::make_pair(1536, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 40MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               40,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 2.4GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 2.4GHz");
    maskSlopes = {
        std::make_pair(0, -45.000),    // Outer band left (start)
        std::make_pair(511, -28.033),  // Outer band left (stop)
        std::make_pair(512, -28.000),  // Middle band left (start)
        std::make_pair(1017, -20.016), // Middle band left (stop)
        std::make_pair(1018, -20.0),   // Flat junction band left (start)
        std::make_pair(1022, -20.0),   // Flat junction band left (stop)
        std::make_pair(1023, -20.0),   // Inner band left (start)
        std::make_pair(1035, -1.538),  // Inner band left (stop)
        std::make_pair(1036, 0.0),     // allocated band left (start)
        std::make_pair(1533, 0.0),     // allocated band left (stop)
        std::make_pair(1534, -20.0),   // DC band (start)
        std::make_pair(1538, -20.0),   // DC band (stop)
        std::make_pair(1539, 0.0),     // allocated band right (start)
        std::make_pair(2036, 0.0),     // allocated band right (stop)
        std::make_pair(2037, -1.538),  // Inner band right (start)
        std::make_pair(2049, -20.0),   // Inner band right (stop)
        std::make_pair(2050, -20.0),   // Flat junction band right (start)
        std::make_pair(2054, -20.0),   // Flat junction band right (stop)
        std::make_pair(2055, -20.016), // Middle band right (start)
        std::make_pair(2560, -28.000), // Middle band right (stop)
        std::make_pair(2561, -28.033), // Outer band right (start)
        std::make_pair(3072, -45.000), // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_2.4GHz 80MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_2_4GHZ,
                                               80,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(511, -28.023),  // Outer band left (stop)
        std::make_pair(512, -28.000),  // Middle band left (start)
        std::make_pair(1017, -20.016), // Middle band left (stop)
        std::make_pair(1018, -20.0),   // Flat junction band left (start)
        std::make_pair(1022, -20.0),   // Flat junction band left (stop)
        std::make_pair(1023, -20.0),   // Inner band left (start)
        std::make_pair(1035, -1.538),  // Inner band left (stop)
        std::make_pair(1036, 0.0),     // allocated band left (start)
        std::make_pair(1533, 0.0),     // allocated band left (stop)
        std::make_pair(1534, -20.0),   // DC band (start)
        std::make_pair(1538, -20.0),   // DC band (stop)
        std::make_pair(1539, 0.0),     // allocated band right (start)
        std::make_pair(2036, 0.0),     // allocated band right (stop)
        std::make_pair(2037, -1.538),  // Inner band right (start)
        std::make_pair(2049, -20.0),   // Inner band right (stop)
        std::make_pair(2050, -20.0),   // Flat junction band right (start)
        std::make_pair(2054, -20.0),   // Flat junction band right (stop)
        std::make_pair(2055, -20.016), // Middle band right (start)
        std::make_pair(2560, -28.000), // Middle band right (stop)
        std::make_pair(2561, -28.023), // Outer band right (start)
        std::make_pair(3072, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               80,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 160MHz @ 2.4GHz -> not enough space so skip

    // ============================================================================================
    // 11ax 160MHz @ 5GHz
    NS_LOG_FUNCTION("Check slopes for 11ax 160MHz @ 5GHz");
    maskSlopes = {
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(1023, -28.012), // Outer band left (stop)
        std::make_pair(1024, -28.000), // Middle band left (start)
        std::make_pair(2041, -20.008), // Middle band left (stop)
        std::make_pair(2042, -20.0),   // Flat junction band left (start)
        std::make_pair(2046, -20.0),   // Flat junction band left (stop)
        std::make_pair(2047, -20.0),   // Inner band left (start)
        std::make_pair(2059, -1.538),  // Inner band left (stop)
        std::make_pair(2060, 0.0),     // first 80 MHz allocated band left (start)
        std::make_pair(2557, 0.0),     // first 80 MHz allocated band left (stop)
        std::make_pair(2558, -20.0),   // first 80 MHz DC band (start)
        std::make_pair(2562, -20.0),   // first 80 MHz DC band (stop)
        std::make_pair(2563, 0.0),     // first 80 MHz allocated band right (start)
        std::make_pair(3060, 0.0),     // first 80 MHz allocated band right (stop)
        std::make_pair(3061, -20.0),   // gap between 80 MHz bands (start)
        std::make_pair(3083, -20.0),   // gap between 80 MHz bands (start)
        std::make_pair(3084, 0.0),     // second 80 MHz allocated band left (start)
        std::make_pair(3581, 0.0),     // second 80 MHz allocated band left (stop)
        std::make_pair(3582, -20.0),   // second 80 MHz DC band (start)
        std::make_pair(3586, -20.0),   // second 80 MHz DC band (stop)
        std::make_pair(3587, 0.0),     // second 80 MHz allocated band right (start)
        std::make_pair(4084, 0.0),     // second 80 MHz allocated band right (stop)
        std::make_pair(4085, -1.538),  // Inner band right (start)
        std::make_pair(4097, -20.0),   // Inner band right (stop)
        std::make_pair(4098, -20.0),   // Flat junction band right (start)
        std::make_pair(4102, -20.0),   // Flat junction band right (stop)
        std::make_pair(4103, -20.008), // Middle band right (start)
        std::make_pair(5120, -28.000), // Middle band right (stop)
        std::make_pair(5121, -28.012), // Outer band right (start)
        std::make_pair(6144, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 160MHz",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               160,
                                               maskSlopes,
                                               tol,
                                               prec),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 5GHz - first 20 MHz subchannel punctured
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 5GHz with first 20 MHz subchannel punctured");
    maskSlopes = {
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(511, -28.023),  // Outer band left (stop)
        std::make_pair(512, -28.000),  // Middle band left (start)
        std::make_pair(1017, -20.016), // Middle band left (stop)
        std::make_pair(1018, -20.0),   // Flat junction band left (start)
        std::make_pair(1022, -20.0),   // Flat junction band left (stop)
        std::make_pair(1023, -20.0),   // punctured band (start)
        std::make_pair(1272, -20.0),   // punctured band (stop)
        std::make_pair(1273, -20.0),   // punctured band increasing slope (start)
        std::make_pair(1279, 0.0),     // punctured band increasing slope (stop)
        std::make_pair(1280, 0.0),     // allocated band left (start)
        std::make_pair(1533, 0.0),     // allocated band left (stop)
        std::make_pair(1534, -20.0),   // DC band (start)
        std::make_pair(1538, -20.0),   // DC band (stop)
        std::make_pair(1539, 0.0),     // allocated band right (start)
        std::make_pair(2036, 0.0),     // allocated band right (stop)
        std::make_pair(2037, -1.538),  // Inner band right (start)
        std::make_pair(2049, -20.0),   // Inner band right (stop)
        std::make_pair(2050, -20.0),   // Flat junction band right (start)
        std::make_pair(2054, -20.0),   // Flat junction band right (stop)
        std::make_pair(2055, -20.016), // Middle band right (start)
        std::make_pair(2560, -28.000), // Middle band right (stop)
        std::make_pair(2561, -28.023), // Outer band right (start)
        std::make_pair(3072, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80MHz first subchannel punctured",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               80,
                                               maskSlopes,
                                               tol,
                                               prec,
                                               {true, false, false, false}),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 5GHz - second 20 MHz subchannel punctured
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 5GHz with second 20 MHz subchannel punctured");
    maskSlopes = {
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(511, -28.023),  // Outer band left (stop)
        std::make_pair(512, -28.000),  // Middle band left (start)
        std::make_pair(1017, -20.016), // Middle band left (stop)
        std::make_pair(1018, -20.0),   // Flat junction band left (start)
        std::make_pair(1022, -20.0),   // Flat junction band left (stop)
        std::make_pair(1023, -20.0),   // Inner band left (start)
        std::make_pair(1035, -1.538),  // Inner band left (stop)
        std::make_pair(1036, 0.0),     // allocated band left (start)
        std::make_pair(1279, 0.0),     // allocated band left (stop)
        std::make_pair(1280, 0.0),     // punctured band decreasing slope (start)
        std::make_pair(1286, -20.0),   // punctured band decreasing slope (stop)
        std::make_pair(1287, -20.0),   // punctured band (start)
        std::make_pair(1528, -20.0),   // punctured band (stop)
        std::make_pair(1529, -20.0),   // punctured band increasing slope (start)
        std::make_pair(1533, -6.667),  // punctured band increasing slope (stop)
        std::make_pair(1534, -20.0),   // DC band (start)
        std::make_pair(1538, -20.0),   // DC band (stop)
        std::make_pair(1539, 0.0),     // allocated band right (start)
        std::make_pair(2036, 0.0),     // allocated band right (stop)
        std::make_pair(2037, -1.538),  // Inner band right (start)
        std::make_pair(2049, -20.0),   // Inner band right (stop)
        std::make_pair(2050, -20.0),   // Flat junction band right (start)
        std::make_pair(2054, -20.0),   // Flat junction band right (stop)
        std::make_pair(2055, -20.016), // Middle band right (start)
        std::make_pair(2560, -28.000), // Middle band right (stop)
        std::make_pair(2561, -28.023), // Outer band right (start)
        std::make_pair(3072, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80MHz second subchannel punctured",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               80,
                                               maskSlopes,
                                               tol,
                                               prec,
                                               {false, true, false, false}),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 5GHz - third 20 MHz subchannel punctured
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 5GHz with third 20 MHz subchannel punctured");
    maskSlopes = {
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(511, -28.023),  // Outer band left (stop)
        std::make_pair(512, -28.000),  // Middle band left (start)
        std::make_pair(1017, -20.016), // Middle band left (stop)
        std::make_pair(1018, -20.0),   // Flat junction band left (start)
        std::make_pair(1022, -20.0),   // Flat junction band left (stop)
        std::make_pair(1023, -20.0),   // Inner band left (start)
        std::make_pair(1035, -1.538),  // Inner band left (stop)
        std::make_pair(1036, 0.0),     // allocated band left (start)
        std::make_pair(1533, 0.0),     // allocated band left (stop)
        std::make_pair(1534, -20.0),   // DC band (start)
        std::make_pair(1535, -20.0),   // DC band (stop)
        std::make_pair(1539, -10.0),   // punctured band decreasing slope (start)
        std::make_pair(1542, -20.0),   // punctured band decreasing slope (stop)
        std::make_pair(1543, -20.0),   // punctured band (start)
        std::make_pair(1784, -20.0),   // punctured band (stop)
        std::make_pair(1785, -20.0),   // punctured band increasing slope (start)
        std::make_pair(1791, 0.0),     // punctured band increasing slope (stop)
        std::make_pair(1792, 0.0),     // allocated band right (start)
        std::make_pair(2036, 0.0),     // allocated band right (stop)
        std::make_pair(2037, -1.538),  // Inner band right (start)
        std::make_pair(2049, -20.0),   // Inner band right (stop)
        std::make_pair(2050, -20.0),   // Flat junction band right (start)
        std::make_pair(2054, -20.0),   // Flat junction band right (stop)
        std::make_pair(2055, -20.016), // Middle band right (start)
        std::make_pair(2560, -28.000), // Middle band right (stop)
        std::make_pair(2561, -28.023), // Outer band right (start)
        std::make_pair(3072, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80MHz third subchannel punctured",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               80,
                                               maskSlopes,
                                               tol,
                                               prec,
                                               {false, false, true, false}),
                TestCase::Duration::QUICK);

    // ============================================================================================
    // 11ax 80MHz @ 5GHz - last 20 MHz subchannel punctured
    NS_LOG_FUNCTION("Check slopes for 11ax 80MHz @ 5GHz with last 20 MHz subchannel punctured");
    maskSlopes = {
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(511, -28.023),  // Outer band left (stop)
        std::make_pair(512, -28.000),  // Middle band left (start)
        std::make_pair(1017, -20.016), // Middle band left (stop)
        std::make_pair(1018, -20.0),   // Flat junction band left (start)
        std::make_pair(1022, -20.0),   // Flat junction band left (stop)
        std::make_pair(1023, -20.0),   // Inner band left (start)
        std::make_pair(1035, -1.538),  // Inner band left (stop)
        std::make_pair(1036, 0.0),     // allocated band left (start)
        std::make_pair(1533, 0.0),     // allocated band left (stop)
        std::make_pair(1534, -20.0),   // DC band (start)
        std::make_pair(1538, -20.0),   // DC band (stop)
        std::make_pair(1539, 0.0),     // allocated band right (start)
        std::make_pair(1791, 0.0),     // allocated band right (stop)
        std::make_pair(1792, 0.0),     // punctured band decreasing slope (start)
        std::make_pair(1798, -20.0),   // punctured band decreasing slope (stop)
        std::make_pair(1799, -20.0),   // punctured band (start)
        std::make_pair(2049, -20.0),   // punctured band (stop)
        std::make_pair(2050, -20.0),   // Flat junction band right (start)
        std::make_pair(2054, -20.0),   // Flat junction band right (stop)
        std::make_pair(2055, -20.016), // Middle band right (start)
        std::make_pair(2560, -28.000), // Middle band right (stop)
        std::make_pair(2561, -28.023), // Outer band right (start)
        std::make_pair(3072, -40.0),   // Outer band right (stop)
    };

    AddTestCase(new WifiOfdmMaskSlopesTestCase("11ax_5GHz 80MHz last subchannel punctured",
                                               WIFI_STANDARD_80211ax,
                                               WIFI_PHY_BAND_5GHZ,
                                               80,
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
        std::make_pair(0, -40.0),       // Outer band left (start)
        std::make_pair(1023, -28.012),  // Outer band left (stop)
        std::make_pair(1024, -28.000),  // Middle band left (start)
        std::make_pair(2041, -20.008),  // Middle band left (stop)
        std::make_pair(2042, -20.0),    // Flat junction band left (start)
        std::make_pair(2046, -20.0),    // Flat junction band left (stop)
        std::make_pair(2047, -20.0),    // punctured band (start)
        std::make_pair(2552, -20.0),    // punctured band (stop)
        std::make_pair(2553, -20.0),    // punctured band increasing slope (start)
        std::make_pair(2557, -6.66667), // punctured band increasing slope (stop)
        std::make_pair(2558, -20.0),    // first 80 MHz DC band (start)
        std::make_pair(2562, -20.0),    // first 80 MHz DC band (stop)
        std::make_pair(2563, 0.0),      // first 80 MHz allocated band right (start)
        std::make_pair(3060, 0.0),      // first 80 MHz allocated band right (stop)
        std::make_pair(3061, -20.0),    // gap between 80 MHz bands (start)
        std::make_pair(3083, -20.0),    // gap between 80 MHz bands (start)
        std::make_pair(3084, 0.0),      // second 80 MHz allocated band left (start)
        std::make_pair(3581, 0.0),      // second 80 MHz allocated band left (stop)
        std::make_pair(3582, -20.0),    // second 80 MHz DC band (start)
        std::make_pair(3586, -20.0),    // second 80 MHz DC band (stop)
        std::make_pair(3587, 0.0),      // second 80 MHz allocated band right (start)
        std::make_pair(4084, 0.0),      // second 80 MHz allocated band right (stop)
        std::make_pair(4085, -1.538),   // Inner band right (start)
        std::make_pair(4097, -20.0),    // Inner band right (stop)
        std::make_pair(4098, -20.0),    // Flat junction band right (start)
        std::make_pair(4102, -20.0),    // Flat junction band right (stop)
        std::make_pair(4103, -20.008),  // Middle band right (start)
        std::make_pair(5120, -28.000),  // Middle band right (stop)
        std::make_pair(5121, -28.012),  // Outer band right (start)
        std::make_pair(6144, -40.0),    // Outer band right (stop)
    };

    AddTestCase(
        new WifiOfdmMaskSlopesTestCase("11ax_5GHz 160MHz first subchannels punctured",
                                       WIFI_STANDARD_80211ax,
                                       WIFI_PHY_BAND_5GHZ,
                                       160,
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
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(1023, -28.012), // Outer band left (stop)
        std::make_pair(1024, -28.000), // Middle band left (start)
        std::make_pair(2041, -20.008), // Middle band left (stop)
        std::make_pair(2042, -20.0),   // Flat junction band left (start)
        std::make_pair(2046, -20.0),   // Flat junction band left (stop)
        std::make_pair(2047, -20.0),   // Inner band left (start)
        std::make_pair(2059, -1.538),  // Inner band left (stop)
        std::make_pair(2060, 0.0),     // first 80 MHz allocated band left (start)
        std::make_pair(2557, 0.0),     // first 80 MHz allocated band left (stop)
        std::make_pair(2558, -20.0),   // first 80 MHz DC band (start)
        std::make_pair(2562, -20.0),   // first 80 MHz DC band (stop)
        std::make_pair(2563, -10.0),   // punctured band decreasing slope (start)
        std::make_pair(2566, -20.0),   // punctured band decreasing slope (stop)
        std::make_pair(2567, -20.0),   // punctured band (start)
        std::make_pair(3060, -20.0),   // punctured band (stop)
        std::make_pair(3061, -20.0),   // gap between 80 MHz bands (start)
        std::make_pair(3083, -20.0),   // gap between 80 MHz bands (start)
        std::make_pair(3084, 0.0),     // second 80 MHz allocated band left (start)
        std::make_pair(3581, 0.0),     // second 80 MHz allocated band left (stop)
        std::make_pair(3582, -20.0),   // second 80 MHz DC band (start)
        std::make_pair(3586, -20.0),   // second 80 MHz DC band (stop)
        std::make_pair(3587, 0.0),     // second 80 MHz allocated band right (start)
        std::make_pair(4084, 0.0),     // second 80 MHz allocated band right (stop)
        std::make_pair(4085, -1.538),  // Inner band right (start)
        std::make_pair(4097, -20.0),   // Inner band right (stop)
        std::make_pair(4098, -20.0),   // Flat junction band right (start)
        std::make_pair(4102, -20.0),   // Flat junction band right (stop)
        std::make_pair(4103, -20.008), // Middle band right (start)
        std::make_pair(5120, -28.000), // Middle band right (stop)
        std::make_pair(5121, -28.012), // Outer band right (start)
        std::make_pair(6144, -40.0),   // Outer band right (stop)
    };

    AddTestCase(
        new WifiOfdmMaskSlopesTestCase("11ax_5GHz 160MHz third and fourth subchannels punctured",
                                       WIFI_STANDARD_80211ax,
                                       WIFI_PHY_BAND_5GHZ,
                                       160,
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
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(1023, -28.012), // Outer band left (stop)
        std::make_pair(1024, -28.000), // Middle band left (start)
        std::make_pair(2041, -20.008), // Middle band left (stop)
        std::make_pair(2042, -20.0),   // Flat junction band left (start)
        std::make_pair(2046, -20.0),   // Flat junction band left (stop)
        std::make_pair(2047, -20.0),   // Inner band left (start)
        std::make_pair(2059, -1.538),  // Inner band left (stop)
        std::make_pair(2060, 0.0),     // first 80 MHz allocated band left (start)
        std::make_pair(2557, 0.0),     // first 80 MHz allocated band left (stop)
        std::make_pair(2558, -20.0),   // first 80 MHz DC band (start)
        std::make_pair(2562, -20.0),   // first 80 MHz DC band (stop)
        std::make_pair(2563, 0.0),     // first 80 MHz allocated band right (start)
        std::make_pair(3060, 0.0),     // first 80 MHz allocated band right (stop)
        std::make_pair(3061, -20.0),   // gap between 80 MHz bands (start)
        std::make_pair(3083, -20.0),   // gap between 80 MHz bands (start)
        std::make_pair(3084, -20.0),   // punctured band (start)
        std::make_pair(3576, -20.0),   // punctured band (stop)
        std::make_pair(3577, -20.0),   // punctured band increasing slope (start)
        std::make_pair(3581, -6.667),  // punctured band increasing slope (stop)
        std::make_pair(3582, -20.0),   // second 80 MHz DC band (start)
        std::make_pair(3586, -20.0),   // second 80 MHz DC band (stop)
        std::make_pair(3587, 0.0),     // second 80 MHz allocated band right (start)
        std::make_pair(4084, 0.0),     // second 80 MHz allocated band right (stop)
        std::make_pair(4085, -1.538),  // Inner band right (start)
        std::make_pair(4097, -20.0),   // Inner band right (stop)
        std::make_pair(4098, -20.0),   // Flat junction band right (start)
        std::make_pair(4102, -20.0),   // Flat junction band right (stop)
        std::make_pair(4103, -20.008), // Middle band right (start)
        std::make_pair(5120, -28.000), // Middle band right (stop)
        std::make_pair(5121, -28.012), // Outer band right (start)
        std::make_pair(6144, -40.0),   // Outer band right (stop)
    };

    AddTestCase(
        new WifiOfdmMaskSlopesTestCase("11ax_5GHz 160MHz fifth and sixth subchannels punctured",
                                       WIFI_STANDARD_80211ax,
                                       WIFI_PHY_BAND_5GHZ,
                                       160,
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
        std::make_pair(0, -40.0),      // Outer band left (start)
        std::make_pair(1023, -28.012), // Outer band left (stop)
        std::make_pair(1024, -28.000), // Middle band left (start)
        std::make_pair(2041, -20.008), // Middle band left (stop)
        std::make_pair(2042, -20.0),   // Flat junction band left (start)
        std::make_pair(2046, -20.0),   // Flat junction band left (stop)
        std::make_pair(2047, -20.0),   // Inner band left (start)
        std::make_pair(2059, -1.538),  // Inner band left (stop)
        std::make_pair(2060, 0.0),     // first 80 MHz allocated band left (start)
        std::make_pair(2557, 0.0),     // first 80 MHz allocated band left (stop)
        std::make_pair(2558, -20.0),   // first 80 MHz DC band (start)
        std::make_pair(2562, -20.0),   // first 80 MHz DC band (stop)
        std::make_pair(2563, 0.0),     // first 80 MHz allocated band right (start)
        std::make_pair(3060, 0.0),     // first 80 MHz allocated band right (stop)
        std::make_pair(3061, -20.0),   // gap between 80 MHz bands (start)
        std::make_pair(3083, -20.0),   // gap between 80 MHz bands (start)
        std::make_pair(3084, 0.0),     // second 80 MHz allocated band left (start)
        std::make_pair(3581, 0.0),     // second 80 MHz allocated band left (stop)
        std::make_pair(3582, -20.0),   // second 80 MHz DC band (start)
        std::make_pair(3586, -20.0),   // second 80 MHz DC band (stop)
        std::make_pair(3587, -10.0),   // punctured band decreasing slope (start)
        std::make_pair(3590, -20.0),   // punctured band decreasing slope (stop)
        std::make_pair(3591, -20.0),   // punctured band (start)
        std::make_pair(4097, -20.0),   // punctured band (stop)
        std::make_pair(4098, -20.0),   // Flat junction band right (start)
        std::make_pair(4102, -20.0),   // Flat junction band right (stop)
        std::make_pair(4103, -20.008), // Middle band right (start)
        std::make_pair(5120, -28.000), // Middle band right (stop)
        std::make_pair(5121, -28.012), // Outer band right (start)
        std::make_pair(6144, -40.0),   // Outer band right (stop)
    };

    AddTestCase(
        new WifiOfdmMaskSlopesTestCase("11ax_5GHz 160MHz last subchannels punctured",
                                       WIFI_STANDARD_80211ax,
                                       WIFI_PHY_BAND_5GHZ,
                                       160,
                                       maskSlopes,
                                       tol,
                                       prec,
                                       {false, false, false, false, false, false, true, true}),
        TestCase::Duration::QUICK);
}
