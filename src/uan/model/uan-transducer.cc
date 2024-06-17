/*
 * Copyright (c) 2011 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mitch Watrous <watrous@u.washington.edu>
 */

#include "uan-transducer.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(UanTransducer);

TypeId
UanTransducer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UanTransducer").SetParent<Object>().SetGroupName("Uan");
    return tid;
}

} // namespace ns3
