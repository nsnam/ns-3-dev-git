/*
 * Copyright (c) 2011-2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *   Jaume Nin <jnin@cttc.es>
 *   Nicola Baldo <nbaldo@cttc.es>
 *   Manuel Requena <manuel.requena@cttc.es>
 */

#include "epc-helper.h"

#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/log.h"
#include "ns3/node.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EpcHelper");

NS_OBJECT_ENSURE_REGISTERED(EpcHelper);

EpcHelper::EpcHelper()
{
    NS_LOG_FUNCTION(this);
}

EpcHelper::~EpcHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId
EpcHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::EpcHelper").SetParent<Object>().SetGroupName("Lte");
    return tid;
}

void
EpcHelper::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Object::DoDispose();
}

} // namespace ns3
