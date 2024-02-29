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
#include "ns3/matrix-array.h"
#include "ns3/test.h"

/**
 * \defgroup matrixArray-tests MatrixArray tests
 * \ingroup core-tests
 * \ingroup Matrices
 */

/**
 * \file
 * \ingroup matrixArray-tests
 * MatrixArray test suite
 */
namespace ns3
{

namespace tests
{

NS_LOG_COMPONENT_DEFINE("MatrixArrayTest");

/**
 * \brief Function casts an input valArray "in" (type IN) to an output valArray "out" (type T)
 * \param in Input valarray to be casted
 * \param out Output valarray to receive casted values
 */
template <typename IN, typename T>
void
CastStdValarray(const std::valarray<IN>& in, std::valarray<T>& out)
{
    // Ensure output valarray is the right size
    if (out.size() != in.size())
    {
        out.resize(in.size());
    }

    // Perform the cast operation
    std::transform(std::begin(in), std::end(in), std::begin(out), [](IN i) {
        return static_cast<T>(i);
    });
}

/**
 * \ingroup matrixArray-tests
 *  MatrixArray test case for testing constructors, operators and other functions
 */
template <class T>
class MatrixArrayTestCase : public TestCase
{
  public:
    MatrixArrayTestCase() = default;
    /**
     * Constructor
     *
     * \param [in] name reference name
     */
    MatrixArrayTestCase(const std::string& name);

    /** Destructor. */
    ~MatrixArrayTestCase() override;
    /**
     * \brief Copy constructor.
     * Instruct the compiler to generate the implicitly declared copy constructor
     */
    MatrixArrayTestCase(const MatrixArrayTestCase<T>&) = default;
    /**
     * \brief Copy assignment operator.
     * Instruct the compiler to generate the implicitly declared copy assignment operator.
     * \return A reference to this MatrixArrayTestCase
     */
    MatrixArrayTestCase<T>& operator=(const MatrixArrayTestCase<T>&) = default;
    /**
     * \brief Move constructor.
     * Instruct the compiler to generate the implicitly declared move constructor
     */
    MatrixArrayTestCase(MatrixArrayTestCase<T>&&) = default;
    /**
     * \brief Move assignment operator.
     * Instruct the compiler to generate the implicitly declared copy constructor
     * \return A reference to this MatrixArrayTestCase
     */
    MatrixArrayTestCase<T>& operator=(MatrixArrayTestCase<T>&&) = default;

  protected:
  private:
    void DoRun() override;
};

template <class T>
MatrixArrayTestCase<T>::MatrixArrayTestCase(const std::string& name)
    : TestCase(name)
{
}

template <class T>
MatrixArrayTestCase<T>::~MatrixArrayTestCase()
{
}

template <class T>
void
MatrixArrayTestCase<T>::DoRun()
{
    // test multiplication of matrices (MatrixArray containing only 1 matrix)
    MatrixArray<T> m1 = MatrixArray<T>(2, 3);
    MatrixArray<T> m2 = MatrixArray<T>(m1.GetNumCols(), m1.GetNumRows());
    for (size_t i = 0; i < m1.GetNumRows(); ++i)
    {
        for (size_t j = 0; j < m1.GetNumCols(); ++j)
        {
            m1(i, j) = 1;
            m2(j, i) = 1;
        }
    }
    MatrixArray<T> m3 = m1 * m2;
    NS_LOG_INFO("m1:" << m1);
    NS_LOG_INFO("m2:" << m2);
    NS_LOG_INFO("m3 = m1 * m2:" << m3);
    NS_TEST_ASSERT_MSG_EQ(m3.GetNumRows(),
                          m1.GetNumRows(),
                          "The number of rows in resulting matrix is not correct");
    NS_TEST_ASSERT_MSG_EQ(m3.GetNumCols(),
                          m2.GetNumCols(),
                          "The number of cols in resulting matrix is not correct");
    NS_TEST_ASSERT_MSG_EQ(m3.GetNumRows(),
                          m3.GetNumCols(),
                          "The number of rows and cols should be equal");
    for (size_t i = 0; i < m3.GetNumCols(); ++i)
    {
        for (size_t j = 0; j < m3.GetNumRows(); ++j)
        {
            NS_TEST_ASSERT_MSG_EQ(std::real(m3(i, j)),
                                  m1.GetNumCols(),
                                  "The element value should be " << m1.GetNumCols());
        }
    }

    // multiplication with a scalar value
    MatrixArray<T> m4 = m3 * (static_cast<T>(5.0));
    for (size_t i = 0; i < m4.GetNumCols(); ++i)
    {
        for (size_t j = 0; j < m4.GetNumRows(); ++j)
        {
            NS_TEST_ASSERT_MSG_EQ(m3(i, j) * (static_cast<T>(5.0)),
                                  m4(i, j),
                                  "The values are not equal");
        }
    }
    NS_LOG_INFO("m4 = m3 * 5:" << m4);

    // test multiplication of arrays of matrices
    MatrixArray<T> m5 = MatrixArray<T>(2, 3, 2);
    MatrixArray<T> m6 = MatrixArray<T>(m5.GetNumCols(), m5.GetNumRows(), m5.GetNumPages());
    for (size_t p = 0; p < m5.GetNumPages(); ++p)
    {
        for (size_t i = 0; i < m5.GetNumRows(); ++i)
        {
            for (size_t j = 0; j < m5.GetNumCols(); ++j)
            {
                m5(i, j, p) = 1;
                m6(j, i, p) = 1;
            }
        }
    }
    MatrixArray<T> m7 = m5 * m6;
    NS_TEST_ASSERT_MSG_EQ(m7.GetNumRows(),
                          m5.GetNumRows(),
                          "The number of rows in resulting matrix is not correct");
    NS_TEST_ASSERT_MSG_EQ(m7.GetNumCols(),
                          m6.GetNumCols(),
                          "The number of cols in resulting matrix is not correct");
    NS_TEST_ASSERT_MSG_EQ(m7.GetNumRows(),
                          m7.GetNumCols(),
                          "The number of rows and cols should be equal");

    for (size_t p = 0; p < m7.GetNumPages(); ++p)
    {
        for (size_t i = 0; i < m7.GetNumCols(); ++i)
        {
            for (size_t j = 0; j < m7.GetNumRows(); ++j)
            {
                NS_TEST_ASSERT_MSG_EQ(std::real(m7(i, j, p)),
                                      m5.GetNumCols(),
                                      "The element value should be " << m5.GetNumCols());
            }
        }
    }
    // test ostream operator
    NS_LOG_INFO("m5:" << m5);
    NS_LOG_INFO("m6:" << m6);
    NS_LOG_INFO("m7 = m5 * m6:" << m7);

    // test transpose function
    MatrixArray<T> m8 = m5.Transpose();
    NS_TEST_ASSERT_MSG_EQ(m6, m8, "These two matrices should be equal");
    NS_LOG_INFO("m8 = m5.Transpose ()" << m8);

    // test transpose using initialization arrays
    std::valarray<int> a{0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4};
    std::valarray<int> b{0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4};
    std::valarray<T> aCasted(a.size());
    std::valarray<T> bCasted(b.size());
    for (size_t i = 0; i < a.size(); ++i)
    {
        aCasted[i] = static_cast<T>(a[i]);
    }
    for (size_t i = 0; i < b.size(); ++i)
    {
        bCasted[i] = static_cast<T>(b[i]);
    }
    m5 = MatrixArray<T>(3, 5, 1, aCasted);
    m6 = MatrixArray<T>(5, 3, 1, bCasted);
    m8 = m5.Transpose();
    NS_TEST_ASSERT_MSG_EQ(m6, m8, "These two matrices should be equal");
    NS_LOG_INFO("m5 (3, 5, 1):" << m5);
    NS_LOG_INFO("m6 (5, 3, 1):" << m6);
    NS_LOG_INFO("m8 (5, 3, 1) = m5.Transpose ()" << m8);

    // test 1D array creation, i.e. vector and transposing it
    MatrixArray<T> m9 = MatrixArray<T>(std::vector<T>({0, 1, 2, 3, 4, 5, 6, 7}));
    NS_TEST_ASSERT_MSG_EQ((m9.GetNumRows() == 8) && (m9.GetNumCols() == 1) &&
                              (m9.GetNumPages() == 1),
                          true,
                          "Creation of vector is not correct.");

    NS_LOG_INFO("Vector:" << m9);
    NS_LOG_INFO("Vector after transposing:" << m9.Transpose());

    // Test basic operators
    MatrixArray<T> m10 =
        MatrixArray<T>(m9.GetNumRows(), m9.GetNumCols(), m9.GetNumPages(), m9.GetValues());
    NS_TEST_ASSERT_MSG_EQ(m10, m9, "m10 and m9 should be equal");
    m10 -= m9;
    NS_TEST_ASSERT_MSG_NE(m10, m9, "m10 and m9 should not be equal");
    m10 += m9;
    NS_TEST_ASSERT_MSG_EQ(m10, m9, "m10 and m9 should be equal");
    m10 = m9;
    NS_TEST_ASSERT_MSG_EQ(m10, m9, "m10 and m9 should be equal");
    m10 = m9 + m9;
    NS_TEST_ASSERT_MSG_NE(m10, m9, "m10 and m9 should not be equal");
    m10 = m10 - m9;
    NS_TEST_ASSERT_MSG_EQ(m10, m9, "m10 and m9 should be equal");

    // test multiplication by using an initialization matrixArray
    // matrix dimensions in each page are 2x3, 3x2, and the resulting matrix per page is a square
    // matrix 2x2
    a = {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5};
    b = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
    std::valarray<int> c{2, 3, 4, 6, 2, 3, 4, 6};
    aCasted = std::valarray<T>(a.size());
    bCasted = std::valarray<T>(b.size());
    std::valarray<T> cCasted = std::valarray<T>(c.size());

    for (size_t i = 0; i < a.size(); ++i)
    {
        aCasted[i] = static_cast<T>(a[i]);
    }
    for (size_t i = 0; i < b.size(); ++i)
    {
        bCasted[i] = static_cast<T>(b[i]);
    }
    for (size_t i = 0; i < c.size(); ++i)
    {
        cCasted[i] = static_cast<T>(c[i]);
    }
    MatrixArray<T> m11 = MatrixArray<T>(2, 3, 2, aCasted);
    MatrixArray<T> m12 = MatrixArray<T>(3, 2, 2, bCasted);
    MatrixArray<T> m13 = m11 * m12;
    MatrixArray<T> m14 = MatrixArray<T>(2, 2, 2, cCasted);
    NS_TEST_ASSERT_MSG_EQ(m13.GetNumCols(),
                          m14.GetNumCols(),
                          "The number of columns is not as expected.");
    NS_TEST_ASSERT_MSG_EQ(m13.GetNumRows(),
                          m14.GetNumRows(),
                          "The number of rows is not as expected.");
    NS_TEST_ASSERT_MSG_EQ(m13, m14, "The values are not equal.");
    NS_LOG_INFO("m11 (2,3,2):" << m11);
    NS_LOG_INFO("m12 (3,2,2):" << m12);
    NS_LOG_INFO("m13 = m11 * m12:" << m13);

    // test multiplication by using an initialization matrixArray
    // matrices have different number of elements per page
    // matrix dimensions in each page are 4x3, 3x2, and the resulting matrix
    // dimensions are 4x2
    a = std::valarray<int>(
        {0, 1, 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 0, 1, 0, 1, 2, 3, 2, 3, 4, 5, 4, 5});
    b = std::valarray<int>({0, 1, 0, 1, 0, 1, 0, 10, 0, 10, 0, 10});
    c = std::valarray<int>({2, 3, 2, 3, 4, 6, 4, 6, 20, 30, 20, 30, 40, 60, 40, 60});
    aCasted = std::valarray<T>(a.size());
    bCasted = std::valarray<T>(b.size());
    cCasted = std::valarray<T>(c.size());

    for (size_t i = 0; i < a.size(); ++i)
    {
        aCasted[i] = static_cast<T>(a[i]);
    }
    for (size_t i = 0; i < b.size(); ++i)
    {
        bCasted[i] = static_cast<T>(b[i]);
    }
    for (size_t i = 0; i < c.size(); ++i)
    {
        cCasted[i] = static_cast<T>(c[i]);
    }

    m11 = MatrixArray<T>(4, 3, 2, aCasted);
    m12 = MatrixArray<T>(3, 2, 2, bCasted);
    m13 = m11 * m12;
    m14 = MatrixArray<T>(4, 2, 2, cCasted);
    NS_TEST_ASSERT_MSG_EQ(m13.GetNumCols(),
                          m14.GetNumCols(),
                          "The number of columns is not as expected.");
    NS_TEST_ASSERT_MSG_EQ(m13.GetNumRows(),
                          m14.GetNumRows(),
                          "The number of rows is not as expected.");
    NS_TEST_ASSERT_MSG_EQ(m13, m14, "The values are not equal.");
    NS_LOG_INFO("m11 (4,3,2):" << m11);
    NS_LOG_INFO("m12 (3,2,2):" << m12);
    NS_LOG_INFO("m13 = m11 * m12:" << m13);

    // test multiplication by using an initialization matrixArray
    // matrices have different number of elements per page
    // matrix dimensions in each page are 1x3, 3x2, and the resulting matrix has
    // dimensions 1x2
    a = std::valarray<int>({5, 4, 5, 5, 4, 5});
    b = std::valarray<int>({0, 1, 0, 1, 0, 1, 1, 2, 3, 10, 100, 1000});
    c = std::valarray<int>({4, 10, 28, 5450});
    aCasted = std::valarray<T>(a.size());
    bCasted = std::valarray<T>(b.size());
    cCasted = std::valarray<T>(c.size());

    for (size_t i = 0; i < a.size(); ++i)
    {
        aCasted[i] = static_cast<T>(a[i]);
    }
    for (size_t i = 0; i < b.size(); ++i)
    {
        bCasted[i] = static_cast<T>(b[i]);
    }
    for (size_t i = 0; i < c.size(); ++i)
    {
        cCasted[i] = static_cast<T>(c[i]);
    }

    m11 = MatrixArray<T>(1, 3, 2, aCasted);
    m12 = MatrixArray<T>(3, 2, 2, bCasted);
    m13 = m11 * m12;
    m14 = MatrixArray<T>(1, 2, 2, cCasted);
    NS_TEST_ASSERT_MSG_EQ(m13.GetNumCols(),
                          m14.GetNumCols(),
                          "The number of columns is not as expected.");
    NS_TEST_ASSERT_MSG_EQ(m13.GetNumRows(),
                          m14.GetNumRows(),
                          "The number of rows is not as expected.");
    NS_TEST_ASSERT_MSG_EQ(m13, m14, "The values are not equal.");
    NS_LOG_INFO("m11 (1,3,2):" << m11);
    NS_LOG_INFO("m12 (3,2,2):" << m12);
    NS_LOG_INFO("m13 = m11 * m12:" << m13);

    // test MultiplyByLeftAndRightMatrix
    std::valarray<int> d{1, 1, 1};
    std::valarray<int> e{1, 1};
    std::valarray<int> f{1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3};
    std::valarray<int> g{12, 12};
    std::valarray<T> dCasted(d.size());
    std::valarray<T> eCasted(e.size());
    std::valarray<T> fCasted(f.size());
    std::valarray<T> gCasted(g.size());
    for (size_t i = 0; i < d.size(); ++i)
    {
        dCasted[i] = static_cast<T>(d[i]);
    }
    for (size_t i = 0; i < e.size(); ++i)
    {
        eCasted[i] = static_cast<T>(e[i]);
    }
    for (size_t i = 0; i < f.size(); ++i)
    {
        fCasted[i] = static_cast<T>(f[i]);
    }
    for (size_t i = 0; i < g.size(); ++i)
    {
        gCasted[i] = static_cast<T>(g[i]);
    }
    MatrixArray<T> m15 = MatrixArray<T>(1, 3, dCasted);
    MatrixArray<T> m16 = MatrixArray<T>(2, 1, eCasted);
    MatrixArray<T> m17 = MatrixArray<T>(3, 2, 2, fCasted);
    MatrixArray<T> m18 = MatrixArray<T>(1, 1, 2, gCasted);
    MatrixArray<T> m19 = m17.MultiplyByLeftAndRightMatrix(m15, m16);
    NS_TEST_ASSERT_MSG_EQ(m19, m18, "The matrices should be equal.");

    // test MultiplyByLeftAndRightMatrix
    std::valarray<int> h{1, 3, 2, 2, 4, 0};
    std::valarray<int> j{2, 2, 3, 4, 1, 3, 0, 5};
    std::valarray<int> k{1, 2, 0, 0, 2, 3, 4, 1, 2, 3, 4, 1, 1, 2, 0, 0, 2, 3, 4, 1, 2, 3, 4, 1};
    std::valarray<int> l{144, 132, 128, 104, 144, 132, 128, 104};
    std::valarray<T> hCasted(h.size());
    std::valarray<T> jCasted(j.size());
    std::valarray<T> kCasted(k.size());
    std::valarray<T> lCasted(l.size());
    for (size_t i = 0; i < h.size(); ++i)
    {
        hCasted[i] = static_cast<T>(h[i]);
    }
    for (size_t i = 0; i < j.size(); ++i)
    {
        jCasted[i] = static_cast<T>(j[i]);
    }
    for (size_t i = 0; i < k.size(); ++i)
    {
        kCasted[i] = static_cast<T>(k[i]);
    }
    for (size_t i = 0; i < l.size(); ++i)
    {
        lCasted[i] = static_cast<T>(l[i]);
    }
    MatrixArray<T> m20 = MatrixArray<T>(2, 3, hCasted);
    MatrixArray<T> m21 = MatrixArray<T>(4, 2, jCasted);
    MatrixArray<T> m22 = MatrixArray<T>(3, 4, 2, kCasted);
    MatrixArray<T> m23 = MatrixArray<T>(2, 2, 2, lCasted);
    MatrixArray<T> m24 = m22.MultiplyByLeftAndRightMatrix(m20, m21);
    NS_TEST_ASSERT_MSG_EQ(m24, m23, "The matrices should be equal.");
    NS_LOG_INFO("m20:" << m20);
    NS_LOG_INFO("m21:" << m21);
    NS_LOG_INFO("m22:" << m22);
    NS_LOG_INFO("m24 = m20 * m22 * m21" << m24);

    // test initialization with moving
    size_t lCastedSize = lCasted.size();
    NS_LOG_INFO("size() of lCasted before move: " << lCasted.size());
    MatrixArray<T> m25 = MatrixArray<T>(2, 2, 2, std::move(lCasted));
    NS_LOG_INFO("m25.GetSize ()" << m25.GetSize());
    NS_TEST_ASSERT_MSG_EQ(lCastedSize, m25.GetSize(), "The number of elements are not equal.");

    size_t hCastedSize = hCasted.size();
    NS_LOG_INFO("size() of hCasted before move: " << hCasted.size());
    MatrixArray<T> m26 = MatrixArray<T>(2, 3, std::move(hCasted));
    NS_LOG_INFO("m26.GetSize ()" << m26.GetSize());
    NS_TEST_ASSERT_MSG_EQ(hCastedSize, m26.GetSize(), "The number of elements are not equal.");

    size_t jCastedSize = jCasted.size();
    NS_LOG_INFO("size() of jCasted before move: " << jCasted.size());
    MatrixArray<T> m27 = MatrixArray<T>(std::move(jCasted));
    NS_LOG_INFO("m27.GetSize ()" << m27.GetSize());
    NS_TEST_ASSERT_MSG_EQ(jCastedSize, m27.GetSize(), "The number of elements are not equal.");

    // test determinant
    {
        std::vector<std::pair<std::valarray<int>, T>> detTestCases{
            // right-wraparound
            {{1, 0, 7, 4, 2, 0, 6, 5, 3}, 62},
            // left-wraparound
            {{1, 4, 6, 0, 2, 5, 7, 0, 3}, 62},
            // identity rank 3
            {{1, 0, 0, 0, 1, 0, 0, 0, 1}, 1},
            // identity rank 4
            {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, 1},
            // permutation matrix rank 4
            {{0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0}, -1},
            // positive det matrix rank 2
            {{36, -5, -5, 43}, 1523},
            // single value matrix rank 1
            {{1}, 1},
        };
        for (const auto& [detVal, detRef] : detTestCases)
        {
            std::valarray<T> detCast(detVal.size());
            CastStdValarray(detVal, detCast);
            auto side = sqrt(detVal.size());
            MatrixArray<T> detMat = MatrixArray<T>(side, side, std::move(detCast));
            NS_TEST_ASSERT_MSG_EQ(detMat.Determinant()(0),
                                  static_cast<T>(detRef),
                                  "The determinants are not equal.");
        }
    }
    // clang-format off
    std::valarray<int> multiPageMatrixValues{
            // page 0: identity matrix
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
            // page 1: permutation matrix
            0, 0, 1,
            0, 1, 0,
            1, 0, 0,
            // page 2: random matrix
            1, 4, 6,
            0, 2, 5,
            7, 0, 3,
            // page 3: upper triangular
            5, -9, 2,
            0, -4, -10,
            0, 0, -3,
            // page 4: lower triangular
            -7, 0, 0,
            -1, -9, 0,
            -5, 6, -5,
            // page 5: zero diagonal
            0, 1, 2,
            1, 0, 1,
            2, 1, 0,
            // page 6: zero bidiagonal
            0, 1, 0,
            1, 0, 1,
            0, 1, 0
    };
    // clang-format on
    std::valarray<T> castMultiPageMatrixValues(multiPageMatrixValues.size());
    CastStdValarray(multiPageMatrixValues, castMultiPageMatrixValues);
    MatrixArray<T> multiPageMatrix = MatrixArray<T>(3,
                                                    3,
                                                    multiPageMatrixValues.size() / 9,
                                                    std::move(castMultiPageMatrixValues));
    // test the determinant in a multi-page MatrixArray
    std::vector<int> determinants = {1, -1, 62, 60, -315, 4, 0};
    auto det = multiPageMatrix.Determinant();
    for (size_t page = 0; page < multiPageMatrix.GetNumPages(); page++)
    {
        NS_TEST_ASSERT_MSG_EQ(det(page),
                              static_cast<T>(determinants[page]),
                              "The determinants from the page " << std::to_string(page)
                                                                << " are not equal.");
    }

    // test Frobenius norm in a multi-page MatrixArray
    std::vector<double> fnorms = {sqrt(3), sqrt(3), 11.8322, 15.3297, 14.7309, 3.4641, 2};
    auto frob = multiPageMatrix.FrobeniusNorm();
    for (size_t page = 0; page < multiPageMatrix.GetNumPages(); page++)
    {
        NS_TEST_ASSERT_MSG_EQ_TOL(std::abs(frob(page)),
                                  std::abs(static_cast<T>(fnorms[page])),
                                  0.0001,
                                  "The Frobenius norm from the page " << std::to_string(page)
                                                                      << " are not equal.");
    }

    // test page copying
    for (size_t noOfCopies = 1; noOfCopies < 4; noOfCopies++)
    {
        auto copies = m27.MakeNCopies(noOfCopies);
        NS_TEST_ASSERT_MSG_EQ(copies.GetNumPages(),
                              noOfCopies,
                              "Creating " << std::to_string(noOfCopies) << " copies failed.");
        NS_TEST_ASSERT_MSG_EQ(copies.GetNumRows(),
                              m27.GetNumRows(),
                              "The copy doesn't have the same number of rows as the original.");
        NS_TEST_ASSERT_MSG_EQ(copies.GetNumCols(),
                              m27.GetNumCols(),
                              "The copy doesn't have the same number of columns as the original.");
        for (size_t page = 0; page < copies.GetNumPages(); page++)
        {
            T diff{};
            for (size_t row = 0; row < copies.GetNumRows(); row++)
            {
                for (size_t col = 0; col < copies.GetNumCols(); col++)
                {
                    diff += m27(row, col, 0) - copies(row, col, page);
                }
            }
            NS_TEST_ASSERT_MSG_EQ(diff, T{}, "Mismatch in copied values.");
        }
    }

    // test page 1 and 0 extraction
    std::vector<MatrixArray<T>> pages{multiPageMatrix.ExtractPage(1),
                                      multiPageMatrix.ExtractPage(0)};

    // test page 1 and 0 joining
    auto jointPagesMatrix = MatrixArray<T>::JoinPages(pages);
    NS_TEST_ASSERT_MSG_EQ(jointPagesMatrix.GetNumPages(), 2, "Mismatch in number of join pages.");
    for (size_t page = 0; page < jointPagesMatrix.GetNumPages(); page++)
    {
        T diff{};
        for (size_t row = 0; row < jointPagesMatrix.GetNumRows(); row++)
        {
            for (size_t col = 0; col < jointPagesMatrix.GetNumCols(); col++)
            {
                diff += multiPageMatrix(row, col, 1 - page) - jointPagesMatrix(row, col, page);
            }
        }
        NS_TEST_ASSERT_MSG_EQ(diff, T{}, "Mismatching pages.");
    }

    // test identity matrix
    auto identityRank3Reference = multiPageMatrix.ExtractPage(0);
    auto identityRank3 = MatrixArray<T>::IdentityMatrix(3);
    NS_TEST_ASSERT_MSG_EQ(identityRank3, identityRank3Reference, "Mismatch in identity matrices.");

    identityRank3 = MatrixArray<T>::IdentityMatrix(3, 10).ExtractPage(9);
    NS_TEST_ASSERT_MSG_EQ(identityRank3, identityRank3Reference, "Mismatch in identity matrices.");

    identityRank3 = MatrixArray<T>::IdentityMatrix(identityRank3Reference);
    NS_TEST_ASSERT_MSG_EQ(identityRank3, identityRank3Reference, "Mismatch in identity matrices.");
}

/**
 * \ingroup matrixArray-tests
 *  Test for testing functions that apply to MatrixArrays that use complex numbers,
 *  such as HermitianTranspose that is only defined for complex type
 */
class ComplexMatrixArrayTestCase : public TestCase
{
  public:
    /** Constructor*/
    ComplexMatrixArrayTestCase();
    /**
     * Constructor
     *
     * \param [in] name reference name
     */
    ComplexMatrixArrayTestCase(const std::string& name);
    /** Destructor*/
    ~ComplexMatrixArrayTestCase() override;

  private:
    void DoRun() override;
};

ComplexMatrixArrayTestCase::ComplexMatrixArrayTestCase()
    : TestCase("ComplexMatrixArrayTestCase")
{
}

ComplexMatrixArrayTestCase::ComplexMatrixArrayTestCase(const std::string& name)
    : TestCase(name)
{
}

ComplexMatrixArrayTestCase::~ComplexMatrixArrayTestCase()
{
}

void
ComplexMatrixArrayTestCase::DoRun()
{
    std::valarray<std::complex<double>> complexValarray1 = {
        {1, 1},
        {2, 2},
        {3, 3},
        {4, 4},
        {5, 5},
        {6, 6},
        {-1, 1},
        {-2, 2},
        {-3, 3},
        {-4, 4},
        {-5, 5},
        {-6, 6},
    };
    std::valarray<std::complex<double>> complexValarray2 = {
        {1, -1},
        {4, -4},
        {2, -2},
        {5, -5},
        {3, -3},
        {6, -6},
        {-1, -1},
        {-4, -4},
        {-2, -2},
        {-5, -5},
        {-3, -3},
        {-6, -6},
    };
    ComplexMatrixArray m1 = ComplexMatrixArray(3, 2, 2, complexValarray1);
    ComplexMatrixArray m2 = ComplexMatrixArray(2, 3, 2, complexValarray2);
    ComplexMatrixArray m3 = m1.HermitianTranspose();
    NS_LOG_INFO("m1 (3, 2, 2):" << m1);
    NS_LOG_INFO("m2 (2, 3, 2):" << m2);
    NS_LOG_INFO("m3 (2, 3, 2):" << m3);
    NS_TEST_ASSERT_MSG_EQ(m2, m3, "m2 and m3 matrices should be equal");
}

/**
 * \ingroup matrixArray-tests
 * MatrixArray test suite
 */
class MatrixArrayTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    MatrixArrayTestSuite();
};

MatrixArrayTestSuite::MatrixArrayTestSuite()
    : TestSuite("matrix-array-test")
{
    AddTestCase(new MatrixArrayTestCase<double>("Test MatrixArray<double>"));
    AddTestCase(
        new MatrixArrayTestCase<std::complex<double>>("Test MatrixArray<std::complex<double>>"));
    AddTestCase(new MatrixArrayTestCase<int>("Test MatrixArray<int>"));
    AddTestCase(new ComplexMatrixArrayTestCase("Test ComplexMatrixArray"));
}

/**
 * \ingroup matrixArray-tests
 * MatrixArrayTestSuite instance variable.
 */
static MatrixArrayTestSuite g_matrixArrayTestSuite;

} // namespace tests
} // namespace ns3
