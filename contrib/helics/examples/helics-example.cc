/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/helics-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("HelicsExample");

int 
main (int argc, char *argv[])
{
  bool verbose = true;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);

  cmd.Parse (argc,argv);

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::HelicsSimulatorImpl"));

  HelicsHelper helics;
  helics.SetupFederate();
  NS_LOG_INFO ("Simulator Impl bound, about to Run simulator");

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}


