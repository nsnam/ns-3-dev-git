/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/vector.h"

namespace ns3
{

/**
 * @ingroup mobility
 * @brief a 2d rectangle
 * @see attribute_Rectangle
 */
class Rectangle
{
  public:
    /**
     * enum for naming sides
     */
    enum Side
    {
        RIGHTSIDE = 0,
        LEFTSIDE,
        TOPSIDE,
        BOTTOMSIDE,
        TOPRIGHTCORNER,
        TOPLEFTCORNER,
        BOTTOMRIGHTCORNER,
        BOTTOMLEFTCORNER
    };

    /**
     * @param _xMin x coordinates of left boundary.
     * @param _xMax x coordinates of right boundary.
     * @param _yMin y coordinates of bottom boundary.
     * @param _yMax y coordinates of top boundary.
     *
     * Create a rectangle.
     */
    Rectangle(double _xMin, double _xMax, double _yMin, double _yMax);
    /**
     * Create a zero-sized rectangle located at coordinates (0.0,0.0)
     */
    Rectangle();
    /**
     * @param position the position to test.
     * @return true if the input position is located within the rectangle, false otherwise.
     *
     * This method compares only the x and y coordinates of the input position.
     * It ignores the z coordinate.
     */
    bool IsInside(const Vector& position) const;
    /**
     * @param position the position to test.
     * @return true if the input position is located on the rectable border, false otherwise.
     *
     * This method compares only the x and y coordinates of the input position.
     * It ignores the z coordinate.
     */
    bool IsOnTheBorder(const Vector& position) const;
    /**
     * @param position the position to test.
     * @return the side of the rectangle the input position is closest to.
     *
     * This method compares only the x and y coordinates of the input position.
     * It ignores the z coordinate.
     *
     * This method assumes assumes a right-handed Cartesian orientation, so that
     * (xMin, yMin) is the BOTTOMLEFTCORNER,  the TOP and BOTTOM sides are parallel to
     * the x-axis, the LEFT and RIGHT sides are parallel to the y-axis, and the
     * (xMax, yMax) point is the TOPRIGHTCORNER.
     *
     * Beware: the method has an ambiguity in the "center" of the rectangle. Assume
     * a rectangle between the points (0, 0) and (4, 2), i.e., wider than taller.
     * All the points on the line between (1, 1) and (3, 1) are equally closer to the
     * TOP and BOTTOM than to the other sides of the Rectangle.
     * These points are classified as TOPSIDE by convention.
     *
     * Similarly, for a Rectangle taller than wider, the "center" points will be
     * classified as RIGHTSIDE, and for a square box, the center will return RIGHTSIDE.
     */
    Side GetClosestSideOrCorner(const Vector& position) const;
    /**
     * @param current the current position
     * @param speed the current speed
     * @return the intersection point between the rectangle and the current+speed vector.
     *
     * This method assumes that the current position is located _inside_
     * the rectangle and checks for this with an assert.
     * This method compares only the x and y coordinates of the input position
     * and speed. It ignores the z coordinate.
     */
    Vector CalculateIntersection(const Vector& current, const Vector& speed) const;

    double xMin; //!< The x coordinate of the left bound of the rectangle
    double xMax; //!< The x coordinate of the right bound of the rectangle
    double yMin; //!< The y coordinate of the bottom bound of the rectangle
    double yMax; //!< The y coordinate of the top bound of the rectangle
};

std::ostream& operator<<(std::ostream& os, const Rectangle& rectangle);
std::istream& operator>>(std::istream& is, Rectangle& rectangle);
std::ostream& operator<<(std::ostream& os, const Rectangle::Side& side);

ATTRIBUTE_HELPER_HEADER(Rectangle);

} // namespace ns3

#endif /* RECTANGLE_H */
