/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Alexander Krotov <krotov@iitp.ru>
 *
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
 *
 */

#include <iostream>

#include "ns3/test.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief Test that connection failed callback is called when
 * SYN retransmission number is exceeded.
 */
class TcpSynConnectionFailedTest : public TestCase
{
public:
  /**
   * Constructor.
   * \param desc Test description.
   */
  TcpSynConnectionFailedTest (std::string desc);

  /**
   * \brief Handle a connection failure.
   * \param socket The receiving socket.
   */
  void HandleConnectionFailed (Ptr<Socket> socket);
  virtual void DoRun ();

private:
  bool m_connectionFailed{false};
};

TcpSynConnectionFailedTest::TcpSynConnectionFailedTest (std::string desc) : TestCase (desc)
{
}

void
TcpSynConnectionFailedTest::HandleConnectionFailed (Ptr<Socket> socket)
{
  m_connectionFailed = true;
}

void
TcpSynConnectionFailedTest::DoRun ()
{
  Ptr<Node> node = CreateObject<Node> ();

  InternetStackHelper internet;
  internet.Install (node);

  TypeId tid = TcpSocketFactory::GetTypeId ();

  Ptr<Socket> socket = Socket::CreateSocket (node, tid);
  socket->Bind ();
  socket->SetConnectCallback (MakeNullCallback <void, Ptr<Socket>>(),
                              MakeCallback (&TcpSynConnectionFailedTest::HandleConnectionFailed, this));
  socket->Connect (InetSocketAddress (Ipv4Address::GetLoopback (), 9));

  Simulator::Run ();
  Simulator::Destroy ();

  NS_TEST_ASSERT_MSG_EQ (m_connectionFailed, true, "Connection failed callback was not called");
}

/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief TestSuite
 */
class TcpSynConnectionFailedTestSuite : public TestSuite
{
public:
  TcpSynConnectionFailedTestSuite () : TestSuite ("tcp-syn-connection-failed-test", UNIT)
  {
    AddTestCase (new TcpSynConnectionFailedTest ("TCP SYN connection failed test"), TestCase::QUICK);
  }
};

static TcpSynConnectionFailedTestSuite g_TcpSynConnectionFailedTestSuite; //!< Static variable for test initialization
