/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
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

#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/mgt-headers.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiEmlsrTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test EML Operating Mode Notification frame serialization and deserialization
 */
class EmlOperatingModeNotificationTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     */
    EmlOperatingModeNotificationTest();
    ~EmlOperatingModeNotificationTest() override = default;

  private:
    void DoRun() override;
};

EmlOperatingModeNotificationTest::EmlOperatingModeNotificationTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of the EML Operating Mode Notification frame")
{
}

void
EmlOperatingModeNotificationTest::DoRun()
{
    MgtEmlOperatingModeNotification frame;

    // Both EMLSR Mode and EMLMR Mode subfields set to 0 (no link bitmap);
    TestHeaderSerialization(frame);

    frame.m_emlControl.emlsrMode = 1;
    frame.SetLinkIdInBitmap(0);
    frame.SetLinkIdInBitmap(5);
    frame.SetLinkIdInBitmap(15);

    // Adding Link Bitmap
    TestHeaderSerialization(frame);

    NS_TEST_EXPECT_MSG_EQ((frame.GetLinkBitmap() == std::list<uint8_t>{0, 5, 15}),
                          true,
                          "Unexpected link bitmap");

    auto padding = MicroSeconds(64);
    auto transition = MicroSeconds(128);

    frame.m_emlControl.emlsrParamUpdateCtrl = 1;
    frame.m_emlsrParamUpdate = MgtEmlOperatingModeNotification::EmlsrParamUpdate{};
    frame.m_emlsrParamUpdate->paddingDelay = CommonInfoBasicMle::EncodeEmlsrPaddingDelay(padding);
    frame.m_emlsrParamUpdate->transitionDelay =
        CommonInfoBasicMle::EncodeEmlsrTransitionDelay(transition);

    // Adding the EMLSR Parameter Update field
    TestHeaderSerialization(frame);

    NS_TEST_EXPECT_MSG_EQ(
        CommonInfoBasicMle::DecodeEmlsrPaddingDelay(frame.m_emlsrParamUpdate->paddingDelay),
        padding,
        "Unexpected EMLSR Padding Delay");
    NS_TEST_EXPECT_MSG_EQ(
        CommonInfoBasicMle::DecodeEmlsrTransitionDelay(frame.m_emlsrParamUpdate->transitionDelay),
        transition,
        "Unexpected EMLSR Transition Delay");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi EMLSR Test Suite
 */
class WifiEmlsrTestSuite : public TestSuite
{
  public:
    WifiEmlsrTestSuite();
};

WifiEmlsrTestSuite::WifiEmlsrTestSuite()
    : TestSuite("wifi-emlsr", UNIT)
{
    AddTestCase(new EmlOperatingModeNotificationTest(), TestCase::QUICK);
}

static WifiEmlsrTestSuite g_wifiEmlsrTestSuite; ///< the test suite
