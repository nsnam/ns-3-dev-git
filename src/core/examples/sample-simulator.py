#
# Copyright (c) 2010 INRIA
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
    ns.Simulator.Schedule(ns.Seconds(10.0), ev)

    ev2 = ns.cppyy.gbl.RandomFunctionEvent(model)
    ns.Simulator.Schedule(ns.Seconds(v.GetValue()), ev2)

    ev3 = ns.cppyy.gbl.CancelledFunctionEvent()
    id = ns.Simulator.Schedule(ns.Seconds(30.0), ev3)
    ns.Simulator.Cancel(id)

    ns.Simulator.Run()

    ns.Simulator.Destroy()


if __name__ == "__main__":
    import sys

    main(sys.argv)
