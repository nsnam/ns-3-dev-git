/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/adhoc-wifi-mac.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/channel-access-manager.h"
#include "ns3/config.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/interference-helper.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-spectrum-phy-interface.h"

#include <iomanip>
#include <list>
#include <numeric>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiChannelAccessManagerTest");

template <typename TxopType>
class ChannelAccessManagerTest;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief TxopTest Txop Test
 */
template <typename TxopType>
class TxopTest : public TxopType
{
  public:
    /**
     * Constructor
     *
     * @param test the test channel access manager
     * @param i the index of the Txop
     */
    TxopTest(ChannelAccessManagerTest<TxopType>* test, uint32_t i);

    /**
     * Queue transmit function
     * @param txTime the transmit time
     * @param expectedGrantTime the expected grant time
     */
    void QueueTx(uint64_t txTime, uint64_t expectedGrantTime);

  private:
    /// allow ChannelAccessManagerTest class access
    friend class ChannelAccessManagerTest<TxopType>;

    /// @copydoc ns3::Txop::DoDispose
    void DoDispose() override;
    /// @copydoc ns3::Txop::NotifyChannelAccessed
    void NotifyChannelAccessed(uint8_t linkId, Time txopDuration = Seconds(0)) override;
    /// @copydoc ns3::Txop::HasFramesToTransmit
    bool HasFramesToTransmit(uint8_t linkId) override;
    /// @copydoc ns3::Txop::NotifySleep
    void NotifySleep(uint8_t linkId) override;
    /// @copydoc ns3::Txop::NotifyWakeUp
    void NotifyWakeUp(uint8_t linkId) override;
    /// @copydoc ns3::Txop::GenerateBackoff
    void GenerateBackoff(uint8_t linkId) override;

    typedef std::pair<uint64_t, uint64_t> ExpectedGrant; //!< the expected grant typedef
    typedef std::list<ExpectedGrant> ExpectedGrants; //!< the collection of expected grants typedef

    /// ExpectedBackoff structure
    struct ExpectedBackoff
    {
        uint64_t at;     //!< at
        uint32_t nSlots; //!< number of slots
    };

    typedef std::list<ExpectedBackoff> ExpectedBackoffs; //!< expected backoffs typedef

    ExpectedBackoffs m_expectedInternalCollision; //!< expected backoff due to an internal collision
    ExpectedBackoffs m_expectedBackoff; //!< expected backoff (not due to an internal collision)
    ExpectedGrants m_expectedGrants;    //!< expected grants

    /**
     * Check if the Txop has frames to transmit.
     * @return true if the Txop has frames to transmit.
     */

    ChannelAccessManagerTest<TxopType>* m_test; //!< the test DCF/EDCA manager
    uint32_t m_i;                               //!< the index of the Txop
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief ChannelAccessManager Stub
 */
class ChannelAccessManagerStub : public ChannelAccessManager
{
  public:
    ChannelAccessManagerStub()
    {
    }

    /**
     * Set the Short Interframe Space (SIFS).
     *
     * @param sifs the SIFS duration
     */
    void SetSifs(Time sifs)
    {
        m_sifs = sifs;
    }

    /**
     * Set the slot duration.
     *
     * @param slot the slot duration
     */
    void SetSlot(Time slot)
    {
        m_slot = slot;
    }

    /**
     * Set the duration of EIFS - DIFS
     *
     * @param eifsNoDifs the duration of EIFS - DIFS
     */
    void SetEifsNoDifs(Time eifsNoDifs)
    {
        m_eifsNoDifs = eifsNoDifs;
    }

  private:
    Time GetSifs() const override
    {
        return m_sifs;
    }

    Time GetSlot() const override
    {
        return m_slot;
    }

    Time GetEifsNoDifs() const override
    {
        return m_eifsNoDifs;
    }

    Time m_slot;       //!< slot duration
    Time m_sifs;       //!< SIFS duration
    Time m_eifsNoDifs; //!< EIFS duration minus a DIFS
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Frame Exchange Manager Stub
 */
template <typename TxopType>
class FrameExchangeManagerStub : public FrameExchangeManager
{
  public:
    /**
     * Constructor
     *
     * @param test the test channel access manager
     */
    FrameExchangeManagerStub(ChannelAccessManagerTest<TxopType>* test)
        : m_test(test)
    {
    }

    /**
     * Request the FrameExchangeManager to start a frame exchange sequence.
     *
     * @param dcf the channel access function that gained channel access. It is
     *            the DCF on non-QoS stations and an EDCA on QoS stations.
     * @param allowedWidth the maximum allowed TX width
     * @return true if a frame exchange sequence was started, false otherwise
     */
    bool StartTransmission(Ptr<Txop> dcf, MHz_u allowedWidth) override
    {
        dcf->NotifyChannelAccessed(0);
        return true;
    }

    /// @copydoc ns3::FrameExchangeManager::NotifyInternalCollision
    void NotifyInternalCollision(Ptr<Txop> txop) override
    {
        m_test->NotifyInternalCollision(DynamicCast<TxopTest<TxopType>>(txop));
    }

    /// @copydoc ns3::FrameExchangeManager::NotifySwitchingStartNow
    void NotifySwitchingStartNow(Time duration) override
    {
        m_test->NotifyChannelSwitching();
    }

  private:
    ChannelAccessManagerTest<TxopType>* m_test; //!< the test DCF/EDCA manager
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Channel Access Manager Test
 */
template <typename TxopType>
class ChannelAccessManagerTest : public TestCase
{
  public:
    ChannelAccessManagerTest();
    void DoRun() override;

    /**
     * Notify access granted function
     * @param i the index of the Txop
     */
    void NotifyAccessGranted(uint32_t i);
    /**
     * Notify internal collision function
     * @param state the Txop
     */
    void NotifyInternalCollision(Ptr<TxopTest<TxopType>> state);
    /**
     * Generate backoff function
     * @param i the index of the Txop
     */
    void GenerateBackoff(uint32_t i);
    /**
     * Notify channel switching function
     */
    void NotifyChannelSwitching();

  private:
    /**
     * Start test function
     * @param slotTime the slot time
     * @param sifs the SIFS
     * @param eifsNoDifsNoSifs the EIFS no DIFS no SIFS
     * @param ackTimeoutValue the Ack timeout value
     * @param chWidth the channel width
     */
    void StartTest(uint64_t slotTime,
                   uint64_t sifs,
                   uint64_t eifsNoDifsNoSifs,
                   uint32_t ackTimeoutValue = 20,
                   MHz_u chWidth = MHz_u{20});
    /**
     * Add Txop function
     * @param aifsn the AIFSN
     */
    void AddTxop(uint32_t aifsn);
    /// End test function
    void EndTest();
    /**
     * Expect internal collision function
     * @param time the expected time
     * @param nSlots the number of slots
     * @param from the expected from
     */
    void ExpectInternalCollision(uint64_t time, uint32_t nSlots, uint32_t from);
    /**
     * Expect generate backoff function
     * @param time the expected time
     * @param nSlots the number of slots
     * @param from the expected from
     */
    void ExpectBackoff(uint64_t time, uint32_t nSlots, uint32_t from);
    /**
     * Schedule a check that the channel access manager is busy or idle
     * @param time the expected time
     * @param busy whether the manager is expected to be busy
     */
    void ExpectBusy(uint64_t time, bool busy);
    /**
     * Perform check that channel access manager is busy or idle
     * @param busy whether expected state is busy
     */
    void DoCheckBusy(bool busy);
    /**
     * Add receive OK event function
     * @param at the event time
     * @param duration the duration
     */
    void AddRxOkEvt(uint64_t at, uint64_t duration);
    /**
     * Add receive error event function for error at end of frame
     * @param at the event time
     * @param duration the duration
     */
    void AddRxErrorEvt(uint64_t at, uint64_t duration);
    /**
     * Add receive error event function for error during frame
     * @param at the event time
     * @param duration the duration
     * @param timeUntilError the time after event time to force the error
     */
    void AddRxErrorEvt(uint64_t at, uint64_t duration, uint64_t timeUntilError);
    /**
     * Add receive inside SIFS event function
     * @param at the event time
     * @param duration the duration
     */
    void AddRxInsideSifsEvt(uint64_t at, uint64_t duration);
    /**
     * Add transmit event function
     * @param at the event time
     * @param duration the duration
     */
    void AddTxEvt(uint64_t at, uint64_t duration);
    /**
     * Add NAV reset function
     * @param at the event time
     * @param duration the duration
     */
    void AddNavReset(uint64_t at, uint64_t duration);
    /**
     * Add NAV start function
     * @param at the event time
     * @param duration the duration
     */
    void AddNavStart(uint64_t at, uint64_t duration);
    /**
     * Add Ack timeout reset function
     * @param at the event time
     */
    void AddAckTimeoutReset(uint64_t at);
    /**
     * Add access function
     * @param at the event time
     * @param txTime the transmit time
     * @param expectedGrantTime the expected grant time
     * @param from the index of the requesting Txop
     */
    void AddAccessRequest(uint64_t at, uint64_t txTime, uint64_t expectedGrantTime, uint32_t from);
    /**
     * Add access request with Ack timeout
     * @param at time to schedule DoAccessRequest event
     * @param txTime the transmit time
     * @param expectedGrantTime the expected grant time
     * @param from the index of the requesting Txop
     */
    void AddAccessRequestWithAckTimeout(uint64_t at,
                                        uint64_t txTime,
                                        uint64_t expectedGrantTime,
                                        uint32_t from);
    /**
     * Add access request with successful ack
     * @param at time to schedule DoAccessRequest event
     * @param txTime the transmit time
     * @param expectedGrantTime the expected grant time
     * @param ackDelay the delay of the Ack after txEnd
     * @param from the index of the requesting Txop
     */
    void AddAccessRequestWithSuccessfulAck(uint64_t at,
                                           uint64_t txTime,
                                           uint64_t expectedGrantTime,
                                           uint32_t ackDelay,
                                           uint32_t from);
    /**
     * Add access request with successful Ack
     * @param txTime the transmit time
     * @param expectedGrantTime the expected grant time
     * @param state TxopTest
     */
    void DoAccessRequest(uint64_t txTime,
                         uint64_t expectedGrantTime,
                         Ptr<TxopTest<TxopType>> state);
    /**
     * Add CCA busy event function
     * @param at the event time
     * @param duration the duration
     * @param channelType the channel type
     * @param per20MhzDurations vector that indicates for how long each 20 MHz subchannel is busy
     */
    void AddCcaBusyEvt(uint64_t at,
                       uint64_t duration,
                       WifiChannelListType channelType = WIFI_CHANLIST_PRIMARY,
                       const std::vector<Time>& per20MhzDurations = {});
    /**
     * Add switching event function
     * @param at the event time
     * @param duration the duration
     */
    void AddSwitchingEvt(uint64_t at, uint64_t duration);
    /**
     * Add receive start event function
     * @param at the event time
     * @param duration the duration
     */
    void AddRxStartEvt(uint64_t at, uint64_t duration);

    typedef std::vector<Ptr<TxopTest<TxopType>>> TxopTests; //!< the TXOP tests typedef

    Ptr<FrameExchangeManagerStub<TxopType>> m_feManager;  //!< the Frame Exchange Manager stubbed
    Ptr<ChannelAccessManagerStub> m_ChannelAccessManager; //!< the channel access manager
    Ptr<SpectrumWifiPhy> m_phy;                           //!< the PHY object
    TxopTests m_txop;                                     //!< the vector of Txop test instances
    uint32_t m_ackTimeoutValue;                           //!< the Ack timeout value
};

template <typename TxopType>
void
TxopTest<TxopType>::QueueTx(uint64_t txTime, uint64_t expectedGrantTime)
{
    m_expectedGrants.emplace_back(txTime, expectedGrantTime);
}

template <typename TxopType>
TxopTest<TxopType>::TxopTest(ChannelAccessManagerTest<TxopType>* test, uint32_t i)
    : m_test(test),
      m_i(i)
{
}

template <typename TxopType>
void
TxopTest<TxopType>::DoDispose()
{
    m_test = nullptr;
    TxopType::DoDispose();
}

template <typename TxopType>
void
TxopTest<TxopType>::NotifyChannelAccessed(uint8_t linkId, Time txopDuration)
{
    Txop::GetLink(0).access = Txop::NOT_REQUESTED;
    m_test->NotifyAccessGranted(m_i);
}

template <typename TxopType>
void
TxopTest<TxopType>::GenerateBackoff(uint8_t linkId)
{
    m_test->GenerateBackoff(m_i);
}

template <typename TxopType>
bool
TxopTest<TxopType>::HasFramesToTransmit(uint8_t linkId)
{
    return !m_expectedGrants.empty();
}

template <typename TxopType>
void
TxopTest<TxopType>::NotifySleep(uint8_t linkId)
{
}

template <typename TxopType>
void
TxopTest<TxopType>::NotifyWakeUp(uint8_t linkId)
{
}

template <typename TxopType>
ChannelAccessManagerTest<TxopType>::ChannelAccessManagerTest()
    : TestCase("ChannelAccessManager")
{
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::NotifyAccessGranted(uint32_t i)
{
    Ptr<TxopTest<TxopType>> state = m_txop[i];
    NS_TEST_EXPECT_MSG_EQ(state->m_expectedGrants.empty(), false, "Have expected grants");
    if (!state->m_expectedGrants.empty())
    {
        std::pair<uint64_t, uint64_t> expected = state->m_expectedGrants.front();
        state->m_expectedGrants.pop_front();
        NS_TEST_EXPECT_MSG_EQ(Simulator::Now(),
                              MicroSeconds(expected.second),
                              "Expected access grant is now");
        m_ChannelAccessManager->NotifyTxStartNow(MicroSeconds(expected.first));
        m_ChannelAccessManager->NotifyAckTimeoutStartNow(
            MicroSeconds(m_ackTimeoutValue + expected.first));
    }
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddTxEvt(uint64_t at, uint64_t duration)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifyTxStartNow,
                        m_ChannelAccessManager,
                        MicroSeconds(duration));
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::NotifyInternalCollision(Ptr<TxopTest<TxopType>> state)
{
    NS_TEST_EXPECT_MSG_EQ(state->m_expectedInternalCollision.empty(),
                          false,
                          "Have expected internal collisions");
    if (!state->m_expectedInternalCollision.empty())
    {
        struct TxopTest<TxopType>::ExpectedBackoff expected =
            state->m_expectedInternalCollision.front();
        state->m_expectedInternalCollision.pop_front();
        NS_TEST_EXPECT_MSG_EQ(Simulator::Now(),
                              MicroSeconds(expected.at),
                              "Expected internal collision time is now");
        state->StartBackoffNow(expected.nSlots, 0);
    }
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::GenerateBackoff(uint32_t i)
{
    Ptr<TxopTest<TxopType>> state = m_txop[i];
    NS_TEST_EXPECT_MSG_EQ(state->m_expectedBackoff.empty(), false, "Have expected backoffs");
    if (!state->m_expectedBackoff.empty())
    {
        struct TxopTest<TxopType>::ExpectedBackoff expected = state->m_expectedBackoff.front();
        state->m_expectedBackoff.pop_front();
        NS_TEST_EXPECT_MSG_EQ(Simulator::Now(),
                              MicroSeconds(expected.at),
                              "Expected backoff is now");
        state->StartBackoffNow(expected.nSlots, 0);
    }
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::NotifyChannelSwitching()
{
    for (auto& state : m_txop)
    {
        if (!state->m_expectedGrants.empty())
        {
            std::pair<uint64_t, uint64_t> expected = state->m_expectedGrants.front();
            state->m_expectedGrants.pop_front();
            NS_TEST_EXPECT_MSG_EQ(Simulator::Now(),
                                  MicroSeconds(expected.second),
                                  "Expected grant is now");
        }
        state->Txop::GetLink(0).access = Txop::NOT_REQUESTED;
    }
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::ExpectInternalCollision(uint64_t time,
                                                            uint32_t nSlots,
                                                            uint32_t from)
{
    Ptr<TxopTest<TxopType>> state = m_txop[from];
    struct TxopTest<TxopType>::ExpectedBackoff col;
    col.at = time;
    col.nSlots = nSlots;
    state->m_expectedInternalCollision.push_back(col);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::ExpectBackoff(uint64_t time, uint32_t nSlots, uint32_t from)
{
    Ptr<TxopTest<TxopType>> state = m_txop[from];
    struct TxopTest<TxopType>::ExpectedBackoff backoff;
    backoff.at = time;
    backoff.nSlots = nSlots;
    state->m_expectedBackoff.push_back(backoff);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::ExpectBusy(uint64_t time, bool busy)
{
    Simulator::Schedule(MicroSeconds(time) - Now(),
                        &ChannelAccessManagerTest::DoCheckBusy,
                        this,
                        busy);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::DoCheckBusy(bool busy)
{
    NS_TEST_EXPECT_MSG_EQ(m_ChannelAccessManager->IsBusy(), busy, "Incorrect busy/idle state");
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::StartTest(uint64_t slotTime,
                                              uint64_t sifs,
                                              uint64_t eifsNoDifsNoSifs,
                                              uint32_t ackTimeoutValue,
                                              MHz_u chWidth)
{
    m_ChannelAccessManager = CreateObject<ChannelAccessManagerStub>();
    m_feManager = CreateObject<FrameExchangeManagerStub<TxopType>>(this);
    m_ChannelAccessManager->SetupFrameExchangeManager(m_feManager);
    m_ChannelAccessManager->SetSlot(MicroSeconds(slotTime));
    m_ChannelAccessManager->SetSifs(MicroSeconds(sifs));
    m_ChannelAccessManager->SetEifsNoDifs(MicroSeconds(eifsNoDifsNoSifs + sifs));
    m_ackTimeoutValue = ackTimeoutValue;
    // the purpose of the following operations is to initialize the last busy struct
    // of the ChannelAccessManager. Indeed, InitLastBusyStructs(), which is called by
    // SetupPhyListener(), requires an attached PHY to determine the channel types
    // to initialize
    m_phy = CreateObject<SpectrumWifiPhy>();
    m_phy->SetInterferenceHelper(CreateObject<InterferenceHelper>());
    m_phy->AddChannel(CreateObject<MultiModelSpectrumChannel>());
    m_phy->SetOperatingChannel(WifiPhy::ChannelTuple{0, chWidth, WIFI_PHY_BAND_UNSPECIFIED, 0});
    m_phy->ConfigureStandard(WIFI_STANDARD_80211ac); // required to use 160 MHz channels
    m_ChannelAccessManager->SetupPhyListener(m_phy);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddTxop(uint32_t aifsn)
{
    Ptr<TxopTest<TxopType>> txop = CreateObject<TxopTest<TxopType>>(this, m_txop.size());
    m_txop.push_back(txop);
    m_ChannelAccessManager->Add(txop);
    // the following causes the creation of a link for the txop object
    auto mac = CreateObjectWithAttributes<AdhocWifiMac>(
        "Txop",
        PointerValue(CreateObjectWithAttributes<Txop>("AcIndex", StringValue("AC_BE_NQOS"))));
    mac->SetWifiPhys({nullptr});
    txop->SetWifiMac(mac);
    txop->SetAifsn(aifsn);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::EndTest()
{
    Simulator::Run();

    for (auto i = m_txop.begin(); i != m_txop.end(); i++)
    {
        Ptr<TxopTest<TxopType>> state = *i;
        NS_TEST_EXPECT_MSG_EQ(state->m_expectedGrants.empty(), true, "Have no expected grants");
        NS_TEST_EXPECT_MSG_EQ(state->m_expectedInternalCollision.empty(),
                              true,
                              "Have no internal collisions");
        NS_TEST_EXPECT_MSG_EQ(state->m_expectedBackoff.empty(), true, "Have no expected backoffs");
        state->Dispose();
        state = nullptr;
    }
    m_txop.clear();

    m_ChannelAccessManager->RemovePhyListener(m_phy);
    m_phy->Dispose();
    m_ChannelAccessManager->Dispose();
    m_ChannelAccessManager = nullptr;
    m_feManager = nullptr;
    Simulator::Destroy();
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddRxOkEvt(uint64_t at, uint64_t duration)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifyRxStartNow,
                        m_ChannelAccessManager,
                        MicroSeconds(duration));
    Simulator::Schedule(MicroSeconds(at + duration) - Now(),
                        &ChannelAccessManager::NotifyRxEndOkNow,
                        m_ChannelAccessManager);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddRxInsideSifsEvt(uint64_t at, uint64_t duration)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifyRxStartNow,
                        m_ChannelAccessManager,
                        MicroSeconds(duration));
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddRxErrorEvt(uint64_t at, uint64_t duration)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifyRxStartNow,
                        m_ChannelAccessManager,
                        MicroSeconds(duration));
    Simulator::Schedule(MicroSeconds(at + duration) - Now(),
                        &ChannelAccessManager::NotifyRxEndErrorNow,
                        m_ChannelAccessManager);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddRxErrorEvt(uint64_t at,
                                                  uint64_t duration,
                                                  uint64_t timeUntilError)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifyRxStartNow,
                        m_ChannelAccessManager,
                        MicroSeconds(duration));
    Simulator::Schedule(MicroSeconds(at + timeUntilError) - Now(),
                        &ChannelAccessManager::NotifyRxEndErrorNow,
                        m_ChannelAccessManager);
    Simulator::Schedule(MicroSeconds(at + timeUntilError) - Now(),
                        &ChannelAccessManager::NotifyCcaBusyStartNow,
                        m_ChannelAccessManager,
                        MicroSeconds(duration - timeUntilError),
                        WIFI_CHANLIST_PRIMARY,
                        std::vector<Time>{});
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddNavReset(uint64_t at, uint64_t duration)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifyNavResetNow,
                        m_ChannelAccessManager,
                        MicroSeconds(duration));
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddNavStart(uint64_t at, uint64_t duration)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifyNavStartNow,
                        m_ChannelAccessManager,
                        MicroSeconds(duration));
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddAckTimeoutReset(uint64_t at)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifyAckTimeoutResetNow,
                        m_ChannelAccessManager);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddAccessRequest(uint64_t at,
                                                     uint64_t txTime,
                                                     uint64_t expectedGrantTime,
                                                     uint32_t from)
{
    AddAccessRequestWithSuccessfulAck(at, txTime, expectedGrantTime, 0, from);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddAccessRequestWithAckTimeout(uint64_t at,
                                                                   uint64_t txTime,
                                                                   uint64_t expectedGrantTime,
                                                                   uint32_t from)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManagerTest::DoAccessRequest,
                        this,
                        txTime,
                        expectedGrantTime,
                        m_txop[from]);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddAccessRequestWithSuccessfulAck(uint64_t at,
                                                                      uint64_t txTime,
                                                                      uint64_t expectedGrantTime,
                                                                      uint32_t ackDelay,
                                                                      uint32_t from)
{
    NS_ASSERT(ackDelay < m_ackTimeoutValue);
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManagerTest::DoAccessRequest,
                        this,
                        txTime,
                        expectedGrantTime,
                        m_txop[from]);
    AddAckTimeoutReset(expectedGrantTime + txTime + ackDelay);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::DoAccessRequest(uint64_t txTime,
                                                    uint64_t expectedGrantTime,
                                                    Ptr<TxopTest<TxopType>> state)
{
    auto hadFramesToTransmit = state->HasFramesToTransmit(SINGLE_LINK_OP_ID);
    state->QueueTx(txTime, expectedGrantTime);
    if (m_ChannelAccessManager->NeedBackoffUponAccess(state, hadFramesToTransmit, true))
    {
        state->GenerateBackoff(0);
    }
    m_ChannelAccessManager->RequestAccess(state);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddCcaBusyEvt(uint64_t at,
                                                  uint64_t duration,
                                                  WifiChannelListType channelType,
                                                  const std::vector<Time>& per20MhzDurations)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifyCcaBusyStartNow,
                        m_ChannelAccessManager,
                        MicroSeconds(duration),
                        channelType,
                        per20MhzDurations);
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddSwitchingEvt(uint64_t at, uint64_t duration)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifySwitchingStartNow,
                        m_ChannelAccessManager,
                        nullptr,
                        MicroSeconds(duration));
}

template <typename TxopType>
void
ChannelAccessManagerTest<TxopType>::AddRxStartEvt(uint64_t at, uint64_t duration)
{
    Simulator::Schedule(MicroSeconds(at) - Now(),
                        &ChannelAccessManager::NotifyRxStartNow,
                        m_ChannelAccessManager,
                        MicroSeconds(duration));
}

/*
 * Specialization of DoRun () method for DCF
 */
template <>
void
ChannelAccessManagerTest<Txop>::DoRun()
{
    // DCF immediate access (no backoff)
    //  1      4       5    6      8     11      12
    //  | sifs | aifsn | tx | idle | sifs | aifsn | tx |
    //
    StartTest(1, 3, 10);
    AddTxop(1);
    AddAccessRequest(1, 1, 5, 0);
    AddAccessRequest(8, 2, 12, 0);
    EndTest();
    // Check that receiving inside SIFS shall be cancelled properly:
    //  1      4       5    6      9    10     14     17      18
    //  | sifs | aifsn | tx | sifs | ack | idle | sifs | aifsn | tx |
    //                        |
    //                        7 start rx
    //

    StartTest(1, 3, 10);
    AddTxop(1);
    AddAccessRequest(1, 1, 5, 0);
    AddRxInsideSifsEvt(7, 10);
    AddTxEvt(9, 1);
    AddAccessRequest(14, 2, 18, 0);
    EndTest();
    // The test below mainly intends to test the case where the medium
    // becomes busy in the middle of a backoff slot: the backoff counter
    // must not be decremented for this backoff slot. This is the case
    // below for the backoff slot starting at time 78us.
    //
    //  20          60     66      70        74        78  80    100     106      110      114 118
    //  120
    //   |    rx     | sifs | aifsn | bslot0  | bslot1  |   | rx   | sifs  |  aifsn | bslot2 |
    //   bslot3 | tx  |
    //        |
    //       30 request access. backoff slots: 4

    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxOkEvt(20, 40);
    AddRxOkEvt(80, 20);
    AddAccessRequest(30, 2, 118, 0);
    ExpectBackoff(30, 4, 0); // backoff: 4 slots
    EndTest();
    // Test the case where the backoff slots is zero.
    //
    //  20          60     66      70   72
    //   |    rx     | sifs | aifsn | tx |
    //        |
    //       30 request access. backoff slots: 0

    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxOkEvt(20, 40);
    AddAccessRequest(30, 2, 70, 0);
    ExpectBackoff(30, 0, 0); // backoff: 0 slots
    EndTest();
    // Test shows when two frames are received without interval between
    // them:
    //  20          60         100   106     110  112
    //   |    rx     |    rx     |sifs | aifsn | tx |
    //        |
    //       30 request access. backoff slots: 0

    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxOkEvt(20, 40);
    AddRxOkEvt(60, 40);
    AddAccessRequest(30, 2, 110, 0);
    ExpectBackoff(30, 0, 0); // backoff: 0 slots
    EndTest();

    // Requesting access within SIFS interval (DCF immediate access)
    //
    //  20    60     62     68      72
    //   | rx  | idle | sifs | aifsn | tx |
    //
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxOkEvt(20, 40);
    AddAccessRequest(62, 2, 72, 0);
    EndTest();

    // Requesting access after DIFS (DCF immediate access)
    //
    //   20   60     70     76      80
    //   | rx  | idle | sifs | aifsn | tx |
    //
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxOkEvt(20, 40);
    AddAccessRequest(70, 2, 80, 0);
    EndTest();

    // Test an EIFS
    //
    //  20          60     66           76             86       90       94       98       102   106
    //   |    rx     | sifs | acktxttime | sifs + aifsn | bslot0 | bslot1 | bslot2 | bslot3 | tx |
    //        |      | <------eifs------>|
    //       30 request access. backoff slots: 4
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxErrorEvt(20, 40);
    AddAccessRequest(30, 2, 102, 0);
    ExpectBackoff(30, 4, 0); // backoff: 4 slots
    EndTest();

    // Test DCF immediate access after an EIFS (EIFS is greater)
    //
    //  20          60     66           76             86
    //               | <----+-eifs------>|
    //   |    rx     | sifs | acktxttime | sifs + aifsn | tx |
    //                             | sifs + aifsn |
    //             request access 70             80
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxErrorEvt(20, 40);
    AddAccessRequest(70, 2, 86, 0);
    EndTest();

    // Test that channel stays busy for first frame's duration after Rx error
    //
    //  20          60
    //   |    rx     |
    //        |
    //       40 force Rx error
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxErrorEvt(20, 40, 20); // At time 20, start reception for 40, but force error 20 into frame
    ExpectBusy(41, true);      // channel should remain busy for remaining duration
    ExpectBusy(59, true);
    ExpectBusy(61, false);
    EndTest();

    // Test an EIFS which is interrupted by a successful transmission.
    //
    //  20          60      66  69     75     81      85       89       93       97      101  103
    //   |    rx     | sifs  |   |  rx  | sifs | aifsn | bslot0 | bslot1 | bslot2 | bslot3 | tx |
    //        |      | <--eifs-->|
    //       30 request access. backoff slots: 4
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxErrorEvt(20, 40);
    AddAccessRequest(30, 2, 101, 0);
    ExpectBackoff(30, 4, 0); // backoff: 4 slots
    AddRxOkEvt(69, 6);
    EndTest();

    // Test two DCFs which suffer an internal collision. the first DCF has a higher
    // priority than the second DCF.
    //
    //      20          60      66      70       74       78    88
    // DCF0  |    rx     | sifs  | aifsn | bslot0 | bslot1 | tx  |
    // DCF1  |    rx     | sifs  | aifsn | aifsn  | aifsn  |     | sifs | aifsn | aifsn | aifsn |
    // bslot |  tx  |
    //                                                                 94      98     102     106
    //                                                                 110    112
    StartTest(4, 6, 10);
    AddTxop(1); // high priority DCF
    AddTxop(3); // low priority DCF
    AddRxOkEvt(20, 40);
    AddAccessRequest(30, 10, 78, 0);
    ExpectBackoff(30, 2, 0); // backoff: 2 slot
    AddAccessRequest(40, 2, 110, 1);
    ExpectBackoff(40, 0, 1);           // backoff: 0 slot
    ExpectInternalCollision(78, 1, 1); // backoff: 1 slot
    EndTest();

    // Test of AckTimeout handling: First queue requests access and ack procedure fails,
    // inside the Ack timeout second queue with higher priority requests access.
    //
    //            20     26      34       54            74     80
    // DCF1 - low  | sifs | aifsn |   tx   | Ack timeout | sifs |       |
    // DCF0 - high |                              |      | sifs |  tx   |
    //                                            ^ request access
    StartTest(4, 6, 10);
    AddTxop(0); // high priority DCF
    AddTxop(2); // low priority DCF
    AddAccessRequestWithAckTimeout(20, 20, 34, 1);
    AddAccessRequest(64, 10, 80, 0);
    EndTest();

    // Test of AckTimeout handling:
    //
    // First queue requests access and Ack is 2 us delayed (got Ack interval at the picture),
    // inside this interval second queue with higher priority requests access.
    //
    //            20     26      34           54        56     62
    // DCF1 - low  | sifs | aifsn |     tx     | got Ack | sifs |       |
    // DCF0 - high |                                |    | sifs |  tx   |
    //                                              ^ request access
    StartTest(4, 6, 10);
    AddTxop(0); // high priority DCF
    AddTxop(2); // low priority DCF
    AddAccessRequestWithSuccessfulAck(20, 20, 34, 2, 1);
    AddAccessRequest(55, 10, 62, 0);
    EndTest();

    // Repeat the same but with one queue:
    //       20     26      34         54     60    62     68      76       80
    //  DCF0  | sifs | aifsn |    tx    | sifs | Ack | sifs | aifsn | bslot0 | tx |
    //                                            ^ request access
    StartTest(4, 6, 10);
    AddTxop(2);
    AddAccessRequest(20, 20, 34, 0);
    AddRxOkEvt(60, 2); // Ack
    AddAccessRequest(61, 10, 80, 0);
    ExpectBackoff(61, 1, 0); // 1 slot
    EndTest();

    // test simple NAV count. This scenario models a simple Data+Ack handshake
    // where the data rate used for the Ack is higher than expected by the Data source
    // so, the data exchange completes before the end of NAV.
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxOkEvt(20, 40);
    AddNavStart(60, 15);
    AddRxOkEvt(66, 5);
    AddNavStart(71, 0);
    AddAccessRequest(30, 10, 93, 0);
    ExpectBackoff(30, 2, 0); // backoff: 2 slots
    EndTest();

    // test more complex NAV handling by a CF-poll. This scenario models a
    // simple Data+Ack handshake interrupted by a CF-poll which resets the
    // NAV counter.
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxOkEvt(20, 40);
    AddNavStart(60, 15);
    AddRxOkEvt(66, 5);
    AddNavReset(71, 2);
    AddAccessRequest(30, 10, 91, 0);
    ExpectBackoff(30, 2, 0); // backoff: 2 slots
    EndTest();

    //  20         60         80     86      94
    //   |    rx    |   idle   | sifs | aifsn |    tx    |
    //                         ^ request access
    StartTest(4, 6, 10);
    AddTxop(2);
    AddRxOkEvt(20, 40);
    AddAccessRequest(80, 10, 94, 0);
    EndTest();

    StartTest(4, 6, 10);
    AddTxop(2);
    AddRxOkEvt(20, 40);
    AddRxOkEvt(78, 8);
    AddAccessRequest(30, 50, 108, 0);
    ExpectBackoff(30, 3, 0); // backoff: 3 slots
    EndTest();

    // Channel switching tests

    //  0          20     21     24      25   26
    //  | switching | idle | sifs | aifsn | tx |
    //                     ^ access request.
    StartTest(1, 3, 10);
    AddTxop(1);
    AddSwitchingEvt(0, 20);
    AddAccessRequest(21, 1, 25, 0);
    EndTest();

    //  20          40       50     53      54       55        56   57
    //   | switching |  busy  | sifs | aifsn | bslot0 | bslot 1 | tx |
    //         |          |
    //        30 busy.   45 access request.
    //
    StartTest(1, 3, 10);
    AddTxop(1);
    AddSwitchingEvt(20, 20);
    AddCcaBusyEvt(30, 20);
    ExpectBackoff(45, 2, 0); // backoff: 2 slots
    AddAccessRequest(45, 1, 56, 0);
    EndTest();

    //  20     30          50     51     54      55   56
    //   |  rx  | switching | idle | sifs | aifsn | tx |
    //                             ^ access request.
    //
    StartTest(1, 3, 10);
    AddTxop(1);
    AddRxStartEvt(20, 40);
    AddSwitchingEvt(30, 20);
    AddAccessRequest(51, 1, 55, 0);
    EndTest();

    //  20     30          50     51     54      55   56
    //   | busy | switching | idle | sifs | aifsn | tx |
    //                             ^ access request.
    //
    StartTest(1, 3, 10);
    AddTxop(1);
    AddCcaBusyEvt(20, 40);
    AddSwitchingEvt(30, 20);
    AddAccessRequest(51, 1, 55, 0);
    EndTest();

    //  20      30          50     51     54      55   56
    //   |  nav  | switching | idle | sifs | aifsn | tx |
    //                              ^ access request.
    //
    StartTest(1, 3, 10);
    AddTxop(1);
    AddNavStart(20, 40);
    AddSwitchingEvt(30, 20);
    AddAccessRequest(51, 1, 55, 0);
    EndTest();

    //  20     23      24      44             54          59     60     63      64   65
    //   | sifs | aifsn |  tx   | Ack timeout  | switching | idle | sifs | aifsn | tx |
    //                                 |                          |
    //                                49 access request.          ^ access request.
    //
    StartTest(1, 3, 10);
    AddTxop(1);
    AddAccessRequestWithAckTimeout(20, 20, 24, 0);
    AddAccessRequest(49, 1, 54, 0);
    AddSwitchingEvt(54, 5);
    AddAccessRequest(60, 1, 64, 0);
    EndTest();

    //  20         60     66      70       74       78  80         100    101    107     111  113
    //   |    rx    | sifs | aifsn | bslot0 | bslot1 |   | switching | idle | sifs | aifsn | tx |
    //        |                                                             |
    //       30 access request.                                             ^ access request.
    //
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxOkEvt(20, 40);
    AddAccessRequest(30, 2, 80, 0);
    ExpectBackoff(30, 4, 0); // backoff: 4 slots
    AddSwitchingEvt(80, 20);
    AddAccessRequest(101, 2, 111, 0);
    EndTest();
}

/*
 * Specialization of DoRun () method for EDCA
 */
template <>
void
ChannelAccessManagerTest<QosTxop>::DoRun()
{
    // Check alignment at slot boundary after successful reception (backoff = 0).
    // Also, check that CCA BUSY on a secondary channel does not affect channel access:
    //    20     50     56      60     80
    //            |   cca_busy   |
    //     |  rx  | sifs | aifsn |  tx  |
    //                |
    //               52 request access
    StartTest(4, 6, 10, 20, MHz_u{40});
    AddTxop(1);
    AddRxOkEvt(20, 30);
    AddCcaBusyEvt(50, 10, WIFI_CHANLIST_SECONDARY);
    AddAccessRequest(52, 20, 60, 0);
    EndTest();

    // Check alignment at slot boundary after successful reception (backoff = 0).
    // Also, check that CCA BUSY on a secondary channel does not affect channel access:
    //    20     50     56      60     80
    //            |   cca_busy   |
    //     |  rx  | sifs | aifsn |  tx  |
    //                       |
    //                      58 request access
    StartTest(4, 6, 10, 20, MHz_u{80});
    AddTxop(1);
    AddRxOkEvt(20, 30);
    AddCcaBusyEvt(50, 10, WIFI_CHANLIST_SECONDARY);
    AddAccessRequest(58, 20, 60, 0);
    EndTest();

    // Check alignment at slot boundary after successful reception (backoff = 0).
    // Also, check that CCA BUSY on a secondary channel does not affect channel access:
    //    20     50     56      60     64     84
    //            |      cca_busy       |
    //     |  rx  | sifs | aifsn | idle |  tx  |
    //                               |
    //                              62 request access
    StartTest(4, 6, 10, 20, MHz_u{80});
    AddTxop(1);
    AddRxOkEvt(20, 30);
    AddCcaBusyEvt(50, 14, WIFI_CHANLIST_SECONDARY40);
    AddAccessRequest(62, 20, 64, 0);
    EndTest();

    // Check alignment at slot boundary after failed reception (backoff = 0).
    // Also, check that CCA BUSY on a secondary channel does not affect channel access:
    //  20         50     56           66             76     96
    //              |             cca_busy             |
    //   |          | <------eifs------>|              |      |
    //   |    rx    | sifs | acktxttime | sifs + aifsn |  tx  |
    //                   |
    //                  55 request access
    StartTest(4, 6, 10, 20, MHz_u{160});
    AddTxop(1);
    AddRxErrorEvt(20, 30);
    AddCcaBusyEvt(50, 26, WIFI_CHANLIST_SECONDARY);
    AddAccessRequest(55, 20, 76, 0);
    EndTest();

    // Check alignment at slot boundary after failed reception (backoff = 0).
    // Also, check that CCA BUSY on a secondary channel does not affect channel access:
    //  20         50     56           66             76     96
    //              |             cca_busy             |
    //   |          | <------eifs------>|              |      |
    //   |    rx    | sifs | acktxttime | sifs + aifsn |  tx  |
    //                                        |
    //                                       70 request access
    StartTest(4, 6, 10, 20, MHz_u{160});
    AddTxop(1);
    AddRxErrorEvt(20, 30);
    AddCcaBusyEvt(50, 26, WIFI_CHANLIST_SECONDARY40);
    AddAccessRequest(70, 20, 76, 0);
    EndTest();

    // Check alignment at slot boundary after failed reception (backoff = 0).
    // Also, check that CCA BUSY on a secondary channel does not affect channel access:
    //  20         50     56           66             76     84
    //              |             cca_busy                    |
    //   |          | <------eifs------>|              |      |
    //   |    rx    | sifs | acktxttime | sifs + aifsn | idle |  tx  |
    //                                                     |
    //                                                    82 request access
    StartTest(4, 6, 10, 20, MHz_u{160});
    AddTxop(1);
    AddRxErrorEvt(20, 30);
    AddCcaBusyEvt(50, 34, WIFI_CHANLIST_SECONDARY80);
    AddAccessRequest(82, 20, 84, 0);
    EndTest();

    // Check backoff decrement at slot boundaries. Medium idle during backoff
    //  20           50     56      60         64         68         72         76     96
    //   |     rx     | sifs | aifsn |   idle   |   idle   |   idle   |   idle   |  tx  |
    //      |                        |          |          |          |
    //     30 request access.    decrement  decrement  decrement  decrement
    //        backoff slots: 4    slots: 3   slots: 2   slots: 1   slots: 0
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxOkEvt(20, 30);
    AddAccessRequest(30, 20, 76, 0);
    ExpectBackoff(30, 4, 0);
    EndTest();

    // Check backoff decrement at slot boundaries. Medium becomes busy during backoff
    //  20           50     56      60     61     71     77      81         85     87     97    103
    //  107    127
    //   |     rx     | sifs | aifsn | idle |  rx  | sifs | aifsn |   idle   | idle |  rx  | sifs |
    //   aifsn |  tx  |
    //      |                        |                            |          |
    //     30 request access.    decrement                    decrement  decrement
    //        backoff slots: 3    slots: 2                     slots: 1   slots: 0
    StartTest(4, 6, 10);
    AddTxop(1);
    AddRxOkEvt(20, 30);
    AddRxOkEvt(61, 10);
    AddRxOkEvt(87, 10);
    AddAccessRequest(30, 20, 107, 0);
    ExpectBackoff(30, 3, 0);
    EndTest();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the calculation of the largest idle primary channel performed by
 * ChannelAccessManager::GetLargestIdlePrimaryChannel().
 *
 * In every test, the ChannelAccessManager is notified of a CCA_BUSY period and
 * subsequently of the start of RX. The value returned by GetLargestIdlePrimaryChannel()
 * is checked at different times and for different intervals. All the possible
 * combinations of operating channel width and busy channel type are tested.
 */
class LargestIdlePrimaryChannelTest : public TestCase
{
  public:
    LargestIdlePrimaryChannelTest();
    ~LargestIdlePrimaryChannelTest() override = default;

  private:
    void DoRun() override;

    /**
     * Test a specific combination of operating channel width and busy channel type.
     *
     * @param chWidth the operating channel width
     * @param busyChannel the busy channel type
     */
    void RunOne(MHz_u chWidth, WifiChannelListType busyChannel);

    Ptr<ChannelAccessManager> m_cam; //!< channel access manager
    Ptr<SpectrumWifiPhy> m_phy;      //!< PHY object
};

LargestIdlePrimaryChannelTest::LargestIdlePrimaryChannelTest()
    : TestCase("Check calculation of the largest idle primary channel")
{
}

void
LargestIdlePrimaryChannelTest::RunOne(MHz_u chWidth, WifiChannelListType busyChannel)
{
    /**
     *                 <  Interval1  >< Interval2 >
     *                                <     Interval3   >
     *                                       < Interval4>       < Interval5 >
     *                                                       <  Interval6   >
     * --------|-------^--------------^------------^-----^------^------------^---
     * P20     |       |              |            |     |  RX  |            |
     * --------|-------|-----IDLE-----|----IDLE----|-----|------|------------|---
     * S20     |       |              |            |     |      |    IDLE    |
     * --------|-------v--------------v------------v-----|------|------------|---
     * S40     |               |  CCA_BUSY   |   IDLE    |      |            |
     * --------|-----------------------------|-----------|------|------------|---
     * S80     |                             |           |      |            |
     * --------|----------------------|------v-----|-----v------|------------|---
     *       start     Check times:   t1           t2           t3           t5
     *                                                          t4           t6
     */

    Time start = Simulator::Now();

    // After 1ms, we are notified of CCA_BUSY for 1ms on the given channel
    Time ccaBusyStartDelay = MilliSeconds(1);
    Time ccaBusyDuration = MilliSeconds(1);
    Simulator::Schedule(
        ccaBusyStartDelay,
        &ChannelAccessManager::NotifyCcaBusyStartNow,
        m_cam,
        ccaBusyDuration,
        busyChannel,
        std::vector<Time>(chWidth == MHz_u{20} ? 0 : Count20MHzSubchannels(chWidth), Seconds(0)));

    // During any interval ending within CCA_BUSY period, the idle channel is the
    // primary channel contiguous to the busy secondary channel, if the busy channel
    // is a secondary channel, or there is no idle channel, otherwise.
    const auto idleWidth = (busyChannel == WifiChannelListType::WIFI_CHANLIST_PRIMARY)
                               ? MHz_u{0}
                               : ((1 << (busyChannel - 1)) * MHz_u{20});

    Time checkTime1 = start + ccaBusyStartDelay + ccaBusyDuration / 2;
    Simulator::Schedule(checkTime1 - start, [=, this]() {
        Time interval1 = (ccaBusyStartDelay + ccaBusyDuration) / 2;
        NS_TEST_EXPECT_MSG_EQ(m_cam->GetLargestIdlePrimaryChannel(interval1, checkTime1),
                              idleWidth,
                              "Incorrect width of the idle channel in an interval "
                                  << "ending within CCA_BUSY (channel width: " << chWidth
                                  << " MHz, busy channel: " << busyChannel << ")");
    });

    // During any interval starting within CCA_BUSY period, the idle channel is the
    // same as the previous case
    Time ccaBusyRxInterval = MilliSeconds(1);
    Time checkTime2 = start + ccaBusyStartDelay + ccaBusyDuration + ccaBusyRxInterval / 2;
    Simulator::Schedule(checkTime2 - start, [=, this]() {
        Time interval2 = (ccaBusyDuration + ccaBusyRxInterval) / 2;
        NS_TEST_EXPECT_MSG_EQ(m_cam->GetLargestIdlePrimaryChannel(interval2, checkTime2),
                              idleWidth,
                              "Incorrect width of the idle channel in an interval "
                                  << "starting within CCA_BUSY (channel width: " << chWidth
                                  << " MHz, busy channel: " << busyChannel << ")");
    });

    // Notify RX start
    Time rxDuration = MilliSeconds(1);
    Simulator::Schedule(ccaBusyStartDelay + ccaBusyDuration + ccaBusyRxInterval,
                        &ChannelAccessManager::NotifyRxStartNow,
                        m_cam,
                        rxDuration);

    // At RX end, we check the status of the channel during an interval immediately
    // preceding RX start and overlapping the CCA_BUSY period.
    Time checkTime3 = start + ccaBusyStartDelay + ccaBusyDuration + ccaBusyRxInterval + rxDuration;
    Simulator::Schedule(checkTime3 - start, [=, this]() {
        Time interval3 = ccaBusyDuration / 2 + ccaBusyRxInterval;
        Time end3 = checkTime3 - rxDuration;
        NS_TEST_EXPECT_MSG_EQ(m_cam->GetLargestIdlePrimaryChannel(interval3, end3),
                              idleWidth,
                              "Incorrect width of the idle channel in an interval "
                                  << "preceding RX start and overlapping CCA_BUSY "
                                  << "(channel width: " << chWidth
                                  << " MHz, busy channel: " << busyChannel << ")");
    });

    // At RX end, we check the status of the channel during the interval following
    // the CCA_BUSY period and preceding RX start. The entire operating channel is idle.
    const Time& checkTime4 = checkTime3;
    Simulator::Schedule(checkTime4 - start, [=, this]() {
        const Time& interval4 = ccaBusyRxInterval;
        Time end4 = checkTime4 - rxDuration;
        NS_TEST_EXPECT_MSG_EQ(m_cam->GetLargestIdlePrimaryChannel(interval4, end4),
                              chWidth,
                              "Incorrect width of the idle channel in the interval "
                                  << "following CCA_BUSY and preceding RX start (channel "
                                  << "width: " << chWidth << " MHz, busy channel: " << busyChannel
                                  << ")");
    });

    // After RX end, the entire operating channel is idle if the interval does not
    // overlap the RX period
    Time interval5 = MilliSeconds(1);
    Time checkTime5 = checkTime4 + interval5;
    Simulator::Schedule(checkTime5 - start, [=, this]() {
        NS_TEST_EXPECT_MSG_EQ(m_cam->GetLargestIdlePrimaryChannel(interval5, checkTime5),
                              chWidth,
                              "Incorrect width of the idle channel in an interval "
                                  << "following RX end (channel width: " << chWidth
                                  << " MHz, busy channel: " << busyChannel << ")");
    });

    // After RX end, no channel is idle if the interval overlaps the RX period
    const Time& checkTime6 = checkTime5;
    Simulator::Schedule(checkTime6 - start, [=, this]() {
        Time interval6 = interval5 + rxDuration / 2;
        NS_TEST_EXPECT_MSG_EQ(m_cam->GetLargestIdlePrimaryChannel(interval6, checkTime6),
                              MHz_u{0},
                              "Incorrect width of the idle channel in an interval "
                                  << "overlapping RX (channel width: " << chWidth
                                  << " MHz, busy channel: " << busyChannel << ")");
    });
}

void
LargestIdlePrimaryChannelTest::DoRun()
{
    m_cam = CreateObject<ChannelAccessManager>();
    uint16_t delay = 0;
    uint8_t channel = 0;
    std::list<WifiChannelListType> busyChannels;

    for (auto chWidth : {MHz_u{20}, MHz_u{40}, MHz_u{80}, MHz_u{160}})
    {
        busyChannels.push_back(static_cast<WifiChannelListType>(channel));

        for (const auto busyChannel : busyChannels)
        {
            Simulator::Schedule(Seconds(delay), [=, this]() {
                // reset PHY
                if (m_phy)
                {
                    m_cam->RemovePhyListener(m_phy);
                    m_phy->Dispose();
                }
                // create a new PHY operating on a channel of the current width
                m_phy = CreateObject<SpectrumWifiPhy>();
                m_phy->SetInterferenceHelper(CreateObject<InterferenceHelper>());
                m_phy->AddChannel(CreateObject<MultiModelSpectrumChannel>());
                m_phy->SetOperatingChannel(
                    WifiPhy::ChannelTuple{0, chWidth, WIFI_PHY_BAND_5GHZ, 0});
                m_phy->ConfigureStandard(WIFI_STANDARD_80211ax);
                // call SetupPhyListener to initialize the ChannelAccessManager
                // last busy structs
                m_cam->SetupPhyListener(m_phy);
                // run the tests
                RunOne(chWidth, busyChannel);
            });
            delay++;
        }
        channel++;
    }

    Simulator::Run();
    m_cam->RemovePhyListener(m_phy);
    m_phy->Dispose();
    m_cam->Dispose();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the GenerateBackoffIfTxopWithoutTx and ProactiveBackoff attributes of the
 *        ChannelAccessManager. The backoff values generated by the VO AC of the AP are checked.
 *
 * The GenerateBackoffIfTxopWithoutTx test checks the generation of backoff values when the
 * attribute is set to true. A QoS data frame is queued at the AP but the queue is blocked so
 * that the frame is not transmitted. A backoff value is kept being generated as long as the
 * frame is kept in the queue.
 *
 *                                                       Backoff                             Last
 * Backoff                 Backoff        Backoff        value #3,                          backoff
 * value #0                value #1       value #2       unblock queue                       value
 *  |              ┌─────┐    |              |              |              ┌─────┐   ┌────┐    |
 *  |       ┌───┐  │Assoc│    |    |Decrement|    |Decrement|    |Decrement│ADDBA│   │QoS │    |
 *  |       │ACK│  │Resp │    |AIFS| backoff |slot| backoff |slot| backoff │ Req │. .│data│    |
 * ──┬─────┬┴───┴──┴─────┴┬───┬────────────────────────────────────────────┴─────┴───┴────┴┬───┬──
 *   │Assoc│              │ACK│                                                            │ACK│
 *   │ Req │              └───┘                                                            └───┘
 *   └─────┘
 *
 * The ProactiveBackoff test checks the generation of backoff values when the attribute is set
 * to true. A noise is generated to trigger the generation of a new backoff value, provided
 * that the backoff counter is zero.
 *
 *
 * Backoff       Backoff                Backoff                     Backoff
 * value #0      value #1               value #2                    value #3
 *  |             |           ┌─────┐    |                           |
 *  |             |    ┌───┐  │Assoc│    |                           |
 *  |             |    │ACK│  │Resp │    |SIFS| noise | AIFS+backoff | noise |
 * ─────────────┬─────┬┴───┴──┴─────┴┬───┬──────────────────────────────────────────────────
 *              │Assoc│              │ACK│
 *              │ Req │              └───┘
 *              └─────┘
 */
class BackoffGenerationTest : public TestCase
{
  public:
    /**
     * Tested attributes
     */
    enum TestType : uint8_t
    {
        GEN_BACKOFF_IF_TXOP_NO_TX = 0,
        PROACTIVE_BACKOFF
    };

    /**
     * Constructor
     *
     * @param type the test type
     */
    BackoffGenerationTest(TestType type);

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);

    /**
     * Callback invoked when a new backoff value is generated by the given AC on the station.
     *
     * @param ac the AC index
     * @param backoff the generated backoff value
     * @param linkId the ID of the link for which the backoff value has been generated
     */
    void BackoffGenerated(AcIndex ac, uint32_t backoff, uint8_t linkId);

    /**
     * Indicate that a new backoff value has not been generated as expected.
     */
    void MissedBackoff();

    /**
     * Generate interference to make CCA busy.
     */
    void GenerateInterference();

    Ptr<ApWifiMac> m_apMac;                ///< AP wifi MAC
    Ptr<StaWifiMac> m_staMac;              ///< MAC of the non-AP STA
    bool m_generateBackoffIfTxopWithoutTx; ///< whether the GenerateBackoffIfTxopWithoutTx
                                           ///< attribute is set to true
    bool m_proactiveBackoff;              ///< whether the ProactiveBackoff attribute is set to true
    static constexpr uint8_t m_tid{6};    ///< TID of generated packet
    std::size_t m_nGenBackoff{0};         ///< number of generated backoff values
    std::size_t m_nExpectedGenBackoff{0}; ///< expected total number of generated backoff values
    EventId m_nextBackoffGen;             ///< timer elapsing when next backoff value is expected
                                          ///< to be generated
    Time m_assocReqStartTxTime{0};        ///< Association Request start TX time
    Time m_assocReqPpduHdrDuration{0};    ///< Association Request PPDU header TX duration
    std::size_t m_nAcks{0};               ///< number of transmitted Ack frames
    const Time m_interferenceDuration{MicroSeconds(10)}; ///< interference duration
    Ptr<PacketSocketClient> m_client; ///< client to be installed on the AP after association
};

BackoffGenerationTest::BackoffGenerationTest(TestType type)
    : TestCase("Check attributes impacting the generation of backoff values"),
      m_generateBackoffIfTxopWithoutTx(type == GEN_BACKOFF_IF_TXOP_NO_TX),
      m_proactiveBackoff(type == PROACTIVE_BACKOFF)
{
    if (m_proactiveBackoff)
    {
        m_nExpectedGenBackoff = 4;
    }
}

void
BackoffGenerationTest::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 10;

    Config::SetDefault("ns3::ChannelAccessManager::GenerateBackoffIfTxopWithoutTx",
                       BooleanValue(m_generateBackoffIfTxopWithoutTx));
    Config::SetDefault("ns3::ChannelAccessManager::ProactiveBackoff",
                       BooleanValue(m_proactiveBackoff));

    auto apNode = CreateObject<Node>();
    auto staNode = CreateObject<Node>();

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs0"),
                                 "ControlMode",
                                 StringValue("HtMcs0"));

    // MLDs are configured with three links
    SpectrumWifiPhyHelper phyHelper;
    phyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phyHelper.Set("ChannelSettings", StringValue("{36, 0, BAND_5GHZ, 0}"));
    phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>());

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(Ssid("ns-3-ssid")),
                "BeaconGeneration",
                BooleanValue(true));

    auto apDevice = DynamicCast<WifiNetDevice>(wifi.Install(phyHelper, mac, apNode).Get(0));

    mac.SetType("ns3::StaWifiMac",
                "Ssid",
                SsidValue(Ssid("ns-3-ssid")),
                "ActiveProbing",
                BooleanValue(false));

    auto staDevice = DynamicCast<WifiNetDevice>(wifi.Install(phyHelper, mac, staNode).Get(0));

    m_apMac = DynamicCast<ApWifiMac>(apDevice->GetMac());
    m_staMac = DynamicCast<StaWifiMac>(staDevice->GetMac());

    // Trace PSDUs passed to the PHY
    apDevice->GetPhy(SINGLE_LINK_OP_ID)
        ->TraceConnectWithoutContext("PhyTxPsduBegin",
                                     MakeCallback(&BackoffGenerationTest::Transmit, this));
    staDevice->GetPhy(SINGLE_LINK_OP_ID)
        ->TraceConnectWithoutContext("PhyTxPsduBegin",
                                     MakeCallback(&BackoffGenerationTest::Transmit, this));

    // Trace backoff generation
    m_apMac->GetQosTxop(AC_VO)->TraceConnectWithoutContext(
        "BackoffTrace",
        MakeCallback(&BackoffGenerationTest::BackoffGenerated, this).Bind(AC_VO));

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(NetDeviceContainer(apDevice), streamNumber);
    streamNumber += WifiHelper::AssignStreams(NetDeviceContainer(staDevice), streamNumber);

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));

    MobilityHelper mobility;
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNode);
    mobility.Install(staNode);

    // // install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(apNode);
    packetSocket.Install(staNode);

    // install a packet socket server on the non-AP station
    PacketSocketAddress srvAddr;
    srvAddr.SetSingleDevice(staDevice->GetIfIndex());
    srvAddr.SetProtocol(1);

    auto server = CreateObject<PacketSocketServer>();
    server->SetLocal(srvAddr);
    server->SetStartTime(Seconds(0));
    server->SetStopTime(Seconds(1));
    staNode->AddApplication(server);

    // Prepare a packet socket client that generates one packet at the AP. This client will be
    // installed as soon as association is completed
    PacketSocketAddress remoteAddr;
    remoteAddr.SetSingleDevice(apDevice->GetIfIndex());
    remoteAddr.SetPhysicalAddress(staDevice->GetAddress());
    remoteAddr.SetProtocol(1);

    m_client = CreateObject<PacketSocketClient>();
    m_client->SetAttribute("PacketSize", UintegerValue(1000));
    m_client->SetAttribute("MaxPackets", UintegerValue(1));
    m_client->SetAttribute("Interval", TimeValue(Time{0}));
    m_client->SetAttribute("Priority", UintegerValue(m_tid)); // AC VO
    m_client->SetRemote(remoteAddr);
    m_client->SetStartTime(Seconds(0));
    m_client->SetStopTime(Seconds(1));

    // Block VO queue so that the AP does not send QoS data frames
    m_apMac->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                 AC_VO,
                                                 {WIFI_QOSDATA_QUEUE},
                                                 m_staMac->GetAddress(),
                                                 m_apMac->GetAddress(),
                                                 {m_tid},
                                                 {SINGLE_LINK_OP_ID});
}

void
BackoffGenerationTest::DoRun()
{
    Simulator::Stop(Seconds(1));
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_nExpectedGenBackoff,
                          m_nGenBackoff,
                          "Unexpected total number of generated backoff values");

    Simulator::Destroy();
}

void
BackoffGenerationTest::Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW)
{
    auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap,
                                     txVector,
                                     m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());

    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        ss << std::setprecision(10) << psdu->GetHeader(0).GetTypeString();
        if (psdu->GetHeader(0).IsAction())
        {
            ss << " ";
            WifiActionHeader actionHdr;
            psdu->GetPayload(0)->PeekHeader(actionHdr);
            actionHdr.Print(ss);
        }
        ss << " #MPDUs " << psdu->GetNMpdus() << " duration/ID " << psdu->GetHeader(0).GetDuration()
           << " RA = " << psdu->GetAddr1() << " TA = " << psdu->GetAddr2()
           << " ADDR3 = " << psdu->GetHeader(0).GetAddr3()
           << " ToDS = " << psdu->GetHeader(0).IsToDs()
           << " FromDS = " << psdu->GetHeader(0).IsFromDs();
        if (psdu->GetHeader(0).IsAssocReq())
        {
            m_assocReqStartTxTime = Simulator::Now();
            m_assocReqPpduHdrDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
        }
        else if (psdu->GetHeader(0).IsAck())
        {
            m_nAcks++;
            if (m_nAcks == 2)
            {
                // generate a packet destined to the non-AP station (this packet is held because
                // the queue is blocked) as soon as association is completed
                Simulator::Schedule(txDuration,
                                    &Node::AddApplication,
                                    m_apMac->GetDevice()->GetNode(),
                                    m_client);
            }
        }
        else if (psdu->GetHeader(0).IsQosData())
        {
            ss << " seqNo = {";
            for (auto& mpdu : *PeekPointer(psdu))
            {
                ss << mpdu->GetHeader().GetSequenceNumber() << ",";
            }
            ss << "} TID = " << +psdu->GetHeader(0).GetQosTid();

            // after sending the QoS data frame, we expect one more backoff value to be generated
            // (at the end of the TXOP)
            m_nExpectedGenBackoff = m_nGenBackoff + 1;
        }
        NS_LOG_INFO(ss.str());
    }
    NS_LOG_INFO("TX duration = " << txDuration.As(Time::MS) << "  TXVECTOR = " << txVector << "\n");
}

void
BackoffGenerationTest::BackoffGenerated(AcIndex ac, uint32_t backoff, uint8_t linkId)
{
    NS_LOG_INFO("Backoff value " << backoff << " generated by AP on link " << +linkId << " for "
                                 << ac << "\n");

    // number of backoff values to generate when the GenerateBackoffIfTxopWithoutTx attribute is
    // set to true (can be any value >= 3)
    const std::size_t nValues = 5;

    switch (m_nGenBackoff)
    {
    case 0:
        NS_TEST_EXPECT_MSG_EQ(Simulator::Now().IsZero(),
                              true,
                              "First backoff value should be generated at initialization time");
        m_nGenBackoff++;
        return;
    case 1:
        if (m_generateBackoffIfTxopWithoutTx)
        {
            NS_TEST_EXPECT_MSG_EQ(m_apMac->IsAssociated(m_staMac->GetAddress()).has_value(),
                                  true,
                                  "Second backoff value should be generated after association");
        }
        if (m_proactiveBackoff)
        {
            NS_TEST_ASSERT_MSG_GT(
                Simulator::Now(),
                m_assocReqStartTxTime,
                "Second backoff value should be generated after AssocReq TX start time");
            NS_TEST_EXPECT_MSG_LT(Simulator::Now(),
                                  m_assocReqStartTxTime + m_assocReqPpduHdrDuration,
                                  "Second backoff value should be generated right after AssocReq "
                                  "PPDU payload starts");
        }
        break;
    case 2:
        if (m_proactiveBackoff)
        {
            NS_TEST_EXPECT_MSG_EQ(m_apMac->IsAssociated(m_staMac->GetAddress()).has_value(),
                                  true,
                                  "Third backoff value should be generated after association");
            // after a SIFS:
            Simulator::Schedule(m_apMac->GetWifiPhy(linkId)->GetSifs(), [=, this]() {
                // generate interference (lasting 10 us)
                GenerateInterference();

                if (backoff == 0)
                {
                    // backoff value is 0, thus a new backoff value is generated due to the
                    // interference
                    NS_TEST_EXPECT_MSG_EQ(m_nGenBackoff,
                                          4,
                                          "Unexpected number of generated backoff values");
                }
                else
                {
                    // interference does not cause the generation of a new backoff value because
                    // the backoff counter is non-zero.
                    // At the end of the interference:
                    Simulator::Schedule(m_interferenceDuration, [=, this]() {
                        auto voEdcaf = m_apMac->GetQosTxop(AC_VO);
                        // update backoff (backoff info is only updated when some event occurs)
                        m_apMac->GetChannelAccessManager(linkId)->NeedBackoffUponAccess(voEdcaf,
                                                                                        true,
                                                                                        true);
                        auto delay =
                            m_apMac->GetChannelAccessManager(linkId)->GetBackoffEndFor(voEdcaf) -
                            Simulator::Now() + NanoSeconds(1);

                        // right after the backoff counts down to zero:
                        Simulator::Schedule(delay, [=, this]() {
                            // check that the number of generated backoff values is still 3
                            NS_TEST_EXPECT_MSG_EQ(m_nGenBackoff,
                                                  3,
                                                  "Unexpected number of generated backoff values");
                            GenerateInterference();
                            // check that a new backoff value is generated due to the interference
                            NS_TEST_EXPECT_MSG_EQ(m_nGenBackoff,
                                                  4,
                                                  "Unexpected number of generated backoff values");
                        });
                    });
                }
            });
        }
        break;
    case nValues:
        // Unblock VO queue so that the AP can send QoS data frames
        m_apMac->GetMacQueueScheduler()->UnblockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                       AC_VO,
                                                       {WIFI_QOSDATA_QUEUE},
                                                       m_staMac->GetAddress(),
                                                       m_apMac->GetAddress(),
                                                       {m_tid},
                                                       {SINGLE_LINK_OP_ID});
        break;
    }

    if (m_generateBackoffIfTxopWithoutTx)
    {
        Time delay; // expected time until the generation of the next backoff value
        const auto offset =
            NanoSeconds(1); // offset between expected time and the time when check is made

        if (m_nGenBackoff == 1)
        {
            // we have to wait an AIFS before invoking backoff
            delay = m_apMac->GetWifiPhy(linkId)->GetSifs() +
                    m_apMac->GetQosTxop(AC_VO)->GetAifsn(linkId) *
                        m_apMac->GetWifiPhy(linkId)->GetSlot();
        }
        else if (m_nGenBackoff <= nValues)
        {
            NS_TEST_EXPECT_MSG_EQ(m_nextBackoffGen.IsPending(),
                                  true,
                                  "Expected a timer to be running");
            NS_TEST_EXPECT_MSG_EQ(Simulator::GetDelayLeft(m_nextBackoffGen),
                                  offset,
                                  "Backoff value generated too early");
            m_nextBackoffGen.Cancel();

            // we get here when the backoff expired but no transmission occurred, thus we have
            // generated a new backoff value and we will start decrementing the counter in a slot
            delay = m_apMac->GetWifiPhy(linkId)->GetSlot();
        }

        if (m_nGenBackoff < nValues)
        {
            // add the time corresponding to the generated number of slots
            delay += backoff * m_apMac->GetWifiPhy(linkId)->GetSlot();

            m_nextBackoffGen =
                Simulator::Schedule(delay + offset, &BackoffGenerationTest::MissedBackoff, this);
        }
    }

    m_nGenBackoff++;
}

void
BackoffGenerationTest::MissedBackoff()
{
    NS_TEST_EXPECT_MSG_EQ(true,
                          false,
                          "Expected a new backoff value to be generated at time "
                              << Simulator::Now().As(Time::S));
}

void
BackoffGenerationTest::GenerateInterference()
{
    NS_LOG_FUNCTION(this);
    auto phy = DynamicCast<SpectrumWifiPhy>(m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID));
    auto psd = Create<SpectrumValue>(phy->GetCurrentInterface()->GetRxSpectrumModel());
    *psd = DbmToW(dBm_u{20}) / 80e6; // PSD spread across 80 MHz to generate some noise

    auto spectrumSignalParams = Create<SpectrumSignalParameters>();
    spectrumSignalParams->duration = m_interferenceDuration;
    spectrumSignalParams->txPhy = phy->GetCurrentInterface();
    spectrumSignalParams->txAntenna = phy->GetAntenna();
    spectrumSignalParams->psd = psd;

    phy->StartRx(spectrumSignalParams, phy->GetCurrentInterface());
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Txop Test Suite
 */
class TxopTestSuite : public TestSuite
{
  public:
    TxopTestSuite();
};

TxopTestSuite::TxopTestSuite()
    : TestSuite("wifi-devices-dcf", Type::UNIT)
{
    AddTestCase(new ChannelAccessManagerTest<Txop>, TestCase::Duration::QUICK);
}

static TxopTestSuite g_dcfTestSuite;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief QosTxop Test Suite
 */
class QosTxopTestSuite : public TestSuite
{
  public:
    QosTxopTestSuite();
};

QosTxopTestSuite::QosTxopTestSuite()
    : TestSuite("wifi-devices-edca", Type::UNIT)
{
    AddTestCase(new ChannelAccessManagerTest<QosTxop>, TestCase::Duration::QUICK);
}

static QosTxopTestSuite g_edcaTestSuite;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief ChannelAccessManager Test Suite
 */
class ChannelAccessManagerTestSuite : public TestSuite
{
  public:
    ChannelAccessManagerTestSuite();
};

ChannelAccessManagerTestSuite::ChannelAccessManagerTestSuite()
    : TestSuite("wifi-channel-access-manager", Type::UNIT)
{
    AddTestCase(new LargestIdlePrimaryChannelTest, TestCase::Duration::QUICK);
    AddTestCase(new BackoffGenerationTest(BackoffGenerationTest::GEN_BACKOFF_IF_TXOP_NO_TX),
                TestCase::Duration::QUICK);
    AddTestCase(new BackoffGenerationTest(BackoffGenerationTest::PROACTIVE_BACKOFF),
                TestCase::Duration::QUICK);
}

static ChannelAccessManagerTestSuite g_camTestSuite;
