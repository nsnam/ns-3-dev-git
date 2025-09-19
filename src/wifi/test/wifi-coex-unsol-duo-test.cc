/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-coex-unsol-duo-test.h"

#include "ns3/log.h"

#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiCoexUnsolDuoTest");

BsrpNtbTfWithFeedbackTest::BsrpNtbTfWithFeedbackTest()
    : HeaderSerializationTestCase("Check serialization and deserialization of BSRP NTB Trigger "
                                  "Frame with Feedback User Info field")
{
}

std::optional<WifiUnavailPeriod>
BsrpNtbTfWithFeedbackTest::CheckFeedback(const CtrlTriggerHeader& trigger,
                                         const std::string& testStr)
{
    NS_TEST_EXPECT_MSG_EQ(trigger.IsBsrpNtb(),
                          true,
                          testStr + ": Expected a BSRP NTB Trigger Frame");
    auto& fui = trigger.GetFeedbackUserInfo();
    NS_TEST_EXPECT_MSG_EQ(fui.has_value(), true, testStr + ": No feedback user info");
    if (fui.has_value())
    {
        NS_TEST_EXPECT_MSG_EQ(
            +static_cast<uint8_t>(fui->m_feedbackType),
            +static_cast<uint8_t>(
                CtrlTriggerFeedbackUserInfoField::Type::UNSOLICITED_UNAVAILABILITY),
            testStr + ": Unexpected feedback type");

        return fui->GetUnavailability();
    }
    return std::nullopt;
}

void
BsrpNtbTfWithFeedbackTest::DoRun()
{
    /*
     * Feedback User Info field indicating that the STA is available
     */
    {
        std::string testStr = "STA available";
        CtrlTriggerHeader trigger(TriggerFrameType::BSRP_TRIGGER, TriggerFrameVariant::UHR);
        auto& fui = trigger.AddFeedbackUserInfoField();
        fui.SetStaAvailable();

        auto deserTrigger = TestHeaderSerialization(trigger);
        auto unavailPeriod = CheckFeedback(deserTrigger, testStr);

        NS_TEST_EXPECT_MSG_EQ(unavailPeriod.has_value(),
                              false,
                              testStr + ": Expected no value, STA is available");
    }

    /*
     * Feedback User Info field indicating indefinite unavailability
     */
    constexpr auto nIgnoredBits = 6;
    WifiUnavailPeriod inputUnavail;
    inputUnavail.start = MicroSeconds(0xabcdef);
    inputUnavail.end =
        inputUnavail.start + MicroSeconds(1023 * (1 << nIgnoredBits)); // maximum duration exceeded
    auto rxTime = MicroSeconds(0xabc111); // upper 48 bits are the same as inputUnavail.start

    Simulator::Schedule(rxTime, [=, this] {
        std::string testStr = "Indefinite unavailability";
        CtrlTriggerHeader trigger(TriggerFrameType::BSRP_TRIGGER, TriggerFrameVariant::UHR);
        auto& fui = trigger.AddFeedbackUserInfoField();
        bool indefinite{};
        fui.SetStaUnavailable(inputUnavail, indefinite);
        NS_TEST_EXPECT_MSG_EQ(indefinite, true, testStr + ": Expected an indefinite duration");

        auto deserTrigger = TestHeaderSerialization(trigger);
        auto unavailPeriod = CheckFeedback(deserTrigger, testStr);

        NS_TEST_ASSERT_MSG_EQ(unavailPeriod.has_value(),
                              true,
                              testStr + ": Expected an unavailability period");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(unavailPeriod->start,
                                    inputUnavail.start,
                                    testStr +
                                        "Actual unavailable start must be no later than requested");
        const auto expectedStart =
            MicroSeconds(inputUnavail.start.GetMicroSeconds() >> nIgnoredBits) *
            (1 << nIgnoredBits);
        NS_TEST_EXPECT_MSG_EQ(unavailPeriod->start,
                              expectedStart,
                              testStr + "Unexpected encoded start time");
        NS_TEST_EXPECT_MSG_EQ(unavailPeriod->end.has_value(),
                              false,
                              testStr + "Unexpected encoded end time");
    });

    /*
     * Feedback User Info field indicating finite unavailability
     */
    inputUnavail.start = MicroSeconds(0xabcdef);
    inputUnavail.end = inputUnavail.start + MicroSeconds(10 * (1 << nIgnoredBits));
    rxTime = MicroSeconds(0xabc111); // upper 48 bits are the same as unavailStart

    Simulator::Schedule(rxTime, [=, this] {
        std::string testStr = "Finite unavailability";
        CtrlTriggerHeader trigger(TriggerFrameType::BSRP_TRIGGER, TriggerFrameVariant::UHR);
        auto& fui = trigger.AddFeedbackUserInfoField();
        bool indefinite{};
        fui.SetStaUnavailable(inputUnavail, indefinite);
        NS_TEST_EXPECT_MSG_EQ(indefinite, false, testStr + ": Expected a finite duration");

        auto deserTrigger = TestHeaderSerialization(trigger);
        auto unavailPeriod = CheckFeedback(deserTrigger, testStr);

        NS_TEST_ASSERT_MSG_EQ(unavailPeriod.has_value(),
                              true,
                              testStr + ": Expected an unavailability period");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(unavailPeriod->start,
                                    inputUnavail.start,
                                    testStr +
                                        "Actual unavailable start must be no later than requested");
        const auto expectedStart =
            MicroSeconds(inputUnavail.start.GetMicroSeconds() >> nIgnoredBits) *
            (1 << nIgnoredBits);
        NS_TEST_EXPECT_MSG_EQ(unavailPeriod->start,
                              expectedStart,
                              testStr + "Unexpected encoded start time");
        NS_TEST_EXPECT_MSG_EQ(unavailPeriod->end.has_value(),
                              true,
                              testStr + "Expected an encoded end time");
        NS_TEST_EXPECT_MSG_GT_OR_EQ(unavailPeriod->end.value(),
                                    inputUnavail.end.value(),
                                    testStr +
                                        "Actual unavailable end must be no earlier than requested");
        const auto expectedEnd =
            MicroSeconds(std::ceil(static_cast<double>(inputUnavail.end->GetMicroSeconds()) /
                                   (1 << nIgnoredBits))) *
            (1 << nIgnoredBits);
        NS_TEST_EXPECT_MSG_EQ(unavailPeriod->end.value(),
                              expectedEnd,
                              testStr + "Unexpected encoded end time");
    });

    /*
     * Feedback User Info field causing a wrap around issue
     */
    inputUnavail.start = MicroSeconds(0xab0010); // 11.206 672 s
    inputUnavail.end.reset();
    rxTime =
        MicroSeconds(0xaaff99); // 11.206 553 s -- upper 48 bits are different than unavailStart

    Simulator::Schedule(rxTime, [=, this] {
        std::string testStr = "Wrap around issue";
        CtrlTriggerHeader trigger(TriggerFrameType::BSRP_TRIGGER, TriggerFrameVariant::UHR);
        auto& fui = trigger.AddFeedbackUserInfoField();
        bool indefinite{};
        fui.SetStaUnavailable(inputUnavail, indefinite);
        NS_TEST_EXPECT_MSG_EQ(indefinite, true, testStr + ": Expected an indefinite duration");

        auto deserTrigger = TestHeaderSerialization(trigger);
        auto unavailPeriod = CheckFeedback(deserTrigger, testStr);

        NS_TEST_ASSERT_MSG_EQ(unavailPeriod.has_value(),
                              true,
                              testStr + ": Expected an unavailability period");
        NS_TEST_EXPECT_MSG_LT_OR_EQ(unavailPeriod->start,
                                    inputUnavail.start,
                                    testStr +
                                        "Actual unavailable start must be no later than requested");
        const auto expectedStart =
            MicroSeconds(inputUnavail.start.GetMicroSeconds() >> nIgnoredBits) *
            (1 << nIgnoredBits);
        NS_TEST_EXPECT_MSG_EQ(unavailPeriod->start,
                              expectedStart,
                              testStr + "Unexpected encoded start time");
        NS_TEST_EXPECT_MSG_EQ(unavailPeriod->end.has_value(),
                              false,
                              testStr + "Unexpected encoded end time");
    });

    Simulator::Run();
}

WifiCoexUnsolDuoTestSuite::WifiCoexUnsolDuoTestSuite()
    : TestSuite("wifi-coex-unsol-duo-test", Type::UNIT)
{
    AddTestCase(new BsrpNtbTfWithFeedbackTest, TestCase::Duration::QUICK);
}

static WifiCoexUnsolDuoTestSuite g_wifiCoexUnsolDuoTestSuite; ///< the test suite
