/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef EPS_BEARER
#define EPS_BEARER

#include "ns3/object-base.h"
#include "ns3/uinteger.h"

#include <unordered_map>

namespace ns3
{

/**
 * 3GPP TS 36.413 9.2.1.18 GBR QoS Information
 *
 */
struct GbrQosInformation
{
    /**
     * Default constructor, initializes member variables to zero or equivalent
     */
    GbrQosInformation();

    uint64_t gbrDl; /**< Guaranteed Bit Rate (bit/s) in downlink */
    uint64_t gbrUl; /**< Guaranteed Bit Rate (bit/s) in uplink */
    uint64_t mbrDl; /**< Maximum Bit Rate (bit/s) in downlink */
    uint64_t mbrUl; /**< Maximum Bit Rate (bit/s) in uplink */
};

/**
 * 3GPP 23.203 Section 6.1.7.3 Allocation and Retention Priority characteristics
 *
 */
struct AllocationRetentionPriority
{
    /**
     * Default constructor, initializes member variables to zero or equivalent
     */
    AllocationRetentionPriority();
    uint8_t priorityLevel;        ///< 1-15; 1 = highest
    bool preemptionCapability;    ///< true if bearer can preempt others
    bool preemptionVulnerability; ///< true if bearer can be preempted by others
};

/**
 * @brief This class contains the specification of EPS Bearers.
 *
 * See the following references:
 * 3GPP TS 23.401, Section 4.7.2 The EPS bearer
 * 3GPP TS 23.401, Section 4.7.3 Bearer level QoS parameters
 * 3GPP TS 36.413 Section 9.2.1.15 E-RAB Level QoS Parameters
 *
 * It supports the selection of different specifications depending on the
 * release. To change the release, change the attribute "Release". Please remember
 * that we must expose to all releases the most recent Qci. Asking for Qci parameters
 * for a release in which it has not been created will result in a crash.
 *
 * For example, if you select Release 11 (or if you don't select anything, as
 * it is the default selection) and then ask for the packet error rate of
 * the NGBR_MC_DELAY_SIGNAL Qci, the program will crash.
 *
 * Please note that from Release 8 (the latest when the LENA project finished)
 * to Release 11, the bearers ID and requirements are the same. From Release 12,
 * they started to change, and the latest version is now Release 18. However,
 * we do not support intermediate types between releases 11 and 15: in other words,
 * you can select from Release 8 to Release 11, or Release 15 or 18.
 * Any other value will result in a program crash.
 *
 * The release version only affect Bearer definitions. Other part of the LTE
 * module are not affected when changing the Release attribute.
 */
class EpsBearer : public ObjectBase
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;

    /**
     * QoS Class Indicator. See 3GPP 23.203 Section 6.1.7.2 for standard values.
     * Updated to Release 18.
     */
    enum Qci : uint8_t
    {
        GBR_CONV_VOICE = 1,        ///< GBR Conversational Voice
        GBR_CONV_VIDEO = 2,        ///< GBR Conversational Video (Live streaming)
        GBR_GAMING = 3,            ///< GBR Real Time Gaming
        GBR_NON_CONV_VIDEO = 4,    ///< GBR Non-Conversational Video (Buffered Streaming)
        GBR_MC_PUSH_TO_TALK = 65,  ///< GBR Mission Critical User Plane Push To Talk voice
        GBR_NMC_PUSH_TO_TALK = 66, ///< GBR Non-Mission-Critical User Plane Push To Talk voice
        GBR_MC_VIDEO = 67,         ///< GBR Mission Critical Video User Plane
        GBR_V2X = 75,              ///< GBR V2X Messages
        GBR_LIVE_UL_71 = 71,       ///< GBR Live UL streaming
        GBR_LIVE_UL_72 = 72,       ///< GBR Live UL streaming
        GBR_LIVE_UL_73 = 73,       ///< GBR Live UL streaming
        GBR_LIVE_UL_74 = 74,       ///< GBR Live UL streaming
        GBR_LIVE_UL_76 = 76,       ///< GBR Live UL streaming
        NGBR_IMS = 5,              ///< Non-GBR IMS Signalling
        NGBR_VIDEO_TCP_OPERATOR =
            6, ///< Non-GBR TCP-based Video (Buffered Streaming, e.g., www, e-mail...)
        NGBR_VOICE_VIDEO_GAMING = 7, ///< Non-GBR Voice, Video, Interactive Streaming
        NGBR_VIDEO_TCP_PREMIUM =
            8, ///< Non-GBR TCP-based Video (Buffered Streaming, e.g., www, e-mail...)
        NGBR_VIDEO_TCP_DEFAULT =
            9, ///< Non-GBR TCP-based Video (Buffered Streaming, e.g., www, e-mail...)
        NGBR_MC_DELAY_SIGNAL =
            69,            ///< Non-GBR Mission Critical Delay Sensitive Signalling (e.g., MC-PTT)
        NGBR_MC_DATA = 70, ///< Non-GBR Mission Critical Data
        NGBR_V2X = 79,     ///< Non-GBR V2X Messages
        NGBR_LOW_LAT_EMBB = 80, ///< Non-GBR Low Latency eMBB applications
        DGBR_DISCRETE_AUT_SMALL =
            82, ///< Delay-Critical GBR Discrete Automation Small Packets (TS 22.261)
        DGBR_DISCRETE_AUT_LARGE =
            83,        ///< Delay-Critical GBR Discrete Automation Large Packets (TS 22.261)
        DGBR_ITS = 84, ///< Delay-Critical GBR Intelligent Transport Systems (TS 22.261)
        DGBR_ELECTRICITY =
            85,        ///< Delay-Critical GBR Electricity Distribution High Voltage (TS 22.261)
        DGBR_V2X = 86, ///< Delay-Critical GBR V2X Messages (TS 23.501)
        DGBR_INTER_SERV_87 =
            87, ///< Delay-Critical GBR Interactive Service - Motion tracking data (TS 23.501)
        DGBR_INTER_SERV_88 =
            88, ///< Delay-Critical GBR Interactive Service - Motion tracking data (TS 23.501)
        DGBR_VISUAL_CONTENT_89 =
            89, ///< Delay-Critical GBR Visual Content for cloud/edge/split rendering (TS 23.501)
        DGBR_VISUAL_CONTENT_90 =
            90, ///< Delay-Critical GBR Visual Content for cloud/edge/split rendering (TS 23.501)
    };

    Qci qci; ///< Qos class indicator

    GbrQosInformation gbrQosInfo;    ///< GBR QOS information
    AllocationRetentionPriority arp; ///< allocation retention priority

    /**
     * Default constructor. QCI will be initialized to NGBR_VIDEO_TCP_DEFAULT
     *
     */
    EpsBearer();

    /**
     *
     * @param x the QoS Class Indicator
     *
     */
    EpsBearer(Qci x);

    /**
     *
     * @param x the QoS Class Indicator
     * @param y the GbrQosInformation
     *
     */
    EpsBearer(Qci x, GbrQosInformation y);

    /**
     * @brief EpsBearer copy constructor
     * @param o other instance
     */
    EpsBearer(const EpsBearer& o);

    /**
     * @brief Deconstructor
     */
    ~EpsBearer() override
    {
    }

    /**
     * @brief SetRelease
     * @param release The release the user want for this bearer
     *
     * Releases introduces new types, and change values for existing ones.
     * While we can't do much for the added type (we must expose them even
     * if the user want to work with older releases) by calling this method
     * we can, at least, select the specific parameters value the bearer returns.
     *
     * For instance, if the user select release 10 (the default) the priority
     * of CONV_VIDEO will be 2. With release 15, such priority will be 20.
     */
    void SetRelease(uint8_t release);

    /**
     * @brief GetRelease
     * @return The release currently set for this bearer type
     */
    uint8_t GetRelease() const
    {
        return m_release;
    }

    /**
     *
     * @return the resource type (NON-GBR, GBR, DC-GBR) of the selected QCI
     */
    uint8_t GetResourceType() const;

    /**
     *
     * @return the priority associated with the QCI of this bearer as per 3GPP 23.203
     * Section 6.1.7.2
     */
    uint8_t GetPriority() const;

    /**
     *
     *
     *
     * @return the packet delay budget associated with the QCI of this bearer as per 3GPP 23.203
     * Section 6.1.7.2
     */
    uint16_t GetPacketDelayBudgetMs() const;

    /**
     *
     *
     *
     * @return the packet error loss rate associated with the QCI of this bearer as per 3GPP 23.203
     * Section 6.1.7.2
     */
    double GetPacketErrorLossRate() const;

  private:
    /**
     * @brief Struct containing bearer requirements
     */
    struct BearerRequirements
    {
        uint8_t resourceType{0};         //!< resource type
        uint8_t priority{0};             //!< priority
        uint16_t packetDelayBudgetMs{0}; //!< packet delay budget in ms
        double packetErrorLossRate{0.0}; //!< packet error rate
        uint32_t maxDataBurst{0};        //!< default maximum data burst
        uint32_t avgWindow{0};           //!< default averaging window (0 when does not apply)
    };

    /**
     * @brief Map between QCI and requirements
     */
    using BearerRequirementsMap = std::unordered_map<Qci, BearerRequirements>;

    /**
     * @brief Get the resource type (NON-GBR, GBR, DC-GBR) of the selected QCI
     * @param map Map between QCI and requirements
     * @param qci QCI to look for
     * @return the resource type (NON-GBR, GBR, DC-GBR) of the selected QCI
     */
    static uint8_t GetResourceType(const BearerRequirementsMap& map, Qci qci)
    {
        return map.at(qci).resourceType;
    }

    /**
     * @brief Get priority for the selected QCI
     * @param map Map between QCI and requirements
     * @param qci QCI to look for
     * @return priority for the selected QCI
     */
    static uint8_t GetPriority(const BearerRequirementsMap& map, Qci qci)
    {
        return map.at(qci).priority;
    }

    /**
     * @brief Get packet delay in ms for the selected QCI
     * @param map Map between QCI and requirements
     * @param qci QCI to look for
     * @return packet delay in ms for the selected QCI
     */
    static uint16_t GetPacketDelayBudgetMs(const BearerRequirementsMap& map, Qci qci)
    {
        return map.at(qci).packetDelayBudgetMs;
    }

    /**
     * @brief Get packet error rate for the selected QCI
     * @param map Map between QCI and requirements
     * @param qci QCI to look for
     * @return packet error rate for the selected QCI
     */
    static double GetPacketErrorLossRate(const BearerRequirementsMap& map, Qci qci)
    {
        return map.at(qci).packetErrorLossRate;
    }

    /**
     * @brief Get maximum data burst for the selected QCI
     * @param map Map between QCI and requirements
     * @param qci QCI to look for
     * @return maximum data burst for the selected QCI
     */
    static uint32_t GetMaxDataBurst(const BearerRequirementsMap& map, Qci qci)
    {
        return map.at(qci).maxDataBurst;
    }

    /**
     * @brief Get default averaging window for the selected QCI
     * @param map Map between QCI and requirements
     * @param qci QCI to look for
     * @return default averaging window for the selected QCI
     */
    static uint32_t GetAvgWindow(const BearerRequirementsMap& map, Qci qci)
    {
        return map.at(qci).avgWindow;
    }

    /**
     * @brief Retrieve requirements for Rel. 11
     * @return the BearerRequirementsMap for Release 11
     */
    static const BearerRequirementsMap& GetRequirementsRel11();

    /**
     * @brief Retrieve requirements for Rel. 15
     * @return the BearerRequirementsMap for Release 15
     */
    static const BearerRequirementsMap& GetRequirementsRel15();

    /**
     * @brief Retrieve requirements for Rel. 18
     * @return the BearerRequirementsMap for Release 18
     */
    static const BearerRequirementsMap& GetRequirementsRel18();

    BearerRequirementsMap m_requirements; //!< Map of requirements per bearer

    uint8_t m_release{30}; //!< Release (8 to 11 or 15 or 18)
};

} // namespace ns3

#endif // EPS_BEARER
