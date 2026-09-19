/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmRoceDcQcn: rate-based DCQCN (Pansā et al., IEEE 802.1Qau /
 * "Congestion Control for Large-Scale RDMA Deployments") re-implemented
 * from the algorithm description (no-license HPCC/donor code is never
 * copied — R6). Parameter set is the published default set.
 *
 * Model (all state per QP; all timing via Simulator events — PPS-safe):
 *   alpha (congestion level, 0..1, init 0.5):
 *     - on CNP:            alpha = min(1, alpha + alpha*beta)
 *     - per slot w/o CNP:  alpha *= (1 - beta*Tslot/RTT)   (exponential decay)
 *   target rate (per slot, Tslot = 50us):
 *     - alpha >= 1:        r_target = r_min
 *     - alpha >= 0.5:      r_target = max(r*(1-beta), 0.5*r)
 *     - alpha <  0.5:      r_target = r + max(r/gain, onePacketPerRtt)
 *   rate movement: increases toward r_target are immediate; decreases are
 *   spread over the recovery period RP = RTT/4 (r decreases by
 *   (r - r_target)*Tslot/RP per slot), never below r_target.
 *   in-flight window (bytes) = r * RTT_est — the QP's send credit.
 */

#ifndef NDM_ROCE_DCQCN_H
#define NDM_ROCE_DCQCN_H

#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"

#include <cstdint>

namespace ns3
{

class NdmRoceDcQcn
{
  public:
    /// One congestion controller per sending QP.
    NdmRoceDcQcn();
    ~NdmRoceDcQcn();

    void SetLinkRate(DataRate linkRate); ///< r_max
    void SetRttEstimate(Time rtt);       ///< from QP ACK sampling
    Time GetRttEstimate() const
    {
        return m_rtt;
    }

    /// CNP received (rate-based reaction). No-op if alpha already 1.
    void OnCnp(Time now);

    /// Start the periodic slot loop (called once the QP is live).
    void Start(Time now);
    void Stop();

    DataRate GetRate() const
    {
        return m_rate;
    }
    /// Send credit in bytes: rate * RTT.
    uint64_t GetWindowBytes() const;

    double GetAlpha() const
    {
        return m_alpha;
    }

  private:
    void DoSlot();

    // Parameters (published DCQCN defaults).
    static constexpr double kBeta = 0.5;    ///< rate-decrease factor
    static constexpr double kGain = 10.0;   ///< multiplicative-increase divisor
    Time m_slot{NanoSeconds(50000)};        ///< T_slot = 50 us

    DataRate m_rMax{DataRate("10Gbps")};
    DataRate m_rMin{DataRate("1Mbps")};
    DataRate m_rate{DataRate("10Gbps")};
    DataRate m_target{DataRate("10Gbps")};

    Time m_rtt{MicroSeconds(10)};
    double m_alpha{0.5};
    bool m_cnpInSlot{false};

    EventId m_slotEvent;
    bool m_started{false};
};

} // namespace ns3

#endif // NDM_ROCE_DCQCN_H
