/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include <memory>

#include "ns3/helics.h"
#include "ns3/helics-application.h"
#include "ns3/helics-helper.h"

#include "helics/core/core-types.h"

namespace ns3 {

HelicsHelper::HelicsHelper()
: broker("")
, name("ns3")
, core("zmq")
, stop(1.0)
, timedelta(1.0)
, coreinit("--loglevel=4")
{
    m_factory.SetTypeId (HelicsApplication::GetTypeId ());
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
  helics_federate = std::make_shared<helics::MessageFilterFederate> (fi);
  helics_endpoint = helics_federate->registerEndpoint ("fout");

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::HelicsSimulatorImpl"));
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

ApplicationContainer
HelicsHelper::InstallFilter (Ptr<Node> node, const std::string &name) const
{
    ApplicationContainer apps;
    Ptr<HelicsApplication> app = m_factory.Create<HelicsApplication> ();
    app->SetFilterName (name);
    node->AddApplication (app);
    apps.Add (app);
    return apps;
}

ApplicationContainer
HelicsHelper::InstallEndpoint (Ptr<Node> node, const std::string &name) const
{
    ApplicationContainer apps;
    Ptr<HelicsApplication> app = m_factory.Create<HelicsApplication> ();
    app->SetEndpointName (name);
    node->AddApplication (app);
    apps.Add (app);
    return apps;
}

}

