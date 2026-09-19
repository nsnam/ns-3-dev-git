/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/enum.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "ns3/ndm-link-loss-model.h"

#include "ns3/packet.h"
#include "ns3/rng-stream.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(NdmLinkLossModel);

TypeId
NdmLinkLossModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NdmLinkLossModel")
            .SetParent<ErrorModel>()
            .SetGroupName("NdmTopology")
            .AddConstructor<NdmLinkLossModel>()
            .AddAttribute("Mode",
                          "Loss mode: NONE, BERN (Bernoulli), BURST (burst runs)",
                          EnumValue<Mode>(Mode::NONE),
                          MakeEnumAccessor(&NdmLinkLossModel::m_mode),
                          MakeEnumChecker(Mode::NONE, "NONE", Mode::BERN, "BERN", Mode::BURST,
                                          "BURST"))
            .AddAttribute("LossProbability",
                          "Per-packet (BERN) or burst-entry (BURST) loss probability",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&NdmLinkLossModel::m_lossProbability),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("BurstLength",
                          "Number of packets dropped per burst (BURST mode)",
                          UintegerValue(1),
                          MakeUintegerAccessor(&NdmLinkLossModel::m_burstLen),
                          MakeUintegerChecker<uint32_t>(1));
    return tid;
}

NdmLinkLossModel::NdmLinkLossModel() = default;

NdmLinkLossModel::~NdmLinkLossModel() = default;

void
NdmLinkLossModel::SetRng(std::unique_ptr<RngStream> rng)
{
    NS_ASSERT_MSG(rng, "NdmLinkLossModel: rng required for stochastic modes");
    m_rng = std::move(rng);
}

bool
NdmLinkLossModel::DoCorrupt(Ptr<Packet> p)
{
    (void)p; // loss is a drop, not bit flips
    switch (m_mode)
    {
        case Mode::NONE:
            return false;
        case Mode::BERN:
            return m_rng->RandU01() < m_lossProbability;
        case Mode::BURST:
            if (m_burstRemaining > 0)
            {
                m_burstRemaining--;
                return true;
            }
            if (m_rng->RandU01() < m_lossProbability)
            {
                m_burstRemaining = m_burstLen - 1;
                return true;
            }
            return false;
    }
    return false;
}

NdmLinkLossModel::Mode
NdmLinkLossModel::GetMode() const
{
    return m_mode;
}

void
NdmLinkLossModel::DoReset()
{
    m_burstRemaining = 0;
}

} // namespace ns3
