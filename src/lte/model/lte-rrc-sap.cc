/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "lte-rrc-sap.h"

namespace ns3
{

LteRrcSap::~LteRrcSap()
{
}

LteRrcSap::ReportConfigEutra::ReportConfigEutra()
{
    triggerType = EVENT;
    eventId = EVENT_A1;
    threshold1.choice = ThresholdEutra::THRESHOLD_RSRP;
    threshold1.range = 0;
    threshold2.choice = ThresholdEutra::THRESHOLD_RSRP;
    threshold2.range = 0;
    reportOnLeave = false;
    a3Offset = 0;
    hysteresis = 0;
    timeToTrigger = 0;
    purpose = REPORT_STRONGEST_CELLS;
    triggerQuantity = RSRP;
    reportQuantity = BOTH;
    maxReportCells = MaxReportCells;
    reportInterval = MS480;
    reportAmount = 255;
}

} // namespace ns3
