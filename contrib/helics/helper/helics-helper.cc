/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "helics-helper.h"

#include <memory>

#include "ns3/helics.h"

namespace ns3 {

void
HelicsHelper::SetupFederate(void)
{
  helics::FederateInfo fi ("ns3");
  fi.coreType = helics::coreTypeFromString ("zmq");
  //fi.coreInitString = "--broker=" + "brokid" + " --broker_address=" + "addr" + " --federates 1";
  fi.coreInitString = "--federates 1";
  federate = std::make_shared<helics::MessageFederate> (fi);
}

}

