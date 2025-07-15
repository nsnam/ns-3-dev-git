/*
 * Copyright (c) 2025 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "hexagonal-wraparound-model.h"

#include "ns3/log.h"

#include <algorithm>

constexpr double M_SQRT3 = 1.732050807568877; // sqrt(3)

namespace ns3
{
/**
 * Coefficients used to wraparound a cluster with 7 sites (1 ring)
 */
std::vector<Vector2D> WRAPPING_COEFF_CLUSTER7 =
    {{0.0, 0.0}, {-3.0, 2.0}, {3.0, -2.0}, {-1.5, -2.5}, {1.5, 2.5}, {-4.5, -0.5}, {4.5, 0.5}};

/**
 * Coefficients used to wraparound a cluster with 19 sites (3 rings)
 */
std::vector<Vector2D> WRAPPING_COEFF_CLUSTER19 =
    {{0.0, 0.0}, {-3.0, -4.0}, {3.0, 4.0}, {-4.5, 3.5}, {4.5, -3.5}, {-7.5, -0.5}, {7.5, 0.5}};

NS_LOG_COMPONENT_DEFINE("HexagonalWraparoundModel");

NS_OBJECT_ENSURE_REGISTERED(HexagonalWraparoundModel);

TypeId
HexagonalWraparoundModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::HexagonalWraparoundModel")
                            .SetParent<WraparoundModel>()
                            .SetGroupName("Spectrum")
                            .AddConstructor<HexagonalWraparoundModel>();
    return tid;
}

HexagonalWraparoundModel::HexagonalWraparoundModel()
    : m_numSites(1)
{
}

HexagonalWraparoundModel::HexagonalWraparoundModel(double isd, size_t numSites)
    : m_numSites(numSites)
{
    SetSiteDistance(isd);
}

void
HexagonalWraparoundModel::SetSiteDistance(double isd)
{
    m_isd = isd;
    m_radius = isd / M_SQRT3;
}

void
HexagonalWraparoundModel::SetNumSites(uint8_t numSites)
{
    NS_ASSERT((numSites == 1) || (numSites == 7) || (numSites == 19));
    m_numSites = numSites;
}

void
HexagonalWraparoundModel::AddSitePosition(const Vector3D& pos)
{
    NS_LOG_FUNCTION(this << pos);
    NS_ASSERT(m_sitePositions.size() < m_numSites);
    m_sitePositions.push_back(pos);
}

void
HexagonalWraparoundModel::SetSitePositions(const std::vector<Vector3D>& positions)
{
    NS_ASSERT(positions.size() == m_numSites);
    m_sitePositions = positions;
}

double
HexagonalWraparoundModel::CalculateDistance(const Vector3D& a, const Vector3D& b) const
{
    return (a - GetVirtualPosition(b, a)).GetLength();
}

// Wraparound calculation is based on
// R. S. Panwar and K. M. Sivalingam, "Implementation of wrap
// around mechanism for system level simulation of LTE cellular
// networks in NS3," 2017 IEEE 18th International Symposium on
// A World of Wireless, Mobile and Multimedia Networks (WoWMoM),
// Macau, 2017, pp. 1-9, doi: 10.1109/WoWMoM.2017.7974289.
Vector3D
HexagonalWraparoundModel::GetSitePosition(const Vector3D& pos) const
{
    NS_ASSERT(m_numSites == m_sitePositions.size());

    std::vector<double> virtDists;
    virtDists.resize(m_numSites);
    std::transform(m_sitePositions.begin(),
                   m_sitePositions.end(),
                   virtDists.begin(),
                   [&](const Vector3D& sitePos) {
                       return (this->GetVirtualPosition(pos, sitePos) - sitePos).GetLength();
                   });
    auto idx = std::min_element(virtDists.begin(), virtDists.end()) - virtDists.begin();
    return m_sitePositions[idx];
}

Vector3D
HexagonalWraparoundModel::GetVirtualPosition(const Vector3D txPos, const Vector3D rxPos) const
{
    NS_ASSERT_MSG((m_numSites == 1) || (m_numSites == 7) || (m_numSites == 19),
                  "invalid numSites: " << m_numSites);

    if (m_numSites == 1)
    {
        return txPos;
    }

    auto& coeffs = (m_numSites == 7) ? WRAPPING_COEFF_CLUSTER7 : WRAPPING_COEFF_CLUSTER19;

    std::vector<double> virtDists;
    virtDists.resize(coeffs.size());
    std::transform(coeffs.begin(), coeffs.end(), virtDists.begin(), [&](const Vector2D& coeff) {
        Vector3D virtPos2 = txPos;
        virtPos2.x += coeff.x * m_radius;
        virtPos2.y += coeff.y * m_isd;
        return static_cast<double>((virtPos2 - rxPos).GetLength());
    });

    auto idx = std::min_element(virtDists.begin(), virtDists.end()) - virtDists.begin();
    Vector3D offset(m_radius * coeffs[idx].x, m_isd * coeffs[idx].y, 0.0);
    return txPos + offset;
}

} // namespace ns3
