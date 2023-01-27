/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef HE_FRAME_EXCHANGE_MANAGER_H
#define HE_FRAME_EXCHANGE_MANAGER_H

#include "mu-snr-tag.h"

#include "ns3/vht-frame-exchange-manager.h"

#include <map>
#include <unordered_map>

namespace ns3
{

class MultiUserScheduler;
class ApWifiMac;
class StaWifiMac;
class CtrlTriggerHeader;

/**
 * Map of PSDUs indexed by STA-ID
 */
typedef std::unordered_map<uint16_t /* staId */, Ptr<WifiPsdu> /* PSDU */> WifiPsduMap;
/**
 * Map of const PSDUs indexed by STA-ID
 */
typedef std::unordered_map<uint16_t /* staId */, Ptr<const WifiPsdu> /* PSDU */> WifiConstPsduMap;

/**
 * \param psduMap a PSDU map
 * \return true if the given PSDU map contains a single PSDU including a single MPDU
 *         that carries a Trigger Frame
 */
bool IsTrigger(const WifiPsduMap& psduMap);

/**
 * \ingroup wifi
 *
 * HeFrameExchangeManager handles the frame exchange sequences
 * for HE stations.
 */
class HeFrameExchangeManager : public VhtFrameExchangeManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    HeFrameExchangeManager();
    ~HeFrameExchangeManager() override;

    uint16_t GetSupportedBaBufferSize() const override;
    bool StartFrameExchange(Ptr<QosTxop> edca, Time availableTime, bool initialFrame) override;
    void SetWifiMac(const Ptr<WifiMac> mac) override;
    void SetWifiPhy(const Ptr<WifiPhy> phy) override;
    void CalculateAcknowledgmentTime(WifiAcknowledgment* acknowledgment) const override;
    void CalculateProtectionTime(WifiProtection* protection) const override;
    void SetTxopHolder(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) override;
    bool VirtualCsMediumIdle() const override;

    /**
     * Set the Multi-user Scheduler associated with this Frame Exchange Manager.
     *
     * \param muScheduler the Multi-user Scheduler associated with this Frame Exchange Manager
     */
    void SetMultiUserScheduler(const Ptr<MultiUserScheduler> muScheduler);

    /**
     * Get the PSDU in the given PSDU map that is addressed to the given MAC address,
     * if any, or a null pointer, otherwise.
     *
     * \param to the MAC address
     * \param psduMap the PSDU map
     * \return the PSDU, if any, or a null pointer, otherwise
     */
    static Ptr<WifiPsdu> GetPsduTo(Mac48Address to, const WifiPsduMap& psduMap);

    /**
     * Set the UL Target RSSI subfield of every User Info fields of the given
     * Trigger Frame to the most recent RSSI observed from the corresponding
     * station.
     *
     * \param trigger the given Trigger Frame
     */
    virtual void SetTargetRssi(CtrlTriggerHeader& trigger) const;

    /**
     * Get the RSSI (in dBm) of the most recent packet received from the station having
     * the given address.
     *
     * \param address of the remote station
     * \return the RSSI (in dBm) of the most recent packet received from the remote station
     */
    virtual std::optional<double> GetMostRecentRssi(const Mac48Address& address) const;

    /**
     * Return whether the received frame is classified as intra-BSS. It is assumed that
     * this station is already associated with an AP.
     *
     * \param psdu the received PSDU
     * \param txVector TX vector of the received PSDU
     * \return true if the received frame is classified as intra-BSS, false otherwise
     *         (the received frame is classified as inter-BSS or it cannot be classified
     *         as intra-BSS or inter-BSS)
     */
    bool IsIntraBssPpdu(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) const;

    /**
     * This method is intended to be called a SIFS after the reception of a Trigger Frame
     * to determine whether the station is allowed to respond.
     *
     * \param trigger the Trigger Frame soliciting a response
     * \return true if CS is not required or the UL MU CS mechanism indicates that the medium
     *         is idle, false otherwise
     */
    bool UlMuCsMediumIdle(const CtrlTriggerHeader& trigger) const;

  protected:
    void DoDispose() override;
    void Reset() override;

    void ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                     RxSignalInfo rxSignalInfo,
                     const WifiTxVector& txVector,
                     bool inAmpdu) override;
    void EndReceiveAmpdu(Ptr<const WifiPsdu> psdu,
                         const RxSignalInfo& rxSignalInfo,
                         const WifiTxVector& txVector,
                         const std::vector<bool>& perMpduStatus) override;
    void PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) override;
    Time GetTxDuration(uint32_t ppduPayloadSize,
                       Mac48Address receiver,
                       const WifiTxParameters& txParams) const override;
    bool SendMpduFromBaManager(Ptr<WifiMpdu> mpdu, Time availableTime, bool initialFrame) override;
    void NormalAckTimeout(Ptr<WifiMpdu> mpdu, const WifiTxVector& txVector) override;
    void BlockAckTimeout(Ptr<WifiPsdu> psdu, const WifiTxVector& txVector) override;
    void CtsTimeout(Ptr<WifiMpdu> rts, const WifiTxVector& txVector) override;
    void UpdateNav(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) override;
    void NavResetTimeout() override;

    /**
     * Clear the TXOP holder if the intra-BSS NAV counted down to zero (includes the case
     * of intra-BSS NAV reset).
     */
    void ClearTxopHolderIfNeeded() override;

    /**
     * Reset the intra-BSS NAV upon expiration of the intra-BSS NAV reset timer.
     */
    virtual void IntraBssNavResetTimeout();

    /**
     * Compute how to set the Duration/ID field of an MU-RTS Trigger Frame to send to protect
     * a frame transmitted with the given TX vector.
     *
     * \param muRtsSize the size of the MU-RTS Trigger Frame in bytes
     * \param muRtsTxVector the TX vector used to send the MU-RTS Trigger Frame
     * \param txDuration the TX duration of the data frame
     * \param response the time taken by the response (acknowledgment) to the data frame
     * \return the computed Duration/ID value for the MU-RTS Trigger Frame
     */
    virtual Time GetMuRtsDurationId(uint32_t muRtsSize,
                                    const WifiTxVector& muRtsTxVector,
                                    Time txDuration,
                                    Time response) const;

    /**
     * Send an MU-RTS to begin an MU-RTS/CTS frame exchange protecting an MU PPDU.
     *
     * \param txParams the TX parameters for the data frame
     */
    void SendMuRts(const WifiTxParameters& txParams);

    /**
     * Called when no CTS frame is received after an MU-RTS.
     *
     * \param muRts the MU-RTS that solicited CTS responses
     * \param txVector the TXVECTOR used to transmit the MU-RTS frame
     */
    virtual void CtsAfterMuRtsTimeout(Ptr<WifiMpdu> muRts, const WifiTxVector& txVector);

    /**
     * Send CTS after receiving an MU-RTS.
     *
     * \param muRtsHdr the MAC header of the received MU-RTS
     * \param trigger the MU-RTS Trigger Frame header
     * \param muRtsSnr the SNR of the MU-RTS in linear scale
     */
    void SendCtsAfterMuRts(const WifiMacHeader& muRtsHdr,
                           const CtrlTriggerHeader& trigger,
                           double muRtsSnr);

    /**
     * \return the mode used to transmit a CTS after an MU-RTS.
     */
    WifiMode GetCtsModeAfterMuRts() const;

    /**
     * Get the TXVECTOR that the station having the given station ID has to use to send a
     * CTS frame after receiving an MU-RTS Trigger Frame from the AP it is associated with.
     *
     * \param trigger the MU-RTS Trigger Frame
     * \param staId the station ID for MU
     * \return the TXVECTOR to use to send a CTS frame
     */
    WifiTxVector GetCtsTxVectorAfterMuRts(const CtrlTriggerHeader& trigger, uint16_t staId) const;

    /**
     * Send a map of PSDUs as a DL MU PPDU.
     * Note that both <i>psduMap</i> and <i>txParams</i> are moved to m_psduMap and
     * m_txParams, respectively, and hence are left in an undefined state.
     *
     * \param psduMap the map of PSDUs to send
     * \param txParams the TX parameters to use to transmit the PSDUs
     */
    void SendPsduMapWithProtection(WifiPsduMap psduMap, WifiTxParameters& txParams);

    /**
     * Forward a map of PSDUs down to the PHY layer.
     *
     * \param psduMap the map of PSDUs to transmit
     * \param txVector the TXVECTOR used to transmit the MU PPDU
     */
    void ForwardPsduMapDown(WifiConstPsduMap psduMap, WifiTxVector& txVector);

    /**
     * Take the necessary actions after that some BlockAck frames are missing
     * in response to a DL MU PPDU. This method must not be called if all the
     * expected BlockAck frames were received.
     *
     * \param psduMap a pointer to PSDU map transmitted in a DL MU PPDU
     * \param staMissedBlockAckFrom set of stations we missed a BlockAck frame from
     * \param nSolicitedStations the number of stations solicited to send a TB PPDU
     */
    virtual void BlockAcksInTbPpduTimeout(WifiPsduMap* psduMap,
                                          const std::set<Mac48Address>* staMissedBlockAckFrom,
                                          std::size_t nSolicitedStations);

    /**
     * Take the necessary actions after that some TB PPDUs are missing in
     * response to Trigger Frame. This method must not be called if all the
     * expected TB PPDUs were received.
     *
     * \param psduMap a pointer to PSDU map transmitted in a DL MU PPDU
     * \param staMissedTbPpduFrom set of stations we missed a TB PPDU from
     * \param nSolicitedStations the number of stations solicited to send a TB PPDU
     */
    virtual void TbPpduTimeout(WifiPsduMap* psduMap,
                               const std::set<Mac48Address>* staMissedTbPpduFrom,
                               std::size_t nSolicitedStations);

    /**
     * Take the necessary actions after that a Block Ack is missing after a
     * TB PPDU solicited through a Trigger Frame.
     *
     * \param psdu the PSDU in the TB PPDU
     * \param txVector the TXVECTOR used to transmit the TB PPDU
     */
    virtual void BlockAckAfterTbPpduTimeout(Ptr<WifiPsdu> psdu, const WifiTxVector& txVector);

    /**
     * Get the TRIGVECTOR that the MAC has to pass to the PHY when transmitting
     * the given Trigger Frame.
     *
     * \param trigger the given Trigger Frame
     * \return the TRIGVECTOR
     */
    WifiTxVector GetTrigVector(const CtrlTriggerHeader& trigger) const;

    /**
     * Return a TXVECTOR for the UL frame that the station will send in response to
     * the given Trigger frame, configured with the BSS color and transmit power
     * level to use for the consequent HE TB PPDU.
     * Note that this method should only be called by non-AP stations only.
     *
     * \param trigger the received Trigger frame
     * \param triggerSender the MAC address of the AP sending the Trigger frame
     * \return TXVECTOR for the HE TB PPDU frame
     */
    WifiTxVector GetHeTbTxVector(CtrlTriggerHeader trigger, Mac48Address triggerSender) const;

    /**
     * Build a MU-BAR Trigger Frame starting from the TXVECTOR used to respond to
     * the MU-BAR (in case of multiple responders, their TXVECTORs need to be
     * "merged" into a single TXVECTOR) and from the BlockAckReq headers for
     * every recipient.
     * Note that the number of recipients must match the number of users addressed
     * by the given TXVECTOR.
     *
     * \param responseTxVector the given TXVECTOR
     * \param recipients the list of BlockAckReq headers indexed by the station's AID
     * \return the MPDU containing the built MU-BAR
     */
    Ptr<WifiMpdu> PrepareMuBar(const WifiTxVector& responseTxVector,
                               std::map<uint16_t, CtrlBAckRequestHeader> recipients) const;

    /**
     * Send a Multi-STA Block Ack frame after the reception of some TB PPDUs.
     *
     * \param txParams the TX parameters for the Trigger Frame that solicited the TB PPDUs
     * \param durationId the Duration/ID field of the HE TB PPDU that elicited the Multi-STA
     *                   Block Ack response
     */
    void SendMultiStaBlockAck(const WifiTxParameters& txParams, Time durationId);

    /**
     * Send QoS Null frames in response to a Basic or BSRP Trigger Frame. The number
     * of QoS Null frames that are actually aggregated depends on the available time
     * as indicated by the Trigger Frame and is at most 8 (one QoS Null frame per TID).
     *
     * \param trigger the Basic or BSRP Trigger Frame content
     * \param hdr the MAC header of the Basic or BSRP Trigger Frame
     */
    void SendQosNullFramesInTbPpdu(const CtrlTriggerHeader& trigger, const WifiMacHeader& hdr);

    Ptr<ApWifiMac> m_apMac;          //!< MAC pointer (null if not an AP)
    Ptr<StaWifiMac> m_staMac;        //!< MAC pointer (null if not a STA)
    WifiTxVector m_trigVector;       //!< the TRIGVECTOR
    Time m_intraBssNavEnd;           //!< intra-BSS NAV expiration time
    EventId m_intraBssNavResetEvent; //!< the event to reset the intra-BSS NAV after an RTS

  private:
    /**
     * Send the current PSDU map as a DL MU PPDU.
     */
    void SendPsduMap();

    /**
     * Take the necessary actions when receiving a Basic Trigger Frame.
     *
     * \param trigger the Basic Trigger Frame content
     * \param hdr the MAC header of the Basic Trigger Frame
     */
    void ReceiveBasicTrigger(const CtrlTriggerHeader& trigger, const WifiMacHeader& hdr);

    /**
     * Respond to a MU-BAR Trigger Frame (if permitted by UL MU CS mechanism).
     *
     * \param trigger the Basic Trigger Frame content
     * \param tid the TID requested for us in the MU-BAR Trigger Frame
     * \param durationId the Duration/ID field of the MPDU carrying the Trigger Frame
     * \param snr the receive SNR
     */
    void ReceiveMuBarTrigger(const CtrlTriggerHeader& trigger,
                             uint8_t tid,
                             Time durationId,
                             double snr);

    WifiPsduMap m_psduMap;                        //!< the A-MPDU being transmitted
    WifiTxParameters m_txParams;                  //!< the TX parameters for the current PPDU
    Ptr<MultiUserScheduler> m_muScheduler;        //!< Multi-user Scheduler (HE APs only)
    Ptr<WifiMpdu> m_triggerFrame;                 //!< Trigger Frame being sent
    std::set<Mac48Address> m_staExpectTbPpduFrom; //!< set of stations expected to send a TB PPDU
    EventId m_multiStaBaEvent;                    //!< Sending a Multi-STA BlockAck event
    MuSnrTag m_muSnrTag;                          //!< Tag to attach to Multi-STA BlockAck frames
    bool m_triggerFrameInAmpdu; //!< True if the received A-MPDU contains an MU-BAR
};

} // namespace ns3

#endif /* HE_FRAME_EXCHANGE_MANAGER_H */
