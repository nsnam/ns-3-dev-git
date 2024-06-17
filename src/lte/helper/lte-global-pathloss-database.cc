/*
 * Copyright (c) 2011,2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "lte-global-pathloss-database.h"

#include "ns3/lte-enb-net-device.h"
#include "ns3/lte-spectrum-phy.h"
#include "ns3/lte-ue-net-device.h"

#include <limits>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteGlobalPathlossDatabase");

LteGlobalPathlossDatabase::~LteGlobalPathlossDatabase()
{
}

void
LteGlobalPathlossDatabase::Print()
{
    NS_LOG_FUNCTION(this);
    for (auto cellIdIt = m_pathlossMap.begin(); cellIdIt != m_pathlossMap.end(); ++cellIdIt)
    {
        for (auto imsiIt = cellIdIt->second.begin(); imsiIt != cellIdIt->second.end(); ++imsiIt)
        {
            std::cout << "CellId: " << cellIdIt->first << " IMSI: " << imsiIt->first
                      << " pathloss: " << imsiIt->second << " dB" << std::endl;
        }
    }
}

double
LteGlobalPathlossDatabase::GetPathloss(uint16_t cellId, uint64_t imsi)
{
    NS_LOG_FUNCTION(this);
    auto cellIt = m_pathlossMap.find(cellId);
    if (cellIt == m_pathlossMap.end())
    {
        return std::numeric_limits<double>::infinity();
    }
    auto ueIt = cellIt->second.find(imsi);
    if (ueIt == cellIt->second.end())
    {
        return std::numeric_limits<double>::infinity();
    }
    return ueIt->second;
}

void
DownlinkLteGlobalPathlossDatabase::UpdatePathloss(std::string context,
                                                  Ptr<const SpectrumPhy> txPhy,
                                                  Ptr<const SpectrumPhy> rxPhy,
                                                  double lossDb)
{
    NS_LOG_FUNCTION(this << lossDb);
    uint16_t cellId = txPhy->GetDevice()->GetObject<LteEnbNetDevice>()->GetCellId();
    uint16_t imsi = rxPhy->GetDevice()->GetObject<LteUeNetDevice>()->GetImsi();
    m_pathlossMap[cellId][imsi] = lossDb;
}

void
UplinkLteGlobalPathlossDatabase::UpdatePathloss(std::string context,
                                                Ptr<const SpectrumPhy> txPhy,
                                                Ptr<const SpectrumPhy> rxPhy,
                                                double lossDb)
{
    NS_LOG_FUNCTION(this << lossDb);
    uint16_t imsi = txPhy->GetDevice()->GetObject<LteUeNetDevice>()->GetImsi();
    uint16_t cellId = rxPhy->GetDevice()->GetObject<LteEnbNetDevice>()->GetCellId();
    m_pathlossMap[cellId][imsi] = lossDb;
}

} // namespace ns3
