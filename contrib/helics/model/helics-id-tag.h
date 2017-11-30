/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef HELICS_ID_TAG_H
#define HELICS_ID_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class HelicsIdTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer buf) const;
  virtual void Deserialize (TagBuffer buf);
  virtual void Print (std::ostream &os) const;
  HelicsIdTag ();
  
  /**
   *  Constructs a HelicsIdTag with the given helics id
   *
   *  \param helicsId Id to use for the tag
   */
  HelicsIdTag (uint32_t helicsId);
  /**
   *  Sets the helics id for the tag
   *  \param helicsId Id to assign to the tag
   */
  void SetHelicsId (uint32_t helicsId);
  /**
   *  Gets the helics id for the tag
   *  \returns current helics id for this tag
   */
  uint32_t GetHelicsId (void) const;
  /**
   *  Gets the helics id for the tag
   */
  operator uint32_t() const { return GetHelicsId(); }
private:
  uint32_t m_helicsId; //!< Helics ID
};

} // namespace ns3

#endif /* HELICS_ID_TAG_H */
