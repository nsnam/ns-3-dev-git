/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/helics-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("HelicsExample");

int 
main (int argc, char *argv[])
{
  bool verbose = false;

  HelicsHelper helics;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  helics.SetupCommandLine(cmd);
  cmd.Parse (argc,argv);

  if (verbose) {
    LogComponentEnable ("HelicsExample", LOG_LEVEL_ALL);
    LogComponentEnable ("HelicsSimulatorImpl", LOG_LEVEL_ALL);
  }

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::HelicsSimulatorImpl"));

  helics.SetupFederate();
  NS_LOG_INFO ("Simulator Impl bound, about to Run simulator");

  Simulator::Stop (Seconds (10.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}


