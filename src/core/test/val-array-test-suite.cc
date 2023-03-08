/*
 * Copyright (c) 2022 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Biljana Bojovic <bbojovic@cttc.es>
 */

#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/val-array.h"

/**
 * \ingroup core-tests
 */

namespace ns3
{
namespace tests
{

NS_LOG_COMPONENT_DEFINE("ValArrayTest");

/**
 * @brief ValArray test case for testing ValArray class
 *
 * @tparam T the template parameter that can be a complex number, double or int
 */
template <class T>
class ValArrayTestCase : public TestCase
{
  public:
    /** Default constructor*/
    ValArrayTestCase<T>() = default;
    /**
     * Constructor
     *
     * \param [in] name reference name
     */
    ValArrayTestCase<T>(const std::string name);

    /** Destructor. */
    ~ValArrayTestCase<T>() override;
    /**
     * \brief Copy constructor.
     * Instruct the compiler to generate the implicitly declared copy constructor
     */
    ValArrayTestCase<T>(const ValArrayTestCase<T>&) = default;
    /**
     * \brief Copy assignment operator.
     * Instruct the compiler to generate the implicitly declared copy assignment operator.
     * \return A reference to this ValArrayTestCase
     */
    ValArrayTestCase<T>& operator=(const ValArrayTestCase<T>&) = default;
    /**
     * \brief Move constructor.
     * Instruct the compiler to generate the implicitly declared move constructor
     */
    ValArrayTestCase<T>(ValArrayTestCase<T>&&) = default;
    /**
     * \brief Move assignment operator.
     * Instruct the compiler to generate the implicitly declared copy constructor
     * \return A reference to this ValArrayTestCase
     */
    ValArrayTestCase<T>& operator=(ValArrayTestCase<T>&&) = default;

  private:
    void DoRun() override;
};

template <class T>
ValArrayTestCase<T>::ValArrayTestCase(const std::string name)
    : TestCase(name)
{
}

template <class T>
ValArrayTestCase<T>::~ValArrayTestCase<T>()
{
}

template <class T>
void
ValArrayTestCase<T>::DoRun()
{
    ValArray<T> v1 = ValArray<T>(2, 3);
    for (uint16_t i = 0; i < v1.GetNumRows(); ++i)
    {
        for (uint16_t j = 0; j < v1.GetNumCols(); ++j)
        {
            v1(i, j) = 1;
        }
    }

    ValArray<T> v2 = ValArray<T>(v1);
    NS_TEST_ASSERT_MSG_EQ(v1.GetNumRows(), v2.GetNumRows(), "The number of rows are not equal.");
    NS_TEST_ASSERT_MSG_EQ(v1.GetNumCols(), v2.GetNumCols(), "The number of cols are not equal.");

    // test copy constructor
    for (uint16_t i = 0; i < v1.GetNumRows(); ++i)
    {
        for (uint16_t j = 0; j < v1.GetNumCols(); ++j)
        {
            NS_TEST_ASSERT_MSG_EQ(v1(i, j), v2(i, j), "The elements are not equal.");
        }
    }

    // test assign constructor
    ValArray<T> v3 = v1;
    NS_TEST_ASSERT_MSG_EQ(v1.GetNumRows(), v3.GetNumRows(), "The number of rows are not equal.");
    NS_TEST_ASSERT_MSG_EQ(v1.GetNumCols(), v3.GetNumCols(), "The number of cols are not equal.");
    for (uint16_t i = 0; i < v1.GetNumRows(); ++i)
    {
        for (uint16_t j = 0; j < v1.GetNumCols(); ++j)
        {
            NS_TEST_ASSERT_MSG_EQ(v1(i, j), v2(i, j), "The elements are not equal.");
        }
    }

    // test move assignment operator
    ValArray<T> v4;
    NS_LOG_INFO("v1 size before move: " << v1.GetSize());
    NS_LOG_INFO("v4 size before move: " << v4.GetSize());
    size_t v1size = v1.GetSize();
    v4 = std::move(v1);
    NS_LOG_INFO("v4 size after move: " << v4.GetSize());
    NS_TEST_ASSERT_MSG_EQ(v1size, v4.GetSize(), "The number of elements are not equal.");
    for (uint16_t i = 0; i < v4.GetNumRows(); ++i)
    {
        for (uint16_t j = 0; j < v4.GetNumCols(); ++j)
        {
            // Use v3 for comparison since it hasn't moved
            NS_TEST_ASSERT_MSG_EQ(v3(i, j), v4(i, j), "The elements are not equal.");
        }
    }

    // test move constructor
    NS_LOG_INFO("v3 size before move: " << v3.GetSize());
    size_t v3size = v3.GetSize();
    ValArray<T> v5(std::move(v3));
    NS_TEST_ASSERT_MSG_EQ(v3size, v5.GetSize(), "The number of elements are not equal.");
    for (uint16_t i = 0; i < v5.GetNumRows(); ++i)
    {
        for (uint16_t j = 0; j < v5.GetNumCols(); ++j)
        {
            // Use v4 for comparison since it hasn't moved
            NS_TEST_ASSERT_MSG_EQ(v4(i, j), v5(i, j), "The elements are not equal.");
        }
    }

    // test constructor with initialization valArray
    std::valarray<int> initArray1{0, 1, 2, 3, 4, 5, 6, 7};
    std::valarray<T> valArray1(initArray1.size()); // length is 8 elements
    for (size_t i = 0; i < initArray1.size(); i++)
    {
        valArray1[i] = static_cast<T>(initArray1[i]);
    }
    ValArray<T> v6 = ValArray<T>(2, 4, valArray1);

    // test constructor that moves valArray
    NS_LOG_INFO("valarray1 size before move: " << valArray1.size());
    ValArray<T> v11 = ValArray<T>(2, 4, std::move(valArray1));
    NS_LOG_INFO("valarray1 size after move: " << valArray1.size());
    NS_LOG_INFO("v11 size after move: " << v11.GetSize());

    // test whether column-major order was respected during the initialization and
    // also in the access operator if we iterate over rows first we should find 0, 2, 4, 6, ...
    std::valarray<int> initArray2{0, 2, 4, 6, 1, 3, 5, 7};
    auto testIndex = 0;
    for (uint16_t i = 0; i < v6.GetNumRows(); ++i)
    {
        for (uint16_t j = 0; j < v6.GetNumCols(); ++j)
        {
            NS_TEST_ASSERT_MSG_EQ(v6(i, j),
                                  static_cast<T>(initArray2[testIndex]),
                                  "The values are not equal.");
            testIndex++;
        }
    }

    // test constructor with initialization valArray for 3D array
    std::valarray<int> initArray3{0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7};
    std::valarray<T> valArray2(initArray3.size()); // length is 8 elements
    for (size_t i = 0; i < initArray3.size(); i++)
    {
        valArray2[i] = static_cast<T>(initArray3[i]);
    }

    ValArray<T> v7 = ValArray<T>(2, 4, 2, valArray2);
    // test whether column-major order was respected during the initialization and
    // also in the access operator
    // if we iterate over rows first we should find 0, 2, 4, 6, ...
    std::valarray<int> initArray4{0, 2, 4, 6, 1, 3, 5, 7, 0, 2, 4, 6, 1, 3, 5, 7};
    testIndex = 0;
    for (uint16_t p = 0; p < v7.GetNumPages(); ++p)
    {
        for (uint16_t i = 0; i < v7.GetNumRows(); ++i)
        {
            for (uint16_t j = 0; j < v7.GetNumCols(); ++j)
            {
                NS_TEST_ASSERT_MSG_EQ(v7(i, j, p),
                                      static_cast<T>(initArray4[testIndex]),
                                      "The values are not equal.");
                testIndex++;
            }
        }
    }

    // multiplication with a scalar value with 3D array
    ValArray<T> v8 = v7 * (static_cast<T>(5.0));
    for (uint16_t p = 0; p < v8.GetNumPages(); ++p)
    {
        for (uint16_t i = 0; i < v8.GetNumRows(); ++i)
        {
            for (uint16_t j = 0; j < v8.GetNumCols(); ++j)
            {
                NS_TEST_ASSERT_MSG_EQ(v7(i, j, p) * (static_cast<T>(5.0)),
                                      v8(i, j, p),
                                      "The values are not equal");
            }
        }
    }

    NS_LOG_INFO("v8 = v7 * 5:" << v8);
    // test +, - (binary, unary) operators
    NS_LOG_INFO("v8 + v8" << v8 + v8);
    NS_LOG_INFO("v8 - v8" << v8 - v8);
    NS_LOG_INFO("-v8" << -v8);

    // test +=  and -= assignment operators
    ValArray<T> v9(v8.GetNumRows(), v8.GetNumCols(), v8.GetNumPages());
    v9 += v8;
    NS_LOG_INFO("v9 += v8" << v9);
    ValArray<T> v10(v8.GetNumRows(), v8.GetNumCols(), v8.GetNumPages());
    v10 -= v8;
    NS_LOG_INFO("v10 -= v8" << v10);

    // test == and != operators
    NS_TEST_ASSERT_MSG_EQ(bool(v9 == v8), true, "Matrices v8 and v9 should be equal");
    NS_TEST_ASSERT_MSG_EQ(bool(v10 == v8), false, "Matrices v8 and v10 should not be equal");
    NS_TEST_ASSERT_MSG_EQ(bool(v10 != v8), true, "Matrices v8 and v10 should not be equal");
    // test whether arrays are equal when they have different lengths
    NS_TEST_ASSERT_MSG_NE(ValArray<int>(std::valarray({1, 2, 3})),
                          ValArray<int>(std::valarray({1, 2, 3, 4})),
                          "Arrays should not be equal, they have different dimensions.");

    // test the function IsAlmostEqual
    v9(0, 0, 0) = v9(0, 0, 0) + static_cast<T>(1);
    NS_TEST_ASSERT_MSG_EQ(v9.IsAlmostEqual(v8, 2) && (v9 != v8),
                          true,
                          "Matrices should be almost equal, but not equal.");

    // test the initialization with std::vector
    ValArray<T> v12 = ValArray(std::vector<T>({1, 2, 3}));
    NS_LOG_INFO("v12:" << v12);
}

/**
 * \ingroup valArray-tests
 * ValArray test suite
 *
 * \brief The test checks the correct behaviour of ValArray class
 */
class ValArrayTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    ValArrayTestSuite();
};

ValArrayTestSuite::ValArrayTestSuite()
    : TestSuite("val-array-test")
{
    AddTestCase(new ValArrayTestCase<double>("Test ValArray<double>"));
    AddTestCase(new ValArrayTestCase<std::complex<double>>("Test ValArray<std::complex<double>>"));
    AddTestCase(new ValArrayTestCase<int>("Test ValArray<int>"));
}

/**
 * \ingroup valArray-tests
 * ValArrayTestSuite instance variable.
 */
static ValArrayTestSuite g_valArrayTestSuite;

} // namespace tests
} // namespace ns3
