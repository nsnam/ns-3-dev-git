#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/tuple.h"
#include "ns3/units-aliases.h"
#include "ns3/units-angle.h"
#include "ns3/units-frequency.h"
#include "ns3/units-power.h"
#include "ns3/units-ratio.h"
#include "ns3/units-time.h"

#include <istream>
#include <streambuf>
#include <tuple>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SiUnitsTest");

// clang-format off

/// A stream buffer for testing.
/// \details See TestInputOperatorPositives() function for usage
struct StreamBuffer : public std::streambuf
{
    /// Constructor.
    /// \param str The string to use as the stream buffer.
    StreamBuffer(const std::string& str)
    {
        auto cstr = str.c_str();
        auto beg = const_cast<char*>(cstr);
        auto len = str.size();
        setg(beg, beg, beg + len);
    }
};

/// Test case for SI units.
class TestCaseSiUnits : public TestCase
{
  public:
    TestCaseSiUnits()
        : TestCase("SiUnits") ///< Boiler plate constructor
    {
    }

  private:
    /// Test input operators - positive cases
    /// @tparam T type
    /// @param tvs test vectors
    template <typename T>
    void TestInputOperatorPositives(const std::vector<std::string>& tvs)
    {
        T got;
        for (auto& tv : tvs)
        {
            T want{tv};
            StreamBuffer buf(tv);
            std::istream is(&buf);
            is >> got;
            NS_TEST_EXPECT_MSG_EQ(got, want, tv);
        }
    }

    /// Test input operators - negative cases
    /// @tparam T type
    /// @param tvs test vectors
    template <typename T>
    void TestInputOperatorNegatives(const std::vector<std::string>& tvs)
    {
        T got;
        for (auto& tv : tvs)
        {
            StreamBuffer buf(tv);
            std::istream is(&buf);
            is >> got;
            NS_TEST_EXPECT_MSG_EQ(is.fail(), true, tv);
        }
    }

    /// Test degree_t
    void Unit_degree() // NOLINT(readability-identifier-naming)
    {
        NS_TEST_EXPECT_MSG_EQ(degree_t{1}, degree_t{1.0}, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{-1}, degree_t{-1.0}, "");
        NS_TEST_EXPECT_MSG_EQ(0_degree, -(0_degree), "");
        NS_TEST_EXPECT_MSG_EQ(0_degree, 0.0_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{"20.1 degree"}, 20.1_degree, "");

        NS_TEST_EXPECT_MSG_EQ((30_degree == 30.0_degree), true, "");
        NS_TEST_EXPECT_MSG_EQ((30_degree != 40.0_degree), true, "");
        NS_TEST_EXPECT_MSG_EQ((30_degree < 40_degree), true, "");
        NS_TEST_EXPECT_MSG_EQ((30_degree <= 40_degree), true, "");
        NS_TEST_EXPECT_MSG_EQ((30_degree <= 30_degree), true, "");
        NS_TEST_EXPECT_MSG_EQ((40_degree > 30_degree), true, "");
        NS_TEST_EXPECT_MSG_EQ((40_degree >= 30_degree), true, "");
        NS_TEST_EXPECT_MSG_EQ((30_degree >= 30_degree), true, "");

        NS_TEST_EXPECT_MSG_EQ((30_degree + 40_degree), 70_degree, "");
        NS_TEST_EXPECT_MSG_EQ((100_degree + 150_degree), 250_degree, "");
        NS_TEST_EXPECT_MSG_EQ((100_degree - 150_degree), -50_degree, "");
        NS_TEST_EXPECT_MSG_EQ((100_degree - 350_degree), -250_degree, "");

        NS_TEST_EXPECT_MSG_EQ((300_degree * 2.5), 750_degree, "");
        NS_TEST_EXPECT_MSG_EQ((2.5 * 300_degree), 750_degree, "");
        NS_TEST_EXPECT_MSG_EQ((300_degree / 4.0), 75_degree, "");

        NS_TEST_EXPECT_MSG_EQ(degree_t{100}.normalize(), 100_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{170}.normalize(), 170_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{190}.normalize(), -(170_degree), "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{370}.normalize(), 10_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{-100}.normalize(), -(100_degree), "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{-170}.normalize(), -(170_degree), "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{-190}.normalize(), 170_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{-370}.normalize(), -(10_degree), "");

        NS_TEST_EXPECT_MSG_EQ((123.4_degree).str(), "123.4 degree", "");
        NS_TEST_EXPECT_MSG_EQ((123.4_degree).str(false), "123.4degree", "");
        NS_TEST_EXPECT_MSG_EQ(degree_t::from_radian(radian_t{M_PI}), 180_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{180}.to_radian(), radian_t{M_PI}, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{180}.in_radian(), M_PI, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{123.4}.in_degree(), 123.4, "");

        // Conversion from string
        NS_TEST_EXPECT_MSG_EQ(degree_t::from_str("90degree").value(), 90_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t::from_str("90.0degree").value(), 90_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t::from_str("90.00degree").value(), 90_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t::from_str("3.14degree").value(), 3.14_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t::from_str("3.14 degree").value(), 3.14_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t::from_str("  3.14  degree  ").value(), 3.14_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t::from_str("-3.14degree").value(), -3.14_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t::from_str("3.14 Degree").has_value(), false, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t::from_str("3.14_degree").has_value(), false, "");

        TestInputOperatorPositives<degree_t>({"12.3degree", "12.3 degree", " 12.3  degree "});
        TestInputOperatorNegatives<degree_t>(
            {"12.3Degree", "12.3'", "12.3_degree", "12.3degree_t", "12.3", "12.3dBm"});

        NS_TEST_EXPECT_MSG_EQ(degree_t{"90degree"}, 90_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{"90.0degree"}, 90_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{"90.00degree"}, 90_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{"3.14degree"}, 3.14_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{"3.14 degree"}, 3.14_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{"  3.14  degree  "}, 3.14_degree, "");
        NS_TEST_EXPECT_MSG_EQ(degree_t{"-3.14degree"}, -3.14_degree, "");
    }

    /// Test radian_t
    void Unit_radian() // NOLINT(readability-identifier-naming)
    {
        NS_TEST_EXPECT_MSG_EQ(radian_t{1}, radian_t{1.0}, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{-1}, radian_t{-1.0}, "");
        NS_TEST_EXPECT_MSG_EQ(0_radian, -(0_radian), "");
        NS_TEST_EXPECT_MSG_EQ(0_radian, 0.0_radian, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{"1.5 radian"}, 1.5_radian, "");

        NS_TEST_EXPECT_MSG_EQ((30_radian == 30.0_radian), true, "");
        NS_TEST_EXPECT_MSG_EQ((30_radian != 40.0_radian), true, "");
        NS_TEST_EXPECT_MSG_EQ((30_radian < 40_radian), true, "");
        NS_TEST_EXPECT_MSG_EQ((30_radian <= 40_radian), true, "");
        NS_TEST_EXPECT_MSG_EQ((30_radian <= 30_radian), true, "");
        NS_TEST_EXPECT_MSG_EQ((40_radian > 30_radian), true, "");
        NS_TEST_EXPECT_MSG_EQ((40_radian >= 30_radian), true, "");
        NS_TEST_EXPECT_MSG_EQ((30_radian >= 30_radian), true, "");

        NS_TEST_EXPECT_MSG_EQ((30_radian + 40_radian), 70_radian, "");
        NS_TEST_EXPECT_MSG_EQ((100_radian + 150_radian), 250_radian, "");
        NS_TEST_EXPECT_MSG_EQ((100_radian - 150_radian), -50_radian, "");
        NS_TEST_EXPECT_MSG_EQ((100_radian - 350_radian), -250_radian, "");

        NS_TEST_EXPECT_MSG_EQ((300_radian * 2.5), 750_radian, "");
        NS_TEST_EXPECT_MSG_EQ((2.5 * 300_radian), 750_radian, "");
        NS_TEST_EXPECT_MSG_EQ((300_radian / 4.0), 75_radian, "");

        // Normalization is subject to the floating-point precision error. Adopt the rough
        // comparison at will.
        NS_TEST_EXPECT_MSG_EQ(radian_t{0.75 * M_PI}.normalize(), radian_t{0.75 * M_PI}, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{1.25 * M_PI}.normalize(), radian_t{-0.75 * M_PI}, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{2.00 * M_PI}.normalize(), radian_t{0.00 * M_PI}, "");
        NS_TEST_EXPECT_MSG_EQ_TOL(radian_t{2.25 * M_PI}.normalize().in_radian(),
                                  radian_t{0.25 * M_PI}.in_radian(),
                                  1e-10, // sufficient resolution
                                  "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{-0.75 * M_PI}.normalize(), radian_t{-0.75 * M_PI}, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{-1.25 * M_PI}.normalize(), radian_t{0.75 * M_PI}, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{-2.00 * M_PI}.normalize(), radian_t{0.00 * M_PI}, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{-2.25 * M_PI}.normalize(), radian_t{-0.25 * M_PI}, "");

        NS_TEST_EXPECT_MSG_EQ((123.4_radian).str(), "123.4 radian", "");
        NS_TEST_EXPECT_MSG_EQ((123.4_radian).str(false), "123.4radian", "");
        NS_TEST_EXPECT_MSG_EQ(radian_t::from_degree(180_degree), radian_t{M_PI}, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{M_PI}.to_degree(), 180_degree, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{M_PI}.in_degree(), 180, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t{123.4}.in_radian(), 123.4, "");

        // Conversion from string
        NS_TEST_EXPECT_MSG_EQ(radian_t::from_str("3.14radian").value(), 3.14_radian, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t::from_str("3.14 radian").value(), 3.14_radian, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t::from_str("  3.14  radian  ").value(), 3.14_radian, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t::from_str("-3.14radian").value(), -3.14_radian, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t::from_str("3.14 Radian").has_value(), false, "");
        NS_TEST_EXPECT_MSG_EQ(radian_t::from_str("3.14_radian").has_value(), false, "");

        TestInputOperatorPositives<radian_t>({"12.3radian", "12.3 radian", " 12.3  radian "});
        TestInputOperatorNegatives<radian_t>(
            {"12.3Radian", "12.3_radian", "12.3radian_t", "12.3", "12.3degree"});
    }

    /// Test dB_t
    void Unit_dB() // NOLINT(readability-identifier-naming)
    {
        // Notations
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, dB_t{0.}, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, dB_t{0.0}, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, dB_t{-0}, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, 0_dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, 0._dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, 0.0_dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, -0_dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, -0._dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, -0.0_dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, dB_t(0.0), "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, dB_t{0_dB}, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{0}, dB_t(0_dB), "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{"1.5 dB"}, 1.5_dB, "");

        // Equality, inequality
        NS_TEST_EXPECT_MSG_EQ(dB_t{10}, 10_dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t{-10}, -10_dB, "");
        NS_TEST_EXPECT_MSG_EQ((dB_t{10} != 10_dB), false, "");
        NS_TEST_EXPECT_MSG_EQ((dB_t{10} == 20.0_dB), false, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ((dB_t{10} != 20_dB), true, "");    // NOLINT

        // Comparison
        NS_TEST_EXPECT_MSG_LT(1_dB, 2_dB, "");
        NS_TEST_EXPECT_MSG_GT(2_dB, 1_dB, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1_dB, 1_dB, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1_dB, 2_dB, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_dB, 1_dB, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_dB, 2_dB, "");
        NS_TEST_EXPECT_MSG_LT(-1_dB, 2_dB, "");
        NS_TEST_EXPECT_MSG_GT(2_dB, -1_dB, "");
        NS_TEST_EXPECT_MSG_EQ((10_dB < 20_dB), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_dB <= 20_dB), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_dB > 20_dB), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_dB >= 20_dB), false, "");

        // Arithmetic
        NS_TEST_EXPECT_MSG_EQ((1_dB + 2_dB), 3_dB, "");
        NS_TEST_EXPECT_MSG_EQ((3_dB - 1_dB), 2_dB, "");
        NS_TEST_EXPECT_MSG_EQ((3_dB - 9_dB), -6_dB, "");
        NS_TEST_EXPECT_MSG_EQ((5_dB += 10_dB), 15_dB, "");
        NS_TEST_EXPECT_MSG_EQ((5_dB -= 10_dB), -5_dB, "");
        NS_TEST_EXPECT_MSG_EQ(-8_dB, (0_dB - 8_dB), "");

        // Utilities
        NS_TEST_EXPECT_MSG_EQ(dB_t{123}.str(), "123.0 dB", "");     // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dB_t{123}.str(false), "123.0dB", ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dB_t{123.45}.val, 123.45, "");        // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dB_t{123.45}.str(), "123.5 dB", "");  // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dB_t{20}.to_linear(), 100.0, "");

        // Conversion from string
        NS_TEST_EXPECT_MSG_EQ(dB_t::from_str("3.14dB").value(), 3.14_dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t::from_str("3.14 dB").value(), 3.14_dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t::from_str("  3.14  dB  ").value(), 3.14_dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t::from_str("-3.14dB").value(), -3.14_dB, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t::from_str("3.14 Db").has_value(), false, "");
        NS_TEST_EXPECT_MSG_EQ(dB_t::from_str("3.14_dB").has_value(), false, "");

        TestInputOperatorPositives<dB_t>({"12.3dB", "12.3 dB", " 12.3  dB "});
        TestInputOperatorNegatives<dB_t>(
            {"12.3db", "12.3DB", "12.3_dB", "12.3dB_t", "12.3", "12.3dBm"});
    }

    /// Test dBr_t
    void Unit_dBr() // NOLINT(readability-identifier-naming)
    {
        // Notations
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, dBr_t{0.}, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, dBr_t{0.0}, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, dBr_t{-0}, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, 0_dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, 0._dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, 0.0_dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, -0_dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, -0._dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, -0.0_dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, dBr_t(0.0), "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, dBr_t{0_dBr}, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{0}, dBr_t(0_dBr), "");

        // Equality, inequality
        NS_TEST_EXPECT_MSG_EQ(dBr_t{10}, 10_dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t{-10}, -10_dBr, "");
        NS_TEST_EXPECT_MSG_EQ((dBr_t{10} != 10_dBr), false, "");
        NS_TEST_EXPECT_MSG_EQ((dBr_t{10} == 20.0_dBr), false, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ((dBr_t{10} != 20_dBr), true, "");    // NOLINT

        // Comparison
        NS_TEST_EXPECT_MSG_LT(1_dBr, 2_dBr, "");
        NS_TEST_EXPECT_MSG_GT(2_dBr, 1_dBr, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1_dBr, 1_dBr, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1_dBr, 2_dBr, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_dBr, 1_dBr, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_dBr, 2_dBr, "");
        NS_TEST_EXPECT_MSG_LT(-1_dBr, 2_dBr, "");
        NS_TEST_EXPECT_MSG_GT(2_dBr, -1_dBr, "");
        NS_TEST_EXPECT_MSG_EQ((10_dBr < 20_dBr), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_dBr <= 20_dBr), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_dBr > 20_dBr), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_dBr >= 20_dBr), false, "");

        // Arithmetic
        NS_TEST_EXPECT_MSG_EQ((1_dBr + 2_dBr), 3_dBr, "");
        NS_TEST_EXPECT_MSG_EQ((3_dBr - 1_dBr), 2_dBr, "");
        NS_TEST_EXPECT_MSG_EQ((3_dBr - 9_dBr), -6_dBr, "");
        NS_TEST_EXPECT_MSG_EQ((5_dBr += 10_dBr), 15_dBr, "");
        NS_TEST_EXPECT_MSG_EQ((5_dBr -= 10_dBr), -5_dBr, "");
        NS_TEST_EXPECT_MSG_EQ(-8_dBr, (0_dBr - 8_dBr), "");

        // Utilities
        NS_TEST_EXPECT_MSG_EQ(dBr_t{123}.str(), "123.0 dBr", "");     // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBr_t{123}.str(false), "123.0dBr", ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBr_t{123.45}.val, 123.45, "");         // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBr_t{123.45}.str(), "123.5 dBr", "");  // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBr_t{20}.to_linear(), 100.0, "");

        // Conversion from string
        NS_TEST_EXPECT_MSG_EQ(dBr_t::from_str("3.14dBr").value(), 3.14_dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t::from_str("3.14 dBr").value(), 3.14_dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t::from_str("  3.14  dBr  ").value(), 3.14_dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t::from_str("-3.14dBr").value(), -3.14_dBr, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t::from_str("3.14 Dbr").has_value(), false, "");
        NS_TEST_EXPECT_MSG_EQ(dBr_t::from_str("3.14_dBr").has_value(), false, "");

        TestInputOperatorPositives<dBr_t>({"12.3dBr", "12.3 dBr", " 12.3  dBr "});
        TestInputOperatorNegatives<dBr_t>(
            {"12.3DBR", "12.3dbr", "12.3_dBr", "12.3dBr_t", "12.3", "12.3dB"});
    }

    /// Test mWatt_t
    void Unit_mWatt() // NOLINT
    {
        // Notations
        NS_TEST_EXPECT_MSG_EQ(mWatt_t{0}, 0_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ(mWatt_t{"1.5 mWatt"}, 1.5_mWatt, "");

        // Equality, inequality
        NS_TEST_EXPECT_MSG_EQ_TOL(1_mWatt, 1e9_pWatt, 1_pWatt, ""); // NOLINT

        // Comparison
        NS_TEST_EXPECT_MSG_LT(1_mWatt, 2_mWatt, "");
        NS_TEST_EXPECT_MSG_GT(2_mWatt, 1_mWatt, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1_mWatt, 1_mWatt, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1_mWatt, 2_mWatt, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_mWatt, 1_mWatt, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_mWatt, 2_mWatt, "");

        // Arithmetic
        NS_TEST_EXPECT_MSG_EQ((1_mWatt + 2_mWatt), 3_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((3_mWatt - 1_mWatt), 2_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((3_mWatt - 9_mWatt), -6_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((5_mWatt += 10_mWatt), 15_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((5_mWatt -= 10_mWatt), -5_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ(-8_mWatt, (0_mWatt - 8_mWatt), "");

        // Utilities
        NS_TEST_EXPECT_MSG_EQ(mWatt_t{123}.str(), "123.0 mWatt", "");     // NOLINT
        NS_TEST_EXPECT_MSG_EQ(mWatt_t{123}.str(false), "123.0mWatt", ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(mWatt_t{123.45}.str(), "123.5 mWatt", "");  // NOLINT
        NS_TEST_EXPECT_MSG_EQ(mWatt_t{100}.in_dBm(), 20.0, "");
        NS_TEST_EXPECT_MSG_EQ(mWatt_t{123.45}.in_Watt(), 0.12345, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(mWatt_t{123.45}.in_mWatt(), 123.45, ""); // NOLINT

        TestInputOperatorPositives<mWatt_t>({"12.3mWatt", "12.3 mWatt", " 12.3  mWatt "});
        TestInputOperatorNegatives<mWatt_t>(
            {"12.3MWATT", "12.3mW", "12.3_mWatt", "12.3mWatt_t", "12.3", "12.3dBm"});
    }

    /// Test Watt_t
    void Unit_Watt() // NOLINT
    {
        // Notations
        NS_TEST_EXPECT_MSG_EQ(Watt_t{0}, 0_Watt, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{0}, Watt_t{0.}, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{0}, Watt_t{0.0}, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{0}, Watt_t{-0}, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{0}, 0_Watt, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{0}, 0._Watt, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{0}, 0.0_Watt, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{0}, -0_Watt, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{0}, -0._Watt, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{0}, -0.0_Watt, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{"1.5 Watt"}, 1.5_Watt, "");

        // Equality, inequality
        NS_TEST_EXPECT_MSG_EQ(Watt_t{10}, 10_Watt, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{-10}, -10_Watt, "");
        NS_TEST_EXPECT_MSG_EQ((Watt_t{10} != 10_Watt), false, "");
        NS_TEST_EXPECT_MSG_EQ((Watt_t{10} == 20.0_Watt), false, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ((Watt_t{10} == 10.0_Watt), true, "");  // NOLINT
        NS_TEST_EXPECT_MSG_EQ((Watt_t{10} != 20_Watt), true, "");

        // Comparison
        NS_TEST_EXPECT_MSG_LT(1_Watt, 2_Watt, "");
        NS_TEST_EXPECT_MSG_GT(2_Watt, 1_Watt, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1_Watt, 1_Watt, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1_Watt, 2_Watt, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_Watt, 1_Watt, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_Watt, 2_Watt, "");
        NS_TEST_EXPECT_MSG_LT(-2_Watt, 1_Watt, "");
        NS_TEST_EXPECT_MSG_LT(-2_Watt, -1_Watt, "");
        NS_TEST_EXPECT_MSG_GT(-1_Watt, -2_Watt, "");
        NS_TEST_EXPECT_MSG_GT(1_Watt, -2_Watt, "");
        NS_TEST_EXPECT_MSG_EQ((10_Watt < 20_Watt), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_Watt <= 20_Watt), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_Watt > 20_Watt), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_Watt >= 20_Watt), false, "");

        // Arithmetic
        NS_TEST_EXPECT_MSG_EQ((1_Watt + 2_Watt), 3_Watt, "");
        NS_TEST_EXPECT_MSG_EQ((3_Watt - 1_Watt), 2_Watt, "");
        NS_TEST_EXPECT_MSG_EQ((3_Watt - 9_Watt), -6_Watt, "");
        NS_TEST_EXPECT_MSG_EQ((5_Watt += 10_Watt), 15_Watt, "");
        NS_TEST_EXPECT_MSG_EQ((5_Watt -= 10_Watt), -5_Watt, "");
        NS_TEST_EXPECT_MSG_EQ(-8_Watt, (0_Watt - 8_Watt), "");

        // Utilities
        NS_TEST_EXPECT_MSG_EQ(Watt_t{123}.str(), "123.0 Watt", "");     // NOLINT
        NS_TEST_EXPECT_MSG_EQ(Watt_t{123}.str(false), "123.0Watt", ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(Watt_t{123.45}.str(), "123.5 Watt", "");  // NOLINT
        NS_TEST_EXPECT_MSG_EQ(Watt_t{100}.in_dBm(), 50.0, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{1.2345}.in_mWatt(), 1234.5, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(Watt_t{123.45}.in_Watt(), 123.45, "");  // NOLINT

        TestInputOperatorPositives<Watt_t>({"12.3Watt", "12.3 Watt", " 12.3  Watt "});
        TestInputOperatorNegatives<Watt_t>(
            {"12.3watt", "12.3W", "12.3_Watt", "12.3Watt_t", "12.3", "12.3mWatt"});
    }

    /// Test dBm_t
    void Unit_dBm() // NOLINT
    {
        // Notations
        NS_TEST_EXPECT_MSG_EQ(dBm_t{0}, dBm_t{0.}, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{0}, dBm_t{0.0}, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{0}, dBm_t{-0}, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{0}, 0_dBm, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{0}, 0._dBm, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{0}, 0.0_dBm, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{0}, -0_dBm, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{0}, -0._dBm, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{0}, -0.0_dBm, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{"1.5 dBm"}, 1.5_dBm, "");

        // Equality, inequality

        NS_TEST_EXPECT_MSG_EQ(dBm_t{10}, 10_dBm, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{-10}, -10_dBm, "");
        NS_TEST_EXPECT_MSG_EQ((dBm_t{10} != 10_dBm), false, "");
        NS_TEST_EXPECT_MSG_EQ((dBm_t{10} == 20.0_dBm), false, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ((dBm_t{10} != 20_dBm), true, "");

        // Comparison
        NS_TEST_EXPECT_MSG_LT(1_dBm, 2_dBm, "");
        NS_TEST_EXPECT_MSG_GT(2_dBm, 1_dBm, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1_dBm, 1_dBm, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1_dBm, 2_dBm, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_dBm, 1_dBm, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_dBm, 2_dBm, "");
        NS_TEST_EXPECT_MSG_LT(-1_dBm, 2_dBm, "");
        NS_TEST_EXPECT_MSG_GT(2_dBm, -1_dBm, "");
        NS_TEST_EXPECT_MSG_EQ((10_dBm < 20_dBm), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_dBm <= 20_dBm), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_dBm > 20_dBm), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_dBm >= 20_dBm), false, "");

        // Arithmetic
#if false // IMPLEMENTED BUT NOT ALLOWED TO BE USED
        NS_TEST_EXPECT_MSG_EQ_TOL((20_dBm + 20_dBm).val, (23.0102999566_dBm).val, 1e-4, "");
        NS_TEST_EXPECT_MSG_EQ_TOL((20_dBm += 20_dBm).val, (23.0102999566_dBm).val, 1e-4, "");
        NS_TEST_EXPECT_MSG_EQ_TOL((20_dBm - 10_dBm).val, (19.5424250944_dBm).val, 1e-4, "");
        NS_TEST_EXPECT_MSG_EQ_TOL((20_dBm -= 10_dBm).val, (19.5424250944_dBm).val, 1e-4, "");
        NS_TEST_EXPECT_MSG_EQ_TOL((20_dBm - 19_dBm).val, (13.1317467562_dBm).val, 1e-4, "");
#endif    // IMPLEMENTED BUT NOT ALLOWED TO BE USED

        // Utilities
        NS_TEST_EXPECT_MSG_EQ(dBm_t{123}.str(), "123.0 dBm", "");     // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_t{123}.str(false), "123.0dBm", ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_t{123.45}.str(), "123.5 dBm", "");  // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_t{20}.in_mWatt(), 100.0, "");
        // Need tolerance due to math precision error on M1 Ultra with --ffast-math
        NS_TEST_EXPECT_MSG_EQ_TOL(dBm_t{20}.in_Watt(), 0.1, 1e-10, "");
        NS_TEST_EXPECT_MSG_EQ(dBm_t{123.45}.in_dBm(), 123.45, ""); // NOLINT

        TestInputOperatorPositives<dBm_t>({"12.3dBm", "12.3 dBm", " 12.3  dBm "});
        TestInputOperatorNegatives<dBm_t>(
            {"12.3DBM", "12.3dbm", "12.3_dBm", "12.3dBm_t", "12.3", "12.3Watt"});
    }

    /// Test dBm_t and dB_t operations
    void Unit_dBm_and_dB() // NOLINT
    {
        NS_TEST_EXPECT_MSG_EQ((10_dBm + 20_dB), 30_dBm, "");
        NS_TEST_EXPECT_MSG_EQ((10_dBm - 20_dB), -10_dBm, "");
        NS_TEST_EXPECT_MSG_EQ((10_dB + 20_dBm), 30_dBm, "");  // Commutativity
        NS_TEST_EXPECT_MSG_EQ((10_dB - 20_dBm), -10_dBm, ""); // Commutativity
    }

    /// Test mWatt_t and Watt_t operations
    void Unit_mWatt_and_Watt() // NOLINT
    {
        // Equality, inequality
        NS_TEST_EXPECT_MSG_EQ(mWatt_t{0}, 0_Watt, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t{10}, 10000_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((Watt_t{1} != 1000_mWatt), false, "");
        NS_TEST_EXPECT_MSG_EQ((Watt_t{2} == 1000_mWatt), false, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ((mWatt_t{1} == 0.001_Watt), true, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ((mWatt_t{10} != 20_Watt), true, "");

        // Comparison
        NS_TEST_EXPECT_MSG_LT(1_mWatt, 2_Watt, "");
        NS_TEST_EXPECT_MSG_GT(2_mWatt, 0.001_Watt, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(1000_mWatt, 1_Watt, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_mWatt, 0.001_Watt, "");
        NS_TEST_EXPECT_MSG_EQ((10_mWatt < 20_Watt), true, "");
        NS_TEST_EXPECT_MSG_EQ((2000_mWatt <= 2_Watt), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_mWatt > 10_Watt), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_mWatt >= 20_Watt), false, "");
        NS_TEST_EXPECT_MSG_LT(1_Watt, 2000_mWatt, "");
        NS_TEST_EXPECT_MSG_GT(2_Watt, 0.001_mWatt, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(0.001_Watt, 1_mWatt, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(2_Watt, 2_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((0.1_Watt < 200_mWatt), true, "");
        NS_TEST_EXPECT_MSG_EQ((2_Watt <= 2000_mWatt), true, "");
        NS_TEST_EXPECT_MSG_EQ((0.1_Watt > 100_mWatt), false, "");
        NS_TEST_EXPECT_MSG_EQ((1_Watt >= 2000_mWatt), false, "");

        // Arithmetic
        NS_TEST_EXPECT_MSG_EQ((1_mWatt + 2_Watt), 2001_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((3_mWatt - 0.001_Watt), 2_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((5_mWatt += 0.01_Watt), 15_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((5_mWatt -= 0.002_Watt), 3_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ(8_mWatt, (8_mWatt - 0_Watt), "");
        NS_TEST_EXPECT_MSG_EQ((1_Watt + 2_mWatt), 1002_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((0.03_Watt - 1_mWatt), 29_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((1_Watt += 0.01_Watt), 1.01_Watt, "");
        NS_TEST_EXPECT_MSG_EQ((4_Watt -= 0.2_Watt), 3.8_Watt, "");
        NS_TEST_EXPECT_MSG_EQ(8_Watt, (8_Watt - 0_mWatt), "");
    }

    /// Test conversions of physical units
    void Conversion() // NOLINT
    {
        // dBm_t-mWatt_t
        NS_TEST_EXPECT_MSG_EQ(20_dBm, (100_mWatt).to_dBm(), "");
        NS_TEST_EXPECT_MSG_EQ(20_dBm, dBm_t::from_mWatt(100_mWatt), "");
        NS_TEST_EXPECT_MSG_EQ((20_dBm).to_mWatt(), 100_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ(mWatt_t::from_dBm(20_dBm), 100_mWatt, "");

        // dBm_t-Watt_t
        NS_TEST_EXPECT_MSG_EQ(10_dBm, (0.01_Watt).to_dBm(), "");
        NS_TEST_EXPECT_MSG_EQ(10_dBm, dBm_t::from_Watt(0.01_Watt), "");
        NS_TEST_EXPECT_MSG_EQ((10_dBm).to_Watt(), 0.01_Watt, "");
        NS_TEST_EXPECT_MSG_EQ(Watt_t::from_dBm(10_dBm), 0.01_Watt, "");

        // Watt_t-mWatt_t
        NS_TEST_EXPECT_MSG_EQ(0.1_Watt, (100_mWatt).to_Watt(), "");
        NS_TEST_EXPECT_MSG_EQ(0.1_Watt, Watt_t::from_mWatt(100_mWatt), "");
        NS_TEST_EXPECT_MSG_EQ((0.1_Watt).to_mWatt(), 100_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ(mWatt_t::from_Watt(0.1_Watt), 100_mWatt, "");
    }

    /// Test Hz_t
    void Unit_Hz() // NOLINT
    {
        NS_TEST_EXPECT_MSG_EQ(Hz_t{123.}, 123_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t{123.45}, 123.45_Hz, "");
        NS_TEST_EXPECT_MSG_EQ((-123_Hz), Hz_t{-123}, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t{123000.}, 123_kHz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t{123000000.}, 123_MHz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t{123000000000.}, 123_GHz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t{123000000000000.}, 123_THz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t{123000000000000.}, 123000000000000_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t{"123 Hz"}, 123_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t{"123.45Hz"}, 123.45_Hz, "");

        NS_TEST_EXPECT_MSG_EQ(10_Hz + 20_Hz, 30_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(10_MHz - 20_MHz, -10_MHz, "");
        NS_TEST_EXPECT_MSG_EQ((10_MHz - 20_MHz != 40_MHz), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_MHz - 20_MHz == 40_MHz), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_kHz < 20_kHz), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_kHz <= 20_kHz), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_kHz <= 10_kHz), true, "");

        NS_TEST_EXPECT_MSG_EQ((10_kHz > 20_kHz), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_kHz >= 20_kHz), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_kHz >= 10_kHz), true, "");

        NS_TEST_EXPECT_MSG_EQ((10_Hz += 100_Hz), 110_Hz, "");
        NS_TEST_EXPECT_MSG_EQ((10_Hz -= 100_Hz), -90_Hz, "");
        NS_TEST_EXPECT_MSG_EQ((1_kHz / 4), 250_Hz, "");
        NS_TEST_EXPECT_MSG_EQ((1_kHz / 4_Hz), 250.0, "");
        NS_TEST_EXPECT_MSG_EQ((1_kHz * 4), 4_kHz, "");
        NS_TEST_EXPECT_MSG_EQ((4 * 1_kHz), 4_kHz, "");
        NS_TEST_EXPECT_MSG_EQ((1_Hz * MilliSeconds(1)), 0.001, "");
        NS_TEST_EXPECT_MSG_EQ((1_kHz * MilliSeconds(1)), 1.0, "");
        NS_TEST_EXPECT_MSG_EQ((1_MHz * MilliSeconds(1)), 1000.0, "");
        NS_TEST_EXPECT_MSG_EQ((MilliSeconds(1) * 1_MHz), 1000.0, "");
        NS_TEST_EXPECT_MSG_EQ((MilliSeconds(1) * 1_kHz), 1.0, "");
        NS_TEST_EXPECT_MSG_EQ((MilliSeconds(1) * 1_Hz), 0.001, "");

        NS_TEST_EXPECT_MSG_EQ((123_Hz).str(), "123 Hz", "");
        NS_TEST_EXPECT_MSG_EQ((123_Hz).str(false), "123Hz", "");
        NS_TEST_EXPECT_MSG_EQ((123_kHz).str(), "123 kHz", "");
        NS_TEST_EXPECT_MSG_EQ((123_MHz).str(), "123 MHz", "");
        NS_TEST_EXPECT_MSG_EQ((123_GHz).str(), "123 GHz", "");
        NS_TEST_EXPECT_MSG_EQ((123_THz).str(), "123 THz", "");
        NS_TEST_EXPECT_MSG_EQ((123000_THz).str(), "123000 THz", "");

        NS_TEST_EXPECT_MSG_EQ((123_GHz).in_Hz(), 123000000000, "");
        NS_TEST_EXPECT_MSG_EQ((123_GHz).in_kHz(), 123000000.0, "");
        NS_TEST_EXPECT_MSG_EQ((123_GHz).in_MHz(), 123000.0, "");
        NS_TEST_EXPECT_MSG_EQ((123.45e6_kHz).in_Hz(), 123450000000, "");
        NS_TEST_EXPECT_MSG_EQ((123.45e6_kHz).in_kHz(), 123450000, "");
        NS_TEST_EXPECT_MSG_EQ((123.45e6_kHz).in_MHz(), 123450, "");
        NS_TEST_EXPECT_MSG_EQ((123.456789e6_kHz).in_MHz(), 123456.789, "");

        NS_TEST_EXPECT_MSG_EQ(kHz_t{123.4}, 123.4_kHz, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{123.4}, 123.4_MHz, "");
        NS_TEST_EXPECT_MSG_EQ(GHz_t{123.4}, 123.4_GHz, "");
        NS_TEST_EXPECT_MSG_EQ(THz_t{123.4}, 123.4_THz, "");
        NS_TEST_EXPECT_MSG_EQ(kHz_t{123}, 123_kHz, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{123}, 123_MHz, "");
        NS_TEST_EXPECT_MSG_EQ(GHz_t{123}, 123_GHz, "");
        NS_TEST_EXPECT_MSG_EQ(THz_t{123}, 123_THz, "");

        NS_TEST_EXPECT_MSG_EQ(kHz_t{123.4}, 123400_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{123.4}, 123400000_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(GHz_t{123.4}, 123400000000_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(THz_t{123.4}, 123400000000000_Hz, "");

        // Conversion from string
        NS_TEST_EXPECT_MSG_EQ(Hz_t::from_str("3.14Hz").value(), 3.14_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t::from_str("3.14 Hz").value(), 3.14_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t::from_str("  3.14  Hz  ").value(), 3.14_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t::from_str("-3.14Hz").value(), -3.14_Hz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t::from_str("3.14 hz").has_value(), false, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t::from_str("3.14_Hz").has_value(), false, "");

        NS_TEST_EXPECT_MSG_EQ(Hz_t::from_str("3.14kHz").value(), 3.14_kHz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t::from_str("3.14MHz").value(), 3.14_MHz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t::from_str("3.14GHz").value(), 3.14_GHz, "");
        NS_TEST_EXPECT_MSG_EQ(Hz_t::from_str("3.14THz").value(), 3.14_THz, "");

        TestInputOperatorPositives<Hz_t>({"12.3Hz", "12.3 Hz", " 12.3  Hz "});
        TestInputOperatorNegatives<Hz_t>(
            {"12.3hz", "12.3HZ", "12.3hZ", "12.3_MHz", "12.3MHz_t", "12.3", "12.3dBm"});
        TestInputOperatorPositives<kHz_t>({"12.3kHz", "12.3 kHz"});
        TestInputOperatorNegatives<kHz_t>({"12.3khz", "12.3KHZ"});
        TestInputOperatorPositives<MHz_t>({"12.3MHz", "12.3 MHz"});
        TestInputOperatorNegatives<MHz_t>({"12.3Mhz", "12.3MHZ"});
        TestInputOperatorPositives<GHz_t>({"12.3GHz", "12.3 GHz"});
        TestInputOperatorNegatives<GHz_t>({"12.3Ghz", "12.3GHZ"});
        TestInputOperatorPositives<THz_t>({"12.3THz", "12.3 THz"});
        TestInputOperatorNegatives<THz_t>({"12.3Thz", "12.3THZ"});

        NS_TEST_EXPECT_MSG_EQ(MHz_t::from_str("3.14 MHz").value(), 3.14_MHz, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{"123MHz"}, 123_MHz, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{"123.45 MHz"}, 123.45_MHz, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{0.5}.IsMultipleOf(0.1_MHz), true, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{0.5}.IsMultipleOf(0.15_MHz), false, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{20}.IsMultipleOf(5_MHz), true, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{20}.IsMultipleOf(20_MHz), true, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{80}.IsMultipleOf(20_MHz), true, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{80}.IsMultipleOf(40_MHz), true, "");
        NS_TEST_EXPECT_MSG_EQ(MHz_t{80}.IsMultipleOf(21_MHz), false, "");

        TestInputOperatorPositives<MHz_t>({"12.3MHz", "12.3 MHz", " 12.3  MHz "});
        TestInputOperatorNegatives<MHz_t>({"12.3mhz",
                                           "12.3Mhz",
                                           "12.3mHz",
                                           "12.3MHZ",
                                           "12.3_MHz",
                                           "12.3MHz_t",
                                           "12.3",
                                           "12.3dBm"});
    }

    /// Test mWatt and unitless value operations
    void Unit_mWatt_and_double() // NOLINT
    {
        // Arithmetic
        NS_TEST_EXPECT_MSG_EQ((1_mWatt * 2.0), 2_mWatt, "");
        NS_TEST_EXPECT_MSG_EQ((1_mWatt / 2.0), 0.5_mWatt, "");
    }

    /// Test unitless value and mWatt operations
    void Unit_double_and_mWatt() // NOLINT
    {
        // Arithmetic
        NS_TEST_EXPECT_MSG_EQ((2.0 * 1_mWatt), 2_mWatt, "");
    }

    /// Test dBm_per_Hz_t
    void Unit_dBm_per_Hz() // NOLINT
    {
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{-43.21}, -43.21_dBm_per_Hz, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{"1.5 dBm/Hz"}, 1.5_dBm_per_Hz, "");


        // Utilities
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{123}.val, 123.0, "");                // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{123}.str(), "123.0 dBm/Hz", "");     // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{123}.str(false), "123.0dBm/Hz", ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{123.45}.val, 123.45, "");            // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{123.45}.str(), "123.5 dBm/Hz", "");  // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{123.45}.in_dBm(), 123.45, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{123}, 123_dBm_per_Hz, ""); // NOLINT

        NS_TEST_EXPECT_MSG_EQ_TOL(dBm_per_Hz_t{-80.0}.OverBandwidth(2_Hz), -77_dBm, 0.1_dB, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ_TOL(dBm_per_Hz_t{-80.0}.OverBandwidth(0.5_Hz), -83_dBm, 0.1_dB, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{-80.0}.OverBandwidth(1_MHz), -20_dBm, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t{-80.0}.OverBandwidth(100_kHz), -30_dBm, ""); // NOLINT

        NS_TEST_EXPECT_MSG_EQ(dBm_per_Hz_t::AveragePsd(-20_dBm, 1_MHz),
                              dBm_per_Hz_t{-80}, ""); // NOLINT

        TestInputOperatorPositives<dBm_per_Hz_t>({"12.3dBm/Hz", "12.3 dBm/Hz", " 12.3  dBm/Hz "});
        TestInputOperatorNegatives<dBm_per_Hz_t>(
            {"12.3dbm/Hz", "12.3dBm/hz", "12.3dBm_per_Hz", "12.3", "12.3dBm"});
    }

    /// Test dBm_per_MHz_t
    void Unit_dBm_per_MHz() // NOLINT
    {
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{-43.21}, -43.21_dBm_per_MHz, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{"1.5 dBm/MHz"}, 1.5_dBm_per_MHz, "");

        // Utilities
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{123}.val, 123.0, "");                 // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{123}.str(), "123.0 dBm/MHz", "");     // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{123}.str(false), "123.0dBm/MHz", ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{123.45}.val, 123.45, "");             // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{123.45}.str(), "123.5 dBm/MHz", "");  // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{123.45}.in_dBm(), 123.45, "");              // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{123}, 123_dBm_per_MHz, ""); // NOLINT


        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t::AveragePsd(-20_dBm, 1_MHz),
                              dBm_per_MHz_t{-20}, ""); // NOLINT

        NS_TEST_EXPECT_MSG_EQ_TOL(dBm_per_MHz_t{-80.0}.OverBandwidth(2_MHz), -77_dBm, 0.1_dB, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ_TOL(dBm_per_MHz_t{-80.0}.OverBandwidth(500_kHz), -83_dBm, 0.1_dB, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{-80.0}.OverBandwidth(1_MHz), -80_dBm, ""); // NOLINT
        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t{-80.0}.OverBandwidth(100_kHz), -90_dBm, ""); // NOLINT

        NS_TEST_EXPECT_MSG_EQ(dBm_per_MHz_t::AveragePsd(-20_dBm, 1_MHz),
                              dBm_per_MHz_t{-20}, "");                                        // NOLINT

        TestInputOperatorPositives<dBm_per_MHz_t>(
            {"12.3dBm/MHz", "12.3 dBm/MHz", " 12.3  dBm/MHz "});
        TestInputOperatorNegatives<dBm_per_MHz_t>(
            {"12.3dbm/MHz", "12.3dBm/mhz", "12.3dBm_per_MHz", "12.3", "12.3dBm/Hz"});
    }

    /// Test vector conversions
    void Vectors()
    {
        { // dB_t Empty vector
            std::vector<double> tvs;
            auto got1 = dB_t::from_doubles(tvs);
            auto got2 = dB_t::to_doubles(got1);
            auto got3 = dB_t::from_doubles(got2);
            NS_TEST_EXPECT_MSG_EQ((tvs == got2), true, "vector of double's do not match");
            NS_TEST_EXPECT_MSG_EQ((got1 == got3), true, "vector of dB_t's do not match");
            for (size_t idx = 0; idx < tvs.size(); ++idx)
            {
                NS_TEST_EXPECT_MSG_EQ(got1[idx].val, tvs[idx], "");
            }
        }

        std::vector<double> tvs = {0.1, -0.2, 1.3, -4.5, 5.6e7, -8e-9};
        { // dB_t
            auto got1 = dB_t::from_doubles(tvs);
            auto got2 = dB_t::to_doubles(got1);
            auto got3 = dB_t::from_doubles(got2);
            NS_TEST_EXPECT_MSG_EQ((tvs == got2), true, "vector of double's do not match");
            NS_TEST_EXPECT_MSG_EQ((got1 == got3), true, "vector of dB_t's do not match");
            for (size_t idx = 0; idx < tvs.size(); ++idx)
            {
                NS_TEST_EXPECT_MSG_EQ(got1[idx].val, tvs[idx], "");
            }
        }

        { // dBm_t
            auto got1 = dBm_t::from_doubles(tvs);
            auto got2 = dBm_t::to_doubles(got1);
            auto got3 = dBm_t::from_doubles(got2);
            NS_TEST_EXPECT_MSG_EQ((tvs == got2), true, "vector of double's do not match");
            NS_TEST_EXPECT_MSG_EQ((got1 == got3), true, "vector of dBm_t's do not match");
            for (size_t idx = 0; idx < tvs.size(); ++idx)
            {
                NS_TEST_EXPECT_MSG_EQ(got1[idx].val, tvs[idx], "");
            }
        }

        { // mWatt_t
            auto got1 = mWatt_t::from_doubles(tvs);
            auto got2 = mWatt_t::to_doubles(got1);
            auto got3 = mWatt_t::from_doubles(got2);
            NS_TEST_EXPECT_MSG_EQ((tvs == got2), true, "vector of double's do not match");
            NS_TEST_EXPECT_MSG_EQ((got1 == got3), true, "vector of mWatt_t's do not match");
            for (size_t idx = 0; idx < tvs.size(); ++idx)
            {
                NS_TEST_EXPECT_MSG_EQ(got1[idx].val, tvs[idx], "");
            }
        }

        { // Watt_t
            auto got1 = Watt_t::from_doubles(tvs);
            auto got2 = Watt_t::to_doubles(got1);
            auto got3 = Watt_t::from_doubles(got2);
            NS_TEST_EXPECT_MSG_EQ((tvs == got2), true, "vector of double's do not match");
            NS_TEST_EXPECT_MSG_EQ((got1 == got3), true, "vector of Watt_t's do not match");
            for (size_t idx = 0; idx < tvs.size(); ++idx)
            {
                NS_TEST_EXPECT_MSG_EQ(got1[idx].val, tvs[idx], "");
            }
        }

        { // dBm_per_Hz_t
            auto got1 = dBm_per_Hz_t::from_doubles(tvs);
            auto got2 = dBm_per_Hz_t::to_doubles(got1);
            auto got3 = dBm_per_Hz_t::from_doubles(got2);
            NS_TEST_EXPECT_MSG_EQ((tvs == got2), true, "vector of double's do not match");
            NS_TEST_EXPECT_MSG_EQ((got1 == got3), true, "vector of dBm_per_Hz_t's do not match");
            for (size_t idx = 0; idx < tvs.size(); ++idx)
            {
                NS_TEST_EXPECT_MSG_EQ(got1[idx].val, tvs[idx], "");
            }
        }

        { // dBm_per_MHz_t
            auto got1 = dBm_per_MHz_t::from_doubles(tvs);
            auto got2 = dBm_per_MHz_t::to_doubles(got1);
            auto got3 = dBm_per_MHz_t::from_doubles(got2);
            NS_TEST_EXPECT_MSG_EQ((tvs == got2), true, "vector of double's do not match");
            NS_TEST_EXPECT_MSG_EQ((got1 == got3), true, "vector of dBm_per_MHz_t's do not match");
            for (size_t idx = 0; idx < tvs.size(); ++idx)
            {
                NS_TEST_EXPECT_MSG_EQ(got1[idx].val, tvs[idx], "");
            }
        }

        { // Hz_t
            std::vector<double> tvs = {1, -2, 3000, -4000000};
            auto got1 = Hz_t::from_doubles(tvs);
            auto got2 = Hz_t::to_doubles(got1);
            auto got3 = Hz_t::from_doubles(got2);
            NS_TEST_EXPECT_MSG_EQ((tvs == got2), true, "vector of double's do not match");
            NS_TEST_EXPECT_MSG_EQ((got1 == got3), true, "vector of Hz_t's do not match");
            for (size_t idx = 0; idx < tvs.size(); ++idx)
            {
                NS_TEST_EXPECT_MSG_EQ(got1[idx].val, tvs[idx], "");
            }
        }
    }

    /// Test nSEC_t
    void Unit_nsec() // NOLINT(readability-identifier-naming)
    {
        NS_TEST_EXPECT_MSG_EQ(nSEC_t{123}, 123_nSEC, "");
        NS_TEST_EXPECT_MSG_EQ(nSEC_t{123000}, 123_uSEC, "");
        NS_TEST_EXPECT_MSG_EQ(nSEC_t{123000000}, 123_mSEC, "");
        NS_TEST_EXPECT_MSG_EQ(nSEC_t{123000000000}, 123_SEC, "");
        NS_TEST_EXPECT_MSG_EQ(nSEC_t{123000000000000}, 123000_SEC, "");

        NS_TEST_EXPECT_MSG_EQ(10_nSEC + 20_nSEC, 30_nSEC, "");
        NS_TEST_EXPECT_MSG_EQ(10_mSEC - 20_mSEC, -10_mSEC, "");
        NS_TEST_EXPECT_MSG_EQ((10_mSEC - 20_mSEC != 40_mSEC), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_mSEC - 20_mSEC == 40_mSEC), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_uSEC < 20_uSEC), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_uSEC <= 20_uSEC), true, "");
        NS_TEST_EXPECT_MSG_EQ((10_uSEC <= 10_uSEC), true, "");
        NS_TEST_EXPECT_MSG_EQ((20_nSEC + 10_SEC), 10000000020_nSEC, "");
        NS_TEST_EXPECT_MSG_EQ((20_nSEC - 10_SEC), -9999999980_nSEC, "");

        NS_TEST_EXPECT_MSG_EQ((10_uSEC > 20_uSEC), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_uSEC >= 20_uSEC), false, "");
        NS_TEST_EXPECT_MSG_EQ((10_uSEC >= 10_uSEC), true, "");

        NS_TEST_EXPECT_MSG_EQ((10_nSEC += 100_nSEC), 110_nSEC, "");
        NS_TEST_EXPECT_MSG_EQ((10_nSEC -= 100_nSEC), -90_nSEC, "");

        NS_TEST_EXPECT_MSG_EQ((123_nSEC).str(), "123 nSEC", "");
        NS_TEST_EXPECT_MSG_EQ((123_nSEC).str(false), "123nSEC", "");
        NS_TEST_EXPECT_MSG_EQ((123_uSEC).str(), "123 uSEC", "");
        NS_TEST_EXPECT_MSG_EQ((123_mSEC).str(), "123 mSEC", "");
        NS_TEST_EXPECT_MSG_EQ((123_SEC).str(), "123 SEC", "");
        NS_TEST_EXPECT_MSG_EQ(nSEC_t{123456789}.in_usec(), 123456, "");
        NS_TEST_EXPECT_MSG_EQ(nSEC_t{123456789}.in_msec(), 123, "");
        NS_TEST_EXPECT_MSG_EQ(nSEC_t{123456789}.in_sec(), 0, "");
    }

    /// Test percent_t
    void Unit_percent() // NOLINT(readability-identifier-naming)
    {
        // Equality
        NS_TEST_EXPECT_MSG_EQ(percent_t{}.val, 0., "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{0.}.val, 0., "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{31}.val, 31, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{31.4}.val, 31.4, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{31.41592}.val, 31.41592, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{314.1}.val, 314.1, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{-31.4}.val, -31.4, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{3.14}, percent_t{3.14}, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{3.14}, 3.14_percent, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t::from_ratio(0.125), percent_t{12.5}, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{12.5}.to_ratio(), 0.125, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{50}, 50_percent, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{"50.1 percent"}, 50.1_percent, "");

        // Arithmetic
        NS_TEST_EXPECT_MSG_EQ(percent_t{10} + percent_t{20}, percent_t{30}, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{20} + percent_t{10}, percent_t{30}, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{30} - percent_t{10}, percent_t{20}, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{10} - percent_t{30}, percent_t{-20}, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{10} * 10.0, percent_t{100}, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{10} * -10.0, percent_t{-100}, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t{10} / 10.0, percent_t{1}, "");
        NS_TEST_EXPECT_MSG_EQ(10.0 * percent_t{10}, percent_t{100}, "");

        // Comparison
        NS_TEST_EXPECT_MSG_LT(percent_t{10}, percent_t{20}, "");
        NS_TEST_EXPECT_MSG_LT(percent_t{-100}, percent_t{0}, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(percent_t{10}, percent_t{20}, "");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(percent_t{20}, percent_t{20}, "");
        NS_TEST_EXPECT_MSG_GT(percent_t{20}, percent_t{10}, "");
        NS_TEST_EXPECT_MSG_GT(percent_t{0}, percent_t{-100}, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(percent_t{20}, percent_t{10}, "");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(percent_t{20}, percent_t{20}, "");

        // Inequality
        NS_TEST_EXPECT_MSG_NE(percent_t{31.41592}.val, 31.41593, "");
        NS_TEST_EXPECT_MSG_NE(percent_t{31.4159265358979}, percent_t{31.4159265358978}, "");

        // Assignment
        percent_t got{20};
        got += percent_t{0.1};
        auto want = percent_t{20.1};
        NS_TEST_EXPECT_MSG_EQ(got, want, "");

        // String
        NS_TEST_EXPECT_MSG_EQ(got.str(), "20.1 %", "");
        NS_TEST_EXPECT_MSG_EQ(got.str(false), "20.1%", "");
        NS_TEST_EXPECT_MSG_EQ(percent_t::from_str("20.1 %").value(), got, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t::from_str("20.1%").value(), got, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t::from_str("20.1 percent").value(), got, "");
        NS_TEST_EXPECT_MSG_EQ(percent_t::from_str("20.1percent").value(), got, "");

        // Banned operations
        // NS_TEST_EXPECT_MSG_EQ(percent_t{30} * percent_t{10}, percent_t{3}, "");
        // NS_TEST_EXPECT_MSG_EQ(percent_t{30} / percent_t{10}, percent_t{300}, "");
    }

    void DoRun() override
    {
        Unit_degree();
        Unit_radian();

        Unit_dB();
        Unit_dBm();
        Unit_dBm_and_dB();
        Unit_mWatt();
        Unit_Watt();
        Unit_mWatt_and_Watt();
        Unit_mWatt_and_double();
        Unit_double_and_mWatt();
        Unit_dBm_per_Hz();
        Unit_dBm_per_MHz();
        Conversion();
        Unit_Hz();
        Unit_dBm_and_dB();
        Vectors();
        Unit_nsec();
        Unit_percent();
    }
};

/// A mock class with attributes
class AttributeMock : public Object
{
  public:
    /// Get the type ID.
    /// @return The type ID.
    static TypeId GetTypeId()
    {
        static TypeId tid = //
            TypeId("ns3:AttributeMock")
                .SetParent<Object>()
                .SetGroupName("AttributeMock")
                .AddConstructor<AttributeMock>()
                .AddAttribute("dB",
                              "help message for dB",
                              dBValue(0_dB),
                              MakedBAccessor(&AttributeMock::m_dB),
                              MakedBChecker())
                .AddAttribute("dBr",
                              "help message for dBr",
                              dBrValue(0_dBr),
                              MakedBrAccessor(&AttributeMock::m_dBr),
                              MakedBrChecker())
                .AddAttribute("dBm",
                              "help message for dBm",
                              dBmValue(20_dBm),
                              MakedBmAccessor(&AttributeMock::m_dBm),
                              MakedBmChecker())
                .AddAttribute("mWatt",
                              "help message for mWatt",
                              mWattValue(100_mWatt),
                              MakemWattAccessor(&AttributeMock::m_mWatt),
                              MakemWattChecker())
                .AddAttribute("Watt",
                              "help message for Watt",
                              WattValue(123_Watt),
                              MakeWattAccessor(&AttributeMock::m_Watt),
                              MakeWattChecker())
                .AddAttribute("dBm_per_Hz",
                              "help message for dBm_per_Hz",
                              dBm_per_HzValue(0.0004_dBm_per_Hz),
                              MakedBm_per_HzAccessor(&AttributeMock::m_dBm_per_Hz),
                              MakedBm_per_HzChecker())
                .AddAttribute("dBm_per_MHz",
                              "help message for dBm_per_MHz",
                              dBm_per_MHzValue(0.001_dBm_per_MHz),
                              MakedBm_per_MHzAccessor(&AttributeMock::m_dBm_per_MHz),
                              MakedBm_per_MHzChecker())
                .AddAttribute("Hz",
                              "help message for Hz",
                              HzValue(415000_Hz),
                              MakeHzAccessor(&AttributeMock::m_Hz),
                              MakeHzChecker())
                .AddAttribute("kHz",
                              "help message for kHz",
                              kHzValue(415_kHz),
                              MakekHzAccessor(&AttributeMock::m_kHz),
                              MakekHzChecker())
                .AddAttribute("MHz",
                              "help message for MHz",
                              MHzValue(300_MHz),
                              MakeMHzAccessor(&AttributeMock::m_MHz),
                              MakeMHzChecker())
                .AddAttribute("GHz",
                              "help message for GHz",
                              GHzValue(300_GHz),
                              MakeGHzAccessor(&AttributeMock::m_GHz),
                              MakeGHzChecker())
                .AddAttribute("THz",
                              "help message for THz",
                              THzValue(300_THz),
                              MakeTHzAccessor(&AttributeMock::m_THz),
                              MakeTHzChecker())
                .AddAttribute("nSEC",
                              "help message for nSEC",
                              nSECValue(-20_nSEC),
                              MakenSECAccessor(&AttributeMock::m_nSEC),
                              MakenSECChecker())
                .AddAttribute("degree",
                              "help message for degree",
                              degreeValue(720_degree),
                              MakedegreeAccessor(&AttributeMock::m_degree),
                              MakedegreeChecker())
                .AddAttribute("radian",
                              "help message for radian",
                              radianValue(20_radian),
                              MakeradianAccessor(&AttributeMock::m_radian),
                              MakeradianChecker())
                .AddAttribute("percent",
                              "help message for percent",
                              percentValue(20.5_percent),
                              MakepercentAccessor(&AttributeMock::m_percent),
                              MakepercentChecker())
                .AddAttribute("tuple1",
                              "a tuple of 1 entry setting with TupleValue",
                              TupleValue<dBmValue>(-1.5_dBm),
                              MakeTupleAccessor<dBmValue>(&AttributeMock::m_tuple1),
                              MakeTupleChecker<dBmValue>(MakedBmChecker()))
                .AddAttribute("tuple2",
                              "a tuple of 1 entry setting with StringValue",
                              StringValue("{-1.5dBm}"),
                              MakeTupleAccessor<dBmValue>(&AttributeMock::m_tuple2),
                              MakeTupleChecker<dBmValue>(MakedBmChecker()))
                .AddAttribute(
                    "tuple3",
                    "a tuple of 2 entries setting with TupleValue",
                    TupleValue<dBmValue, dBmValue>({-1.5_dBm, 1.5_dBm}),
                    MakeTupleAccessor<dBmValue, dBmValue>(&AttributeMock::m_tuple3),
                    MakeTupleChecker<dBmValue, dBmValue>(MakedBmChecker(), MakedBmChecker()))
                .AddAttribute(
                    "tuple4",
                    "a tuple of 2 entries setting with StringValue",
                    StringValue("{-1.5dBm, 1.5dBm}"),
                    MakeTupleAccessor<dBmValue, dBmValue>(&AttributeMock::m_tuple4),
                    MakeTupleChecker<dBmValue, dBmValue>(MakedBmChecker(), MakedBmChecker()));

        return tid;
    }

    dB_t m_dB{};                         ///< value of dB
    dBr_t m_dBr{};                       ///< value of dBr
    dBm_t m_dBm{};                       ///< value of dBm
    mWatt_t m_mWatt{};                   ///< value of mWatt
    Watt_t m_Watt{};                     ///< value of Watt
    dBm_per_Hz_t m_dBm_per_Hz{};         ///< value of dBm_per_Hz
    dBm_per_MHz_t m_dBm_per_MHz{};       ///< value of dBm_per_MHz
    Hz_t m_Hz{};                         ///< value of Hz
    kHz_t m_kHz{};                       ///< value of kHz
    GHz_t m_GHz{};                       ///< value of GHz
    THz_t m_THz{};                       ///< value of THz
    MHz_t m_MHz{};                       ///< value of MHz
    nSEC_t m_nSEC{};                     ///< value of nSEC
    degree_t m_degree{};                 ///< value of degree
    radian_t m_radian{};                 ///< value of radian
    percent_t m_percent{};               ///< value of percent
    std::tuple<dBm_t> m_tuple1{};        ///< tuple of dBm_t
    std::tuple<dBm_t> m_tuple2{};        ///< tuple of dBm_t
    std::tuple<dBm_t, dBm_t> m_tuple3{}; ///< tuple of dBm_t
    std::tuple<dBm_t, dBm_t> m_tuple4{}; ///< tuple of dBm_t
};

/// Test case for SiUnitsAttributes
class TestCaseSiUnitsAttributes : public TestCase
{
  public:
    TestCaseSiUnitsAttributes()
        : TestCase("SiUnitsAttributes") ///< Boilerplate code for TestCase
    {
    }

  private:
    void DoRun() override
    {
        auto mock = CreateObject<AttributeMock>();

        {
            auto want = 9_dB;
            mock->SetAttribute("dB", dBValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dB, want, "");
        }
        {
            auto want = 10_dB;
            mock->SetAttribute("dB", DoubleValue(10));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dB, want, "");
        }
        {
            auto want = 25_dBr;
            mock->SetAttribute("dBr", dBrValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBr, want, "");
        }
        {
            auto want = 5_dBr;
            mock->SetAttribute("dBr", DoubleValue(5));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBr, want, "");
        }
        {
            auto want = 20_dBm;
            mock->SetAttribute("dBm", dBmValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm, want, "");
        }
        {
            auto want = 10_dBm;
            mock->SetAttribute("dBm", DoubleValue(10));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm, want, "");
        }
        {
            auto want = 100_mWatt;
            mock->SetAttribute("mWatt", mWattValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_mWatt, want, "");
        }
        {
            auto want = 50_mWatt;
            mock->SetAttribute("mWatt", DoubleValue(50));
            NS_TEST_EXPECT_MSG_EQ(mock->m_mWatt, want, "");
        }
        {
            auto want = 221_Watt;
            mock->SetAttribute("Watt", WattValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_Watt, want, "");
        }
        {
            auto want = 0.0001_dBm_per_Hz;
            mock->SetAttribute("dBm_per_Hz", dBm_per_HzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm_per_Hz, want, "");
        }
        {
            auto want = 0.001_dBm_per_Hz;
            mock->SetAttribute("dBm_per_Hz", dBm_per_HzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm_per_Hz, want, "");
        }
        {
            auto want = 0.02_dBm_per_Hz;
            mock->SetAttribute("dBm_per_Hz", DoubleValue(0.02));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm_per_Hz, want, "");
        }
        {
            auto want = 0.001_dBm_per_MHz;
            mock->SetAttribute("dBm_per_MHz", dBm_per_MHzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm_per_MHz, want, "");
        }
        {
            auto want = 0.02_dBm_per_MHz;
            mock->SetAttribute("dBm_per_MHz", DoubleValue(0.02));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm_per_MHz, want, "");
        }
        {
            auto want = 365_Hz;
            mock->SetAttribute("Hz", HzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_Hz, want, "");
        }
        {
            auto want = 500_Hz;
            mock->SetAttribute("Hz", DoubleValue(500));
            NS_TEST_EXPECT_MSG_EQ(mock->m_Hz, want, "");
        }
        {
            auto want = 10_MHz;
            mock->SetAttribute("MHz", MHzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_MHz, want, "");
        }
        {
            auto want = 123.4_kHz;
            mock->SetAttribute("kHz", kHzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_kHz, want, "");
        }
        {
            auto want = 100_MHz;
            mock->SetAttribute("MHz", DoubleValue(100));
            NS_TEST_EXPECT_MSG_EQ(mock->m_MHz, want, "");
        }
        {
            auto want = 123.4_GHz;
            mock->SetAttribute("GHz", GHzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_GHz, want, "");
        }
        {
            auto want = 123.4_THz;
            mock->SetAttribute("THz", THzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_THz, want, "");
        }
        {
            auto want = 100_nSEC;
            mock->SetAttribute("nSEC", nSECValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_nSEC, want, "");
        }
        {
            auto want = 720_degree;
            mock->SetAttribute("degree", degreeValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_degree, want, "");
        }
        {
            auto want = 360_degree;
            mock->SetAttribute("degree", DoubleValue(360));
            NS_TEST_EXPECT_MSG_EQ(mock->m_degree, want, "");
        }
        {
            auto want = 2.4_radian;
            mock->SetAttribute("radian", radianValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_radian, want, "");
        }
        {
            auto want = 2_radian;
            mock->SetAttribute("radian", DoubleValue(2));
            NS_TEST_EXPECT_MSG_EQ(mock->m_radian, want, "");
        }
        {
            auto want = 2.4_percent;
            mock->SetAttribute("percent", percentValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_percent, want, "");
        }
        {
            auto want = 5.9_percent;
            mock->SetAttribute("percent", DoubleValue(5.9));
            NS_TEST_EXPECT_MSG_EQ(mock->m_percent, want, "");
        }
        { // A tuple of single entry setting with TupleValue
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple1), -1.5_dBm, "");
            mock->SetAttribute("tuple1", TupleValue<dBmValue>(-15_dBm));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple1), -15_dBm, "");
        }
        { // A tuple of single entry setting with StringValue
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -1.5_dBm, "");

            // change value
            mock->SetAttribute("tuple2", StringValue("{-15dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -15_dBm, "");

            // space after opening brace
            mock->SetAttribute("tuple2", StringValue("{ -27dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -27_dBm, "");

            // space before closing brace
            mock->SetAttribute("tuple2", StringValue("{-35dBm }"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -35_dBm, "");

            // space combination of after opening brace and before closing brace
            mock->SetAttribute("tuple2", StringValue("{ -45dBm }"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -45_dBm, "");

            // space between the number and the SI unit
            mock->SetAttribute("tuple2", StringValue("{-15 dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -15_dBm, "");
        }
        { // A tuple of two entries setting with TupleValue
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple3), -1.5_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple3), 1.5_dBm, "");
            mock->SetAttribute("tuple3", TupleValue<dBmValue, dBmValue>({-15_dBm, 15_dBm}));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple3), -15_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple3), 15_dBm, "");
        }
        { // A tuple of two entries setting with StringValueStrong type StringValue attribute
            // tuple access
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple4), -1.5_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple4), 1.5_dBm, "");

            // no space before and after comma
            mock->SetAttribute("tuple4", StringValue("{-1dBm,1dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple4), -1_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple4), 1_dBm, "");

            // space after comma
            mock->SetAttribute("tuple4", StringValue("{-15dBm, 15dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple4), -15_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple4), 15_dBm, "");

            // space before comma
            mock->SetAttribute("tuple4", StringValue("{-25dBm , 25dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple4), -25_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple4), 25_dBm, "");

            // space between comma and between the number and the SI unit
            mock->SetAttribute("tuple4", StringValue("{-35 dBm , 35 dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple4), -35_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple4), 35_dBm, "");
        }
        { // Test cases crashing or failing: StringValue for TupleValue

            // // space before opening brace
            // mock->SetAttribute("tuple2", StringValue(" {-25dBm}"));
            // NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -25_dBm, "");

            // // space after closing brace
            // mock->SetAttribute("tuple2", StringValue("{-37dBm} "));
            // NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -37_dBm, "");
        }
    }
};

/// Test suite for SiUnits
class SiUnitsTestSuite : public TestSuite
{
  public:
    SiUnitsTestSuite()
        : TestSuite("si-units-test", Type::UNIT) ///< Add test cases
    {
        AddTestCase(new TestCaseSiUnits, TestCase::Duration::QUICK);
        AddTestCase(new TestCaseSiUnitsAttributes, TestCase::Duration::QUICK);
    }
};

static SiUnitsTestSuite g_siUnitsTestSuite; ///< Register the test suite
