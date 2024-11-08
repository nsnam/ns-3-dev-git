/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/object.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-value.h"
#include "ns3/uinteger.h"

#include <iostream>

using namespace ns3;

/**
 * Tutorial 4 - a simple Object to show how to hook a trace.
 */
class MyObject : public Object
{
  public:
    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("MyObject")
                                .SetParent<Object>()
                                .SetGroupName("Tutorial")
                                .AddConstructor<MyObject>()
                                .AddTraceSource("MyInteger",
                                                "An integer value to trace.",
                                                MakeTraceSourceAccessor(&MyObject::m_myInt),
                                                "ns3::TracedValueCallback::Int32");
        return tid;
    }

    MyObject()
    {
    }

    TracedValue<int32_t> m_myInt; //!< The traced value.
};

void
IntTrace(int32_t oldValue, int32_t newValue)
{
    std::cout << "Traced " << oldValue << " to " << newValue << std::endl;
}

int
main(int argc, char* argv[])
{
    Ptr<MyObject> myObject = CreateObject<MyObject>();
    myObject->TraceConnectWithoutContext("MyInteger", MakeCallback(&IntTrace));

    myObject->m_myInt = 1234;

    return 0;
}
