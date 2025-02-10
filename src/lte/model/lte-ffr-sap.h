/*
 * Copyright (c) 2014 Piotr Gawlowicz
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#ifndef LTE_FFR_SAP_H
#define LTE_FFR_SAP_H

#include "ff-mac-sched-sap.h"

#include <map>

namespace ns3
{

/**
 * @brief Service Access Point (SAP) offered by the Frequency Reuse algorithm
 *        instance to the MAC Scheduler instance.
 *
 * This is the *LteFfrSapProvider*, i.e., the part of the SAP
 * that contains the Frequency Reuse algorithm methods called by the MAC Scheduler
 * instance.
 */
class LteFfrSapProvider
{
  public:
    virtual ~LteFfrSapProvider();

    /**
     * @brief Get vector of available RBG in DL for this Cell
     * @return vector of size (m_dlBandwidth/RbgSize); false indicates
     *                   that RBG is free to use, true otherwise
     *
     * This function is called by MAC Scheduler in the beginning of DL
     * scheduling process. Frequency Reuse Algorithm based on its policy
     * generates vector of RBG which can be used and which can not be used
     * by Scheduler to schedule transmission.
     */
    virtual std::vector<bool> GetAvailableDlRbg() = 0;

    /**
     * @brief Check if UE can be served on i-th RB in DL
     * @param i RBG ID
     * @param rnti Radio Network Temporary Identity, an integer identifying the UE
     *             where the report originates from
     * @return true if UE can be served on i-th RB, false otherwise
     *
     * This function is called by MAC Scheduler during DL scheduling process
     * to check if UE is allowed to be served with i-th RBG. Frequency Reuse
     * Algorithm based on its policy decides if RBG is allowed to UE.
     * If yes, Scheduler will try to allocate this RBG for UE, if not this UE
     * will not be served with this RBG.
     */
    virtual bool IsDlRbgAvailableForUe(int i, uint16_t rnti) = 0;

    /**
     * @brief Get vector of available RB in UL for this Cell
     * @return vector of size m_ulBandwidth; false indicates
     *                    that RB is free to use, true otherwise
     *
     * This function is called by MAC Scheduler in the beginning of UL
     * scheduling process. Frequency Reuse Algorithm based on its policy
     * generates vector of RB which can be used and which can not be used
     * by Scheduler to schedule transmission.
     */
    virtual std::vector<bool> GetAvailableUlRbg() = 0;

    /**
     * @brief Check if UE can be served on i-th RB in UL
     * @param i RB ID
     * @param rnti Radio Network Temporary Identity, an integer identifying the UE
     *             where the report originates from
     * @return true if UE can be served on i-th RB, false otherwise
     *
     * This function is called by MAC Scheduler during UL scheduling process
     * to check if UE is allowed to be served with i-th RB. Frequency Reuse
     * Algorithm based on its policy decides if RB is allowed to UE.
     * If yes, Scheduler will try to allocate this RB for UE, if not this UE
     * will not be served with this RB.
     */
    virtual bool IsUlRbgAvailableForUe(int i, uint16_t rnti) = 0;

    /**
     * @brief ReportDlCqiInfo
     * @param params the struct FfMacSchedSapProvider::SchedDlCqiInfoReqParameters
     */
    virtual void ReportDlCqiInfo(
        const FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params) = 0;

    /**
     * @brief ReportUlCqiInfo
     * @param params the struct FfMacSchedSapProvider::SchedUlCqiInfoReqParameters
     */
    virtual void ReportUlCqiInfo(
        const FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params) = 0;

    /**
     * @brief ReportUlCqiInfo
     * @param ulCqiMap the UL CQI map
     */
    virtual void ReportUlCqiInfo(std::map<uint16_t, std::vector<double>> ulCqiMap) = 0;

    /**
     * @brief GetTpc
     * @param rnti the RNTI
     * @returns the TCP
     */
    virtual uint8_t GetTpc(uint16_t rnti) = 0;

    /**
     * @brief Get the minimum continuous Ul bandwidth
     * @returns the minimum continuous UL bandwidth
     */
    virtual uint16_t GetMinContinuousUlBandwidth() = 0;
}; // end of class LteFfrSapProvider

/**
 * @brief Service Access Point (SAP) offered by the eNodeB RRC instance to the
 *        Frequency Reuse algorithm instance.
 *
 * This is the *LteFfrSapUser*, i.e., the part of the SAP that
 * contains the MAC Scheduler methods called by the Frequency Reuse algorithm instance.
 */
class LteFfrSapUser
{
  public:
    virtual ~LteFfrSapUser();

}; // end of class LteFfrSapUser

/**
 * @brief Template for the implementation of the LteFfrSapProvider
 *        as a member of an owner class of type C to which all methods are
 *        forwarded.
 */
template <class C>
class MemberLteFfrSapProvider : public LteFfrSapProvider
{
  public:
    /**
     * Constructor
     *
     * @param owner the owner class
     */
    MemberLteFfrSapProvider(C* owner);

    // Delete default constructor to avoid misuse
    MemberLteFfrSapProvider() = delete;

    // inherited from LteFfrSapProvider
    std::vector<bool> GetAvailableDlRbg() override;
    bool IsDlRbgAvailableForUe(int i, uint16_t rnti) override;
    std::vector<bool> GetAvailableUlRbg() override;
    bool IsUlRbgAvailableForUe(int i, uint16_t rnti) override;
    void ReportDlCqiInfo(const FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params) override;
    void ReportUlCqiInfo(const FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params) override;
    void ReportUlCqiInfo(std::map<uint16_t, std::vector<double>> ulCqiMap) override;
    uint8_t GetTpc(uint16_t rnti) override;
    uint16_t GetMinContinuousUlBandwidth() override;

  private:
    C* m_owner; ///< the owner class

    // end of class MemberLteFfrSapProvider
};

template <class C>
MemberLteFfrSapProvider<C>::MemberLteFfrSapProvider(C* owner)
    : m_owner(owner)
{
}

template <class C>
std::vector<bool>
MemberLteFfrSapProvider<C>::GetAvailableDlRbg()
{
    return m_owner->DoGetAvailableDlRbg();
}

template <class C>
bool
MemberLteFfrSapProvider<C>::IsDlRbgAvailableForUe(int i, uint16_t rnti)
{
    return m_owner->DoIsDlRbgAvailableForUe(i, rnti);
}

template <class C>
std::vector<bool>
MemberLteFfrSapProvider<C>::GetAvailableUlRbg()
{
    return m_owner->DoGetAvailableUlRbg();
}

template <class C>
bool
MemberLteFfrSapProvider<C>::IsUlRbgAvailableForUe(int i, uint16_t rnti)
{
    return m_owner->DoIsUlRbgAvailableForUe(i, rnti);
}

template <class C>
void
MemberLteFfrSapProvider<C>::ReportDlCqiInfo(
    const FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params)
{
    m_owner->DoReportDlCqiInfo(params);
}

template <class C>
void
MemberLteFfrSapProvider<C>::ReportUlCqiInfo(
    const FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params)
{
    m_owner->DoReportUlCqiInfo(params);
}

template <class C>
void
MemberLteFfrSapProvider<C>::ReportUlCqiInfo(std::map<uint16_t, std::vector<double>> ulCqiMap)
{
    m_owner->DoReportUlCqiInfo(ulCqiMap);
}

template <class C>
uint8_t
MemberLteFfrSapProvider<C>::GetTpc(uint16_t rnti)
{
    return m_owner->DoGetTpc(rnti);
}

template <class C>
uint16_t
MemberLteFfrSapProvider<C>::GetMinContinuousUlBandwidth()
{
    return m_owner->DoGetMinContinuousUlBandwidth();
}

/**
 * @brief Template for the implementation of the LteFfrSapUser
 *        as a member of an owner class of type C to which all methods are
 *        forwarded.
 */
template <class C>
class MemberLteFfrSapUser : public LteFfrSapUser
{
  public:
    /**
     * Constructor
     *
     * @param owner the owner class
     */
    MemberLteFfrSapUser(C* owner);

    // Delete default constructor to avoid misuse
    MemberLteFfrSapUser() = delete;

  private:
    C* m_owner; ///< the owner class

    // end of class LteFfrSapUser
};

template <class C>
MemberLteFfrSapUser<C>::MemberLteFfrSapUser(C* owner)
    : m_owner(owner)
{
}

} // end of namespace ns3

#endif /* LTE_FFR_SAP_H */
