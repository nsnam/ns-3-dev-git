/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018
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

#ifndef BLOCK_ACK_TYPE_H
#define BLOCK_ACK_TYPE_H

#include <ostream>
#include <vector>

namespace ns3 {

/**
 * \ingroup wifi
 * The different BlockAck variants.
 */
struct BlockAckType
{
  /**
   * \enum Variant
   * \brief The BlockAck variants
   */
  enum Variant
  {
    BASIC,
    COMPRESSED,
    EXTENDED_COMPRESSED,
    MULTI_TID
  };
  enum Variant m_variant;           //!< Block Ack variant
  std::vector<uint8_t> m_bitmapLen; //!< Length (bytes) of included bitmaps

  /// Constructors
  BlockAckType ();
  BlockAckType (Variant v);
  BlockAckType (Variant v, std::vector<uint8_t> l);
};

/**
 * \ingroup wifi
 * The different BlockAckRequest variants.
 */
struct BlockAckReqType
{
  /**
   * \enum Variant
   * \brief The BlockAckReq variants
   */
  enum Variant
  {
    BASIC,
    COMPRESSED,
    EXTENDED_COMPRESSED,
    MULTI_TID
  };
  enum Variant m_variant;           //!< Block Ack Request variant
  uint8_t m_nSeqControls;           //!< Number of included Starting Sequence Control fields.
                                    //!< This member is added for future support of Multi-TID BARs

  /// Constructors
  BlockAckReqType ();
  BlockAckReqType (Variant v);
  BlockAckReqType (Variant v, uint8_t nSeqControls);
};

/**
 * Serialize BlockAckType to ostream in a human-readable form.
 *
 * \param os std::ostream
 * \param type block ack type
 * \return std::ostream
 */
std::ostream &operator << (std::ostream &os, const BlockAckType &type);

/**
 * Serialize BlockAckReqType to ostream in a human-readable form.
 *
 * \param os std::ostream
 * \param type block ack request type
 * \return std::ostream
 */
std::ostream &operator << (std::ostream &os, const BlockAckReqType &type);

} //namespace ns3

#endif /* BLOCK_ACK_TYPE_H */
