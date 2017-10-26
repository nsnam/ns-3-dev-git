/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "helics-helper.h"

#include <memory>

#include "ns3/helics.h"
#include "helics/core/core-types.h"

namespace ns3 {

HelicsHelper::HelicsHelper()
: broker("")
, name("ns3")
, core("zmq")
, stop(1.0)
, timedelta(1.0)
, coreinit("")
{
}

void
HelicsHelper::SetupFederate(void)
{
  helics::FederateInfo fi (name);
  fi.coreType = helics::coreTypeFromString (core);
  //fi.coreInitString = "--broker=" + "brokid" + " --broker_address=" + "addr" + " --federates 1";
  //fi.coreInitString = "--federates 1";
  fi.timeDelta = timedelta;
  if (!coreinit.empty()) {
    fi.coreInitString = coreinit;
  }
  federate = std::make_shared<helics::MessageFederate> (fi);
}

void
HelicsHelper::SetupCommandLine(CommandLine &cmd)
{
  cmd.AddValue ("broker", "address to connect the broker to", broker);
  cmd.AddValue ("name", "name of the ns3 federate", name);
  cmd.AddValue ("core", "name of the core to connect to", core);
  cmd.AddValue ("stop", "the time to stop the player", stop);
  cmd.AddValue ("timedelta", "the time delta of the federate", timedelta);
  cmd.AddValue ("coreinit", "the core initializion string", coreinit);
}

}

