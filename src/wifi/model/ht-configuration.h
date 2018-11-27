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

#ifndef HT_CONFIGURATION_H
#define HT_CONFIGURATION_H

#include "ns3/object.h"

namespace ns3 {

/**
 * \brief HT configuration
 * \ingroup wifi
 *
 * This object stores HT configuration information, for use in modifying
 * AP or STA behavior and for constructing HT-related information elements.
 *
 */
class HtConfiguration : public Object
{
public:
  HtConfiguration ();
  virtual ~HtConfiguration ();

  static TypeId GetTypeId (void);

  /**
   * Enable or disable SGI support.
   *
   * \param enable true if SGI is to be supported,
   *               false otherwise
   */
  void SetShortGuardIntervalSupported (bool enable);
  /**
   * \return whether the device supports SGI.
   *
   * \return true if SGI is supported,
   *         false otherwise.
   */
  bool GetShortGuardIntervalSupported (void) const;
  /**
   * Enable or disable Greenfield support.
   *
   * \param enable true if Greenfield is to be supported,
   *               false otherwise
   */
  void SetGreenfieldSupported (bool enable);
  /**
   * \return whether the device supports Greenfield.
   *
   * \return true if Greenfield is supported,
   *         false otherwise.
   */
  bool GetGreenfieldSupported (void) const;
  /**
   * Enable or disable RIFS support.
   *
   * \param enable true if RIFS is to be supported,
   *               false otherwise
   */
  void SetRifsSupported (bool enable);
  /**
   * \return whether the device supports RIFS capability.
   *
   * \return true if short RIFS is supported,
   *         false otherwise.
   */
  bool GetRifsSupported (void) const;


private:
  bool m_sgiSupported;        ///< flag whether short guard interval is supported
  bool m_rifsSupported;       ///< flag whether RIFS is supported
  bool m_greenfieldSupported; ///< flag whether Greenfield is supported

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

#endif /* HT_CONFIGURATION_H */
