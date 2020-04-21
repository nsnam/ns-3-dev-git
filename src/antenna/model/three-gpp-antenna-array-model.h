/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
*   University of Padova
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License version 2 as
*   published by the Free Software Foundation;
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

#ifndef THREE_GPP_ANTENNA_ARRAY_MODEL_H_
#define THREE_GPP_ANTENNA_ARRAY_MODEL_H_

#include <ns3/antenna-model.h>
#include <complex>

namespace ns3 {

/**
 * \ingroup antenna
 *
 * \brief Class implementing the antenna model defined in 3GPP TR 38.901 V15.0.0
 * 
 * \note the current implementation supports the modeling of antenna arrays 
 * composed of a single panel and with single (vertical) polarization.
 */
class ThreeGppAntennaArrayModel : public Object
{
public:

  /**
   * Constructor
   */
  ThreeGppAntennaArrayModel (void);

  /**
   * Destructor
   */
  virtual ~ThreeGppAntennaArrayModel (void);

  // inherited from Object
  static TypeId GetTypeId (void);

  typedef std::vector<std::complex<double> > ComplexVector; //!< type definition for complex vectors

  /**
   * Returns the horizontal and vertical components of the antenna element field
   * pattern at the specified direction. Only vertical polarization is considered.
   * \param a the angle indicating the interested direction
   * \return a pair in which the first element is the horizontal component
   *         of the field pattern and the second element is the vertical
   *         component of the field pattern
   */
  std::pair<double, double> GetElementFieldPattern (Angles a) const;

  /**
   * Returns the location of the antenna element with the specified
   * index assuming the left bottom corner is (0,0,0), normalized
   * with respect to the wavelength.
   * Antenna elements are scanned row by row, left to right and bottom to top.
   * For example, an antenna with 2 rows and 3 columns will be ordered as follows:
   * ^ z
   * |  3 4 5
   * |  0 1 2
   * ----------> y
   *
   * \param index index of the antenna element
   * \return the 3D vector that represents the position of the element
   */
  virtual Vector GetElementLocation (uint64_t index) const;

  /**
   * Returns the number of antenna elements
   * \return the number of antenna elements
   */
  virtual uint64_t GetNumberOfElements (void) const;

  /**
   * Returns true if the antenna is configured for omnidirectional transmissions
   * \return whether the transmission is set to omni
   */
  bool IsOmniTx (void) const;

  /**
   * Change the antenna model to omnidirectional (ignoring the beams)
   */
  void ChangeToOmniTx (void);

  /**
   * Sets the beamforming vector to be used
   * \param beamformingVector the beamforming vector
   */
  void SetBeamformingVector (const ComplexVector &beamformingVector);

  /**
   * Returns the beamforming vector that is currently being used
   * \return the current beamforming vector
   */
  const ComplexVector & GetBeamformingVector (void) const;

private:
  /**
   * Returns the radiation power pattern of a single antenna element in dB,
   * generated according to Table 7.3-1 in 3GPP TR 38.901
   * \param vAngleRadian the vertical angle in radians
   * \param hAngleRadian the horizontal angle in radians
   * \return the radiation power pattern in dB
   */
  double GetRadiationPattern (double vAngleRadian, double hAngleRadian) const;

  bool m_isOmniTx; //!< true if the antenna is configured for omni transmissions
  ComplexVector m_beamformingVector; //!< the beamforming vector in use
  uint32_t m_numColumns; //!< number of columns
  uint32_t m_numRows; //!< number of rows
  double m_disV; //!< antenna spacing in the vertical direction in multiples of wave length
  double m_disH; //!< antenna spacing in the horizontal direction in multiples of wave length
  double m_alpha; //!< the bearing angle in radians
  double m_beta; //!< the downtilt angle in radians
  double m_gE; //!< directional gain of a single antenna element (dBi)
  bool m_isIsotropic; //!< if true, antenna elements are isotropic
};

} /* namespace ns3 */

#endif /* SRC_THREE_GPP_ANTENNA_ARRAY_MODEL_H_ */
