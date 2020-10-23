/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Orange Labs
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
 * Authors: Rediet <getachew.redieteab@orange.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com> (for logic ported from wifi-phy)
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr> (for logic ported from wifi-phy)
 */

#ifndef PHY_ENTITY_H
#define PHY_ENTITY_H

#include "wifi-mpdu-type.h"
#include "wifi-tx-vector.h"
#include "wifi-phy-band.h"
#include "ns3/simple-ref-count.h"
#include "ns3/nstime.h"
#include <list>
#include <map>

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::PhyEntity class.
 */

namespace ns3 {

/**
 * \brief Abstract class for PHY entities
 * \ingroup wifi
 *
 * This class enables to have a unique set of APIs
 * to be used by each PHY entity, corresponding to
 * the different amendments of the IEEE 802.11 standard.
 */
class PhyEntity : public SimpleRefCount<PhyEntity>
{
public:
  /**
   * Destructor for PHY entity
   */
  virtual ~PhyEntity ();

  /**
   * Check if the WifiMode is supported.
   *
   * \param mode the WifiMode to check
   * \return true if the WifiMode is supported,
   *         false otherwise
   */
  virtual bool IsModeSupported (WifiMode mode) const;
  /**
   * \return the number of WifiModes supported by this entity
   */
  virtual uint8_t GetNumModes (void) const;

  /**
   * Get the WifiMode corresponding to the given MCS index.
   *
   * \param index the index of the MCS
   * \return the WifiMode corresponding to the MCS index
   *
   * This method should be used only for HtPhy and child classes.
   */
  virtual WifiMode GetMcs (uint8_t index) const;
  /**
   * Check if the WifiMode corresponding to the given MCS index is supported.
   *
   * \param index the index of the MCS
   * \return true if the WifiMode corresponding to the MCS index is supported,
   *         false otherwise
   *
   * Will return false for non-MCS modes.
   */
  virtual bool IsMcsSupported (uint8_t index) const;
  /**
   * Check if the WifiModes handled by this PHY are MCSs.
   *
   * \return true if the handled WifiModes are MCSs,
   *         false if they are non-MCS modes
   */
  virtual bool HandlesMcsModes (void) const;

  /**
   * Get the WifiMode for the SIG field specified by the PPDU field.
   *
   * \param field the PPDU field
   * \param txVector the transmission parameters
   *
   * \return the WifiMode used for the SIG field
   *
   * This method is overridden by child classes.
   */
  virtual WifiMode GetSigMode (WifiPpduField field, WifiTxVector txVector) const;

  /**
   * \brief Return a const iterator to the first WifiMode
   *
   * \return a const iterator to the first WifiMode.
   */
  std::list<WifiMode>::const_iterator begin (void) const;
  /**
   * \brief Return a const iterator to past-the-last WifiMode
   *
   * \return a const iterator to past-the-last WifiMode.
   */
  std::list<WifiMode>::const_iterator end (void) const;

  /**
   * Return the field following the provided one.
   *
   * \param currentField the considered PPDU field
   * \param preamble the preamble indicating the PPDU format
   * \return the PPDU field following the reference one
   */
  WifiPpduField GetNextField (WifiPpduField currentField, WifiPreamble preamble) const;

  /**
   * Get the duration of the PPDU field (or group of fields)
   * used by this entity for the given transmission parameters.
   *
   * \param field the PPDU field (or group of fields)
   * \param txVector the transmission parameters
   *
   * \return the duration of the PPDU field
   *
   * This method is overridden by child classes.
   */
  virtual Time GetDuration (WifiPpduField field, WifiTxVector txVector) const;
  /**
   * \param txVector the transmission parameters
   *
   * \return the total duration of the PHY preamble and PHY header.
   */
  Time CalculatePhyPreambleAndHeaderDuration (WifiTxVector txVector) const;

  /**
   * \param size the number of bytes in the packet to send
   * \param txVector the TXVECTOR used for the transmission of this packet
   * \param band the frequency band
   * \param mpdutype the type of the MPDU as defined in WifiPhy::MpduType.
   * \param incFlag this flag is used to indicate that the variables need to be update or not
   * This function is called a couple of times for the same packet so variables should not be increased each time.
   * \param totalAmpduSize the total size of the previously transmitted MPDUs for the concerned A-MPDU.
   * If incFlag is set, this parameter will be updated.
   * \param totalAmpduNumSymbols the number of symbols previously transmitted for the MPDUs in the concerned A-MPDU,
   * used for the computation of the number of symbols needed for the last MPDU.
   * If incFlag is set, this parameter will be updated.
   * \param staId the STA-ID of the PSDU (only used for MU PPDUs)
   *
   * \return the duration of the PSDU
   */
  virtual Time GetPayloadDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand band, MpduType mpdutype,
                                   bool incFlag, uint32_t &totalAmpduSize, double &totalAmpduNumSymbols,
                                   uint16_t staId) const = 0;

  /**
   * A pair containing information on the PHY header chunk, namely
   * the start and stop times of the chunk and the WifiMode used.
   */
  typedef std::pair<std::pair<Time /* start */, Time /* stop */>, WifiMode> PhyHeaderChunkInfo;
  /**
   * A map of PhyHeaderChunkInfo elements per PPDU field.
   * \see PhyHeaderChunkInfo
   */
  typedef std::map<WifiPpduField, PhyHeaderChunkInfo> PhyHeaderSections;
  /**
   * Return a map of PHY header chunk information per PPDU field.
   * This map will contain the PPDU fields that are actually present based
   * on the \p txVector information.
   *
   * \param txVector the transmission parameters
   * \param ppduStart the time at which the PPDU started
   * \return the list of preamble sections
   *
   * \see PhyHeaderSections
   */
  PhyHeaderSections GetPhyHeaderSections (WifiTxVector txVector, Time ppduStart) const;

protected:
  /**
   * A map of PPDU field elements per preamble type.
   * This corresponds to the different PPDU formats introduced by each amendment.
   */
  typedef std::map<WifiPreamble, std::vector<WifiPpduField> > PpduFormats;

  /**
   * Return the PPDU formats of the PHY.
   *
   * This method should be implemented (overridden) by each child
   * class introducing new formats.
   *
   * \return the PPDU formats of the PHY
   */
  virtual const PpduFormats & GetPpduFormats (void) const = 0;

  std::list<WifiMode> m_modeList; //!< the list of supported modes
}; //class PhyEntity

} //namespace ns3

#endif /* PHY_ENTITY_H */
