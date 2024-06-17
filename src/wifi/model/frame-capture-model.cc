/*
 * Copyright (c) 2017
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "frame-capture-model.h"

#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(FrameCaptureModel);

TypeId
FrameCaptureModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::FrameCaptureModel")
                            .SetParent<Object>()
                            .SetGroupName("Wifi")
                            .AddAttribute("CaptureWindow",
                                          "The duration of the capture window.",
                                          TimeValue(MicroSeconds(16)),
                                          MakeTimeAccessor(&FrameCaptureModel::m_captureWindow),
                                          MakeTimeChecker());
    return tid;
}

bool
FrameCaptureModel::IsInCaptureWindow(Time timePreambleDetected) const
{
    return (timePreambleDetected + m_captureWindow >= Simulator::Now());
}

} // namespace ns3
