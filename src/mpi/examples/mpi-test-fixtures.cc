/*
 *  Copyright 2018. Lawrence Livermore National Security, LLC.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Steven Smith <smith84@llnl.gov>
 */

#include "mpi-test-fixtures.h"

#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"

#include <mpi.h>

namespace ns3
{

unsigned long SinkTracer::m_sinkCount = 0;
unsigned long SinkTracer::m_line = 0;
int SinkTracer::m_worldRank = -1;
int SinkTracer::m_worldSize = -1;

void
SinkTracer::Init()
{
    m_sinkCount = 0;
    m_line = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &m_worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &m_worldSize);
}

void
SinkTracer::SinkTrace(const ns3::Ptr<const ns3::Packet> packet,
                      const ns3::Address& srcAddress,
                      const ns3::Address& destAddress)
{
    m_sinkCount++;
}

void
SinkTracer::Verify(unsigned long expectedCount)
{
    unsigned long globalCount;

#ifdef NS3_MPI
    MPI_Reduce(&m_sinkCount, &globalCount, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
#else
    globalCount = m_sinkCount;
#endif

    if (expectedCount == globalCount)
    {
        RANK0COUT("PASSED\n");
    }
    else
    {
        RANK0COUT("FAILED  Observed sink traces (" << globalCount << ") not equal to expected ("
                                                   << expectedCount << ")\n");
    }
}

std::string
SinkTracer::FormatAddress(const ns3::Address address)
{
    std::stringstream ss;

    if (InetSocketAddress::IsMatchingType(address))
    {
        ss << InetSocketAddress::ConvertFrom(address).GetIpv4() << ":"
           << InetSocketAddress::ConvertFrom(address).GetPort();
    }
    else if (Inet6SocketAddress::IsMatchingType(address))
    {
        ss << Inet6SocketAddress::ConvertFrom(address).GetIpv6() << ":"
           << Inet6SocketAddress::ConvertFrom(address).GetPort();
    }
    return ss.str();
}

} // namespace ns3
