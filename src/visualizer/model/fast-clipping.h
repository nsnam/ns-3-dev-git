/*
 * Copyright (c) 2008 INESC Porto
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */

namespace ns3
{

namespace visualizer
{
/**
 * @ingroup Visualizer
 *
 * This class is used by the visualizer for the process of removing lines or portions of
 * lines outside of an area of interest.
 *
 * This code has been adapted from
 * http://en.wikipedia.org/w/index.php?title=Line_clipping&oldid=248609574
 *
 **/
class FastClipping
{
  public:
    /**
     * @ingroup Visualizer
     *
     * The Vector 2 struct
     */
    struct Vector2
    {
        double x; //!< X coordinate.
        double y; //!< Y coordinate.
    };

    /**
     * @ingroup Visualizer
     *
     * The line struct
     */
    struct Line
    {
        Vector2 start; //!< The start point of the line.
        Vector2 end;   //!< The end point of the line.
        double dx;     //!< dX
        double dy;     //!< dY
    };

    /**
     * Constructor
     *
     * @param clipMin minimum clipping vector
     * @param clipMax maximum clipping vector
     */
    FastClipping(Vector2 clipMin, Vector2 clipMax);

    /**
     * Clip line function
     *
     * @param line the clip line
     * @returns true if clipped
     */
    bool ClipLine(Line& line);

  private:
    /**
     * Clip start top function
     *
     * @param line the clip line
     */
    void ClipStartTop(Line& line) const;

    /**
     * Clip start bottom function
     *
     * @param line the clip line
     */
    void ClipStartBottom(Line& line) const;

    /**
     * Clip start right function
     *
     * @param line the clip line
     */
    void ClipStartRight(Line& line) const;

    /**
     * Clip start left function
     *
     * @param line the clip line
     */
    void ClipStartLeft(Line& line) const;

    /**
     * Clip end top function
     *
     * @param line the clip line
     */
    void ClipEndTop(Line& line) const;

    /**
     * Clip end bottom function
     *
     * @param line the clip line
     */
    void ClipEndBottom(Line& line) const;

    /**
     * Clip end right function
     *
     * @param line the clip line
     */
    void ClipEndRight(Line& line) const;

    /**
     * Clip end left function
     *
     * @param line the clip line
     */
    void ClipEndLeft(Line& line) const;

    Vector2 m_clipMin; //!< The minimum point of the bounding area required clipping.
    Vector2 m_clipMax; //!< The maximum point of the bounding area required clipping.
};

} // namespace visualizer
} // namespace ns3
