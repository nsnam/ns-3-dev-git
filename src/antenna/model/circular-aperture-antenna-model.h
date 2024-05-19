/*
 * Copyright (c) 2022 University of Padova, Dep. of Information Engineering, SIGNET lab.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mattia Sandri <mattia.sandri@unipd.it>
 */

#ifndef CIRCULAR_APERTURE_ANTENNA_MODEL_H
#define CIRCULAR_APERTURE_ANTENNA_MODEL_H

#include "antenna-model.h"

#include <ns3/object.h>

/**
 * \file
 * \ingroup antenna
 * Class CircularApertureAntennaModel declaration
 */

namespace ns3
{
/**
 * \brief Circular Aperture Antenna Model
 *
 * This class implements the circular aperture antenna as described in 3GPP 38.811 6.4.1
 * https://www.3gpp.org/ftp/Specs/archive/38_series/38.811 without the cosine approximation, thanks
 * to the Bessel functions introduced in C++17. Spherical coordinates are used, in particular of the
 * azimuth and inclination angles. All working parameters can be set, namely: operating frequency,
 * aperture radius, maximum and minimum gain.
 * Since Clang libc++ does not support the Mathematical special functions (P0226R1) yet, this class
 * falls back to Boost's implementation of cyl_bessel_j whenever the above standard library is in
 * use. If neither is available in the host system, this class is not compiled.
 */
class CircularApertureAntennaModel : public AntennaModel
{
  public:
    CircularApertureAntennaModel() = default;
    ~CircularApertureAntennaModel() override = default;

    /**
     * Register this type.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * \brief Set the antenna aperture radius
     *
     * Sets the antenna operating frequency, asserting that
     * the provided value is within the acceptable range [0, +inf[.
     *
     * \param aMeter the strictly positive antenna radius in meters
     */
    void SetApertureRadius(double aMeter);

    /**
     * \brief Return the antenna aperture radius
     *
     * \return the antenna radius in meters
     */
    double GetApertureRadius() const;

    /**
     * \brief Set the antenna operating frequency.
     *
     * Sets the antenna operating frequency, asserting that
     * the provided value is within the acceptable range [0, +inf[.
     *
     * \param freqHz the strictly positive antenna operating frequency, in Hz
     */
    void SetOperatingFrequency(double freqHz);

    /**
     * \brief Return the antenna operating frequency
     *
     * \return the antenna operating frequency, in Hz
     */
    double GetOperatingFrequency() const;

    /**
     * \brief Set the antenna max gain
     *
     * \param gainDb the antenna max gain in dB
     */
    void SetMaxGain(double gainDb);

    /**
     * \brief Return the antenna max gain
     *
     * \return the antenna max gain in dB
     */
    double GetMaxGain() const;

    /**
     * \brief Set the antenna min gain
     *
     * \param gainDb the antenna min gain in dB
     */
    void SetMinGain(double gainDb);

    /**
     * \brief Return the antenna min gain
     *
     * \return the antenna min gain in dB
     */
    double GetMinGain() const;

    /**
     * \brief Get the gain in dB, using Bessel equation of first kind and first order.
     *
     * \param a the angle at which the gain need to be calculated with respect to the antenna
     * bore sight
     *
     * \return the antenna gain at the specified Angles a
     */
    double GetGainDb(Angles a) override;

  private:
    double m_apertureRadiusMeter;  //!< antenna aperture radius in meters
    double m_operatingFrequencyHz; //!< antenna operating frequency in Hz
    double m_maxGain;              //!< antenna gain in dB towards the main orientation
    double m_minGain;              //!< antenna min gain in dB
};

} // namespace ns3

#endif // CIRCULAR_APERTURE_ANTENNA_MODEL_H
