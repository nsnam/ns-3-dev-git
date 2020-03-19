/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#ifndef CTRL_HEADERS_H
#define CTRL_HEADERS_H

#include "ns3/header.h"
#include "block-ack-type.h"

namespace ns3 {

/**
 * \ingroup wifi
 * \brief Headers for BlockAckRequest.
 *
 *  802.11n standard includes three types of BlockAck:
 *    - Basic BlockAck (unique type in 802.11e)
 *    - Compressed BlockAck
 *    - Multi-TID BlockAck
 *  For now only basic BlockAck and compressed BlockAck
 *  are supported.
 *  Basic BlockAck is also default variant.
 */
class CtrlBAckRequestHeader : public Header
{
public:
  CtrlBAckRequestHeader ();
  ~CtrlBAckRequestHeader ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Enable or disable HT immediate Ack.
   *
   * \param immediateAck enable or disable HT immediate Ack
   */
  void SetHtImmediateAck (bool immediateAck);
  /**
   * Set the block ack type.
   *
   * \param type the BA type
   */
  void SetType (BlockAckType type);
  /**
   * Set Traffic ID (TID).
   *
   * \param tid the TID
   */
  void SetTidInfo (uint8_t tid);
  /**
   * Set the starting sequence number from the given
   * raw sequence control field.
   *
   * \param seq the raw sequence control
   */
  void SetStartingSequence (uint16_t seq);

  /**
   * Check if the current Ack Policy is immediate.
   *
   * \return true if the current Ack Policy is immediate,
   *         false otherwise
   */
  bool MustSendHtImmediateAck (void) const;
  /**
   * Return the Block Ack type ID.
   *
   * \return the BA type
   */
  BlockAckType GetType (void) const;
  /**
   * Return the Traffic ID (TID).
   *
   * \return TID
   */
  uint8_t GetTidInfo (void) const;
  /**
   * Return the starting sequence number.
   *
   * \return the starting sequence number
   */
  uint16_t GetStartingSequence (void) const;
  /**
   * Check if the current Ack Policy is Basic Block Ack
   * (i.e. not multi-TID nor compressed).
   *
   * \return true if the current Ack Policy is Basic Block Ack,
   *         false otherwise
   */
  bool IsBasic (void) const;
  /**
   * Check if the current Ack Policy is Compressed Block Ack
   * and not multi-TID.
   *
   * \return true if the current Ack Policy is Compressed Block Ack,
   *         false otherwise
   */
  bool IsCompressed (void) const;
  /**
   * Check if the current Ack Policy is Extended Compressed Block Ack.
   *
   * \return true if the current Ack Policy is Extended Compressed Block Ack,
   *         false otherwise
   */
  bool IsExtendedCompressed (void) const;
  /**
   * Check if the current Ack Policy has Multi-TID Block Ack.
   *
   * \return true if the current Ack Policy has Multi-TID Block Ack,
   *         false otherwise
   */
  bool IsMultiTid (void) const;

  /**
   * Return the starting sequence control.
   *
   * \return the starting sequence control
   */
  uint16_t GetStartingSequenceControl (void) const;


private:
  /**
   * Set the starting sequence control with the given
   * sequence control value
   *
   * \param seqControl the sequence control value
   */
  void SetStartingSequenceControl (uint16_t seqControl);
  /**
   * Return the Block Ack control.
   *
   * \return the Block Ack control
   */
  uint16_t GetBarControl (void) const;
  /**
   * Set the Block Ack control.
   *
   * \param bar the BAR control value
   */
  void SetBarControl (uint16_t bar);

  /**
   * The LSB bit of the BAR control field is used only for the
   * HT (High Throughput) delayed block ack configuration.
   * For now only non HT immediate BlockAck is implemented so this field
   * is here only for a future implementation of HT delayed variant.
   */
  bool m_barAckPolicy;    ///< BAR Ack Policy
  BlockAckType m_baType;  ///< BA type
  uint16_t m_tidInfo;     ///< TID info
  uint16_t m_startingSeq; ///< starting sequence number
};


/**
 * \ingroup wifi
 * \brief Headers for BlockAck response.
 *
 *  802.11n standard includes three types of BlockAck:
 *    - Basic BlockAck (unique type in 802.11e)
 *    - Compressed BlockAck
 *    - Multi-TID BlockAck
 *  For now only basic BlockAck and compressed BlockAck
 *  are supported.
 *  Basic BlockAck is also default variant.
 */
class CtrlBAckResponseHeader : public Header
{
public:
  CtrlBAckResponseHeader ();
  ~CtrlBAckResponseHeader ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Enable or disable HT immediate Ack.
   *
   * \param immediateAck enable or disable HT immediate Ack
   */
  void SetHtImmediateAck (bool immediateAck);
  /**
   * Set the block ack type.
   *
   * \param type the BA type
   */
  void SetType (BlockAckType type);
  /**
   * Set Traffic ID (TID).
   *
   * \param tid the TID
   */
  void SetTidInfo (uint8_t tid);
  /**
   * Set the starting sequence number from the given
   * raw sequence control field.
   *
   * \param seq the raw sequence control
   */
  void SetStartingSequence (uint16_t seq);

  /**
   * Check if the current Ack Policy is immediate.
   *
   * \return true if the current Ack Policy is immediate,
   *         false otherwise
   */
  bool MustSendHtImmediateAck (void) const;
  /**
   * Return the block ack type ID.
   *
   * \return type
   */
  BlockAckType GetType (void) const;
  /**
   * Return the Traffic ID (TID).
   *
   * \return TID
   */
  uint8_t GetTidInfo (void) const;
  /**
   * Return the starting sequence number.
   *
   * \return the starting sequence number
   */
  uint16_t GetStartingSequence (void) const;
  /**
   * Check if the current BA policy is Basic Block Ack.
   *
   * \return true if the current BA policy is Basic Block Ack,
   *         false otherwise
   */
  bool IsBasic (void) const;
  /**
   * Check if the current BA policy is Compressed Block Ack.
   *
   * \return true if the current BA policy is Compressed Block Ack,
   *         false otherwise
   */
  bool IsCompressed (void) const;
  /**
   * Check if the current BA policy is Extended Compressed Block Ack.
   *
   * \return true if the current BA policy is Extended Compressed Block Ack,
   *         false otherwise
   */
  bool IsExtendedCompressed (void) const;
  /**
   * Check if the current BA policy is Multi-TID Block Ack.
   *
   * \return true if the current BA policy is Multi-TID Block Ack,
   *         false otherwise
   */
  bool IsMultiTid (void) const;

  /**
   * Set the bitmap that the packet with the given sequence
   * number was received.
   *
   * \param seq the sequence number
   */
  void SetReceivedPacket (uint16_t seq);
  /**
   * Set the bitmap that the packet with the given sequence
   * number and fragment number was received.
   *
   * \param seq the sequence number
   * \param frag the fragment number
   */
  void SetReceivedFragment (uint16_t seq, uint8_t frag);
  /**
   * Check if the packet with the given sequence number
   * was acknowledged in this BlockAck response.
   *
   * \param seq the sequence number
   * \return true if the packet with the given sequence number
   *         was ACKed in this BlockAck response, false otherwise
   */
  bool IsPacketReceived (uint16_t seq) const;
  /**
   * Check if the packet with the given sequence number
   * and fragment number was acknowledged in this BlockAck response.
   *
   * \param seq the sequence number
   * \param frag the fragment number
   * \return true if the packet with the given sequence number
   *         and sequence number was acknowledged in this BlockAck response,
   *         false otherwise
   */
  bool IsFragmentReceived (uint16_t seq, uint8_t frag) const;

  /**
   * Return the starting sequence control.
   *
   * \return the starting sequence control
   */
  uint16_t GetStartingSequenceControl (void) const;
  /**
   * Set the starting sequence control with the given
   * sequence control value
   *
   * \param seqControl the raw sequence control value
   */
  void SetStartingSequenceControl (uint16_t seqControl);
  /**
   * Return the bitmap from the BlockAck response header.
   *
   * \return the bitmap from the BlockAck response header
   */
  const uint16_t* GetBitmap (void) const;
  /**
   * Return the compressed bitmap from the BlockAck response header.
   *
   * \return the compressed bitmap from the BlockAck response header
   */
  uint64_t GetCompressedBitmap (void) const;
  /**
   * Return the extended compressed bitmap from the BlockAck response header.
   *
   * \return the extended compressed bitmap from the BlockAck response header
   */
  const uint64_t* GetExtendedCompressedBitmap (void) const;

  /**
   * Reset the bitmap to 0.
   */
  void ResetBitmap (void);


private:
  /**
   * Return the Block Ack control.
   *
   * \return the Block Ack control
   */
  uint16_t GetBaControl (void) const;
  /**
   * Set the Block Ack control.
   *
   * \param ba the BA control to set
   */
  void SetBaControl (uint16_t ba);

  /**
   * Serialize bitmap to the given buffer.
   *
   * \param start the iterator
   * \return Buffer::Iterator to the next available buffer
   */
  Buffer::Iterator SerializeBitmap (Buffer::Iterator start) const;
  /**
   * Deserialize bitmap from the given buffer.
   *
   * \param start the iterator
   * \return Buffer::Iterator to the next available buffer
   */
  Buffer::Iterator DeserializeBitmap (Buffer::Iterator start);

  /**
   * This function is used to correctly index in both bitmap
   * and compressed bitmap, one bit or one block of 16 bits respectively.
   *
   * for more details see 7.2.1.8 in IEEE 802.11n/D4.00
   *
   * \param seq the sequence number
   *
   * \return If we are using basic block ack, return value represents index of
   * block of 16 bits for packet having sequence number equals to <i>seq</i>.
   * If we are using compressed block ack, return value represents bit
   * to set to 1 in the compressed bitmap to indicate that packet having
   * sequence number equals to <i>seq</i> was correctly received.
   */
  uint16_t IndexInBitmap (uint16_t seq) const;

  /**
   * Checks if sequence number <i>seq</i> can be acknowledged in the bitmap.
   *
   * \param seq the sequence number
   *
   * \return true if the sequence number is concerned by the bitmap
   */
  bool IsInBitmap (uint16_t seq) const;

  /**
   * The LSB bit of the BA control field is used only for the
   * HT (High Throughput) delayed block ack configuration.
   * For now only non HT immediate block ack is implemented so this field
   * is here only for a future implementation of HT delayed variant.
   */
  bool m_baAckPolicy;     ///< BA Ack Policy
  BlockAckType m_baType;  ///< BA type
  uint16_t m_tidInfo;     ///< TID info
  uint16_t m_startingSeq; ///< starting sequence number

  union
  {
    uint16_t m_bitmap[64]; ///< the basic BlockAck bitmap
    uint64_t m_compressedBitmap; ///< the compressed BlockAck bitmap
    uint64_t m_extendedCompressedBitmap[4]; ///< the extended compressed BlockAck bitmap
  } bitmap; ///< bitmap union type
};

} //namespace ns3

#endif /* CTRL_HEADERS_H */
