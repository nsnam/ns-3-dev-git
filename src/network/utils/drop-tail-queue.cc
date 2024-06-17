/*
 * Copyright (c) 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "drop-tail-queue.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DropTailQueue");

NS_OBJECT_TEMPLATE_CLASS_DEFINE(DropTailQueue, Packet);
NS_OBJECT_TEMPLATE_CLASS_DEFINE(DropTailQueue, QueueDiscItem);

} // namespace ns3
