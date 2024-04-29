/*
 * Copyright (c) 2015
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/eht-configuration.h"
#include "ns3/fcfs-wifi-queue-scheduler.h"
#include "ns3/he-configuration.h"
#include "ns3/ht-configuration.h"
#include "ns3/ht-frame-exchange-manager.h"
#include "ns3/interference-helper.h"
#include "ns3/mac-tx-middle.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/mpdu-aggregator.h"
#include "ns3/msdu-aggregator.h"
#include "ns3/multi-link-element.h"
#include "ns3/node-container.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/vht-configuration.h"
#include "ns3/wifi-default-ack-manager.h"
#include "ns3/wifi-default-protection-manager.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/yans-wifi-phy.h"
#include <ns3/attribute-container.h>

#include <algorithm>
#include <iterator>
#include <vector>

using namespace ns3;

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Ampdu Aggregation Test
 */
class AmpduAggregationTest : public TestCase
{
  public:
    AmpduAggregationTest();

    /// Test parameters
    struct Params
    {
        WifiStandard standard; //!< the standard of the device
        uint8_t nLinks;        //!< number of links (>1 only for EHT)
        std::string dataMode;  //!< data mode
        uint16_t bufferSize;   //!< the size (in number of MPDUs) of the BlockAck buffer
        uint16_t maxAmsduSize; //!< maximum A-MSDU size (bytes)
        uint32_t maxAmpduSize; //!< maximum A-MPDU size (bytes)
        Time txopLimit;        //!< TXOP limit duration
    };

    /**
     * Construct object with non-default test parameters
     *
     * \param name the name of the test case
     * \param params the test parameters
     */
    AmpduAggregationTest(const std::string& name, const Params& params);

  protected:
    /**
     * Establish a BlockAck agreement.
     *
     * \param recipient the recipient MAC address
     */
    void EstablishAgreement(const Mac48Address& recipient);

    /**
     * Enqueue the given number of packets addressed to the given station and of the given size.
     *
     * \param count the number of packets
     * \param size the size (bytes) of each packet
     * \param dest the destination address
     */
    void EnqueuePkts(std::size_t count, uint32_t size, const Mac48Address& dest);

    /**
     * \return the Best Effort QosTxop
     */
    Ptr<QosTxop> GetBeQueue() const;

    /**
     * Dequeue a PSDU.
     *
     * \param mpduList the MPDUs contained in the PSDU
     */
    void DequeueMpdus(const std::vector<Ptr<WifiMpdu>>& mpduList);

    Ptr<StaWifiMac> m_mac;            ///< Mac
    std::vector<Ptr<WifiPhy>> m_phys; ///< Phys
    Params m_params;                  //!< test parameters

  private:
    /**
     * Fired when the MAC discards an MPDU.
     *
     * \param reason the reason why the MPDU was discarded
     * \param mpdu the discarded MPDU
     */
    void MpduDiscarded(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu);

    void DoSetup() override;
    void DoRun() override;
    void DoTeardown() override;

    Ptr<WifiNetDevice> m_device;                           ///< WifiNetDevice
    std::vector<Ptr<WifiRemoteStationManager>> m_managers; ///< remote station managers
    ObjectFactory m_factory;                               ///< factory
    bool m_discarded; ///< whether the packet should be discarded
};

AmpduAggregationTest::AmpduAggregationTest()
    : AmpduAggregationTest("Check the correctness of MPDU aggregation operations",
                           Params{.standard = WIFI_STANDARD_80211n,
                                  .nLinks = 1,
                                  .dataMode = "HtMcs7",
                                  .bufferSize = 64,
                                  .maxAmsduSize = 0,
                                  .maxAmpduSize = 65535,
                                  .txopLimit = Seconds(0)})
{
}

AmpduAggregationTest::AmpduAggregationTest(const std::string& name, const Params& params)
    : TestCase(name),
      m_params(params),
      m_discarded(false)
{
}

void
AmpduAggregationTest::MpduDiscarded(WifiMacDropReason, Ptr<const WifiMpdu>)
{
    m_discarded = true;
}

void
AmpduAggregationTest::DoSetup()
{
    /*
     * Create device and attach HT configuration.
     */
    m_device = CreateObject<WifiNetDevice>();
    m_device->SetStandard(m_params.standard);
    auto htConfiguration = CreateObject<HtConfiguration>();
    m_device->SetHtConfiguration(htConfiguration);
    if (m_params.standard >= WIFI_STANDARD_80211ax)
    {
        auto vhtConfiguration = CreateObject<VhtConfiguration>();
        m_device->SetVhtConfiguration(vhtConfiguration);
        auto heConfiguration = CreateObject<HeConfiguration>();
        m_device->SetHeConfiguration(heConfiguration);
    }
    if (m_params.standard >= WIFI_STANDARD_80211be)
    {
        auto ehtConfiguration = CreateObject<EhtConfiguration>();
        m_device->SetEhtConfiguration(ehtConfiguration);
    }

    /*
     * Create and configure phy layer.
     */
    for (uint8_t i = 0; i < m_params.nLinks; i++)
    {
        m_phys.emplace_back(CreateObject<YansWifiPhy>());
        auto interferenceHelper = CreateObject<InterferenceHelper>();
        m_phys.back()->SetInterferenceHelper(interferenceHelper);
        m_phys.back()->SetDevice(m_device);
        m_phys.back()->ConfigureStandard(m_params.standard);
    }
    m_device->SetPhys(m_phys);

    /*
     * Create and configure manager.
     */
    m_factory = ObjectFactory();
    m_factory.SetTypeId("ns3::ConstantRateWifiManager");
    m_factory.Set("DataMode", StringValue(m_params.dataMode));
    for (uint8_t i = 0; i < m_params.nLinks; i++)
    {
        m_managers.emplace_back(m_factory.Create<WifiRemoteStationManager>());
        m_managers.back()->SetupPhy(m_phys.at(i));
    }
    m_device->SetRemoteStationManagers(m_managers);

    /*
     * Create and configure mac layer.
     */
    m_mac = CreateObjectWithAttributes<StaWifiMac>(
        "QosSupported",
        BooleanValue(true),
        "BE_Txop",
        PointerValue(CreateObjectWithAttributes<QosTxop>("AcIndex", StringValue("AC_BE"))),
        "BK_Txop",
        PointerValue(CreateObjectWithAttributes<QosTxop>("AcIndex", StringValue("AC_BK"))),
        "VI_Txop",
        PointerValue(CreateObjectWithAttributes<QosTxop>("AcIndex", StringValue("AC_VI"))),
        "VO_Txop",
        PointerValue(CreateObjectWithAttributes<QosTxop>("AcIndex", StringValue("AC_VO"))));
    m_mac->SetDevice(m_device);
    m_mac->SetWifiRemoteStationManagers(m_managers);
    for (uint8_t i = 0; i < m_params.nLinks; i++)
    {
        m_managers.at(i)->SetupMac(m_mac);
    }
    m_mac->SetAddress(Mac48Address("00:00:00:00:00:01"));
    m_device->SetMac(m_mac);
    m_mac->SetWifiPhys(m_phys);
    std::vector<Ptr<ChannelAccessManager>> caManagers;
    for (uint8_t i = 0; i < m_params.nLinks; i++)
    {
        caManagers.emplace_back(CreateObject<ChannelAccessManager>());
    }
    m_mac->SetChannelAccessManagers(caManagers);
    ObjectFactory femFactory;
    femFactory.SetTypeId(GetFrameExchangeManagerTypeIdName(m_params.standard, true));
    std::vector<Ptr<FrameExchangeManager>> feManagers;
    for (uint8_t i = 0; i < m_params.nLinks; i++)
    {
        auto fem = femFactory.Create<FrameExchangeManager>();
        feManagers.emplace_back(fem);
        auto protectionManager = CreateObject<WifiDefaultProtectionManager>();
        protectionManager->SetWifiMac(m_mac);
        fem->SetProtectionManager(protectionManager);
        auto ackManager = CreateObject<WifiDefaultAckManager>();
        ackManager->SetWifiMac(m_mac);
        fem->SetAckManager(ackManager);
        // here we should assign distinct link addresses in case of MLDs, but we don't actually use
        // link addresses in this test
        fem->SetAddress(m_mac->GetAddress());
    }
    m_mac->SetFrameExchangeManagers(feManagers);
    m_mac->SetState(StaWifiMac::ASSOCIATED);
    if (m_params.nLinks > 1)
    {
        // the bssid field of StaLinkEntity must hold a value
        for (const auto& [id, link] : m_mac->GetLinks())
        {
            static_cast<StaWifiMac::StaLinkEntity&>(*link).bssid = Mac48Address::GetBroadcast();
        }
    }
    m_mac->SetMacQueueScheduler(CreateObject<FcfsWifiQueueScheduler>());

    /*
     * Configure A-MSDU and A-MPDU aggregation.
     */
    // Make sure that at least 1024 MPDUs are buffered (to test aggregation on EHT devices)
    m_mac->GetTxopQueue(AC_BE)->SetAttribute("MaxSize", StringValue("2000p"));
    m_mac->SetAttribute("BE_MaxAmsduSize", UintegerValue(m_params.maxAmsduSize));
    m_mac->SetAttribute("BE_MaxAmpduSize", UintegerValue(m_params.maxAmpduSize));
    GetBeQueue()->SetAttribute(
        "TxopLimits",
        AttributeContainerValue<TimeValue>(std::vector<Time>(m_params.nLinks, m_params.txopLimit)));

    if (m_params.nLinks > 1)
    {
        auto mleCommonInfo2 = std::make_shared<CommonInfoBasicMle>();
        mleCommonInfo2->m_mldMacAddress = Mac48Address("00:00:00:00:00:02");
        for (uint8_t i = 0; i < m_params.nLinks; i++)
        {
            // we don't actually use the link addresses of the receiver, so we just use one address
            // as both the MLD address and the link address of the receiver (the first argument in
            // the call below should be the link address)
            m_managers.at(i)->AddStationMleCommonInfo(mleCommonInfo2->m_mldMacAddress,
                                                      mleCommonInfo2);
        }

        auto mleCommonInfo3 = std::make_shared<CommonInfoBasicMle>();
        mleCommonInfo3->m_mldMacAddress = Mac48Address("00:00:00:00:00:03");
        for (uint8_t i = 0; i < m_params.nLinks; i++)
        {
            m_managers.at(i)->AddStationMleCommonInfo(mleCommonInfo3->m_mldMacAddress,
                                                      mleCommonInfo3);
        }
    }

    for (uint8_t i = 0; i < m_params.nLinks; i++)
    {
        HtCapabilities htCapabilities;
        htCapabilities.SetMaxAmsduLength(7935);
        htCapabilities.SetMaxAmpduLength(65535);
        m_managers.at(i)->AddStationHtCapabilities(Mac48Address("00:00:00:00:00:02"),
                                                   htCapabilities);
        m_managers.at(i)->AddStationHtCapabilities(Mac48Address("00:00:00:00:00:03"),
                                                   htCapabilities);

        if (m_params.standard >= WIFI_STANDARD_80211ac)
        {
            VhtCapabilities vhtCapabilities;
            vhtCapabilities.SetMaxMpduLength(11454);
            m_managers.at(i)->AddStationVhtCapabilities(Mac48Address("00:00:00:00:00:02"),
                                                        vhtCapabilities);
        }
        if (m_params.standard >= WIFI_STANDARD_80211ax)
        {
            HeCapabilities heCapabilities;
            heCapabilities.SetMaxAmpduLength((1 << 23) - 1);
            m_managers.at(i)->AddStationHeCapabilities(Mac48Address("00:00:00:00:00:02"),
                                                       heCapabilities);
        }
        if (m_params.standard >= WIFI_STANDARD_80211be)
        {
            EhtCapabilities ehtCapabilities;
            ehtCapabilities.SetMaxMpduLength(11454);
            ehtCapabilities.SetMaxAmpduLength((1 << 24) - 1);
            m_managers.at(i)->AddStationEhtCapabilities(Mac48Address("00:00:00:00:00:02"),
                                                        ehtCapabilities);
        }
    }

    /*
     * Establish agreement.
     */
    EstablishAgreement(Mac48Address("00:00:00:00:00:02"));
}

Ptr<QosTxop>
AmpduAggregationTest::GetBeQueue() const
{
    return m_mac->GetBEQueue();
}

void
AmpduAggregationTest::DequeueMpdus(const std::vector<Ptr<WifiMpdu>>& mpduList)
{
    std::list<Ptr<const WifiMpdu>> mpdus(mpduList.cbegin(), mpduList.cend());
    m_mac->GetTxopQueue(AC_BE)->DequeueIfQueued(mpdus);
}

void
AmpduAggregationTest::EstablishAgreement(const Mac48Address& recipient)
{
    MgtAddBaRequestHeader reqHdr;
    reqHdr.SetImmediateBlockAck();
    reqHdr.SetTid(0);
    reqHdr.SetBufferSize(m_params.bufferSize);
    reqHdr.SetTimeout(0);
    reqHdr.SetStartingSequence(0);
    GetBeQueue()->GetBaManager()->CreateOriginatorAgreement(reqHdr, recipient);

    MgtAddBaResponseHeader respHdr;
    StatusCode code;
    code.SetSuccess();
    respHdr.SetStatusCode(code);
    respHdr.SetAmsduSupport(reqHdr.IsAmsduSupported());
    respHdr.SetImmediateBlockAck();
    respHdr.SetTid(reqHdr.GetTid());
    respHdr.SetBufferSize(m_params.bufferSize);
    respHdr.SetTimeout(reqHdr.GetTimeout());
    GetBeQueue()->GetBaManager()->UpdateOriginatorAgreement(respHdr, recipient, 0);
}

void
AmpduAggregationTest::EnqueuePkts(std::size_t count, uint32_t size, const Mac48Address& dest)
{
    for (std::size_t i = 0; i < count; i++)
    {
        auto pkt = Create<Packet>(size);
        WifiMacHeader hdr;

        hdr.SetAddr1(dest);
        hdr.SetAddr2(Mac48Address("00:00:00:00:00:01"));
        hdr.SetType(WIFI_MAC_QOSDATA);
        hdr.SetQosTid(0);

        GetBeQueue()->GetWifiMacQueue()->Enqueue(Create<WifiMpdu>(pkt, hdr));
    }
}

void
AmpduAggregationTest::DoRun()
{
    /*
     * Test behavior when no other packets are in the queue
     */
    auto fem = m_mac->GetFrameExchangeManager(SINGLE_LINK_OP_ID);
    auto htFem = DynamicCast<HtFrameExchangeManager>(fem);
    auto mpduAggregator = htFem->GetMpduAggregator();

    /*
     * Create a dummy packet of 1500 bytes and fill mac header fields.
     */
    EnqueuePkts(1, 1500, Mac48Address("00:00:00:00:00:02"));

    auto peeked = GetBeQueue()->PeekNextMpdu(SINGLE_LINK_OP_ID);
    WifiTxParameters txParams;
    txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
        peeked->GetHeader(),
        m_phys.at(SINGLE_LINK_OP_ID)->GetChannelWidth());
    auto item = GetBeQueue()->GetNextMpdu(SINGLE_LINK_OP_ID, peeked, txParams, Time::Min(), true);

    auto mpduList = mpduAggregator->GetNextAmpdu(item, txParams, Time::Min());

    NS_TEST_EXPECT_MSG_EQ(mpduList.empty(), true, "a single packet should not result in an A-MPDU");

    // the packet has not been "transmitted", release its sequence number
    m_mac->m_txMiddle->SetSequenceNumberFor(&item->GetHeader());
    item->UnassignSeqNo();

    //---------------------------------------------------------------------------------------------

    /*
     * Test behavior when 2 more packets are in the queue
     */
    EnqueuePkts(2, 1500, Mac48Address("00:00:00:00:00:02"));

    item = GetBeQueue()->GetNextMpdu(SINGLE_LINK_OP_ID, peeked, txParams, Time::Min(), true);
    mpduList = mpduAggregator->GetNextAmpdu(item, txParams, Time::Min());

    NS_TEST_EXPECT_MSG_EQ(mpduList.empty(), false, "MPDU aggregation failed");

    auto psdu = Create<WifiPsdu>(mpduList);
    DequeueMpdus(mpduList);

    NS_TEST_EXPECT_MSG_EQ(psdu->GetSize(), 4606, "A-MPDU size is not correct");
    NS_TEST_EXPECT_MSG_EQ(mpduList.size(), 3, "A-MPDU should contain 3 MPDUs");
    NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                          0,
                          "queue should be empty");

    for (uint32_t i = 0; i < psdu->GetNMpdus(); i++)
    {
        NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(i).GetSequenceNumber(), i, "wrong sequence number");
    }

    //---------------------------------------------------------------------------------------------

    /*
     * Test behavior when the 802.11n station and another non-QoS station are associated to the AP.
     * The AP sends an A-MPDU to the 802.11n station followed by the last retransmission of a
     * non-QoS data frame to the non-QoS station. This is used to reproduce bug 2224.
     */
    EnqueuePkts(1, 1500, Mac48Address("00:00:00:00:00:02"));
    EnqueuePkts(2, 1500, Mac48Address("00:00:00:00:00:03"));

    peeked = GetBeQueue()->PeekNextMpdu(SINGLE_LINK_OP_ID);
    txParams.Clear();
    txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
        peeked->GetHeader(),
        m_phys.at(SINGLE_LINK_OP_ID)->GetChannelWidth());
    item = GetBeQueue()->GetNextMpdu(SINGLE_LINK_OP_ID, peeked, txParams, Time::Min(), true);

    mpduList = mpduAggregator->GetNextAmpdu(item, txParams, Time::Min());

    NS_TEST_EXPECT_MSG_EQ(mpduList.empty(),
                          true,
                          "a single packet for this destination should not result in an A-MPDU");
    // dequeue the MPDU
    DequeueMpdus({item});

    peeked = GetBeQueue()->PeekNextMpdu(SINGLE_LINK_OP_ID);
    txParams.Clear();
    txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
        peeked->GetHeader(),
        m_phys.at(SINGLE_LINK_OP_ID)->GetChannelWidth());
    item = GetBeQueue()->GetNextMpdu(SINGLE_LINK_OP_ID, peeked, txParams, Time::Min(), true);

    mpduList = mpduAggregator->GetNextAmpdu(item, txParams, Time::Min());

    NS_TEST_EXPECT_MSG_EQ(mpduList.empty(),
                          true,
                          "no MPDU aggregation should be performed if there is no agreement");

    m_managers.at(SINGLE_LINK_OP_ID)
        ->SetMaxSsrc(
            0); // set to 0 in order to fake that the maximum number of retries has been reached
    m_mac->TraceConnectWithoutContext("DroppedMpdu",
                                      MakeCallback(&AmpduAggregationTest::MpduDiscarded, this));
    htFem->m_dcf = GetBeQueue();
    htFem->NormalAckTimeout(item, txParams.m_txVector);

    NS_TEST_EXPECT_MSG_EQ(m_discarded, true, "packet should be discarded");
    GetBeQueue()->GetWifiMacQueue()->Flush();
}

void
AmpduAggregationTest::DoTeardown()
{
    Simulator::Destroy();

    for (auto manager : m_managers)
    {
        manager->Dispose();
    }
    m_managers.clear();

    m_device->Dispose();
    m_device = nullptr;
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Two Level Aggregation Test
 */
class TwoLevelAggregationTest : public AmpduAggregationTest
{
  public:
    TwoLevelAggregationTest();

  private:
    void DoRun() override;
};

TwoLevelAggregationTest::TwoLevelAggregationTest()
    : AmpduAggregationTest("Check the correctness of two-level aggregation operations",
                           Params{.standard = WIFI_STANDARD_80211n,
                                  .nLinks = 1,
                                  .dataMode = "HtMcs2", // 19.5Mbps
                                  .bufferSize = 64,
                                  .maxAmsduSize = 3050,
                                  .maxAmpduSize = 65535,
                                  .txopLimit = MicroSeconds(3008)})
{
}

void
TwoLevelAggregationTest::DoRun()
{
    /*
     * Create dummy packets of 1500 bytes and fill mac header fields that will be used for the
     * tests.
     */
    EnqueuePkts(3, 1500, Mac48Address("00:00:00:00:00:02"));

    //---------------------------------------------------------------------------------------------

    /*
     * Test MSDU and MPDU aggregation. Three MSDUs are in the queue and the maximum A-MSDU size
     * is such that only two MSDUs can be aggregated. Therefore, the first MPDU we get contains
     * an A-MSDU of 2 MSDUs.
     */
    auto fem = m_mac->GetFrameExchangeManager(SINGLE_LINK_OP_ID);
    auto htFem = DynamicCast<HtFrameExchangeManager>(fem);
    auto msduAggregator = htFem->GetMsduAggregator();
    auto mpduAggregator = htFem->GetMpduAggregator();

    auto peeked = GetBeQueue()->PeekNextMpdu(SINGLE_LINK_OP_ID);
    WifiTxParameters txParams;
    txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
        peeked->GetHeader(),
        m_phys.at(SINGLE_LINK_OP_ID)->GetChannelWidth());
    htFem->TryAddMpdu(peeked, txParams, Time::Min());
    auto item = msduAggregator->GetNextAmsdu(peeked, txParams, Time::Min());

    bool result{item};
    NS_TEST_EXPECT_MSG_EQ(result, true, "aggregation failed");
    NS_TEST_EXPECT_MSG_EQ(item->GetPacketSize(), 3030, "wrong packet size");

    // dequeue the MSDUs
    DequeueMpdus({item});

    NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                          1,
                          "Unexpected number of MSDUs left in the EDCA queue");

    //---------------------------------------------------------------------------------------------

    /*
     * A-MSDU aggregation fails when there is just one MSDU in the queue.
     */

    peeked = GetBeQueue()->PeekNextMpdu(SINGLE_LINK_OP_ID);
    txParams.Clear();
    txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
        peeked->GetHeader(),
        m_phys.at(SINGLE_LINK_OP_ID)->GetChannelWidth());
    htFem->TryAddMpdu(peeked, txParams, Time::Min());
    item = msduAggregator->GetNextAmsdu(peeked, txParams, Time::Min());

    NS_TEST_EXPECT_MSG_EQ(item, nullptr, "A-MSDU aggregation did not fail");

    DequeueMpdus({peeked});

    NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                          0,
                          "queue should be empty");

    //---------------------------------------------------------------------------------------------

    /*
     * Aggregation of MPDUs is stopped to prevent that the PPDU duration exceeds the TXOP limit.
     * In this test, a TXOP limit of 3008 microseconds is used.
     */

    // Add 10 MSDUs to the EDCA queue
    EnqueuePkts(10, 1300, Mac48Address("00:00:00:00:00:02"));

    peeked = GetBeQueue()->PeekNextMpdu(SINGLE_LINK_OP_ID);
    txParams.Clear();
    txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
        peeked->GetHeader(),
        m_phys.at(SINGLE_LINK_OP_ID)->GetChannelWidth());

    // Compute the first MPDU to be aggregated in an A-MPDU. It must contain an A-MSDU
    // aggregating two MSDUs
    item = GetBeQueue()->GetNextMpdu(SINGLE_LINK_OP_ID, peeked, txParams, m_params.txopLimit, true);

    NS_TEST_EXPECT_MSG_EQ(std::distance(item->begin(), item->end()),
                          2,
                          "There must be 2 MSDUs in the A-MSDU");

    auto mpduList = mpduAggregator->GetNextAmpdu(item, txParams, m_params.txopLimit);

    // The maximum number of bytes that can be transmitted in a TXOP is (approximately, as we
    // do not consider that the preamble is transmitted at a different rate):
    // 19.5 Mbps * 3.008 ms = 7332 bytes
    // Given that the max A-MSDU size is set to 3050, an A-MSDU will contain two MSDUs and have
    // a size of 2 * 1300 (MSDU size) + 2 * 14 (A-MSDU subframe header size) + 2 (one padding field)
    // = 2630 bytes Hence, we expect that the A-MPDU will consist of:
    // - 2 MPDUs containing each an A-MSDU. The size of each MPDU is 2630 (A-MSDU) + 30
    // (header+trailer) = 2660
    // - 1 MPDU containing a single MSDU. The size of such MPDU is 1300 (MSDU) + 30 (header+trailer)
    // = 1330 The size of the A-MPDU is 4 + 2660 + 4 + 2660 + 4 + 1330 = 6662
    NS_TEST_EXPECT_MSG_EQ(mpduList.empty(), false, "aggregation failed");
    NS_TEST_EXPECT_MSG_EQ(mpduList.size(), 3, "Unexpected number of MPDUs in the A-MPDU");
    NS_TEST_EXPECT_MSG_EQ(mpduList.at(0)->GetSize(), 2660, "Unexpected size of the first MPDU");
    NS_TEST_EXPECT_MSG_EQ(mpduList.at(0)->GetHeader().IsQosAmsdu(),
                          true,
                          "Expecting the first MPDU to contain an A-MSDU");
    NS_TEST_EXPECT_MSG_EQ(mpduList.at(1)->GetSize(), 2660, "Unexpected size of the second MPDU");
    NS_TEST_EXPECT_MSG_EQ(mpduList.at(1)->GetHeader().IsQosAmsdu(),
                          true,
                          "Expecting the second MPDU to contain an A-MSDU");
    NS_TEST_EXPECT_MSG_EQ(mpduList.at(2)->GetSize(), 1330, "Unexpected size of the third MPDU");
    NS_TEST_EXPECT_MSG_EQ(mpduList.at(2)->GetHeader().IsQosAmsdu(),
                          false,
                          "Expecting the third MPDU not to contain an A-MSDU");

    auto psdu = Create<WifiPsdu>(mpduList);
    NS_TEST_EXPECT_MSG_EQ(psdu->GetSize(), 6662, "Unexpected size of the A-MPDU");

    // we now have two A-MSDUs and 6 MSDUs in the queue (5 MSDUs with no assigned sequence number)
    NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                          8,
                          "Unexpected number of items left in the EDCA queue");

    // prepare another A-MPDU (e.g., for transmission on another link)
    peeked = GetBeQueue()->PeekNextMpdu(SINGLE_LINK_OP_ID, 0, psdu->GetAddr1(), mpduList.at(2));
    txParams.Clear();
    txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
        peeked->GetHeader(),
        m_phys.at(SINGLE_LINK_OP_ID)->GetChannelWidth());

    // Compute the first MPDU to be aggregated in an A-MPDU. It must contain an A-MSDU
    // aggregating two MSDUs
    item = GetBeQueue()->GetNextMpdu(SINGLE_LINK_OP_ID, peeked, txParams, m_params.txopLimit, true);

    NS_TEST_EXPECT_MSG_EQ(std::distance(item->begin(), item->end()),
                          2,
                          "There must be 2 MSDUs in the A-MSDU");

    auto mpduList2 = mpduAggregator->GetNextAmpdu(item, txParams, m_params.txopLimit);

    // we now have two A-MSDUs, one MSDU, two A-MSDUs and one MSDU in the queue (all with assigned
    // sequence number)
    NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                          6,
                          "Unexpected number of items left in the EDCA queue");

    // unassign sequence numbers for all MPDUs (emulates an RTS/CTS failure on both links)
    mpduList.at(0)->UnassignSeqNo();
    mpduList.at(1)->UnassignSeqNo();
    mpduList.at(2)->UnassignSeqNo();
    mpduList2.at(0)->UnassignSeqNo();
    mpduList2.at(1)->UnassignSeqNo();
    mpduList2.at(2)->UnassignSeqNo();

    // set A-MSDU max size to a large value
    m_mac->SetAttribute("BE_MaxAmsduSize", UintegerValue(7000));

    // A-MSDU aggregation now fails because the first item in the queue contain A-MSDUs
    peeked = GetBeQueue()->PeekNextMpdu(SINGLE_LINK_OP_ID);
    txParams.Clear();
    txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
        peeked->GetHeader(),
        m_phys.at(SINGLE_LINK_OP_ID)->GetChannelWidth());

    htFem->TryAddMpdu(peeked, txParams, Time::Min());
    item = msduAggregator->GetNextAmsdu(peeked, txParams, Time::Min());

    NS_TEST_EXPECT_MSG_EQ(item, nullptr, "Expecting not to be able to aggregate A-MSDUs");

    // remove the first two items in the queue (containing A-MSDUs)
    DequeueMpdus({mpduList.at(0), mpduList.at(1)});

    // we now have one MSDU, two A-MSDUs and one MSDU in the queue
    NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                          4,
                          "Unexpected number of items left in the EDCA queue");

    peeked = GetBeQueue()->PeekNextMpdu(SINGLE_LINK_OP_ID);
    txParams.Clear();
    txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
        peeked->GetHeader(),
        m_phys.at(SINGLE_LINK_OP_ID)->GetChannelWidth());

    NS_TEST_EXPECT_MSG_EQ(peeked->GetHeader().IsQosAmsdu(),
                          false,
                          "Expecting the peeked MPDU not to contain an A-MSDU");

    item = GetBeQueue()->GetNextMpdu(SINGLE_LINK_OP_ID, peeked, txParams, Time::Min(), true);

    // A-MSDU aggregation is not attempted because the next item contains an A-MSDU
    NS_TEST_EXPECT_MSG_EQ(item->GetHeader().IsQosAmsdu(),
                          false,
                          "Expecting the returned MPDU not to contain an A-MSDU");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief 802.11ax aggregation test which permits 64 or 256 MPDUs in A-MPDU according to the
 * negotiated buffer size.
 */
class HeAggregationTest : public AmpduAggregationTest
{
  public:
    /**
     * Constructor.
     *
     * \param bufferSize the size (in number of MPDUs) of the BlockAck buffer
     */
    HeAggregationTest(uint16_t bufferSize);

  private:
    void DoRun() override;
};

HeAggregationTest::HeAggregationTest(uint16_t bufferSize)
    : AmpduAggregationTest("Check the correctness of 802.11ax aggregation operations, size=" +
                               std::to_string(bufferSize),
                           Params{.standard = WIFI_STANDARD_80211ax,
                                  .nLinks = 1,
                                  .dataMode = "HeMcs11",
                                  .bufferSize = bufferSize,
                                  .maxAmsduSize = 0,
                                  .maxAmpduSize = 65535,
                                  .txopLimit = Seconds(0)})
{
}

void
HeAggregationTest::DoRun()
{
    /*
     * Test behavior when 300 packets are ready for transmission
     */
    EnqueuePkts(300, 100, Mac48Address("00:00:00:00:00:02"));

    auto fem = m_mac->GetFrameExchangeManager(SINGLE_LINK_OP_ID);
    auto htFem = DynamicCast<HtFrameExchangeManager>(fem);
    auto mpduAggregator = htFem->GetMpduAggregator();

    auto peeked = GetBeQueue()->PeekNextMpdu(SINGLE_LINK_OP_ID);
    WifiTxParameters txParams;
    txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
        peeked->GetHeader(),
        m_phys.at(SINGLE_LINK_OP_ID)->GetChannelWidth());
    auto item = GetBeQueue()->GetNextMpdu(SINGLE_LINK_OP_ID, peeked, txParams, Time::Min(), true);

    auto mpduList = mpduAggregator->GetNextAmpdu(item, txParams, Time::Min());
    DequeueMpdus(mpduList);

    NS_TEST_EXPECT_MSG_EQ(mpduList.empty(), false, "MPDU aggregation failed");
    NS_TEST_EXPECT_MSG_EQ(mpduList.size(),
                          m_params.bufferSize,
                          "A-MPDU contains an unexpected number of MPDUs");
    uint16_t expectedRemainingPacketsInQueue = 300 - m_params.bufferSize;
    NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                          expectedRemainingPacketsInQueue,
                          "Queue contains an unexpected number of MPDUs");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief 802.11be aggregation test which permits up to 1024 MPDUs in A-MPDU according to the
 * negotiated buffer size.
 */
class EhtAggregationTest : public AmpduAggregationTest
{
  public:
    /**
     * Constructor.
     *
     * \param bufferSize the size (in number of MPDUs) of the BlockAck buffer
     */
    EhtAggregationTest(uint16_t bufferSize);

  private:
    void DoRun() override;
};

EhtAggregationTest::EhtAggregationTest(uint16_t bufferSize)
    : AmpduAggregationTest("Check the correctness of 802.11be aggregation operations, size=" +
                               std::to_string(bufferSize),
                           Params{.standard = WIFI_STANDARD_80211be,
                                  .nLinks = 2,
                                  .dataMode = "EhtMcs13",
                                  .bufferSize = bufferSize,
                                  .maxAmsduSize = 0,
                                  .maxAmpduSize = 102000,
                                  .txopLimit = Seconds(0)})
{
}

void
EhtAggregationTest::DoRun()
{
    /*
     * Test behavior when 1200 packets of 100 bytes each are ready for transmission. The max
     * A-MPDU size limit (102000 B) is computed to have at most 750 MPDUs aggregated in a single
     * A-MPDU (each MPDU is 130 B, plus 4 B of A-MPDU subframe header, plus 2 B of padding).
     */
    EnqueuePkts(1200, 100, Mac48Address("00:00:00:00:00:02"));
    const std::size_t maxNMpdus = 750;

    for (uint8_t linkId = 0; linkId < m_params.nLinks; linkId++)
    {
        auto fem = m_mac->GetFrameExchangeManager(linkId);
        auto htFem = DynamicCast<HtFrameExchangeManager>(fem);
        auto mpduAggregator = htFem->GetMpduAggregator();
        std::vector<Ptr<WifiMpdu>> mpduList;

        auto peeked = GetBeQueue()->PeekNextMpdu(linkId);
        if (peeked)
        {
            WifiTxParameters txParams;
            txParams.m_txVector = m_mac->GetWifiRemoteStationManager()->GetDataTxVector(
                peeked->GetHeader(),
                m_phys.at(linkId)->GetChannelWidth());
            auto item = GetBeQueue()->GetNextMpdu(linkId, peeked, txParams, Time::Min(), true);

            mpduList = mpduAggregator->GetNextAmpdu(item, txParams, Time::Min());
            DequeueMpdus(mpduList);
        }

        uint16_t expectedRemainingPacketsInQueue;

        if (m_params.bufferSize >= maxNMpdus)
        {
            // two A-MPDUs are transmitted concurrently on the two links and together saturate
            // the transmit window
            switch (linkId)
            {
            case 0:
                // the first A-MPDU includes maxNMpdus MPDUs
                NS_TEST_EXPECT_MSG_EQ(mpduList.empty(), false, "MPDU aggregation failed");
                NS_TEST_EXPECT_MSG_EQ(mpduList.size(),
                                      maxNMpdus,
                                      "A-MPDU contains an unexpected number of MPDUs");
                expectedRemainingPacketsInQueue = 1200 - maxNMpdus;
                NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                                      expectedRemainingPacketsInQueue,
                                      "Queue contains an unexpected number of MPDUs");
                break;
            case 1:
                // the second A-MPDU includes bufferSize - maxNMpdus MPDUs
                NS_TEST_EXPECT_MSG_EQ(mpduList.empty(), false, "MPDU aggregation failed");
                NS_TEST_EXPECT_MSG_EQ(mpduList.size(),
                                      m_params.bufferSize - maxNMpdus,
                                      "A-MPDU contains an unexpected number of MPDUs");
                expectedRemainingPacketsInQueue = 1200 - m_params.bufferSize;
                NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                                      expectedRemainingPacketsInQueue,
                                      "Queue contains an unexpected number of MPDUs");
                break;
            default:
                NS_TEST_ASSERT_MSG_EQ(true, false, "Unexpected link ID " << +linkId);
            }
        }
        else
        {
            // one A-MPDU is transmitted that saturates the transmit window
            switch (linkId)
            {
            case 0:
                // the first A-MPDU includes bufferSize MPDUs
                NS_TEST_EXPECT_MSG_EQ(mpduList.empty(), false, "MPDU aggregation failed");
                NS_TEST_EXPECT_MSG_EQ(mpduList.size(),
                                      m_params.bufferSize,
                                      "A-MPDU contains an unexpected number of MPDUs");
                expectedRemainingPacketsInQueue = 1200 - m_params.bufferSize;
                NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                                      expectedRemainingPacketsInQueue,
                                      "Queue contains an unexpected number of MPDUs");
                break;
            case 1:
                // no more MPDUs can be sent, aggregation fails
                NS_TEST_EXPECT_MSG_EQ(mpduList.empty(), true, "MPDU aggregation did not fail");
                expectedRemainingPacketsInQueue = 1200 - m_params.bufferSize;
                NS_TEST_EXPECT_MSG_EQ(GetBeQueue()->GetWifiMacQueue()->GetNPackets(),
                                      expectedRemainingPacketsInQueue,
                                      "Queue contains an unexpected number of MPDUs");
                break;
            default:
                NS_TEST_ASSERT_MSG_EQ(true, false, "Unexpected link ID " << +linkId);
            }
        }
    }
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test for A-MSDU and A-MPDU aggregation
 *
 * This test aims to check that the packets passed to the MAC layer (on the sender
 * side) are forwarded up to the upper layer (on the receiver side) when A-MSDU and
 * A-MPDU aggregation are used. This test checks that no packet copies are performed,
 * hence packets can be tracked by means of a pointer.
 *
 * In this test, an HT STA sends 8 packets (each of 1000 bytes) to an HT AP.
 * The block ack threshold is set to 2, hence the first packet is sent as an MPDU
 * containing a single MSDU because the establishment of a Block Ack agreement is
 * not triggered yet. The maximum A-MSDU size is set to 4500 bytes and the
 * maximum A-MPDU size is set to 7500 bytes, hence the remaining packets are sent
 * in an A-MPDU containing two MPDUs, the first one including 4 MSDUs and the second
 * one including 3 MPDUs.
 */
class PreservePacketsInAmpdus : public TestCase
{
  public:
    PreservePacketsInAmpdus();
    ~PreservePacketsInAmpdus() override;

    void DoRun() override;

  private:
    std::list<Ptr<const Packet>> m_packetList; ///< List of packets passed to the MAC
    std::vector<std::size_t> m_nMpdus;         ///< Number of MPDUs in PSDUs passed to the PHY
    std::vector<std::size_t> m_nMsdus;         ///< Number of MSDUs in MPDUs passed to the PHY

    /**
     * Callback invoked when an MSDU is passed to the MAC
     * \param packet the MSDU to transmit
     */
    void NotifyMacTransmit(Ptr<const Packet> packet);
    /**
     * Callback invoked when the sender MAC passes a PSDU(s) to the PHY
     * \param psduMap the PSDU map
     * \param txVector the TX vector
     * \param txPowerW the transmit power in Watts
     */
    void NotifyPsduForwardedDown(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);
    /**
     * Callback invoked when the receiver MAC forwards a packet up to the upper layer
     * \param p the packet
     */
    void NotifyMacForwardUp(Ptr<const Packet> p);
};

PreservePacketsInAmpdus::PreservePacketsInAmpdus()
    : TestCase("Test case to check that the Wifi Mac forwards up the same packets received at "
               "sender side.")
{
}

PreservePacketsInAmpdus::~PreservePacketsInAmpdus()
{
}

void
PreservePacketsInAmpdus::NotifyMacTransmit(Ptr<const Packet> packet)
{
    m_packetList.push_back(packet);
}

void
PreservePacketsInAmpdus::NotifyPsduForwardedDown(WifiConstPsduMap psduMap,
                                                 WifiTxVector txVector,
                                                 double txPowerW)
{
    NS_TEST_EXPECT_MSG_EQ((psduMap.size() == 1 && psduMap.begin()->first == SU_STA_ID),
                          true,
                          "No DL MU PPDU expected");

    if (!psduMap[SU_STA_ID]->GetHeader(0).IsQosData())
    {
        return;
    }

    m_nMpdus.push_back(psduMap[SU_STA_ID]->GetNMpdus());

    for (auto& mpdu : *PeekPointer(psduMap[SU_STA_ID]))
    {
        std::size_t dist = std::distance(mpdu->begin(), mpdu->end());
        // the list of aggregated MSDUs is empty if the MPDU includes a non-aggregated MSDU
        m_nMsdus.push_back(dist > 0 ? dist : 1);
    }
}

void
PreservePacketsInAmpdus::NotifyMacForwardUp(Ptr<const Packet> p)
{
    auto it = std::find(m_packetList.begin(), m_packetList.end(), p);
    NS_TEST_EXPECT_MSG_EQ((it != m_packetList.end()), true, "Packet being forwarded up not found");
    m_packetList.erase(it);
}

void
PreservePacketsInAmpdus::DoRun()
{
    NodeContainer wifiStaNode;
    wifiStaNode.Create(1);

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    mac.SetType("ns3::StaWifiMac",
                "BE_MaxAmsduSize",
                UintegerValue(4500),
                "BE_MaxAmpduSize",
                UintegerValue(7500),
                "Ssid",
                SsidValue(ssid),
                /* setting blockack threshold for sta's BE queue */
                "BE_BlockAckThreshold",
                UintegerValue(2),
                "ActiveProbing",
                BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNode);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid), "BeaconGeneration", BooleanValue(true));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, wifiApNode);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNode);

    Ptr<WifiNetDevice> ap_device = DynamicCast<WifiNetDevice>(apDevices.Get(0));
    Ptr<WifiNetDevice> sta_device = DynamicCast<WifiNetDevice>(staDevices.Get(0));

    PacketSocketAddress socket;
    socket.SetSingleDevice(sta_device->GetIfIndex());
    socket.SetPhysicalAddress(ap_device->GetAddress());
    socket.SetProtocol(1);

    // install packet sockets on nodes.
    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiStaNode);
    packetSocket.Install(wifiApNode);

    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(1000));
    client->SetAttribute("MaxPackets", UintegerValue(8));
    client->SetAttribute("Interval", TimeValue(Seconds(1)));
    client->SetRemote(socket);
    wifiStaNode.Get(0)->AddApplication(client);
    client->SetStartTime(Seconds(1));
    client->SetStopTime(Seconds(3.0));
    Simulator::Schedule(Seconds(1.5),
                        &PacketSocketClient::SetAttribute,
                        client,
                        "Interval",
                        TimeValue(MicroSeconds(0)));

    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
    server->SetLocal(socket);
    wifiApNode.Get(0)->AddApplication(server);
    server->SetStartTime(Seconds(0.0));
    server->SetStopTime(Seconds(4.0));

    sta_device->GetMac()->TraceConnectWithoutContext(
        "MacTx",
        MakeCallback(&PreservePacketsInAmpdus::NotifyMacTransmit, this));
    sta_device->GetPhy()->TraceConnectWithoutContext(
        "PhyTxPsduBegin",
        MakeCallback(&PreservePacketsInAmpdus::NotifyPsduForwardedDown, this));
    ap_device->GetMac()->TraceConnectWithoutContext(
        "MacRx",
        MakeCallback(&PreservePacketsInAmpdus::NotifyMacForwardUp, this));

    Simulator::Stop(Seconds(5));
    Simulator::Run();

    Simulator::Destroy();

    // Two packets are transmitted. The first one is an MPDU containing a single MSDU.
    // The second one is an A-MPDU containing two MPDUs: the first MPDU contains 4 MSDUs
    // and the second MPDU contains 3 MSDUs
    NS_TEST_EXPECT_MSG_EQ(m_nMpdus.size(), 2, "Unexpected number of transmitted packets");
    NS_TEST_EXPECT_MSG_EQ(m_nMsdus.size(), 3, "Unexpected number of transmitted MPDUs");
    NS_TEST_EXPECT_MSG_EQ(m_nMpdus[0], 1, "Unexpected number of MPDUs in the first A-MPDU");
    NS_TEST_EXPECT_MSG_EQ(m_nMsdus[0], 1, "Unexpected number of MSDUs in the first MPDU");
    NS_TEST_EXPECT_MSG_EQ(m_nMpdus[1], 2, "Unexpected number of MPDUs in the second A-MPDU");
    NS_TEST_EXPECT_MSG_EQ(m_nMsdus[1], 4, "Unexpected number of MSDUs in the second MPDU");
    NS_TEST_EXPECT_MSG_EQ(m_nMsdus[2], 3, "Unexpected number of MSDUs in the third MPDU");
    // All the packets must have been forwarded up at the receiver
    NS_TEST_EXPECT_MSG_EQ(m_packetList.empty(), true, "Some packets have not been forwarded up");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Wifi Aggregation Test Suite
 */
class WifiAggregationTestSuite : public TestSuite
{
  public:
    WifiAggregationTestSuite();
};

WifiAggregationTestSuite::WifiAggregationTestSuite()
    : TestSuite("wifi-aggregation", Type::UNIT)
{
    AddTestCase(new AmpduAggregationTest, TestCase::Duration::QUICK);
    AddTestCase(new TwoLevelAggregationTest, TestCase::Duration::QUICK);
    AddTestCase(new HeAggregationTest(64), TestCase::Duration::QUICK);
    AddTestCase(new HeAggregationTest(256), TestCase::Duration::QUICK);
    AddTestCase(new EhtAggregationTest(512), TestCase::Duration::QUICK);
    AddTestCase(new EhtAggregationTest(1024), TestCase::Duration::QUICK);
    AddTestCase(new PreservePacketsInAmpdus, TestCase::Duration::QUICK);
}

static WifiAggregationTestSuite g_wifiAggregationTestSuite; ///< the test suite
