/*
 * Copyright (c) 2010 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/command-line.h"
#include "ns3/double.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"

#include <iostream>

/**
 * @file
 * @ingroup core-examples
 * @ingroup simulator
 * Example program demonstrating use of various Schedule functions.
 */

using namespace ns3;

namespace
{

/** Simple model object to illustrate event handling. */
class MyModel
{
  public:
    /** Start model execution by scheduling a HandleEvent. */
    void Start();

  private:
    /**
     *  Simple event handler.
     *
     * @param [in] eventValue Event argument.
     */
    void HandleEvent(double eventValue);
};

void
MyModel::Start()
{
    Simulator::Schedule(Seconds(10), &MyModel::HandleEvent, this, Simulator::Now().GetSeconds());
}

void
MyModel::HandleEvent(double value)
{
    std::cout << "Member method received event at " << Simulator::Now().GetSeconds()
              << "s started at " << value << "s" << std::endl;
}

/**
 * Simple function event handler which Starts a MyModel object.
 *
 * @param [in] model The MyModel object to start.
 */
void
ExampleFunction(MyModel* model)
{
    std::cout << "ExampleFunction received event at " << Simulator::Now().GetSeconds() << "s"
              << std::endl;
    model->Start();
}

/**
 * Simple function event handler; this function is called randomly.
 */
void
RandomFunction()
{
    std::cout << "RandomFunction received event at " << Simulator::Now().GetSeconds() << "s"
              << std::endl;
}

/** Simple function event handler; the corresponding event is cancelled. */
void
CancelledEvent()
{
    std::cout << "I should never be called... " << std::endl;
}

} // unnamed namespace

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    MyModel model;
    Ptr<UniformRandomVariable> v = CreateObject<UniformRandomVariable>();
    v->SetAttribute("Min", DoubleValue(10));
    v->SetAttribute("Max", DoubleValue(20));

    Simulator::Schedule(Seconds(10), &ExampleFunction, &model);

    Simulator::Schedule(Seconds(v->GetValue()), &RandomFunction);

    EventId id = Simulator::Schedule(Seconds(30), &CancelledEvent);
    Simulator::Cancel(id);

    Simulator::Schedule(Seconds(25), []() {
        std::cout << "Code within a lambda expression at time " << Simulator::Now().As(Time::S)
                  << std::endl;
    });

    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
