/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef EHT_FRAME_EXCHANGE_MANAGER_H
#define EHT_FRAME_EXCHANGE_MANAGER_H

#include "ns3/he-frame-exchange-manager.h"
#include "ns3/mgt-headers.h"

#include <unordered_map>

namespace ns3
{

/// aRxPHYStartDelay value to use when waiting for a new frame in the context of EMLSR operations
/// (Sec. 35.3.17 of 802.11be D3.1)
extern const Time EMLSR_RX_PHY_START_DELAY;

class MgtEmlOmn;

/**
 * @ingroup wifi
 * Reasons for an EMLSR client to drop an ICF
 */
enum class WifiIcfDrop : uint8_t
{
    USING_OTHER_LINK = 0,   // another EMLSR link is being used
    NOT_ENOUGH_TIME_TX,     // not enough time for the main PHY to switch (because in TX state)
    NOT_ENOUGH_TIME_RX,     // not enough time for the main PHY to switch (because in RX state)
    NOT_ENOUGH_TIME_SWITCH, // not enough time for the main PHY to switch (already switching)
    NOT_ENOUGH_TIME_SLEEP,  // not enough time for the main PHY to switch (because in SLEEP state)
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param reason the reason for dropping the ICF
 * @returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, WifiIcfDrop reason)
{
    switch (reason)
    {
    case WifiIcfDrop::USING_OTHER_LINK:
        return (os << "USING_OTHER_LINK");
    case WifiIcfDrop::NOT_ENOUGH_TIME_TX:
        return (os << "NOT_ENOUGH_TIME_TX");
    case WifiIcfDrop::NOT_ENOUGH_TIME_RX:
        return (os << "NOT_ENOUGH_TIME_RX");
    case WifiIcfDrop::NOT_ENOUGH_TIME_SWITCH:
        return (os << "NOT_ENOUGH_TIME_SWITCH");
    case WifiIcfDrop::NOT_ENOUGH_TIME_SLEEP:
        return (os << "NOT_ENOUGH_TIME_SLEEP");
    default:
        NS_FATAL_ERROR("Unknown wifi ICF drop reason");
        return (os << "UNKNOWN");
    }
}

/**
 * @ingroup wifi
 *
 * EhtFrameExchangeManager handles the frame exchange sequences
 * for EHT stations.
 */
class EhtFrameExchangeManager : public HeFrameExchangeManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    EhtFrameExchangeManager();
    ~EhtFrameExchangeManager() override;

    void SetLinkId(uint8_t linkId) override;
    Ptr<WifiMpdu> CreateAliasIfNeeded(Ptr<WifiMpdu> mpdu) const override;
    bool StartTransmission(Ptr<Txop> edca, MHz_u allowedWidth) override;

    /**
     * Send an EML Operating Mode Notification frame to the given station.
     *
     * @param dest the MAC address of the receiver
     * @param frame the EML Operating Mode Notification frame to send
     */
    void SendEmlOmn(const Mac48Address& dest, const MgtEmlOmn& frame);

    /**
     * Get the RSSI of the most recent packet received from the station having the given address. If
     * there is no such information for the given station and the station is affiliated with an MLD,
     * return the RSSI of the most recent packet received from another station of the same MLD.
     *
     * @param address of the remote station
     * @return the RSSI of the most recent packet received from the remote station
     */
    std::optional<dBm_u> GetMostRecentRssi(const Mac48Address& address) const override;

    /**
     * @param psdu the given PSDU
     * @param aid the AID of an EMLSR client
     * @param address the link MAC address of an EMLSR client
     * @return whether the EMLSR client having the given AID and MAC address shall switch back to
     *         the listening operation when receiving the given PSDU
     */
    bool GetEmlsrSwitchToListening(Ptr<const WifiPsdu> psdu,
                                   uint16_t aid,
                                   const Mac48Address& address) const;

    /**
     * Notify that the given PHY will switch channel to operate on another EMLSR link
     * after the given delay.
     *
     * @param phy the given PHY
     * @param linkId the ID of the EMLSR link on which the given PHY is operating
     * @param delay the delay after which the channel switch will be completed
     */
    void NotifySwitchingEmlsrLink(Ptr<WifiPhy> phy, uint8_t linkId, Time delay);

    /**
     * @return whether this is an EMLSR link that has been blocked because another EMLSR link
     *         is being used
     */
    bool UsingOtherEmlsrLink() const;

    /**
     * Check if the frame received (or being received) is sent by an EMLSR client to start an
     * UL TXOP. If so, take the appropriate actions (e.g., block transmission to the EMLSR client
     * on the other links). This method is intended to be called when an MPDU (possibly within
     * an A-MPDU) is received or when the reception of the MAC header in an MPDU is notified.
     *
     * @param hdr the MAC header of the received (or being received) MPDU
     * @param txVector the TXVECTOR used to transmit the frame received (or being received)
     * @return whether the frame received (or being received) is sent by an EMLSR client to start
     *         an UL TXOP
     */
    bool CheckEmlsrClientStartingTxop(const WifiMacHeader& hdr, const WifiTxVector& txVector);

    /**
     * This method is intended to be called when an AP MLD detects that an EMLSR client previously
     * involved in the current TXOP will start waiting for the transition delay interval (to switch
     * back to listening operation) after the given delay.
     * This method blocks the transmissions on all the EMLSR links of the given EMLSR client until
     * the transition delay advertised by the EMLSR client expires.
     *
     * @param address the link MAC address of the given EMLSR client
     * @param delay the given delay
     */
    void EmlsrSwitchToListening(Mac48Address address, const Time& delay);

    /**
     * @return a reference to the event indicating the possible end of the current TXOP (of
     *         which this device is not the holder)
     */
    EventId& GetOngoingTxopEndEvent();

    /**
     * Set the padding and the TXVECTOR of the given Trigger Frame, in case it is an Initial
     * Control Frame for some EMLSR client(s).
     *
     * @param trigger the given Trigger Frame
     * @param txVector the TXVECTOR used to transmit the Trigger Frame
     */
    void SetIcfPaddingAndTxVector(CtrlTriggerHeader& trigger, WifiTxVector& txVector) const;

    /// ICF drop reason traced callback (WifiMac exposes this trace source)
    TracedCallback<WifiIcfDrop, uint8_t> m_icfDropCallback;

  protected:
    void DoDispose() override;
    void RxStartIndication(WifiTxVector txVector, Time psduDuration) override;
    void ForwardPsduDown(Ptr<const WifiPsdu> psdu, WifiTxVector& txVector) override;
    void ForwardPsduMapDown(WifiConstPsduMap psduMap, WifiTxVector& txVector) override;
    void CtsAfterMuRtsTimeout(Ptr<WifiMpdu> muRts, const WifiTxVector& txVector) override;
    bool GetUpdateCwOnCtsTimeout() const override;
    void SendCtsAfterMuRts(const WifiMacHeader& muRtsHdr,
                           const CtrlTriggerHeader& trigger,
                           double muRtsSnr) override;
    void TransmissionSucceeded() override;
    void TransmissionFailed(bool forceCurrentCw = false) override;
    void NotifyChannelReleased(Ptr<Txop> txop) override;
    void PreProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) override;
    void PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) override;
    void ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                     RxSignalInfo rxSignalInfo,
                     const WifiTxVector& txVector,
                     bool inAmpdu) override;
    void EndReceiveAmpdu(Ptr<const WifiPsdu> psdu,
                         const RxSignalInfo& rxSignalInfo,
                         const WifiTxVector& txVector,
                         const std::vector<bool>& perMpduStatus) override;
    void NavResetTimeout() override;
    void IntraBssNavResetTimeout() override;
    void SendCtsAfterRts(const WifiMacHeader& rtsHdr, WifiMode rtsTxMode, double rtsSnr) override;
    void PsduRxError(Ptr<const WifiPsdu> psdu) override;
    void ReceivedQosNullAfterBsrpTf(Mac48Address sender) override;
    void SendQosNullFramesInTbPpdu(const CtrlTriggerHeader& trigger,
                                   const WifiMacHeader& hdr) override;
    void TbPpduTimeout(WifiPsduMap* psduMap, std::size_t nSolicitedStations) override;
    void BlockAcksInTbPpduTimeout(WifiPsduMap* psduMap, std::size_t nSolicitedStations) override;
    void ProtectionCompleted() override;

    /**
     * @return whether this is an EMLSR client that cannot respond to an ICF received a SIFS before
     */
    bool EmlsrClientCannotRespondToIcf() const;

    /**
     * Check if the MPDU that has been received on this link shall be dropped. In our model,
     * an aux PHY, or the main PHY that is not involved in any TXOP, can receive:
     * - management frames
     * - CTS
     * - CF-End
     * - broadcast data frames
     * Note that this method does not attempt to detect if the given MPDU is an ICF (this is done
     * by ReceiveMpdu).
     *
     * @param mpdu the MPDU that has been received
     * @return whether the given MPDU shall be dropped
     */
    bool ShallDropReceivedMpdu(Ptr<const WifiMpdu> mpdu) const;

    /**
     * Check whether all the stations that did not respond (to a certain frame) are EMLSR clients
     * trying to start an UL TXOP on another link.
     *
     * @param staMissedResponseFrom stations that did not respond
     * @return whether all the stations that did not respond are EMLSR clients trying to start an
     *         UL TXOP on another link
     */
    bool IsCrossLinkCollision(const std::set<Mac48Address>& staMissedResponseFrom) const;

    /**
     * Unblock transmissions on all the links of the given EMLSR client, provided that the latter
     * is not involved in any DL or UL TXOP on another link.
     *
     * @param address the link MAC address of the given EMLSR client
     * @return whether transmissions could be unblocked
     */
    bool UnblockEmlsrLinksIfAllowed(Mac48Address address);

  private:
    /**
     * @return whether the received ICF must be dropped because we are unable to process it
     *         (e.g., another EMLSR link is being used or there is no time for main PHY switch)
     */
    bool DropReceivedIcf();

    /**
     * For each EMLSR client in the given set of clients that did not respond to a frame requesting
     * a response from multiple clients, have the client switch to listening or simply unblock
     * links depending on whether the EMLSR client was protected or not.
     *
     * @param clients the given set of clients
     */
    void SwitchToListeningOrUnblockLinks(const std::set<Mac48Address>& clients);

    /**
     * Generate an in-device interference of the given power on the given link for the given
     * duration.
     *
     * @param linkId the ID of the link on which in-device interference is generated
     * @param duration the duration of the in-device interference
     * @param txPower the TX power
     */
    void GenerateInDeviceInterference(uint8_t linkId, Time duration, Watt_u txPower);

    /**
     * Update the TXOP end timer when starting a frame transmission.
     *
     * @param txDuration the TX duration of the frame being transmitted
     * @param durationId the Duration/ID value carried by the frame being transmitted
     */
    void UpdateTxopEndOnTxStart(Time txDuration, Time durationId);

    /**
     * Update the TXOP end timer when receiving a PHY-RXSTART.indication.
     *
     * @param psduDuration the TX duration of the PSDU being received
     */
    void UpdateTxopEndOnRxStartIndication(Time psduDuration);

    /**
     * Update the TXOP end timer when a frame reception ends.
     *
     * @param durationId the Duration/ID value carried by the received frame
     */
    void UpdateTxopEndOnRxEnd(Time durationId);

    /**
     * Take actions when a TXOP (of which we are not the holder) ends.
     *
     * @param txopHolder the holder of the TXOP (if any)
     */
    void TxopEnd(const std::optional<Mac48Address>& txopHolder);

    bool m_icfReceived{false}; //!< whether an ICF has been received and needs to be notified to
                               //!< the EMLSR manager after post-processing the frame
    EventId m_ongoingTxopEnd;  //!< event indicating the possible end of the current TXOP (of which
                               //!< we are not the holder)
    std::unordered_map<Mac48Address, EventId, WifiAddressHash>
        m_transDelayTimer; //!< MLD address-indexed map of transition delay timers
};

} // namespace ns3

#endif /* EHT_FRAME_EXCHANGE_MANAGER_H */
