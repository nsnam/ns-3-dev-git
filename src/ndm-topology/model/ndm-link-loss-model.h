/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmLinkLossModel: per-link stochastic loss for the lossy-Ethernet mode
 * (AGENTS.md D5: lossy = PFC off + queue drops, per-link loss optional).
 *
 * Modes:
 *  - NONE: never drops.
 *  - BERN: drop each packet independently with probability p
 *    (Bernoulli), drawn from the link's RngStream.
 *  - BURST: with probability p enter a burst that drops the next `burstLen`
 *    packets (simple burst-loss model; deterministic given the seed).
 *
 * Determinism (D7): all draws go through a Ptr<RngStream> that the topology
 * builder seeds per link from the cell seed — no wall clock, no global RNG.
 *
 * This model is consulted by NdmRxGuard only while the link is UP, so loss
 * and failure effects never interact ambiguously.
 */

#ifndef NDM_LINK_LOSS_MODEL_H
#define NDM_LINK_LOSS_MODEL_H

#include "ns3/error-model.h"

#include <cstdint>
#include <memory>

namespace ns3
{

class RngStream;

class NdmLinkLossModel : public ErrorModel
{
  public:
    static TypeId GetTypeId();

    NdmLinkLossModel();
    ~NdmLinkLossModel() override;

    enum class Mode : uint8_t
    {
        NONE,
        BERN,
        BURST
    };

    /// Set the per-link RNG (built by the topology builder from the cell
    /// seed: RngStream(seed + linkIndex, 0, 0)). Must be set before the
    /// first check in a stochastic mode.
    void SetRng(std::unique_ptr<RngStream> rng);

    Mode GetMode() const;

  private:
    /// ErrorModel override: apply the loss model to one packet.
    bool DoCorrupt(Ptr<Packet> p) override;
    /// ErrorModel override: reset model state (keeps the RNG stream state).
    void DoReset() override;
    Mode m_mode{Mode::NONE};
    double m_lossProbability{0.0};
    uint32_t m_burstLen{1};
    uint32_t m_burstRemaining{0};
    std::unique_ptr<RngStream> m_rng;
};

} // namespace ns3

#endif // NDM_LINK_LOSS_MODEL_H
