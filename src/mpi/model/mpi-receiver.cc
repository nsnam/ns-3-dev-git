/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: George Riley <riley@ece.gatech.edu>
 */

/**
 * @file
 * @ingroup mpi
 * ns3::MpiReceiver implementation,
 * provides an interface to aggregate to MPI-compatible NetDevices.
 */

#include "mpi-receiver.h"

namespace ns3
{

TypeId
MpiReceiver::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MpiReceiver")
                            .SetParent<Object>()
                            .SetGroupName("Mpi")
                            .AddConstructor<MpiReceiver>();
    return tid;
}

MpiReceiver::~MpiReceiver()
{
}

void
MpiReceiver::SetReceiveCallback(Callback<void, Ptr<Packet>> callback)
{
    m_rxCallback = callback;
}

void
MpiReceiver::Receive(Ptr<Packet> p)
{
    NS_ASSERT(!m_rxCallback.IsNull());
    m_rxCallback(p);
}

void
MpiReceiver::DoDispose()
{
    m_rxCallback = MakeNullCallback<void, Ptr<Packet>>();
}

} // namespace ns3
