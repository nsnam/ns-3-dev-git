/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Universita' di Napoli
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "block-ack-type.h"
#include "ns3/fatal-error.h"

namespace ns3 {

std::ostream &operator << (std::ostream &os, const BlockAckType &type)
{
  switch (type)
    {
    case BlockAckType::BASIC_BLOCK_ACK:
      os << "basic-block-ack";
      break;
    case BlockAckType::COMPRESSED_BLOCK_ACK:
      os << "compressed-block-ack";
      break;
    case BlockAckType::EXTENDED_COMPRESSED_BLOCK_ACK:
      os << "extended-compressed-block-ack";
      break;
    case BlockAckType::MULTI_TID_BLOCK_ACK:
      os << "multi-tid-block-ack";
      break;
    default:
      NS_FATAL_ERROR ("Unknown block ack type");
    }
  return os;
}

} //namespace ns3
