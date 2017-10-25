/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef HELICS_HELPER_H
#define HELICS_HELPER_H

#include "ns3/core-module.h"
#include "ns3/helics.h"

namespace ns3 {

class HelicsHelper {
public:
  HelicsHelper();
  void SetupFederate(void);
  void SetupCommandLine(CommandLine &cmd);

private:
  std::string broker;
  std::string name;
  std::string type;
  std::string core;
  double stop;
  double timedelta;
  std::string coreinit;
};

}

#endif /* HELICS_HELPER_H */

