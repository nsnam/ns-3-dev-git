/*
 * Copyright (c) 2014 Universita' degli Studi di Napoli "Federico II"
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stefano.avallone@unina.it>
 */

#include "wifi-tx-current-model.h"

#include "wifi-utils.h"

#include "ns3/double.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiTxCurrentModel");

NS_OBJECT_ENSURE_REGISTERED(WifiTxCurrentModel);

TypeId
WifiTxCurrentModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiTxCurrentModel").SetParent<Object>().SetGroupName("Wifi");
    return tid;
}

WifiTxCurrentModel::WifiTxCurrentModel()
{
}

WifiTxCurrentModel::~WifiTxCurrentModel()
{
}

NS_OBJECT_ENSURE_REGISTERED(LinearWifiTxCurrentModel);

TypeId
LinearWifiTxCurrentModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LinearWifiTxCurrentModel")
            .SetParent<WifiTxCurrentModel>()
            .SetGroupName("Wifi")
            .AddConstructor<LinearWifiTxCurrentModel>()
            .AddAttribute("Eta",
                          "The efficiency of the power amplifier.",
                          DoubleValue(0.10),
                          MakeDoubleAccessor(&LinearWifiTxCurrentModel::m_eta),
                          MakeDoubleChecker<double>())
            .AddAttribute("Voltage",
                          "The supply voltage (in Volts).",
                          DoubleValue(3.0),
                          MakeDoubleAccessor(&LinearWifiTxCurrentModel::m_voltage),
                          MakeDoubleChecker<volt_u>())
            .AddAttribute("IdleCurrent",
                          "The current in the IDLE state (in Ampere).",
                          DoubleValue(0.273333),
                          MakeDoubleAccessor(&LinearWifiTxCurrentModel::m_idleCurrent),
                          MakeDoubleChecker<ampere_u>());
    return tid;
}

LinearWifiTxCurrentModel::LinearWifiTxCurrentModel()
{
    NS_LOG_FUNCTION(this);
}

LinearWifiTxCurrentModel::~LinearWifiTxCurrentModel()
{
    NS_LOG_FUNCTION(this);
}

ampere_u
LinearWifiTxCurrentModel::CalcTxCurrent(dBm_u txPower) const
{
    NS_LOG_FUNCTION(this << txPower);
    return DbmToW(txPower) / (m_voltage * m_eta) + m_idleCurrent;
}

} // namespace ns3
