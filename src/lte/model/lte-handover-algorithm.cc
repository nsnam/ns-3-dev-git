/*
 * Copyright (c) 2013 Budiarto Herman
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#include "lte-handover-algorithm.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteHandoverAlgorithm");

NS_OBJECT_ENSURE_REGISTERED(LteHandoverAlgorithm);

LteHandoverAlgorithm::LteHandoverAlgorithm()
{
}

LteHandoverAlgorithm::~LteHandoverAlgorithm()
{
}

TypeId
LteHandoverAlgorithm::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LteHandoverAlgorithm").SetParent<Object>().SetGroupName("Lte");
    return tid;
}

void
LteHandoverAlgorithm::DoDispose()
{
}

} // end of namespace ns3
