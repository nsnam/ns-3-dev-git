/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * The "SimpleFederate" is a placeholder for a federate that is *not*
 * ns-3. It is a helics::MessageFederate, so it sends messages between
 * registered endpoints.
 */

#include <iostream>

#include "helics/helics.hpp"
#include "helics/core/core-types.h"

int 
main (int argc, char *argv[])
{
  helics::FederateInfo fi ("fed-sndrcv");
  fi.coreType = helics::coreTypeFromString ("zmq");
  fi.coreInitString = "--loglevel=4";
  auto mFed = std::make_shared<helics::MessageFederate> (fi);
  auto p1 = mFed->registerEndpoint ("endpoint1");
  auto p2 = mFed->registerEndpoint ("endpoint2");
  mFed->enterExecutionState ();
  std::cout << "fed-sndrcv entered execution state" << std::endl;
  auto granted = mFed->requestTime (1.0);
  std::cout << "fed-sndrcv granted time " << granted << std::endl;
  helics::data_block data (500, 'a');
  mFed->sendMessage (p1, "ns3/endpoint1", data);
  std::cout << "fed-sndrcv sent message to ns3/endpoint1" << std::endl;
  while (granted < 8.0) {
    granted = mFed->requestTime (granted + 1.0);
    std::cout << "fed-sndrcv hasMessage? " << mFed->hasMessage (p2) << std::endl;
  }
  mFed->finalize ();
  return 0;
}

