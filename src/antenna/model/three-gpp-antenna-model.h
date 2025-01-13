/*
 * Copyright (c) 2020 University of Padova, Dep. of Information Engineering, SIGNET lab.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef THREE_GPP_ANTENNA_MODEL_H
#define THREE_GPP_ANTENNA_MODEL_H

#include "antenna-model.h"

#include "ns3/object.h"

namespace ns3
{

/**
 * @brief  Antenna model based on a parabolic approximation of the main lobe radiation pattern.
 *
 * This class implements the parabolic model as described in 3GPP TR 38.901 v15.0.0
 */
class ThreeGppAntennaModel : public AntennaModel
{
  public:
    /**
     * The different antenna radiation patterns defined in ITU-R M.2412.
     */
    enum class RadiationPattern
    {
        OUTDOOR,
        INDOOR
    };

    ThreeGppAntennaModel();
    ~ThreeGppAntennaModel() override;

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    // inherited from AntennaModel
    double GetGainDb(Angles a) override;

    /**
     * Get the vertical beamwidth of the antenna element.
     * @return the vertical beamwidth in degrees
     */
    double GetVerticalBeamwidth() const;

    /**
     * Get the horizontal beamwidth of the antenna element.
     * @return the horizontal beamwidth in degrees
     */
    double GetHorizontalBeamwidth() const;

    /**
     * Set the antenna radiation pattern
     * @param pattern the antenna radiation pattern to used
     */
    void SetRadiationPattern(RadiationPattern pattern);

    /**
     * Get the antenna radiation pattern
     * @return the radiation pattern of the antenna
     */
    RadiationPattern GetRadiationPattern() const;

    /**
     * Get the side-lobe attenuation in the vertical direction of the antenna element.
     * @return side-lobe attenuation in the vertical direction in dB
     */
    double GetSlaV() const;

    /**
     * Get the maximum attenuation of the antenna element.
     * @return the maximum attenuation in dB
     */
    double GetMaxAttenuation() const;

    /**
     * Get the maximum directional gain of the antenna element.
     * @return the maximum directional gain in dBi
     */
    double GetAntennaElementGain() const;

  private:
    // Inherited from Object.
    // Waits for the attribute values to be set before setting the radiation pattern values
    void DoInitialize() override;

    /**
     * Set the radiation pattern Dense Urban – eMBB, Rural – eMBB, Urban Macro – mMTC, and Urban
     * Macro - URLLC, Table 8-6 in Report ITU-R M.2412
     */
    void SetOutdoorAntennaPattern();

    /**
     * Set the radiation pattern for Indoor Hotspot - eMBB,  Table 8-7 in Report ITU-R M.2412
     */
    void SetIndoorAntennaPattern();

    double m_verticalBeamwidthDegrees; //!< beamwidth in the vertical direction \f$(\theta_{3dB})\f$
                                       //!< [deg]
    double m_horizontalBeamwidthDegrees; //!< beamwidth in the horizontal direction
                                         //!< \f$(\phi_{3dB})\f$ [deg]
    double m_aMax;                       //!< maximum attenuation (A_{max}) [dB]
    double m_slaV;  //!< side-lobe attenuation in the vertical direction (SLA_V) [dB]
    double m_geMax; //!< maximum directional gain of the antenna element (G_{E,max}) [dBi]
    RadiationPattern m_radiationPattern; //!< current antenna radiation pattern
};

} // namespace ns3

#endif // THREE_GPP_ANTENNA_MODEL_H
