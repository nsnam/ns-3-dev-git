/*
 * Copyright (c) 2025 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/symmetric-adjacency-matrix.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup antenna-tests
 *
 * @brief SymmetricAdjacencyMatrix Test Case
 */
class SymmetricAdjacencyMatrixTestCase : public TestCase
{
  public:
    /**
     * The constructor of the test case
     */
    SymmetricAdjacencyMatrixTestCase()
        : TestCase("SymmetricAdjacencyMatrix test case")
    {
    }

  private:
    /**
     * Run the test
     */
    void DoRun() override;
};

void
SymmetricAdjacencyMatrixTestCase::DoRun()
{
    SymmetricAdjacencyMatrix<bool> boolAdj;
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetRows(),
                          0,
                          "Should have 0 rows, but have " << boolAdj.GetRows());
    boolAdj.AddRow();
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetRows(),
                          1,
                          "Should have 1 rows, but have " << boolAdj.GetRows());
    boolAdj.AddRow();
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetRows(),
                          2,
                          "Should have 2 rows, but have " << boolAdj.GetRows());
    boolAdj.AddRow();
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetRows(),
                          3,
                          "Should have 3 rows, but have " << boolAdj.GetRows());
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(0, 0), false, "Should be set to false");
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(1, 0), false, "Should be set to false");
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(1, 1), false, "Should be set to false");
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(2, 0), false, "Should be set to false");
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(2, 1), false, "Should be set to false");
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(2, 2), false, "Should be set to false");

    // Test constructor with arguments
    boolAdj = SymmetricAdjacencyMatrix<bool>(3, true);
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(0, 0), true, "Should be set to false");
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(1, 0), true, "Should be set to false");
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(1, 1), true, "Should be set to false");
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(2, 0), true, "Should be set to false");
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(2, 1), true, "Should be set to false");
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(2, 2), true, "Should be set to false");

    // Set value setting
    boolAdj = SymmetricAdjacencyMatrix<bool>(4, false);
    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetRows(),
                          4,
                          "Should have 4 rows, but have " << boolAdj.GetRows());
    for (int i = 0; i < 4; i++)
    {
        // Mark all adjacent values to row i to true
        boolAdj.SetValueAdjacent(i, true);
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                // Check if adjacent values to i were marked as true
                if (i == j || i == k)
                {
                    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(j, k), true, "Should be set to true");
                    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(k, j), true, "Should be set to true");
                }
                else
                {
                    // Check if all other values are marked as false
                    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(j, k), false, "Should be set to false");
                    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(k, j), false, "Should be set to false");
                }
            }
        }
        // Reset values
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                if (i == j || i == k)
                {
                    boolAdj.SetValue(j, k, false);
                    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(j, k), false, "Should be set to false");
                    NS_TEST_EXPECT_MSG_EQ(boolAdj.GetValue(k, j), false, "Should be set to false");
                }
            }
        }
    }
}

/**
 * @ingroup core-tests
 *
 * @brief AdjacencyMatrix Test Suite
 */
class AdjacencyMatrixTestSuite : public TestSuite
{
  public:
    AdjacencyMatrixTestSuite();
};

AdjacencyMatrixTestSuite::AdjacencyMatrixTestSuite()
    : TestSuite("adjacency-matrix-test", Type::UNIT)
{
    AddTestCase(new SymmetricAdjacencyMatrixTestCase(), TestCase::Duration::QUICK);
}

static AdjacencyMatrixTestSuite adjacencyMatrixTestSuiteInstance;
