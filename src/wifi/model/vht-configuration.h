/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018  Sébastien Deronne
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef VHT_CONFIGURATION_H
#define VHT_CONFIGURATION_H

#include "ns3/object.h"

namespace ns3 {

/**
 * \brief VHT configuration
 * \ingroup wifi
 *
 * This object stores VHT configuration information, for use in modifying
 * AP or STA behavior and for constructing VHT-related information elements.
 *
 */
class VhtConfiguration : public Object
{
public:
  VhtConfiguration ();
  virtual ~VhtConfiguration ();

  static TypeId GetTypeId (void);


private:
  uint16_t m_voMaxAmsduSize; ///< maximum A-MSDU size for AC_VO
  uint16_t m_viMaxAmsduSize; ///< maximum A-MSDU size for AC_VI
  uint16_t m_beMaxAmsduSize; ///< maximum A-MSDU size for AC_BE
  uint16_t m_bkMaxAmsduSize; ///< maximum A-MSDU size for AC_BK

  uint32_t m_voMaxAmpduSize; ///< maximum A-MPDU size for AC_VO
  uint32_t m_viMaxAmpduSize; ///< maximum A-MPDU size for AC_VI
  uint32_t m_beMaxAmpduSize; ///< maximum A-MPDU size for AC_BE
  uint32_t m_bkMaxAmpduSize; ///< maximum A-MPDU size for AC_BK
};

} //namespace ns3

#endif /* VHT_CONFIGURATION_H */
