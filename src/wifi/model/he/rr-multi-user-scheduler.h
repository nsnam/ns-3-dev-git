/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef RR_MULTI_USER_SCHEDULER_H
#define RR_MULTI_USER_SCHEDULER_H

#include "multi-user-scheduler.h"

#include <functional>
#include <list>

namespace ns3
{

/**
 * @ingroup wifi
 *
 * RrMultiUserScheduler is a simple OFDMA scheduler that indicates to perform a DL OFDMA
 * transmission if the AP has frames to transmit to at least one station.
 * RrMultiUserScheduler assigns RUs of equal size (in terms of tones) to stations to
 * which the AP has frames to transmit belonging to the AC who gained access to the
 * channel or higher. The maximum number of stations that can be granted an RU is
 * configurable. Associated stations are served based on their priority. The priority is
 * determined by the credits/debits a station gets when it is selected or not for transmission.
 *
 * @todo Take the supported channel width of the stations into account while selecting
 * stations and assigning RUs to them.
 */
class RrMultiUserScheduler : public MultiUserScheduler
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    RrMultiUserScheduler();
    ~RrMultiUserScheduler() override;

    Time GetExtraTimeForBsrpTfDurationId(uint8_t linkId) const override;

  protected:
    void DoDispose() override;
    void DoInitialize() override;

    void UpdateTriggerFrameAfterProtection(uint8_t linkId,
                                           CtrlTriggerHeader& trigger,
                                           WifiTxParameters& txParams) const override;
    void UpdateDlMuAfterProtection(uint8_t linkId,
                                   WifiPsduMap& psduMap,
                                   WifiTxParameters& txParams) const override;

    /**
     * Information used to sort stations
     */
    struct MasterInfo
    {
        uint16_t aid;         //!< station's AID
        Mac48Address address; //!< station's MAC Address
        double credits;       //!< credits accumulated by the station
    };

    /**
     * Determine whether the given STA can be solicited via a Basic Trigger Frame.
     *
     * @param info the information about the given STA
     * @return whether the given STA can be solicited via a Basic Trigger Frame
     */
    virtual bool CanSolicitStaInBasicTf(const MasterInfo& info) const;

    /**
     * Determine whether the given STA can be solicited via a BSRP Trigger Frame.
     *
     * @param info the information about the given STA
     * @return whether the given STA can be solicited via a BSRP Trigger Frame
     */
    virtual bool CanSolicitStaInBsrpTf(const MasterInfo& info) const;

  private:
    TxFormat SelectTxFormat() override;
    DlMuInfo ComputeDlMuInfo() override;
    UlMuInfo ComputeUlMuInfo() override;

    /**
     * Check if it is possible to send a BSRP Trigger Frame given the current
     * time limits.
     *
     * @return UL_MU_TX if it is possible to send a BSRP TF, NO_TX otherwise
     */
    virtual TxFormat TrySendingBsrpTf();

    /**
     * Check if it is possible to send a Basic Trigger Frame given the current
     * time limits.
     *
     * @return UL_MU_TX if it is possible to send a Basic TF, DL_MU_TX if we can try
     *         to send a DL MU PPDU and NO_TX if the remaining time is too short
     */
    virtual TxFormat TrySendingBasicTf();

    /**
     * Check if it is possible to send a DL MU PPDU given the current
     * time limits.
     *
     * @return DL_MU_TX if it is possible to send a DL MU PPDU, SU_TX if a SU PPDU
     *         can be transmitted (e.g., there are no HE stations associated or sending
     *         a DL MU PPDU is not possible and m_forceDlOfdma is false) or NO_TX otherwise
     */
    virtual TxFormat TrySendingDlMuPpdu();

    /**
     * Compute a TXVECTOR that can be used to construct a Trigger Frame to solicit
     * transmissions from suitable stations, i.e., stations that have established a
     * BlockAck agreement with the AP and for which the given predicate returns true.
     *
     * @param canBeSolicited a predicate returning false for stations that shall not be solicited
     * @return a TXVECTOR that can be used to construct a Trigger Frame to solicit
     *         transmissions from suitable stations
     */
    virtual WifiTxVector GetTxVectorForUlMu(std::function<bool(const MasterInfo&)> canBeSolicited);

    /**
     * Notify the scheduler that a station associated with the AP
     *
     * @param aid the AID of the station
     * @param address the MAC address of the station
     */
    void NotifyStationAssociated(uint16_t aid, Mac48Address address);
    /**
     * Notify the scheduler that a station deassociated with the AP
     *
     * @param aid the AID of the station
     * @param address the MAC address of the station
     */
    void NotifyStationDeassociated(uint16_t aid, Mac48Address address);

    /**
     * Finalize the given TXVECTOR by only including the largest subset of the
     * current set of candidate stations that can be allocated equal-sized RUs
     * (with the possible exception of using central 26-tone RUs) without
     * leaving RUs unallocated. The given TXVECTOR must be a MU TXVECTOR and must
     * contain an HeMuUserInfo entry for each candidate station. The finalized
     * TXVECTOR contains a subset of such HeMuUserInfo entries. The set of candidate
     * stations is also updated by removing stations that are not allocated an RU.
     *
     * @param txVector the given TXVECTOR
     */
    void FinalizeTxVector(WifiTxVector& txVector);
    /**
     * Update credits of the stations in the given list considering that a PPDU having
     * the given duration is being transmitted or solicited by using the given TXVECTOR.
     *
     * @param staList the list of stations
     * @param txDuration the TX duration of the PPDU being transmitted or solicited
     * @param txVector the TXVECTOR for the PPDU being transmitted or solicited
     */
    void UpdateCredits(std::list<MasterInfo>& staList,
                       Time txDuration,
                       const WifiTxVector& txVector);

    /**
     * Information stored for candidate stations
     */
    typedef std::pair<std::list<MasterInfo>::iterator, Ptr<WifiMpdu>> CandidateInfo;

    uint8_t m_nStations;         //!< Number of stations/slots to fill
    bool m_enableTxopSharing;    //!< allow A-MPDUs of different TIDs in a DL MU PPDU
    bool m_forceDlOfdma;         //!< return DL_OFDMA even if no DL MU PPDU was built
    bool m_enableUlOfdma;        //!< enable the scheduler to also return UL_OFDMA
    bool m_enableBsrp;           //!< send a BSRP before an UL MU transmission
    bool m_useCentral26TonesRus; //!< whether to allocate central 26-tone RUs
    uint32_t m_ulPsduSize;       //!< the size in byte of the solicited PSDU
    std::map<AcIndex, std::list<MasterInfo>>
        m_staListDl;                       //!< Per-AC list of stations (next to serve for DL first)
    std::list<MasterInfo> m_staListUl;     //!< List of stations to serve for UL
    std::list<CandidateInfo> m_candidates; //!< Candidate stations for MU TX
    Time m_maxCredits;                     //!< Max amount of credits a station can have
    CtrlTriggerHeader m_trigger;           //!< Trigger Frame to send
    WifiMacHeader m_triggerMacHdr;         //!< MAC header for Trigger Frame
    Time m_triggerTxDuration{0};           //!< Trigger Frame TX duration
    WifiTxParameters m_txParams;           //!< TX parameters
};

} // namespace ns3

#endif /* RR_MULTI_USER_SCHEDULER_H */
