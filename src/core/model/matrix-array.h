/*
 * Copyright (c) 2022 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Biljana Bojovic <bbojovic@cttc.es>
 */

#ifndef MATRIX_ARRAY_H
#define MATRIX_ARRAY_H

#include "val-array.h"

#include <valarray>

namespace ns3
{

/**
 * @ingroup Matrices
 *
 * @brief MatrixArray class inherits ValArray class and provides additional
 * interfaces to ValArray which enable page-wise linear algebra operations for
 * arrays of matrices. While ValArray class is focused on efficient storage,
 * access and basic algebra element-wise operations on 3D numerical arrays,
 * MatrixArray is focused on the efficient algebra operations on arrays of
 * matrices.
 *
 * Each page (2-dimensional array) within a 3D array is considered to
 * be a matrix, and operations are performed separately for each page/matrix.
 * For example, a multiplication of two MatrixArrays means that a matrix
 * multiplication is performed between each page in the first MatrixArray with
 * the corresponding page in the second MatrixArray. Some of the operations can
 * use the Eigen3 matrix library for speedup. In addition, this class can be
 * extended to provide interfaces to the Eigen3 matrix library and enable
 * additional linear algebra operations. The objective of this class is to two-fold:
 *
 * - First, it provides simple interfaces to perform matrix operations in parallel
 * on all pages without the need to write an outer loop over all the pages.
 *
 * - Second, it can improve the performance of such parallel matrix operations,
 * as this class uses a single memory allocation and one common set of size
 * variables (rows, cols) for all pages, which can be an advantage compared to
 * having separate memory allocations and size variables for each page/matrix,
 * especially when those matrices are very small.
 *
 * Important characteristics:
 *
 * - Matrices are in column-major order.
 *
 * - Matrices can be 1-dimensional, in the sense that either the number of rows
 * or columns is equal to 1.
 *
 * - The array of matrices can contain just one matrix or more matrices.
 *
 * - For binary operators, both MatrixArrays must have an equal number of pages,
 * as well compatible row and column dimensions between the matrices.
 *
 * - Binary operators are performed among matrices with the same index from lhs
 * and rhs matrix.
 *
 * - MatrixArray improves computationally complex matrix operations by using
 * Eigen library when it is available at compile time. MatrixArray class makes
 * use of Eigen Map class, which allows re-using already existing portion of
 * memory as an Eigen type, e.g. ValArray can return a pointer to a memory
 * where are placed elements of a specific matrix, and this can be passed to
 * Eigen Map class which does not do a copy of the elements, but is using these
 * elements directly in linear algebra operations.
 */
template <class T>
class MatrixArray : public ValArray<T>
{
  public:
    // instruct the compiler to generate the default constructor
    MatrixArray() = default;
    /**
     * @brief Constructor "pages" number of matrices that are of size "numRows"x"numCols",
     * and are initialized with all-zero elements.
     * If only 1 parameter, numRows, is provided then a single 1D array is being created.
     * @param numRows the number of rows
     * @param numCols the number of columns
     * @param numPages the number of pages
     */
    MatrixArray(size_t numRows, size_t numCols = 1, size_t numPages = 1);
    /**
     * @brief Constructor creates a single array of values.size () elements and 1 column,
     * and uses std::valarray<T> values to initialize the elements.
     * @param values std::valarray<T> that will be used to initialize elements of 1D array
     */
    explicit MatrixArray(const std::valarray<T>& values);
    /**
     * @brief Constructor creates a single array of values.size () elements and 1 column,
     * and moves std::valarray<T> values to initialize the elements.
     * @param values std::valarray<T> that will be moved to initialize elements of 1D array
     */
    MatrixArray(std::valarray<T>&& values);
    /**
     * @brief Constructor creates a single array of values.size () elements and 1 column,
     * and uses std::vector<T> values to initialize the elements.
     * @param values std::vector<T> that will be used to initialize elements of 1D array
     */
    explicit MatrixArray(const std::vector<T>& values);
    /**
     * @brief Constructor creates a single matrix of numRows and numCols, and uses
     * std::valarray<T> values to initialize the elements.
     * @param numRows the number of rows
     * @param numCols the number of columns
     * @param values std::valarray<T> that will be used to initialize elements of 3D array
     */
    MatrixArray(size_t numRows, size_t numCols, const std::valarray<T>& values);
    /**
     * @brief Constructor creates a single matrix of numRows and numCols, and moves
     * std::valarray<T> values to initialize the elements.
     * @param numRows the number of rows
     * @param numCols the number of columns
     * @param values std::valarray<T> that will be moved to initialize elements of 3D array
     */
    MatrixArray(size_t numRows, size_t numCols, std::valarray<T>&& values);
    /**
     * @brief Constructor creates the array of numPages matrices of numRows x numCols dimensions,
     * and uses std::valarray<T> values to initialize all the elements.
     * @param numRows the number of rows
     * @param numCols the number of columns
     * @param numPages the number of pages
     * @param values std::valarray<T> that will be used to initialize elements of 3D array
     */
    MatrixArray(size_t numRows, size_t numCols, size_t numPages, const std::valarray<T>& values);
    /**
     * @brief Constructor creates the array of numPages matrices of numRows x numCols dimensions,
     * and moves std::valarray<T> values to initialize all the elements.
     * @param numRows the number of rows
     * @param numCols the number of columns
     * @param numPages the number of pages
     * @param values std::valarray<T> that will be used to initialize elements of 3D array
     */
    MatrixArray(size_t numRows, size_t numCols, size_t numPages, std::valarray<T>&& values);

    /**
     * @brief Element-wise multiplication with a scalar value.
     * @param rhs is a scalar value of type T
     * @returns The matrix in which each element has been multiplied by the given
     * scalar value.
     */
    MatrixArray operator*(const T& rhs) const;
    /**
     * @brief operator+ definition for MatrixArray<T>.
     * @param rhs The rhs MatrixArray object
     * @returns The result of operator+ of this MatrixArray and rhs MatrixArray
     */
    MatrixArray operator+(const MatrixArray<T>& rhs) const;
    /**
     * @brief binary operator- definition for MatrixArray<T>.
     * @param rhs The rhs MatrixArray object
     * @returns The result of operator- of this MatrixArray and rhs MatrixArray
     */
    MatrixArray operator-(const MatrixArray<T>& rhs) const;
    /**
     * @brief unary operator- definition for MatrixArray<T>.
     * @return the result of the operator-
     */
    MatrixArray operator-() const;
    /**
     * @brief Page-wise matrix multiplication.
     * This operator interprets the 3D array as an array of matrices,
     * and performs a linear algebra operation on each of the matrices (pages),
     * i.e., matrices from this MatrixArray and rhs with the same page indices are
     * multiplied.
     * The number of columns of this MatrixArray must be equal to the number of
     * rows in rhs, and rhs must have the same number of pages as this MatrixArray.
     * @param rhs is another MatrixArray instance
     * @returns The array of results of matrix multiplications.
     */
    MatrixArray operator*(const MatrixArray<T>& rhs) const;
    /**
     * @brief This operator interprets the 3D array as an array of matrices,
     * and performs a linear algebra operation on each of the matrices (pages),
     * i.e., transposes the matrix or array of matrices definition for MatrixArray<T>.
     * @return The resulting MatrixArray composed of the array of transposed matrices.
     */
    MatrixArray Transpose() const;
    /**
     * @brief This operator calculates a vector o determinants, one for each page
     * @return The resulting MatrixArray with the determinants for each page.
     */
    MatrixArray Determinant() const;
    /**
     * @brief This operator calculates a vector of Frobenius norm, one for each page
     * @return The resulting MatrixArray with the Frobenius norm for each page.
     */
    MatrixArray FrobeniusNorm() const;
    /**
     * @brief Multiply each matrix in the array by the left and the right matrix.
     * For each page of this MatrixArray the operation performed is
     * lMatrix * matrix(pageIndex) * rMatrix, and the resulting MatrixArray<T>
     * contains the array of the results per page. If "this" has dimensions
     * M x N x P, lMatrix must have dimensions M number of columns, and a single
     * page, and rMatrix must have N number of rows, and also a single page.
     *
     * The result of this function will have the dimensions J x K x P. Where J is
     * the number of rows of the lMatrix, and K is the number of
     * columns of rMatrix. Dimensions are:
     *
     * lMatrix(JxMx1) * this (MxNxP) * rMatrix(NxKx1) -> result(JxKxP)
     *
     * This operation is not possible when using the multiplication operator because
     * the number of pages does not match.
     *
     * @param lMatrix the left matrix in the multiplication
     * @param rMatrix the right matrix in the multiplication
     * @returns Returns the result of the multiplication which is a 3D MatrixArray
     * with dimensions J x K x P as explained previously.
     */
    MatrixArray MultiplyByLeftAndRightMatrix(const MatrixArray<T>& lMatrix,
                                             const MatrixArray<T>& rMatrix) const;

    using ValArray<T>::GetPagePtr;
    using ValArray<T>::EqualDims;
    using ValArray<T>::AssertEqualDims;

    /**
     * @brief Function that performs the Hermitian transpose of this MatrixArray
     * and returns a new matrix that is the result of the operation.
     * This function assumes that the matrix contains complex numbers, because of that
     * it is only available for the <std::complex<double>> specialization of MatrixArray.
     * @return Returns a new matrix that is the result of the Hermitian transpose
     */
    template <bool EnableBool = true,
              typename = std::enable_if_t<(std::is_same_v<T, std::complex<double>> && EnableBool)>>
    MatrixArray<T> HermitianTranspose() const;

    /**
     * @brief Function that copies the current 1-page matrix into a new matrix with n copies of the
     * original matrix
     * @param nCopies Number of copies to make of the input 1-page MatrixArray
     * @return Returns a new matrix with n page copies
     */
    MatrixArray<T> MakeNCopies(size_t nCopies) const;
    /**
     * @brief Function extracts a page from a MatrixArray
     * @param page Index of the page to be extracted from the n-page MatrixArray
     * @return Returns an extracted page of the original MatrixArray
     */
    MatrixArray<T> ExtractPage(size_t page) const;
    /**
     * @brief Function joins multiple pages into a single MatrixArray
     * @param pages Vector containing n 1-page MatrixArrays to be merged into a n-page MatrixArray
     * @return Returns a joint MatrixArray
     */
    static MatrixArray<T> JoinPages(const std::vector<MatrixArray<T>>& pages);
    /**
     * @brief Function produces an identity MatrixArray with the specified size
     * @param size Size of the output identity matrix
     * @param pages Number of copies of the identity matrix
     * @return Returns an identity MatrixArray
     */
    static MatrixArray<T> IdentityMatrix(const size_t size, const size_t pages = 1);
    /**
     * @brief Function produces an identity MatrixArray with the same size
     * as the input MatrixArray
     * @param likeme Input matrix used to determine the size of the identity matrix
     * @return Returns an identity MatrixArray
     */
    static MatrixArray<T> IdentityMatrix(const MatrixArray& likeme);

  protected:
    // To simplify functions in MatrixArray that are using members from the template base class
    using ValArray<T>::m_numRows;
    using ValArray<T>::m_numCols;
    using ValArray<T>::m_numPages;
    using ValArray<T>::m_values;
};

/// Create an alias for MatrixArray using int type
using IntMatrixArray = MatrixArray<int>;

/// Create an alias for MatrixArray using double type
using DoubleMatrixArray = MatrixArray<double>;

/// Create an alias for MatrixArray using complex type
using ComplexMatrixArray = MatrixArray<std::complex<double>>;

/*************************************************
 **  Class MatrixArray inline implementations
 ************************************************/
template <class T>
inline MatrixArray<T>
MatrixArray<T>::operator*(const T& rhs) const
{
    return MatrixArray<T>(m_numRows,
                          m_numCols,
                          m_numPages,
                          m_values * std::valarray<T>(rhs, m_numRows * m_numCols * m_numPages));
}

template <class T>
inline MatrixArray<T>
MatrixArray<T>::operator+(const MatrixArray<T>& rhs) const
{
    AssertEqualDims(rhs);
    return MatrixArray<T>(m_numRows, m_numCols, m_numPages, m_values + rhs.m_values);
}

template <class T>
inline MatrixArray<T>
MatrixArray<T>::operator-(const MatrixArray<T>& rhs) const
{
    AssertEqualDims(rhs);
    return MatrixArray<T>(m_numRows, m_numCols, m_numPages, m_values - rhs.m_values);
}

template <class T>
inline MatrixArray<T>
MatrixArray<T>::operator-() const
{
    return MatrixArray<T>(m_numRows, m_numCols, m_numPages, -m_values);
}

} // namespace ns3

#endif // MATRIX_ARRAY_H
