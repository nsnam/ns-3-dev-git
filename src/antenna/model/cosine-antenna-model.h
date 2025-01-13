/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef COSINE_ANTENNA_MODEL_H
#define COSINE_ANTENNA_MODEL_H

#include "antenna-model.h"

#include "ns3/object.h"

namespace ns3
{

/**
 * @ingroup antenna
 *
 * @brief Cosine Antenna Model
 *
 * This class implements the cosine model, similarly to what is described in:
 * Cosine Antenna Element, Mathworks, Phased Array System Toolbox (Sep. 2020)
 * Available online: https://www.mathworks.com/help/phased/ug/cosine-antenna-element.html
 *
 * The power pattern of the element is equal to:
  // P(az,el) = cos(az/2)^2m * cos(pi/2 - incl/2)^2n,
  // where az is the azimuth angle, and incl is the inclination angle.
 *
 * Differently from the source, the response is defined for azimuth and elevation angles
 * between –180 and 180 degrees and is always positive.
 * There is no response at the backside of a cosine antenna.
 * The cosine response pattern achieves a maximum value of 1 (0 dB) at 0 degrees azimuth
 * and 90 degrees inclination.
 * An extra settable gain is added to the original model, to improve its generality.
 */
class CosineAntennaModel : public AntennaModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    // inherited from AntennaModel
    double GetGainDb(Angles a) override;

    /**
     * Get the vertical 3 dB beamwidth of the cosine antenna model.
     * @return the vertical beamwidth in degrees
     */
    double GetVerticalBeamwidth() const;

    /**
     * Get the horizontal 3 dB beamwidth of the cosine antenna model.
     * @return the horizontal beamwidth in degrees
     */
    double GetHorizontalBeamwidth() const;

    /**
     * Get the horizontal orientation of the antenna element.
     * @return the horizontal orientation in degrees
     */
    double GetOrientation() const;

  private:
    /**
     * Set the vertical 3 dB beamwidth (bilateral) of the cosine antenna model.
     * @param verticalBeamwidthDegrees the vertical beamwidth in degrees
     */
    void SetVerticalBeamwidth(double verticalBeamwidthDegrees);

    /**
     * Set the horizontal 3 dB beamwidth (bilateral) of the cosine antenna model.
     * @param horizontalBeamwidthDegrees the horizontal beamwidth in degrees
     */
    void SetHorizontalBeamwidth(double horizontalBeamwidthDegrees);

    /**
     * Set the horizontal orientation of the antenna element.
     * @param orientationDegrees the horizontal orientation in degrees
     */
    void SetOrientation(double orientationDegrees);

    /**
     * Compute the exponent of the cosine antenna model from the beamwidth
     * @param beamwidthDegrees the beamwidth in degrees
     * @return the exponent
     */
    static double GetExponentFromBeamwidth(double beamwidthDegrees);

    /**
     * Compute the beamwidth of the cosine antenna model from the exponent
     * @param exponent the exponent
     * @return beamwidth in degrees
     */
    static double GetBeamwidthFromExponent(double exponent);

    double m_verticalExponent;   //!< exponent of the vertical direction
    double m_horizontalExponent; //!< exponent of the horizontal direction
    double m_orientationRadians; //!< orientation in radians in the horizontal direction (bearing)
    double m_maxGain;            //!< antenna gain in dB towards the main orientation
};

} // namespace ns3

#endif // COSINE_ANTENNA_MODEL_H
