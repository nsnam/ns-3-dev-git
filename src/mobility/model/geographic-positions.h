/*
 * Copyright (c) 2014 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Benjamin Cizdziel <ben.cizdziel@gmail.com>
 */

#include "ns3/random-variable-stream.h"
#include "ns3/vector.h"

#ifndef GEOGRAPHIC_POSITIONS_H
#define GEOGRAPHIC_POSITIONS_H

namespace ns3
{

/**
 * @ingroup mobility
 *
 * Consists of methods dealing with Earth geographic coordinates and locations.
 */
class GeographicPositions
{
  public:
    /**
     * Spheroid model to use for earth: perfect sphere (SPHERE), Geodetic
     * Reference System 1980 (GRS80), or World Geodetic System 1984 (WGS84)
     *
     * Moritz, H. "Geodetic Reference System 1980." GEODETIC REFERENCE SYSTEM 1980.
     * <https://web.archive.org/web/20170712034716/http://www.gfy.ku.dk/~iag/HB2000/part4/grs80_corr.htm>.
     *
     * "Department of Defense World Geodetic System 1984." National Imagery and
     * Mapping Agency, 1 Jan. 2000.
     * <https://web.archive.org/web/20200729203634/https://earth-info.nga.mil/GandG/publications/tr8350.2/wgs84fin.pdf>.
     */

    /// Earth's radius in meters if modeled as a perfect sphere
    static constexpr double EARTH_SPHERE_RADIUS = 6371e3;

    /// Earth's eccentricity if modeled as a perfect sphere
    static constexpr double EARTH_SPHERE_ECCENTRICITY = 0;

    /// Earth's flattening if modeled as a perfect sphere
    static constexpr double EARTH_SPHERE_FLATTENING = 0;

    /// <Earth's semi-major axis in meters as defined by both GRS80 and WGS84
    /// https://en.wikipedia.org/wiki/World_Geodetic_System
    static constexpr double EARTH_SEMIMAJOR_AXIS = 6378137;

    /// Earth's first eccentricity as defined by GRS80
    /// https://en.wikipedia.org/wiki/Geodetic_Reference_System_1980
    static constexpr double EARTH_GRS80_ECCENTRICITY = 0.0818191910428158;

    /// Earth's first flattening as defined by GRS80
    /// https://en.wikipedia.org/wiki/Geodetic_Reference_System_1980
    static constexpr double EARTH_GRS80_FLATTENING = 0.003352810681183637418;

    /// Earth's first eccentricity as defined by
    /// https://en.wikipedia.org/wiki/World_Geodetic_System#WGS84
    static constexpr double EARTH_WGS84_ECCENTRICITY = 0.0818191908426215;

    /// Earth's first flattening as defined by WGS84
    /// https://en.wikipedia.org/wiki/World_Geodetic_System#WGS84
    static constexpr double EARTH_WGS84_FLATTENING = 0.00335281;

    /// The possible Earth spheroid models.  .
    enum EarthSpheroidType
    {
        SPHERE,
        GRS80,
        WGS84
    };

    /**
     * Converts earth geographic/geodetic coordinates (latitude and longitude in
     * degrees) with a given altitude above earth's surface (in meters) to Earth
     * Centered Earth Fixed (ECEF) Cartesian coordinates (x, y, z in meters),
     * where origin (0, 0, 0) is the center of the earth.
     *
     * @param latitude earth-referenced latitude (in degrees) of the point
     * @param longitude earth-referenced longitude (in degrees) of the point
     * @param altitude height of the point (in meters) above earth's surface
     * @param sphType earth spheroid model to use for conversion
     *
     * @return a vector representing the Cartesian coordinates (x, y, z referenced
     * in meters) of the point (origin (0, 0, 0) is center of earth)
     */
    static Vector GeographicToCartesianCoordinates(double latitude,
                                                   double longitude,
                                                   double altitude,
                                                   EarthSpheroidType sphType);

    /**
     * Inverse of GeographicToCartesianCoordinates using [1]
     *
     * This function iteratively converts cartesian (ECEF) coordinates to
     * geographic coordinates. The residual delta is 1 m, which is approximately
     * 1 / 30 arc seconds or 9.26e-6 deg.
     *
     * @param pos a vector representing the Cartesian coordinates (x, y, z referenced
     * in meters) of the point (origin (0, 0, 0) is center of earth)
     * @param sphType earth spheroid model to use for conversion
     *
     * @return Vector position where x = latitude (deg), y = longitude (deg),
     * z = altitude above the ellipsoid (m)
     *
     * [1] "Ellipsoidal and Cartesian Coordinates Conversion", Navipedia,
     * European Space Agency, Jul 8, 2019.
     * <https://gssc.esa.int/navipedia/index.php/Ellipsoidal_and_Cartesian_Coordinates_Conversion>
     */
    static Vector CartesianToGeographicCoordinates(Vector pos, EarthSpheroidType sphType);

    /**
     * Conversion from geographic to topocentric coordinates.
     *
     * Conversion formulas taken from [1, Sec. 4.1.3 "Geographic/topocentric conversions"].
     * [1] IOGP. Geomatics guidance note 7, part 2: coordinate conversions & transformations
     * including formulas. IOGP Publication 373-7-2, International Association For Oil And Gas
     * Producers, Sep. 2019
     * https://www.iogp.org/bookstore/product/coordinate-conversions-and-transformation-including-formulas/
     *
     * @param pos a vector representing the Geographic coordinates (latitude, longitude, altitude)
     * in degrees (lat/lon) and meters (altitude).
     * @param refPoint a vector representing the reference point (latitude, longitude, altitude) in
     * degrees (lat/lon) and meters (altitude). Default is (0,0,0)
     * @param sphType earth spheroid model to use for conversion
     *
     * @return Vector position in meters and using planar Cartesian coordinates, i.e., ns-3's
     * defaults
     */
    static Vector GeographicToTopocentricCoordinates(Vector pos,
                                                     Vector refPoint,
                                                     EarthSpheroidType sphType);

    /**
     * Conversion from topocentric to geographic.
     *
     * Conversion formulas taken from [1, Sec. 4.1.3 "Geographic/topocentric conversions"].
     * [1] IOGP. Geomatics guidance note 7, part 2: coordinate conversions & transformations
     * including formulas. IOGP Publication 373-7-2, International Association For Oil And Gas
     * Producers, Sep. 2019
     * https://www.iogp.org/bookstore/product/coordinate-conversions-and-transformation-including-formulas/
     *
     * @param pos a vector representing the topocentric coordinates (u, v, w) in meters, which
     * represent the cartesian coordinates along the xEast, yNorth and zUp axes, respectively..
     * @param refPoint a vector representing the reference point (latitude, longitude, altitude) in
     * degrees (lat/lon) and meters (altitude). Default is (0,0,0)
     * @param sphType earth spheroid model to use for conversion
     *
     * @return Vector position (latitude, longitude, altitude) in degrees (lat/lon) and meters
     * (altitude) converted to geographic coordinates
     */
    static Vector TopocentricToGeographicCoordinates(Vector pos,
                                                     Vector refPoint,
                                                     EarthSpheroidType sphType);

    /**
     * Generates uniformly distributed random points (in ECEF Cartesian
     * coordinates) within a given altitude above earth's surface centered around
     * a given origin point (on earth's surface, in geographic/geodetic coordinates)
     * within a given distance radius (using arc length of earth's surface, not
     * pythagorean distance).
     * Distance radius is measured as if all generated points are on earth's
     * surface (with altitude = 0).
     * Assumes earth is a perfect sphere.
     *
     * @param originLatitude origin point latitude in degrees
     * @param originLongitude origin point longitude in degrees
     * @param maxAltitude maximum altitude in meters above earth's surface with
     * which random points can be generated
     * @param numPoints number of points to generate
     * @param maxDistFromOrigin max distance in meters from origin with which
     * random transmitters can be generated (all transmitters are less than or
     * equal to this distance from the origin, relative to points being on earth's
     * surface)
     * @param uniRand pointer to the uniform random variable to use for random
     * location and altitude generation
     *
     * @return a list containing the vectors (x, y, z location referenced in
     * meters from origin at center of earth) of each point generated
     */
    static std::list<Vector> RandCartesianPointsAroundGeographicPoint(
        double originLatitude,
        double originLongitude,
        double maxAltitude,
        int numPoints,
        double maxDistFromOrigin,
        Ptr<UniformRandomVariable> uniRand);

    /**
     * @param type the type of model which is used to model the Earth
     *
     * @return the corresponding radius (in meters), first eccentricity and first flattening values
     */
    static std::tuple<double, double, double> GetRadiusEccentFlat(EarthSpheroidType type);
};

} // namespace ns3

#endif /* GEOGRAPHIC_POSITIONS_H */
