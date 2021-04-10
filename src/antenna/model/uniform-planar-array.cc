/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2020 University of Padova, Dep. of Information Engineering, SIGNET lab.
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
*/


#include "uniform-planar-array.h"
#include <ns3/log.h>
#include <ns3/double.h>
#include <ns3/uinteger.h>
#include <ns3/boolean.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UniformPlanarArray");

NS_OBJECT_ENSURE_REGISTERED (UniformPlanarArray);



UniformPlanarArray::UniformPlanarArray ()
  : PhasedArrayModel (),
    m_numColumns {1},
    m_numRows {1},
    m_disV {0.5},
    m_disH {0.5},
    m_alpha {0},
    m_beta {0}
{}

UniformPlanarArray::~UniformPlanarArray ()
{}

TypeId
UniformPlanarArray::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UniformPlanarArray")
    .SetParent<PhasedArrayModel> ()
    .AddConstructor<UniformPlanarArray> ()
    .SetGroupName ("Antenna")
    .AddAttribute ("AntennaHorizontalSpacing",
                   "Horizontal spacing between antenna elements, in multiples of wave length",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&UniformPlanarArray::SetAntennaHorizontalSpacing,
                                       &UniformPlanarArray::GetAntennaHorizontalSpacing),
                   MakeDoubleChecker<double> (0.0))
    .AddAttribute ("AntennaVerticalSpacing",
                   "Vertical spacing between antenna elements, in multiples of wave length",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&UniformPlanarArray::SetAntennaVerticalSpacing,
                                       &UniformPlanarArray::GetAntennaVerticalSpacing),
                   MakeDoubleChecker<double> (0.0))
    .AddAttribute ("NumColumns",
                   "Horizontal size of the array",
                   UintegerValue (4),
                   MakeUintegerAccessor (&UniformPlanarArray::SetNumColumns,
                                         &UniformPlanarArray::GetNumColumns),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("NumRows",
                   "Vertical size of the array",
                   UintegerValue (4),
                   MakeUintegerAccessor (&UniformPlanarArray::SetNumRows,
                                         &UniformPlanarArray::GetNumRows),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("BearingAngle",
                   "The bearing angle in radians",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&UniformPlanarArray::m_alpha),
                   MakeDoubleChecker<double> (-M_PI, M_PI))
    .AddAttribute ("DowntiltAngle",
                   "The downtilt angle in radians",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&UniformPlanarArray::m_beta),
                   MakeDoubleChecker<double> (-M_PI, M_PI))
  ;
  return tid;
}


void
UniformPlanarArray::SetNumColumns (uint32_t n)
{
  NS_LOG_FUNCTION (this << n);
  if (n != m_numColumns)
    {
      m_isBfVectorValid = false;
    }
  m_numColumns = n;
}


uint32_t
UniformPlanarArray::GetNumColumns (void) const
{
  return m_numColumns;
}


void
UniformPlanarArray::SetNumRows (uint32_t n)
{
  NS_LOG_FUNCTION (this << n);
  if (n != m_numRows)
    {
      m_isBfVectorValid = false;
    }
  m_numRows = n;
}


uint32_t
UniformPlanarArray::GetNumRows (void) const
{
  return m_numRows;
}


void
UniformPlanarArray::SetAntennaHorizontalSpacing (double s)
{
  NS_LOG_FUNCTION (this << s);
  NS_ABORT_MSG_IF (s <= 0, "Trying to set an invalid spacing: " << s);

  if (s != m_disH)
    {
      m_isBfVectorValid = false;
    }
  m_disH = s;
}


double
UniformPlanarArray::GetAntennaHorizontalSpacing (void) const
{
  return m_disH;
}


void
UniformPlanarArray::SetAntennaVerticalSpacing (double s)
{
  NS_LOG_FUNCTION (this << s);
  NS_ABORT_MSG_IF (s <= 0, "Trying to set an invalid spacing: " << s);

  if (s != m_disV)
    {
      m_isBfVectorValid = false;
    }
  m_disV = s;
}


double
UniformPlanarArray::GetAntennaVerticalSpacing (void) const
{
  return m_disV;
}


std::pair<double, double>
UniformPlanarArray::GetElementFieldPattern (Angles a) const
{
  NS_LOG_FUNCTION (this << a);

  // convert the theta and phi angles from GCS to LCS using eq. 7.1-7 and 7.1-8 in 3GPP TR 38.901
  // NOTE we assume a fixed slant angle of 0 degrees
  double thetaPrime = std::acos (cos (m_beta) * cos (a.GetInclination ()) + sin (m_beta) * cos (a.GetAzimuth () - m_alpha) * sin (a.GetInclination ()));
  double phiPrime = std::arg (std::complex<double> (cos (m_beta) * sin (a.GetInclination ()) * cos (a.GetAzimuth () - m_alpha) - sin (m_beta) * cos (a.GetInclination ()), sin (a.GetAzimuth () - m_alpha) * sin (a.GetInclination ())));
  Angles aPrime (phiPrime, thetaPrime);
  NS_LOG_DEBUG (a << " -> " << aPrime);

  // compute the antenna element field pattern in the vertical polarization using
  // eq. 7.3-4 in 3GPP TR 38.901
  // NOTE we assume vertical polarization, hence the field pattern in the
  // horizontal polarization is 0
  double aPrimeDb = m_antennaElement->GetGainDb (aPrime);
  double fieldThetaPrime = pow (10, aPrimeDb / 20); // convert to linear magnitude

  // compute psi using eq. 7.1-15 in 3GPP TR 38.901, assuming that the slant
  // angle (gamma) is 0
  double psi = std::arg (std::complex<double> (cos (m_beta) * sin (a.GetInclination ()) - sin (m_beta) * cos (a.GetInclination ()) * cos (a.GetAzimuth () - m_alpha), sin (m_beta) * sin (a.GetAzimuth () - m_alpha)));
  NS_LOG_DEBUG ("psi " << psi);

  // convert the antenna element field pattern to GCS using eq. 7.1-11
  // in 3GPP TR 38.901
  double fieldTheta = cos (psi) * fieldThetaPrime;
  double fieldPhi = sin (psi) * fieldThetaPrime;
  NS_LOG_DEBUG (RadiansToDegrees (a.GetAzimuth ()) << " " << RadiansToDegrees (a.GetInclination ()) << " " << fieldTheta * fieldTheta + fieldPhi * fieldPhi);

  return std::make_pair (fieldPhi, fieldTheta);
}


Vector
UniformPlanarArray::GetElementLocation (uint64_t index) const
{
  NS_LOG_FUNCTION (this << index);

  // compute the element coordinates in the LCS
  // assume the left bottom corner is (0,0,0), and the rectangular antenna array is on the y-z plane.
  double xPrime = 0;
  double yPrime = m_disH * (index % m_numColumns);
  double zPrime = m_disV * floor (index / m_numColumns);

  // convert the coordinates to the GCS using the rotation matrix 7.1-4 in 3GPP
  // TR 38.901
  Vector loc;
  loc.x = cos (m_alpha) * cos (m_beta) * xPrime - sin (m_alpha) * yPrime + cos (m_alpha) * sin (m_beta) * zPrime;
  loc.y = sin (m_alpha) * cos (m_beta) * xPrime + cos (m_alpha) * yPrime + sin (m_alpha) * sin (m_beta) * zPrime;
  loc.z = -sin (m_beta) * xPrime + cos (m_beta) * zPrime;
  return loc;
}

uint64_t
UniformPlanarArray::GetNumberOfElements () const
{
  return m_numRows * m_numColumns;
}

} /* namespace ns3 */
