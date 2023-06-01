/*
 * Copyright (c) 2004 Francisco J. Ros
 * Copyright (c) 2007 INESC Porto
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
 * Authors: Francisco J. Ros  <fjrm@dif.um.es>
 *          Gustavo J. A. M. Carneiro <gjc@inescporto.pt>
 */

///
/// \file olsr-state.cc
/// \brief Implementation of all functions needed for manipulating the internal
///        state of an OLSR node.
///

#include "olsr-state.h"

namespace ns3
{
namespace olsr
{

/********** MPR Selector Set Manipulation **********/

MprSelectorTuple*
OlsrState::FindMprSelectorTuple(const Ipv4Address& mainAddr)
{
    for (MprSelectorSet::iterator it = m_mprSelectorSet.begin(); it != m_mprSelectorSet.end(); it++)
    {
        if (it->mainAddr == mainAddr)
        {
            return &(*it);
        }
    }
    return nullptr;
}

void
OlsrState::EraseMprSelectorTuple(const MprSelectorTuple& tuple)
{
    for (MprSelectorSet::iterator it = m_mprSelectorSet.begin(); it != m_mprSelectorSet.end(); it++)
    {
        if (*it == tuple)
        {
            m_mprSelectorSet.erase(it);
            break;
        }
    }
}

void
OlsrState::EraseMprSelectorTuples(const Ipv4Address& mainAddr)
{
    for (MprSelectorSet::iterator it = m_mprSelectorSet.begin(); it != m_mprSelectorSet.end();)
    {
        if (it->mainAddr == mainAddr)
        {
            it = m_mprSelectorSet.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void
OlsrState::InsertMprSelectorTuple(const MprSelectorTuple& tuple)
{
    m_mprSelectorSet.push_back(tuple);
}

std::string
OlsrState::PrintMprSelectorSet() const
{
    std::ostringstream os;
    os << "[";
    for (MprSelectorSet::const_iterator iter = m_mprSelectorSet.begin();
         iter != m_mprSelectorSet.end();
         iter++)
    {
        MprSelectorSet::const_iterator next = iter;
        next++;
        os << iter->mainAddr;
        if (next != m_mprSelectorSet.end())
        {
            os << ", ";
        }
    }
    os << "]";
    return os.str();
}

/********** Neighbor Set Manipulation **********/

NeighborTuple*
OlsrState::FindNeighborTuple(const Ipv4Address& mainAddr)
{
    for (NeighborSet::iterator it = m_neighborSet.begin(); it != m_neighborSet.end(); it++)
    {
        if (it->neighborMainAddr == mainAddr)
        {
            return &(*it);
        }
    }
    return nullptr;
}

const NeighborTuple*
OlsrState::FindSymNeighborTuple(const Ipv4Address& mainAddr) const
{
    for (NeighborSet::const_iterator it = m_neighborSet.begin(); it != m_neighborSet.end(); it++)
    {
        if (it->neighborMainAddr == mainAddr && it->status == NeighborTuple::STATUS_SYM)
        {
            return &(*it);
        }
    }
    return nullptr;
}

NeighborTuple*
OlsrState::FindNeighborTuple(const Ipv4Address& mainAddr, Willingness willingness)
{
    for (NeighborSet::iterator it = m_neighborSet.begin(); it != m_neighborSet.end(); it++)
    {
        if (it->neighborMainAddr == mainAddr && it->willingness == willingness)
        {
            return &(*it);
        }
    }
    return nullptr;
}

void
OlsrState::EraseNeighborTuple(const NeighborTuple& tuple)
{
    for (NeighborSet::iterator it = m_neighborSet.begin(); it != m_neighborSet.end(); it++)
    {
        if (*it == tuple)
        {
            m_neighborSet.erase(it);
            break;
        }
    }
}

void
OlsrState::EraseNeighborTuple(const Ipv4Address& mainAddr)
{
    for (NeighborSet::iterator it = m_neighborSet.begin(); it != m_neighborSet.end(); it++)
    {
        if (it->neighborMainAddr == mainAddr)
        {
            it = m_neighborSet.erase(it);
            break;
        }
    }
}

void
OlsrState::InsertNeighborTuple(const NeighborTuple& tuple)
{
    for (NeighborSet::iterator it = m_neighborSet.begin(); it != m_neighborSet.end(); it++)
    {
        if (it->neighborMainAddr == tuple.neighborMainAddr)
        {
            // Update it
            *it = tuple;
            return;
        }
    }
    m_neighborSet.push_back(tuple);
}

/********** Neighbor 2 Hop Set Manipulation **********/

TwoHopNeighborTuple*
OlsrState::FindTwoHopNeighborTuple(const Ipv4Address& neighborMainAddr,
                                   const Ipv4Address& twoHopNeighborAddr)
{
    for (TwoHopNeighborSet::iterator it = m_twoHopNeighborSet.begin();
         it != m_twoHopNeighborSet.end();
         it++)
    {
        if (it->neighborMainAddr == neighborMainAddr &&
            it->twoHopNeighborAddr == twoHopNeighborAddr)
        {
            return &(*it);
        }
    }
    return nullptr;
}

void
OlsrState::EraseTwoHopNeighborTuple(const TwoHopNeighborTuple& tuple)
{
    for (TwoHopNeighborSet::iterator it = m_twoHopNeighborSet.begin();
         it != m_twoHopNeighborSet.end();
         it++)
    {
        if (*it == tuple)
        {
            m_twoHopNeighborSet.erase(it);
            break;
        }
    }
}

void
OlsrState::EraseTwoHopNeighborTuples(const Ipv4Address& neighborMainAddr,
                                     const Ipv4Address& twoHopNeighborAddr)
{
    for (TwoHopNeighborSet::iterator it = m_twoHopNeighborSet.begin();
         it != m_twoHopNeighborSet.end();)
    {
        if (it->neighborMainAddr == neighborMainAddr &&
            it->twoHopNeighborAddr == twoHopNeighborAddr)
        {
            it = m_twoHopNeighborSet.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void
OlsrState::EraseTwoHopNeighborTuples(const Ipv4Address& neighborMainAddr)
{
    for (TwoHopNeighborSet::iterator it = m_twoHopNeighborSet.begin();
         it != m_twoHopNeighborSet.end();)
    {
        if (it->neighborMainAddr == neighborMainAddr)
        {
            it = m_twoHopNeighborSet.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void
OlsrState::InsertTwoHopNeighborTuple(const TwoHopNeighborTuple& tuple)
{
    m_twoHopNeighborSet.push_back(tuple);
}

/********** MPR Set Manipulation **********/

bool
OlsrState::FindMprAddress(const Ipv4Address& addr)
{
    MprSet::iterator it = m_mprSet.find(addr);
    return (it != m_mprSet.end());
}

void
OlsrState::SetMprSet(MprSet mprSet)
{
    m_mprSet = mprSet;
}

MprSet
OlsrState::GetMprSet() const
{
    return m_mprSet;
}

/********** Duplicate Set Manipulation **********/

DuplicateTuple*
OlsrState::FindDuplicateTuple(const Ipv4Address& addr, uint16_t sequenceNumber)
{
    for (DuplicateSet::iterator it = m_duplicateSet.begin(); it != m_duplicateSet.end(); it++)
    {
        if (it->address == addr && it->sequenceNumber == sequenceNumber)
        {
            return &(*it);
        }
    }
    return nullptr;
}

void
OlsrState::EraseDuplicateTuple(const DuplicateTuple& tuple)
{
    for (DuplicateSet::iterator it = m_duplicateSet.begin(); it != m_duplicateSet.end(); it++)
    {
        if (*it == tuple)
        {
            m_duplicateSet.erase(it);
            break;
        }
    }
}

void
OlsrState::InsertDuplicateTuple(const DuplicateTuple& tuple)
{
    m_duplicateSet.push_back(tuple);
}

/********** Link Set Manipulation **********/

LinkTuple*
OlsrState::FindLinkTuple(const Ipv4Address& ifaceAddr)
{
    for (LinkSet::iterator it = m_linkSet.begin(); it != m_linkSet.end(); it++)
    {
        if (it->neighborIfaceAddr == ifaceAddr)
        {
            return &(*it);
        }
    }
    return nullptr;
}

LinkTuple*
OlsrState::FindSymLinkTuple(const Ipv4Address& ifaceAddr, Time now)
{
    for (LinkSet::iterator it = m_linkSet.begin(); it != m_linkSet.end(); it++)
    {
        if (it->neighborIfaceAddr == ifaceAddr)
        {
            if (it->symTime > now)
            {
                return &(*it);
            }
            else
            {
                break;
            }
        }
    }
    return nullptr;
}

void
OlsrState::EraseLinkTuple(const LinkTuple& tuple)
{
    for (LinkSet::iterator it = m_linkSet.begin(); it != m_linkSet.end(); it++)
    {
        if (*it == tuple)
        {
            m_linkSet.erase(it);
            break;
        }
    }
}

LinkTuple&
OlsrState::InsertLinkTuple(const LinkTuple& tuple)
{
    m_linkSet.push_back(tuple);
    return m_linkSet.back();
}

/********** Topology Set Manipulation **********/

TopologyTuple*
OlsrState::FindTopologyTuple(const Ipv4Address& destAddr, const Ipv4Address& lastAddr)
{
    for (TopologySet::iterator it = m_topologySet.begin(); it != m_topologySet.end(); it++)
    {
        if (it->destAddr == destAddr && it->lastAddr == lastAddr)
        {
            return &(*it);
        }
    }
    return nullptr;
}

TopologyTuple*
OlsrState::FindNewerTopologyTuple(const Ipv4Address& lastAddr, uint16_t ansn)
{
    for (TopologySet::iterator it = m_topologySet.begin(); it != m_topologySet.end(); it++)
    {
        if (it->lastAddr == lastAddr && it->sequenceNumber > ansn)
        {
            return &(*it);
        }
    }
    return nullptr;
}

void
OlsrState::EraseTopologyTuple(const TopologyTuple& tuple)
{
    for (TopologySet::iterator it = m_topologySet.begin(); it != m_topologySet.end(); it++)
    {
        if (*it == tuple)
        {
            m_topologySet.erase(it);
            break;
        }
    }
}

void
OlsrState::EraseOlderTopologyTuples(const Ipv4Address& lastAddr, uint16_t ansn)
{
    for (TopologySet::iterator it = m_topologySet.begin(); it != m_topologySet.end();)
    {
        if (it->lastAddr == lastAddr && it->sequenceNumber < ansn)
        {
            it = m_topologySet.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void
OlsrState::InsertTopologyTuple(const TopologyTuple& tuple)
{
    m_topologySet.push_back(tuple);
}

/********** Interface Association Set Manipulation **********/

IfaceAssocTuple*
OlsrState::FindIfaceAssocTuple(const Ipv4Address& ifaceAddr)
{
    for (IfaceAssocSet::iterator it = m_ifaceAssocSet.begin(); it != m_ifaceAssocSet.end(); it++)
    {
        if (it->ifaceAddr == ifaceAddr)
        {
            return &(*it);
        }
    }
    return nullptr;
}

const IfaceAssocTuple*
OlsrState::FindIfaceAssocTuple(const Ipv4Address& ifaceAddr) const
{
    for (IfaceAssocSet::const_iterator it = m_ifaceAssocSet.begin(); it != m_ifaceAssocSet.end();
         it++)
    {
        if (it->ifaceAddr == ifaceAddr)
        {
            return &(*it);
        }
    }
    return nullptr;
}

void
OlsrState::EraseIfaceAssocTuple(const IfaceAssocTuple& tuple)
{
    for (IfaceAssocSet::iterator it = m_ifaceAssocSet.begin(); it != m_ifaceAssocSet.end(); it++)
    {
        if (*it == tuple)
        {
            m_ifaceAssocSet.erase(it);
            break;
        }
    }
}

void
OlsrState::InsertIfaceAssocTuple(const IfaceAssocTuple& tuple)
{
    m_ifaceAssocSet.push_back(tuple);
}

std::vector<Ipv4Address>
OlsrState::FindNeighborInterfaces(const Ipv4Address& neighborMainAddr) const
{
    std::vector<Ipv4Address> retval;
    for (IfaceAssocSet::const_iterator it = m_ifaceAssocSet.begin(); it != m_ifaceAssocSet.end();
         it++)
    {
        if (it->mainAddr == neighborMainAddr)
        {
            retval.push_back(it->ifaceAddr);
        }
    }
    return retval;
}

/********** Host-Network Association Set Manipulation **********/

AssociationTuple*
OlsrState::FindAssociationTuple(const Ipv4Address& gatewayAddr,
                                const Ipv4Address& networkAddr,
                                const Ipv4Mask& netmask)
{
    for (AssociationSet::iterator it = m_associationSet.begin(); it != m_associationSet.end(); it++)
    {
        if (it->gatewayAddr == gatewayAddr and it->networkAddr == networkAddr and
            it->netmask == netmask)
        {
            return &(*it);
        }
    }
    return nullptr;
}

void
OlsrState::EraseAssociationTuple(const AssociationTuple& tuple)
{
    for (AssociationSet::iterator it = m_associationSet.begin(); it != m_associationSet.end(); it++)
    {
        if (*it == tuple)
        {
            m_associationSet.erase(it);
            break;
        }
    }
}

void
OlsrState::InsertAssociationTuple(const AssociationTuple& tuple)
{
    m_associationSet.push_back(tuple);
}

void
OlsrState::EraseAssociation(const Association& tuple)
{
    for (Associations::iterator it = m_associations.begin(); it != m_associations.end(); it++)
    {
        if (*it == tuple)
        {
            m_associations.erase(it);
            break;
        }
    }
}

void
OlsrState::InsertAssociation(const Association& tuple)
{
    m_associations.push_back(tuple);
}

} // namespace olsr
} // namespace ns3
