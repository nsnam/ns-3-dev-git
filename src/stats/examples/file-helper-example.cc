/*
 * Copyright (c) 2013 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This is based on double-probe-example.cc.
 *
 * Author: Mitch Watrous (watrous@u.washington.edu)
 */

/*
 * This example is designed to show the main features of an
 * ns3::FileHelper.
 */

#include "ns3/core-module.h"
#include "ns3/stats-module.h"

#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FileHelperExample");

/**
 * This is our test object, an object that increments a counter according
 * to a Poisson process, and exports the (integer-valued) count as a
 * trace source.
 */
class Emitter : public Object
{
  public:
    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();
    Emitter();

  private:
    void DoInitialize() override;
    /// Counts how many time this function is called.
    void Count();

    TracedValue<uint32_t> m_counter;      //!< Simple counter
    Ptr<ExponentialRandomVariable> m_var; //!< Random number generator.
};

NS_OBJECT_ENSURE_REGISTERED(Emitter);

TypeId
Emitter::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Emitter")
                            .SetParent<Object>()
                            .SetGroupName("Stats")
                            .AddConstructor<Emitter>()
                            .AddTraceSource("Counter",
                                            "sample counter",
                                            MakeTraceSourceAccessor(&Emitter::m_counter),
                                            "ns3::TracedValueCallback::Double");
    return tid;
}

Emitter::Emitter()
{
    NS_LOG_FUNCTION(this);
    m_counter = 0;
    m_var = CreateObject<ExponentialRandomVariable>();
}

void
Emitter::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    Simulator::Schedule(Seconds(m_var->GetValue()), &Emitter::Count, this);
}

void
Emitter::Count()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Counting at " << Simulator::Now().As(Time::S));
    m_counter += 1.0;
    Simulator::Schedule(Seconds(m_var->GetValue()), &Emitter::Count, this);
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    //
    // This Emitter has a trace source object that will emit values at
    // random times.
    //

    Ptr<Emitter> emitter = CreateObject<Emitter>();
    Names::Add("/Names/Emitter", emitter);

    //
    // This file helper will be used to put data values into a file.
    //

    // Create the file helper.
    FileHelper fileHelper;

    // Configure the file to be written.
    fileHelper.ConfigureFile("file-helper-example", FileAggregator::FORMATTED);

    // Set the labels for this formatted output file.
    fileHelper.Set2dFormat("Time (Seconds) = %.3f\tCount = %.0f");

    // Write the values generated by the probe.  The path that we
    // provide helps to disambiguate the source of the trace.
    fileHelper.WriteProbe("ns3::Uinteger32Probe", "/Names/Emitter/Counter", "Output");

    // The Emitter object is not associated with an ns-3 node, so
    // it won't get started automatically, so we need to do this ourselves
    Simulator::Schedule(Seconds(0), &Emitter::Initialize, emitter);

    Simulator::Stop(Seconds(100));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
