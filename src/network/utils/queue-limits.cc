/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stefano.avallone@unina.it>
 */

#include "queue-limits.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QueueLimits");

NS_OBJECT_ENSURE_REGISTERED(QueueLimits);

TypeId
QueueLimits::GetTypeId()
{
    static TypeId tid = TypeId("ns3::QueueLimits").SetParent<Object>().SetGroupName("Network");
    return tid;
}

QueueLimits::~QueueLimits()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
