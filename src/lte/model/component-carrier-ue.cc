/*
 * Copyright (c) 2015 Danilo Abrignani
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it>
 */

#include "component-carrier-ue.h"

#include "lte-ue-mac.h"
#include "lte-ue-phy.h"

#include "ns3/abort.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ComponentCarrierUe");

NS_OBJECT_ENSURE_REGISTERED(ComponentCarrierUe);

TypeId
ComponentCarrierUe::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ComponentCarrierUe")
                            .SetParent<ComponentCarrier>()
                            .AddConstructor<ComponentCarrierUe>()
                            .AddAttribute("LteUePhy",
                                          "The PHY associated to this EnbNetDevice",
                                          PointerValue(),
                                          MakePointerAccessor(&ComponentCarrierUe::m_phy),
                                          MakePointerChecker<LteUePhy>())
                            .AddAttribute("LteUeMac",
                                          "The MAC associated to this UeNetDevice",
                                          PointerValue(),
                                          MakePointerAccessor(&ComponentCarrierUe::m_mac),
                                          MakePointerChecker<LteUeMac>());
    return tid;
}

ComponentCarrierUe::ComponentCarrierUe()
{
    NS_LOG_FUNCTION(this);
}

ComponentCarrierUe::~ComponentCarrierUe()
{
    NS_LOG_FUNCTION(this);
}

void
ComponentCarrierUe::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_phy->Dispose();
    m_phy = nullptr;
    m_mac->Dispose();
    m_mac = nullptr;
    Object::DoDispose();
}

void
ComponentCarrierUe::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    m_phy->Initialize();
    m_mac->Initialize();
}

void
ComponentCarrierUe::SetPhy(Ptr<LteUePhy> s)
{
    NS_LOG_FUNCTION(this);
    m_phy = s;
}

Ptr<LteUePhy>
ComponentCarrierUe::GetPhy() const
{
    NS_LOG_FUNCTION(this);
    return m_phy;
}

void
ComponentCarrierUe::SetMac(Ptr<LteUeMac> s)
{
    NS_LOG_FUNCTION(this);
    m_mac = s;
}

Ptr<LteUeMac>
ComponentCarrierUe::GetMac() const
{
    NS_LOG_FUNCTION(this);
    return m_mac;
}

} // namespace ns3
