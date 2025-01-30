/*
 * Copyright (c) 2009 Duy Nguyen
 * Copyright (c) 2015 Ghada Badawy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Duy Nguyen <duy@soe.ucsc.edu>
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Matias Richart <mrichart@fing.edu.uy>
 *
 * Some Comments:
 *
 * 1) By default, Minstrel applies the multi-rate retry (the core of Minstrel
 *    algorithm). Otherwise, please use ConstantRateWifiManager instead.
 *
 * 2) Sampling is done differently from legacy Minstrel. Minstrel-HT tries
 * to sample all rates in all groups at least once and to avoid many
 * consecutive samplings.
 *
 * 3) Sample rate is tried only once, at first place of the MRR chain.
 *
 * reference: http://lwn.net/Articles/376765/
 */

#include "minstrel-ht-wifi-manager.h"

#include "ns3/ht-configuration.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"

#include <algorithm>
#include <iomanip>

#define Min(a, b) ((a < b) ? a : b)
#define Max(a, b) ((a > b) ? a : b)

NS_LOG_COMPONENT_DEFINE("MinstrelHtWifiManager");

namespace ns3
{

/**
 * @brief standard information for each modulation class
 */
const std::map<WifiModulationClass, MinstrelHtWifiManager::StandardInfo> minstrelHtStandardInfos{
    {
        WIFI_MOD_CLASS_HT,
        {
            .groupType = WIFI_MINSTREL_GROUP_HT,
            .maxMcs = 7,
            .maxWidth = MHz_u{40},
            .guardIntervals = {NanoSeconds(800), NanoSeconds(400)},
            .maxStreams = 4,
        },
    },
    {
        WIFI_MOD_CLASS_VHT,
        {
            .groupType = WIFI_MINSTREL_GROUP_VHT,
            .maxMcs = 9,
            .maxWidth = MHz_u{160},
            .guardIntervals = {NanoSeconds(800), NanoSeconds(400)},
            .maxStreams = 8,
        },
    },
    {
        WIFI_MOD_CLASS_HE,
        {
            .groupType = WIFI_MINSTREL_GROUP_HE,
            .maxMcs = 11,
            .maxWidth = MHz_u{160},
            .guardIntervals = {NanoSeconds(3200), NanoSeconds(1600), NanoSeconds(800)},
            .maxStreams = 8,
        },
    },
};

/// MinstrelHtWifiRemoteStation structure
struct MinstrelHtWifiRemoteStation : MinstrelWifiRemoteStation
{
    uint8_t m_sampleGroup; //!< The group that the sample rate belongs to.

    uint32_t m_sampleWait;     //!< How many transmission attempts to wait until a new sample.
    uint32_t m_sampleTries;    //!< Number of sample tries after waiting sampleWait.
    uint32_t m_sampleCount;    //!< Max number of samples per update interval.
    uint32_t m_numSamplesSlow; //!< Number of times a slow rate was sampled.

    uint32_t m_avgAmpduLen;      //!< Average number of MPDUs in an A-MPDU.
    uint32_t m_ampduLen;         //!< Number of MPDUs in an A-MPDU.
    uint32_t m_ampduPacketCount; //!< Number of A-MPDUs transmitted.

    McsGroupData m_groupsTable; //!< Table of groups with stats.
    bool m_isHt;                //!< If the station is HT capable.

    std::ofstream m_statsFile; //!< File where statistics table is written.
};

NS_OBJECT_ENSURE_REGISTERED(MinstrelHtWifiManager);

TypeId
MinstrelHtWifiManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MinstrelHtWifiManager")
            .SetParent<WifiRemoteStationManager>()
            .AddConstructor<MinstrelHtWifiManager>()
            .SetGroupName("Wifi")
            .AddAttribute("UpdateStatistics",
                          "The interval between updating statistics table",
                          TimeValue(MilliSeconds(50)),
                          MakeTimeAccessor(&MinstrelHtWifiManager::m_updateStats),
                          MakeTimeChecker())
            .AddAttribute("LegacyUpdateStatistics",
                          "The interval between updating statistics table (for legacy Minstrel)",
                          TimeValue(MilliSeconds(100)),
                          MakeTimeAccessor(&MinstrelHtWifiManager::m_legacyUpdateStats),
                          MakeTimeChecker())
            .AddAttribute("LookAroundRate",
                          "The percentage to try other rates (for legacy Minstrel)",
                          UintegerValue(10),
                          MakeUintegerAccessor(&MinstrelHtWifiManager::m_lookAroundRate),
                          MakeUintegerChecker<uint8_t>(0, 100))
            .AddAttribute("EWMA",
                          "EWMA level",
                          UintegerValue(75),
                          MakeUintegerAccessor(&MinstrelHtWifiManager::m_ewmaLevel),
                          MakeUintegerChecker<uint8_t>(0, 100))
            .AddAttribute("SampleColumn",
                          "The number of columns used for sampling",
                          UintegerValue(10),
                          MakeUintegerAccessor(&MinstrelHtWifiManager::m_nSampleCol),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("PacketLength",
                          "The packet length used for calculating mode TxTime (bytes)",
                          UintegerValue(1200),
                          MakeUintegerAccessor(&MinstrelHtWifiManager::m_frameLength),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("UseLatestAmendmentOnly",
                          "Use only the latest amendment when it is supported by both peers",
                          BooleanValue(true),
                          MakeBooleanAccessor(&MinstrelHtWifiManager::m_useLatestAmendmentOnly),
                          MakeBooleanChecker())
            .AddAttribute("PrintStats",
                          "Control the printing of the statistics table",
                          BooleanValue(false),
                          MakeBooleanAccessor(&MinstrelHtWifiManager::m_printStats),
                          MakeBooleanChecker())
            .AddTraceSource("Rate",
                            "Traced value for rate changes (b/s)",
                            MakeTraceSourceAccessor(&MinstrelHtWifiManager::m_currentRate),
                            "ns3::TracedValueCallback::Uint64");
    return tid;
}

MinstrelHtWifiManager::MinstrelHtWifiManager()
    : m_numGroups(0),
      m_numRates(0),
      m_currentRate(0)
{
    NS_LOG_FUNCTION(this);
    m_uniformRandomVariable = CreateObject<UniformRandomVariable>();
    /**
     *  Create the legacy Minstrel manager in case HT is not supported by the device
     *  or non-HT stations want to associate.
     */
    m_legacyManager = CreateObject<MinstrelWifiManager>();
}

MinstrelHtWifiManager::~MinstrelHtWifiManager()
{
    NS_LOG_FUNCTION(this);
    for (std::size_t i = 0; i < m_numGroups; i++)
    {
        m_minstrelGroups[i].ratesFirstMpduTxTimeTable.clear();
        m_minstrelGroups[i].ratesTxTimeTable.clear();
    }
}

int64_t
MinstrelHtWifiManager::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    int64_t numStreamsAssigned = 0;
    m_uniformRandomVariable->SetStream(stream);
    numStreamsAssigned++;
    numStreamsAssigned += m_legacyManager->AssignStreams(stream);
    return numStreamsAssigned;
}

void
MinstrelHtWifiManager::SetupPhy(const Ptr<WifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    // Setup PHY for legacy manager.
    m_legacyManager->SetupPhy(phy);
    WifiRemoteStationManager::SetupPhy(phy);
}

void
MinstrelHtWifiManager::SetupMac(const Ptr<WifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_legacyManager->SetupMac(mac);
    WifiRemoteStationManager::SetupMac(mac);
}

void
MinstrelHtWifiManager::InitializeGroups(WifiModulationClass mc)
{
    const auto& standardInfos = minstrelHtStandardInfos.at(mc);
    for (MHz_u chWidth{20}; chWidth <= standardInfos.maxWidth; chWidth *= 2)
    {
        for (const auto& guardInterval : standardInfos.guardIntervals)
        {
            for (uint8_t streams = 1; streams <= standardInfos.maxStreams; ++streams)
            {
                const auto groupId =
                    GetGroupIdForType(standardInfos.groupType, streams, guardInterval, chWidth);

                m_minstrelGroups[groupId].streams = streams;
                m_minstrelGroups[groupId].gi = guardInterval;
                m_minstrelGroups[groupId].chWidth = chWidth;
                m_minstrelGroups[groupId].type = standardInfos.groupType;
                m_minstrelGroups[groupId].isSupported = false;

                // Check capabilities of the device
                if ((((mc >= WIFI_MOD_CLASS_HE) &&
                      (GetGuardInterval() <=
                       guardInterval)) || /// Is GI supported by the transmitter?
                     ((mc < WIFI_MOD_CLASS_HE) &&
                      !(!GetShortGuardIntervalSupported() &&
                        (guardInterval.GetNanoSeconds() ==
                         400)))) /// Is SGI supported by the transmitter?
                    && (GetPhy()->GetChannelWidth() >=
                        chWidth) /// Is channel width supported by the transmitter?
                    && (GetPhy()->GetMaxSupportedTxSpatialStreams() >=
                        streams)) /// Are streams supported by the transmitter?
                {
                    m_minstrelGroups[groupId].isSupported = true;

                    // Calculate TX time for all rates of the group
                    WifiModeList mcsList = GetDeviceMcsList(mc);
                    for (uint8_t i = 0; i <= standardInfos.maxMcs; ++i)
                    {
                        const auto mcsIndex = (mc == WIFI_MOD_CLASS_HT)
                                                  ? (i + (m_minstrelGroups[groupId].streams - 1) *
                                                             (standardInfos.maxMcs + 1))
                                                  : i;
                        WifiMode mode = mcsList.at(mcsIndex);
                        // Check for invalid MCSs and do not add time to array.
                        if (IsValidMcs(streams, chWidth, mode))
                        {
                            AddFirstMpduTxTime(groupId,
                                               mode,
                                               CalculateMpduTxDuration(streams,
                                                                       guardInterval,
                                                                       chWidth,
                                                                       mode,
                                                                       FIRST_MPDU_IN_AGGREGATE));
                            AddMpduTxTime(groupId,
                                          mode,
                                          CalculateMpduTxDuration(streams,
                                                                  guardInterval,
                                                                  chWidth,
                                                                  mode,
                                                                  MIDDLE_MPDU_IN_AGGREGATE));
                        }
                    }
                    NS_LOG_DEBUG("Initialized group " << groupId << ": (" << +streams << ","
                                                      << guardInterval << "," << chWidth << ")");
                }
            }
        }
    }
}

void
MinstrelHtWifiManager::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    /**
     * Here we initialize m_minstrelGroups with all the possible groups.
     * If a group is not supported by the device, then it is marked as not supported.
     * Then, after all initializations are finished, we check actual support for each receiving
     * station.
     */

    m_numGroups = 0;
    if (GetHtSupported())
    {
        m_numGroups += GetNumGroups(WIFI_MOD_CLASS_HT);
        m_numRates = minstrelHtStandardInfos.at(WIFI_MOD_CLASS_HT).maxMcs + 1;
    }
    if (GetVhtSupported())
    {
        m_numGroups += GetNumGroups(WIFI_MOD_CLASS_VHT);
        m_numRates = minstrelHtStandardInfos.at(WIFI_MOD_CLASS_VHT).maxMcs + 1;
    }
    if (GetHeSupported())
    {
        m_numGroups += GetNumGroups(WIFI_MOD_CLASS_HE);
        m_numRates = minstrelHtStandardInfos.at(WIFI_MOD_CLASS_HE).maxMcs + 1;
    }

    /**
     *  Initialize the groups array.
     *  The HT groups come first, then the VHT ones, and finally the HE ones.
     *  Minstrel maintains different types of indexes:
     *  - A global continuous index, which identifies all rates within all groups, in [0,
     * m_numGroups * m_numRates]
     *  - A groupId, which indexes a group in the array, in [0, m_numGroups]
     *  - A rateId, which identifies a rate within a group, in [0, m_numRates]
     *  - A deviceIndex, which indexes a MCS in the PHY MCS array.
     *  - A mcsIndex, which indexes a MCS in the wifi-remote-station-manager supported MCSs
     * array.
     */
    NS_LOG_DEBUG("Initialize MCS Groups:");
    m_minstrelGroups = MinstrelMcsGroups(m_numGroups);

    if (GetHtSupported())
    {
        // Initialize all HT groups
        InitializeGroups(WIFI_MOD_CLASS_HT);
    }

    if (GetVhtSupported())
    {
        // Initialize all VHT groups
        InitializeGroups(WIFI_MOD_CLASS_VHT);
    }

    if (GetHeSupported())
    {
        // Initialize all HE groups
        InitializeGroups(WIFI_MOD_CLASS_HE);
    }
}

bool
MinstrelHtWifiManager::IsValidMcs(uint8_t streams, MHz_u chWidth, WifiMode mode)
{
    NS_LOG_FUNCTION(this << streams << chWidth << mode);
    WifiTxVector txvector;
    txvector.SetNss(streams);
    txvector.SetChannelWidth(chWidth);
    txvector.SetMode(mode);
    return txvector.IsValid();
}

bool
MinstrelHtWifiManager::ShouldAddMcsToGroup(WifiMode mode, std::size_t groupId)
{
    for (const auto& [mc, infos] : minstrelHtStandardInfos)
    {
        if ((m_minstrelGroups[groupId].type == infos.groupType) &&
            (mode.GetModulationClass() == mc) &&
            IsValidMcs(m_minstrelGroups[groupId].streams, m_minstrelGroups[groupId].chWidth, mode))
        {
            if (mc == WIFI_MOD_CLASS_HT)
            {
                /// Check if the HT MCS corresponds to groups number of streams.
                return ((mode.GetMcsValue() < (m_minstrelGroups[groupId].streams * 8)) &&
                        (mode.GetMcsValue() >= ((m_minstrelGroups[groupId].streams - 1) * 8)));
            }
            return true;
        }
    }
    return false;
}

Time
MinstrelHtWifiManager::CalculateMpduTxDuration(uint8_t streams,
                                               Time gi,
                                               MHz_u chWidth,
                                               WifiMode mode,
                                               MpduType mpduType)
{
    NS_LOG_FUNCTION(this << streams << gi << chWidth << mode << mpduType);
    WifiTxVector txvector{};
    txvector.SetNss(streams);
    txvector.SetGuardInterval(gi);
    txvector.SetChannelWidth(chWidth);
    txvector.SetMode(mode);
    txvector.SetPreambleType(GetPreambleForTransmission(mode.GetModulationClass()));
    return WifiPhy::CalculatePhyPreambleAndHeaderDuration(txvector) +
           WifiPhy::GetPayloadDuration(m_frameLength, txvector, GetPhy()->GetPhyBand(), mpduType);
}

Time
MinstrelHtWifiManager::GetFirstMpduTxTime(std::size_t groupId, WifiMode mode) const
{
    NS_LOG_FUNCTION(this << groupId << mode);
    const auto it = m_minstrelGroups[groupId].ratesFirstMpduTxTimeTable.find(mode);
    NS_ASSERT(it != m_minstrelGroups[groupId].ratesFirstMpduTxTimeTable.cend());
    return it->second;
}

void
MinstrelHtWifiManager::AddFirstMpduTxTime(std::size_t groupId, WifiMode mode, Time t)
{
    NS_LOG_FUNCTION(this << groupId << mode << t);
    m_minstrelGroups[groupId].ratesFirstMpduTxTimeTable.insert(std::make_pair(mode, t));
}

Time
MinstrelHtWifiManager::GetMpduTxTime(std::size_t groupId, WifiMode mode) const
{
    NS_LOG_FUNCTION(this << groupId << mode);
    auto it = m_minstrelGroups[groupId].ratesTxTimeTable.find(mode);
    NS_ASSERT(it != m_minstrelGroups[groupId].ratesTxTimeTable.end());
    return it->second;
}

void
MinstrelHtWifiManager::AddMpduTxTime(std::size_t groupId, WifiMode mode, Time t)
{
    NS_LOG_FUNCTION(this << groupId << mode << t);
    m_minstrelGroups[groupId].ratesTxTimeTable.insert(std::make_pair(mode, t));
}

WifiRemoteStation*
MinstrelHtWifiManager::DoCreateStation() const
{
    NS_LOG_FUNCTION(this);
    auto station = new MinstrelHtWifiRemoteStation();

    // Initialize variables common to both stations.
    station->m_nextStatsUpdate = Simulator::Now() + m_updateStats;
    station->m_col = 0;
    station->m_index = 0;
    station->m_maxTpRate = 0;
    station->m_maxTpRate2 = 0;
    station->m_maxProbRate = 0;
    station->m_nModes = 0;
    station->m_totalPacketsCount = 0;
    station->m_samplePacketsCount = 0;
    station->m_isSampling = false;
    station->m_sampleRate = 0;
    station->m_sampleDeferred = false;
    station->m_shortRetry = 0;
    station->m_longRetry = 0;
    station->m_txrate = 0;
    station->m_initialized = false;

    // Variables specific to HT station
    station->m_sampleGroup = 0;
    station->m_numSamplesSlow = 0;
    station->m_sampleCount = 16;
    station->m_sampleWait = 0;
    station->m_sampleTries = 4;

    station->m_avgAmpduLen = 1;
    station->m_ampduLen = 0;
    station->m_ampduPacketCount = 0;

    // Use the variable in the station to indicate whether the device supports HT.
    // When correct information available it will be checked.
    station->m_isHt = static_cast<bool>(GetPhy()->GetDevice()->GetHtConfiguration());

    return station;
}

void
MinstrelHtWifiManager::CheckInit(MinstrelHtWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
    // Note: we appear to be doing late initialization of the table
    // to make sure that the set of supported rates has been initialized
    // before we perform our own initialization.
    if (!station->m_initialized)
    {
        /**
         *  Check if the station supports HT.
         *  Assume that if the device do not supports HT then
         *  the station will not support HT either.
         *  We save from using another check and variable.
         */
        if (!GetHtSupported(station) && !GetStationHe6GhzCapabilities(station->m_state->m_address))
        {
            NS_LOG_INFO("non-HT station " << station);
            station->m_isHt = false;
            // We will use non-HT minstrel for this station. Initialize the manager.
            m_legacyManager->SetAttribute("UpdateStatistics", TimeValue(m_legacyUpdateStats));
            m_legacyManager->SetAttribute("LookAroundRate", UintegerValue(m_lookAroundRate));
            m_legacyManager->SetAttribute("EWMA", UintegerValue(m_ewmaLevel));
            m_legacyManager->SetAttribute("SampleColumn", UintegerValue(m_nSampleCol));
            m_legacyManager->SetAttribute("PacketLength", UintegerValue(m_frameLength));
            m_legacyManager->SetAttribute("PrintStats", BooleanValue(m_printStats));
            m_legacyManager->CheckInit(station);
        }
        else
        {
            NS_LOG_DEBUG("HT station " << station);
            station->m_isHt = true;
            station->m_nModes = GetNMcsSupported(station);
            station->m_minstrelTable = MinstrelRate(station->m_nModes);
            station->m_sampleTable = SampleRate(m_numRates, std::vector<uint8_t>(m_nSampleCol));
            InitSampleTable(station);
            RateInit(station);
            station->m_initialized = true;
        }
    }
}

void
MinstrelHtWifiManager::DoReportRxOk(WifiRemoteStation* st, double rxSnr, WifiMode txMode)
{
    NS_LOG_FUNCTION(this << st);
    NS_LOG_DEBUG(
        "DoReportRxOk m_txrate=" << static_cast<MinstrelHtWifiRemoteStation*>(st)->m_txrate);
}

void
MinstrelHtWifiManager::DoReportRtsFailed(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    auto station = static_cast<MinstrelHtWifiRemoteStation*>(st);
    CheckInit(station);
    if (!station->m_initialized)
    {
        return;
    }
    NS_LOG_DEBUG("DoReportRtsFailed m_txrate = " << station->m_txrate);
    station->m_shortRetry++;
}

void
MinstrelHtWifiManager::DoReportRtsOk(WifiRemoteStation* st,
                                     double ctsSnr,
                                     WifiMode ctsMode,
                                     double rtsSnr)
{
    NS_LOG_FUNCTION(this << st);
}

void
MinstrelHtWifiManager::DoReportFinalRtsFailed(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    auto station = static_cast<MinstrelHtWifiRemoteStation*>(st);
    NS_LOG_DEBUG("Final RTS failed");
    CheckInit(station);
    if (!station->m_initialized)
    {
        return;
    }
    UpdateRetry(station);
}

void
MinstrelHtWifiManager::DoReportDataFailed(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    auto station = static_cast<MinstrelHtWifiRemoteStation*>(st);

    CheckInit(station);
    if (!station->m_initialized)
    {
        return;
    }

    NS_LOG_DEBUG("DoReportDataFailed " << station << "\t rate " << station->m_txrate
                                       << "\tlongRetry \t" << station->m_longRetry);

    if (!station->m_isHt)
    {
        m_legacyManager->UpdateRate(station);
    }
    else if (station->m_longRetry < CountRetries(station))
    {
        const auto rateId = GetRateId(station->m_txrate);
        const auto groupId = GetGroupId(station->m_txrate);
        station->m_groupsTable[groupId]
            .m_ratesTable[rateId]
            .numRateAttempt++; // Increment the attempts counter for the rate used.
        UpdateRate(station);
    }
}

void
MinstrelHtWifiManager::DoReportDataOk(WifiRemoteStation* st,
                                      double ackSnr,
                                      WifiMode ackMode,
                                      double dataSnr,
                                      MHz_u dataChannelWidth,
                                      uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << ackSnr << ackMode << dataSnr << dataChannelWidth << dataNss);
    auto station = static_cast<MinstrelHtWifiRemoteStation*>(st);

    CheckInit(station);
    if (!station->m_initialized)
    {
        return;
    }

    if (!station->m_isHt)
    {
        NS_LOG_DEBUG("DoReportDataOk m_txrate = "
                     << station->m_txrate
                     << ", attempt = " << station->m_minstrelTable[station->m_txrate].numRateAttempt
                     << ", success = " << station->m_minstrelTable[station->m_txrate].numRateSuccess
                     << " (before update).");

        station->m_minstrelTable[station->m_txrate].numRateSuccess++;
        station->m_minstrelTable[station->m_txrate].numRateAttempt++;

        m_legacyManager->UpdatePacketCounters(station);

        NS_LOG_DEBUG("DoReportDataOk m_txrate = "
                     << station->m_txrate
                     << ", attempt = " << station->m_minstrelTable[station->m_txrate].numRateAttempt
                     << ", success = " << station->m_minstrelTable[station->m_txrate].numRateSuccess
                     << " (after update).");

        UpdateRetry(station);
        m_legacyManager->UpdateStats(station);

        if (station->m_nModes >= 1)
        {
            station->m_txrate = m_legacyManager->FindRate(station);
        }
    }
    else
    {
        const auto rateId = GetRateId(station->m_txrate);
        const auto groupId = GetGroupId(station->m_txrate);

        NS_LOG_DEBUG(
            "DoReportDataOk m_txrate = "
            << station->m_txrate
            << ", attempt = " << station->m_groupsTable[groupId].m_ratesTable[rateId].numRateAttempt
            << ", success = " << station->m_groupsTable[groupId].m_ratesTable[rateId].numRateSuccess
            << " (before update).");

        station->m_groupsTable[groupId].m_ratesTable[rateId].numRateSuccess++;
        station->m_groupsTable[groupId].m_ratesTable[rateId].numRateAttempt++;

        UpdatePacketCounters(station, 1, 0);

        NS_LOG_DEBUG(
            "DoReportDataOk m_txrate = "
            << station->m_txrate
            << ", attempt = " << station->m_groupsTable[groupId].m_ratesTable[rateId].numRateAttempt
            << ", success = " << station->m_groupsTable[groupId].m_ratesTable[rateId].numRateSuccess
            << " (after update).");

        station->m_isSampling = false;
        station->m_sampleDeferred = false;

        UpdateRetry(station);
        if (Simulator::Now() >= station->m_nextStatsUpdate)
        {
            UpdateStats(station);
        }

        if (station->m_nModes >= 1)
        {
            station->m_txrate = FindRate(station);
        }
    }

    NS_LOG_DEBUG("Next rate to use TxRate = " << station->m_txrate);
}

void
MinstrelHtWifiManager::DoReportFinalDataFailed(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    auto station = static_cast<MinstrelHtWifiRemoteStation*>(st);

    CheckInit(station);
    if (!station->m_initialized)
    {
        return;
    }

    NS_LOG_DEBUG("DoReportFinalDataFailed - TxRate=" << station->m_txrate);

    if (!station->m_isHt)
    {
        m_legacyManager->UpdatePacketCounters(station);

        UpdateRetry(station);

        m_legacyManager->UpdateStats(station);
        if (station->m_nModes >= 1)
        {
            station->m_txrate = m_legacyManager->FindRate(station);
        }
    }
    else
    {
        UpdatePacketCounters(station, 0, 1);

        station->m_isSampling = false;
        station->m_sampleDeferred = false;

        UpdateRetry(station);
        if (Simulator::Now() >= station->m_nextStatsUpdate)
        {
            UpdateStats(station);
        }

        if (station->m_nModes >= 1)
        {
            station->m_txrate = FindRate(station);
        }
    }
    NS_LOG_DEBUG("Next rate to use TxRate = " << station->m_txrate);
}

void
MinstrelHtWifiManager::DoReportAmpduTxStatus(WifiRemoteStation* st,
                                             uint16_t nSuccessfulMpdus,
                                             uint16_t nFailedMpdus,
                                             double rxSnr,
                                             double dataSnr,
                                             MHz_u dataChannelWidth,
                                             uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << nSuccessfulMpdus << nFailedMpdus << rxSnr << dataSnr
                         << dataChannelWidth << dataNss);
    auto station = static_cast<MinstrelHtWifiRemoteStation*>(st);

    CheckInit(station);
    if (!station->m_initialized)
    {
        return;
    }

    NS_ASSERT_MSG(station->m_isHt, "A-MPDU Tx Status called but this is a non-HT STA.");

    NS_LOG_DEBUG("DoReportAmpduTxStatus. TxRate=" << station->m_txrate
                                                  << " SuccMpdus=" << nSuccessfulMpdus
                                                  << " FailedMpdus=" << nFailedMpdus);

    station->m_ampduPacketCount++;
    station->m_ampduLen += nSuccessfulMpdus + nFailedMpdus;

    UpdatePacketCounters(station, nSuccessfulMpdus, nFailedMpdus);

    const auto rateId = GetRateId(station->m_txrate);
    const auto groupId = GetGroupId(station->m_txrate);
    station->m_groupsTable[groupId].m_ratesTable[rateId].numRateSuccess += nSuccessfulMpdus;
    station->m_groupsTable[groupId].m_ratesTable[rateId].numRateAttempt +=
        nSuccessfulMpdus + nFailedMpdus;

    if (nSuccessfulMpdus == 0 && station->m_longRetry < CountRetries(station))
    {
        // We do not receive a BlockAck. The entire AMPDU fail.
        UpdateRate(station);
    }
    else
    {
        station->m_isSampling = false;
        station->m_sampleDeferred = false;

        UpdateRetry(station);
        if (Simulator::Now() >= station->m_nextStatsUpdate)
        {
            UpdateStats(station);
        }

        if (station->m_nModes >= 1)
        {
            station->m_txrate = FindRate(station);
        }
        NS_LOG_DEBUG("Next rate to use TxRate = " << station->m_txrate);
    }
}

void
MinstrelHtWifiManager::UpdateRate(MinstrelHtWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);

    /**
     * Retry Chain table is implemented here.
     *
     * FIXME
     * Currently, NS3 does not retransmit an entire A-MPDU when BACK is missing
     * but retransmits each MPDU until MPDUs lifetime expires (or a BACK is received).
     * Then, there is no way to control A-MPDU retries (no call to NeedDataRetransmission).
     * So, it is possible that the A-MPDU keeps retrying after longRetry reaches its limit.
     *
     *
     * Try |     LOOKAROUND RATE     | NORMAL RATE
     * -------------------------------------------------------
     *  1  |  Random rate            | Best throughput
     *  2  |  Next best throughput   | Next best throughput
     *  3  |  Best probability       | Best probability
     *
     * Note: For clarity, multiple blocks of if's and else's are used
     * Following implementation in Linux, in MinstrelHT lowest base rate is not used.
     * Explanation can be found here: http://marc.info/?l=linux-wireless&m=144602778611966&w=2
     */

    CheckInit(station);
    if (!station->m_initialized)
    {
        return;
    }
    station->m_longRetry++;

    /**
     * Get the IDs for all rates.
     */
    const auto maxTpRateId = GetRateId(station->m_maxTpRate);
    const auto maxTpGroupId = GetGroupId(station->m_maxTpRate);
    const auto maxTp2RateId = GetRateId(station->m_maxTpRate2);
    const auto maxTp2GroupId = GetGroupId(station->m_maxTpRate2);
    const auto maxProbRateId = GetRateId(station->m_maxProbRate);
    const auto maxProbGroupId = GetGroupId(station->m_maxProbRate);

    /// For normal rate, we're not currently sampling random rates.
    if (!station->m_isSampling)
    {
        /// Use best throughput rate.
        if (station->m_longRetry <
            station->m_groupsTable[maxTpGroupId].m_ratesTable[maxTpRateId].retryCount)
        {
            NS_LOG_DEBUG("Not Sampling; use the same rate again");
            station->m_txrate = station->m_maxTpRate; //!<  There are still a few retries.
        }

        /// Use second best throughput rate.
        else if (station->m_longRetry <
                 (station->m_groupsTable[maxTpGroupId].m_ratesTable[maxTpRateId].retryCount +
                  station->m_groupsTable[maxTp2GroupId].m_ratesTable[maxTp2RateId].retryCount))
        {
            NS_LOG_DEBUG("Not Sampling; use the Max TP2");
            station->m_txrate = station->m_maxTpRate2;
        }

        /// Use best probability rate.
        else if (station->m_longRetry <=
                 (station->m_groupsTable[maxTpGroupId].m_ratesTable[maxTpRateId].retryCount +
                  station->m_groupsTable[maxTp2GroupId].m_ratesTable[maxTp2RateId].retryCount +
                  station->m_groupsTable[maxProbGroupId].m_ratesTable[maxProbRateId].retryCount))
        {
            NS_LOG_DEBUG("Not Sampling; use Max Prob");
            station->m_txrate = station->m_maxProbRate;
        }
        else
        {
            NS_FATAL_ERROR("Max retries reached and m_longRetry not cleared properly. longRetry= "
                           << station->m_longRetry);
        }
    }

    /// We're currently sampling random rates.
    else
    {
        /// Sample rate is used only once
        /// Use the best rate.
        if (station->m_longRetry <
            1 + station->m_groupsTable[maxTpGroupId].m_ratesTable[maxTp2RateId].retryCount)
        {
            NS_LOG_DEBUG("Sampling use the MaxTP rate");
            station->m_txrate = station->m_maxTpRate2;
        }

        /// Use the best probability rate.
        else if (station->m_longRetry <=
                 1 + station->m_groupsTable[maxTpGroupId].m_ratesTable[maxTp2RateId].retryCount +
                     station->m_groupsTable[maxProbGroupId].m_ratesTable[maxProbRateId].retryCount)
        {
            NS_LOG_DEBUG("Sampling use the MaxProb rate");
            station->m_txrate = station->m_maxProbRate;
        }
        else
        {
            NS_FATAL_ERROR("Max retries reached and m_longRetry not cleared properly. longRetry= "
                           << station->m_longRetry);
        }
    }
    NS_LOG_DEBUG("Next rate to use TxRate = " << station->m_txrate);
}

void
MinstrelHtWifiManager::UpdateRetry(MinstrelHtWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
    station->m_shortRetry = 0;
    station->m_longRetry = 0;
}

void
MinstrelHtWifiManager::UpdatePacketCounters(MinstrelHtWifiRemoteStation* station,
                                            uint16_t nSuccessfulMpdus,
                                            uint16_t nFailedMpdus)
{
    NS_LOG_FUNCTION(this << station << nSuccessfulMpdus << nFailedMpdus);

    station->m_totalPacketsCount += nSuccessfulMpdus + nFailedMpdus;
    if (station->m_isSampling)
    {
        station->m_samplePacketsCount += nSuccessfulMpdus + nFailedMpdus;
    }
    if (station->m_totalPacketsCount == ~0)
    {
        station->m_samplePacketsCount = 0;
        station->m_totalPacketsCount = 0;
    }

    if (!station->m_sampleWait && !station->m_sampleTries && station->m_sampleCount > 0)
    {
        station->m_sampleWait = 16 + 2 * station->m_avgAmpduLen;
        station->m_sampleTries = 1;
        station->m_sampleCount--;
    }
}

uint16_t
MinstrelHtWifiManager::UpdateRateAfterAllowedWidth(uint16_t txRate, MHz_u allowedWidth)
{
    NS_LOG_FUNCTION(this << txRate << allowedWidth);

    auto groupId = GetGroupId(txRate);
    McsGroup group = m_minstrelGroups[groupId];

    if (group.chWidth <= allowedWidth)
    {
        NS_LOG_DEBUG("Channel width is not greater than allowed width, nothing to do");
        return txRate;
    }

    NS_ASSERT(GetPhy()->GetDevice()->GetHtConfiguration() != nullptr);
    NS_ASSERT(static_cast<uint16_t>(group.chWidth) % 20 == 0);
    // try halving the channel width and check if the group with the same number of
    // streams and same GI is supported, until either a supported group is found or
    // the width becomes lower than 20 MHz
    auto width = group.chWidth / 2;

    while (width >= MHz_u{20})
    {
        if (width > allowedWidth)
        {
            width /= 2;
            continue;
        }
        groupId = GetGroupIdForType(group.type, group.streams, group.gi, width);
        group = m_minstrelGroups[groupId];
        if (group.isSupported)
        {
            break;
        }
        width /= 2;
    }

    NS_ABORT_MSG_IF(width < MHz_u{20}, "No rate compatible with the allowed width found");

    return GetIndex(groupId, GetRateId(txRate));
}

WifiTxVector
MinstrelHtWifiManager::DoGetDataTxVector(WifiRemoteStation* st, MHz_u allowedWidth)
{
    NS_LOG_FUNCTION(this << st << allowedWidth);
    auto station = static_cast<MinstrelHtWifiRemoteStation*>(st);

    if (!station->m_initialized)
    {
        CheckInit(station);
    }

    if (!station->m_isHt)
    {
        WifiTxVector vector = m_legacyManager->GetDataTxVector(station);
        uint64_t dataRate = vector.GetMode().GetDataRate(vector);
        if (m_currentRate != dataRate && !station->m_isSampling)
        {
            NS_LOG_DEBUG("New datarate: " << dataRate);
            m_currentRate = dataRate;
        }
        return vector;
    }

    station->m_txrate = UpdateRateAfterAllowedWidth(station->m_txrate, allowedWidth);
    NS_LOG_DEBUG("DoGetDataMode m_txrate= " << station->m_txrate);

    const auto rateId = GetRateId(station->m_txrate);
    const auto groupId = GetGroupId(station->m_txrate);
    const auto mcsIndex = station->m_groupsTable[groupId].m_ratesTable[rateId].mcsIndex;

    NS_LOG_DEBUG("DoGetDataMode rateId= " << rateId << " groupId= " << groupId
                                          << " mode= " << GetMcsSupported(station, mcsIndex));

    McsGroup group = m_minstrelGroups[groupId];

    // Check consistency of rate selected.
    if (((group.type >= WIFI_MINSTREL_GROUP_HE) && (group.gi < GetGuardInterval(station))) ||
        ((group.type < WIFI_MINSTREL_GROUP_HE) && (group.gi.GetNanoSeconds() == 400) &&
         !GetShortGuardIntervalSupported(station)) ||
        (group.chWidth > GetChannelWidth(station)) ||
        (group.streams > GetNumberOfSupportedStreams(station)))
    {
        NS_FATAL_ERROR("Inconsistent group selected. Group: ("
                       << +group.streams << "," << group.gi << "," << group.chWidth << ")"
                       << " Station capabilities: (" << GetNumberOfSupportedStreams(station) << ","
                       << ((group.type >= WIFI_MINSTREL_GROUP_HE)
                               ? GetGuardInterval(station)
                               : NanoSeconds(GetShortGuardIntervalSupported(station) ? 400 : 800))
                       << "," << GetChannelWidth(station) << ")");
    }
    WifiMode mode = GetMcsSupported(station, mcsIndex);
    WifiTxVector txVector{
        mode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(mode.GetModulationClass(), GetShortPreambleEnabled()),
        group.gi,
        GetNumberOfAntennas(),
        group.streams,
        GetNess(station),
        GetPhy()->GetTxBandwidth(mode, group.chWidth),
        GetAggregation(station) && !station->m_isSampling};
    uint64_t dataRate = mode.GetDataRate(txVector);
    if (m_currentRate != dataRate && !station->m_isSampling)
    {
        NS_LOG_DEBUG("New datarate: " << dataRate);
        m_currentRate = dataRate;
    }
    return txVector;
}

WifiTxVector
MinstrelHtWifiManager::DoGetRtsTxVector(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    auto station = static_cast<MinstrelHtWifiRemoteStation*>(st);

    if (!station->m_initialized)
    {
        CheckInit(station);
    }

    if (!station->m_isHt)
    {
        return m_legacyManager->GetRtsTxVector(station);
    }
    else
    {
        NS_LOG_DEBUG("DoGetRtsMode m_txrate=" << station->m_txrate);

        /* RTS is sent in a non-HT frame. RTS with HT is not supported yet in NS3.
         * When supported, decision of using HT has to follow rules in Section 9.7.6 from
         * 802.11-2012. From Sec. 9.7.6.5: "A frame other than a BlockAckReq or BlockAck that is
         * carried in a non-HT PPDU shall be transmitted by the STA using a rate no higher than the
         * highest rate in  the BSSBasicRateSet parameter that is less than or equal to the rate or
         * non-HT reference rate (see 9.7.9) of the previously transmitted frame that was
         * directed to the same receiving STA. If no rate in the BSSBasicRateSet parameter meets
         * these conditions, the control frame shall be transmitted at a rate no higher than the
         * highest mandatory rate of the attached PHY that is less than or equal to the rate
         * or non-HT reference rate (see 9.7.9) of the previously transmitted frame that was
         * directed to the same receiving STA."
         */

        // As we are in Minstrel HT, assume the last rate was an HT rate.
        const auto rateId = GetRateId(station->m_txrate);
        const auto groupId = GetGroupId(station->m_txrate);
        const auto mcsIndex = station->m_groupsTable[groupId].m_ratesTable[rateId].mcsIndex;

        const auto lastRate = GetMcsSupported(station, mcsIndex);
        const auto lastDataRate = lastRate.GetNonHtReferenceRate();
        const auto nBasicRates = GetNBasicModes();

        WifiMode rtsRate;
        bool rateFound = false;

        for (uint8_t i = 0; i < nBasicRates; i++)
        {
            const auto rate = GetBasicMode(i).GetDataRate(MHz_u{20});
            if (rate <= lastDataRate)
            {
                rtsRate = GetBasicMode(i);
                rateFound = true;
            }
        }

        if (!rateFound)
        {
            for (const auto& mode : GetPhy()->GetModeList())
            {
                const auto rate = mode.GetDataRate(MHz_u{20});
                if (rate <= lastDataRate)
                {
                    rtsRate = mode;
                    rateFound = true;
                }
            }
        }

        NS_ASSERT(rateFound);

        return WifiTxVector(
            rtsRate,
            GetDefaultTxPowerLevel(),
            GetPreambleForTransmission(rtsRate.GetModulationClass(), GetShortPreambleEnabled()),
            NanoSeconds(800),
            1,
            1,
            0,
            GetPhy()->GetTxBandwidth(rtsRate, GetChannelWidth(station)),
            GetAggregation(station));
    }
}

std::list<Ptr<WifiMpdu>>
MinstrelHtWifiManager::DoGetMpdusToDropOnTxFailure(WifiRemoteStation* station, Ptr<WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);

    std::list<Ptr<WifiMpdu>> mpdusToDrop;

    for (const auto& mpdu : *PeekPointer(psdu))
    {
        if (!DoNeedRetransmission(station,
                                  mpdu->GetPacket(),
                                  (mpdu->GetRetryCount() < GetMac()->GetFrameRetryLimit())))
        {
            // this MPDU needs to be dropped
            mpdusToDrop.push_back(mpdu);
        }
    }

    return mpdusToDrop;
}

bool
MinstrelHtWifiManager::DoNeedRetransmission(WifiRemoteStation* st,
                                            Ptr<const Packet> packet,
                                            bool normally)
{
    NS_LOG_FUNCTION(this << st << packet << normally);

    auto station = static_cast<MinstrelHtWifiRemoteStation*>(st);

    CheckInit(station);
    if (!station->m_initialized)
    {
        return normally;
    }

    uint32_t maxRetries;

    if (!station->m_isHt)
    {
        maxRetries = m_legacyManager->CountRetries(station);
    }
    else
    {
        maxRetries = CountRetries(station);
    }

    if (station->m_longRetry >= maxRetries)
    {
        NS_LOG_DEBUG("No re-transmission allowed. Retries: " << station->m_longRetry
                                                             << " Max retries: " << maxRetries);
        return false;
    }
    else
    {
        NS_LOG_DEBUG("Re-transmit. Retries: " << station->m_longRetry
                                              << " Max retries: " << maxRetries);
        return true;
    }
}

uint32_t
MinstrelHtWifiManager::CountRetries(MinstrelHtWifiRemoteStation* station)
{
    const auto maxProbRateId = GetRateId(station->m_maxProbRate);
    const auto maxProbGroupId = GetGroupId(station->m_maxProbRate);
    const auto maxTpRateId = GetRateId(station->m_maxTpRate);
    const auto maxTpGroupId = GetGroupId(station->m_maxTpRate);
    const auto maxTp2RateId = GetRateId(station->m_maxTpRate2);
    const auto maxTp2GroupId = GetGroupId(station->m_maxTpRate2);

    if (!station->m_isSampling)
    {
        return station->m_groupsTable[maxTpGroupId].m_ratesTable[maxTpRateId].retryCount +
               station->m_groupsTable[maxTp2GroupId].m_ratesTable[maxTp2RateId].retryCount +
               station->m_groupsTable[maxProbGroupId].m_ratesTable[maxProbRateId].retryCount;
    }
    else
    {
        return 1 + station->m_groupsTable[maxTpGroupId].m_ratesTable[maxTp2RateId].retryCount +
               station->m_groupsTable[maxProbGroupId].m_ratesTable[maxProbRateId].retryCount;
    }
}

uint16_t
MinstrelHtWifiManager::GetNextSample(MinstrelHtWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
    const auto sampleGroup = station->m_sampleGroup;
    const auto index = station->m_groupsTable[sampleGroup].m_index;
    const auto col = station->m_groupsTable[sampleGroup].m_col;
    const auto sampleIndex = station->m_sampleTable[index][col];
    const auto rateIndex = GetIndex(sampleGroup, sampleIndex);
    NS_LOG_DEBUG("Next Sample is " << rateIndex);
    SetNextSample(station); // Calculate the next sample rate.
    return rateIndex;
}

void
MinstrelHtWifiManager::SetNextSample(MinstrelHtWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
    do
    {
        station->m_sampleGroup++;
        station->m_sampleGroup %= m_numGroups;
    } while (!station->m_groupsTable[station->m_sampleGroup].m_supported);

    station->m_groupsTable[station->m_sampleGroup].m_index++;

    const auto sampleGroup = station->m_sampleGroup;
    auto index = station->m_groupsTable[station->m_sampleGroup].m_index;
    auto col = station->m_groupsTable[sampleGroup].m_col;

    if (index >= m_numRates)
    {
        station->m_groupsTable[station->m_sampleGroup].m_index = 0;
        station->m_groupsTable[station->m_sampleGroup].m_col++;
        if (station->m_groupsTable[station->m_sampleGroup].m_col >= m_nSampleCol)
        {
            station->m_groupsTable[station->m_sampleGroup].m_col = 0;
        }
        index = station->m_groupsTable[station->m_sampleGroup].m_index;
        col = station->m_groupsTable[sampleGroup].m_col;
    }
    NS_LOG_DEBUG("New sample set: group= " << +sampleGroup
                                           << " index= " << +station->m_sampleTable[index][col]);
}

uint16_t
MinstrelHtWifiManager::FindRate(MinstrelHtWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
    NS_LOG_DEBUG("FindRate packet=" << station->m_totalPacketsCount);

    if ((station->m_samplePacketsCount + station->m_totalPacketsCount) == 0)
    {
        return station->m_maxTpRate;
    }

    // If we have waited enough, then sample.
    if (station->m_sampleWait == 0 && station->m_sampleTries != 0)
    {
        // SAMPLING
        NS_LOG_DEBUG("Obtaining a sampling rate");
        /// Now go through the table and find an index rate.
        const auto sampleIdx = GetNextSample(station);
        NS_LOG_DEBUG("Sampling rate = " << sampleIdx);

        // Evaluate if the sampling rate selected should be used.
        const auto sampleGroupId = GetGroupId(sampleIdx);
        const auto sampleRateId = GetRateId(sampleIdx);

        // If the rate selected is not supported, then don't sample.
        if (station->m_groupsTable[sampleGroupId].m_supported &&
            station->m_groupsTable[sampleGroupId].m_ratesTable[sampleRateId].supported)
        {
            /**
             * Sampling might add some overhead to the frame.
             * Hence, don't use sampling for the currently used rates.
             *
             * Also do not sample if the probability is already higher than 95%
             * to avoid wasting airtime.
             */
            const auto sampleRateInfo =
                station->m_groupsTable[sampleGroupId].m_ratesTable[sampleRateId];

            NS_LOG_DEBUG("Use sample rate? MaxTpRate= "
                         << station->m_maxTpRate << " CurrentRate= " << station->m_txrate
                         << " SampleRate= " << sampleIdx
                         << " SampleProb= " << sampleRateInfo.ewmaProb);

            if (sampleIdx != station->m_maxTpRate && sampleIdx != station->m_maxTpRate2 &&
                sampleIdx != station->m_maxProbRate && sampleRateInfo.ewmaProb <= 95)
            {
                /**
                 * Make sure that lower rates get sampled only occasionally,
                 * if the link is working perfectly.
                 */

                const auto maxTpGroupId = GetGroupId(station->m_maxTpRate);
                const auto maxTp2GroupId = GetGroupId(station->m_maxTpRate2);
                const auto maxTp2RateId = GetRateId(station->m_maxTpRate2);
                const auto maxProbGroupId = GetGroupId(station->m_maxProbRate);
                const auto maxProbRateId = GetRateId(station->m_maxProbRate);

                const auto maxTpStreams = m_minstrelGroups[maxTpGroupId].streams;
                const auto sampleStreams = m_minstrelGroups[sampleGroupId].streams;

                const auto sampleDuration = sampleRateInfo.perfectTxTime;
                const auto maxTp2Duration =
                    station->m_groupsTable[maxTp2GroupId].m_ratesTable[maxTp2RateId].perfectTxTime;
                const auto maxProbDuration = station->m_groupsTable[maxProbGroupId]
                                                 .m_ratesTable[maxProbRateId]
                                                 .perfectTxTime;

                NS_LOG_DEBUG("Use sample rate? SampleDuration= "
                             << sampleDuration << " maxTp2Duration= " << maxTp2Duration
                             << " maxProbDuration= " << maxProbDuration << " sampleStreams= "
                             << +sampleStreams << " maxTpStreams= " << +maxTpStreams);
                if (sampleDuration < maxTp2Duration ||
                    (sampleStreams < maxTpStreams && sampleDuration < maxProbDuration))
                {
                    /// Set flag that we are currently sampling.
                    station->m_isSampling = true;

                    /// set the rate that we're currently sampling
                    station->m_sampleRate = sampleIdx;

                    NS_LOG_DEBUG("FindRate sampleRate=" << sampleIdx);
                    station->m_sampleTries--;
                    return sampleIdx;
                }
                else
                {
                    station->m_numSamplesSlow++;
                    if (sampleRateInfo.numSamplesSkipped >= 20 && station->m_numSamplesSlow <= 2)
                    {
                        /// Set flag that we are currently sampling.
                        station->m_isSampling = true;

                        /// set the rate that we're currently sampling
                        station->m_sampleRate = sampleIdx;

                        NS_LOG_DEBUG("FindRate sampleRate=" << sampleIdx);
                        station->m_sampleTries--;
                        return sampleIdx;
                    }
                }
            }
        }
    }
    if (station->m_sampleWait > 0)
    {
        station->m_sampleWait--;
    }

    /// Continue using the best rate.

    NS_LOG_DEBUG("FindRate maxTpRrate=" << station->m_maxTpRate);
    return station->m_maxTpRate;
}

void
MinstrelHtWifiManager::UpdateStats(MinstrelHtWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);

    station->m_nextStatsUpdate = Simulator::Now() + m_updateStats;

    station->m_numSamplesSlow = 0;
    station->m_sampleCount = 0;

    double tempProb;

    if (station->m_ampduPacketCount > 0)
    {
        uint32_t newLen = station->m_ampduLen / station->m_ampduPacketCount;
        station->m_avgAmpduLen =
            (newLen * (100 - m_ewmaLevel) + (station->m_avgAmpduLen * m_ewmaLevel)) / 100;
        station->m_ampduLen = 0;
        station->m_ampduPacketCount = 0;
    }

    /* Initialize global rate indexes */
    station->m_maxTpRate = GetLowestIndex(station);
    station->m_maxTpRate2 = GetLowestIndex(station);
    station->m_maxProbRate = GetLowestIndex(station);

    /// Update throughput and EWMA for each rate inside each group.
    for (std::size_t j = 0; j < m_numGroups; j++)
    {
        if (station->m_groupsTable[j].m_supported)
        {
            station->m_sampleCount++;

            /* (re)Initialize group rate indexes */
            station->m_groupsTable[j].m_maxTpRate = GetLowestIndex(station, j);
            station->m_groupsTable[j].m_maxTpRate2 = GetLowestIndex(station, j);
            station->m_groupsTable[j].m_maxProbRate = GetLowestIndex(station, j);

            for (uint8_t i = 0; i < m_numRates; i++)
            {
                if (station->m_groupsTable[j].m_ratesTable[i].supported)
                {
                    station->m_groupsTable[j].m_ratesTable[i].retryUpdated = false;

                    NS_LOG_DEBUG(
                        +i << " "
                           << GetMcsSupported(station,
                                              station->m_groupsTable[j].m_ratesTable[i].mcsIndex)
                           << "\t attempt="
                           << station->m_groupsTable[j].m_ratesTable[i].numRateAttempt
                           << "\t success="
                           << station->m_groupsTable[j].m_ratesTable[i].numRateSuccess);

                    /// If we've attempted something.
                    if (station->m_groupsTable[j].m_ratesTable[i].numRateAttempt > 0)
                    {
                        station->m_groupsTable[j].m_ratesTable[i].numSamplesSkipped = 0;
                        /**
                         * Calculate the probability of success.
                         * Assume probability scales from 0 to 100.
                         */
                        tempProb =
                            (100 * station->m_groupsTable[j].m_ratesTable[i].numRateSuccess) /
                            station->m_groupsTable[j].m_ratesTable[i].numRateAttempt;

                        /// Bookkeeping.
                        station->m_groupsTable[j].m_ratesTable[i].prob = tempProb;

                        if (station->m_groupsTable[j].m_ratesTable[i].successHist == 0)
                        {
                            station->m_groupsTable[j].m_ratesTable[i].ewmaProb = tempProb;
                        }
                        else
                        {
                            station->m_groupsTable[j].m_ratesTable[i].ewmsdProb =
                                CalculateEwmsd(station->m_groupsTable[j].m_ratesTable[i].ewmsdProb,
                                               tempProb,
                                               station->m_groupsTable[j].m_ratesTable[i].ewmaProb,
                                               m_ewmaLevel);
                            /// EWMA probability
                            tempProb =
                                (tempProb * (100 - m_ewmaLevel) +
                                 station->m_groupsTable[j].m_ratesTable[i].ewmaProb * m_ewmaLevel) /
                                100;
                            station->m_groupsTable[j].m_ratesTable[i].ewmaProb = tempProb;
                        }

                        station->m_groupsTable[j].m_ratesTable[i].throughput =
                            CalculateThroughput(station, j, i, tempProb);

                        station->m_groupsTable[j].m_ratesTable[i].successHist +=
                            station->m_groupsTable[j].m_ratesTable[i].numRateSuccess;
                        station->m_groupsTable[j].m_ratesTable[i].attemptHist +=
                            station->m_groupsTable[j].m_ratesTable[i].numRateAttempt;
                    }
                    else
                    {
                        station->m_groupsTable[j].m_ratesTable[i].numSamplesSkipped++;
                    }

                    /// Bookkeeping.
                    station->m_groupsTable[j].m_ratesTable[i].prevNumRateSuccess =
                        station->m_groupsTable[j].m_ratesTable[i].numRateSuccess;
                    station->m_groupsTable[j].m_ratesTable[i].prevNumRateAttempt =
                        station->m_groupsTable[j].m_ratesTable[i].numRateAttempt;
                    station->m_groupsTable[j].m_ratesTable[i].numRateSuccess = 0;
                    station->m_groupsTable[j].m_ratesTable[i].numRateAttempt = 0;

                    if (station->m_groupsTable[j].m_ratesTable[i].throughput != 0)
                    {
                        SetBestStationThRates(station, GetIndex(j, i));
                        SetBestProbabilityRate(station, GetIndex(j, i));
                    }
                }
            }
        }
    }

    // Try to sample all available rates during each interval.
    station->m_sampleCount *= 8;

    // Recalculate retries for the rates selected.
    CalculateRetransmits(station, station->m_maxTpRate);
    CalculateRetransmits(station, station->m_maxTpRate2);
    CalculateRetransmits(station, station->m_maxProbRate);

    NS_LOG_DEBUG("max tp=" << station->m_maxTpRate << "\nmax tp2=" << station->m_maxTpRate2
                           << "\nmax prob=" << station->m_maxProbRate);
    if (m_printStats)
    {
        PrintTable(station);
    }
}

double
MinstrelHtWifiManager::CalculateThroughput(MinstrelHtWifiRemoteStation* station,
                                           std::size_t groupId,
                                           uint8_t rateId,
                                           double ewmaProb)
{
    /**
     * Calculating throughput.
     * Do not account throughput if probability of success is below 10%
     * (as done in minstrel_ht linux implementation).
     */
    if (ewmaProb < 10)
    {
        return 0;
    }
    else
    {
        /**
         * For the throughput calculation, limit the probability value to 90% to
         * account for collision related packet error rate fluctuation.
         */
        const auto txTime = station->m_groupsTable[groupId].m_ratesTable[rateId].perfectTxTime;
        if (ewmaProb > 90)
        {
            return 90 / txTime.GetSeconds();
        }
        else
        {
            return ewmaProb / txTime.GetSeconds();
        }
    }
}

void
MinstrelHtWifiManager::SetBestProbabilityRate(MinstrelHtWifiRemoteStation* station, uint16_t index)
{
    GroupInfo* group;
    MinstrelHtRateInfo rate;
    std::size_t tmpGroupId;
    uint8_t tmpRateId;
    double tmpTh;
    double tmpProb;
    std::size_t groupId;
    uint8_t rateId;
    double currentTh;
    // maximum group probability (GP) variables
    std::size_t maxGPGroupId;
    uint8_t maxGPRateId;
    double maxGPTh;

    groupId = GetGroupId(index);
    rateId = GetRateId(index);
    group = &station->m_groupsTable[groupId];
    rate = group->m_ratesTable[rateId];

    tmpGroupId = GetGroupId(station->m_maxProbRate);
    tmpRateId = GetRateId(station->m_maxProbRate);
    tmpProb = station->m_groupsTable[tmpGroupId].m_ratesTable[tmpRateId].ewmaProb;
    tmpTh = station->m_groupsTable[tmpGroupId].m_ratesTable[tmpRateId].throughput;

    if (rate.ewmaProb > 75)
    {
        currentTh = station->m_groupsTable[groupId].m_ratesTable[rateId].throughput;
        if (currentTh > tmpTh)
        {
            station->m_maxProbRate = index;
        }

        maxGPGroupId = GetGroupId(group->m_maxProbRate);
        maxGPRateId = GetRateId(group->m_maxProbRate);
        maxGPTh = station->m_groupsTable[maxGPGroupId].m_ratesTable[maxGPRateId].throughput;

        if (currentTh > maxGPTh)
        {
            group->m_maxProbRate = index;
        }
    }
    else
    {
        if (rate.ewmaProb > tmpProb)
        {
            station->m_maxProbRate = index;
        }
        maxGPRateId = GetRateId(group->m_maxProbRate);
        if (rate.ewmaProb > group->m_ratesTable[maxGPRateId].ewmaProb)
        {
            group->m_maxProbRate = index;
        }
    }
}

/*
 * Find & sort topmost throughput rates
 *
 * If multiple rates provide equal throughput the sorting is based on their
 * current success probability. Higher success probability is preferred among
 * MCS groups.
 */
void
MinstrelHtWifiManager::SetBestStationThRates(MinstrelHtWifiRemoteStation* station, uint16_t index)
{
    std::size_t groupId;
    uint8_t rateId;
    double th;
    double prob;
    std::size_t maxTpGroupId;
    uint8_t maxTpRateId;
    std::size_t maxTp2GroupId;
    uint8_t maxTp2RateId;
    double maxTpTh;
    double maxTpProb;
    double maxTp2Th;
    double maxTp2Prob;

    groupId = GetGroupId(index);
    rateId = GetRateId(index);
    prob = station->m_groupsTable[groupId].m_ratesTable[rateId].ewmaProb;
    th = station->m_groupsTable[groupId].m_ratesTable[rateId].throughput;

    maxTpGroupId = GetGroupId(station->m_maxTpRate);
    maxTpRateId = GetRateId(station->m_maxTpRate);
    maxTpProb = station->m_groupsTable[maxTpGroupId].m_ratesTable[maxTpRateId].ewmaProb;
    maxTpTh = station->m_groupsTable[maxTpGroupId].m_ratesTable[maxTpRateId].throughput;

    maxTp2GroupId = GetGroupId(station->m_maxTpRate2);
    maxTp2RateId = GetRateId(station->m_maxTpRate2);
    maxTp2Prob = station->m_groupsTable[maxTp2GroupId].m_ratesTable[maxTp2RateId].ewmaProb;
    maxTp2Th = station->m_groupsTable[maxTp2GroupId].m_ratesTable[maxTp2RateId].throughput;

    if (th > maxTpTh || (th == maxTpTh && prob > maxTpProb))
    {
        station->m_maxTpRate2 = station->m_maxTpRate;
        station->m_maxTpRate = index;
    }
    else if (th > maxTp2Th || (th == maxTp2Th && prob > maxTp2Prob))
    {
        station->m_maxTpRate2 = index;
    }

    // Find best rates per group

    GroupInfo* group = &station->m_groupsTable[groupId];
    maxTpGroupId = GetGroupId(group->m_maxTpRate);
    maxTpRateId = GetRateId(group->m_maxTpRate);
    maxTpProb = group->m_ratesTable[maxTpRateId].ewmaProb;
    maxTpTh = station->m_groupsTable[maxTpGroupId].m_ratesTable[maxTpRateId].throughput;

    maxTp2GroupId = GetGroupId(group->m_maxTpRate2);
    maxTp2RateId = GetRateId(group->m_maxTpRate2);
    maxTp2Prob = group->m_ratesTable[maxTp2RateId].ewmaProb;
    maxTp2Th = station->m_groupsTable[maxTp2GroupId].m_ratesTable[maxTp2RateId].throughput;

    if (th > maxTpTh || (th == maxTpTh && prob > maxTpProb))
    {
        group->m_maxTpRate2 = group->m_maxTpRate;
        group->m_maxTpRate = index;
    }
    else if (th > maxTp2Th || (th == maxTp2Th && prob > maxTp2Prob))
    {
        group->m_maxTpRate2 = index;
    }
}

void
MinstrelHtWifiManager::RateInit(MinstrelHtWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);

    station->m_groupsTable = McsGroupData(m_numGroups);

    /**
     * Initialize groups supported by the receiver.
     */
    NS_LOG_DEBUG("Supported groups by station:");
    bool noSupportedGroupFound = true;
    for (std::size_t groupId = 0; groupId < m_numGroups; groupId++)
    {
        if (m_minstrelGroups[groupId].isSupported)
        {
            station->m_groupsTable[groupId].m_supported = false;

            if ((m_minstrelGroups[groupId].type == WIFI_MINSTREL_GROUP_HE) &&
                !GetHeSupported(station))
            {
                // It is a HE group but the receiver does not support HE: skip
                continue;
            }
            if ((m_minstrelGroups[groupId].type == WIFI_MINSTREL_GROUP_VHT) &&
                !GetVhtSupported(station))
            {
                // It is a VHT group but the receiver does not support VHT: skip
                continue;
            }
            if ((m_minstrelGroups[groupId].type != WIFI_MINSTREL_GROUP_HE) &&
                GetHeSupported(station) && m_useLatestAmendmentOnly)
            {
                // It is not a HE group and the receiver supports HE: skip since
                // UseLatestAmendmentOnly attribute is enabled
                continue;
            }
            if (!GetHeSupported(station) &&
                (m_minstrelGroups[groupId].type != WIFI_MINSTREL_GROUP_VHT) &&
                GetVhtSupported(station) && m_useLatestAmendmentOnly)
            {
                // It is not a VHT group and the receiver supports VHT (but not HE): skip since
                // UseLatestAmendmentOnly attribute is enabled
                continue;
            }
            if ((m_minstrelGroups[groupId].type < WIFI_MINSTREL_GROUP_HE) &&
                (m_minstrelGroups[groupId].gi.GetNanoSeconds() == 400) &&
                !GetShortGuardIntervalSupported(station))
            {
                // It is a SGI group but the receiver does not support SGI: skip
                continue;
            }
            if ((m_minstrelGroups[groupId].type >= WIFI_MINSTREL_GROUP_HE) &&
                (m_minstrelGroups[groupId].gi < GetGuardInterval(station)))
            {
                // The receiver does not support the GI: skip
                continue;
            }
            if (GetChannelWidth(station) < m_minstrelGroups[groupId].chWidth)
            {
                // The receiver does not support the channel width: skip
                continue;
            }
            if (GetNumberOfSupportedStreams(station) < m_minstrelGroups[groupId].streams)
            {
                // The receiver does not support the number of spatial streams: skip
                continue;
            }

            NS_LOG_DEBUG("Group: " << groupId << " type: " << m_minstrelGroups[groupId].type
                                   << " streams: " << +m_minstrelGroups[groupId].streams
                                   << " GI: " << m_minstrelGroups[groupId].gi
                                   << " width: " << m_minstrelGroups[groupId].chWidth);

            noSupportedGroupFound = false;
            station->m_groupsTable[groupId].m_supported = true;
            station->m_groupsTable[groupId].m_col = 0;
            station->m_groupsTable[groupId].m_index = 0;

            station->m_groupsTable[groupId].m_ratesTable =
                MinstrelHtRate(m_numRates); /// Create the rate list for the group.
            for (uint8_t i = 0; i < m_numRates; i++)
            {
                station->m_groupsTable[groupId].m_ratesTable[i].supported = false;
            }

            // Initialize all modes supported by the remote station that belong to the current
            // group.
            for (uint8_t i = 0; i < station->m_nModes; i++)
            {
                if (const auto mode = GetMcsSupported(station, i);
                    ShouldAddMcsToGroup(mode, groupId))
                {
                    NS_LOG_DEBUG("Mode " << +i << ": " << mode);

                    /// Use the McsValue as the index in the rate table.
                    /// This way, MCSs not supported are not initialized.
                    auto rateId = mode.GetMcsValue();
                    if (mode.GetModulationClass() == WIFI_MOD_CLASS_HT)
                    {
                        rateId %= (minstrelHtStandardInfos.at(WIFI_MOD_CLASS_HT).maxMcs + 1);
                    }

                    station->m_groupsTable[groupId].m_ratesTable[rateId].supported = true;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].mcsIndex =
                        i; /// Mapping between rateId and operationalMcsSet
                    station->m_groupsTable[groupId].m_ratesTable[rateId].numRateAttempt = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].numRateSuccess = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].prob = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].ewmaProb = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].prevNumRateAttempt = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].prevNumRateSuccess = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].numSamplesSkipped = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].successHist = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].attemptHist = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].throughput = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].perfectTxTime =
                        GetFirstMpduTxTime(groupId, GetMcsSupported(station, i));
                    station->m_groupsTable[groupId].m_ratesTable[rateId].retryCount = 0;
                    station->m_groupsTable[groupId].m_ratesTable[rateId].adjustedRetryCount = 0;
                    CalculateRetransmits(station, groupId, rateId);
                }
            }
        }
    }
    /// make sure at least one group is supported, otherwise we end up with an infinite loop in
    /// SetNextSample
    if (noSupportedGroupFound)
    {
        NS_FATAL_ERROR("No supported group has been found");
    }
    SetNextSample(station);                /// Select the initial sample index.
    UpdateStats(station);                  /// Calculate the initial high throughput rates.
    station->m_txrate = FindRate(station); /// Select the rate to use.
}

void
MinstrelHtWifiManager::CalculateRetransmits(MinstrelHtWifiRemoteStation* station, uint16_t index)
{
    NS_LOG_FUNCTION(this << station << index);
    const auto groupId = GetGroupId(index);
    const auto rateId = GetRateId(index);
    if (!station->m_groupsTable[groupId].m_ratesTable[rateId].retryUpdated)
    {
        CalculateRetransmits(station, groupId, rateId);
    }
}

void
MinstrelHtWifiManager::CalculateRetransmits(MinstrelHtWifiRemoteStation* station,
                                            std::size_t groupId,
                                            uint8_t rateId)
{
    NS_LOG_FUNCTION(this << station << groupId << rateId);

    uint32_t cw = 15; // Is an approximation.
    uint32_t cwMax = 1023;
    Time cwTime;
    Time txTime;
    Time dataTxTime;
    const auto slotTime = GetPhy()->GetSlot();
    const auto ackTime = GetPhy()->GetSifs() + GetPhy()->GetBlockAckTxTime();

    if (station->m_groupsTable[groupId].m_ratesTable[rateId].ewmaProb < 1)
    {
        station->m_groupsTable[groupId].m_ratesTable[rateId].retryCount = 1;
    }
    else
    {
        station->m_groupsTable[groupId].m_ratesTable[rateId].retryCount = 2;
        station->m_groupsTable[groupId].m_ratesTable[rateId].retryUpdated = true;

        dataTxTime =
            GetFirstMpduTxTime(
                groupId,
                GetMcsSupported(station,
                                station->m_groupsTable[groupId].m_ratesTable[rateId].mcsIndex)) +
            GetMpduTxTime(
                groupId,
                GetMcsSupported(station,
                                station->m_groupsTable[groupId].m_ratesTable[rateId].mcsIndex)) *
                (station->m_avgAmpduLen - 1);

        /* Contention time for first 2 tries */
        cwTime = (cw / 2) * slotTime;
        cw = Min((cw + 1) * 2, cwMax);
        cwTime += (cw / 2) * slotTime;
        cw = Min((cw + 1) * 2, cwMax);

        /* Total TX time for data and Contention after first 2 tries */
        txTime = cwTime + 2 * (dataTxTime + ackTime);

        /* See how many more tries we can fit inside segment size */
        do
        {
            /* Contention time for this try */
            cwTime = (cw / 2) * slotTime;
            cw = Min((cw + 1) * 2, cwMax);

            /* Total TX time after this try */
            txTime += cwTime + ackTime + dataTxTime;
        } while ((txTime < MilliSeconds(6)) &&
                 (++station->m_groupsTable[groupId].m_ratesTable[rateId].retryCount < 7));
    }
}

double
MinstrelHtWifiManager::CalculateEwmsd(double oldEwmsd,
                                      double currentProb,
                                      double ewmaProb,
                                      double weight)
{
    double diff;
    double incr;
    double tmp;

    /* calculate exponential weighted moving variance */
    diff = currentProb - ewmaProb;
    incr = (100 - weight) * diff / 100;
    tmp = oldEwmsd * oldEwmsd;
    tmp = weight * (tmp + diff * incr) / 100;

    /* return standard deviation */
    return sqrt(tmp);
}

void
MinstrelHtWifiManager::InitSampleTable(MinstrelHtWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
    station->m_col = station->m_index = 0;

    // for off-setting to make rates fall between 0 and nModes
    auto numSampleRates = m_numRates;

    uint16_t newIndex;
    for (uint8_t col = 0; col < m_nSampleCol; col++)
    {
        for (uint8_t i = 0; i < numSampleRates; i++)
        {
            /**
             * The next two lines basically tries to generate a random number
             * between 0 and the number of available rates
             */
            int uv = m_uniformRandomVariable->GetInteger(0, numSampleRates);
            newIndex = (i + uv) % numSampleRates;

            // this loop is used for filling in other uninitialized places
            while (station->m_sampleTable[newIndex][col] != 0)
            {
                newIndex = (newIndex + 1) % m_numRates;
            }
            station->m_sampleTable[newIndex][col] = i;
        }
    }
}

void
MinstrelHtWifiManager::PrintTable(MinstrelHtWifiRemoteStation* station)
{
    if (!station->m_statsFile.is_open())
    {
        std::ostringstream tmp;
        tmp << "minstrel-ht-stats-" << station->m_state->m_address << ".txt";
        station->m_statsFile.open(tmp.str(), std::ios::out);
    }

    station->m_statsFile
        << "               best   ____________rate__________    ________statistics________    "
           "________last_______    ______sum-of________\n"
        << " mode guard #  rate  [name   idx airtime  max_tp]  [avg(tp) avg(prob) sd(prob)]  "
           "[prob.|retry|suc|att]  [#success | #attempts]\n";
    for (std::size_t i = 0; i < m_numGroups; i++)
    {
        StatsDump(station, i, station->m_statsFile);
    }

    station->m_statsFile << "\nTotal packet count::    ideal "
                         << Max(0, station->m_totalPacketsCount - station->m_samplePacketsCount)
                         << "              lookaround " << station->m_samplePacketsCount << "\n";
    station->m_statsFile << "Average # of aggregated frames per A-MPDU: " << station->m_avgAmpduLen
                         << "\n\n";

    station->m_statsFile.flush();
}

void
MinstrelHtWifiManager::StatsDump(MinstrelHtWifiRemoteStation* station,
                                 std::size_t groupId,
                                 std::ofstream& of)
{
    auto numRates = m_numRates;
    McsGroup group = m_minstrelGroups[groupId];
    Time txTime;
    for (uint8_t i = 0; i < numRates; i++)
    {
        if (station->m_groupsTable[groupId].m_supported &&
            station->m_groupsTable[groupId].m_ratesTable[i].supported)
        {
            of << group.type << " " << group.chWidth << "   " << group.gi << "  " << +group.streams
               << "   ";

            const auto maxTpRate = station->m_maxTpRate;
            const auto maxTpRate2 = station->m_maxTpRate2;
            const auto maxProbRate = station->m_maxProbRate;

            const auto idx = GetIndex(groupId, i);
            if (idx == maxTpRate)
            {
                of << 'A';
            }
            else
            {
                of << ' ';
            }
            if (idx == maxTpRate2)
            {
                of << 'B';
            }
            else
            {
                of << ' ';
            }
            if (idx == maxProbRate)
            {
                of << 'P';
            }
            else
            {
                of << ' ';
            }

            if (group.type == WIFI_MINSTREL_GROUP_HT)
            {
                of << std::setw(4) << "   MCS"
                   << (group.streams - 1) *
                              (minstrelHtStandardInfos.at(WIFI_MOD_CLASS_HT).maxMcs + 1) +
                          i;
            }
            else
            {
                of << std::setw(7) << "   MCS" << +i << "/" << static_cast<int>(group.streams);
            }

            of << "  " << std::setw(3) << idx << "  ";

            /* tx_time[rate(i)] in usec */
            txTime = GetFirstMpduTxTime(
                groupId,
                GetMcsSupported(station, station->m_groupsTable[groupId].m_ratesTable[i].mcsIndex));
            of << std::setw(6) << txTime.GetMicroSeconds() << "  ";

            of << std::setw(7) << CalculateThroughput(station, groupId, i, 100) / 100 << "   "
               << std::setw(7) << station->m_groupsTable[groupId].m_ratesTable[i].throughput / 100
               << "   " << std::setw(7) << station->m_groupsTable[groupId].m_ratesTable[i].ewmaProb
               << "  " << std::setw(7) << station->m_groupsTable[groupId].m_ratesTable[i].ewmsdProb
               << "  " << std::setw(7) << station->m_groupsTable[groupId].m_ratesTable[i].prob
               << "  " << std::setw(2) << station->m_groupsTable[groupId].m_ratesTable[i].retryCount
               << "   " << std::setw(3)
               << station->m_groupsTable[groupId].m_ratesTable[i].prevNumRateSuccess << "  "
               << std::setw(3) << station->m_groupsTable[groupId].m_ratesTable[i].prevNumRateAttempt
               << "   " << std::setw(9)
               << station->m_groupsTable[groupId].m_ratesTable[i].successHist << "   "
               << std::setw(9) << station->m_groupsTable[groupId].m_ratesTable[i].attemptHist
               << "\n";
        }
    }
}

uint16_t
MinstrelHtWifiManager::GetIndex(std::size_t groupId, uint8_t rateId)
{
    NS_LOG_FUNCTION(this << groupId << rateId);
    return groupId * m_numRates + rateId;
}

uint8_t
MinstrelHtWifiManager::GetRateId(uint16_t index)
{
    NS_LOG_FUNCTION(this << index);
    return index % m_numRates;
}

std::size_t
MinstrelHtWifiManager::GetGroupId(uint16_t index)
{
    NS_LOG_FUNCTION(this << index);
    return index / m_numRates;
}

std::size_t
MinstrelHtWifiManager::GetIdInGroup(WifiModulationClass mc,
                                    uint8_t streams,
                                    Time guardInterval,
                                    MHz_u chWidth)
{
    NS_LOG_FUNCTION(this << mc << streams << guardInterval << chWidth);
    const auto& standardInfos = minstrelHtStandardInfos.at(mc);
    const auto it = std::find(standardInfos.guardIntervals.cbegin(),
                              standardInfos.guardIntervals.cend(),
                              guardInterval);
    const auto giIndex = std::distance(standardInfos.guardIntervals.cbegin(), it);
    const auto widthIndex = std::log2(chWidth / MHz_u{20});
    return (standardInfos.maxStreams * standardInfos.guardIntervals.size() * widthIndex) +
           (standardInfos.maxStreams * giIndex) + streams - 1;
}

std::size_t
MinstrelHtWifiManager::GetNumGroups(WifiModulationClass mc)
{
    const auto& standardInfos = minstrelHtStandardInfos.at(mc);
    const auto numWidths = std::log2(standardInfos.maxWidth / MHz_u{20}) + 1;
    return numWidths * standardInfos.guardIntervals.size() * standardInfos.maxStreams;
}

std::size_t
MinstrelHtWifiManager::GetHtGroupId(uint8_t streams, Time guardInterval, MHz_u chWidth)
{
    NS_LOG_FUNCTION(this << streams << guardInterval << chWidth);
    return GetIdInGroup(WIFI_MOD_CLASS_HT, streams, guardInterval, chWidth);
}

std::size_t
MinstrelHtWifiManager::GetVhtGroupId(uint8_t streams, Time guardInterval, MHz_u chWidth)
{
    NS_LOG_FUNCTION(this << streams << guardInterval << chWidth);
    const auto allHtGroups = GetNumGroups(WIFI_MOD_CLASS_HT);
    const auto vhtGroupId = GetIdInGroup(WIFI_MOD_CLASS_VHT, streams, guardInterval, chWidth);
    return allHtGroups + vhtGroupId;
}

std::size_t
MinstrelHtWifiManager::GetHeGroupId(uint8_t streams, Time guardInterval, MHz_u chWidth)
{
    NS_LOG_FUNCTION(this << streams << guardInterval << chWidth);
    // This check is needed since HT is not supported in 6 GHz band
    std::size_t allHtGroups = (GetHtSupported()) ? GetNumGroups(WIFI_MOD_CLASS_HT) : 0;
    // This check is needed since VHT is not supported in 2.4 and 6 GHz bands
    std::size_t allVhtGroups = (GetVhtSupported()) ? GetNumGroups(WIFI_MOD_CLASS_VHT) : 0;
    const auto heGroupId = GetIdInGroup(WIFI_MOD_CLASS_HE, streams, guardInterval, chWidth);
    return allHtGroups + allVhtGroups + heGroupId;
}

std::size_t
MinstrelHtWifiManager::GetGroupIdForType(McsGroupType type,
                                         uint8_t streams,
                                         Time guardInterval,
                                         MHz_u chWidth)
{
    switch (type)
    {
    case WIFI_MINSTREL_GROUP_HT:
        return GetHtGroupId(streams, guardInterval, chWidth);
    case WIFI_MINSTREL_GROUP_VHT:
        return GetVhtGroupId(streams, guardInterval, chWidth);
    case WIFI_MINSTREL_GROUP_HE:
        return GetHeGroupId(streams, guardInterval, chWidth);
    default:
        NS_ABORT_MSG("Unknown group type: " << type);
    }
    return 0;
}

uint16_t
MinstrelHtWifiManager::GetLowestIndex(MinstrelHtWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);

    std::size_t groupId = 0;
    uint8_t rateId = 0;
    while (groupId < m_numGroups && !station->m_groupsTable[groupId].m_supported)
    {
        groupId++;
    }
    while (rateId < m_numRates && !station->m_groupsTable[groupId].m_ratesTable[rateId].supported)
    {
        rateId++;
    }
    NS_ASSERT(station->m_groupsTable[groupId].m_supported &&
              station->m_groupsTable[groupId].m_ratesTable[rateId].supported);
    return GetIndex(groupId, rateId);
}

uint16_t
MinstrelHtWifiManager::GetLowestIndex(MinstrelHtWifiRemoteStation* station, std::size_t groupId)
{
    NS_LOG_FUNCTION(this << station << groupId);

    uint8_t rateId = 0;
    while (rateId < m_numRates && !station->m_groupsTable[groupId].m_ratesTable[rateId].supported)
    {
        rateId++;
    }
    NS_ASSERT(station->m_groupsTable[groupId].m_supported &&
              station->m_groupsTable[groupId].m_ratesTable[rateId].supported);
    return GetIndex(groupId, rateId);
}

WifiModeList
MinstrelHtWifiManager::GetDeviceMcsList(WifiModulationClass mc) const
{
    const auto& phyMcsList = GetPhy()->GetMcsList(mc);
    WifiModeList mcsList(phyMcsList.cbegin(), phyMcsList.cend());
    return mcsList;
}

} // namespace ns3
