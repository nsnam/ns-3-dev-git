/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
 *               2007 INRIA
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
 * Authors: Matthieu Coudron <matthieu.coudron@lip6.fr>
 *
 */

#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "ns3/socket.h"
#include "ns3/traced-callback.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/nstime.h"

namespace ns3 {

class Node;
class Packet;

/**
 * \brief Names of the  MPTCP states
 */
typedef enum {
  M_CLOSED,       // 0
  M_LISTEN,       // 1
  M_SYN_SENT,     // 2
  M_SYN_RCVD,     // 3
  M_ESTA_WAIT,  // 4
  M_ESTA_SP,  // 4
  M_ESTA_MP,
  M_CLOSE_WAIT,   // 5
  M_LAST_ACK,     // 6
  M_FIN_WAIT_1,   // 7
  M_FIN_WAIT_2,   // 8
  M_CLOSING,      // 9
  TIME_WAIT,   // 10
  LAST_STATE
} MpTcpStates_t;

} // end of namespace
