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
#ifndef VAL_ARRAY_H
#define VAL_ARRAY_H

#include "assert.h"
#include "simple-ref-count.h"

#include <complex>
#include <valarray>
#include <vector>

namespace ns3
{

/**
 * \defgroup Matrices Classes to do efficient math operations on arrays
 * \ingroup core
 */

/**
 * \ingroup Matrices
 *
 * \brief ValArray is a class to efficiently store 3D array. The class is general
 * enough to represent 1D array or 2D arrays. ValArray also provides basic
 * algebra element-wise operations over the whole array (1D, 2D, 3D).
 *
 * Main characteristics of ValArray are the following:
 *
 * - ValArray uses std::valarray to efficiently store data.
 *
 * - In general, the elements are stored in memory as a sequence of consecutive
 * 2D arrays. The dimensions of 2D arrays are defined by numRows and numCols,
 * while the number of 2D arrays is defined by numPages. Notice that if we set
 * the number of pages to 1, we will have only a single 2D array. If we
 * additionally set numRows or numCols to 1, we will have 1D array.
 *
 * - All 2D arrays have the same dimensions, i.e. numRows and numCols.
 *
 * - 2D arrays are stored in column-major order, which is the default
 * order in Eigen and Armadillo libraries, which allows a straightforward mapping
 * of any page (2D array) within ValArray to Eigen or Armadillo matrices.
 *
 * Examples of column-major order:
 *
 * a) in the case of a 2D array, we will have in memory the following order of
 * elements, assuming that the indexes are rowIndex, colIndex, pageIndex:
 *
 * a000 a100 a010 a110 a020 a120.
 *
 * b) in the case of a 3D array, e.g, if there are two 2D arrays of 2x3 dimensions
 * we will have in memory the following order of elements,
 * assuming that the indexes are rowIndex, colIndex, pageIndex:
 *
 * a000 a100 a010 a110 a020 a120 a001 a101 a011 a111 a021 a121.
 *
 * - The access to the elements is implemented in operators:
 * - operator (rowIndex) and operator[] (rowIndex) for 1D array (assuming colIndex=0, pageIndex=0),
 * - operator (rowIndex,colIndex) for 2D array (assuming pageIndex=0) and
 * - operator(rowIndex, colIndex, pageIndex) for 3D array.
 *
 * Definition of ValArray as a template class allows using different numerical
 * types as the elements of the vectors/matrices, e.g., complex numbers, double,
 * int, etc.
 */

template <class T>
class ValArray : public SimpleRefCount<ValArray<T>>
{
  public:
    // instruct the compiler to generate the default constructor
    ValArray() = default;
    /**
     * \brief Constructor that creates "numPages" number of 2D arrays that are of
     * dimensions "numRows"x"numCols", and are initialized with all-zero elements.
     * If only 1 parameter, numRows, is provided then a single 1D array is being created.
     * \param numRows the number of rows
     * \param numCols the number of columns
     * \param numPages the number of pages
     */
    ValArray(size_t numRows, size_t numCols = 1, size_t numPages = 1);
    /**
     * \brief Constructor creates a single 1D array of values.size () elements and 1 column,
     * and uses std::valarray<T> values to initialize the elements.
     * \param values std::valarray<T> that will be used to initialize elements of 1D array
     */
    explicit ValArray(const std::valarray<T>& values);
    /**
     * \brief Constructor creates a single 1D array of values.size () elements and 1 column,
     * and moves std::valarray<T> values to initialize the elements.
     * \param values std::valarray<T> that will be moved to initialize elements of 1D array
     */
    ValArray(std::valarray<T>&& values);
    /**
     * \brief Constructor creates a single 1D array of values.size () elements and 1 column,
     * and uses values std::vector<T> to initialize the elements.
     * \param values std::vector<T> that will be used to initialize elements of 1D array
     */
    explicit ValArray(const std::vector<T>& values);
    /**
     * \brief Constructor creates a single 2D array of numRows and numCols, and uses
     * std::valarray<T> values to initialize the elements.
     * \param numRows the number of rows
     * \param numCols the number of columns
     * \param values valarray<T> that will be used to initialize elements of 3D array
     */
    ValArray(size_t numRows, size_t numCols, const std::valarray<T>& values);
    /**
     * \brief Constructor creates a single 2D array of numRows and numCols, and moves
     * std::valarray<T> values to initialize the elements.
     * \param numRows the number of rows
     * \param numCols the number of columns
     * \param values valarray<T> that will be used to initialize elements of 3D array
     */
    ValArray(size_t numRows, size_t numCols, std::valarray<T>&& values);
    /**
     * \brief Constructor creates the 3D array of numRows x numCols x numPages dimensions,
     * and uses std::valarray<T> values to initialize all the 2D arrays, where first
     * numRows*numCols elements will belong to the first 2D array.
     * \param numRows the number of rows
     * \param numCols the number of columns
     * \param numPages the number of pages
     * \param values valarray<T> that will be used to initialize elements of 3D array
     */
    ValArray(size_t numRows, size_t numCols, size_t numPages, const std::valarray<T>& values);
    /**
     * \brief Constructor creates the 3D array of numRows x numCols x numPages dimensions,
     * and moves std::valarray<T> values to initialize all the 2D arrays, where first
     * numRows*numCols elements will belong to the first 2D array.
     * \param numRows the number of rows
     * \param numCols the number of columns
     * \param numPages the number of pages
     * \param values valarray<T> that will be used to initialize elements of 3D array
     */
    ValArray(size_t numRows, size_t numCols, size_t numPages, std::valarray<T>&& values);
    /** instruct the compiler to generate the implicitly declared destructor*/
    virtual ~ValArray() = default;
    /** instruct the compiler to generate the implicitly declared copy constructor*/
    ValArray(const ValArray<T>&) = default;
    /**
     * \brief Copy assignment operator.
     * Instruct the compiler to generate the implicitly declared copy assignment operator.
     * \return a reference to the assigned object
     */
    ValArray& operator=(const ValArray<T>&) = default;
    /** instruct the compiler to generate the implicitly declared move constructor*/
    ValArray(ValArray<T>&&) = default;
    /**
     * \brief Move assignment operator.
     * Instruct the compiler to generate the implicitly declared move assignment operator.
     * \return a reference to the assigned object
     */
    ValArray<T>& operator=(ValArray<T>&&) = default;
    /**
     * \returns Number of rows
     */
    size_t GetNumRows() const;
    /**
     * \returns Number of columns
     */
    size_t GetNumCols() const;
    /**
     * \returns Number of pages, i.e., the number of 2D arrays
     */
    size_t GetNumPages() const;
    /**
     * \returns Total number of elements
     */
    size_t GetSize() const;
    /**
     * \brief Access operator, with bound-checking in debug profile
     * \param rowIndex The index of the row
     * \param colIndex The index of the column
     * \param pageIndex The index of the page
     * \returns A const reference to the element with with rowIndex, colIndex and pageIndex indices.
     */
    T& operator()(size_t rowIndex, size_t colIndex, size_t pageIndex);
    /**
     * \brief Const access operator, with bound-checking in debug profile
     * \param rowIndex The index of the row
     * \param colIndex The index of the column
     * \param pageIndex The index of the page
     * \returns A const reference to the element with with rowIndex, colIndex and pageIndex indices.
     */
    const T& operator()(size_t rowIndex, size_t colIndex, size_t pageIndex) const;
    /**
     * \brief Access operator for 2D ValArrays.
     * Assuming that the third dimension is equal to 1, e.g. ValArray contains
     * a single 2D array.
     * Note: intentionally not implemented through three parameters access operator,
     * to avoid accidental mistakes by user, e.g., providing 2 parameters when
     * 3 are necessary, but access operator would return valid value if default
     * value of pages provided is 0.
     * \param rowIndex The index of the row
     * \param colIndex The index of the column
     * \returns A reference to the element with the specified indices
     */
    T& operator()(size_t rowIndex, size_t colIndex);
    /**
     * \brief Const access operator for 2D ValArrays.
     * Assuming that the third dimension is equal to 1, e.g. ValArray contains
     * a single 2D array.
     * \param rowIndex row index
     * \param colIndex column index
     * \returns a Const reference to the value with the specified row and column index.
     */
    const T& operator()(size_t rowIndex, size_t colIndex) const;
    /**
     * \brief Single-element access operator() for 1D ValArrays.
     * Assuming that the number of columns and pages is equal to 1, e.g. ValArray
     * contains a single column or a single row.
     *
     * Note: intentionally not implemented through three parameters access operator,
     * to avoid accidental mistakes by user, e.g., providing 1 parameters when
     * 2 or 3 are necessary.
     * \param index The index of the 1D ValArray.
     * \returns A reference to the value with the specified index.
     */
    T& operator()(size_t index);
    /**
     * \brief Single-element access operator() for 1D ValArrays.
     * \param index The index of the 1D ValArray.
     * \returns The const reference to the values with the specified index.
     */
    const T& operator()(size_t index) const;
    /**
     * \brief Element-wise multiplication with a scalar value.
     * \param rhs A scalar value of type T
     * \returns ValArray in which each element has been multiplied by the given
     * scalar value.
     */
    ValArray operator*(const T& rhs) const;
    /**
     * \brief operator+ definition for ValArray<T>.
     * \param rhs The rhs ValArray to be added to this ValArray.
     * \return the ValArray instance that holds the results of the operator+
     */
    ValArray operator+(const ValArray<T>& rhs) const;
    /**
     * \brief binary operator- definition for ValArray<T>.
     * \param rhs The rhs ValArray to be subtracted from this ValArray.
     * \return the ValArray instance that holds the results of the operator-
     */
    ValArray operator-(const ValArray<T>& rhs) const;
    /**
     * \brief unary operator- definition for ValArray<T>.
     * \return the ValArray instance that holds the results of the operator-
     */
    ValArray operator-() const;
    /**
     * \brief operator+= definition for ValArray<T>.
     * \param rhs The rhs ValArray to be added to this ValArray.
     * \return a reference to this ValArray instance
     */
    ValArray<T>& operator+=(const ValArray<T>& rhs);
    /**
     * \brief operator-= definition for ValArray<T>.
     * \param rhs The rhs ValArray to be subtracted from this ValArray.
     ** \return a reference to this ValArray instance
     */
    ValArray<T>& operator-=(const ValArray<T>& rhs);
    /**
     * \brief operator== definition for ValArray<T>.
     * \param rhs The ValArray instance to be compared with lhs ValArray instance
     * \return true if rhs ValArray is equal to this ValArray, otherwise it returns false
     */
    bool operator==(const ValArray<T>& rhs) const;
    /**
     * \brief operator!= definition for ValArray<T>.
     * \param rhs The ValArray instance to be compared with lhs ValArray instance
     * \return true if rhs ValArray is not equal to this ValArray, otherwise it returns true
     */
    bool operator!=(const ValArray<T>& rhs) const;
    /**
     * \brief Compare Valarray up to a given absolute tolerance. This operation
     * is element-wise operation, i.e., the elements with the same indices from
     * the lhs and rhs ValArray are being compared, allowing the tolerance defined
     * byt "tol" parameter.
     * \param rhs The rhs ValArray
     * \param tol The absolute tolerance
     * \returns true if the differences in each element-wise comparison is less
     * or equal to tol.
     */
    bool IsAlmostEqual(const ValArray<T>& rhs, T tol) const;
    /**
     * \brief Get a data pointer to a specific 2D array for use in linear
     * algebra libraries
     * \param pageIndex The index of the desired 2D array
     * \returns a pointer to the data elements of the 2D array
     */
    T* GetPagePtr(size_t pageIndex);
    /**
     * \brief Get a data pointer to a specific 2D array for use in linear
     * algebra libraries
     * \param pageIndex An index of the desired 2D array
     * \returns a pointer to the data elements of the 2D array
     */
    const T* GetPagePtr(size_t pageIndex) const;
    /**
     * \brief Checks whether rhs and lhs ValArray objects have the same dimensions.
     * \param rhs The rhs ValArray
     * \returns true if the dimensions of lhs and rhs are equal, otherwise it returns false
     */
    bool EqualDims(const ValArray<T>& rhs) const;
    /**
     * \brief Function that asserts if the dimensions of lhs and rhs ValArray are
     * not equal and prints a message with the matrices dimensions.
     * \param rhs the rhs ValArray
     */
    void AssertEqualDims(const ValArray<T>& rhs) const;
    /**
     * \brief Single-element access operator[] that can be used to access a specific
     * element of 1D ValArray. It mimics operator[] from std::vector.
     * This function is introduced for compatibility with ns-3 usage of 1D arrays,
     * which are usually represented through std::vector operators in spectrum
     * and antenna module.
     *
     * \param index The index of the element to be returned
     * \returns A reference to a specific element from the underlying std::valarray.
     */
    T& operator[](size_t index);
    /**
     * \brief Const access operator that can be used to access a specific element of
     * 1D ValArray.
     *
     * \param index The index of the element to be returned
     * \returns A const reference to a specific element from the underlying std::valarray.
     */
    const T& operator[](size_t index) const;
    /**
     * \brief Returns underlying values. This function allows to directly work
     * with the underlying values, which can be faster then using access operators.
     * \returns A const reference to the underlying std::valarray<T>.
     */
    const std::valarray<T>& GetValues() const;
    /**
     * \brief Alternative access operator to access a specific element.
     * \param row the row index of the element to be obtained
     * \param col the col index of the element to be obtained
     * \param page the page index of the element to be obtained
     * \return a reference to the element of this ValArray
     */
    T& Elem(size_t row, size_t col, size_t page);
    /**
     * \brief Alternative const access operator to access a specific element.
     * \param row the row index of the element to be obtained
     * \param col the column index of the element to be obtained
     * \param page the page index of the element to be obtained
     * \return a const reference to the element of this ValArray
     */
    const T& Elem(size_t row, size_t col, size_t page) const;

  protected:
    size_t m_numRows =
        0; //!< The size of the first dimension, i.e., the number of rows of each 2D array
    size_t m_numCols =
        0; //!< The size of the second dimension, i.e., the number of columns of each 2D array
    size_t m_numPages = 0;     //!< The size of the third dimension, i.e., the number of 2D arrays
    std::valarray<T> m_values; //!< The data values
};

/*************************************************
 **  Class ValArray inline implementations
 ************************************************/

template <class T>
inline size_t
ValArray<T>::GetNumRows() const
{
    return m_numRows;
}

template <class T>
inline size_t
ValArray<T>::GetNumCols() const
{
    return m_numCols;
}

template <class T>
inline size_t
ValArray<T>::GetNumPages() const
{
    return m_numPages;
}

template <class T>
inline size_t
ValArray<T>::GetSize() const
{
    return m_values.size();
}

template <class T>
inline T&
ValArray<T>::operator()(size_t rowIndex, size_t colIndex, size_t pageIndex)
{
    NS_ASSERT_MSG(rowIndex < m_numRows, "Row index out of bounds");
    NS_ASSERT_MSG(colIndex < m_numCols, "Column index out of bounds");
    NS_ASSERT_MSG(pageIndex < m_numPages, "Pages index out of bounds");
    size_t index = (rowIndex + m_numRows * (colIndex + m_numCols * pageIndex));
    return m_values[index];
}

template <class T>
inline const T&
ValArray<T>::operator()(size_t rowIndex, size_t colIndex, size_t pageIndex) const
{
    NS_ASSERT_MSG(rowIndex < m_numRows, "Row index out of bounds");
    NS_ASSERT_MSG(colIndex < m_numCols, "Column index out of bounds");
    NS_ASSERT_MSG(pageIndex < m_numPages, "Pages index out of bounds");
    size_t index = (rowIndex + m_numRows * (colIndex + m_numCols * pageIndex));
    return m_values[index];
}

template <class T>
inline T&
ValArray<T>::operator()(size_t rowIndex, size_t colIndex)
{
    NS_ASSERT_MSG(m_numPages == 1, "Cannot use 2D access operator for 3D ValArray.");
    return (*this)(rowIndex, colIndex, 0);
}

template <class T>
inline const T&
ValArray<T>::operator()(size_t rowIndex, size_t colIndex) const
{
    NS_ASSERT_MSG(m_numPages == 1, "Cannot use 2D access operator for 3D ValArray.");
    return (*this)(rowIndex, colIndex, 0);
}

template <class T>
inline T&
ValArray<T>::operator()(size_t index)
{
    NS_ASSERT_MSG(index < m_values.size(),
                  "Invalid index to 1D ValArray. The size of the array should be set through "
                  "constructor.");
    NS_ASSERT_MSG(((m_numRows == 1 || m_numCols == 1) && (m_numPages == 1)) ||
                      (m_numRows == 1 && m_numCols == 1),
                  "Access operator allowed only for 1D ValArray.");
    return m_values[index];
}

template <class T>
inline const T&
ValArray<T>::operator()(size_t index) const
{
    NS_ASSERT_MSG(index < m_values.size(),
                  "Invalid index to 1D ValArray.The size of the array should be set through "
                  "constructor.");
    NS_ASSERT_MSG(((m_numRows == 1 || m_numCols == 1) && (m_numPages == 1)) ||
                      (m_numRows == 1 && m_numCols == 1),
                  "Access operator allowed only for 1D ValArray.");
    return m_values[index];
}

template <class T>
inline ValArray<T>
ValArray<T>::operator*(const T& rhs) const
{
    return ValArray<T>(m_numRows,
                       m_numCols,
                       m_numPages,
                       m_values * std::valarray<T>(rhs, m_numRows * m_numCols * m_numPages));
}

template <class T>
inline ValArray<T>
ValArray<T>::operator+(const ValArray<T>& rhs) const
{
    AssertEqualDims(rhs);
    return ValArray<T>(m_numRows, m_numCols, m_numPages, m_values + rhs.m_values);
}

template <class T>
inline ValArray<T>
ValArray<T>::operator-(const ValArray<T>& rhs) const
{
    AssertEqualDims(rhs);
    return ValArray<T>(m_numRows, m_numCols, m_numPages, m_values - rhs.m_values);
}

template <class T>
inline ValArray<T>
ValArray<T>::operator-() const
{
    return ValArray<T>(m_numRows, m_numCols, m_numPages, -m_values);
}

template <class T>
inline ValArray<T>&
ValArray<T>::operator+=(const ValArray<T>& rhs)
{
    AssertEqualDims(rhs);
    m_values += rhs.m_values;
    return *this;
}

template <class T>
inline ValArray<T>&
ValArray<T>::operator-=(const ValArray<T>& rhs)
{
    AssertEqualDims(rhs);
    m_values -= rhs.m_values;
    return *this;
}

template <class T>
inline T*
ValArray<T>::GetPagePtr(size_t pageIndex)
{
    NS_ASSERT_MSG(pageIndex < m_numPages, "Invalid page index.");
    return &(m_values[m_numRows * m_numCols * pageIndex]);
}

template <class T>
inline const T*
ValArray<T>::GetPagePtr(size_t pageIndex) const
{
    NS_ASSERT_MSG(pageIndex < m_numPages, "Invalid page index.");
    return &(m_values[m_numRows * m_numCols * pageIndex]);
}

template <class T>
inline bool
ValArray<T>::EqualDims(const ValArray<T>& rhs) const
{
    return (m_numRows == rhs.m_numRows) && (m_numCols == rhs.m_numCols) &&
           (m_numPages == rhs.m_numPages);
}

template <class T>
inline T&
ValArray<T>::operator[](size_t index)
{
    return (*this)(index);
}

template <class T>
inline const T&
ValArray<T>::operator[](size_t index) const
{
    return (*this)(index);
}

template <class T>
inline const std::valarray<T>&
ValArray<T>::GetValues() const
{
    return m_values;
}

template <class T>
inline T&
ValArray<T>::Elem(size_t row, size_t col, size_t page)
{
    return (*this)(row, col, page);
}

template <class T>
inline const T&
ValArray<T>::Elem(size_t row, size_t col, size_t page) const
{
    return (*this)(row, col, page);
}

/*************************************************
 **  Class ValArray non-inline implementations
 ************************************************/

template <class T>
ValArray<T>::ValArray(size_t numRows, size_t numCols, size_t numPages)
    : m_numRows{numRows},
      m_numCols{numCols},
      m_numPages{numPages}
{
    m_values.resize(m_numRows * m_numCols * m_numPages);
}

template <class T>
ValArray<T>::ValArray(const std::valarray<T>& values)
    : m_numRows{values.size()},
      m_numCols{1},
      m_numPages{1},
      m_values{values}
{
}

template <class T>
ValArray<T>::ValArray(std::valarray<T>&& values)
    : m_numRows{values.size()},
      m_numCols{1},
      m_numPages{1},
      m_values{std::move(values)}
{
}

template <class T>
ValArray<T>::ValArray(const std::vector<T>& values)
    : m_numRows{values.size()},
      m_numCols{1},
      m_numPages{1}
{
    m_values.resize(values.size());
    std::copy(values.begin(), values.end(), std::begin(m_values));
}

template <class T>
ValArray<T>::ValArray(size_t numRows, size_t numCols, const std::valarray<T>& values)
    : m_numRows{numRows},
      m_numCols{numCols},
      m_numPages{1},
      m_values{values}
{
    NS_ASSERT_MSG(m_numRows * m_numCols == values.size(),
                  "Dimensions and the initialization array size do not match.");
}

template <class T>
ValArray<T>::ValArray(size_t numRows, size_t numCols, std::valarray<T>&& values)
    : m_numRows{numRows},
      m_numCols{numCols},
      m_numPages{1}
{
    NS_ASSERT_MSG(m_numRows * m_numCols == values.size(),
                  "Dimensions and the initialization array size do not match.");
    m_values = std::move(values);
}

template <class T>
ValArray<T>::ValArray(size_t numRows,
                      size_t numCols,
                      size_t numPages,
                      const std::valarray<T>& values)
    : m_numRows{numRows},
      m_numCols{numCols},
      m_numPages{numPages},
      m_values{values}
{
    NS_ASSERT_MSG(m_numRows * m_numCols * m_numPages == values.size(),
                  "Dimensions and the initialization array size do not match.");
}

template <class T>
ValArray<T>::ValArray(size_t numRows, size_t numCols, size_t numPages, std::valarray<T>&& values)
    : m_numRows{numRows},
      m_numCols{numCols},
      m_numPages{numPages}
{
    NS_ASSERT_MSG(m_numRows * m_numCols * m_numPages == values.size(),
                  "Dimensions and the initialization array size do not match.");
    m_values = std::move(values);
}

template <class T>
bool
ValArray<T>::operator==(const ValArray<T>& rhs) const
{
    return EqualDims(rhs) &&
           std::equal(std::begin(m_values), std::end(m_values), std::begin(rhs.m_values));
}

template <class T>
bool
ValArray<T>::operator!=(const ValArray<T>& rhs) const
{
    return !((*this) == rhs);
}

template <class T>
bool
ValArray<T>::IsAlmostEqual(const ValArray<T>& rhs, T tol) const
{
    return EqualDims(rhs) && std::equal(std::begin(m_values),
                                        std::end(m_values),
                                        std::begin(rhs.m_values),
                                        [tol](T lhsValue, T rhsValue) {
                                            return lhsValue == rhsValue ||
                                                   std::abs(lhsValue - rhsValue) <= std::abs(tol);
                                        });
}

template <class T>
void
ValArray<T>::AssertEqualDims(const ValArray<T>& rhs) const
{
    NS_ASSERT_MSG(EqualDims(rhs),
                  "Dimensions mismatch: "
                  "lhs (rows, cols, pages) = ("
                      << m_numRows << ", " << m_numCols << ", " << m_numPages
                      << ") and "
                         "rhs (rows, cols, pages) = ("
                      << rhs.m_numRows << ", " << rhs.m_numCols << ", " << rhs.m_numPages << ")");
}

/**
 * \brief Overloads output stream operator.
 * \tparam T the type of the ValArray for which will be called this function
 * \param os a reference to the output stream
 * \param a the ValArray instance using type T
 * \return a reference to the output stream
 */
template <class T>
std::ostream&
operator<<(std::ostream& os, const ValArray<T>& a)
{
    os << "\n";
    for (size_t p = 0; p != a.GetNumPages(); ++p)
    {
        os << "Page " << p << ":\n";
        for (size_t i = 0; i != a.GetNumRows(); ++i)
        {
            for (size_t j = 0; j != a.GetNumCols(); ++j)
            {
                os << "\t" << a(i, j, p);
            }
            os << "\n";
        }
    }
    return os;
}

} // namespace ns3

#endif // VAL_ARRAY_H
