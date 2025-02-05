/* Copyright (c) 2025 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#ifndef NS3_SYMMETRIC_ADJACENCY_MATRIX_H
#define NS3_SYMMETRIC_ADJACENCY_MATRIX_H

#include <vector>

namespace ns3
{

/**
 * @brief A class representing a symmetric adjacency matrix.
 *
 * Since the matrix is symmetric, we save up on memory by
 * storing only the lower left triangle, including the main
 * diagonal.
 *
 * In pseudocode, the matrix is stored as a vector m_matrix, where
 * each new row is accessed via an offset precomputed in m_rowOffsets.
 * We also keep track of the number of rows in m_rows.
 *
 * A 4x4 matrix would be represented as follows:
 *
 * @code
 * m_matrix= [
 * 0
 * 1 2
 * 3 4 5
 * 6 7 8 9
 * ];
 * m_rowOffsets = [0, 1, 3, 6];
 * m_rows = 4;
 * @endcode
 *
 * To add a new row (`AddRow()`) in the adjacency matrix (equivalent to an additional node in a
 bidirected graph),
 * we need to first add a new offset, then increment the number of rows and finally resize the
 vector.
 *
 * @code
 * m_rowOffsets.push_back(m_matrix.size());
 * m_rows++;
 * m_matrix.resize(m_matrix.size()+m_rows);
 * @endcode
 *
 * The resulting state would be:
 *
 * @code
 * m_rowOffsets = [0, 1, 3, 6, 10];
 * m_rows = 5;
 * m_matrix= [
 *  0
 *  1  2
 *  3  4  5
 *  6  7  8  9
 * 10 11 12 13 14
 * ];
 * @endcode
 *
 * In this previous example, the elements of the matrix are
 * the offset of the values from the beginning of the vector.
 *
 * In practice, this matrix could store the state between a given
 * pair of a link between two nodes. The state could be a boolean
 * value, in case just tracking valid/invalid,
 * connected/disconnected link, or numerical types to store
 * weights, which can be used for routing algorithms.
 *
 * The `adjacency-matrix-example` illustrates the usage of the adjacency matrix
 * in a routing example.
 *
 * First we set up the matrix with capacity for 10 nodes.
 * All values are initialized to maximum, to indicate a disconnected node.
 *
 * @code
 *  constexpr float maxFloat = std::numeric_limits<float>::max();
 *  // Create routing weight matrix for 10 nodes and initialize weights to infinity (disconnected)
 *  ns3::SymmetricAdjacencyMatrix<float> routeWeights(10, maxFloat);
 * @endcode
 *
 * We can then map graph nodes into the table rows
 * @code
 *   // Node | Corresponding matrix row
 *   //  A   | 0
 *   //  B   | 1
 *   //  C   | 2
 *   //  D   | 3
 *   //  E   | 4
 *   //  F   | 5
 *   //  G   | 6
 *   //  H   | 7
 *   //  I   | 8
 *   //  J   | 9
 * @endcode
 *
 * Then proceed to populate the matrix to reflect the graph
 *
 * @code
 *   // A------5-------B-------------14-------C
 *   // \               \                   /1|
 *   //  \               3                 J  |
 *   //   \               \               /1  | 7
 *   //    4           E-2-F--4---G--3--H     |
 *   //     \       8 /                  \    |
 *   //      D--------                    10--I
 *
 *  // Distance from nodes to other nodes
 *  routeWeights.SetValue(0, 1, 5);  // A-B=5
 *  routeWeights.SetValue(1, 2, 14); // B-C=14
 *  routeWeights.SetValue(0, 3, 4);  // A-D=4
 *  routeWeights.SetValue(1, 5, 3);  // B-F=3
 *  routeWeights.SetValue(2, 9, 1);  // C-J=1
 *  routeWeights.SetValue(9, 7, 1);  // J-H=1
 *  routeWeights.SetValue(2, 8, 7);  // C-I=7
 *  routeWeights.SetValue(3, 4, 8);  // D-E=8
 *  routeWeights.SetValue(4, 5, 2);  // E-F=2
 *  routeWeights.SetValue(5, 6, 4);  // F-G=4
 *  routeWeights.SetValue(6, 7, 3);  // G-H=3
 *  routeWeights.SetValue(7, 8, 10); // H-I=10
 * @endcode
 *
 * Then we set the weights from the nodes to themselves as 0
 * @code
 * for (size_t i=0; i < routeWeights.GetRows(); i++)
 * {
 *     routeWeights.SetValue(i, i, 0);
 * }
 * @endcode
 *
 * Create the known shortest paths
 * @code
 *  std::map<std::pair<int, int>, std::vector<int>> routeMap;
 *  for (size_t i = 0; i < routeWeights.GetRows(); i++)
 *  {
 *      for (size_t j = 0; j < routeWeights.GetRows(); j++)
 *      {
 *          if (routeWeights.GetValue(i, j) != maxFloat)
 *          {
 *              if (i != j)
 *              {
 *                  routeMap[{i, j}] = {(int)i, (int)j};
 *              }
 *              else
 *              {
 *                  routeMap[{i, j}] = {(int)i};
 *              }
 *          }
 *      }
 *  }
 *  @endcode
 *
 * And we finally can proceed to assemble paths between nodes
 * and store them in a routing table. In this case, by brute-force
 *
 * @code
 * for (size_t bridgeNode = 0; bridgeNode < routeWeights.GetRows(); bridgeNode++)
 *  {
 *      for (size_t srcNode = 0; srcNode < routeWeights.GetRows(); srcNode++)
 *      {
 *          for (size_t dstNode = 0; dstNode < routeWeights.GetRows(); dstNode++)
 *          {
 *              auto weightA = routeWeights.GetValue(srcNode, bridgeNode);
 *              auto weightB = routeWeights.GetValue(bridgeNode, dstNode);
 *              // If there is a path between A and bridge, plus bridge and B
 *              if (std::max(weightA, weightB) == maxFloat)
 *              {
 *                  continue;
 *              }
 *              // Check if sum of weights is lower than existing path
 *              auto weightAB = routeWeights.GetValue(srcNode, dstNode);
 *              if (weightA + weightB < weightAB)
 *              {
 *                  // If it is, update adjacency matrix with the new weight of the shortest
 *                  // path
 *                  routeWeights.SetValue(srcNode, dstNode, weightA + weightB);
 *
 *                  // Retrieve the partial routes A->bridge and bridge->C,
 *                  // and assemble the new route A->bridge->C
 *                  const auto srcToBridgeRoute = routeMap.at({srcNode, bridgeNode});
 *                  const auto bridgeToDstRoute = routeMap.at({bridgeNode, dstNode});
 *                  std::vector<int> dst;
 *                  dst.insert(dst.end(), srcToBridgeRoute.begin(), srcToBridgeRoute.end());
 *                  dst.insert(dst.end(), bridgeToDstRoute.begin() + 1, bridgeToDstRoute.end());
 *                  routeMap[{srcNode, dstNode}] = dst;
 *
 *                  // We also include the reverse path, since the graph is bidirectional
 *                  std::vector<int> invDst(dst.rbegin(), dst.rend());
 *                  routeMap[{dstNode, srcNode}] = invDst;
 *              }
 *          }
 *      }
 *  }
 * @endcode
 *
 * After this, we have both the complete route, weight of the route, and the weights for each hop in
 * the route.
 *
 * We can print all this information for a given route between nodes srcNodeOpt and
 * dstNodeOpt with
 *
 * @code
 * std::cout << "route between " << (char)(srcNodeOpt + 'A') << " and "
 *            << (char)(dstNodeOpt + 'A') << " (length "
 *            << routeWeights.GetValue(srcNodeOpt, dstNodeOpt) << "):";
 *  auto lastNodeNumber = srcNodeOpt;
 *  for (auto nodeNumber : routeMap.at({srcNodeOpt, dstNodeOpt}))
 *  {
 *      std::cout << "--" << routeWeights.GetValue(lastNodeNumber, nodeNumber) << "-->"
 *                << (char)('A' + nodeNumber);
 *      lastNodeNumber = nodeNumber;
 *  }
 * @endcode
 *
 * Which, for example, between nodes A and I, would print
 *
 * @verbatim
   route between A and I (length 24):--0-->A--5-->B--3-->F--4-->G--3-->H--1-->J--1-->C--7-->I
   @endverbatim
 *
 * In case one of the links is disconnected, the weights of the adjacency matrix can be reset
 * with SetValueAdjacent(disconnectedNode, maxFloat).
 *
 * Note that, in this implementation, all the routes containing the node need to be removed from
 * routeMap, and the search needs to be re-executed.
 */
template <typename T>
class SymmetricAdjacencyMatrix
{
  public:
    /**
     * Default constructor
     * @param [in] numRows The number of rows in the matrix
     * @param [in] value The default initialization value of matrix
     */
    SymmetricAdjacencyMatrix(size_t numRows = 0, T value = {})
    {
        m_rows = numRows;
        m_matrix.resize(m_rows * (m_rows + 1) / 2);
        std::fill(m_matrix.begin(), m_matrix.end(), value);
        for (size_t i = 0; i < numRows; i++)
        {
            m_rowOffsets.push_back(i * (i + 1) / 2);
        }
    };

    /**
     * @brief Retrieve the value of matrix (row, column) node
     * @param [in] row The row of the matrix to retrieve the value
     * @param [in] column  The column of the matrix to retrieve the value
     * @return value retrieved from matrix (row, column) or matrix (column, row)
     */
    T GetValue(size_t row, size_t column)
    {
        // Highest id should be always row, since we have only half matrix
        const auto maxIndex = std::max(row, column);
        const auto minIndex = std::min(row, column);
        return m_matrix.at(m_rowOffsets.at(maxIndex) + minIndex);
    }

    /**
     * @brief Set the value of matrix (row, column) node
     * @param [in] row The row of the matrix to set the value
     * @param [in] column The column of the matrix to set the value
     * @param [in] value value to be assigned to matrix (row, column) or matrix (column, row)
     */
    void SetValue(size_t row, size_t column, T value)
    {
        // Highest id should be always row, since we have only half matrix
        const auto maxIndex = std::max(row, column);
        const auto minIndex = std::min(row, column);
        m_matrix.at(m_rowOffsets.at(maxIndex) + minIndex) = value;
    }

    /**
     * @brief Set the value of adjacent nodes of a given node (all columns of a given row, and its
     * reflection)
     * @param [in] row The row of the matrix to set the value
     * @param [in] value Value to be assigned to matrix (row, column) or matrix (column, row)
     */
    void SetValueAdjacent(size_t row, T value)
    {
        // Since we only store the lower-left half of the adjacency matrix,
        // we need to set the adjacent values in both rows and columns involving this row id

        // First set the columns of row m_id
        for (size_t i = 0; i < row; i++)
        {
            m_matrix.at(m_rowOffsets.at(row) + i) = value;
        }
        // Then set the column m_id of rows >= m_id
        for (size_t i = row; i < m_rows; i++)
        {
            m_matrix.at(m_rowOffsets.at(i) + row) = value;
        }
    }

    /**
     * @brief Add new row to the adjacency matrix
     */
    void AddRow()
    {
        m_rowOffsets.push_back(m_matrix.size());
        m_rows++;
        m_matrix.resize(m_matrix.size() + m_rows);
    };

    /**
     * @brief Retrieve number of rows in the adjacency matrix
     * @return the number of rows
     */
    size_t GetRows()
    {
        return m_rows;
    }

  private:
    size_t m_rows; //!< Number of rows in matrix
    std::vector<T>
        m_matrix; //!< The adjacency matrix. For efficiency purposes, we store only lower
                  //!< left half, including the main diagonal. It also is stored as a vector
                  //!< not to introduce gaps between different rows or items (in case T = bool)
    std::vector<size_t> m_rowOffsets; //!< Precomputed row starting offsets of m_matrix
};

} // namespace ns3
#endif // NS3_SYMMETRIC_ADJACENCY_MATRIX_H
