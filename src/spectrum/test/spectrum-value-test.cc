/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/spectrum-converter.h"
#include "ns3/spectrum-test.h"
#include "ns3/spectrum-value.h"
#include "ns3/test.h"

#include <cmath>
#include <iostream>

using namespace ns3;

// NS_LOG_COMPONENT_DEFINE ("SpectrumValueTest");

#define TOLERANCE 1e-6

/**
 * @ingroup spectrum-tests
 *
 * @brief Spectrum Value Test
 */
class SpectrumModelTestCase : public TestCase
{
  public:
    /**
     * Constructor
     * @param a first SpectrumModel
     * @param b second SpectrumModel
     * @param name test name
     * @param expectOrthogonal whether the two models are expected to be orthogonal
     * @param expectAligned whether the two models are expected to be aligned
     */
    SpectrumModelTestCase(Ptr<SpectrumModel> a,
                          Ptr<SpectrumModel> b,
                          const std::string& name,
                          bool expectOrthogonal,
                          bool expectAligned);
    void DoRun() override;

  private:
    Ptr<SpectrumModel> m_a;  //!< first SpectrumModel
    Ptr<SpectrumModel> m_b;  //!< second SpectrumModel
    bool m_expectOrthogonal; //!< whether the two models are expected to be orthogonal
    bool m_expectAligned;    //!< whether the two models are expected to be aligned
};

SpectrumModelTestCase::SpectrumModelTestCase(Ptr<SpectrumModel> a,
                                             Ptr<SpectrumModel> b,
                                             const std::string& name,
                                             bool expectOrthogonal,
                                             bool expectAligned)
    : TestCase(name),
      m_a(a),
      m_b(b),
      m_expectOrthogonal(expectOrthogonal),
      m_expectAligned(expectAligned)
{
}

void
SpectrumModelTestCase::DoRun()
{
    NS_TEST_EXPECT_MSG_EQ(
        m_a->IsOrthogonal(*m_b),
        m_expectOrthogonal,
        (m_expectOrthogonal ? "Expected orthogonal models" : "Expected non-orthogonal models"));
    NS_TEST_EXPECT_MSG_EQ(
        m_a->IsAligned(*m_b),
        m_expectAligned,
        (m_expectAligned ? "Expected aligned models" : "Expected non-aligned models"));
}

/**
 * @ingroup spectrum-tests
 *
 * @brief Spectrum Value Test
 */
class SpectrumValueTestCase : public TestCase
{
  public:
    /**
     * Constructor
     * @param a first SpectrumValue
     * @param b second SpectrumValue
     * @param name test name
     */
    SpectrumValueTestCase(SpectrumValue a, SpectrumValue b, std::string name);
    ~SpectrumValueTestCase() override;
    void DoRun() override;

  private:
    /**
     * Check that two SpectrumValue are equal within a tolerance
     * @param x first SpectrumValue
     * @param y second SpectrumValue
     * @return true if the two values are within the tolerance
     */
    bool MoreOrLessEqual(SpectrumValue x, SpectrumValue y);

    SpectrumValue m_a; //!< first SpectrumValue
    SpectrumValue m_b; //!< second SpectrumValue
};

SpectrumValueTestCase::SpectrumValueTestCase(SpectrumValue a, SpectrumValue b, std::string name)
    : TestCase(name),
      m_a(a),
      m_b(b)
{
}

SpectrumValueTestCase::~SpectrumValueTestCase()
{
}

bool
SpectrumValueTestCase::MoreOrLessEqual(SpectrumValue x, SpectrumValue y)
{
    SpectrumValue z = x - y;
    return (Norm(z) < TOLERANCE);
}

void
SpectrumValueTestCase::DoRun()
{
    NS_TEST_ASSERT_MSG_SPECTRUM_MODEL_EQ_TOL(*m_a.GetSpectrumModel(),
                                             *m_b.GetSpectrumModel(),
                                             TOLERANCE,
                                             "");
    NS_TEST_ASSERT_MSG_SPECTRUM_VALUE_EQ_TOL(m_a, m_b, TOLERANCE, "");
}

/**
 * @ingroup spectrum-tests
 *
 * @brief Spectrum Value TestSuite
 */
class SpectrumValueTestSuite : public TestSuite
{
  public:
    SpectrumValueTestSuite();
};

SpectrumValueTestSuite::SpectrumValueTestSuite()
    : TestSuite("spectrum-value", Type::UNIT)
{
    // NS_LOG_INFO("creating SpectrumValueTestSuite");

    std::vector<double> freqs;

    for (int i = 1; i <= 5; i++)
    {
        freqs.push_back(i);
    }

    Ptr<SpectrumModel> f = Create<SpectrumModel>(freqs);

    SpectrumValue v1(f);
    SpectrumValue v2(f);
    SpectrumValue v3(f);
    SpectrumValue v4(f);
    SpectrumValue v5(f);
    SpectrumValue v6(f);
    SpectrumValue v7(f);
    SpectrumValue v8(f);
    SpectrumValue v9(f);
    SpectrumValue v10(f);

    double doubleValue;

    doubleValue = 1.12345600000000;

    v1[0] = 0.700539792840;
    v1[1] = -0.554277600423;
    v1[2] = 0.750309319469;
    v1[3] = -0.892299213192;
    v1[4] = 0.987045234885;

    v2[0] = 0.870441628737;
    v2[1] = 0.271419263880;
    v2[2] = 0.451557288312;
    v2[3] = 0.968992859395;
    v2[4] = -0.929186654705;

    v3[0] = 1.570981421577;
    v3[1] = -0.282858336543;
    v3[2] = 1.201866607781;
    v3[3] = 0.076693646203;
    v3[4] = 0.057858580180;

    v4[0] = -0.169901835897;
    v4[1] = -0.825696864302;
    v4[2] = 0.298752031158;
    v4[3] = -1.861292072588;
    v4[4] = 1.916231889590;

    v5[0] = 0.609778998275;
    v5[1] = -0.150441618292;
    v5[2] = 0.338807641695;
    v5[3] = -0.864631566028;
    v5[4] = -0.917149259846;

    v6[0] = 0.804809615846;
    v6[1] = -2.042145397125;
    v6[2] = 1.661603829438;
    v6[3] = -0.920852207053;
    v6[4] = -1.062267984465;

    v7[0] = 1.823995792840;
    v7[1] = 0.569178399577;
    v7[2] = 1.873765319469;
    v7[3] = 0.231156786808;
    v7[4] = 2.110501234885;

    v8[0] = -0.422916207160;
    v8[1] = -1.677733600423;
    v8[2] = -0.373146680531;
    v8[3] = -2.015755213192;
    v8[4] = -0.136410765115;

    v9[0] = 0.787025633505;
    v9[1] = -0.622706495860;
    v9[2] = 0.842939506814;
    v9[3] = -1.002458904856;
    v9[4] = 1.108901891403;

    v10[0] = 0.623557836569;
    v10[1] = -0.493368320987;
    v10[2] = 0.667858215604;
    v10[3] = -0.794244913190;
    v10[4] = 0.878579343459;

    SpectrumValue tv3(f);
    SpectrumValue tv4(f);
    SpectrumValue tv5(f);
    SpectrumValue tv6(f);

    tv3 = v1 + v2;
    tv4 = v1 - v2;
    tv5 = v1 * v2;
    tv6 = v1 / v2;

    AddTestCase(new SpectrumValueTestCase(tv3, v3, "tv3 = v1 + v2"), TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv4, v4, "tv4 = v1 - v2"), TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv5, v5, "tv5 = v1 * v2"), TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv6, v6, "tv6 = v1 div v2"), TestCase::Duration::QUICK);

    // std::cerr << v6 << std::endl;
    // std::cerr << tv6 << std::endl;

    tv3 = v1;
    tv4 = v1;
    tv5 = v1;
    tv6 = v1;

    tv3 += v2;
    tv4 -= v2;
    tv5 *= v2;
    tv6 /= v2;

    AddTestCase(new SpectrumValueTestCase(tv3, v3, "tv3 += v2"), TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv4, v4, "tv4 -= v2"), TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv5, v5, "tv5 *= v2"), TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv6, v6, "tv6 div= v2"), TestCase::Duration::QUICK);

    SpectrumValue tv7a(f);
    SpectrumValue tv8a(f);
    SpectrumValue tv9a(f);
    SpectrumValue tv10a(f);
    tv7a = v1 + doubleValue;
    tv8a = v1 - doubleValue;
    tv9a = v1 * doubleValue;
    tv10a = v1 / doubleValue;
    AddTestCase(new SpectrumValueTestCase(tv7a, v7, "tv7a = v1 + doubleValue"),
                TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv8a, v8, "tv8a = v1 - doubleValue"),
                TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv9a, v9, "tv9a = v1 * doubleValue"),
                TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv10a, v10, "tv10a = v1 div doubleValue"),
                TestCase::Duration::QUICK);

    SpectrumValue tv7b(f);
    SpectrumValue tv8b(f);
    SpectrumValue tv9b(f);
    SpectrumValue tv10b(f);
    tv7b = doubleValue + v1;
    tv8b = doubleValue - v1;
    tv9b = doubleValue * v1;
    tv10b = doubleValue / v1;
    AddTestCase(new SpectrumValueTestCase(tv7b, v7, "tv7b =  doubleValue + v1"),
                TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv8b, v8, "tv8b =  doubleValue - v1"),
                TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv9b, v9, "tv9b =  doubleValue * v1"),
                TestCase::Duration::QUICK);
    AddTestCase(new SpectrumValueTestCase(tv10b, v10, "tv10b = doubleValue div v1"),
                TestCase::Duration::QUICK);

    SpectrumValue v1ls3(f);
    SpectrumValue v1rs3(f);
    SpectrumValue tv1ls3(f);
    SpectrumValue tv1rs3(f);

    v1ls3[0] = v1[3];
    v1ls3[1] = v1[4];
    tv1ls3 = v1 << 3;
    AddTestCase(new SpectrumValueTestCase(tv1ls3, v1ls3, "tv1ls3 = v1 << 3"),
                TestCase::Duration::QUICK);

    v1rs3[3] = v1[0];
    v1rs3[4] = v1[1];
    tv1rs3 = v1 >> 3;
    AddTestCase(new SpectrumValueTestCase(tv1rs3, v1rs3, "tv1rs3 = v1 >> 3"),
                TestCase::Duration::QUICK);
}

/**
 * @ingroup spectrum-tests
 *
 * @brief Spectrum Converter TestSuite
 */
class SpectrumConverterTestSuite : public TestSuite
{
  public:
    SpectrumConverterTestSuite();
};

SpectrumConverterTestSuite::SpectrumConverterTestSuite()
    : TestSuite("spectrum-converter", Type::UNIT)
{
    double f;

    std::vector<double> f1;
    for (f = 3; f <= 7; f += 2)
    {
        f1.push_back(f);
    }
    auto sof1 = Create<SpectrumModel>(f1);

    std::vector<double> f2;
    for (f = 2; f <= 8; f += 1)
    {
        f2.push_back(f);
    }
    auto sof2 = Create<SpectrumModel>(f2);

    auto v1 = Create<SpectrumValue>(sof1);
    *v1 = 4;
    SpectrumConverter c12(sof1, sof2);
    auto res = c12.Convert(v1);
    SpectrumValue t12(sof2);
    t12 = 4;
    t12[0] = 2;
    t12[6] = 2;
    //   NS_LOG_LOGIC(*v1);
    //   NS_LOG_LOGIC(t12);
    //   NS_LOG_LOGIC(*res);

    AddTestCase(new SpectrumValueTestCase(t12, *res, ""), TestCase::Duration::QUICK);
    // TEST_ASSERT(MoreOrLessEqual(t12, *res));

    auto v2a = Create<SpectrumValue>(sof2);
    *v2a = -2;
    SpectrumConverter c21(sof2, sof1);
    res = c21.Convert(v2a);
    SpectrumValue t21a(sof1);
    t21a = -2;
    //   NS_LOG_LOGIC(*v2a);
    //   NS_LOG_LOGIC(t21a);
    //   NS_LOG_LOGIC(*res);
    AddTestCase(new SpectrumValueTestCase(t21a, *res, ""), TestCase::Duration::QUICK);
    // TEST_ASSERT(MoreOrLessEqual(t21a, *res));

    auto v2b = Create<SpectrumValue>(sof2);
    (*v2b)[0] = 3;
    (*v2b)[1] = 5;
    (*v2b)[2] = 1;
    (*v2b)[3] = 2;
    (*v2b)[4] = 4;
    (*v2b)[5] = 6;
    (*v2b)[6] = 3;
    res = c21.Convert(v2b);
    SpectrumValue t21b(sof1);
    t21b[0] = 3 * 0.25 + 5 * 0.5 + 1 * 0.25;
    t21b[1] = 1 * 0.25 + 2 * 0.5 + 4 * 0.25;
    t21b[2] = 4 * 0.25 + 6 * 0.5 + 3 * 0.25;
    //   NS_LOG_LOGIC(*v2b);
    //   NS_LOG_LOGIC(t21b);
    //   NS_LOG_LOGIC(*res);
    AddTestCase(new SpectrumValueTestCase(t21b, *res, ""), TestCase::Duration::QUICK);

    // orthogonal models
    std::vector<double> f3{10, 11, 12, 13, 14};
    auto sof3 = Create<SpectrumModel>(f3);
    AddTestCase(
        new SpectrumModelTestCase(sof1, sof3, "Check sof1 and sof3 are orthogonal", true, false),
        TestCase::Duration::QUICK);

    SpectrumConverter c13(sof1, sof3);
    res = c13.Convert(v1);
    SpectrumValue t13(sof3);
    t13 = 0;
    AddTestCase(
        new SpectrumValueTestCase(t13, *res, "Check spectrum conversion with orthogonal models"),
        TestCase::Duration::QUICK);

    // non-orthogonal and non-aligned models
    std::vector<double> f4{3, 4, 5, 6, 7};
    auto sof4 = Create<SpectrumModel>(f4);
    AddTestCase(new SpectrumModelTestCase(sof1,
                                          sof4,
                                          "Check sof1 and sof4 are not orthogonal nor aligned",
                                          false,
                                          false),
                TestCase::Duration::QUICK);

    SpectrumConverter c14(sof1, sof4);
    res = c14.Convert(v1);
    SpectrumValue t14(sof4);
    t14 = 4;
    t14[0] = 4;
    t14[1] = 4;
    t14[2] = 4;
    t14[3] = 4;
    t14[4] = 4;
    AddTestCase(new SpectrumValueTestCase(
                    t14,
                    *res,
                    "Check spectrum conversion with non-orthogonal and non-aligned models"),
                TestCase::Duration::QUICK);

    // aligned models
    std::vector<double> f5{5, 7, 9};
    auto sof5 = Create<SpectrumModel>(f5);
    AddTestCase(
        new SpectrumModelTestCase(sof1, sof5, "Check sof1 and sof5 are aligned", false, true),
        TestCase::Duration::QUICK);

    SpectrumConverter c15(sof1, sof5);
    res = c15.Convert(v1);
    SpectrumValue t15(sof5);
    t15 = 4;
    t15[0] = 4;
    t15[1] = 4;
    t15[2] = 0;
    AddTestCase(
        new SpectrumValueTestCase(t15, *res, "Check spectrum conversion with aligned models"),
        TestCase::Duration::QUICK);

    // aligned models with different band sizes
    auto sof6 = Create<SpectrumModel>(std::vector<double>{1, 2.5, 4});
    auto sof7 = Create<SpectrumModel>(std::vector<double>{2.5, 4, 5.5});

    AddTestCase(
        new SpectrumModelTestCase(sof6, sof7, "Check sof6 and sof7 are aligned", false, true),
        TestCase::Duration::QUICK);

    // models with different band sizes where only some bands are aligned
    auto sof8 = Create<SpectrumModel>(std::vector<double>{2.5, 3.5, 4.5, 5.5});
    AddTestCase(
        new SpectrumModelTestCase(sof6, sof8, "Check sof6 and sof8 are not aligned", false, false),
        TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static SpectrumValueTestSuite g_SpectrumValueTestSuite;
/// Static variable for test initialization
static SpectrumConverterTestSuite g_SpectrumConverterTestSuite;
