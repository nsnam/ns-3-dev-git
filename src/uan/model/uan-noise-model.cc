/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#include "uan-noise-model.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(UanNoiseModel);

TypeId
UanNoiseModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UanNoiseModel").SetParent<Object>().SetGroupName("Uan");
    return tid;
}

void
UanNoiseModel::Clear()
{
}

void
UanNoiseModel::DoDispose()
{
    Clear();
    Object::DoDispose();
}

} // namespace ns3
