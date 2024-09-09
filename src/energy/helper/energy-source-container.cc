/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "energy-source-container.h"

#include "ns3/names.h"

namespace ns3
{
namespace energy
{

NS_OBJECT_ENSURE_REGISTERED(EnergySourceContainer);

TypeId
EnergySourceContainer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::energy::EnergySourceContainer")
                            .AddDeprecatedName("ns3::EnergySourceContainer")
                            .SetParent<Object>()
                            .SetGroupName("Energy")
                            .AddConstructor<EnergySourceContainer>();
    return tid;
}

EnergySourceContainer::EnergySourceContainer()
{
}

EnergySourceContainer::~EnergySourceContainer()
{
}

EnergySourceContainer::EnergySourceContainer(Ptr<EnergySource> source)
{
    NS_ASSERT(source);
    m_sources.push_back(source);
}

EnergySourceContainer::EnergySourceContainer(std::string sourceName)
{
    Ptr<EnergySource> source = Names::Find<EnergySource>(sourceName);
    NS_ASSERT(source);
    m_sources.push_back(source);
}

EnergySourceContainer::EnergySourceContainer(const EnergySourceContainer& a,
                                             const EnergySourceContainer& b)
{
    *this = a;
    Add(b);
}

EnergySourceContainer::Iterator
EnergySourceContainer::Begin() const
{
    return m_sources.begin();
}

EnergySourceContainer::Iterator
EnergySourceContainer::End() const
{
    return m_sources.end();
}

uint32_t
EnergySourceContainer::GetN() const
{
    return m_sources.size();
}

Ptr<EnergySource>
EnergySourceContainer::Get(uint32_t i) const
{
    return m_sources[i];
}

void
EnergySourceContainer::Add(EnergySourceContainer container)
{
    for (auto i = container.Begin(); i != container.End(); i++)
    {
        m_sources.push_back(*i);
    }
}

void
EnergySourceContainer::Add(Ptr<EnergySource> source)
{
    NS_ASSERT(source);
    m_sources.push_back(source);
}

void
EnergySourceContainer::Add(std::string sourceName)
{
    Ptr<EnergySource> source = Names::Find<EnergySource>(sourceName);
    NS_ASSERT(source);
    m_sources.push_back(source);
}

/*
 * Private functions start here.
 */

void
EnergySourceContainer::DoDispose()
{
    // call Object::Dispose for all EnergySource objects
    for (auto i = m_sources.begin(); i != m_sources.end(); i++)
    {
        (*i)->DisposeDeviceModels();
        (*i)->Dispose();
    }
    m_sources.clear();
}

void
EnergySourceContainer::DoInitialize()
{
    // call Object::Start for all EnergySource objects
    for (auto i = m_sources.begin(); i != m_sources.end(); i++)
    {
        (*i)->Initialize();
        (*i)->InitializeDeviceModels();
    }
}

} // namespace energy
} // namespace ns3
