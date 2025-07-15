/*
 * Copyright (c) 2025 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#ifndef HEXAGONAL_WRAPAROUND_H
#define HEXAGONAL_WRAPAROUND_H

#include "wraparound-model.h"

#include "ns3/vector.h"

#include <map>
#include <vector>

namespace ns3
{
/**
 * @ingroup propagation
 * @brief Hexagonal wraparound model wraps nodes around in space based on an
 * hexagonal deployment. It is used to determine a virtual position after
 * wraparound and calculate the distance between a point of reference and
 * the virtual position.
 *
 * This model is used for the hexagonal deployments typical of mobile networks.
 * It supports rings 0 (1 site), 1 (7 sites) and 3 rings (19 sites).
 * To use it, set the inter-site distance (isd) and the number of sites.
 * Then, add the position coordinates of the sites.
 * All space coordinates in this class and its subclasses are understood
 * to be meters or meters/s. i.e., they are all metric international units.
 *
 * When GetRelativeVirtualPosition (absPos1, absPos2) is called,
 * the relative position of the absPos2 is calculated respective to the
 * reference coordinate absPos1, applying the wrapping based on the model
 * described in:
 *
 * R. S. Panwar and K. M. Sivalingam, "Implementation of wrap
 * around mechanism for system level simulation of LTE cellular
 * networks in NS3," 2017 IEEE 18th International Symposium on
 * A World of Wireless, Mobile and Multimedia Networks (WoWMoM),
 * Macau, 2017, pp. 1-9, doi: 10.1109/WoWMoM.2017.7974289.
 */
class HexagonalWraparoundModel : public WraparoundModel
{
  public:
    /**
     * Register this type with the TypeId system.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Default constructor
     */
    HexagonalWraparoundModel();

    /**
     * @brief Constructor
     * @param isd inter-site distance
     * @param numSites number of sites
     */
    HexagonalWraparoundModel(double isd, size_t numSites);

    /**
     * @brief Set site distance
     * @param isd site distance
     */
    void SetSiteDistance(double isd);

    /**
     * @brief Set number of sites
     * @param numSites number of sites
     */
    void SetNumSites(uint8_t numSites);

    /**
     * @brief Add a site position
     * @param pos position of a site
     */
    void AddSitePosition(const Vector3D& pos);

    /**
     * @brief Set site positions
     * @param positions positions of all sites
     */
    void SetSitePositions(const std::vector<Vector3D>& positions);

    /**
     * @brief Calculate the site position
     * @param pos original position
     * @return closest cell center based on wraparound distance
     */
    Vector3D GetSitePosition(const Vector3D& pos) const;

    /**
     * @brief Calculate distance after wraparound between two points
     * @param a position of point a
     * @param b position of point b
     * @return wraparound distance between a and b
     */
    double CalculateDistance(const Vector3D& a, const Vector3D& b) const;
    /**
     * @brief Get virtual position of txPos with respect to rxPos
     * @param txPos
     * @param rxPos
     * @return virtual position of txPos
     */
    Vector3D GetVirtualPosition(const Vector3D txPos, const Vector3D rxPos) const override;

  private:
    double m_isd;                          //!< distance between sites
    double m_radius;                       //!< site radius
    uint8_t m_numSites;                    //!< number of sites
    std::vector<Vector3D> m_sitePositions; //!< site positions
};
} // namespace ns3

#endif /* HEXAGONAL_WRAPAROUND_H */
