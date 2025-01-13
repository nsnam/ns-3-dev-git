/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "application-helper.h"

#include "ns3/data-rate.h"
#include "ns3/names.h"
#include "ns3/string.h"

namespace ns3
{

ApplicationHelper::ApplicationHelper(TypeId typeId)
{
    SetTypeId(typeId);
}

ApplicationHelper::ApplicationHelper(const std::string& typeId)
{
    SetTypeId(typeId);
}

void
ApplicationHelper::SetTypeId(TypeId typeId)
{
    m_factory.SetTypeId(typeId);
}

void
ApplicationHelper::SetTypeId(const std::string& typeId)
{
    m_factory.SetTypeId(typeId);
}

void
ApplicationHelper::SetAttribute(const std::string& name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

ApplicationContainer
ApplicationHelper::Install(Ptr<Node> node)
{
    return ApplicationContainer(DoInstall(node));
}

ApplicationContainer
ApplicationHelper::Install(const std::string& nodeName)
{
    auto node = Names::Find<Node>(nodeName);
    NS_ABORT_MSG_IF(!node, "Node " << nodeName << " does not exist");
    return ApplicationContainer(DoInstall(node));
}

ApplicationContainer
ApplicationHelper::Install(NodeContainer c)
{
    ApplicationContainer apps;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        apps.Add(DoInstall(*i));
    }
    return apps;
}

Ptr<Application>
ApplicationHelper::DoInstall(Ptr<Node> node)
{
    NS_ABORT_MSG_IF(!node, "Node does not exist");
    auto app = m_factory.Create<Application>();
    node->AddApplication(app);
    return app;
}

int64_t
ApplicationHelper::AssignStreams(NodeContainer c, int64_t stream)
{
    NS_ABORT_MSG_IF(!m_factory.IsTypeIdSet(), "Type ID not set");
    auto currentStream = stream;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        auto node = (*i);
        for (uint32_t j = 0; j < node->GetNApplications(); ++j)
        {
            if (auto app = node->GetApplication(j);
                app->GetInstanceTypeId() == m_factory.GetTypeId())
            {
                currentStream += app->AssignStreams(currentStream);
            }
        }
    }
    return (currentStream - stream);
}

int64_t
ApplicationHelper::AssignStreamsToAllApps(NodeContainer c, int64_t stream)
{
    auto currentStream = stream;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        auto node = (*i);
        for (uint32_t j = 0; j < node->GetNApplications(); ++j)
        {
            currentStream += node->GetApplication(j)->AssignStreams(currentStream);
        }
    }
    return (currentStream - stream);
}

} // namespace ns3
