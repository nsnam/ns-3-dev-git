/*
 * Copyright (c) 2018 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "preamble-detection-model.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(PreambleDetectionModel);

TypeId
PreambleDetectionModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PreambleDetectionModel").SetParent<Object>().SetGroupName("Wifi");
    return tid;
}

} // namespace ns3
