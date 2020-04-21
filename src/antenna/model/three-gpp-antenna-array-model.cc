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

#include "three-gpp-antenna-array-model.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ThreeGppAntennaArrayModel");

NS_OBJECT_ENSURE_REGISTERED (ThreeGppAntennaArrayModel);

ThreeGppAntennaArrayModel::ThreeGppAntennaArrayModel (void)
{
  NS_LOG_FUNCTION (this);
  m_isOmniTx = false;
}

ThreeGppAntennaArrayModel::~ThreeGppAntennaArrayModel (void)
{
  NS_LOG_FUNCTION (this);
}

TypeId
ThreeGppAntennaArrayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ThreeGppAntennaArrayModel")
    .SetParent<Object> ()
    .AddConstructor<ThreeGppAntennaArrayModel> ()
    .AddAttribute ("AntennaHorizontalSpacing",
               "Horizontal spacing between antenna elements, in multiples of wave length",
               DoubleValue (0.5),
               MakeDoubleAccessor (&ThreeGppAntennaArrayModel::m_disH),
               MakeDoubleChecker<double> ())
    .AddAttribute ("AntennaVerticalSpacing",
               "Vertical spacing between antenna elements, in multiples of wave length",
               DoubleValue (0.5),
               MakeDoubleAccessor (&ThreeGppAntennaArrayModel::m_disV),
               MakeDoubleChecker<double> ())
    .AddAttribute ("NumColumns",
               "Horizontal size of the array",
               UintegerValue (4),
               MakeUintegerAccessor (&ThreeGppAntennaArrayModel::m_numColumns),
               MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NumRows",
               "Vertical size of the array",
               UintegerValue (4),
               MakeUintegerAccessor (&ThreeGppAntennaArrayModel::m_numRows),
               MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("BearingAngle",
               "The bearing angle in radians",
               DoubleValue (0.0),
               MakeDoubleAccessor (&ThreeGppAntennaArrayModel::m_alpha),
               MakeDoubleChecker<double> (-M_PI, M_PI))
    .AddAttribute ("DowntiltAngle",
               "The downtilt angle in radians",
               DoubleValue (0.0),
               MakeDoubleAccessor (&ThreeGppAntennaArrayModel::m_beta),
               MakeDoubleChecker<double> (0, M_PI))
    .AddAttribute ("ElementGain",
               "Directional gain of an antenna element in dBi",
               DoubleValue (4.97),
               MakeDoubleAccessor (&ThreeGppAntennaArrayModel::m_gE),
               MakeDoubleChecker<double> (0, 8))
    .AddAttribute ("IsotropicElements",
               "If true, use an isotropic radiation pattern (for testing purposes)",
               BooleanValue (false),
               MakeBooleanAccessor (&ThreeGppAntennaArrayModel::m_isIsotropic),
               MakeBooleanChecker ())
  ;
  return tid;
}

bool
ThreeGppAntennaArrayModel::IsOmniTx (void) const
{
  NS_LOG_FUNCTION (this);
  return m_isOmniTx;
}

void
ThreeGppAntennaArrayModel::ChangeToOmniTx (void)
{
  NS_LOG_FUNCTION (this);
  m_isOmniTx = true;
}

void
ThreeGppAntennaArrayModel::SetBeamformingVector (const ComplexVector &beamformingVector)
{
  NS_LOG_FUNCTION (this);
  m_isOmniTx = false;
  m_beamformingVector = beamformingVector;
}

const ThreeGppAntennaArrayModel::ComplexVector &
ThreeGppAntennaArrayModel::GetBeamformingVector(void) const
{
  NS_LOG_FUNCTION (this);
  return m_beamformingVector;
}

std::pair<double, double>
ThreeGppAntennaArrayModel::GetElementFieldPattern (Angles a) const
{
  NS_LOG_FUNCTION (this);

  // normalize phi (if needed)
  a.phi = fmod (a.phi + M_PI, 2 * M_PI);
  if (a.phi < 0)
      a.phi += M_PI;
  else 
      a.phi -= M_PI;

  NS_ASSERT_MSG (a.theta >= 0 && a.theta <= M_PI, "The vertical angle should be between 0 and M_PI");
  NS_ASSERT_MSG (a.phi >= -M_PI && a.phi <= M_PI, "The horizontal angle should be between -M_PI and M_PI");

  // convert the theta and phi angles from GCS to LCS using eq. 7.1-7 and 7.1-8 in 3GPP TR 38.901
  // NOTE we assume a fixed slant angle of 0 degrees
  double thetaPrime = std::acos (cos (m_beta)*cos (a.theta) + sin (m_beta)*cos (a.phi-m_alpha)*sin (a.theta));
  double phiPrime = std::arg (std::complex<double> (cos (m_beta)*sin (a.theta)*cos (a.phi-m_alpha) - sin (m_beta)*cos (a.theta), sin (a.phi-m_alpha)*sin (a.theta)));
  NS_LOG_DEBUG (a.theta << " " << thetaPrime << " " << a.phi << " " << phiPrime);

  double aPrimeDb = GetRadiationPattern (thetaPrime, phiPrime);
  double aPrime = pow (10, aPrimeDb / 10); // convert to linear

  // compute psi using eq. 7.1-15 in 3GPP TR 38.901, assuming that the slant 
  // angle (gamma) is 0
  double psi = std::arg (std::complex<double> (cos (m_beta) * sin (a.theta) - sin (m_beta) * cos (a.theta)* cos (a.phi - m_alpha), sin (m_beta)* sin (a.phi-m_alpha)));
  NS_LOG_DEBUG ("psi " << psi);

  // compute the antenna element field pattern in the vertical polarization using
  // eq. 7.3-4 in 3GPP TR 38.901
  // NOTE we assume vertical polarization, hence the field pattern in the
  // horizontal polarization is 0
  double fieldThetaPrime = std::sqrt (aPrime);

  // convert the antenna element field pattern to GCS using eq. 7.1-11
  // in 3GPP TR 38.901
  double fieldTheta = cos (psi) * fieldThetaPrime;
  double fieldPhi = sin (psi) * fieldThetaPrime;
  NS_LOG_DEBUG (a.phi/M_PI*180 << " " << a.theta/M_PI*180 << " " << fieldTheta*fieldTheta + fieldPhi*fieldPhi);

  return std::make_pair (fieldPhi, fieldTheta);
}

double
ThreeGppAntennaArrayModel::GetRadiationPattern (double thetaRadian, double phiRadian) const
{
  if (m_isIsotropic)
  {
    return 0;
  }

  // convert the angles in degrees
  double thetaDeg = thetaRadian * 180 / M_PI;
  double phiDeg = phiRadian * 180 / M_PI;
  NS_ASSERT_MSG (thetaDeg >= 0 && thetaDeg <= 180, "the vertical angle should be the range of [0,180]");
  NS_ASSERT_MSG (phiDeg >= -180 && phiDeg <= 180, "the horizontal angle should be the range of [-180,180]");

  // compute the radiation power pattern using equations in table 7.3-1 in
  // 3GPP TR 38.901
  double A_M = 30; // front-back ratio expressed in dB
  double SLA = 30; // side-lobe level limit expressed in dB

  double A_v = -1 * std::min (SLA,12 * pow ((thetaDeg - 90) / 65,2)); // vertical cut of the radiation power pattern (dB)
  double A_h = -1 * std::min (A_M,12 * pow (phiDeg / 65,2)); // horizontal cut of the radiation power pattern (dB)

  double A = m_gE - 1 * std::min (A_M,- A_v - A_h); // 3D radiation power pattern (dB)

  return A; // 3D radiation power pattern in dB
}

Vector
ThreeGppAntennaArrayModel::GetElementLocation (uint64_t index) const
{
  NS_LOG_FUNCTION (this);

  // compute the element coordinates in the LCS
  // assume the left bottom corner is (0,0,0), and the rectangular antenna array is on the y-z plane.
  double xPrime = 0;
  double yPrime = m_disH * (index % m_numColumns);
  double zPrime = m_disV * floor (index / m_numColumns);

  // convert the coordinates to the GCS using the rotation matrix 7.1-4 in 3GPP
  // TR 38.901
  Vector loc;
  loc.x = cos(m_alpha)*cos (m_beta)*xPrime - sin (m_alpha)*yPrime + cos (m_alpha)*sin (m_beta)*zPrime;
  loc.y = sin (m_alpha)*cos(m_beta)*xPrime + cos (m_alpha)*yPrime + sin (m_alpha)*sin (m_beta)*zPrime;
  loc.z = -sin (m_beta)*xPrime+cos(m_beta)*zPrime;
  return loc;
}

uint64_t
ThreeGppAntennaArrayModel::GetNumberOfElements (void) const
{
  NS_LOG_FUNCTION (this);
  return m_numRows * m_numColumns;
}

} /* namespace ns3 */
