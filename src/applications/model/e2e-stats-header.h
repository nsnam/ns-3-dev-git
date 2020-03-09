/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2018 Natale Patriciello <natale.patriciello@gmail.com>
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

#pragma once

#include <ns3/seq-ts-header.h>

namespace ns3 {
/**
 * \ingroup applications
 * \brief Header with a sequence, a timestamp, and a "size" attribute
 *
 * Sometimes, you would need an header that not only track an application
 * sequence number, or an application timestamp, but also track
 * how big are these application packets.
 *
 * This header extends SeqTsHeader, adding space to store the information
 * about the size of these packets.
 *
 * When you will use a protocol like TCP, you will find the answer to the question
 * "isn't SeqTsHeader enough?".
 */
class E2eStatsHeader : public SeqTsHeader
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * @brief SizeHeader constructor
   */
  E2eStatsHeader ();

  /**
   * \brief ~SizeHeader
   *
   * Nothing much to add here
   */
  virtual ~E2eStatsHeader () override
  {
  }

  /**
   * @brief Set the size information that the header will carry
   * @param size the size
   */
  void SetSize (uint64_t size)
  {
    m_size = size;
  }
  /**
   * @brief Get the size information that the header is carrying
   * @return the size
   */
  uint64_t GetSize (void) const
  {
    return m_size;
  }

  // Inherited
  virtual TypeId GetInstanceTypeId (void) const override;
  virtual void Print (std::ostream &os) const override;
  virtual uint32_t GetSerializedSize (void) const override;
  virtual void Serialize (Buffer::Iterator start) const override;
  virtual uint32_t Deserialize (Buffer::Iterator start) override;

private:
  uint64_t m_size {0}; //!< The 'size' information that the header is carrying
};

} // namespace ns3
