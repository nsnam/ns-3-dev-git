/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>
 */

#include "lte-vendor-specific-parameters.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteVendorSpecificParameters");

SrsCqiRntiVsp::SrsCqiRntiVsp(uint16_t rnti)
    : m_rnti(rnti)
{
}

SrsCqiRntiVsp::~SrsCqiRntiVsp()
{
}

uint16_t
SrsCqiRntiVsp::GetRnti() const
{
    return m_rnti;
}

} // namespace ns3
