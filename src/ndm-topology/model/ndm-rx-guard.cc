/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ndm-rx-guard.h"

#include "ndm-link.h"
#include "ndm-link-loss-model.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NdmRxGuard");

NS_OBJECT_ENSURE_REGISTERED(NdmRxGuard);

TypeId
NdmRxGuard::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NdmRxGuard").SetBase<ErrorModel>().SetGroupName("NdmTopology")
            .AddConstructor<NdmRxGuard>();
    return tid;
}

NdmRxGuard::NdmRxGuard() = default;

NdmRxGuard::~NdmRxGuard() = default;

void
NdmRxGuard::SetLink(Ptr<NdmLink> link)
{
    m_link = link;
}

void
NdmRxGuard::SetLossModel(Ptr<NdmLinkLossModel> loss)
{
    m_loss = loss;
}

bool
NdmRxGuard::IsCorrupt(Ptr<const Packet> packet)
{
    if (m_link != nullptr && m_link->IsDown())
    {
        const InFlightPolicy policy = m_link->GetPolicy();
        bool lost;
        if (policy == InFlightPolicy::DELIVER_THEN_DROP)
        {
            // Launch time = receive time minus channel delay.
            const Time launch = Simulator::Now() - m_link->GetDelay();
            lost = launch >= m_link->GetTDown();
        }
        else
        {
            lost = true;
        }
        if (lost)
        {
            NS_LOG_INFO("NdmRxGuard: in-flight packet dropped on link "
                        << m_link->GetId().index);
            m_link->CountRxLostInFlight();
            return true;
        }
        return false; // delivered: launched before the link died
    }

    if (m_loss != nullptr)
    {
        return m_loss->IsCorrupt(packet);
    }
    return false;
}

} // namespace ns3
