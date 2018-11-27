/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Washington
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
 */

#ifndef HE_CONFIGURATION_H
#define HE_CONFIGURATION_H

#include "ns3/object.h"

namespace ns3 {

/**
 * \brief HE configuration
 * \ingroup wifi
 *
 * This object stores HE configuration information, for use in modifying
 * AP or STA behavior and for constructing HE-related information elements.
 *
 */
class HeConfiguration : public Object
{
public:
  HeConfiguration ();
  static TypeId GetTypeId (void);

  /**
   * \param guardInterval the supported HE guard interval
   */
  void SetGuardInterval (Time guardInterval);
  /**
   * \return the supported HE guard interval
   */
  Time GetGuardInterval (void) const;
  /**
   * \param size the MPDU buffer size to receive A-MPDUs
   */
  void SetMpduBufferSize (uint16_t size);
  /**
   * \return the MPDU buffer size to receive A-MPDUs
   */
  uint16_t GetMpduBufferSize (void) const;


private:
  Time m_guardInterval;      //!< Supported HE guard interval
  uint8_t m_bssColor;        //!< BSS color
  uint16_t m_mpduBufferSize; //!< MPDU buffer size

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

#endif /* HE_CONFIGURATION_H */
