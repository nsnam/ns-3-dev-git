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

#include "matrix-array.h"

#ifdef HAVE_EIGEN3
#include <Eigen/Dense>
#endif

namespace ns3
{

#ifdef HAVE_EIGEN3
template <class T>
using EigenMatrix = Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>>;
template <class T>
using ConstEigenMatrix = Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>>;
#endif

template <class T>
MatrixArray<T>::MatrixArray(size_t numRows, size_t numCols, size_t numPages)
    : ValArray<T>(numRows, numCols, numPages)
{
}

template <class T>
MatrixArray<T>::MatrixArray(const std::valarray<T>& values)
    : ValArray<T>(values)
{
}

template <class T>
MatrixArray<T>::MatrixArray(std::valarray<T>&& values)
    : ValArray<T>(std::move(values))
{
}

template <class T>
MatrixArray<T>::MatrixArray(const std::vector<T>& values)
    : ValArray<T>(values)
{
}

template <class T>
MatrixArray<T>::MatrixArray(size_t numRows, size_t numCols, const std::valarray<T>& values)
    : ValArray<T>(numRows, numCols, values)
{
}

template <class T>
MatrixArray<T>::MatrixArray(size_t numRows, size_t numCols, std::valarray<T>&& values)
    : ValArray<T>(numRows, numCols, std::move(values))
{
}

template <class T>
MatrixArray<T>::MatrixArray(size_t numRows,
                            size_t numCols,
                            size_t numPages,
                            const std::valarray<T>& values)
    : ValArray<T>(numRows, numCols, numPages, values)
{
}

template <class T>
MatrixArray<T>::MatrixArray(size_t numRows,
                            size_t numCols,
                            size_t numPages,
                            std::valarray<T>&& values)
    : ValArray<T>(numRows, numCols, numPages, std::move(values))
{
}

template <class T>
MatrixArray<T>
MatrixArray<T>::operator*(const MatrixArray<T>& rhs) const
{
    NS_ASSERT_MSG(m_numPages == rhs.m_numPages, "MatrixArrays have different numbers of matrices.");
    NS_ASSERT_MSG(m_numCols == rhs.m_numRows, "Inner dimensions of matrices mismatch.");

    MatrixArray<T> res{m_numRows, rhs.m_numCols, m_numPages};

    for (size_t page = 0; page < res.m_numPages; ++page)
    {
#ifdef HAVE_EIGEN3 // Eigen found and enabled Eigen optimizations

        ConstEigenMatrix<T> lhsEigenMatrix(GetPagePtr(page), m_numRows, m_numCols);
        ConstEigenMatrix<T> rhsEigenMatrix(rhs.GetPagePtr(page), rhs.m_numRows, rhs.m_numCols);
        EigenMatrix<T> resEigenMatrix(res.GetPagePtr(page), res.m_numRows, res.m_numCols);
        resEigenMatrix = lhsEigenMatrix * rhsEigenMatrix;

#else // Eigen not found or Eigen optimizations not enabled

        size_t matrixOffset = page * m_numRows * m_numCols;
        size_t rhsMatrixOffset = page * rhs.m_numRows * rhs.m_numCols;
        for (size_t i = 0; i < res.m_numRows; ++i)
        {
            for (size_t j = 0; j < res.m_numCols; ++j)
            {
                res(i, j, page) = (m_values[std::slice(matrixOffset + i, m_numCols, m_numRows)] *
                                   rhs.m_values[std::slice(rhsMatrixOffset + j * rhs.m_numRows,
                                                           rhs.m_numRows,
                                                           1)])
                                      .sum();
            }
        }

#endif
    }
    return res;
}

template <class T>
MatrixArray<T>
MatrixArray<T>::Transpose() const
{
    // Create the matrix where m_numRows = this.m_numCols, m_numCols = this.m_numRows,
    // m_numPages = this.m_numPages
    MatrixArray<T> res{m_numCols, m_numRows, m_numPages};

    for (size_t page = 0; page < m_numPages; ++page)
    {
#ifdef HAVE_EIGEN3 // Eigen found and Eigen optimizations enabled

        ConstEigenMatrix<T> thisMatrix(GetPagePtr(page), m_numRows, m_numCols);
        EigenMatrix<T> resEigenMatrix(res.GetPagePtr(page), res.m_numRows, res.m_numCols);
        resEigenMatrix = thisMatrix.transpose();

#else // Eigen not found or Eigen optimizations not enabled

        size_t matrixIndex = page * m_numRows * m_numCols;
        for (size_t i = 0; i < m_numRows; ++i)
        {
            res.m_values[std::slice(matrixIndex + i * res.m_numRows, res.m_numRows, 1)] =
                m_values[std::slice(matrixIndex + i, m_numCols, m_numRows)];
        }

#endif
    }
    return res;
}

template <class T>
MatrixArray<T>
MatrixArray<T>::Determinant() const
{
    MatrixArray<T> res{1, 1, m_numPages};
    NS_ASSERT_MSG(m_numRows == m_numCols, "Matrix is not square");
    // In case of small matrices, we use a fast path
    if (m_numRows == 1)
    {
        return *this;
    }
    // Calculate determinant for each matrix
    for (size_t page = 0; page < m_numPages; ++page)
    {
        res(0, 0, page) = 0;
        auto pageValues = GetPagePtr(page);

        // Fast path for 2x2 matrices
        if (m_numRows == 2)
        {
            res(0, 0, page) = pageValues[0] * pageValues[3] - pageValues[1] * pageValues[2];
            continue;
        }
        for (size_t detN = 0; detN < m_numRows; detN++)
        {
            auto partDetP = T{0} + 1.0;
            auto partDetN = T{0} + 1.0;
            for (size_t row = 0; row < m_numRows; row++)
            {
                // Wraparound not to have to extend the matrix
                // Positive determinant
                size_t col = (row + detN) % m_numCols;
                partDetP *= pageValues[row * m_numCols + col];

                // Negative determinant
                col = m_numCols - 1 - (row + detN) % m_numCols;
                partDetN *= pageValues[row * m_numCols + col];
            }
            res(0, 0, page) += partDetP - partDetN;
        }
    }
    return res;
}

template <class T>
MatrixArray<T>
MatrixArray<T>::FrobeniusNorm() const
{
    MatrixArray<T> res{1, 1, m_numPages};
    for (size_t page = 0; page < m_numPages; ++page)
    {
        // Calculate the sum of squared absolute values of each matrix page
        res[page] = 0;
        auto pagePtr = this->GetPagePtr(page);
        for (size_t i = 0; i < m_numRows * m_numCols; i++)
        {
            auto absVal = std::abs(pagePtr[i]);
            res[page] += absVal * absVal;
        }
        res[page] = sqrt(res[page]);
    }
    return res;
}

template <class T>
MatrixArray<T>
MatrixArray<T>::MultiplyByLeftAndRightMatrix(const MatrixArray<T>& lMatrix,
                                             const MatrixArray<T>& rMatrix) const
{
    NS_ASSERT_MSG(lMatrix.m_numPages == 1 && rMatrix.m_numPages == 1,
                  "The left and right MatrixArray should have only one page.");
    NS_ASSERT_MSG(lMatrix.m_numCols == m_numRows,
                  "Left vector numCols and this MatrixArray numRows mismatch.");
    NS_ASSERT_MSG(m_numCols == rMatrix.m_numRows,
                  "Right vector numRows and this MatrixArray numCols mismatch.");

    MatrixArray<T> res{lMatrix.m_numRows, rMatrix.m_numCols, m_numPages};

#ifdef HAVE_EIGEN3

    ConstEigenMatrix<T> lMatrixEigen(lMatrix.GetPagePtr(0), lMatrix.m_numRows, lMatrix.m_numCols);
    ConstEigenMatrix<T> rMatrixEigen(rMatrix.GetPagePtr(0), rMatrix.m_numRows, rMatrix.m_numCols);
#endif

    for (size_t page = 0; page < m_numPages; ++page)
    {
#ifdef HAVE_EIGEN3 // Eigen found and Eigen optimizations enabled

        ConstEigenMatrix<T> matrixEigen(GetPagePtr(page), m_numRows, m_numCols);
        EigenMatrix<T> resEigenMap(res.GetPagePtr(page), res.m_numRows, res.m_numCols);

        resEigenMap = lMatrixEigen * matrixEigen * rMatrixEigen;

#else // Eigen not found or Eigen optimizations not enabled

        size_t matrixOffset = page * m_numRows * m_numCols;
        for (size_t resRow = 0; resRow < res.m_numRows; ++resRow)
        {
            for (size_t resCol = 0; resCol < res.m_numCols; ++resCol)
            {
                // create intermediate row result, a multiply of resRow row of lMatrix and each
                // column of this matrix
                std::valarray<T> interRes(m_numCols);
                for (size_t thisCol = 0; thisCol < m_numCols; ++thisCol)
                {
                    interRes[thisCol] =
                        (lMatrix
                             .m_values[std::slice(resRow, lMatrix.m_numCols, lMatrix.m_numRows)] *
                         m_values[std::slice(matrixOffset + thisCol * m_numRows, m_numRows, 1)])
                            .sum();
                }
                // multiply intermediate results and resCol column of the rMatrix
                res(resRow, resCol, page) =
                    (interRes *
                     rMatrix.m_values[std::slice(resCol * rMatrix.m_numRows, rMatrix.m_numRows, 1)])
                        .sum();
            }
        }
#endif
    }
    return res;
}

template <class T>
template <bool EnableBool, typename>
MatrixArray<T>
MatrixArray<T>::HermitianTranspose() const
{
    MatrixArray<std::complex<double>> retMatrix = this->Transpose();

    for (size_t index = 0; index < this->GetSize(); ++index)
    {
        retMatrix.m_values[index] = std::conj(retMatrix.m_values[index]);
    }
    return retMatrix;
}

template <class T>
MatrixArray<T>
MatrixArray<T>::MakeNCopies(size_t nCopies) const
{
    NS_ASSERT_MSG(m_numPages == 1, "The MatrixArray should have only one page to be copied.");
    auto copiedMatrix = MatrixArray<T>{m_numRows, m_numCols, nCopies};
    for (size_t copy = 0; copy < nCopies; copy++)
    {
        for (size_t i = 0; i < m_numRows * m_numCols; i++)
        {
            copiedMatrix.GetPagePtr(copy)[i] = m_values[i];
        }
    }
    return copiedMatrix;
}

template <class T>
MatrixArray<T>
MatrixArray<T>::ExtractPage(size_t page) const
{
    NS_ASSERT_MSG(page < m_numPages, "The page to extract from the MatrixArray is out of bounds.");
    auto extractedPage = MatrixArray<T>{m_numRows, m_numCols, 1};

    for (size_t i = 0; i < m_numRows * m_numCols; ++i)
    {
        extractedPage.m_values[i] = GetPagePtr(page)[i];
    }
    return extractedPage;
}

template <class T>
MatrixArray<T>
MatrixArray<T>::JoinPages(const std::vector<MatrixArray<T>>& pages)
{
    auto jointMatrix =
        MatrixArray<T>{pages.front().GetNumRows(), pages.front().GetNumCols(), pages.size()};
    for (size_t page = 0; page < jointMatrix.GetNumPages(); page++)
    {
        NS_ASSERT_MSG(pages[page].GetNumRows() == jointMatrix.GetNumRows(),
                      "All page matrices should have the same number of rows");
        NS_ASSERT_MSG(pages[page].GetNumCols() == jointMatrix.GetNumCols(),
                      "All page matrices should have the same number of columns");
        NS_ASSERT_MSG(pages[page].GetNumPages() == 1,
                      "All page matrices should have a single page");

        size_t i = 0;
        for (auto a : pages[page].GetValues())
        {
            jointMatrix.GetPagePtr(page)[i] = a;
            i++;
        }
    }
    return jointMatrix;
}

template <class T>
MatrixArray<T>
MatrixArray<T>::IdentityMatrix(const size_t size, const size_t pages)
{
    auto identityMatrix = MatrixArray<T>{size, size, pages};
    for (std::size_t page = 0; page < pages; page++)
    {
        for (std::size_t i = 0; i < size; i++)
        {
            identityMatrix(i, i, page) = 1.0;
        }
    }
    return identityMatrix;
}

template <class T>
MatrixArray<T>
MatrixArray<T>::IdentityMatrix(const MatrixArray& likeme)
{
    NS_ASSERT_MSG(likeme.GetNumRows() == likeme.GetNumCols(), "Template array is not square.");
    return IdentityMatrix(likeme.GetNumRows(), likeme.GetNumPages());
}

template MatrixArray<std::complex<double>> MatrixArray<std::complex<double>>::HermitianTranspose()
    const;
template class MatrixArray<std::complex<double>>;
template class MatrixArray<double>;
template class MatrixArray<int>;

} // namespace ns3
