/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jaume Nin <jnin@cttc.es>
 * modified by: Marco Miozzo <mmiozzo@cttc.es>
 *        Convert MacStatsCalculator in PhyTxStatsCalculator
 */

#include "phy-tx-stats-calculator.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PhyTxStatsCalculator");

NS_OBJECT_ENSURE_REGISTERED(PhyTxStatsCalculator);

PhyTxStatsCalculator::PhyTxStatsCalculator()
    : m_dlTxFirstWrite(true),
      m_ulTxFirstWrite(true)
{
    NS_LOG_FUNCTION(this);
}

PhyTxStatsCalculator::~PhyTxStatsCalculator()
{
    NS_LOG_FUNCTION(this);
    if (m_dlTxOutFile.is_open())
    {
        m_dlTxOutFile.close();
    }

    if (m_ulTxOutFile.is_open())
    {
        m_ulTxOutFile.close();
    }
}

TypeId
PhyTxStatsCalculator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PhyTxStatsCalculator")
            .SetParent<LteStatsCalculator>()
            .SetGroupName("Lte")
            .AddConstructor<PhyTxStatsCalculator>()
            .AddAttribute("DlTxOutputFilename",
                          "Name of the file where the downlink results will be saved.",
                          StringValue("DlTxPhyStats.txt"),
                          MakeStringAccessor(&PhyTxStatsCalculator::SetDlTxOutputFilename),
                          MakeStringChecker())
            .AddAttribute("UlTxOutputFilename",
                          "Name of the file where the uplink results will be saved.",
                          StringValue("UlTxPhyStats.txt"),
                          MakeStringAccessor(&PhyTxStatsCalculator::SetUlTxOutputFilename),
                          MakeStringChecker());
    return tid;
}

void
PhyTxStatsCalculator::SetUlTxOutputFilename(std::string outputFilename)
{
    LteStatsCalculator::SetUlOutputFilename(outputFilename);
}

std::string
PhyTxStatsCalculator::GetUlTxOutputFilename()
{
    return LteStatsCalculator::GetUlOutputFilename();
}

void
PhyTxStatsCalculator::SetDlTxOutputFilename(std::string outputFilename)
{
    LteStatsCalculator::SetDlOutputFilename(outputFilename);
}

std::string
PhyTxStatsCalculator::GetDlTxOutputFilename()
{
    return LteStatsCalculator::GetDlOutputFilename();
}

void
PhyTxStatsCalculator::DlPhyTransmission(PhyTransmissionStatParameters params)
{
    NS_LOG_FUNCTION(this << params.m_cellId << params.m_imsi << params.m_timestamp << params.m_rnti
                         << params.m_layer << params.m_mcs << params.m_size << params.m_rv
                         << params.m_ndi);
    NS_LOG_INFO("Write DL Tx Phy Stats in " << GetDlTxOutputFilename());

    if (m_dlTxFirstWrite)
    {
        m_dlTxOutFile.open(GetDlOutputFilename());
        if (!m_dlTxOutFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetDlTxOutputFilename());
            return;
        }
        m_dlTxFirstWrite = false;
        m_dlTxOutFile << "% time\tcellId\tIMSI\tRNTI\tlayer\tmcs\tsize\trv\tndi\tccId";
        m_dlTxOutFile << "\n";
    }

    m_dlTxOutFile << params.m_timestamp << "\t";
    m_dlTxOutFile << (uint32_t)params.m_cellId << "\t";
    m_dlTxOutFile << params.m_imsi << "\t";
    m_dlTxOutFile << params.m_rnti << "\t";
    // m_dlTxOutFile << (uint32_t) params.m_txMode << "\t"; // txMode is not available at dl tx side
    m_dlTxOutFile << (uint32_t)params.m_layer << "\t";
    m_dlTxOutFile << (uint32_t)params.m_mcs << "\t";
    m_dlTxOutFile << params.m_size << "\t";
    m_dlTxOutFile << (uint32_t)params.m_rv << "\t";
    m_dlTxOutFile << (uint32_t)params.m_ndi << "\t";
    m_dlTxOutFile << (uint32_t)params.m_ccId << std::endl;
}

void
PhyTxStatsCalculator::UlPhyTransmission(PhyTransmissionStatParameters params)
{
    NS_LOG_FUNCTION(this << params.m_cellId << params.m_imsi << params.m_timestamp << params.m_rnti
                         << params.m_layer << params.m_mcs << params.m_size << params.m_rv
                         << params.m_ndi);
    NS_LOG_INFO("Write UL Tx Phy Stats in " << GetUlTxOutputFilename());

    if (m_ulTxFirstWrite)
    {
        m_ulTxOutFile.open(GetUlTxOutputFilename());
        if (!m_ulTxOutFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetUlTxOutputFilename());
            return;
        }
        m_ulTxFirstWrite = false;
        // m_ulTxOutFile << "% time\tcellId\tIMSI\tRNTI\ttxMode\tlayer\tmcs\tsize\trv\tndi";
        m_ulTxOutFile << "% time\tcellId\tIMSI\tRNTI\tlayer\tmcs\tsize\trv\tndi\tccId";
        m_ulTxOutFile << "\n";
    }

    m_ulTxOutFile << params.m_timestamp << "\t";
    m_ulTxOutFile << (uint32_t)params.m_cellId << "\t";
    m_ulTxOutFile << params.m_imsi << "\t";
    m_ulTxOutFile << params.m_rnti << "\t";
    // m_ulTxOutFile << (uint32_t) params.m_txMode << "\t";
    m_ulTxOutFile << (uint32_t)params.m_layer << "\t";
    m_ulTxOutFile << (uint32_t)params.m_mcs << "\t";
    m_ulTxOutFile << params.m_size << "\t";
    m_ulTxOutFile << (uint32_t)params.m_rv << "\t";
    m_ulTxOutFile << (uint32_t)params.m_ndi << "\t";
    m_ulTxOutFile << (uint32_t)params.m_ccId << std::endl;
}

void
PhyTxStatsCalculator::DlPhyTransmissionCallback(Ptr<PhyTxStatsCalculator> phyTxStats,
                                                std::string path,
                                                PhyTransmissionStatParameters params)
{
    NS_LOG_FUNCTION(phyTxStats << path);
    uint64_t imsi = 0;
    std::ostringstream pathAndRnti;
    std::string pathEnb = path.substr(0, path.find("/ComponentCarrierMap"));
    pathAndRnti << pathEnb << "/LteEnbRrc/UeMap/" << params.m_rnti;
    if (phyTxStats->ExistsImsiPath(pathAndRnti.str()))
    {
        imsi = phyTxStats->GetImsiPath(pathAndRnti.str());
    }
    else
    {
        imsi = FindImsiFromEnbRlcPath(pathAndRnti.str());
        phyTxStats->SetImsiPath(pathAndRnti.str(), imsi);
    }

    params.m_imsi = imsi;
    phyTxStats->DlPhyTransmission(params);
}

void
PhyTxStatsCalculator::UlPhyTransmissionCallback(Ptr<PhyTxStatsCalculator> phyTxStats,
                                                std::string path,
                                                PhyTransmissionStatParameters params)
{
    NS_LOG_FUNCTION(phyTxStats << path);
    uint64_t imsi = 0;
    std::ostringstream pathAndRnti;
    pathAndRnti << path << "/" << params.m_rnti;
    std::string pathUePhy = path.substr(0, path.find("/ComponentCarrierMapUe"));
    if (phyTxStats->ExistsImsiPath(pathAndRnti.str()))
    {
        imsi = phyTxStats->GetImsiPath(pathAndRnti.str());
    }
    else
    {
        imsi = FindImsiFromLteNetDevice(pathUePhy);
        phyTxStats->SetImsiPath(pathAndRnti.str(), imsi);
    }

    params.m_imsi = imsi;
    phyTxStats->UlPhyTransmission(params);
}

} // namespace ns3
