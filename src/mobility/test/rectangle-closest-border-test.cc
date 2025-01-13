/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/rectangle.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup mobility-test
 *
 * @brief Rectangle detection of closest border to a point, inside or outside.
 */
class RectangleClosestBorderTestSuite : public TestSuite
{
  public:
    /**
     * Constructor
     */
    RectangleClosestBorderTestSuite();
};

/**
 * @ingroup mobility-test
 *
 * @brief TestCase to check the rectangle line intersection
 */
class RectangleClosestBorderTestCase : public TestCase
{
  public:
    /**
     * @brief Create RectangleClosestBorderTestCase
     * @param x Index of the first position to generate
     * @param y Index of the second position to generate
     * @param rectangle The 2D rectangle
     * @param side The expected result of the test
     */
    RectangleClosestBorderTestCase(double x, double y, Rectangle rectangle, Rectangle::Side side);
    /**
     * @brief Builds the test name string based on provided parameter values
     * @param x Index of the first position to generate
     * @param y Index of the second position to generate
     * @param rectangle The 2D rectangle
     * @param side The expected result of the test
     *
     * @return The name string
     */
    std::string BuildNameString(double x, double y, Rectangle rectangle, Rectangle::Side side);
    /**
     * Destructor
     */
    ~RectangleClosestBorderTestCase() override;

  private:
    /**
     * @brief Setup the simulation according to the configuration set by the
     *        class constructor, run it, and verify the result.
     */
    void DoRun() override;

    double m_x{0.0};       //!< X coordinate of the point to be tested
    double m_y{0.0};       //!< Y coordinate of the point to be tested
    Rectangle m_rectangle; //!< The rectangle to check the intersection with

    /**
     * Flag to indicate the intersection.
     * True, for intersection, false otherwise.
     */
    Rectangle::Side m_side{ns3::Rectangle::TOPSIDE};
};

/**
 * This TestSuite tests the intersection of a line segment
 * between two 2D positions with a 2D rectangle. It generates two
 * positions from a set of predefined positions (see GeneratePosition method),
 * and then tests the intersection of a line segments between them with a rectangle
 * of predefined dimensions.
 */

RectangleClosestBorderTestSuite::RectangleClosestBorderTestSuite()
    : TestSuite("rectangle-closest-border", Type::UNIT)
{
    // Rectangle in the positive x-plane to check the intersection with.
    Rectangle rectangle = Rectangle(0.0, 10.0, 0.0, 10.0);

    /*            2                3                4
     *              +----------------------------+ (10,10)
     *              |  11         16          12 |
     *              |                            |
     *              |                            |
     *      1       |  15          18         17 |              5
     *              |                            |
     *              |                            |
     *              |  10          14         13 |
     *              +----------------------------+
     *           9                                  7
     *           (0,0)
     *
     *
     *
     *                             8
     */
    // Left side (1 and 15)
    AddTestCase(new RectangleClosestBorderTestCase(-5, 5, rectangle, Rectangle::LEFTSIDE),
                TestCase::Duration::QUICK);
    AddTestCase(new RectangleClosestBorderTestCase(2, 5, rectangle, Rectangle::LEFTSIDE),
                TestCase::Duration::QUICK);
    // Right side (5 and 17)
    AddTestCase(new RectangleClosestBorderTestCase(17, 5, rectangle, Rectangle::RIGHTSIDE),
                TestCase::Duration::QUICK);
    AddTestCase(new RectangleClosestBorderTestCase(7, 5, rectangle, Rectangle::RIGHTSIDE),
                TestCase::Duration::QUICK);
    // Bottom side (8 and 14)
    AddTestCase(new RectangleClosestBorderTestCase(5, -7, rectangle, Rectangle::BOTTOMSIDE),
                TestCase::Duration::QUICK);
    AddTestCase(new RectangleClosestBorderTestCase(5, 1, rectangle, Rectangle::BOTTOMSIDE),
                TestCase::Duration::QUICK);
    // Top side (3 and 16)
    AddTestCase(new RectangleClosestBorderTestCase(5, 15, rectangle, Rectangle::TOPSIDE),
                TestCase::Duration::QUICK);
    AddTestCase(new RectangleClosestBorderTestCase(5, 7, rectangle, Rectangle::TOPSIDE),
                TestCase::Duration::QUICK);
    // Left-Bottom corner (9 and 10)
    AddTestCase(new RectangleClosestBorderTestCase(-1, -1, rectangle, Rectangle::BOTTOMLEFTCORNER),
                TestCase::Duration::QUICK);
    AddTestCase(new RectangleClosestBorderTestCase(0, 0, rectangle, Rectangle::BOTTOMLEFTCORNER),
                TestCase::Duration::QUICK);
    // Right-Bottom corner (7 and 13)
    AddTestCase(new RectangleClosestBorderTestCase(11, -1, rectangle, Rectangle::BOTTOMRIGHTCORNER),
                TestCase::Duration::QUICK);
    AddTestCase(new RectangleClosestBorderTestCase(9, 1, rectangle, Rectangle::BOTTOMRIGHTCORNER),
                TestCase::Duration::QUICK);
    // Left-Top corner (2 and 11)
    AddTestCase(new RectangleClosestBorderTestCase(-1, 11, rectangle, Rectangle::TOPLEFTCORNER),
                TestCase::Duration::QUICK);
    AddTestCase(new RectangleClosestBorderTestCase(1, 9, rectangle, Rectangle::TOPLEFTCORNER),
                TestCase::Duration::QUICK);
    // Right-Top corner (4 and 12)
    AddTestCase(new RectangleClosestBorderTestCase(11, 11, rectangle, Rectangle::TOPRIGHTCORNER),
                TestCase::Duration::QUICK);
    AddTestCase(new RectangleClosestBorderTestCase(9, 9, rectangle, Rectangle::TOPRIGHTCORNER),
                TestCase::Duration::QUICK);
    // Central position (18)
    AddTestCase(new RectangleClosestBorderTestCase(5, 5, rectangle, Rectangle::TOPSIDE),
                TestCase::Duration::QUICK);
}

/**
 * @ingroup mobility-test
 * Static variable for test initialization
 */
static RectangleClosestBorderTestSuite rectangleClosestBorderTestSuite;

RectangleClosestBorderTestCase::RectangleClosestBorderTestCase(double x,
                                                               double y,
                                                               Rectangle rectangle,
                                                               Rectangle::Side side)
    : TestCase(BuildNameString(x, y, rectangle, side)),
      m_x(x),
      m_y(y),
      m_rectangle(rectangle),
      m_side(side)
{
}

RectangleClosestBorderTestCase::~RectangleClosestBorderTestCase()
{
}

std::string
RectangleClosestBorderTestCase::BuildNameString(double x,
                                                double y,
                                                Rectangle rectangle,
                                                Rectangle::Side side)
{
    std::ostringstream oss;
    oss << "Rectangle closest border test : checking"
        << " (x,y) = (" << x << "," << y << ") closest border to the rectangle [(" << rectangle.xMin
        << ", " << rectangle.yMin << "), (" << rectangle.xMax << ", " << rectangle.yMax
        << ")]. The expected side = " << side;
    return oss.str();
}

void
RectangleClosestBorderTestCase::DoRun()
{
    Vector position(m_x, m_y, 0.0);
    Rectangle::Side side = m_rectangle.GetClosestSideOrCorner(position);

    NS_TEST_ASSERT_MSG_EQ(side, m_side, "Unexpected result of rectangle side!");
    Simulator::Destroy();
}
