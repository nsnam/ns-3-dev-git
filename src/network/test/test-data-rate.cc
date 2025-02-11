/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Greg Steinbrecher <grs@fb.com>
 */

#include "ns3/data-rate.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/tuple.h"

using namespace ns3;

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Test Data rate
 *
 */
class DataRateTestCase : public TestCase
{
  public:
    /**
     * Constructor
     * @param name test name
     */
    DataRateTestCase(std::string name);
    ~DataRateTestCase() override;

    /**
     * Checks if two time values are equal
     * @param t1 first time to check
     * @param t2 second time to check
     * @param msg check output message
     */
    void CheckTimesEqual(Time t1, Time t2, const std::string msg);
    /**
     * Checks if two data rates values are equal
     * @param d1 first data rate to check
     * @param d2 second data rate to check
     * @param msg check output message
     */
    void CheckDataRateEqual(DataRate d1, DataRate d2, const std::string msg);

  protected:
    void DoRun() override = 0;
};

DataRateTestCase::DataRateTestCase(std::string name)
    : TestCase(name)
{
}

DataRateTestCase::~DataRateTestCase()
{
}

void
DataRateTestCase::CheckTimesEqual(Time actual, Time correct, const std::string msg)
{
    int64x64_t actualFemtos = actual.GetFemtoSeconds();
    int64x64_t correctFemtos = correct.GetFemtoSeconds();
    NS_TEST_EXPECT_MSG_EQ(actualFemtos, correctFemtos, msg);
}

void
DataRateTestCase::CheckDataRateEqual(DataRate d1, DataRate d2, const std::string msg)
{
    NS_TEST_EXPECT_MSG_EQ(d1, d2, msg);
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Test Data rate
 *
 */
class DataRateTestCase1 : public DataRateTestCase
{
  public:
    DataRateTestCase1();

    /**
     * Checks that a given number of bits, at a specified datarate, are
     * corresponding to a given time
     * @param rate the DataRate
     * @param nBits number of bits
     * @param correctTime expected time
     */
    void SingleTest(std::string rate, size_t nBits, Time correctTime);

  private:
    void DoRun() override;
};

DataRateTestCase1::DataRateTestCase1()
    : DataRateTestCase("Test rounding of conversion from DataRate to time")
{
}

void
DataRateTestCase1::SingleTest(std::string rate, size_t nBits, Time correctTime)
{
    DataRate dr(rate);
    Time bitsTime = dr.CalculateBitsTxTime(nBits);
    CheckTimesEqual(bitsTime, correctTime, "CalculateBitsTxTime returned incorrect value");
    if ((nBits % 8) == 0)
    {
        Time bytesTime = dr.CalculateBytesTxTime(nBits / 8);
        CheckTimesEqual(bytesTime, correctTime, "CalculateBytesTxTime returned incorrect value");
    }
}

void
DataRateTestCase1::DoRun()
{
    if (Time::GetResolution() != Time::FS)
    {
        Time::SetResolution(Time::FS);
    }
    SingleTest("1GB/s", 512, NanoSeconds(64));
    SingleTest("8Gb/s", 512, NanoSeconds(64));
    SingleTest("1Gb/s", 512, NanoSeconds(512));
    SingleTest("8GB/s", 512, NanoSeconds(8));
    size_t nBits;
    for (nBits = 0; nBits <= 512; nBits++)
    {
        SingleTest("1Mb/s", nBits, MicroSeconds(nBits));
        SingleTest("10Mb/s", nBits, NanoSeconds(nBits * 100));
        SingleTest("100Mb/s", nBits, NanoSeconds(nBits * 10));
        SingleTest("1Gb/s", nBits, NanoSeconds(nBits));
        SingleTest("10Gb/s", nBits, PicoSeconds(nBits * 100));
        SingleTest("25Gb/s", nBits, PicoSeconds(nBits * 40));
        SingleTest("40Gb/s", nBits, PicoSeconds(nBits * 25));
        SingleTest("100Gb/s", nBits, PicoSeconds(nBits * 10));
        SingleTest("200Gb/s", nBits, PicoSeconds(nBits * 5));
        SingleTest("400Gb/s", nBits, FemtoSeconds(nBits * 2500));
    }
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Test Data rate
 *
 */
class DataRateTestCase2 : public DataRateTestCase
{
  public:
    DataRateTestCase2();
    /**
     * Checks data rate addition
     * @param rate1 first data rate
     * @param rate2 second data rate
     * @param rate3 third data rate (first plus second)
     */
    void AdditionTest(std::string rate1, std::string rate2, std::string rate3);
    /**
     * Checks data rate subtraction
     * @param rate1 first data rate
     * @param rate2 second data rate
     * @param rate3 third data rate (first minus second)
     */
    void SubtractionTest(std::string rate1, std::string rate2, std::string rate3);
    /**
     * Checks data rate integer multiplication
     * @param rate1 first data rate
     * @param factor multiplication factor
     * @param rate2 second data rate  (first multiplied by factor)
     */
    void MultiplicationIntTest(std::string rate1, uint64_t factor, std::string rate2);
    /**
     * Checks data rate floating point multiplication
     * @param rate1 first data rate
     * @param factor multiplication factor
     * @param rate2 second data rate  (first multiplied by factor)
     */
    void MultiplicationDoubleTest(std::string rate1, double factor, std::string rate2);

  private:
    void DoRun() override;
};

DataRateTestCase2::DataRateTestCase2()
    : DataRateTestCase("Test arithmetic on DateRate")
{
}

void
DataRateTestCase2::AdditionTest(std::string rate1, std::string rate2, std::string rate3)
{
    DataRate dr1(rate1);
    DataRate dr2(rate2);
    DataRate dr3(rate3);

    CheckDataRateEqual(dr1 + dr2, dr3, "DataRate Addition returned incorrect value");

    dr1 += dr2;
    CheckDataRateEqual(dr1, dr3, "DataRate Addition returned incorrect value");
}

void
DataRateTestCase2::SubtractionTest(std::string rate1, std::string rate2, std::string rate3)
{
    DataRate dr1(rate1);
    DataRate dr2(rate2);
    DataRate dr3(rate3);

    CheckDataRateEqual(dr1 - dr2, dr3, "DataRate Subtraction returned incorrect value");

    dr1 -= dr2;
    CheckDataRateEqual(dr1, dr3, "DataRate Subtraction returned incorrect value");
}

void
DataRateTestCase2::MultiplicationIntTest(std::string rate1, uint64_t factor, std::string rate2)
{
    DataRate dr1(rate1);
    DataRate dr2(rate2);

    CheckDataRateEqual(dr1 * factor,
                       dr2,
                       "DataRate Multiplication with Int returned incorrect value");

    dr1 *= factor;
    CheckDataRateEqual(dr1, dr2, "DataRate Multiplication with Int returned incorrect value");
}

void
DataRateTestCase2::MultiplicationDoubleTest(std::string rate1, double factor, std::string rate2)
{
    DataRate dr1(rate1);
    DataRate dr2(rate2);

    CheckDataRateEqual(dr1 * factor,
                       dr2,
                       "DataRate Multiplication with Double returned incorrect value");

    dr1 *= factor;
    CheckDataRateEqual(dr1, dr2, "DataRate Multiplication with Double returned incorrect value");
}

void
DataRateTestCase2::DoRun()
{
    AdditionTest("1Mb/s", "3Mb/s", "4Mb/s");
    AdditionTest("1Gb/s", "1b/s", "1000000001b/s");
    SubtractionTest("1Mb/s", "1b/s", "999999b/s");
    SubtractionTest("2Gb/s", "2Gb/s", "0Gb/s");
    MultiplicationIntTest("5Gb/s", 2, "10Gb/s");
    MultiplicationIntTest("4Mb/s", 1000, "4Gb/s");
    MultiplicationDoubleTest("1Gb/s", 0.001, "1Mb/s");
    MultiplicationDoubleTest("6Gb/s", 1.0 / 7.0, "857142857.14b/s");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * Object with an attribute that is a tuple of data rates and a string.
 */
class DataRateTupleObject : public Object
{
  public:
    ~DataRateTupleObject() override = default;

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    std::tuple<DataRate, DataRate, std::string> m_tupleRatesString; //!< tuple of two data rates
                                                                    //!< and a string
};

TypeId
DataRateTupleObject::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DataRateTupleObject")
            .SetParent<Object>()
            .SetGroupName("Test")
            .AddConstructor<DataRateTupleObject>()
            .AddAttribute(
                "TupleRatesString",
                "An example of tuple of two data rates (comprising value and unit, possibly "
                "separated by a white space) and a string (possibly containing a white space).",
                StringValue("{1 Mb/s, 10kb/s, test string}"),
                MakeTupleAccessor<DataRateValue, DataRateValue, StringValue>(
                    &DataRateTupleObject::m_tupleRatesString),
                MakeTupleChecker<DataRateValue, DataRateValue, StringValue>(MakeDataRateChecker(),
                                                                            MakeDataRateChecker(),
                                                                            MakeStringChecker()));
    return tid;
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * Attribute set and get TestCase.
 */
class DataRateTupleSetGetTestCase : public TestCase
{
  public:
    DataRateTupleSetGetTestCase();
    ~DataRateTupleSetGetTestCase() override = default;

  private:
    void DoRun() override;
};

DataRateTupleSetGetTestCase::DataRateTupleSetGetTestCase()
    : TestCase("test attribute set and get")
{
}

void
DataRateTupleSetGetTestCase::DoRun()
{
    Ptr<DataRateTupleObject> obj = CreateObject<DataRateTupleObject>();

    // check that the tuple of two data rates and a string was correctly initialized
    NS_TEST_EXPECT_MSG_EQ(std::get<0>(obj->m_tupleRatesString),
                          DataRate(1e6),
                          "Unexpected value for the first data rate");
    NS_TEST_EXPECT_MSG_EQ(std::get<1>(obj->m_tupleRatesString),
                          DataRate(1e4),
                          "Unexpected value for the second data rate");
    NS_TEST_EXPECT_MSG_EQ(std::get<2>(obj->m_tupleRatesString),
                          "test string",
                          "Unexpected value for the string");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief DataRate TestSuite
 */
class DataRateTestSuite : public TestSuite
{
  public:
    DataRateTestSuite();
};

DataRateTestSuite::DataRateTestSuite()
    : TestSuite("data-rate", Type::UNIT)
{
    AddTestCase(new DataRateTestCase1(), TestCase::Duration::QUICK);
    AddTestCase(new DataRateTestCase2(), TestCase::Duration::QUICK);
    AddTestCase(new DataRateTupleSetGetTestCase(), TestCase::Duration::QUICK);
}

static DataRateTestSuite sDataRateTestSuite; //!< Static variable for test initialization
