/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Université Pierre et Marie Curie (UPMC)
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
 * Author: Matthieu Coudron <matthieu.coudron@lip6.fr>
 */
#ifndef MPTCP_SCHEDULER_H
#define MPTCP_SCHEDULER_H

#include <stdint.h>
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/sequence-number.h"

namespace ns3
{

class MpTcpSocketBase;

/**
 * This class is responsible for
 *
 * The mptcp linux 0.90 scheduler is composed of mainly 2 functions:
 * -get_available_subflow
 * -get_next_segment
 *
 * Here we wanted to let a maximum number of possibilities for testing, for instance:
 * - you can generate DSS for data that is not in the meta Tx buffer yet (in order to minimize the number of DSS sent).
 * - you can send the
 *
 * The scheduler maps a dsn range to a subflow. It does not map the dsn to an ssn: this is done
 * by the subflow when it actually receives the Tx data.
 *
 * \warn The decoupling between dsn & ssn mapping may prove hard to debug. There are
 * some checks but you should be especially careful when writing a new scheduler.
 */
class MpTcpScheduler : public Object
{

public:

  virtual ~MpTcpScheduler() {}

  /**
   * \param activeSubflowArrayId
   * \param uint16_t
   * \return true if could generate a mapping
   * \see MpTcpSocketBase::SendPendingData
   */
  virtual bool GenerateMapping(
        int& activeSubflowArrayId, SequenceNumber64& dsn, uint16_t& length
                              ) = 0;

  virtual void SetMeta(Ptr<MpTcpSocketBase> metaSock) = 0;
};

}

#endif /* MPTCP_SCHEDULER_H */
