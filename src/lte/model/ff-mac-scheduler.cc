/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "ff-mac-scheduler.h"

#include "ns3/enum.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FfMacScheduler");

NS_OBJECT_ENSURE_REGISTERED(FfMacScheduler);

FfMacScheduler::FfMacScheduler()
    : m_ulCqiFilter(SRS_UL_CQI)
{
    NS_LOG_FUNCTION(this);
}

FfMacScheduler::~FfMacScheduler()
{
    NS_LOG_FUNCTION(this);
}

void
FfMacScheduler::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

TypeId
FfMacScheduler::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::FfMacScheduler")
            .SetParent<Object>()
            .SetGroupName("Lte")
            .AddAttribute("UlCqiFilter",
                          "The filter to apply on UL CQIs received",
                          EnumValue(FfMacScheduler::SRS_UL_CQI),
                          MakeEnumAccessor<UlCqiFilter_t>(&FfMacScheduler::m_ulCqiFilter),
                          MakeEnumChecker(FfMacScheduler::SRS_UL_CQI,
                                          "SRS_UL_CQI",
                                          FfMacScheduler::PUSCH_UL_CQI,
                                          "PUSCH_UL_CQI"));
    return tid;
}

} // namespace ns3
