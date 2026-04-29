/*
 * Copyright © 2011 Marcos Talau
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marcos Talau (talau@users.sourceforge.net)
 *
 * Thanks to: Duy Nguyen<duy@soe.ucsc.edu> by RED efforts in NS3
 *
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (c) 1990-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * PORT NOTE: This code was ported from ns-2 (queue/red.cc).  Almost all
 * comments have also been ported from NS-2
 */

#include "red-queue-disc.h"

#include "ns3/abort.h"
#include "ns3/double.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RedQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(RedQueueDisc);

TypeId
RedQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RedQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<RedQueueDisc>()
            .AddAttribute("MeanPktSize",
                          "Average of packet size",
                          UintegerValue(500),
                          MakeUintegerAccessor(&RedQueueDisc::m_meanPktSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("IdleQPktSize",
                          "Average packet size used during idle times. Used when m_dropCautionMode = 3",
                          UintegerValue(0),
                          MakeUintegerAccessor(&RedQueueDisc::m_idleQPktSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("WaitBetweenDrops",
                          "True for waiting between dropped packets",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RedQueueDisc::m_isWait),
                          MakeBooleanChecker())
            .AddAttribute("Gentle",
                          "True to increases dropping probability slowly when average queue "
                          "exceeds maxthresh",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RedQueueDisc::m_isGentle),
                          MakeBooleanChecker())
            .AddAttribute("ARED",
                          "True to enable ARED",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RedQueueDisc::m_isARED),
                          MakeBooleanChecker())
            .AddAttribute("AdaptMaxP",
                          "True to adapt m_curMaxP",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RedQueueDisc::m_isAdaptMaxP),
                          MakeBooleanChecker())
            .AddAttribute("FengAdaptive",
                          "True to enable Feng's Adaptive RED",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RedQueueDisc::m_isFengAdaptive),
                          MakeBooleanChecker())
            .AddAttribute("NLRED",
                          "True to enable Nonlinear RED",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RedQueueDisc::m_isNonlinear),
                          MakeBooleanChecker())
            .AddAttribute("MinTh",
                          "Minimum average length threshold in packets/bytes",
                          DoubleValue(5),
                          MakeDoubleAccessor(&RedQueueDisc::m_minTh),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxTh",
                          "Maximum average length threshold in packets/bytes",
                          DoubleValue(15),
                          MakeDoubleAccessor(&RedQueueDisc::m_maxTh),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxQueueSize",
                          "The maximum number of packets accepted by this queue disc",
                          QueueSizeValue(QueueSize("25p")),
                          MakeQueueSizeAccessor(&QueueDisc::SetMaxQueueSize, &QueueDisc::GetMaxQueueSize),
                          MakeQueueSizeChecker())
            .AddAttribute("WQ",
                          "Queue weight related to the exponential weighted moving average (EWMA)",
                          DoubleValue(0.002),
                          MakeDoubleAccessor(&RedQueueDisc::m_wQ),
                          MakeDoubleChecker<double>())
            .AddAttribute("LInterm",
                          "The maximum probability of dropping a packet",
                          DoubleValue(50),
                          MakeDoubleAccessor(&RedQueueDisc::m_lInterm),
                          MakeDoubleChecker<double>())
            .AddAttribute("TargetQueueDelay",
                          "Target average queuing delay in ARED",
                          TimeValue(Seconds(0.005)),
                          MakeTimeAccessor(&RedQueueDisc::m_targetQueueDelay),
                          MakeTimeChecker())
            .AddAttribute("Interval",
                          "Time interval to update m_curMaxP",
                          TimeValue(Seconds(0.5)),
                          MakeTimeAccessor(&RedQueueDisc::m_interval),
                          MakeTimeChecker())
            .AddAttribute("UbCurMaxP",
                          "Upper bound for m_curMaxP in ARED",
                          DoubleValue(0.5),
                          MakeDoubleAccessor(&RedQueueDisc::m_ubCurMaxP),
                          MakeDoubleChecker<double>(0, 1))
            .AddAttribute("LbCurMaxP",
                          "Lower bound for m_curMaxP in ARED",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&RedQueueDisc::m_lbCurMaxP),
                          MakeDoubleChecker<double>(0, 1))
            .AddAttribute("AREDAlpha",
                          "Increment parameter for m_curMaxP in ARED",
                          DoubleValue(0.01),
                          MakeDoubleAccessor(&RedQueueDisc::SetAredAlpha),
                          MakeDoubleChecker<double>(0, 1))
            .AddAttribute("AREDBeta",
                          "Decrement parameter for m_curMaxP in ARED",
                          DoubleValue(0.9),
                          MakeDoubleAccessor(&RedQueueDisc::SetAredBeta),
                          MakeDoubleChecker<double>(0, 1))
            .AddAttribute("FengAlpha",
                          "Decrement parameter for m_curMaxP in Feng's Adaptive RED",
                          DoubleValue(3.0),
                          MakeDoubleAccessor(&RedQueueDisc::SetFengAdaptiveA),
                          MakeDoubleChecker<double>())
            .AddAttribute("FengBeta",
                          "Increment parameter for m_curMaxP in Feng's Adaptive RED",
                          DoubleValue(2.0),
                          MakeDoubleAccessor(&RedQueueDisc::SetFengAdaptiveB),
                          MakeDoubleChecker<double>())
            .AddAttribute("lastSet_currMaxP_At",
                          "Store the last time m_curMaxP was updated",
                          TimeValue(Seconds(0)),
                          MakeTimeAccessor(&RedQueueDisc::m_lastSet_currMaxP_At),
                          MakeTimeChecker())
            .AddAttribute("Rtt",
                          "Round Trip Time to be considered while automatically setting m_lbCurMaxP",
                          TimeValue(Seconds(0.1)),
                          MakeTimeAccessor(&RedQueueDisc::m_rtt),
                          MakeTimeChecker())
            .AddAttribute("Ns1Compat",
                          "NS-1 compatibility",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RedQueueDisc::m_isNs1Compat),
                          MakeBooleanChecker())
            .AddAttribute("LinkBandwidth",
                          "The RED link bandwidth",
                          DataRateValue(DataRate("1.5Mbps")),
                          MakeDataRateAccessor(&RedQueueDisc::m_linkBandwidth),
                          MakeDataRateChecker())
            .AddAttribute("LinkDelay",
                          "The RED link delay",
                          TimeValue(MilliSeconds(20)),
                          MakeTimeAccessor(&RedQueueDisc::m_linkDelay),
                          MakeTimeChecker())
            .AddAttribute("UseEcn",
                          "True to use ECN (packets are marked instead of being dropped)",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RedQueueDisc::m_useEcn),
                          MakeBooleanChecker())
            .AddAttribute("UseHardDrop",
                          "True to always drop packets above max threshold",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RedQueueDisc::m_useHardDrop),
                          MakeBooleanChecker());

    return tid;
}

RedQueueDisc::RedQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
    NS_LOG_FUNCTION(this);
    m_uv = CreateObject<UniformRandomVariable>();
}

RedQueueDisc::~RedQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

void
RedQueueDisc::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_uv = nullptr;
    QueueDisc::DoDispose();
}

void
RedQueueDisc::SetAredAlpha(double alpha)
{
    NS_LOG_FUNCTION(this << alpha);
    m_aredAlpha = alpha;

    if (m_aredAlpha > 0.01)

    /* 
    * alpha <= 0.01  
    * [Ref: https://www.icir.org/floyd/papers/adaptiveRed.pdf] Section - 4.2
    */

    {
        NS_LOG_WARN("Alpha value is above the recommended bound!");
    }
}

double
RedQueueDisc::GetAredAlpha()
{
    NS_LOG_FUNCTION(this);
    return m_aredAlpha;
}

void
RedQueueDisc::SetAredBeta(double beta)
{
    NS_LOG_FUNCTION(this << beta);
    m_aredBeta = beta;

    if (m_aredBeta < 0.83)

    /* 
    * beta  >= 0.83 
    * [Ref: https://www.icir.org/floyd/papers/adaptiveRed.pdf] Section - 4.2
    */

    {
        NS_LOG_WARN("Beta value is below the recommended bound!");
    }
}

double
RedQueueDisc::GetAredBeta()
{
    NS_LOG_FUNCTION(this);
    return m_aredBeta;
}

void
RedQueueDisc::SetFengAdaptiveA(double a)
{
    NS_LOG_FUNCTION(this << a);
    m_fengAlpha = a;

    if (m_fengAlpha != 3)

    /* 
    * fengAlpha is set to 3
    * [Ref: https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=752150] Section - IV
    */

    {
        NS_LOG_WARN("Alpha value does not follow the recommendations!");
    }
}

double
RedQueueDisc::GetFengAdaptiveA()
{
    NS_LOG_FUNCTION(this);
    return m_fengAlpha;
}

void
RedQueueDisc::SetFengAdaptiveB(double b)
{
    NS_LOG_FUNCTION(this << b);
    m_fengBeta = b;

    if (m_fengBeta != 2)

    /* 
    * fengBeta is set to 2
    * [Ref: https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=752150] Section - IV
    */

    {
        NS_LOG_WARN("Beta value does not follow the recommendations!");
    }
}

double
RedQueueDisc::GetFengAdaptiveB()
{
    NS_LOG_FUNCTION(this);
    return m_fengBeta;
}

void
RedQueueDisc::SetTh(double minTh, double maxTh)
{
    NS_LOG_FUNCTION(this << minTh << maxTh);
    NS_ASSERT(minTh <= maxTh);
    m_minTh = minTh;
    m_maxTh = maxTh;
}

int64_t
RedQueueDisc::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uv->SetStream(stream);
    return 1;
}

bool
RedQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    uint32_t Current_queue_len = GetInternalQueue(0)->GetCurrentSize().GetValue();

    // simulate number of packets arrival during idle period
    uint32_t m = 0;

    if (m_isIdle == 1)
    {
        NS_LOG_DEBUG("RED Queue Disc is idle.");
        Time now = Simulator::Now();

        if (m_dropCautionMode == 3)
        {
            double ptc = m_ptc * m_meanPktSize / m_idleQPktSize;
            m = uint32_t(ptc * (now - m_idleTime).GetSeconds());
        }
        else
        {
            m = uint32_t(m_ptc * (now - m_idleTime).GetSeconds());
        }

        m_isIdle = 0;
    }

    m_qAvg = Estimator(Current_queue_len, m + 1, m_qAvg, m_wQ);

    NS_LOG_DEBUG("\t bytesInQueue  " << GetInternalQueue(0)->GetNBytes() << "\tQavg " << m_qAvg);
    NS_LOG_DEBUG("\t packetsInQueue  " << GetInternalQueue(0)->GetNPackets() << "\tQavg "<< m_qAvg);

    m_count++;
    m_countBytes += item->GetSize();

    uint32_t dropType = DTYPE_NONE;
    if (m_qAvg >= m_minTh && Current_queue_len > 1)
    {
        if ((!m_isGentle && m_qAvg >= m_maxTh) || (m_isGentle && m_qAvg >= 2 * m_maxTh))
        {
            NS_LOG_DEBUG("adding DROP FORCED MARK");
            dropType = DTYPE_FORCED;
        }
        else if (m_aboveMinTh == 0)
        {
            /*
             * The average queue size has just crossed the
             * threshold from below to above m_minTh, or
             * from above m_minTh with an empty queue to
             * above m_minTh with a nonempty queue.
             */
            m_count = 1;
            m_countBytes = item->GetSize();
            m_aboveMinTh = 1;
        }
        else if (DropEarly(item, Current_queue_len))
        {
            NS_LOG_LOGIC("DropEarly returns 1");
            dropType = DTYPE_UNFORCED;
        }
    }
    else
    {
        // No packets are being dropped
        m_Pa = 0.0;
        m_aboveMinTh = 0;
    }

    if (dropType == DTYPE_UNFORCED)
    {
        if (!m_useEcn || !Mark(item, UNFORCED_MARK))
        {
            NS_LOG_DEBUG("\t Dropping due to Prob Mark " << m_qAvg);
            DropBeforeEnqueue(item, UNFORCED_DROP);
            return false;
        }
        NS_LOG_DEBUG("\t Marking due to Prob Mark " << m_qAvg);
    }
    else if (dropType == DTYPE_FORCED)
    {
        if (m_useHardDrop || !m_useEcn || !Mark(item, FORCED_MARK))
        {
            NS_LOG_DEBUG("\t Dropping due to Hard Mark " << m_qAvg);
            DropBeforeEnqueue(item, FORCED_DROP);
            if (m_isNs1Compat)
            {
                m_count = 0;
                m_countBytes = 0;
            }
            return false;
        }
        NS_LOG_DEBUG("\t Marking due to Hard Mark " << m_qAvg);
    }

    bool retval = GetInternalQueue(0)->Enqueue(item);

    // If Queue::Enqueue fails, QueueDisc::DropBeforeEnqueue is called by the
    // internal queue because QueueDisc::AddInternalQueue sets the trace callback

    NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
    NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

    return retval;
}

/*
 * Note: if the link bandwidth changes in the course of the
 * simulation, the bandwidth-dependent RED parameters do not change.
 * This should be fixed, but it would require some extra parameters,
 * and didn't seem worth the trouble...
 */
void
RedQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Initializing RED params.");

    m_dropCautionMode = 0;
    m_ptc = m_linkBandwidth.GetBitRate() / (8.0 * m_meanPktSize);

    if (m_isARED)
    {
        // Set m_minTh, m_maxTh and m_wQ to zero for automatic setting
        m_minTh = 0;
        m_maxTh = 0;
        m_wQ = 0;

        // Turn on m_isAdaptMaxP to adapt m_curMaxP
        m_isAdaptMaxP = true;
    }

    if (m_isFengAdaptive)
    {
        // Initialize m_fengStatus
        m_fengStatus = Above;
    }

    if (m_minTh == 0 && m_maxTh == 0)
    {
        m_minTh = 5.0;

        /* 
        * set m_minTh to max(m_minTh, targetQueue/2.0)
        * [Ref: http://www.icir.org/floyd/papers/adaptiveRed.pdf] Section - 6
        */

        double targetQueue = m_targetQueueDelay.GetSeconds() * m_ptc;

        if (m_minTh < targetQueue / 2.0)
        {
            m_minTh = targetQueue / 2.0;
        }
        if (GetMaxQueueSize().GetUnit() == QueueSizeUnit::BYTES)
        {
            m_minTh = m_minTh * m_meanPktSize;
        }

        /* 
        * set m_maxTh to three times m_minTh
        * [Ref: http://www.icir.org/floyd/papers/adaptiveRed.pdf] Section - 6
        */

        m_maxTh = 3 * m_minTh;
    }

    NS_ASSERT(m_minTh <= m_maxTh);

    m_qAvg = 0.0;
    m_count = 0;
    m_countBytes = 0;
    m_aboveMinTh = 0;
    m_isIdle = 1;

    double th_diff = (m_maxTh - m_minTh);
    if (th_diff == 0)
    {
        th_diff = 1.0;
    }
    m_vA = 1.0 / th_diff;
    m_curMaxP = 1.0 / m_lInterm;
    m_vB = -m_minTh / th_diff;

    if (m_isGentle)
    {
        m_vC = (1.0 - m_curMaxP) / m_maxTh;
        m_vD = 2.0 * m_curMaxP - 1.0;
    }
    m_idleTime = NanoSeconds(0);

    /*
     * If m_wQ=0, set it to a reasonable value of 1-exp(-1/C)
     * This corresponds to choosing m_wQ to be of that value for
     * which the packet time constant -1/ln(1-m)qW) per default RTT
     * of 100ms is an order of magnitude more than the link capacity, C.
     *
     * If m_wQ=-1, then the queue weight is set to be a function of
     * the bandwidth and the link propagation delay.  In particular,
     * the default RTT is assumed to be three times the link delay and
     * transmission delay, if this gives a default RTT greater than 100 ms.
     *
     * If m_wQ=-2, set it to a reasonable value of 1-exp(-10/C).
     */
    if (m_wQ == 0.0)
    {
        m_wQ = 1.0 - std::exp(-1.0 / m_ptc);
    }
    else if (m_wQ == -1.0)
    {
        double rtt = 3.0 * (m_linkDelay.GetSeconds() + 1.0 / m_ptc);

        if (rtt < 0.1)
        {
            rtt = 0.1;
        }
        m_wQ = 1.0 - std::exp(-1.0 / (10 * rtt * m_ptc));
    }
    else if (m_wQ == -2.0)
    {
        m_wQ = 1.0 - std::exp(-10.0 / m_ptc);
    }

    if (m_lbCurMaxP == 0)
    {
        m_lbCurMaxP = 0.01;
        // Set lbCurMaxP to at most 1/W, where W is the delay-bandwidth
        // product in packets for a connection.
        // So W = m_linkBandwidth.GetBitRate () / (8.0 * m_meanPktSize * m_rtt.GetSeconds())
        double bottom1 = (8.0 * m_meanPktSize * m_rtt.GetSeconds()) / m_linkBandwidth.GetBitRate();
        if (bottom1 < m_lbCurMaxP)
        {
            m_lbCurMaxP = bottom1;
        }
    }

    NS_LOG_DEBUG("\tm_delay " << m_linkDelay.GetSeconds() << "; m_isWait " << m_isWait << "; m_wQ "<< m_wQ << "; m_ptc " << m_ptc << "; m_minTh " << m_minTh<< "; m_maxTh " << m_maxTh << "; m_isGentle " << m_isGentle<< "; th_diff " << th_diff << "; lInterm " << m_lInterm << "; va "<< m_vA << "; cur_max_p " << m_curMaxP << "; v_b " << m_vB<< "; m_vC " << m_vC << "; m_vD " << m_vD);
}

// Updating m_curMaxP, following the pseudocode
// from: A Self-Configuring RED Gateway, INFOCOMM '99.
// They recommend m_fengAlpha = 3, and m_fengBeta = 2.
void
RedQueueDisc::UpdateMaxPFeng(double newAvg)
{
    NS_LOG_FUNCTION(this << newAvg);

    if (m_minTh < newAvg && newAvg < m_maxTh)
    {
        m_fengStatus = Between;
    }
    else if (newAvg < m_minTh && m_fengStatus != Below)
    {
        m_fengStatus = Below;
        m_curMaxP = m_curMaxP / m_fengAlpha;
    }
    else if (newAvg > m_maxTh && m_fengStatus != Above)
    {
        m_fengStatus = Above;
        m_curMaxP = m_curMaxP * m_fengBeta;
    }
}

// Update m_curMaxP to keep the average queue length within the target range.
void
RedQueueDisc::UpdateMaxP(double newAvg)
{
    NS_LOG_FUNCTION(this << newAvg);

    Time now = Simulator::Now();
    double m_threshMargin = 0.4 * (m_maxTh - m_minTh);
    // AIMD rule to keep target Q~1/2(m_minTh + m_maxTh)
    if (newAvg < m_minTh + m_threshMargin && m_curMaxP > m_lbCurMaxP)
    {
        // we should increase the average queue size, so decrease m_curMaxP
        m_curMaxP = m_curMaxP * m_aredBeta;
        m_lastSet_currMaxP_At = now;
    }
    else if (newAvg > m_maxTh - m_threshMargin && m_ubCurMaxP > m_curMaxP)
    {
        // we should decrease the average queue size, so increase m_curMaxP
        double alpha = m_aredAlpha;
        if (alpha > 0.25 * m_curMaxP)
        {
            alpha = 0.25 * m_curMaxP;
        }
        m_curMaxP = m_curMaxP + alpha;
        m_lastSet_currMaxP_At = now;
    }
}

// Compute the average queue size
double
RedQueueDisc::Estimator(uint32_t Current_queue_len, uint32_t m, double oldAvg, double qW)
{
    NS_LOG_FUNCTION(this << Current_queue_len << m << oldAvg << qW);

    double newAvg = oldAvg * std::pow(1.0 - qW, m);
    newAvg += qW * Current_queue_len;

    Time now = Simulator::Now();
    if (m_isAdaptMaxP && now > m_lastSet_currMaxP_At + m_interval)
    {
        UpdateMaxP(newAvg);
    }
    else if (m_isFengAdaptive)
    {
        UpdateMaxPFeng(newAvg); // Update m_curMaxP in MIMD fashion.
    }

    return newAvg;
}

// Check if packet p needs to be dropped due to probability mark
bool
RedQueueDisc::DropEarly(Ptr<QueueDiscItem> item, uint32_t qSize)
{
    NS_LOG_FUNCTION(this << item << qSize);

    double prob1 = CalculatePNew();
    m_Pa = ModifyP(prob1, item->GetSize());

    // Drop probability is computed, pick random number and act
    if (m_dropCautionMode == 1)
    {
        /*
         * Don't drop/mark if the instantaneous queue is much below the average.
         * For experimental purposes only.
         * pkts: the number of packets arriving in 50 ms
         */
        double pkts = m_ptc * 0.05;
        double fraction = std::pow((1 - m_wQ), pkts);

        if ((double)qSize < fraction * m_qAvg)
        {
            // Queue could have been empty for 0.05 seconds
            return false;
        }
    }

    double R = m_uv->GetValue();

    if (m_dropCautionMode == 2)
    {
        /*
         * Decrease the drop probability if the instantaneous
         * queue is much below the average.
         * For experimental purposes only.
         * pkts: the number of packets arriving in 50 ms
         */
        double pkts = m_ptc * 0.05;
        double fraction = std::pow((1 - m_wQ), pkts);
        double ratio = qSize / (fraction * m_qAvg);

        if (ratio < 1.0)
        {
            R *= 1.0 / ratio;
        }
    }

    if (R <= m_Pa)
    {
        NS_LOG_LOGIC("R <= m_Pa; R " << R << "; m_Pa " << m_Pa);

        // DROP or MARK
        m_count = 0;
        m_countBytes = 0;
        /// @todo Implement set bit to mark

        return true; // drop
    }

    return false; // no drop/mark
}

// Returns a probability using these function parameters for the DropEarly function
double
RedQueueDisc::CalculatePNew()
{
    NS_LOG_FUNCTION(this);
    double Pd;

    if (m_isGentle && m_qAvg >= m_maxTh)
    {
        // Pd ranges from m_curMaxP to 1 as the average queue
        // size ranges from m_maxTh to twice m_maxTh
        Pd = m_vC * m_qAvg + m_vD;
    }
    else if (!m_isGentle && m_qAvg >= m_maxTh)
    {
        /*
         * OLD: Pd continues to range linearly above m_curMaxP as
         * the average queue size ranges above m_maxTh.
         * NEW: Pd is set to 1.0
         */
        Pd = 1.0;
    }
    else
    {
        /*
         * Pd ranges from 0 to m_curMaxP as the average queue size ranges from
         * m_minTh to m_maxTh
         */
        Pd = m_vA * m_qAvg + m_vB;

        if (m_isNonlinear)
        {
            Pd *= Pd * 1.5;

        /* 
        * set Pd in Non-linear RED to 1.5*Pd
        * [Ref: https://www.sciencedirect.com/science/article/pii/S1389128606000879?via%3Dihub] Section - 3.1
        */

        }

        Pd *= m_curMaxP;
    }

    if (Pd > 1.0)
    {
        Pd = 1.0;
    }

    return Pd;
}

// Returns a probability using these function parameters for the DropEarly function
double
RedQueueDisc::ModifyP(double Pd, uint32_t size)
{
    NS_LOG_FUNCTION(this << Pd << size);
    auto count1 = (double)m_count;

    if (GetMaxQueueSize().GetUnit() == QueueSizeUnit::BYTES)
    {
        count1 = (double)(m_countBytes / m_meanPktSize);
    }

    if (m_isWait)
    {
        if (count1 * Pd < 1.0)
        {
            Pd = 0.0;
        }
        else if (count1 * Pd < 2.0)
        {
            Pd /= (2.0 - count1 * Pd);
        }
        else
        {
            Pd = 1.0;
        }
    }
    else
    {
        if (count1 * Pd < 1.0)
        {
            Pd /= (1.0 - count1 * Pd);
        }
        else
        {
            Pd = 1.0;
        }
    }

    if ((GetMaxQueueSize().GetUnit() == QueueSizeUnit::BYTES) && (Pd < 1.0))
    {
        Pd = (Pd * size) / m_meanPktSize;
    }

    if (Pd > 1.0)
    {
        Pd = 1.0;
    }

    return Pd;
}

Ptr<QueueDiscItem>
RedQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    if (GetInternalQueue(0)->IsEmpty())
    {
        NS_LOG_LOGIC("Queue empty");
        m_isIdle = 1;
        m_idleTime = Simulator::Now();

        return nullptr;
    }
    else
    {
        m_isIdle = 0;
        Ptr<QueueDiscItem> item = GetInternalQueue(0)->Dequeue();

        NS_LOG_LOGIC("Popped " << item);

        NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
        NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

        return item;
    }
}

Ptr<const QueueDiscItem>
RedQueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);
    if (GetInternalQueue(0)->IsEmpty())
    {
        NS_LOG_LOGIC("Queue empty");
        return nullptr;
    }

    Ptr<const QueueDiscItem> item = GetInternalQueue(0)->Peek();

    NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
    NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

    return item;
}

bool
RedQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNQueueDiscClasses() > 0)
    {
        NS_LOG_ERROR("RedQueueDisc cannot have classes");
        return false;
    }

    if (GetNPacketFilters() > 0)
    {
        NS_LOG_ERROR("RedQueueDisc cannot have packet filters");
        return false;
    }

    if (GetNInternalQueues() == 0)
    {
        // add a DropTail queue
        AddInternalQueue(
            CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>>("MaxQueueSize",QueueSizeValue(GetMaxQueueSize())));
    }

    if (GetNInternalQueues() != 1)
    {
        NS_LOG_ERROR("RedQueueDisc needs 1 internal queue");
        return false;
    }

    if ((m_isARED || m_isAdaptMaxP) && m_isFengAdaptive)
    {
        NS_LOG_ERROR("m_isAdaptMaxP and m_isFengAdaptive cannot be simultaneously true");
    }

    return true;
}

} // namespace ns3
    
