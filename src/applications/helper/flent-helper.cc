/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2022 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is adapted from bulk-send-helper.cc.
 *
 * Author: Ameya Deshpande <ameyanrd@outlook.com>
 */

#include "flent-helper.h"

#include "ns3/names.h"
#include "ns3/string.h"

namespace ns3
{

FlentHelper::FlentHelper(std::string testname, Address address)
{
    m_factory.SetTypeId("ns3::FlentApplication");
    m_factory.Set("TestName", StringValue(testname));
    m_factory.Set("HostAddress", AddressValue(address));
}

void
FlentHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

void
FlentHelper::SetTestStartTime(Time start)
{
    m_startTime = start;

    m_factory.Set("StartTime", TimeValue(m_startTime));
}

void
FlentHelper::SetTestLength(Time length)
{
    m_length = length;

    m_factory.Set("Length", TimeValue(length));
}

Time
FlentHelper::GetStopTime() const
{
    return m_startTime + m_length + Seconds(10);
}

ApplicationContainer
FlentHelper::Install(Ptr<Node> node) const
{
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer
FlentHelper::Install(std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return ApplicationContainer(InstallPriv(node));
}

Ptr<Application>
FlentHelper::InstallPriv(Ptr<Node> node) const
{
    Ptr<Application> app = m_factory.Create<Application>();
    node->AddApplication(app);

    return app;
}

} // namespace ns3
