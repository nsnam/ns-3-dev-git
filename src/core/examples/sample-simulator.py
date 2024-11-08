#
# Copyright (c) 2010 INRIA
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
#
#
# Python version of sample-simulator.cc

## \file
#  \ingroup core-examples
#  \ingroup simulator
#  Python example program demonstrating use of various Schedule functions.

## Import ns-3
try:
    from ns import ns
except ModuleNotFoundError:
    raise SystemExit(
        "Error: ns3 Python module not found;"
        " Python bindings may not be enabled"
        " or your PYTHONPATH might not be properly configured"
    )


## Example function - triggered at a random time.
## \return None.
def RandomFunction():
    print("RandomFunction received event at", ns.Simulator.Now().GetSeconds(), "s")


## Example function - triggered if an event is canceled (should not be called).
## \return None.
def CancelledEvent():
    print("I should never be called... ")


ns.cppyy.cppdef(
    """
    #include "CPyCppyy/API.h"

    using namespace ns3;
    /** Simple model object to illustrate event handling. */
    class MyModel
    {
    public:
      /** Start model execution by scheduling a HandleEvent. */
      void Start ();

    private:
      /**
       *  Simple event handler.
       *
       * \param [in] eventValue Event argument.
       */
      void HandleEvent (double eventValue);
    };

    void
    MyModel::Start ()
    {
      Simulator::Schedule (Seconds (10.0),
                           &MyModel::HandleEvent,
                           this, Simulator::Now ().GetSeconds ());
    }
    void
    MyModel::HandleEvent (double value)
    {
      std::cout << "Member method received event at "
                << Simulator::Now ().GetSeconds ()
                << "s started at " << value << "s" << std::endl;
    }

    void ExampleFunction(MyModel& model){
      std::cout << "ExampleFunction received event at " << Simulator::Now().GetSeconds() << "s" << std::endl;
      model.Start();
    };

    EventImpl* ExampleFunctionEvent(MyModel& model)
    {
        return MakeEvent(&ExampleFunction, model);
    }

    void RandomFunctionCpp(MyModel& model) {
        CPyCppyy::Eval("RandomFunction()");
    }

    EventImpl* RandomFunctionEvent(MyModel& model)
    {
        return MakeEvent(&RandomFunctionCpp, model);
    }

    void CancelledFunctionCpp() {
        CPyCppyy::Eval("CancelledEvent()");
    }

    EventImpl* CancelledFunctionEvent()
    {
        return MakeEvent(&CancelledFunctionCpp);
    }
   """
)


def main(argv):
    cmd = ns.CommandLine(__file__)
    cmd.Parse(argv)

    model = ns.cppyy.gbl.MyModel()
    v = ns.CreateObject[ns.UniformRandomVariable]()
    v.SetAttribute("Min", ns.DoubleValue(10))
    v.SetAttribute("Max", ns.DoubleValue(20))

    ev = ns.cppyy.gbl.ExampleFunctionEvent(model)
    ns.Simulator.Schedule(ns.Seconds(10), ev)

    ev2 = ns.cppyy.gbl.RandomFunctionEvent(model)
    ns.Simulator.Schedule(ns.Seconds(v.GetValue()), ev2)

    ev3 = ns.cppyy.gbl.CancelledFunctionEvent()
    id = ns.Simulator.Schedule(ns.Seconds(30), ev3)
    ns.Simulator.Cancel(id)

    ns.Simulator.Run()

    ns.Simulator.Destroy()


if __name__ == "__main__":
    import sys

    main(sys.argv)
