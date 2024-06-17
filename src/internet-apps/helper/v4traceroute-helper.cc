/*
 * Copyright (c) 2019 Ritsumeikan University, Shiga, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 * Author: Alberto Gallegos Ramonet <ramonet@fc.ritsumei.ac.jp>
 *
 */

#include "v4traceroute-helper.h"

#include "ns3/output-stream-wrapper.h"
#include "ns3/v4traceroute.h"

namespace ns3
{

V4TraceRouteHelper::V4TraceRouteHelper(const Ipv4Address& remote)
    : ApplicationHelper("ns3::V4TraceRoute")
{
    m_factory.Set("Remote", Ipv4AddressValue(remote));
}

void
V4TraceRouteHelper::PrintTraceRouteAt(Ptr<Node> node, Ptr<OutputStreamWrapper> stream)
{
    for (uint32_t i = 0; i < node->GetNApplications(); ++i)
    {
        if (auto trace = node->GetApplication(i)->GetObject<V4TraceRoute>())
        {
            *stream->GetStream() << "Tracing Route from Node " << node->GetId() << "\n";
            trace->Print(stream);
            return;
        }
    }
    NS_ASSERT_MSG(false, "No V4TraceRoute application found in node " << node->GetId());
}

} // namespace ns3
