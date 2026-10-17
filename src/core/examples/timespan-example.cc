/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

/**
 * @file
 * Example to demonstrate that both a TimeSpan attribute and a TimeSpan variable can be set
 * via the CommandLine system.
 *
 * The example expects that the command line arguments are such that the TimeSpan attribute is set
 * to {100ms,200ms} and the TimeSpan variable is set to {300ms,400ms}. If that is not the case, a
 * non-zero value is returned.
 */

#include "ns3/core-module.h"

NS_LOG_COMPONENT_DEFINE("TimeSpanExample");

namespace ns3
{

/** Object containing an attribute represented by a TimeSpan. */
class ObjectWithTimeSpanAttribute : public Object
{
  public:
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TimeSpan m_timeSpan; //!< TimeSpan field
};

TypeId
ObjectWithTimeSpanAttribute::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ObjectWithTimeSpanAttribute")
            .SetParent<Object>()
            .SetGroupName("Test")
            .AddConstructor<ObjectWithTimeSpanAttribute>()
            .AddAttribute(
                "TimeSpanAttr",
                "Attribute whose value is represented by a TimeSpan",
                TimeSpanValue(TimeSpan{MilliSeconds(500), Time{"1s"}}),
                MakeStructAccessor<TimeSpanValue>(&ObjectWithTimeSpanAttribute::m_timeSpan),
                MakeStructChecker<TimeSpanValue>(MakeTimeChecker(), MakeTimeChecker()));
    return tid;
}

NS_OBJECT_ENSURE_REGISTERED(ObjectWithTimeSpanAttribute);

} // namespace ns3

using namespace ns3;

int
main(int argc, char* argv[])
{
    const TimeSpan expectedTsAttr("{100ms, 200ms}");
    const TimeSpan expectedTsVar("{300ms, 400ms}");

    TimeSpan tsVar;

    CommandLine cmd(__FILE__);
    cmd.AddValue("tsAttr", "ns3::ObjectWithTimeSpanAttribute::TimeSpanAttr");
    cmd.AddValue("tsVar", "Example to set a TimeSpan variable", tsVar);
    cmd.Parse(argc, argv);

    auto obj = CreateObject<ObjectWithTimeSpanAttribute>();

    if (obj->m_timeSpan != expectedTsAttr)
    {
        std::cerr << "Unexpected TimeSpan attribute value: " << obj->m_timeSpan << "\n";
        return -1;
    }

    if (tsVar != expectedTsVar)
    {
        std::cerr << "Unexpected TimeSpan variable value: " << tsVar << "\n";
        return -1;
    }

    return 0;
}
