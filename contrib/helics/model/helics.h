/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef HELICS_H
#define HELICS_H

#include <memory>

#include "helics/helics.hpp"

namespace ns3 {

extern std::shared_ptr<helics::MessageFederate> helics_federate;
extern helics::endpoint_id_t helics_endpoint;

}

#endif /* HELICS_H */

