/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */


#include "eps-bearer.h"

#include <ns3/fatal-error.h>


namespace ns3 {


GbrQosInformation::GbrQosInformation ()
  : gbrDl (0),
    gbrUl (0),
    mbrDl (0),
    mbrUl (0)
{
}

AllocationRetentionPriority::AllocationRetentionPriority ()
  : priorityLevel (0),
    preemptionCapability (false),
    preemptionVulnerability (false)
{
}

EpsBearer::EpsBearer ()
  : qci (NGBR_VIDEO_TCP_DEFAULT)
{
  m_requirements = GetRequirementsRel11 ();
}

EpsBearer::EpsBearer (Qci x)
  : qci (x)
{
  m_requirements = GetRequirementsRel11 ();
}

EpsBearer::EpsBearer (Qci x, struct GbrQosInformation y)
  : qci (x), gbrQosInfo (y)
{
  m_requirements = GetRequirementsRel11 ();
}

bool
EpsBearer::IsGbr () const
{
  return IsGbr (m_requirements, qci);
}

uint8_t
EpsBearer::GetPriority () const
{
  return GetPriority (m_requirements, qci);
}

uint16_t
EpsBearer::GetPacketDelayBudgetMs () const
{
  return GetPacketDelayBudgetMs (m_requirements, qci);
}

double
EpsBearer::GetPacketErrorLossRate () const
{
  return GetPacketErrorLossRate (m_requirements, qci);
}

EpsBearer::BearerRequirementsMap
EpsBearer::GetRequirementsRel11 ()
{
  /* Needed to support GCC 4.9. Otherwise, use list constructors, for example:
   * EpsBearer::BearerRequirementsMap
   * EpsBearer::GetRequirementsRel15 ()
   * {
   *   return
   *     {
   *       { GBR_CONV_VOICE          , { true,  20, 100, 1.0e-2,    0, 2000} },
   *       ...
   *     };
   * }
   */
  static EpsBearer::BearerRequirementsMap ret;

  if (ret.size () == 0)
    {
      ret.insert (std::make_pair (GBR_CONV_VOICE,          std::make_tuple (true,  2, 100, 1.0e-2,    0,    0)));
      ret.insert (std::make_pair (GBR_CONV_VIDEO,          std::make_tuple (true,  4, 150, 1.0e-3,    0,    0)));
      ret.insert (std::make_pair (GBR_GAMING,              std::make_tuple (true,  3,  50, 1.0e-3,    0,    0)));
      ret.insert (std::make_pair (GBR_NON_CONV_VIDEO,      std::make_tuple (true,  5, 300, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (NGBR_IMS,                std::make_tuple (false, 1, 100, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (NGBR_VIDEO_TCP_OPERATOR, std::make_tuple (false, 6, 300, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (NGBR_VOICE_VIDEO_GAMING, std::make_tuple (false, 7, 100, 1.0e-3,    0,    0)));
      ret.insert (std::make_pair (NGBR_VIDEO_TCP_PREMIUM,  std::make_tuple (false, 8, 300, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (NGBR_VIDEO_TCP_DEFAULT,  std::make_tuple (false, 9, 300, 1.0e-6,    0,    0)));
    }
  return ret;
}

EpsBearer::BearerRequirementsMap
EpsBearer::GetRequirementsRel15 ()
{
  // Needed to support GCC 4.9. Otherwise, use list constructors (see GetRequirementsRel10)
  static EpsBearer::BearerRequirementsMap ret;

  if (ret.size () == 0)
    {
      ret.insert (std::make_pair (GBR_CONV_VOICE,          std::make_tuple (true,  20, 100, 1.0e-2,    0, 2000)));
      ret.insert (std::make_pair (GBR_CONV_VIDEO,          std::make_tuple (true,  40, 150, 1.0e-3,    0, 2000)));
      ret.insert (std::make_pair (GBR_GAMING,              std::make_tuple (true,  30,  50, 1.0e-3,    0, 2000)));
      ret.insert (std::make_pair (GBR_NON_CONV_VIDEO,      std::make_tuple (true,  50, 300, 1.0e-6,    0, 2000)));
      ret.insert (std::make_pair (GBR_MC_PUSH_TO_TALK,     std::make_tuple (true,   7,  75, 1.0e-2,    0, 2000)));
      ret.insert (std::make_pair (GBR_NMC_PUSH_TO_TALK,    std::make_tuple (true,  20, 100, 1.0e-2,    0, 2000)));
      ret.insert (std::make_pair (GBR_MC_VIDEO,            std::make_tuple (true,  15, 100, 1.0e-3,    0, 2000)));
      ret.insert (std::make_pair (GBR_V2X,                 std::make_tuple (true,  25,  50, 1.0e-2,    0, 2000)));
      ret.insert (std::make_pair (NGBR_IMS,                std::make_tuple (false, 10, 100, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (NGBR_VIDEO_TCP_OPERATOR, std::make_tuple (false, 60, 300, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (NGBR_VOICE_VIDEO_GAMING, std::make_tuple (false, 70, 100, 1.0e-3,    0,    0)));
      ret.insert (std::make_pair (NGBR_VIDEO_TCP_PREMIUM,  std::make_tuple (false, 80, 300, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (NGBR_VIDEO_TCP_DEFAULT,  std::make_tuple (false, 90, 300, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (NGBR_MC_DELAY_SIGNAL,    std::make_tuple (false,  5,  60, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (NGBR_MC_DATA,            std::make_tuple (false, 55, 200, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (NGBR_V2X,                std::make_tuple (false, 65,   5, 1.0e-2,    0,    0)));
      ret.insert (std::make_pair (NGBR_LOW_LAT_EMBB,       std::make_tuple (false, 68,  10, 1.0e-6,    0,    0)));
      ret.insert (std::make_pair (DGBR_DISCRETE_AUT_SMALL, std::make_tuple (false, 19,  10, 1.0e-4,  255, 2000)));
      ret.insert (std::make_pair (DGBR_DISCRETE_AUT_LARGE, std::make_tuple (false, 22,  10, 1.0e-4, 1358, 2000)));
      ret.insert (std::make_pair (DGBR_ITS,                std::make_tuple (false, 24,  30, 1.0e-5, 1354, 2000)));
      ret.insert (std::make_pair (DGBR_ELECTRICITY,        std::make_tuple (false, 21,  5,  1.0e-5,  255, 2000)));
    }
  return ret;
}

} // namespace ns3
