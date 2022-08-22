 /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/test.h"
#include "ns3/log.h"
#include "ns3/mgt-headers.h"
#include "ns3/multi-link-element.h"
#include "ns3/wifi-assoc-manager.h"
#include <vector>
#include "ns3/string.h"
#include "ns3/qos-utils.h"
#include "ns3/packet.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/he-frame-exchange-manager.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-protection.h"
#include "ns3/he-configuration.h"
#include "ns3/mobility-helper.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/config.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/wifi-psdu.h"
#include "ns3/he-phy.h"
#include <iomanip>

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("WifiMloTest");


/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the implementation of WifiAssocManager::GetNextAffiliatedAp(), which
 * searches a given RNR element for APs affiliated to the same AP MLD as the
 * reporting AP that sent the frame containing the element.
 */
class GetRnrLinkInfoTest : public TestCase
{
public:
  /**
   * Constructor
   */
  GetRnrLinkInfoTest ();
  virtual ~GetRnrLinkInfoTest () = default;

private:
  virtual void DoRun (void);
};

GetRnrLinkInfoTest::GetRnrLinkInfoTest ()
  : TestCase ("Check the implementation of WifiAssocManager::GetNextAffiliatedAp()")
{
}

void
GetRnrLinkInfoTest::DoRun (void)
{
  ReducedNeighborReport rnr;
  std::size_t nbrId;
  std::size_t tbttId;

  // Add a first Neighbor AP Information field without MLD Parameters
  rnr.AddNbrApInfoField ();
  nbrId = rnr.GetNNbrApInfoFields () - 1;

  rnr.AddTbttInformationField (nbrId);
  rnr.AddTbttInformationField (nbrId);

  // Add a second Neighbor AP Information field with MLD Parameters; the first
  // TBTT Information field is related to an AP affiliated to the same AP MLD
  // as the reported AP; the second TBTT Information field is not (it does not
  // make sense that two APs affiliated to the same AP MLD are using the same
  // channel).
  rnr.AddNbrApInfoField ();
  nbrId = rnr.GetNNbrApInfoFields () - 1;

  rnr.AddTbttInformationField (nbrId);
  tbttId = rnr.GetNTbttInformationFields (nbrId) - 1;
  rnr.SetMldParameters (nbrId, tbttId, 0, 0, 0);

  rnr.AddTbttInformationField (nbrId);
  tbttId = rnr.GetNTbttInformationFields (nbrId) - 1;
  rnr.SetMldParameters (nbrId, tbttId, 5, 0, 0);

  // Add a third Neighbor AP Information field with MLD Parameters; none of the
  // TBTT Information fields is related to an AP affiliated to the same AP MLD
  // as the reported AP.
  rnr.AddNbrApInfoField ();
  nbrId = rnr.GetNNbrApInfoFields () - 1;

  rnr.AddTbttInformationField (nbrId);
  tbttId = rnr.GetNTbttInformationFields (nbrId) - 1;
  rnr.SetMldParameters (nbrId, tbttId, 3, 0, 0);

  rnr.AddTbttInformationField (nbrId);
  tbttId = rnr.GetNTbttInformationFields (nbrId) - 1;
  rnr.SetMldParameters (nbrId, tbttId, 4, 0, 0);

  // Add a fourth Neighbor AP Information field with MLD Parameters; the first
  // TBTT Information field is not related to an AP affiliated to the same AP MLD
  // as the reported AP; the second TBTT Information field is.
  rnr.AddNbrApInfoField ();
  nbrId = rnr.GetNNbrApInfoFields () - 1;

  rnr.AddTbttInformationField (nbrId);
  tbttId = rnr.GetNTbttInformationFields (nbrId) - 1;
  rnr.SetMldParameters (nbrId, tbttId, 6, 0, 0);

  rnr.AddTbttInformationField (nbrId);
  tbttId = rnr.GetNTbttInformationFields (nbrId) - 1;
  rnr.SetMldParameters (nbrId, tbttId, 0, 0, 0);

  // check implementation of WifiAssocManager::GetNextAffiliatedAp()
  auto ret = WifiAssocManager::GetNextAffiliatedAp (rnr, 0);

  NS_TEST_EXPECT_MSG_EQ (ret.has_value (), true, "Expected to find a suitable reported AP");
  NS_TEST_EXPECT_MSG_EQ (ret->m_nbrApInfoId, 1, "Unexpected neighbor ID of the first reported AP");
  NS_TEST_EXPECT_MSG_EQ (ret->m_tbttInfoFieldId, 0, "Unexpected tbtt ID of the first reported AP");

  ret = WifiAssocManager::GetNextAffiliatedAp (rnr, ret->m_nbrApInfoId + 1);

  NS_TEST_EXPECT_MSG_EQ (ret.has_value (), true, "Expected to find a second suitable reported AP");
  NS_TEST_EXPECT_MSG_EQ (ret->m_nbrApInfoId, 3, "Unexpected neighbor ID of the second reported AP");
  NS_TEST_EXPECT_MSG_EQ (ret->m_tbttInfoFieldId, 1, "Unexpected tbtt ID of the second reported AP");

  ret = WifiAssocManager::GetNextAffiliatedAp (rnr, ret->m_nbrApInfoId + 1);

  NS_TEST_EXPECT_MSG_EQ (ret.has_value (), false, "Did not expect to find a third suitable reported AP");

  // check implementation of WifiAssocManager::GetAllAffiliatedAps()
  auto allAps = WifiAssocManager::GetAllAffiliatedAps (rnr);

  NS_TEST_EXPECT_MSG_EQ (allAps.size (), 2, "Expected to find two suitable reported APs");

  auto apIt = allAps.begin ();
  NS_TEST_EXPECT_MSG_EQ (apIt->m_nbrApInfoId, 1, "Unexpected neighbor ID of the first reported AP");
  NS_TEST_EXPECT_MSG_EQ (apIt->m_tbttInfoFieldId, 0, "Unexpected tbtt ID of the first reported AP");

  apIt++;
  NS_TEST_EXPECT_MSG_EQ (apIt->m_nbrApInfoId, 3, "Unexpected neighbor ID of the second reported AP");
  NS_TEST_EXPECT_MSG_EQ (apIt->m_tbttInfoFieldId, 1, "Unexpected tbtt ID of the second reported AP");
}


/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi 11be MLD Test Suite
 */
class WifiMultiLinkOperationsTestSuite : public TestSuite
{
public:
  WifiMultiLinkOperationsTestSuite ();
};

WifiMultiLinkOperationsTestSuite::WifiMultiLinkOperationsTestSuite ()
  : TestSuite ("wifi-mlo", UNIT)
{
  AddTestCase (new GetRnrLinkInfoTest (), TestCase::QUICK);
}

static WifiMultiLinkOperationsTestSuite g_wifiMultiLinkOperationsTestSuite; ///< the test suite
