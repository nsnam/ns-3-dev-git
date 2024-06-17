/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#include "bs-scheduler-simple.h"

#include "bs-net-device.h"
#include "burst-profile-manager.h"
#include "cid.h"
#include "connection-manager.h"
#include "service-flow-manager.h"
#include "service-flow-record.h"
#include "service-flow.h"
#include "ss-manager.h"
#include "ss-record.h"
#include "wimax-connection.h"
#include "wimax-mac-header.h"
#include "wimax-mac-queue.h"

#include "ns3/log.h"
#include "ns3/packet-burst.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BSSchedulerSimple");

NS_OBJECT_ENSURE_REGISTERED(BSSchedulerSimple);

TypeId
BSSchedulerSimple::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BSSchedulerSimple")
                            .SetParent<BSScheduler>()
                            .SetGroupName("Wimax")
                            .AddConstructor<BSSchedulerSimple>();
    return tid;
}

BSSchedulerSimple::BSSchedulerSimple()
    : m_downlinkBursts(new std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst>>>())
{
    SetBs(nullptr);
}

BSSchedulerSimple::BSSchedulerSimple(Ptr<BaseStationNetDevice> bs)
    : m_downlinkBursts(new std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst>>>())
{
    // m_downlinkBursts is filled by AddDownlinkBurst and emptied by
    // wimax-bs-net-device::sendBurst and wimax-ss-net-device::sendBurst
    SetBs(bs);
}

BSSchedulerSimple::~BSSchedulerSimple()
{
    std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst>>>* downlinkBursts = m_downlinkBursts;
    std::pair<OfdmDlMapIe*, Ptr<PacketBurst>> pair;
    while (!downlinkBursts->empty())
    {
        pair = downlinkBursts->front();
        pair.second = nullptr;
        delete pair.first;
    }
    SetBs(nullptr);
    delete m_downlinkBursts;
    m_downlinkBursts = nullptr;
}

std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst>>>*
BSSchedulerSimple::GetDownlinkBursts() const
{
    return m_downlinkBursts;
}

void
BSSchedulerSimple::AddDownlinkBurst(Ptr<const WimaxConnection> connection,
                                    uint8_t diuc,
                                    WimaxPhy::ModulationType modulationType,
                                    Ptr<PacketBurst> burst)
{
    auto dlMapIe = new OfdmDlMapIe();
    dlMapIe->SetCid(connection->GetCid());
    dlMapIe->SetDiuc(diuc);

    NS_LOG_INFO("BS scheduler, burst size: " << burst->GetSize() << " bytes"
                                             << ", pkts: " << burst->GetNPackets()
                                             << ", connection: " << connection->GetTypeStr()
                                             << ", CID: " << connection->GetCid());
    if (connection->GetType() == Cid::TRANSPORT)
    {
        NS_LOG_INFO(", SFID: " << connection->GetServiceFlow()->GetSfid() << ", service: "
                               << connection->GetServiceFlow()->GetSchedulingTypeStr());
    }
    NS_LOG_INFO(", modulation: " << modulationType << ", DIUC: " << (uint32_t)diuc);

    m_downlinkBursts->emplace_back(dlMapIe, burst);
}

void
BSSchedulerSimple::Schedule()
{
    Ptr<WimaxConnection> connection;
    WimaxPhy::ModulationType modulationType = WimaxPhy::MODULATION_TYPE_BPSK_12;
    uint8_t diuc = OfdmDlBurstProfile::DIUC_BURST_PROFILE_1;
    uint32_t nrSymbolsRequired = 0;
    GenericMacHeader hdr;
    Ptr<Packet> packet;
    Ptr<PacketBurst> burst;
    ServiceFlow::SchedulingType schedulingType = ServiceFlow::SF_TYPE_NONE;
    uint32_t availableSymbols = GetBs()->GetNrDlSymbols();

    while (SelectConnection(connection))
    {
        if (connection != GetBs()->GetInitialRangingConnection() &&
            connection != GetBs()->GetBroadcastConnection())
        {
            /* determines modulation/DIUC only once per burst as it is always same for a particular
             * CID */
            if (connection->GetType() == Cid::MULTICAST)
            {
                modulationType = connection->GetServiceFlow()->GetModulation();
            }
            else
            {
                modulationType =
                    GetBs()->GetSSManager()->GetSSRecord(connection->GetCid())->GetModulationType();
            }
            diuc = GetBs()->GetBurstProfileManager()->GetBurstProfile(
                modulationType,
                WimaxNetDevice::DIRECTION_DOWNLINK);
        }
        else if (connection == GetBs()->GetInitialRangingConnection() ||
                 connection == GetBs()->GetBroadcastConnection())
        {
            modulationType = WimaxPhy::MODULATION_TYPE_BPSK_12;
            diuc = OfdmDlBurstProfile::DIUC_BURST_PROFILE_1;
        }

        if (connection->GetType() == Cid::TRANSPORT || connection->GetType() == Cid::MULTICAST)
        {
            schedulingType = (ServiceFlow::SchedulingType)connection->GetSchedulingType();
        }

        if (schedulingType == ServiceFlow::SF_TYPE_UGS)
        {
            nrSymbolsRequired = connection->GetServiceFlow()->GetRecord()->GetGrantSize();
            if (nrSymbolsRequired < availableSymbols)
            {
                burst =
                    CreateUgsBurst(connection->GetServiceFlow(), modulationType, nrSymbolsRequired);
            }
            else
            {
                burst =
                    CreateUgsBurst(connection->GetServiceFlow(), modulationType, availableSymbols);
            }
            if (burst->GetNPackets() != 0)
            {
                uint32_t BurstSizeSymbols =
                    GetBs()->GetPhy()->GetNrSymbols(burst->GetSize(), modulationType);
                AddDownlinkBurst(connection, diuc, modulationType, burst);

                if (availableSymbols <= BurstSizeSymbols)
                {
                    availableSymbols -=
                        BurstSizeSymbols; /// \todo Overflows but don't know how to fix
                    break;
                }
            }
        }
        else
        {
            burst = Create<PacketBurst>();
            while (connection->HasPackets())
            {
                uint32_t FirstPacketSize = connection->GetQueue()->GetFirstPacketRequiredByte(
                    MacHeaderType::HEADER_TYPE_GENERIC);
                nrSymbolsRequired =
                    GetBs()->GetPhy()->GetNrSymbols(FirstPacketSize, modulationType);
                if (availableSymbols < nrSymbolsRequired &&
                    CheckForFragmentation(connection, availableSymbols, modulationType))
                {
                    uint32_t availableByte =
                        GetBs()->GetPhy()->GetNrBytes(availableSymbols, modulationType);
                    packet = connection->Dequeue(MacHeaderType::HEADER_TYPE_GENERIC, availableByte);
                    availableSymbols = 0;
                }
                else if (availableSymbols >= nrSymbolsRequired)
                {
                    packet = connection->Dequeue();
                    availableSymbols -= nrSymbolsRequired;
                }
                else
                {
                    break;
                }
                burst->AddPacket(packet);
            }
            AddDownlinkBurst(connection, diuc, modulationType, burst);
        }
        if (availableSymbols == 0)
        {
            break;
        }
    }

    if (!m_downlinkBursts->empty())
    {
        NS_LOG_DEBUG(
            "BS scheduler, number of bursts: "
            << m_downlinkBursts->size() << ", symbols left: " << availableSymbols << std::endl
            << "BS scheduler, queues:"
            << " IR " << GetBs()->GetInitialRangingConnection()->GetQueue()->GetSize()
            << " broadcast " << GetBs()->GetBroadcastConnection()->GetQueue()->GetSize()
            << " basic "
            << GetBs()->GetConnectionManager()->GetNPackets(Cid::BASIC, ServiceFlow::SF_TYPE_NONE)
            << " primary "
            << GetBs()->GetConnectionManager()->GetNPackets(Cid::PRIMARY, ServiceFlow::SF_TYPE_NONE)
            << " transport "
            << GetBs()->GetConnectionManager()->GetNPackets(Cid::TRANSPORT,
                                                            ServiceFlow::SF_TYPE_ALL));
    }
}

bool
BSSchedulerSimple::SelectConnection(Ptr<WimaxConnection>& connection)
{
    connection = nullptr;
    Time currentTime = Simulator::Now();
    ServiceFlowRecord* serviceFlowRecord;
    NS_LOG_INFO("BS Scheduler: Selecting connection...");
    if (GetBs()->GetBroadcastConnection()->HasPackets())
    {
        NS_LOG_INFO("Return GetBroadcastConnection");
        connection = GetBs()->GetBroadcastConnection();
        return true;
    }
    else if (GetBs()->GetInitialRangingConnection()->HasPackets())
    {
        NS_LOG_INFO("Return GetInitialRangingConnection");
        connection = GetBs()->GetInitialRangingConnection();
        return true;
    }
    else
    {
        std::vector<Ptr<WimaxConnection>> connections;
        std::vector<ServiceFlow*> serviceFlows;

        connections = GetBs()->GetConnectionManager()->GetConnections(Cid::BASIC);
        for (auto iter1 = connections.begin(); iter1 != connections.end(); ++iter1)
        {
            if ((*iter1)->HasPackets())
            {
                NS_LOG_INFO("Return Basic");
                connection = *iter1;
                return true;
            }
        }

        connections = GetBs()->GetConnectionManager()->GetConnections(Cid::PRIMARY);
        for (auto iter1 = connections.begin(); iter1 != connections.end(); ++iter1)
        {
            if ((*iter1)->HasPackets())
            {
                NS_LOG_INFO("Return Primary");
                connection = *iter1;
                return true;
            }
        }

        serviceFlows = GetBs()->GetServiceFlowManager()->GetServiceFlows(ServiceFlow::SF_TYPE_UGS);
        for (auto iter2 = serviceFlows.begin(); iter2 != serviceFlows.end(); ++iter2)
        {
            serviceFlowRecord = (*iter2)->GetRecord();
            NS_LOG_INFO("processing UGS: HAS PACKET="
                        << (*iter2)->HasPackets() << "max Latency = "
                        << MilliSeconds((*iter2)->GetMaximumLatency()) << "Delay = "
                        << ((currentTime - serviceFlowRecord->GetDlTimeStamp()) +
                            GetBs()->GetPhy()->GetFrameDuration()));
            // if latency would exceed in case grant is allocated in next frame then allocate in
            // current frame
            if ((*iter2)->HasPackets() && ((currentTime - serviceFlowRecord->GetDlTimeStamp()) +
                                           GetBs()->GetPhy()->GetFrameDuration()) >
                                              MilliSeconds((*iter2)->GetMaximumLatency()))
            {
                serviceFlowRecord->SetDlTimeStamp(currentTime);
                connection = (*iter2)->GetConnection();
                NS_LOG_INFO("Return UGS SF: CID = " << (*iter2)->GetCid()
                                                    << "SFID = " << (*iter2)->GetSfid());
                return true;
            }
        }

        serviceFlows = GetBs()->GetServiceFlowManager()->GetServiceFlows(ServiceFlow::SF_TYPE_RTPS);
        for (auto iter2 = serviceFlows.begin(); iter2 != serviceFlows.end(); ++iter2)
        {
            serviceFlowRecord = (*iter2)->GetRecord();
            // if latency would exceed in case poll is allocated in next frame then allocate in
            // current frame
            if ((*iter2)->HasPackets() && ((currentTime - serviceFlowRecord->GetDlTimeStamp()) +
                                           GetBs()->GetPhy()->GetFrameDuration()) >
                                              MilliSeconds((*iter2)->GetMaximumLatency()))
            {
                serviceFlowRecord->SetDlTimeStamp(currentTime);
                connection = (*iter2)->GetConnection();
                NS_LOG_INFO("Return RTPS SF: CID = " << (*iter2)->GetCid()
                                                     << "SFID = " << (*iter2)->GetSfid());
                return true;
            }
        }

        serviceFlows =
            GetBs()->GetServiceFlowManager()->GetServiceFlows(ServiceFlow::SF_TYPE_NRTPS);
        for (auto iter2 = serviceFlows.begin(); iter2 != serviceFlows.end(); ++iter2)
        {
            // unused: serviceFlowRecord = (*iter2)->GetRecord ();
            if ((*iter2)->HasPackets())
            {
                NS_LOG_INFO("Return NRTPS SF: CID = " << (*iter2)->GetCid()
                                                      << "SFID = " << (*iter2)->GetSfid());
                connection = (*iter2)->GetConnection();
                return true;
            }
        }

        serviceFlows = GetBs()->GetServiceFlowManager()->GetServiceFlows(ServiceFlow::SF_TYPE_BE);
        for (auto iter2 = serviceFlows.begin(); iter2 != serviceFlows.end(); ++iter2)
        {
            // unused: serviceFlowRecord = (*iter2)->GetRecord ();
            if ((*iter2)->HasPackets())
            {
                NS_LOG_INFO("Return BE SF: CID = " << (*iter2)->GetCid()
                                                   << "SFID = " << (*iter2)->GetSfid());
                connection = (*iter2)->GetConnection();
                return true;
            }
        }
    }
    NS_LOG_INFO("NO connection is selected!");
    return false;
}

Ptr<PacketBurst>
BSSchedulerSimple::CreateUgsBurst(ServiceFlow* serviceFlow,
                                  WimaxPhy::ModulationType modulationType,
                                  uint32_t availableSymbols)
{
    Time timeStamp;
    GenericMacHeader hdr;
    Ptr<Packet> packet;
    Ptr<PacketBurst> burst = Create<PacketBurst>();
    uint32_t nrSymbolsRequired = 0;

    // serviceFlow->CleanUpQueue ();
    Ptr<WimaxConnection> connection = serviceFlow->GetConnection();
    while (serviceFlow->HasPackets())
    {
        uint32_t FirstPacketSize =
            connection->GetQueue()->GetFirstPacketRequiredByte(MacHeaderType::HEADER_TYPE_GENERIC);
        nrSymbolsRequired = GetBs()->GetPhy()->GetNrSymbols(FirstPacketSize, modulationType);
        if (availableSymbols < nrSymbolsRequired &&
            CheckForFragmentation(connection, availableSymbols, modulationType))
        {
            uint32_t availableByte =
                GetBs()->GetPhy()->GetNrBytes(availableSymbols, modulationType);
            packet = connection->Dequeue(MacHeaderType::HEADER_TYPE_GENERIC, availableByte);
            availableSymbols = 0;
        }
        else
        {
            packet = connection->Dequeue();
            availableSymbols -= nrSymbolsRequired;
        }
        burst->AddPacket(packet);
        if (availableSymbols <= 0)
        {
            break;
        }
    }
    return burst;
}

} // namespace ns3
