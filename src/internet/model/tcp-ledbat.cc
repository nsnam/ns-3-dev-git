/*
 * Copyright (c) 2016 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ankit Deepak <adadeepak8@gmail.com>
 * Modified by: S B L Prateek <sblprateek@gmail.com>
 *
 */

#include "tcp-ledbat.h"

#include "tcp-socket-state.h"

#include "ns3/log.h"
#include "ns3/simulator.h" // Now ()

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpLedbat");
NS_OBJECT_ENSURE_REGISTERED(TcpLedbat);

TypeId
TcpLedbat::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TcpLedbat")
            .SetParent<TcpNewReno>()
            .AddConstructor<TcpLedbat>()
            .SetGroupName("Internet")
            .AddAttribute("TargetDelay",
                          "Targeted Queue Delay",
                          TimeValue(MilliSeconds(100)),
                          MakeTimeAccessor(&TcpLedbat::m_target),
                          MakeTimeChecker())
            .AddAttribute("BaseHistoryLen",
                          "Number of Base delay samples",
                          UintegerValue(10),
                          MakeUintegerAccessor(&TcpLedbat::m_baseHistoLen),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("NoiseFilterLen",
                          "Number of Current delay samples",
                          UintegerValue(4),
                          MakeUintegerAccessor(&TcpLedbat::m_noiseFilterLen),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("Gain",
                          "Offset Gain",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&TcpLedbat::m_gain),
                          MakeDoubleChecker<double>(1e-6))
            .AddAttribute("SSParam",
                          "Possibility of Slow Start",
                          EnumValue(DO_SLOWSTART),
                          MakeEnumAccessor<SlowStartType>(&TcpLedbat::SetDoSs),
                          MakeEnumChecker(DO_SLOWSTART, "yes", DO_NOT_SLOWSTART, "no"))
            .AddAttribute("MinCwnd",
                          "Minimum cWnd for Ledbat",
                          UintegerValue(2),
                          MakeUintegerAccessor(&TcpLedbat::m_minCwnd),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("AllowedIncrease",
                          "Allowed Increase",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&TcpLedbat::m_allowedIncrease),
                          MakeDoubleChecker<double>(1e-6));
    return tid;
}

void
TcpLedbat::SetDoSs(SlowStartType doSS)
{
    NS_LOG_FUNCTION(this << doSS);
    m_doSs = doSS;
    if (m_doSs)
    {
        m_flag |= LEDBAT_CAN_SS;
    }
    else
    {
        m_flag &= ~LEDBAT_CAN_SS;
    }
}

TcpLedbat::TcpLedbat()
    : TcpNewReno()
{
    NS_LOG_FUNCTION(this);
    InitCircBuf(m_baseHistory);
    InitCircBuf(m_noiseFilter);
    m_lastRollover = Seconds(0);
    m_sndCwndCnt = 0;
    m_flag = LEDBAT_CAN_SS;
}

void
TcpLedbat::InitCircBuf(OwdCircBuf& buffer)
{
    NS_LOG_FUNCTION(this);
    buffer.buffer.clear();
    buffer.min = 0;
}

TcpLedbat::TcpLedbat(const TcpLedbat& sock)
    : TcpNewReno(sock)
{
    NS_LOG_FUNCTION(this);
    m_target = sock.m_target;
    m_gain = sock.m_gain;
    m_doSs = sock.m_doSs;
    m_baseHistoLen = sock.m_baseHistoLen;
    m_noiseFilterLen = sock.m_noiseFilterLen;
    m_baseHistory = sock.m_baseHistory;
    m_noiseFilter = sock.m_noiseFilter;
    m_lastRollover = sock.m_lastRollover;
    m_sndCwndCnt = sock.m_sndCwndCnt;
    m_flag = sock.m_flag;
    m_minCwnd = sock.m_minCwnd;
    m_allowedIncrease = sock.m_allowedIncrease;
}

TcpLedbat::~TcpLedbat()
{
    NS_LOG_FUNCTION(this);
}

Ptr<TcpCongestionOps>
TcpLedbat::Fork()
{
    return CopyObject<TcpLedbat>(this);
}

std::string
TcpLedbat::GetName() const
{
    return "TcpLedbat";
}

uint32_t
TcpLedbat::MinCircBuf(OwdCircBuf& b)
{
    NS_LOG_FUNCTION_NOARGS();
    if (b.buffer.empty())
    {
        return ~0U;
    }
    else
    {
        return b.buffer[b.min];
    }
}

uint32_t
TcpLedbat::CurrentDelay(FilterFunction filter)
{
    NS_LOG_FUNCTION(this);
    return filter(m_noiseFilter);
}

uint32_t
TcpLedbat::BaseDelay()
{
    NS_LOG_FUNCTION(this);
    return MinCircBuf(m_baseHistory);
}

void
TcpLedbat::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);
    if (tcb->m_cWnd.Get() <= tcb->m_segmentSize)
    {
        m_flag |= LEDBAT_CAN_SS;
    }
    if (m_doSs == DO_SLOWSTART && tcb->m_cWnd <= tcb->m_ssThresh && (m_flag & LEDBAT_CAN_SS))
    {
        SlowStart(tcb, segmentsAcked);
    }
    else
    {
        m_flag &= ~LEDBAT_CAN_SS;
        CongestionAvoidance(tcb, segmentsAcked);
    }
}

void
TcpLedbat::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);
    if ((m_flag & LEDBAT_VALID_OWD) == 0)
    {
        TcpNewReno::CongestionAvoidance(
            tcb,
            segmentsAcked); // letting it fall to TCP behaviour if no timestamps
        return;
    }
    uint32_t queueDelay;
    double offset;
    uint32_t cwnd = tcb->m_cWnd.Get();
    uint32_t maxCwnd;
    uint32_t currentDelay = CurrentDelay(&TcpLedbat::MinCircBuf);
    uint32_t baseDelay = BaseDelay();

    if (currentDelay > baseDelay)
    {
        queueDelay = currentDelay - baseDelay;
        offset = m_target.GetMilliSeconds() - queueDelay;
    }
    else
    {
        queueDelay = baseDelay - currentDelay;
        offset = m_target.GetMilliSeconds() + queueDelay;
    }
    offset *= m_gain;
    m_sndCwndCnt = offset * segmentsAcked * tcb->m_segmentSize;
    double inc = m_sndCwndCnt / (m_target.GetMilliSeconds() * tcb->m_cWnd.Get());
    cwnd += (inc * tcb->m_segmentSize);

    // Since m_bytesInFlight reflects the state after processing the current ACK,
    // adding segmentsAcked * m_segmentSize reconstructs the flight size before this
    // ACK arrived, as required by the RFC definition of flight size.
    uint32_t flightSizeBeforeAck =
        tcb->m_bytesInFlight.Get() + (segmentsAcked * tcb->m_segmentSize);
    maxCwnd = flightSizeBeforeAck + static_cast<uint32_t>(m_allowedIncrease * tcb->m_segmentSize);
    cwnd = std::min(cwnd, maxCwnd);
    cwnd = std::max(cwnd, m_minCwnd * tcb->m_segmentSize);
    tcb->m_cWnd = cwnd;

    if (tcb->m_cWnd <= tcb->m_ssThresh)
    {
        tcb->m_ssThresh = tcb->m_cWnd - 1;
    }
}

void
TcpLedbat::AddDelay(OwdCircBuf& cb, uint32_t owd, uint32_t maxlen)
{
    NS_LOG_FUNCTION(this << owd << maxlen << cb.buffer.size());
    if (cb.buffer.empty())
    {
        NS_LOG_LOGIC("First Value for queue");
        cb.buffer.push_back(owd);
        cb.min = 0;
        return;
    }
    cb.buffer.push_back(owd);
    if (cb.buffer[cb.min] > owd)
    {
        cb.min = cb.buffer.size() - 1;
    }
    if (cb.buffer.size() >= maxlen)
    {
        NS_LOG_LOGIC("Queue full" << maxlen);
        cb.buffer.erase(cb.buffer.begin());
        auto bufferStart = cb.buffer.begin();
        cb.min = std::distance(bufferStart, std::min_element(bufferStart, cb.buffer.end()));
        NS_LOG_LOGIC("Current min element" << cb.buffer[cb.min]);
    }
}

void
TcpLedbat::UpdateBaseDelay(uint32_t owd)
{
    NS_LOG_FUNCTION(this << owd);
    if (m_baseHistory.buffer.empty())
    {
        AddDelay(m_baseHistory, owd, m_baseHistoLen);
        return;
    }
    Time timestamp = Simulator::Now();

    if ((timestamp - m_lastRollover) > Seconds(60))
    {
        m_lastRollover = timestamp;
        AddDelay(m_baseHistory, owd, m_baseHistoLen);
    }
    else
    {
        size_t last = m_baseHistory.buffer.size() - 1;
        if (owd < m_baseHistory.buffer[last])
        {
            m_baseHistory.buffer[last] = owd;
            if (owd < m_baseHistory.buffer[m_baseHistory.min])
            {
                m_baseHistory.min = last;
            }
        }
    }
}

void
TcpLedbat::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked << rtt);
    if (tcb->m_rcvTimestampValue == 0 || tcb->m_rcvTimestampEchoReply == 0)
    {
        m_flag &= ~LEDBAT_VALID_OWD;
    }
    else
    {
        m_flag |= LEDBAT_VALID_OWD;
    }
    if (rtt.IsPositive() && tcb->m_rcvTimestampValue >= tcb->m_rcvTimestampEchoReply)
    {
        uint32_t owd = tcb->m_rcvTimestampValue - tcb->m_rcvTimestampEchoReply;
        AddDelay(m_noiseFilter, owd, m_noiseFilterLen);
        UpdateBaseDelay(owd);
    }
}

} // namespace ns3
