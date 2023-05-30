/*
 * Copyright (c) 2023 Lawrence Livermore National Laboratory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

/**
 * \file
 * \ingroup core-examples
 * \ingroup logging
 * Example program illustrating the various logging functions.
 */

/** File-local context string */
#define NS_LOG_APPEND_CONTEXT                                                                      \
    {                                                                                              \
        std::clog << "(local context) ";                                                           \
    }

#include "ns3/core-module.h"
#include "ns3/network-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LogExample");

/**
 * Unnamed namespace for log-example.cc
 */
namespace
{

/** A free function with logging. */
void
FreeEvent()
{
    NS_LOG_FUNCTION_NOARGS();

    NS_LOG_ERROR("FreeEvent: error msg");
    NS_LOG_WARN("FreeEvent: warning msg");
    NS_LOG_INFO("FreeEvent: info msg");
    NS_LOG_LOGIC("FreeEvent: logic msg");
    NS_LOG_DEBUG("FreeEvent: debug msg");
}

/** Simple object to aggregate to a node.
 * This helps demonstrate the logging node prefix.
 */

class MyEventObject : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid =
            TypeId("MyEventObject").SetParent<Object>().AddConstructor<MyEventObject>();
        return tid;
    }

    /** Constructor. */
    MyEventObject()
    {
        NS_LOG_FUNCTION(this);
    }

    /** Destructor. */
    ~MyEventObject() override
    {
        NS_LOG_FUNCTION(this);
    }

    /** Class member function with logging. */
    void Event()
    {
        NS_LOG_FUNCTION(this);

        NS_LOG_ERROR("MyEventObject:Event: error msg");
        NS_LOG_WARN("MyEventObject:Event: warning msg");
        NS_LOG_INFO("MyEventObject:Event: info msg");
        NS_LOG_LOGIC("MyEventObject:Event: logic msg");
        NS_LOG_DEBUG("MyEventObject:Event: debug msg");
    }

}; // MyEventObject

NS_OBJECT_ENSURE_REGISTERED(MyEventObject);

} // Unnamed namespace

int
main(int argc, char** argv)
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    NS_LOG_DEBUG("Creating a Node");
    auto node = CreateObject<Node>();

    NS_LOG_DEBUG("Creating MyEventObject");
    auto myObj = CreateObject<MyEventObject>();

    NS_LOG_DEBUG("Aggregating MyEventObject to Node");
    node->AggregateObject(myObj);

    NS_LOG_INFO("Scheduling the MyEventObject::Event with node context");
    Simulator::ScheduleWithContext(node->GetId(), Seconds(3), &MyEventObject::Event, &(*myObj));

    NS_LOG_INFO("Scheduling FreeEvent");
    Simulator::Schedule(Seconds(5), FreeEvent);

    NS_LOG_DEBUG("Starting run...");
    Simulator::Run();
    NS_LOG_DEBUG("... run complete");
    Simulator::Destroy();
    NS_LOG_DEBUG("Goodbye");

    return 0;
}
