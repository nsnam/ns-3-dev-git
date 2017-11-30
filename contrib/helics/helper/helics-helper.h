/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef HELICS_HELPER_H
#define HELICS_HELPER_H

#include "ns3/application-container.h"
#include "ns3/core-module.h"
#include "ns3/node-container.h"

#include "ns3/helics.h"

namespace ns3 {

class HelicsHelper {
public:
  HelicsHelper();
  void SetupFederate(void);
  void SetupCommandLine(CommandLine &cmd);

  ApplicationContainer Install (Ptr<Node> node, const std::string &name) const;

private:
  std::string broker;
  std::string name;
  std::string type;
  std::string core;
  double stop;
  double timedelta;
  std::string coreinit;
  ObjectFactory m_factory;
};

}

#endif /* HELICS_HELPER_H */

