/*
 * Copyright (c) 2012 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef PARABOLIC_ANTENNA_MODEL_H
#define PARABOLIC_ANTENNA_MODEL_H

#include "antenna-model.h"

#include "ns3/object.h"

namespace ns3
{

/**
 * @ingroup antenna
 *
 * @brief  Antenna model based on a parabolic approximation of the main lobe radiation pattern.
 *
 * This class implements the parabolic model as described in some 3GPP document, e.g., R4-092042
 *
 * A similar model appears in:
 *
 * George Calcev and Matt Dillon, "Antenna Tilt Control in CDMA Networks"
 * in Proc. of the 2nd Annual International Wireless Internet Conference (WICON), 2006
 *
 * though the latter addresses also the elevation plane, which the present model doesn't.
 *
 *
 */
class ParabolicAntennaModel : public AntennaModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    // inherited from AntennaModel
    double GetGainDb(Angles a) override;

    // attribute getters/setters
    /**
     * Set the Beam width
     * @param beamwidthDegrees Beam width in degrees
     */
    void SetBeamwidth(double beamwidthDegrees);
    /**
     * Get the Beam width
     * @return beam width in degrees
     */
    double GetBeamwidth() const;
    /**
     * Set the antenna orientation
     * @param orientationDegrees antenna orientation in degrees
     */
    void SetOrientation(double orientationDegrees);
    /**
     * Get the antenna orientation
     * @return antenna orientation in degrees
     */
    double GetOrientation() const;

  private:
    double m_beamwidthRadians;   //!< Beam width in radians
    double m_orientationRadians; //!< Antenna orientation in radians
    double m_maxAttenuation;     //!< Max attenuation
};

} // namespace ns3

#endif // PARABOLIC_ANTENNA_MODEL_H
