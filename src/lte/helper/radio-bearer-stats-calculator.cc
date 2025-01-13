/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jaume Nin <jnin@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#include "radio-bearer-stats-calculator.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/string.h"

#include <algorithm>
#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RadioBearerStatsCalculator");

NS_OBJECT_ENSURE_REGISTERED(RadioBearerStatsCalculator);

RadioBearerStatsCalculator::RadioBearerStatsCalculator()
    : m_firstWrite(true),
      m_pendingOutput(false),
      m_protocolType("RLC")
{
    NS_LOG_FUNCTION(this);
}

RadioBearerStatsCalculator::RadioBearerStatsCalculator(std::string protocolType)
    : m_firstWrite(true),
      m_pendingOutput(false)
{
    NS_LOG_FUNCTION(this);
    m_protocolType = protocolType;
}

RadioBearerStatsCalculator::~RadioBearerStatsCalculator()
{
    NS_LOG_FUNCTION(this);
}

TypeId
RadioBearerStatsCalculator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RadioBearerStatsCalculator")
            .SetParent<LteStatsCalculator>()
            .AddConstructor<RadioBearerStatsCalculator>()
            .SetGroupName("Lte")
            .AddAttribute("StartTime",
                          "Start time of the on going epoch.",
                          TimeValue(Seconds(0.)),
                          MakeTimeAccessor(&RadioBearerStatsCalculator::SetStartTime,
                                           &RadioBearerStatsCalculator::GetStartTime),
                          MakeTimeChecker())
            .AddAttribute("EpochDuration",
                          "Epoch duration.",
                          TimeValue(Seconds(0.25)),
                          MakeTimeAccessor(&RadioBearerStatsCalculator::GetEpoch,
                                           &RadioBearerStatsCalculator::SetEpoch),
                          MakeTimeChecker())
            .AddAttribute("DlRlcOutputFilename",
                          "Name of the file where the downlink results will be saved.",
                          StringValue("DlRlcStats.txt"),
                          MakeStringAccessor(&LteStatsCalculator::SetDlOutputFilename),
                          MakeStringChecker())
            .AddAttribute("UlRlcOutputFilename",
                          "Name of the file where the uplink results will be saved.",
                          StringValue("UlRlcStats.txt"),
                          MakeStringAccessor(&LteStatsCalculator::SetUlOutputFilename),
                          MakeStringChecker())
            .AddAttribute("DlPdcpOutputFilename",
                          "Name of the file where the downlink results will be saved.",
                          StringValue("DlPdcpStats.txt"),
                          MakeStringAccessor(&RadioBearerStatsCalculator::SetDlPdcpOutputFilename),
                          MakeStringChecker())
            .AddAttribute("UlPdcpOutputFilename",
                          "Name of the file where the uplink results will be saved.",
                          StringValue("UlPdcpStats.txt"),
                          MakeStringAccessor(&RadioBearerStatsCalculator::SetUlPdcpOutputFilename),
                          MakeStringChecker());
    return tid;
}

void
RadioBearerStatsCalculator::DoDispose()
{
    NS_LOG_FUNCTION(this);
    if (m_pendingOutput)
    {
        ShowResults();
    }
}

void
RadioBearerStatsCalculator::SetStartTime(Time t)
{
    m_startTime = t;
    RescheduleEndEpoch();
}

Time
RadioBearerStatsCalculator::GetStartTime() const
{
    return m_startTime;
}

void
RadioBearerStatsCalculator::SetEpoch(Time e)
{
    m_epochDuration = e;
    RescheduleEndEpoch();
}

Time
RadioBearerStatsCalculator::GetEpoch() const
{
    return m_epochDuration;
}

void
RadioBearerStatsCalculator::UlTxPdu(uint16_t cellId,
                                    uint64_t imsi,
                                    uint16_t rnti,
                                    uint8_t lcid,
                                    uint32_t packetSize)
{
    NS_LOG_FUNCTION(this << "UlTxPDU" << cellId << imsi << rnti << (uint32_t)lcid << packetSize);
    ImsiLcidPair_t p(imsi, lcid);
    if (Simulator::Now() >= m_startTime)
    {
        m_ulCellId[p] = cellId;
        m_flowId[p] = LteFlowId_t(rnti, lcid);
        m_ulTxPackets[p]++;
        m_ulTxData[p] += packetSize;
    }
    m_pendingOutput = true;
}

void
RadioBearerStatsCalculator::DlTxPdu(uint16_t cellId,
                                    uint64_t imsi,
                                    uint16_t rnti,
                                    uint8_t lcid,
                                    uint32_t packetSize)
{
    NS_LOG_FUNCTION(this << "DlTxPDU" << cellId << imsi << rnti << (uint32_t)lcid << packetSize);
    ImsiLcidPair_t p(imsi, lcid);
    if (Simulator::Now() >= m_startTime)
    {
        m_dlCellId[p] = cellId;
        m_flowId[p] = LteFlowId_t(rnti, lcid);
        m_dlTxPackets[p]++;
        m_dlTxData[p] += packetSize;
    }
    m_pendingOutput = true;
}

void
RadioBearerStatsCalculator::UlRxPdu(uint16_t cellId,
                                    uint64_t imsi,
                                    uint16_t rnti,
                                    uint8_t lcid,
                                    uint32_t packetSize,
                                    uint64_t delay)
{
    NS_LOG_FUNCTION(this << "UlRxPDU" << cellId << imsi << rnti << (uint32_t)lcid << packetSize
                         << delay);
    ImsiLcidPair_t p(imsi, lcid);
    if (Simulator::Now() >= m_startTime)
    {
        m_ulCellId[p] = cellId;
        m_ulRxPackets[p]++;
        m_ulRxData[p] += packetSize;

        auto it = m_ulDelay.find(p);
        if (it == m_ulDelay.end())
        {
            NS_LOG_DEBUG(this << " Creating UL stats calculators for IMSI " << p.m_imsi
                              << " and LCID " << (uint32_t)p.m_lcId);
            m_ulDelay[p] = CreateObject<MinMaxAvgTotalCalculator<uint64_t>>();
            m_ulPduSize[p] = CreateObject<MinMaxAvgTotalCalculator<uint32_t>>();
        }
        m_ulDelay[p]->Update(delay);
        m_ulPduSize[p]->Update(packetSize);
    }
    m_pendingOutput = true;
}

void
RadioBearerStatsCalculator::DlRxPdu(uint16_t cellId,
                                    uint64_t imsi,
                                    uint16_t rnti,
                                    uint8_t lcid,
                                    uint32_t packetSize,
                                    uint64_t delay)
{
    NS_LOG_FUNCTION(this << "DlRxPDU" << cellId << imsi << rnti << (uint32_t)lcid << packetSize
                         << delay);
    ImsiLcidPair_t p(imsi, lcid);
    if (Simulator::Now() >= m_startTime)
    {
        m_dlCellId[p] = cellId;
        m_dlRxPackets[p]++;
        m_dlRxData[p] += packetSize;

        auto it = m_dlDelay.find(p);
        if (it == m_dlDelay.end())
        {
            NS_LOG_DEBUG(this << " Creating DL stats calculators for IMSI " << p.m_imsi
                              << " and LCID " << (uint32_t)p.m_lcId);
            m_dlDelay[p] = CreateObject<MinMaxAvgTotalCalculator<uint64_t>>();
            m_dlPduSize[p] = CreateObject<MinMaxAvgTotalCalculator<uint32_t>>();
        }
        m_dlDelay[p]->Update(delay);
        m_dlPduSize[p]->Update(packetSize);
    }
    m_pendingOutput = true;
}

void
RadioBearerStatsCalculator::ShowResults()
{
    NS_LOG_FUNCTION(this << GetUlOutputFilename() << GetDlOutputFilename());
    NS_LOG_INFO("Write Rlc Stats in " << GetUlOutputFilename() << " and in "
                                      << GetDlOutputFilename());

    std::ofstream ulOutFile;
    std::ofstream dlOutFile;

    if (m_firstWrite)
    {
        ulOutFile.open(GetUlOutputFilename());
        if (!ulOutFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetUlOutputFilename());
            return;
        }

        dlOutFile.open(GetDlOutputFilename());
        if (!dlOutFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetDlOutputFilename());
            return;
        }
        m_firstWrite = false;
        ulOutFile << "% start\tend\tCellId\tIMSI\tRNTI\tLCID\tnTxPDUs\tTxBytes\tnRxPDUs\tRxBytes\t";
        ulOutFile << "delay\tstdDev\tmin\tmax\t";
        ulOutFile << "PduSize\tstdDev\tmin\tmax";
        ulOutFile << std::endl;
        dlOutFile << "% start\tend\tCellId\tIMSI\tRNTI\tLCID\tnTxPDUs\tTxBytes\tnRxPDUs\tRxBytes\t";
        dlOutFile << "delay\tstdDev\tmin\tmax\t";
        dlOutFile << "PduSize\tstdDev\tmin\tmax";
        dlOutFile << std::endl;
    }
    else
    {
        ulOutFile.open(GetUlOutputFilename(), std::ios_base::app);
        if (!ulOutFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetUlOutputFilename());
            return;
        }

        dlOutFile.open(GetDlOutputFilename(), std::ios_base::app);
        if (!dlOutFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetDlOutputFilename());
            return;
        }
    }

    WriteUlResults(ulOutFile);
    WriteDlResults(dlOutFile);
    m_pendingOutput = false;
}

void
RadioBearerStatsCalculator::WriteUlResults(std::ofstream& outFile)
{
    NS_LOG_FUNCTION(this);

    // Get the unique IMSI/LCID pairs list
    std::vector<ImsiLcidPair_t> pairVector;
    for (auto it = m_ulTxPackets.begin(); it != m_ulTxPackets.end(); ++it)
    {
        if (find(pairVector.begin(), pairVector.end(), (*it).first) == pairVector.end())
        {
            pairVector.push_back((*it).first);
        }
    }

    for (auto it = m_ulRxPackets.begin(); it != m_ulRxPackets.end(); ++it)
    {
        if (find(pairVector.begin(), pairVector.end(), (*it).first) == pairVector.end())
        {
            pairVector.push_back((*it).first);
        }
    }

    Time endTime = m_startTime + m_epochDuration;
    for (auto it = pairVector.begin(); it != pairVector.end(); ++it)
    {
        ImsiLcidPair_t p = *it;
        auto flowIdIt = m_flowId.find(p);
        NS_ASSERT_MSG(flowIdIt != m_flowId.end(),
                      "FlowId (imsi " << p.m_imsi << " lcid " << (uint32_t)p.m_lcId
                                      << ") is missing");
        LteFlowId_t flowId = flowIdIt->second;
        NS_ASSERT_MSG(flowId.m_lcId == p.m_lcId, "lcid mismatch");

        outFile << m_startTime.GetSeconds() << "\t";
        outFile << endTime.GetSeconds() << "\t";
        outFile << GetUlCellId(p.m_imsi, p.m_lcId) << "\t";
        outFile << p.m_imsi << "\t";
        outFile << flowId.m_rnti << "\t";
        outFile << (uint32_t)flowId.m_lcId << "\t";
        outFile << GetUlTxPackets(p.m_imsi, p.m_lcId) << "\t";
        outFile << GetUlTxData(p.m_imsi, p.m_lcId) << "\t";
        outFile << GetUlRxPackets(p.m_imsi, p.m_lcId) << "\t";
        outFile << GetUlRxData(p.m_imsi, p.m_lcId) << "\t";
        std::vector<double> stats = GetUlDelayStats(p.m_imsi, p.m_lcId);
        for (auto it = stats.begin(); it != stats.end(); ++it)
        {
            outFile << (*it) * 1e-9 << "\t";
        }
        stats = GetUlPduSizeStats(p.m_imsi, p.m_lcId);
        for (auto it = stats.begin(); it != stats.end(); ++it)
        {
            outFile << (*it) << "\t";
        }
        outFile << std::endl;
    }

    outFile.close();
}

void
RadioBearerStatsCalculator::WriteDlResults(std::ofstream& outFile)
{
    NS_LOG_FUNCTION(this);

    // Get the unique IMSI/LCID pairs list
    std::vector<ImsiLcidPair_t> pairVector;
    for (auto it = m_dlTxPackets.begin(); it != m_dlTxPackets.end(); ++it)
    {
        if (find(pairVector.begin(), pairVector.end(), (*it).first) == pairVector.end())
        {
            pairVector.push_back((*it).first);
        }
    }

    for (auto it = m_dlRxPackets.begin(); it != m_dlRxPackets.end(); ++it)
    {
        if (find(pairVector.begin(), pairVector.end(), (*it).first) == pairVector.end())
        {
            pairVector.push_back((*it).first);
        }
    }

    Time endTime = m_startTime + m_epochDuration;
    for (auto pair = pairVector.begin(); pair != pairVector.end(); ++pair)
    {
        ImsiLcidPair_t p = *pair;
        auto flowIdIt = m_flowId.find(p);
        NS_ASSERT_MSG(flowIdIt != m_flowId.end(),
                      "FlowId (imsi " << p.m_imsi << " lcid " << (uint32_t)p.m_lcId
                                      << ") is missing");
        LteFlowId_t flowId = flowIdIt->second;
        NS_ASSERT_MSG(flowId.m_lcId == p.m_lcId, "lcid mismatch");

        outFile << m_startTime.GetSeconds() << "\t";
        outFile << endTime.GetSeconds() << "\t";
        outFile << GetDlCellId(p.m_imsi, p.m_lcId) << "\t";
        outFile << p.m_imsi << "\t";
        outFile << flowId.m_rnti << "\t";
        outFile << (uint32_t)flowId.m_lcId << "\t";
        outFile << GetDlTxPackets(p.m_imsi, p.m_lcId) << "\t";
        outFile << GetDlTxData(p.m_imsi, p.m_lcId) << "\t";
        outFile << GetDlRxPackets(p.m_imsi, p.m_lcId) << "\t";
        outFile << GetDlRxData(p.m_imsi, p.m_lcId) << "\t";
        std::vector<double> stats = GetDlDelayStats(p.m_imsi, p.m_lcId);
        for (auto it = stats.begin(); it != stats.end(); ++it)
        {
            outFile << (*it) * 1e-9 << "\t";
        }
        stats = GetDlPduSizeStats(p.m_imsi, p.m_lcId);
        for (auto it = stats.begin(); it != stats.end(); ++it)
        {
            outFile << (*it) << "\t";
        }
        outFile << std::endl;
    }

    outFile.close();
}

void
RadioBearerStatsCalculator::ResetResults()
{
    NS_LOG_FUNCTION(this);

    m_ulTxPackets.erase(m_ulTxPackets.begin(), m_ulTxPackets.end());
    m_ulRxPackets.erase(m_ulRxPackets.begin(), m_ulRxPackets.end());
    m_ulRxData.erase(m_ulRxData.begin(), m_ulRxData.end());
    m_ulTxData.erase(m_ulTxData.begin(), m_ulTxData.end());
    m_ulDelay.erase(m_ulDelay.begin(), m_ulDelay.end());
    m_ulPduSize.erase(m_ulPduSize.begin(), m_ulPduSize.end());

    m_dlTxPackets.erase(m_dlTxPackets.begin(), m_dlTxPackets.end());
    m_dlRxPackets.erase(m_dlRxPackets.begin(), m_dlRxPackets.end());
    m_dlRxData.erase(m_dlRxData.begin(), m_dlRxData.end());
    m_dlTxData.erase(m_dlTxData.begin(), m_dlTxData.end());
    m_dlDelay.erase(m_dlDelay.begin(), m_dlDelay.end());
    m_dlPduSize.erase(m_dlPduSize.begin(), m_dlPduSize.end());
}

void
RadioBearerStatsCalculator::RescheduleEndEpoch()
{
    NS_LOG_FUNCTION(this);
    m_endEpochEvent.Cancel();
    NS_ASSERT(Simulator::Now().GetMilliSeconds() == 0); // below event time assumes this
    m_endEpochEvent = Simulator::Schedule(m_startTime + m_epochDuration,
                                          &RadioBearerStatsCalculator::EndEpoch,
                                          this);
}

void
RadioBearerStatsCalculator::EndEpoch()
{
    NS_LOG_FUNCTION(this);
    ShowResults();
    ResetResults();
    m_startTime += m_epochDuration;
    m_endEpochEvent =
        Simulator::Schedule(m_epochDuration, &RadioBearerStatsCalculator::EndEpoch, this);
}

uint32_t
RadioBearerStatsCalculator::GetUlTxPackets(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    return m_ulTxPackets[p];
}

uint32_t
RadioBearerStatsCalculator::GetUlRxPackets(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    return m_ulRxPackets[p];
}

uint64_t
RadioBearerStatsCalculator::GetUlTxData(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    return m_ulTxData[p];
}

uint64_t
RadioBearerStatsCalculator::GetUlRxData(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    return m_ulRxData[p];
}

double
RadioBearerStatsCalculator::GetUlDelay(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    auto it = m_ulDelay.find(p);
    if (it == m_ulDelay.end())
    {
        NS_LOG_ERROR("UL delay for " << imsi << " - " << (uint16_t)lcid << " not found");
        return 0;
    }
    return m_ulDelay[p]->getMean();
}

std::vector<double>
RadioBearerStatsCalculator::GetUlDelayStats(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    std::vector<double> stats;
    auto it = m_ulDelay.find(p);
    if (it == m_ulDelay.end())
    {
        stats.push_back(0.0);
        stats.push_back(0.0);
        stats.push_back(0.0);
        stats.push_back(0.0);
        return stats;
    }
    stats.push_back(m_ulDelay[p]->getMean());
    stats.push_back(m_ulDelay[p]->getStddev());
    stats.push_back(m_ulDelay[p]->getMin());
    stats.push_back(m_ulDelay[p]->getMax());
    return stats;
}

std::vector<double>
RadioBearerStatsCalculator::GetUlPduSizeStats(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    std::vector<double> stats;
    auto it = m_ulPduSize.find(p);
    if (it == m_ulPduSize.end())
    {
        stats.push_back(0.0);
        stats.push_back(0.0);
        stats.push_back(0.0);
        stats.push_back(0.0);
        return stats;
    }
    stats.push_back(m_ulPduSize[p]->getMean());
    stats.push_back(m_ulPduSize[p]->getStddev());
    stats.push_back(m_ulPduSize[p]->getMin());
    stats.push_back(m_ulPduSize[p]->getMax());
    return stats;
}

uint32_t
RadioBearerStatsCalculator::GetDlTxPackets(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    return m_dlTxPackets[p];
}

uint32_t
RadioBearerStatsCalculator::GetDlRxPackets(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    return m_dlRxPackets[p];
}

uint64_t
RadioBearerStatsCalculator::GetDlTxData(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    return m_dlTxData[p];
}

uint64_t
RadioBearerStatsCalculator::GetDlRxData(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    return m_dlRxData[p];
}

uint32_t
RadioBearerStatsCalculator::GetUlCellId(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    return m_ulCellId[p];
}

uint32_t
RadioBearerStatsCalculator::GetDlCellId(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    return m_dlCellId[p];
}

double
RadioBearerStatsCalculator::GetDlDelay(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    auto it = m_dlDelay.find(p);
    if (it == m_dlDelay.end())
    {
        NS_LOG_ERROR("DL delay for " << imsi << " not found");
        return 0;
    }
    return m_dlDelay[p]->getMean();
}

std::vector<double>
RadioBearerStatsCalculator::GetDlDelayStats(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    std::vector<double> stats;
    auto it = m_dlDelay.find(p);
    if (it == m_dlDelay.end())
    {
        stats.push_back(0.0);
        stats.push_back(0.0);
        stats.push_back(0.0);
        stats.push_back(0.0);
        return stats;
    }
    stats.push_back(m_dlDelay[p]->getMean());
    stats.push_back(m_dlDelay[p]->getStddev());
    stats.push_back(m_dlDelay[p]->getMin());
    stats.push_back(m_dlDelay[p]->getMax());
    return stats;
}

std::vector<double>
RadioBearerStatsCalculator::GetDlPduSizeStats(uint64_t imsi, uint8_t lcid)
{
    NS_LOG_FUNCTION(this << imsi << (uint16_t)lcid);
    ImsiLcidPair_t p(imsi, lcid);
    std::vector<double> stats;
    auto it = m_dlPduSize.find(p);
    if (it == m_dlPduSize.end())
    {
        stats.push_back(0.0);
        stats.push_back(0.0);
        stats.push_back(0.0);
        stats.push_back(0.0);
        return stats;
    }
    stats.push_back(m_dlPduSize[p]->getMean());
    stats.push_back(m_dlPduSize[p]->getStddev());
    stats.push_back(m_dlPduSize[p]->getMin());
    stats.push_back(m_dlPduSize[p]->getMax());
    return stats;
}

std::string
RadioBearerStatsCalculator::GetUlOutputFilename()
{
    if (m_protocolType == "RLC")
    {
        return LteStatsCalculator::GetUlOutputFilename();
    }
    else
    {
        return GetUlPdcpOutputFilename();
    }
}

std::string
RadioBearerStatsCalculator::GetDlOutputFilename()
{
    if (m_protocolType == "RLC")
    {
        return LteStatsCalculator::GetDlOutputFilename();
    }
    else
    {
        return GetDlPdcpOutputFilename();
    }
}

void
RadioBearerStatsCalculator::SetUlPdcpOutputFilename(std::string outputFilename)
{
    m_ulPdcpOutputFilename = outputFilename;
}

std::string
RadioBearerStatsCalculator::GetUlPdcpOutputFilename()
{
    return m_ulPdcpOutputFilename;
}

void
RadioBearerStatsCalculator::SetDlPdcpOutputFilename(std::string outputFilename)
{
    m_dlPdcpOutputFilename = outputFilename;
}

std::string
RadioBearerStatsCalculator::GetDlPdcpOutputFilename()
{
    return m_dlPdcpOutputFilename;
}

} // namespace ns3
