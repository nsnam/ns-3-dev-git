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

template MatrixArray<std::complex<double>> MatrixArray<std::complex<double>>::HermitianTranspose()
    const;
template class MatrixArray<std::complex<double>>;
template class MatrixArray<double>;
template class MatrixArray<int>;

} // namespace ns3
