/*
 *   Copyright (c) 2020 University of Padova, Dep. of Information Engineering, SIGNET lab.
 *
 *   SPDX-License-Identifier: GPL-2.0-only
 */

#include "cmath"
#include "iostream"
#include "sstream"
#include "string"

#include "ns3/double.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/three-gpp-antenna-model.h"
#include "ns3/uinteger.h"
#include "ns3/uniform-planar-array.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TestUniformPlanarArray");

/**
 * @ingroup antenna-tests
 *
 * @brief UniformPlanarArray Test Case
 */
class UniformPlanarArrayTestCase : public TestCase
{
  public:
    /**
     * Generate a string containing all relevant parameters
     * @param element the antenna element
     * @param rows the number of rows
     * @param cols the number of columns
     * @param rowSpace the row spacing
     * @param colSpace the column spacing
     * @param alpha the bearing angle
     * @param beta the tilting angle
     * @param direction the direction
     * @return the string containing all relevant parameters
     */
    static std::string BuildNameString(Ptr<AntennaModel> element,
                                       uint32_t rows,
                                       uint32_t cols,
                                       double rowSpace,
                                       double colSpace,
                                       double alpha,
                                       double beta,
                                       Angles direction);
    /**
     * The constructor of the test case
     * @param element the antenna element
     * @param rows the number of rows
     * @param cols the number of columns
     * @param rowSpace the row spacing
     * @param colSpace the column spacing
     * @param alpha the bearing angle
     * @param beta the tilting angle
     * @param direction the direction
     * @param expectedGainDb the expected antenna gain [dB]
     */
    UniformPlanarArrayTestCase(Ptr<AntennaModel> element,
                               uint32_t rows,
                               uint32_t cols,
                               double rowSpace,
                               double colSpace,
                               double alpha,
                               double beta,
                               Angles direction,
                               double expectedGainDb);

  private:
    /**
     * Run the test
     */
    void DoRun() override;
    /**
     * Compute the gain of the antenna array
     * @param a the antenna array
     * @return the gain of the antenna array [dB]
     */
    double ComputeGain(Ptr<UniformPlanarArray> a);

    Ptr<AntennaModel> m_element; //!< the antenna element
    uint32_t m_rows;             //!< the number of rows
    uint32_t m_cols;             //!< the number of columns
    double m_rowSpace;           //!< the row spacing
    double m_colSpace;           //!< the column spacing
    double m_alpha;              //!< the bearing angle [rad]
    double m_beta;               //!< the titling angle [rad]
    Angles m_direction;          //!< the testing direction
    double m_expectedGain;       //!< the expected antenna gain [dB]
};

std::string
UniformPlanarArrayTestCase::BuildNameString(Ptr<AntennaModel> element,
                                            uint32_t rows,
                                            uint32_t cols,
                                            double rowSpace,
                                            double colSpace,
                                            double alpha,
                                            double beta,
                                            Angles direction)
{
    std::ostringstream oss;
    oss << "UPA=" << rows << "x" << cols << ", row spacing=" << rowSpace << "*lambda"
        << ", col spacing=" << colSpace << "*lambda, bearing=" << RadiansToDegrees(alpha) << " deg"
        << ", tilting=" << RadiansToDegrees(beta) << " deg"
        << ", element=" << element->GetInstanceTypeId().GetName() << ", direction=" << direction;
    return oss.str();
}

UniformPlanarArrayTestCase::UniformPlanarArrayTestCase(Ptr<AntennaModel> element,
                                                       uint32_t rows,
                                                       uint32_t cols,
                                                       double rowSpace,
                                                       double colSpace,
                                                       double alpha,
                                                       double beta,
                                                       Angles direction,
                                                       double expectedGainDb)
    : TestCase(BuildNameString(element, rows, cols, rowSpace, colSpace, alpha, beta, direction)),
      m_element(element),
      m_rows(rows),
      m_cols(cols),
      m_rowSpace(rowSpace),
      m_colSpace(colSpace),
      m_alpha(alpha),
      m_beta(beta),
      m_direction(direction),
      m_expectedGain(expectedGainDb)
{
}

double
UniformPlanarArrayTestCase::ComputeGain(Ptr<UniformPlanarArray> a)
{
    // compute gain
    PhasedArrayModel::ComplexVector sv = a->GetSteeringVector(m_direction);
    NS_TEST_EXPECT_MSG_EQ(sv.GetSize(), a->GetNumElems(), "steering vector of wrong size");
    PhasedArrayModel::ComplexVector bf = a->GetBeamformingVector(m_direction);
    NS_TEST_EXPECT_MSG_EQ(bf.GetSize(), a->GetNumElems(), "beamforming vector of wrong size");
    std::pair<double, double> fp = a->GetElementFieldPattern(m_direction);

    // scalar product dot (sv, bf)
    std::complex<double> prod{0};
    for (size_t i = 0; i < sv.GetSize(); i++)
    {
        prod += sv[i] * bf[i];
    }
    double bfGain = std::pow(std::abs(prod), 2);
    double bfGainDb = 10 * std::log10(bfGain);

    // power gain from two polarizations
    double elementPowerGain = std::pow(std::get<0>(fp), 2) + std::pow(std::get<1>(fp), 2);
    double elementPowerGainDb = 10 * std::log10(elementPowerGain);

    // sum BF and element gains
    return bfGainDb + elementPowerGainDb;
}

void
UniformPlanarArrayTestCase::DoRun()
{
    NS_LOG_FUNCTION(this << BuildNameString(m_element,
                                            m_rows,
                                            m_cols,
                                            m_rowSpace,
                                            m_colSpace,
                                            m_alpha,
                                            m_beta,
                                            m_direction));

    Ptr<UniformPlanarArray> a = CreateObject<UniformPlanarArray>();
    a->SetAttribute("AntennaElement", PointerValue(m_element));
    a->SetAttribute("NumRows", UintegerValue(m_rows));
    a->SetAttribute("NumColumns", UintegerValue(m_cols));
    a->SetAttribute("AntennaVerticalSpacing", DoubleValue(m_rowSpace));
    a->SetAttribute("AntennaHorizontalSpacing", DoubleValue(m_colSpace));
    a->SetAttribute("BearingAngle", DoubleValue(m_alpha));
    a->SetAttribute("DowntiltAngle", DoubleValue(m_beta));

    double actualGainDb = ComputeGain(a);
    NS_TEST_EXPECT_MSG_EQ_TOL(actualGainDb,
                              m_expectedGain,
                              0.001,
                              "wrong value of the radiation pattern");
}

/**
 * @ingroup antenna-tests
 *
 * @brief UpdateOnChange Test Case
 */
class UpdateOnChangeTestCase : public TestCase
{
  public:
    /**
     * The constructor of the test case
     * @param element the antenna element
     * @param name the test case name
     */
    UpdateOnChangeTestCase(Ptr<AntennaModel> element, std::string name)
        : TestCase(name),
          m_element(element)
    {
    }

  private:
    /**
     * Run the test
     */
    void DoRun() override;
    Ptr<AntennaModel> m_element; //!< the antenna element
};

void
UpdateOnChangeTestCase::DoRun()
{
    Ptr<UniformPlanarArray> ant = CreateObject<UniformPlanarArray>();
    ant->SetAttribute("AntennaElement", PointerValue(m_element));
    ant->SetAttribute("NumRows", UintegerValue(10));
    ant->SetAttribute("NumColumns", UintegerValue(10));
    ant->SetAttribute("AntennaVerticalSpacing", DoubleValue(0.5));
    ant->SetAttribute("AntennaHorizontalSpacing", DoubleValue(0.5));
    ant->SetAttribute("BearingAngle", DoubleValue(DegreesToRadians(0)));
    ant->SetAttribute("DowntiltAngle", DoubleValue(DegreesToRadians(45)));

    Ptr<UniformPlanarArray> ant2 = CreateObject<UniformPlanarArray>();
    ant2->SetAttribute("AntennaElement", PointerValue(m_element));
    ant2->SetAttribute("NumRows", UintegerValue(10));
    ant2->SetAttribute("NumColumns", UintegerValue(10));
    ant2->SetAttribute("AntennaVerticalSpacing", DoubleValue(0.5));
    ant2->SetAttribute("AntennaHorizontalSpacing", DoubleValue(0.5));
    ant2->SetAttribute("BearingAngle", DoubleValue(DegreesToRadians(0)));
    ant2->SetAttribute("DowntiltAngle", DoubleValue(DegreesToRadians(45)));

    NS_TEST_ASSERT_MSG_EQ(ant.operator bool(), true, "AntennaModel is not a PhasedArrayModel");

    // Initial state of array requires a channel update
    NS_TEST_ASSERT_MSG_EQ(ant->IsChannelOutOfDate(ant2),
                          true,
                          "Expecting update, since the pair was never setup");
    NS_TEST_ASSERT_MSG_EQ(
        ant2->IsChannelOutOfDate(ant),
        false,
        "Not expecting update, since the pair was just updated and no settings changed");
    ant->SetAlpha(DegreesToRadians(90));
    NS_TEST_ASSERT_MSG_EQ(ant2->IsChannelOutOfDate(ant),
                          true,
                          "Expecting update, antenna parameter changed");
    NS_TEST_ASSERT_MSG_EQ(
        ant->IsChannelOutOfDate(ant2),
        false,
        "Not expecting update, since the pair was just updated and no settings changed");
    ant->SetAlpha(DegreesToRadians(90));
    NS_TEST_ASSERT_MSG_EQ(ant->IsChannelOutOfDate(ant2),
                          true,
                          "Expecting update, antenna parameter changed");
}

/**
 * @ingroup antenna-tests
 *
 * @brief UniformPlanarArray Test Suite
 */
class UniformPlanarArrayTestSuite : public TestSuite
{
  public:
    UniformPlanarArrayTestSuite();
};

UniformPlanarArrayTestSuite::UniformPlanarArrayTestSuite()
    : TestSuite("uniform-planar-array-test", Type::UNIT)
{
    Ptr<AntennaModel> isotropic = CreateObject<IsotropicAntennaModel>();
    Ptr<AntennaModel> tgpp = CreateObject<ThreeGppAntennaModel>();

    //                                             element, rows, cols, rowSpace, colSpace, bearing,
    //                                             tilting,               direction (azimuth,
    //                                             inclination), expectedGainDb
    // Single element arrays: check if bearing/tilting works on antenna element
    AddTestCase(new UniformPlanarArrayTestCase(isotropic,
                                               1,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(0),
                                               DegreesToRadians(0),
                                               Angles(DegreesToRadians(0), DegreesToRadians(90)),
                                               0.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               1,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(0),
                                               DegreesToRadians(0),
                                               Angles(DegreesToRadians(0), DegreesToRadians(90)),
                                               8.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               1,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(90),
                                               DegreesToRadians(0),
                                               Angles(DegreesToRadians(90), DegreesToRadians(90)),
                                               8.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               1,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(-90),
                                               DegreesToRadians(0),
                                               Angles(DegreesToRadians(-90), DegreesToRadians(90)),
                                               8.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               1,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(180),
                                               DegreesToRadians(0),
                                               Angles(DegreesToRadians(180), DegreesToRadians(90)),
                                               8.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               1,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(-180),
                                               DegreesToRadians(0),
                                               Angles(DegreesToRadians(-180), DegreesToRadians(90)),
                                               8.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               1,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(0),
                                               DegreesToRadians(45),
                                               Angles(DegreesToRadians(0), DegreesToRadians(135)),
                                               8.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               1,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(0),
                                               DegreesToRadians(-45),
                                               Angles(DegreesToRadians(0), DegreesToRadians(45)),
                                               8.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               1,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(0),
                                               DegreesToRadians(90),
                                               Angles(DegreesToRadians(0), DegreesToRadians(180)),
                                               8.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               1,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(0),
                                               DegreesToRadians(-90),
                                               Angles(DegreesToRadians(0), DegreesToRadians(0)),
                                               8.0),
                TestCase::Duration::QUICK);

    // linear array
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               10,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(0),
                                               DegreesToRadians(0),
                                               Angles(DegreesToRadians(0), DegreesToRadians(90)),
                                               18.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               10,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(90),
                                               DegreesToRadians(0),
                                               Angles(DegreesToRadians(90), DegreesToRadians(90)),
                                               18.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               10,
                                               1,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(0),
                                               DegreesToRadians(45),
                                               Angles(DegreesToRadians(0), DegreesToRadians(135)),
                                               18.0),
                TestCase::Duration::QUICK);

    // planar array
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               10,
                                               10,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(0),
                                               DegreesToRadians(0),
                                               Angles(DegreesToRadians(0), DegreesToRadians(90)),
                                               28.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               10,
                                               10,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(90),
                                               DegreesToRadians(0),
                                               Angles(DegreesToRadians(90), DegreesToRadians(90)),
                                               28.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UniformPlanarArrayTestCase(tgpp,
                                               10,
                                               10,
                                               0.5,
                                               0.5,
                                               DegreesToRadians(0),
                                               DegreesToRadians(45),
                                               Angles(DegreesToRadians(0), DegreesToRadians(135)),
                                               28.0),
                TestCase::Duration::QUICK);
    AddTestCase(new UpdateOnChangeTestCase(tgpp,
                                           "Test IsChannelOutOfDate() and InvalidateChannels() for "
                                           "UniformPlanarArray with 3GPP antenna element"),
                TestCase::Duration::QUICK);
}

static UniformPlanarArrayTestSuite staticUniformPlanarArrayTestSuiteInstance;
