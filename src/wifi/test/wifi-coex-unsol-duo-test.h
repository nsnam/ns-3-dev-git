/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_COEX_UNSOL_DUO_TEST_H
#define WIFI_COEX_UNSOL_DUO_TEST_H

#include "wifi-coex-test-base.h"

#include "ns3/ctrl-headers.h"
#include "ns3/header-serialization-test.h"

#include <optional>

using namespace ns3;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test BSRP NTB Trigger Frame with Feedback User Info field serialization and
 * deserialization
 */
class BsrpNtbTfWithFeedbackTest : public HeaderSerializationTestCase
{
  public:
    BsrpNtbTfWithFeedbackTest();

  private:
    void DoRun() override;

    /**
     * Check that the given Trigger Frame is a BSRP NTB trigger frame including a Feedback User Info
     * field of type Unsolicited Unavailability. Return the unavailability period encoded in the
     * Feedback User Info field.
     *
     * @param trigger the given Trigger Frame
     * @param testStr a string identifying the test case
     * @return the unavailability period encoded in the Feedback User Info field
     */
    std::optional<WifiUnavailPeriod> CheckFeedback(const CtrlTriggerHeader& trigger,
                                                   const std::string& testStr);
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi coex unsolicited DUO Test Suite
 */
class WifiCoexUnsolDuoTestSuite : public TestSuite
{
  public:
    WifiCoexUnsolDuoTestSuite();
};

#endif /* WIFI_COEX_UNSOL_DUO_TEST_H */
