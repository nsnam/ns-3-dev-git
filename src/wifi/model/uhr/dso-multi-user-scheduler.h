/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef DSO_CAPABLE_MULTI_USER_SCHEDULER_H
#define DSO_CAPABLE_MULTI_USER_SCHEDULER_H

#include "ns3/rr-multi-user-scheduler.h"

#include <functional>
#include <list>
#include <optional>

namespace ns3
{

/**
 * @ingroup wifi
 *
 * DsoMultiUserScheduler is a lightweight version of RrMultiUserScheduler that supports DSO
 * (dynamic subband operation).
 *
 * Once the scheduler is called for the initial frame of a TXOP, it will attempt to send a DSO ICF
 * (initial control frame) using a BSRP trigger frame to solicit the DSO stations (DSO STAs) to
 * eventually switch to the DSO subchannel where the assigned RUs are located. Once the DSO TXOP is
 * started, the scheduler will keep using the same assigned RUs for the whole duration of the TXOP.
 * It will alternate between sending DL MU transmissions (if there is at least one packet enqueued
 * for one of the DSO STA with an allocated RU in the TXOP) and soliciting UL MU transmissions from
 * the DSO STAs. In order to ensure that a DSO STA with no DL packets enqueued can still participate
 * in the UL MU solicitations, the scheduler will send a QoS Null frame to the DSO STA in the DL MU
 * PPDU.
 *
 * The current implementation presents the following limitations:
 * - it is assumed that only DSO STAs are associated with the AP;
 * - it is assumed that all DSO STAs have the same channel width;
 * - the scheduler is only able to allocate equal-sized RUs (without using central 26-tone RUs);
 */
class DsoMultiUserScheduler : public RrMultiUserScheduler
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    DsoMultiUserScheduler();
    ~DsoMultiUserScheduler() override;

  protected:
    void NotifyStationAssociated(uint16_t aid, Mac48Address address) override;
    void NotifyStationDeassociated(uint16_t aid, Mac48Address address) override;
    TxFormat TrySendingDlMuPpdu() override;
    WifiTxVector GetTxVectorForUlMu(std::function<bool(const MasterInfo&)> canBeSolicited) override;
    bool CanSolicitStaInBsrpTf(const MasterInfo& info, WifiDirection direction) const override;
    void UpdateTriggerFrameAfterProtection(uint8_t linkId,
                                           CtrlTriggerHeader& trigger,
                                           WifiTxParameters& txParams) const override;
    void UpdateDlMuAfterProtection(uint8_t linkId,
                                   WifiPsduMap& psduMap,
                                   WifiTxParameters& txParams) const override;

    /**
     * Allow the base class to select the transmission format for the next transmission.
     *
     * @return the selected transmission format, or std::nullopt if no transmission format is
     * selected
     */
    virtual std::optional<TxFormat> DoSelectTxFormat();

  private:
    TxFormat SelectTxFormat() override;

    /**
     * Check if it is possible to send a BSRP Trigger Frame as a DSO ICF given the current time
     * limits.
     * @param direction the direction (DL/UL) of the following frame exchange
     *
     * @return UL_MU_TX if it is possible to send a BSRP TF that acts as a DSO ICF
     */
    TxFormat TrySendingDsoIcf(WifiDirection direction);

    /**
     * Determine whether the given STA can be solicited in this TXOP.
     *
     * @param info the information about the given STA
     * @return whether the given STA can be solicited in this TXOP
     */
    bool CanSolicitStaInTxop(const MasterInfo& info) const;

    /**
     * Compute a TXVECTOR that can be used to construct a DSO ICF prior to a DL MU transmission or a
     * TXVECTOR for a DL MU transmission.
     *
     * @param canBeSolicited a predicate returning false for DSO STAs that shall not be solicited by
     * the ICF
     * @return a TXVECTOR that can be used to construct a DSO ICF prior to a DL MU transmission or a
     * TXVECTOR for a DL MU transmission
     */
    WifiTxVector GetTxVectorForDlMu(std::function<bool(const MasterInfo&)> canBeSolicited);

    /**
     * Finalize the given TXVECTOR by only including the largest subset of the
     * current set of candidate stations that can be allocated equal-sized RUs
     * (without using central 26-tone RUs). The given TXVECTOR must be a MU TXVECTOR
     * and must contain an HeMuUserInfo entry for each candidate station. The finalized
     * TXVECTOR contains a subset of such HeMuUserInfo entries. The set of candidate
     * stations is also updated by removing stations that are not allocated an RU.
     *
     * @param txVector the given TXVECTOR
     */
    void FinalizeTxVector(WifiTxVector& txVector);

    bool m_dsoTxop{false};                           //!< whether the current TXOP is a DSO TXOP
    WifiTxVector::HeMuUserInfoMap m_dsoRuAllocation; //!< RU allocation for current DSO TXOP
};

} // namespace ns3

#endif /* DSO_CAPABLE_MULTI_USER_SCHEDULER_H */
