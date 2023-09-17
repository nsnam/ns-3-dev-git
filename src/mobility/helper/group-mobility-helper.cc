/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2021 University of Washington: Group mobility changes
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
 * Adapted from 'mobility-helper.cc' for group mobility by Tom Henderson
 */

#include "group-mobility-helper.h"

#include "ns3/config.h"
#include "ns3/hierarchical-mobility-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/pointer.h"
#include "ns3/position-allocator.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GroupMobilityHelper");

GroupMobilityHelper::GroupMobilityHelper()
    : NS_LOG_TEMPLATE_DEFINE("GroupMobilityHelper")
{
}

GroupMobilityHelper::~GroupMobilityHelper()
{
}

void
GroupMobilityHelper::SetReferencePositionAllocator(Ptr<PositionAllocator> allocator)
{
    m_referencePosition = allocator;
}

void
GroupMobilityHelper::SetMemberPositionAllocator(Ptr<PositionAllocator> allocator)
{
    m_memberPosition = allocator;
}

void
GroupMobilityHelper::SetReferenceMobilityModel(Ptr<MobilityModel> mobility)
{
    m_referenceMobility = mobility;
}

void
GroupMobilityHelper::Install(Ptr<Node> node)
{
    NS_ABORT_MSG_IF(node->GetObject<MobilityModel>(), "Mobility model already installed");
    NS_ABORT_MSG_IF(!m_referenceMobility, "Reference mobility model is empty");
    NS_ABORT_MSG_UNLESS(m_memberMobilityFactory.IsTypeIdSet(), "Member mobility factory is unset");
    if (m_referencePosition && !m_referencePositionSet)
    {
        Vector referencePosition = m_referencePosition->GetNext();
        m_referenceMobility->SetPosition(referencePosition);
        m_referencePositionSet = true;
    }
    Ptr<HierarchicalMobilityModel> hierarchical = CreateObject<HierarchicalMobilityModel>();
    hierarchical->SetParent(m_referenceMobility);
    Ptr<MobilityModel> child = m_memberMobilityFactory.Create()->GetObject<MobilityModel>();
    NS_ABORT_MSG_IF(!child, "Member mobility factory did not produce a MobilityModel");
    if (m_memberPosition)
    {
        Vector position = m_memberPosition->GetNext();
        child->SetPosition(position);
    }
    hierarchical->SetChild(child);
    NS_LOG_DEBUG("node=" << node << ", mob=" << hierarchical);
    node->AggregateObject(hierarchical);
}

void
GroupMobilityHelper::Install(std::string nodeName)
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    Install(node);
}

void
GroupMobilityHelper::Install(NodeContainer c)
{
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        Install(*i);
    }
}

int64_t
GroupMobilityHelper::AssignStreams(NodeContainer c, int64_t stream)
{
    int64_t currentStream = stream;
    Ptr<Node> node;
    bool firstNode = true;
    Ptr<HierarchicalMobilityModel> mobility;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        node = (*i);
        mobility = node->GetObject<HierarchicalMobilityModel>();
        if (!mobility)
        {
            NS_FATAL_ERROR("Did not find a HierarchicalMobilityModel");
        }
        if (firstNode)
        {
            // Assign streams only once for the reference node
            currentStream += mobility->GetParent()->AssignStreams(currentStream);
            firstNode = false;
        }
        currentStream += mobility->GetChild()->AssignStreams(currentStream);
    }
    return (currentStream - stream);
}

} // namespace ns3
