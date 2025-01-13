/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "eps-bearer.h"

#include "ns3/attribute-construction-list.h"
#include "ns3/fatal-error.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(EpsBearer);

GbrQosInformation::GbrQosInformation()
    : gbrDl(0),
      gbrUl(0),
      mbrDl(0),
      mbrUl(0)
{
}

AllocationRetentionPriority::AllocationRetentionPriority()
    : priorityLevel(0),
      preemptionCapability(false),
      preemptionVulnerability(false)
{
}

TypeId
EpsBearer::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::EpsBearer")
            .SetParent<ObjectBase>()
            .SetGroupName("Lte")
            .AddConstructor<EpsBearer>()
            .AddAttribute(
                "Release",
                "Change from 11 to 18 if you need bearer definition as per newer Releases."
                " Reference document: TS 23.203. The change does not impact other LTE code than "
                " bearers definition.",
                UintegerValue(11),
                MakeUintegerAccessor(&EpsBearer::GetRelease, &EpsBearer::SetRelease),
                MakeUintegerChecker<uint32_t>());
    return tid;
}

TypeId
EpsBearer::GetInstanceTypeId() const
{
    return EpsBearer::GetTypeId();
}

EpsBearer::EpsBearer()
    : ObjectBase(),
      qci(NGBR_VIDEO_TCP_DEFAULT)
{
    ObjectBase::ConstructSelf(AttributeConstructionList());
}

EpsBearer::EpsBearer(Qci x)
    : ObjectBase(),
      qci(x)
{
    ObjectBase::ConstructSelf(AttributeConstructionList());
}

EpsBearer::EpsBearer(Qci x, GbrQosInformation y)
    : ObjectBase(),
      qci(x),
      gbrQosInfo(y)
{
    ObjectBase::ConstructSelf(AttributeConstructionList());
}

EpsBearer::EpsBearer(const EpsBearer& o)
    : ObjectBase(o)
{
    qci = o.qci;
    gbrQosInfo = o.gbrQosInfo;
    ObjectBase::ConstructSelf(AttributeConstructionList());
}

void
EpsBearer::SetRelease(uint8_t release)
{
    switch (release)
    {
    case 8:
    case 9:
    case 10:
    case 11:
        m_requirements = GetRequirementsRel11();
        break;
    case 15:
        m_requirements = GetRequirementsRel15();
        break;
    case 18:
        m_requirements = GetRequirementsRel18();
        break;
    default:
        NS_FATAL_ERROR("Not recognized release " << static_cast<uint32_t>(release)
                                                 << " please choose a value between"
                                                    " 8 and 11, or 15 or 18");
    }
    m_release = release;
}

uint8_t
EpsBearer::GetResourceType() const
{
    return GetResourceType(m_requirements, qci);
}

uint8_t
EpsBearer::GetPriority() const
{
    return GetPriority(m_requirements, qci);
}

uint16_t
EpsBearer::GetPacketDelayBudgetMs() const
{
    return GetPacketDelayBudgetMs(m_requirements, qci);
}

double
EpsBearer::GetPacketErrorLossRate() const
{
    return GetPacketErrorLossRate(m_requirements, qci);
}

const EpsBearer::BearerRequirementsMap&
EpsBearer::GetRequirementsRel11()
{
    static EpsBearer::BearerRequirementsMap ret{
        {GBR_CONV_VOICE, {1, 2, 100, 1.0e-2, 0, 0}},
        {GBR_CONV_VIDEO, {1, 4, 150, 1.0e-3, 0, 0}},
        {GBR_GAMING, {1, 3, 50, 1.0e-3, 0, 0}},
        {GBR_NON_CONV_VIDEO, {1, 5, 300, 1.0e-6, 0, 0}},
        {NGBR_IMS, {0, 1, 100, 1.0e-6, 0, 0}},
        {NGBR_VIDEO_TCP_OPERATOR, {0, 6, 300, 1.0e-6, 0, 0}},
        {NGBR_VOICE_VIDEO_GAMING, {0, 7, 100, 1.0e-3, 0, 0}},
        {NGBR_VIDEO_TCP_PREMIUM, {0, 8, 300, 1.0e-6, 0, 0}},
        {NGBR_VIDEO_TCP_DEFAULT, {0, 9, 300, 1.0e-6, 0, 0}},
    };
    return ret;
}

const EpsBearer::BearerRequirementsMap&
EpsBearer::GetRequirementsRel15()
{
    static EpsBearer::BearerRequirementsMap ret{
        {GBR_CONV_VOICE, {1, 20, 100, 1.0e-2, 0, 2000}},
        {GBR_CONV_VIDEO, {1, 40, 150, 1.0e-3, 0, 2000}},
        {GBR_GAMING, {1, 30, 50, 1.0e-3, 0, 2000}},
        {GBR_NON_CONV_VIDEO, {1, 50, 300, 1.0e-6, 0, 2000}},
        {GBR_MC_PUSH_TO_TALK, {1, 7, 75, 1.0e-2, 0, 2000}},
        {GBR_NMC_PUSH_TO_TALK, {1, 20, 100, 1.0e-2, 0, 2000}},
        {GBR_MC_VIDEO, {1, 15, 100, 1.0e-3, 0, 2000}},
        {GBR_V2X, {1, 25, 50, 1.0e-2, 0, 2000}},
        {NGBR_IMS, {0, 10, 100, 1.0e-6, 0, 0}},
        {NGBR_VIDEO_TCP_OPERATOR, {0, 60, 300, 1.0e-6, 0, 0}},
        {NGBR_VOICE_VIDEO_GAMING, {0, 70, 100, 1.0e-3, 0, 0}},
        {NGBR_VIDEO_TCP_PREMIUM, {0, 80, 300, 1.0e-6, 0, 0}},
        {NGBR_VIDEO_TCP_DEFAULT, {0, 90, 300, 1.0e-6, 0, 0}},
        {NGBR_MC_DELAY_SIGNAL, {0, 5, 60, 1.0e-6, 0, 0}},
        {NGBR_MC_DATA, {0, 55, 200, 1.0e-6, 0, 0}},
        {NGBR_V2X, {0, 65, 5, 1.0e-2, 0, 0}},
        {NGBR_LOW_LAT_EMBB, {0, 68, 10, 1.0e-6, 0, 0}},
        {DGBR_DISCRETE_AUT_SMALL, {2, 19, 10, 1.0e-4, 255, 2000}},
        {DGBR_DISCRETE_AUT_LARGE, {2, 22, 10, 1.0e-4, 1358, 2000}},
        {DGBR_ITS, {2, 24, 30, 1.0e-5, 1354, 2000}},
        {DGBR_ELECTRICITY, {2, 21, 5, 1.0e-5, 255, 2000}},
    };
    return ret;
}

const EpsBearer::BearerRequirementsMap&
EpsBearer::GetRequirementsRel18()
{
    static EpsBearer::BearerRequirementsMap ret{
        {GBR_CONV_VOICE, {1, 20, 100, 1.0e-2, 0, 2000}},
        {GBR_CONV_VIDEO, {1, 40, 150, 1.0e-3, 0, 2000}},
        {GBR_GAMING, {1, 30, 50, 1.0e-3, 0, 2000}},
        {GBR_NON_CONV_VIDEO, {1, 50, 300, 1.0e-6, 0, 2000}},
        {GBR_MC_PUSH_TO_TALK, {1, 7, 75, 1.0e-2, 0, 2000}},
        {GBR_NMC_PUSH_TO_TALK, {1, 20, 100, 1.0e-2, 0, 2000}},
        {GBR_MC_VIDEO, {1, 15, 100, 1.0e-3, 0, 2000}},
        {GBR_V2X, {1, 25, 50, 1.0e-2, 0, 2000}},
        {NGBR_IMS, {0, 10, 100, 1.0e-6, 0, 0}},
        {NGBR_VIDEO_TCP_OPERATOR, {0, 60, 300, 1.0e-6, 0, 0}},
        {NGBR_VOICE_VIDEO_GAMING, {0, 70, 100, 1.0e-3, 0, 0}},
        {NGBR_VIDEO_TCP_PREMIUM, {0, 80, 300, 1.0e-6, 0, 0}},
        {NGBR_VIDEO_TCP_DEFAULT, {0, 90, 300, 1.0e-6, 0, 0}},
        {NGBR_MC_DELAY_SIGNAL, {0, 5, 60, 1.0e-6, 0, 0}},
        {NGBR_MC_DATA, {0, 55, 200, 1.0e-6, 0, 0}},
        {NGBR_V2X, {0, 65, 5, 1.0e-2, 0, 0}},
        {NGBR_LOW_LAT_EMBB, {0, 68, 10, 1.0e-6, 0, 0}},
        {GBR_LIVE_UL_71, {1, 56, 150, 1.0e-6, 0, 0}},
        {GBR_LIVE_UL_72, {1, 56, 300, 1.0e-4, 0, 0}},
        {GBR_LIVE_UL_73, {1, 56, 300, 1.0e-8, 0, 0}},
        {GBR_LIVE_UL_74, {1, 56, 500, 1.0e-8, 0, 0}},
        {GBR_LIVE_UL_76, {1, 56, 500, 1.0e-4, 0, 0}},
        {DGBR_DISCRETE_AUT_SMALL, {2, 19, 10, 1.0e-4, 255, 2000}},
        {DGBR_DISCRETE_AUT_LARGE, {2, 22, 10, 1.0e-4, 1358, 2000}},
        {DGBR_ITS, {2, 24, 30, 1.0e-5, 1354, 2000}},
        {DGBR_ELECTRICITY, {2, 21, 5, 1.0e-5, 255, 2000}},
        {DGBR_V2X, {2, 18, 5, 1.0e-4, 1354, 2000}},
        {DGBR_INTER_SERV_87, {2, 25, 5, 1.0e-3, 500, 2000}},
        {DGBR_INTER_SERV_88, {2, 25, 10, 1.0e-3, 1125, 2000}},
        {DGBR_VISUAL_CONTENT_89, {2, 25, 15, 1.0e-4, 17000, 2000}},
        {DGBR_VISUAL_CONTENT_90, {2, 25, 20, 1.0e-4, 63000, 2000}},
    };
    return ret;
}

} // namespace ns3
