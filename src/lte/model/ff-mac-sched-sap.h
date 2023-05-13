/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef FF_MAC_SCHED_SAP_H
#define FF_MAC_SCHED_SAP_H

#include "ff-mac-common.h"

#include <stdint.h>
#include <vector>

namespace ns3
{

/**
 * \ingroup ff-api
 * \brief Provides the SCHED SAP
 *
 * This abstract class defines the MAC Scheduler interface specified in the
 * Femto Forum Technical Document:
 *   - LTE MAC Scheduler Interface Specification v1.11
 *
 * The Technical Document contains a detailed description of the API.
 * The documentation of this class refers to sections of this Technical Document.
 *
 * You can found an example of the implementation of this interface
 * in the SampleFfMacSchedSapProvider and SampleFfMacSchedSapUser classes
 */
class FfMacSchedSapProvider
{
  public:
    virtual ~FfMacSchedSapProvider();

    /**
     * Parameters of the API primitives
     */

    /**
     * Parameters of the SCHED_DL_RLC_BUFFER_REQ primitive.
     * See section 4.2.1 for a detailed description of the parameters.
     */
    struct SchedDlRlcBufferReqParameters
    {
        uint16_t m_rnti;                         ///< RNTI
        uint8_t m_logicalChannelIdentity;        ///< logical channel identity
        uint32_t m_rlcTransmissionQueueSize;     ///< RLC transmission queue size
        uint16_t m_rlcTransmissionQueueHolDelay; ///< RLC transmission queue HOL delay
        uint32_t m_rlcRetransmissionQueueSize;   ///< RLC retransmission queue size
        uint16_t m_rlcRetransmissionHolDelay;    ///< RLC retransmission HOL delay
        uint16_t m_rlcStatusPduSize;             ///< RLC status PDU size

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_DL_PAGING_BUFFER_REQ primitive.
     * See section 4.2.2 for a detailed description of the parameters.
     */
    struct SchedDlPagingBufferReqParameters
    {
        uint16_t m_rnti;                                       ///< RNTI
        std::vector<PagingInfoListElement_s> m_pagingInfoList; ///< paging info list

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_DL_MAC_BUFFER_REQ primitive.
     * See section 4.2.3 for a detailed description of the parameters.
     */
    struct SchedDlMacBufferReqParameters
    {
        uint16_t m_rnti;       ///< RNTI
        CeBitmap_e m_ceBitmap; ///< CE bitmap

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_DL_TRIGGER_REQ primitive.
     * See section 4.2.4 for a detailed description of the parameters.
     */
    struct SchedDlTriggerReqParameters
    {
        uint16_t m_sfnSf;                              ///< sfn SF
        std::vector<DlInfoListElement_s> m_dlInfoList; ///< DL info list

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_DL_RACH_INFO_REQ primitive.
     * See section 4.2.5 for a detailed description of the parameters.
     */
    struct SchedDlRachInfoReqParameters
    {
        uint16_t m_sfnSf;                          ///< sfn SF
        std::vector<RachListElement_s> m_rachList; ///< RACH list

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_DL_CQI_INFO_REQ primitive.
     * See section 4.2.6 for a detailed description of the parameters.
     */
    struct SchedDlCqiInfoReqParameters
    {
        uint16_t m_sfnSf;                        ///< sfn SF
        std::vector<CqiListElement_s> m_cqiList; ///< CQI list

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_UL_TRIGGER_REQ primitive.
     * See section 4.2.8 for a detailed description of the parameters.
     */
    struct SchedUlTriggerReqParameters
    {
        uint16_t m_sfnSf;                              ///< sfn SF
        std::vector<UlInfoListElement_s> m_ulInfoList; ///< UL info list

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_UL_NOISE_INTERFERENCE_REQ primitive.
     * See section 4.2.9 for a detailed description of the parameters.
     */
    struct SchedUlNoiseInterferenceReqParameters
    {
        uint16_t m_sfnSf; ///< sfn SF
        uint16_t m_rip;   ///< RIP
        uint16_t m_tnp;   ///< TNP

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_UL_SR_INFO_REQ primitive.
     * See section 4.2.10 for a detailed description of the parameters.
     */
    struct SchedUlSrInfoReqParameters
    {
        uint16_t m_sfnSf;                      ///< sfn SF
        std::vector<SrListElement_s> m_srList; ///< SR list

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_UL_MAC_CTRL_INFO_REQ primitive.
     * See section 4.2.11 for a detailed description of the parameters.
     */
    struct SchedUlMacCtrlInfoReqParameters
    {
        uint16_t m_sfnSf;                            ///< sfn SF
        std::vector<MacCeListElement_s> m_macCeList; ///< MAC CE list

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_UL_CQI_INFO_REQ primitive.
     * See section 4.2.12 for a detailed description of the parameters.
     */
    struct SchedUlCqiInfoReqParameters
    {
        uint16_t m_sfnSf; ///< sfn SF
        UlCqi_s m_ulCqi;  ///< UL CQI

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    //
    // SCHED - MAC Scheduler SAP primitives
    // (See 4.2 for description of the primitives)
    //

    /**
     * \brief SCHED_DL_RLC_BUFFER_REQ
     *
     * \param params SchedDlRlcBufferReqParameters
     */
    virtual void SchedDlRlcBufferReq(const SchedDlRlcBufferReqParameters& params) = 0;

    /**
     * \brief SCHED_DL_PAGING_BUFFER_REQ
     *
     * \param params SchedDlPagingBufferReqParameters
     */
    virtual void SchedDlPagingBufferReq(const SchedDlPagingBufferReqParameters& params) = 0;

    /**
     * \brief SCHED_DL_MAC_BUFFER_REQ
     *
     * \param params SchedDlMacBufferReqParameters
     */
    virtual void SchedDlMacBufferReq(const SchedDlMacBufferReqParameters& params) = 0;

    /**
     * \brief SCHED_DL_TRIGGER_REQ
     *
     * \param params SchedDlTriggerReqParameters
     */
    virtual void SchedDlTriggerReq(const SchedDlTriggerReqParameters& params) = 0;

    /**
     * \brief SCHED_DL_RACH_INFO_REQ
     *
     * \param params SchedDlRachInfoReqParameters
     */
    virtual void SchedDlRachInfoReq(const SchedDlRachInfoReqParameters& params) = 0;

    /**
     * \brief SCHED_DL_CQI_INFO_REQ
     *
     * \param params SchedDlCqiInfoReqParameters
     */
    virtual void SchedDlCqiInfoReq(const SchedDlCqiInfoReqParameters& params) = 0;

    /**
     * \brief SCHED_UL_TRIGGER_REQ
     *
     * \param params SchedUlTriggerReqParameters
     */
    virtual void SchedUlTriggerReq(const SchedUlTriggerReqParameters& params) = 0;

    /**
     * \brief SCHED_UL_NOISE_INTERFERENCE_REQ
     *
     * \param params SchedUlNoiseInterferenceReqParameters
     */
    virtual void SchedUlNoiseInterferenceReq(
        const SchedUlNoiseInterferenceReqParameters& params) = 0;

    /**
     * \brief SCHED_UL_SR_INFO_REQ
     *
     * \param params SchedUlSrInfoReqParameters
     */
    virtual void SchedUlSrInfoReq(const SchedUlSrInfoReqParameters& params) = 0;

    /**
     * \brief SCHED_UL_MAC_CTRL_INFO_REQ
     *
     * \param params SchedUlMacCtrlInfoReqParameters
     */
    virtual void SchedUlMacCtrlInfoReq(const SchedUlMacCtrlInfoReqParameters& params) = 0;

    /**
     * \brief SCHED_UL_CQI_INFO_REQ
     *
     * \param params SchedUlCqiInfoReqParameters
     */
    virtual void SchedUlCqiInfoReq(const SchedUlCqiInfoReqParameters& params) = 0;

  private:
};

/// FfMacSchedSapUser class
class FfMacSchedSapUser
{
  public:
    virtual ~FfMacSchedSapUser();

    /**
     * Parameters of the API primitives
     */

    /**
     * Parameters of the SCHED_DL_CONFIG_IND primitive.
     * See section 4.2.7 for a detailed description of the parameters.
     */
    struct SchedDlConfigIndParameters
    {
        std::vector<BuildDataListElement_s> m_buildDataList;           ///< build data list
        std::vector<BuildRarListElement_s> m_buildRarList;             ///< build rar list
        std::vector<BuildBroadcastListElement_s> m_buildBroadcastList; ///< build broadcast list

        uint8_t m_nrOfPdcchOfdmSymbols; ///< number of PDCCH OFDM symbols

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    /**
     * Parameters of the SCHED_UL_CONFIG_IND primitive.
     * See section 4.2.13 for a detailed description of the parameters.
     */
    struct SchedUlConfigIndParameters
    {
        std::vector<UlDciListElement_s> m_dciList;   ///< DCI list
        std::vector<PhichListElement_s> m_phichList; ///< PHICH list

        std::vector<VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
    };

    //
    // SCHED - MAC Scheduler SAP primitives
    // (See 4.2 for description of the primitives)
    //

    /**
     * \brief SCHED_DL_CONFIG_IND
     *
     * \param params SchedDlConfigIndParameters
     */
    virtual void SchedDlConfigInd(const SchedDlConfigIndParameters& params) = 0;

    /**
     * \brief SCHED_UL_CONFIG_IND
     *
     * \param params SchedUlConfigIndParameters
     */
    virtual void SchedUlConfigInd(const SchedUlConfigIndParameters& params) = 0;

  private:
};

/// MemberSchedSapProvider class
template <class C>
class MemberSchedSapProvider : public FfMacSchedSapProvider
{
  public:
    /**
     * Constructor
     *
     * \param scheduler the scheduler class
     */
    MemberSchedSapProvider(C* scheduler);

    // Delete default constructor to avoid misuse
    MemberSchedSapProvider() = delete;

    // inherited from FfMacSchedSapProvider
    void SchedDlRlcBufferReq(const SchedDlRlcBufferReqParameters& params) override;
    void SchedDlPagingBufferReq(const SchedDlPagingBufferReqParameters& params) override;
    void SchedDlMacBufferReq(const SchedDlMacBufferReqParameters& params) override;
    void SchedDlTriggerReq(const SchedDlTriggerReqParameters& params) override;
    void SchedDlRachInfoReq(const SchedDlRachInfoReqParameters& params) override;
    void SchedDlCqiInfoReq(const SchedDlCqiInfoReqParameters& params) override;
    void SchedUlTriggerReq(const SchedUlTriggerReqParameters& params) override;
    void SchedUlNoiseInterferenceReq(const SchedUlNoiseInterferenceReqParameters& params) override;
    void SchedUlSrInfoReq(const SchedUlSrInfoReqParameters& params) override;
    void SchedUlMacCtrlInfoReq(const SchedUlMacCtrlInfoReqParameters& params) override;
    void SchedUlCqiInfoReq(const SchedUlCqiInfoReqParameters& params) override;

  private:
    C* m_scheduler; ///< the scheduler class
};

template <class C>
MemberSchedSapProvider<C>::MemberSchedSapProvider(C* scheduler)
    : m_scheduler(scheduler)
{
}

template <class C>
void
MemberSchedSapProvider<C>::SchedDlRlcBufferReq(const SchedDlRlcBufferReqParameters& params)
{
    m_scheduler->DoSchedDlRlcBufferReq(params);
}

template <class C>
void
MemberSchedSapProvider<C>::SchedDlPagingBufferReq(const SchedDlPagingBufferReqParameters& params)
{
    m_scheduler->DoSchedDlPagingBufferReq(params);
}

template <class C>
void
MemberSchedSapProvider<C>::SchedDlMacBufferReq(const SchedDlMacBufferReqParameters& params)
{
    m_scheduler->DoSchedDlMacBufferReq(params);
}

template <class C>
void
MemberSchedSapProvider<C>::SchedDlTriggerReq(const SchedDlTriggerReqParameters& params)
{
    m_scheduler->DoSchedDlTriggerReq(params);
}

template <class C>
void
MemberSchedSapProvider<C>::SchedDlRachInfoReq(const SchedDlRachInfoReqParameters& params)
{
    m_scheduler->DoSchedDlRachInfoReq(params);
}

template <class C>
void
MemberSchedSapProvider<C>::SchedDlCqiInfoReq(const SchedDlCqiInfoReqParameters& params)
{
    m_scheduler->DoSchedDlCqiInfoReq(params);
}

template <class C>
void
MemberSchedSapProvider<C>::SchedUlTriggerReq(const SchedUlTriggerReqParameters& params)
{
    m_scheduler->DoSchedUlTriggerReq(params);
}

template <class C>
void
MemberSchedSapProvider<C>::SchedUlNoiseInterferenceReq(
    const SchedUlNoiseInterferenceReqParameters& params)
{
    m_scheduler->DoSchedUlNoiseInterferenceReq(params);
}

template <class C>
void
MemberSchedSapProvider<C>::SchedUlSrInfoReq(const SchedUlSrInfoReqParameters& params)
{
    m_scheduler->DoSchedUlSrInfoReq(params);
}

template <class C>
void
MemberSchedSapProvider<C>::SchedUlMacCtrlInfoReq(const SchedUlMacCtrlInfoReqParameters& params)
{
    m_scheduler->DoSchedUlMacCtrlInfoReq(params);
}

template <class C>
void
MemberSchedSapProvider<C>::SchedUlCqiInfoReq(const SchedUlCqiInfoReqParameters& params)
{
    m_scheduler->DoSchedUlCqiInfoReq(params);
}

} // namespace ns3

#endif /* FF_MAC_SCHED_SAP_H */
