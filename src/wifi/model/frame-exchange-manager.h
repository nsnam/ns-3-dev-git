/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef FRAME_EXCHANGE_MANAGER_H
#define FRAME_EXCHANGE_MANAGER_H

#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "qos-txop.h"
#include "wifi-mac.h"
#include "wifi-phy.h"
#include "wifi-psdu.h"
#include "wifi-tx-parameters.h"
#include "wifi-tx-timer.h"
#include "wifi-tx-vector.h"

// Needed to compile wave bindings
#include "channel-access-manager.h"
#include "wifi-ack-manager.h"
#include "wifi-protection-manager.h"

#include "ns3/object.h"

#include <functional>
#include <optional>

#define WIFI_FEM_NS_LOG_APPEND_CONTEXT                                                             \
    std::clog << "[link=" << +m_linkId << "][mac=" << m_self << "] "

namespace ns3
{

class ApWifiMac;
class StaWifiMac;

struct RxSignalInfo;
struct WifiProtection;
struct WifiAcknowledgment;

/**
 * @ingroup wifi
 *
 * FrameExchangeManager is a base class handling the basic frame exchange
 * sequences for non-QoS stations.
 *
 * The fragmentation policy implemented uses a simple fragmentation
 * threshold: any packet bigger than this threshold is fragmented
 * in fragments whose size is smaller than the threshold.
 *
 * The retransmission policy is also very simple: every packet is
 * retransmitted until it is either successfully transmitted or
 * it has been retransmitted up until the SSRC or SLRC thresholds.
 */
class FrameExchangeManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    FrameExchangeManager();
    ~FrameExchangeManager() override;

    /**
     * typedef for a callback to invoke when an MPDU is dropped.
     */
    typedef Callback<void, WifiMacDropReason, Ptr<const WifiMpdu>> DroppedMpdu;
    /**
     * typedef for a callback to invoke when an MPDU is successfully acknowledged.
     */
    typedef Callback<void, Ptr<const WifiMpdu>> AckedMpdu;

    /**
     * Request the FrameExchangeManager to start a frame exchange sequence.
     *
     * @param dcf the channel access function that gained channel access. It is
     *            the DCF on non-QoS stations and an EDCA on QoS stations.
     * @param allowedWidth the allowed width for the frame exchange sequence
     * @return true if a frame exchange sequence was started, false otherwise
     */
    virtual bool StartTransmission(Ptr<Txop> dcf, MHz_u allowedWidth);

    /**
     * This method is intended to be called by the PHY layer every time an MPDU
     * is received and also when the reception of an A-MPDU is completed. In case
     * the PSDU contains multiple MPDUs, the <i>perMpduStatus</i> vector is empty
     * when receiving the individual MPDUs.
     *
     * @param psdu the received PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector TxVector of the received PSDU
     * @param perMpduStatus per MPDU reception status
     */
    void Receive(Ptr<const WifiPsdu> psdu,
                 RxSignalInfo rxSignalInfo,
                 const WifiTxVector& txVector,
                 const std::vector<bool>& perMpduStatus);

    /**
     * Information about the MPDU being received. The TXVECTOR is populated upon
     * PHY-RXSTART indication; the MAC header is populated when notified by the PHY.
     */
    struct OngoingRxInfo
    {
        std::optional<WifiMacHeader> macHdr; //!< MAC header of the MPDU being received
        WifiTxVector txVector;               //!< TXVECTOR of the MPDU being received
        Time endOfPsduRx;                    //!< time when reception of PSDU ends
    };

    /**
     * @return the information about the MPDU being received by the PHY, if any. This information
     *         is available from the time the PHY-RXSTART.indication is received until the end
     *         of PSDU reception
     */
    std::optional<std::reference_wrapper<const OngoingRxInfo>> GetOngoingRxInfo() const;

    /**
     * @return the information about the MAC header of the MPDU being received by the PHY, if any.
     *         The MAC header is available from the time its reception is completed until the end
     *         of PSDU reception
     */
    std::optional<std::reference_wrapper<const WifiMacHeader>> GetReceivedMacHdr() const;

    /**
     * Set the ID of the link this Frame Exchange Manager is associated with.
     *
     * @param linkId the ID of the link this Frame Exchange Manager is associated with
     */
    virtual void SetLinkId(uint8_t linkId);
    /**
     * Set the MAC layer to use.
     *
     * @param mac the MAC layer to use
     */
    virtual void SetWifiMac(const Ptr<WifiMac> mac);
    /**
     * Set the MAC TX Middle to use.
     *
     * @param txMiddle the MAC TX Middle to use
     */
    virtual void SetMacTxMiddle(const Ptr<MacTxMiddle> txMiddle);
    /**
     * Set the MAC RX Middle to use.
     *
     * @param rxMiddle the MAC RX Middle to use
     */
    virtual void SetMacRxMiddle(const Ptr<MacRxMiddle> rxMiddle);
    /**
     * Set the channel access manager to use
     *
     * @param channelAccessManager the channel access manager to use
     */
    virtual void SetChannelAccessManager(const Ptr<ChannelAccessManager> channelAccessManager);
    /**
     * Set the PHY layer to use.
     *
     * @param phy the PHY layer to use
     */
    virtual void SetWifiPhy(const Ptr<WifiPhy> phy);
    /**
     * Remove WifiPhy associated with this FrameExchangeManager.
     */
    virtual void ResetPhy();
    /**
     * Set the Protection Manager to use
     *
     * @param protectionManager the Protection Manager to use
     */
    virtual void SetProtectionManager(Ptr<WifiProtectionManager> protectionManager);
    /**
     * Set the Acknowledgment Manager to use
     *
     * @param ackManager the Acknowledgment Manager to use
     */
    virtual void SetAckManager(Ptr<WifiAckManager> ackManager);
    /**
     * Set the MAC address.
     *
     * @param address the MAC address
     */
    virtual void SetAddress(Mac48Address address);
    /**
     * Get the MAC address.
     *
     * @return the MAC address
     */
    Mac48Address GetAddress() const;
    /**
     * Set the Basic Service Set Identification.
     *
     * @param bssid the BSSID
     */
    virtual void SetBssid(Mac48Address bssid);
    /**
     * Get the Basic Service Set Identification.
     *
     * @return the BSSID
     */
    Mac48Address GetBssid() const;
    /**
     * @return the width of the channel that the FEM is allowed to use for the current transmission
     */
    MHz_u GetAllowedWidth() const;
    /**
     * Set the callback to invoke when an MPDU is dropped.
     *
     * @param callback the callback to invoke when an MPDU is dropped
     */
    virtual void SetDroppedMpduCallback(DroppedMpdu callback);
    /**
     * Set the callback to invoke when an MPDU is successfully acked.
     *
     * @param callback the callback to invoke when an MPDU is successfully acked
     */
    void SetAckedMpduCallback(AckedMpdu callback);
    /**
     * Enable promiscuous mode.
     */
    void SetPromisc();
    /**
     * Check if the device is operating in promiscuous mode.
     *
     * @return true if the device is operating in promiscuous mode,
     *         false otherwise
     */
    bool IsPromisc() const;

    /**
     * Get a const reference to the WifiTxTimer object.
     *
     * @return a const reference to the WifiTxTimer object
     */
    const WifiTxTimer& GetWifiTxTimer() const;

    /**
     * Get the Protection Manager used by this node.
     *
     * @return the Protection Manager used by this node
     */
    Ptr<WifiProtectionManager> GetProtectionManager() const;

    /**
     * Calculate the time required to protect a frame according to the given
     * protection method. The protection time is stored in the protection
     * object itself.
     *
     * @param protection the protection method
     */
    virtual void CalculateProtectionTime(WifiProtection* protection) const;

    /**
     * Get the Acknowledgment Manager used by this node.
     *
     * @return the Acknowledgment Manager used by this node
     */
    Ptr<WifiAckManager> GetAckManager() const;

    /**
     * Calculate the time required to acknowledge a frame according to the given
     * acknowledgment method. The acknowledgment time is stored in the acknowledgment
     * object itself.
     *
     * @param acknowledgment the acknowledgment method
     */
    virtual void CalculateAcknowledgmentTime(WifiAcknowledgment* acknowledgment) const;

    /**
     * @return true if the virtual CS indication is that the medium is idle
     */
    virtual bool VirtualCsMediumIdle() const;

    /**
     * @return the set of stations that have successfully received an RTS in this TXOP.
     */
    const std::set<Mac48Address>& GetProtectedStas() const;

    /**
     * Notify that an internal collision has occurred for the given Txop
     *
     * @param txop the Txop for which an internal collision has occurred
     */
    virtual void NotifyInternalCollision(Ptr<Txop> txop);

    /**
     * @param duration switching delay duration.
     *
     * This method is typically invoked by the PhyListener to notify
     * the MAC layer that a channel switching occurred. When a channel switching
     * occurs, pending MAC transmissions (RTS, CTS, Data and Ack) are cancelled.
     */
    virtual void NotifySwitchingStartNow(Time duration);

    /**
     * This method is typically invoked by the PhyListener to notify
     * the MAC layer that the device has been put into sleep mode. When the device is put
     * into sleep mode, pending MAC transmissions (RTS, CTS, Data and Ack) are cancelled.
     */
    void NotifySleepNow();

    /**
     * This method is typically invoked by the PhyListener to notify
     * the MAC layer that the device has been put into off mode. When the device is put
     * into off mode, pending MAC transmissions (RTS, CTS, Data and Ack) are cancelled.
     */
    void NotifyOffNow();

  protected:
    void DoDispose() override;

    /**
     * @return the remote station manager operating on our link
     */
    Ptr<WifiRemoteStationManager> GetWifiRemoteStationManager() const;

    /**
     * Fragment the given MPDU if needed. If fragmentation is needed, return the
     * first fragment; otherwise, return the given MPDU. Note that, if fragmentation
     * is applied, the given MPDU is dequeued from the MAC queue and the first
     * fragment is enqueued in its place.
     *
     * @param mpdu the given MPDU
     * @return the first fragment if fragmentation is needed, the given MPDU otherwise
     */
    Ptr<WifiMpdu> GetFirstFragmentIfNeeded(Ptr<WifiMpdu> mpdu);

    /**
     * Send an MPDU with the given TX parameters (with the specified protection).
     * Note that <i>txParams</i> is moved to m_txParams and hence is left in an
     * undefined state.
     *
     * @param mpdu the MPDU to send
     * @param txParams the TX parameters to use to transmit the MPDU
     */
    void SendMpduWithProtection(Ptr<WifiMpdu> mpdu, WifiTxParameters& txParams);

    /**
     * Start the protection mechanism indicated by the given TX parameters
     *
     * @param txParams the TX parameters
     */
    virtual void StartProtection(const WifiTxParameters& txParams);

    /**
     * Transmit prepared frame immediately, if no protection was used, or in a SIFS, if protection
     * was completed successfully.
     */
    virtual void ProtectionCompleted();

    /**
     * Update the NAV, if needed, based on the Duration/ID of the given <i>psdu</i>.
     *
     * @param psdu the received PSDU
     * @param txVector TxVector of the received PSDU
     */
    virtual void UpdateNav(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector);

    /**
     * Reset the NAV upon expiration of the NAV reset timer.
     */
    virtual void NavResetTimeout();

    /**
     * This method is called when the reception of a PSDU fails.
     *
     * @param psdu the PSDU whose reception failed
     */
    virtual void PsduRxError(Ptr<const WifiPsdu> psdu);

    /**
     * This method handles the reception of an MPDU (possibly included in an A-MPDU)
     *
     * @param mpdu the received MPDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector TxVector of the received PSDU
     * @param inAmpdu true if the MPDU is part of an A-MPDU
     */
    virtual void ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                             RxSignalInfo rxSignalInfo,
                             const WifiTxVector& txVector,
                             bool inAmpdu);

    /**
     * This method is called when the reception of an A-MPDU including multiple
     * MPDUs is completed.
     *
     * @param psdu the received PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector TxVector of the received PSDU
     * @param perMpduStatus per MPDU reception status
     */
    virtual void EndReceiveAmpdu(Ptr<const WifiPsdu> psdu,
                                 const RxSignalInfo& rxSignalInfo,
                                 const WifiTxVector& txVector,
                                 const std::vector<bool>& perMpduStatus);

    /**
     * Perform the actions needed when a Normal Ack is received.
     *
     * @param mpdu the MPDU that was acknowledged
     * @param txVector the TXVECTOR used to transmit the MPDU that was acknowledged
     * @param ackTxVector the TXVECTOR used to transmit the Normal Ack frame
     * @param rxInfo the info on the received signal (\see RxSignalInfo)
     * @param snr the SNR at the receiver for the MPDU that was acknowledged
     */
    virtual void ReceivedNormalAck(Ptr<WifiMpdu> mpdu,
                                   const WifiTxVector& txVector,
                                   const WifiTxVector& ackTxVector,
                                   const RxSignalInfo& rxInfo,
                                   double snr);

    /**
     * Notify other components that an MPDU was acknowledged.
     *
     * @param mpdu the MPDU that was acknowledged
     */
    virtual void NotifyReceivedNormalAck(Ptr<WifiMpdu> mpdu);

    /**
     * Retransmit an MPDU that was not acknowledged.
     *
     * @param mpdu the MPDU to retransmit
     */
    virtual void RetransmitMpduAfterMissedAck(Ptr<WifiMpdu> mpdu) const;

    /**
     * Make the sequence numbers of MPDUs included in the given PSDU available again
     * if the MPDUs have never been transmitted.
     *
     * @param psdu the given PSDU
     */
    virtual void ReleaseSequenceNumbers(Ptr<const WifiPsdu> psdu) const;

    /**
     * Pass the given MPDU, discarded because of the max retry limit was reached,
     * to the MPDU dropped callback.
     *
     * @param mpdu the discarded MPDU
     */
    virtual void NotifyPacketDiscarded(Ptr<const WifiMpdu> mpdu);

    /**
     * Perform actions that are possibly needed when receiving any frame,
     * independently of whether the frame is addressed to this station
     * (e.g., storing buffer status reports).
     *
     * @param psdu the received PSDU
     * @param txVector TX vector of the received PSDU
     */
    virtual void PreProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector);

    /**
     * Perform actions that are possibly needed after receiving any frame,
     * independently of whether the frame is addressed to this station
     * (e.g., setting the NAV or the TXOP holder).
     *
     * @param psdu the received PSDU
     * @param txVector TX vector of the received PSDU
     */
    virtual void PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector);

    /**
     * Get the updated TX duration of the frame associated with the given TX
     * parameters if the size of the PSDU addressed to the given receiver
     * becomes <i>ppduPayloadSize</i>.
     *
     * @param ppduPayloadSize the new PSDU size
     * @param receiver the MAC address of the receiver of the PSDU
     * @param txParams the TX parameters
     * @return the updated TX duration
     */
    virtual Time GetTxDuration(uint32_t ppduPayloadSize,
                               Mac48Address receiver,
                               const WifiTxParameters& txParams) const;

    /**
     * Update the TX duration field of the given TX parameters after that the PSDU
     * addressed to the given receiver has changed.
     *
     * @param receiver the MAC address of the receiver of the PSDU
     * @param txParams the TX parameters
     */
    void UpdateTxDuration(Mac48Address receiver, WifiTxParameters& txParams) const;

    /**
     * Get the size in bytes of the given MPDU, which is to be transmitted with the
     * given TXVECTOR. The purpose of this method is that it can be overridden to
     * compute the size of an S-MPDU.
     *
     * @param mpdu the given MPDU
     * @param txVector the given TXVECTOR
     * @return the size of the MPDU
     */
    virtual uint32_t GetPsduSize(Ptr<const WifiMpdu> mpdu, const WifiTxVector& txVector) const;

    /**
     * Notify the given Txop that channel has been released.
     *
     * @param txop the given Txop
     */
    virtual void NotifyChannelReleased(Ptr<Txop> txop);

    Ptr<Txop> m_dcf;                                  //!< the DCF/EDCAF that gained channel access
    WifiTxTimer m_txTimer;                            //!< the timer set upon frame transmission
    EventId m_navResetEvent;                          //!< the event to reset the NAV after an RTS
    EventId m_sendCtsEvent;                           //!< the event to send a CTS after an (MU-)RTS
    Ptr<WifiMac> m_mac;                               //!< the MAC layer on this station
    Ptr<ApWifiMac> m_apMac;                           //!< AP MAC layer pointer (null if not an AP)
    Ptr<StaWifiMac> m_staMac;                         //!< STA MAC layer pointer (null if not a STA)
    Ptr<MacTxMiddle> m_txMiddle;                      //!< the MAC TX Middle on this station
    Ptr<MacRxMiddle> m_rxMiddle;                      //!< the MAC RX Middle on this station
    Ptr<ChannelAccessManager> m_channelAccessManager; //!< the channel access manager
    Ptr<WifiPhy> m_phy;                               //!< the PHY layer on this station
    Mac48Address m_self;                              //!< the MAC address of this device
    Mac48Address m_bssid;                             //!< BSSID address (Mac48Address)
    Time m_navEnd;                                    //!< NAV expiration time
    Time m_txNav;                                     //!< the TXNAV timer
    std::set<Mac48Address> m_sentRtsTo; //!< the STA(s) which we sent an RTS to (waiting for CTS)
    std::set<Mac48Address>
        m_sentFrameTo; //!< the STA(s) to which we sent a frame requesting a response
    std::set<Mac48Address> m_protectedStas; //!< STAs that have replied to an RTS in this TXOP
    bool m_protectedIfResponded;       //!< whether a STA is assumed to be protected if replied to a
                                       //!< frame requiring acknowledgment
    uint8_t m_linkId;                  //!< the ID of the link this object is associated with
    MHz_u m_allowedWidth;              //!< the allowed width for the current transmission
    bool m_promisc;                    //!< Flag if the device is operating in promiscuous mode
    DroppedMpdu m_droppedMpduCallback; //!< the dropped MPDU callback
    AckedMpdu m_ackedMpduCallback;     //!< the acknowledged MPDU callback

    /**
     * Finalize the MAC header of the MPDUs in the given PSDU before transmission. Tasks
     * performed by this method include setting the Power Management flag in the MAC header.
     *
     * @param psdu the given PSDU
     */
    virtual void FinalizeMacHeader(Ptr<const WifiPsdu> psdu);

    /**
     * Forward an MPDU down to the PHY layer.
     *
     * @param mpdu the MPDU to forward down
     * @param txVector the TXVECTOR used to transmit the MPDU
     */
    virtual void ForwardMpduDown(Ptr<WifiMpdu> mpdu, WifiTxVector& txVector);

    /**
     * Dequeue the given MPDU from the queue in which it is stored.
     *
     * @param mpdu the given MPDU
     */
    virtual void DequeueMpdu(Ptr<const WifiMpdu> mpdu);

    /**
     * Compute how to set the Duration/ID field of a frame being transmitted with
     * the given TX parameters
     *
     * @param header the MAC header of the frame
     * @param size the size of the frame in bytes
     * @param txParams the TX parameters used to send the frame
     * @param fragmentedPacket the packet that originated the frame to transmit, in case
     *                         the latter is a fragment
     * @return the computed Duration/ID value
     */
    virtual Time GetFrameDurationId(const WifiMacHeader& header,
                                    uint32_t size,
                                    const WifiTxParameters& txParams,
                                    Ptr<Packet> fragmentedPacket) const;

    /**
     * Compute how to set the Duration/ID field of an RTS frame to send to protect
     * a frame transmitted with the given TX vector.
     *
     * @param rtsTxVector the TX vector used to send the RTS frame
     * @param txDuration the TX duration of the data frame
     * @param response the time taken by the response (acknowledgment) to the data frame
     * @return the computed Duration/ID value for the RTS frame
     */
    virtual Time GetRtsDurationId(const WifiTxVector& rtsTxVector,
                                  Time txDuration,
                                  Time response) const;

    /**
     * Send RTS to begin RTS-CTS-Data-Ack transaction.
     *
     * @param txParams the TX parameters for the data frame
     */
    void SendRts(const WifiTxParameters& txParams);

    /**
     * Send CTS after receiving RTS.
     *
     * @param rtsHdr the header of the received RTS
     * @param rtsTxMode the TX mode used to transmit the RTS
     * @param rtsSnr the SNR of the RTS in linear scale
     */
    virtual void SendCtsAfterRts(const WifiMacHeader& rtsHdr, WifiMode rtsTxMode, double rtsSnr);

    /**
     * Send CTS after receiving RTS.
     *
     * @param rtsHdr the header of the received RTS
     * @param ctsTxVector the TXVECTOR to use to transmit the CTS
     * @param rtsSnr the SNR of the RTS in linear scale
     */
    void DoSendCtsAfterRts(const WifiMacHeader& rtsHdr, WifiTxVector& ctsTxVector, double rtsSnr);

    /**
     * Compute how to set the Duration/ID field of a CTS-to-self frame to send to
     * protect a frame transmitted with the given TX vector.
     *
     * @param ctsTxVector the TX vector used to send the CTS-to-self frame
     * @param txDuration the TX duration of the data frame
     * @param response the time taken by the response (acknowledgment) to the data frame
     * @return the computed Duration/ID value for the CTS-to-self frame
     */
    virtual Time GetCtsToSelfDurationId(const WifiTxVector& ctsTxVector,
                                        Time txDuration,
                                        Time response) const;

    /**
     * Send CTS for a CTS-to-self mechanism.
     *
     * @param txParams the TX parameters for the data frame
     */
    void SendCtsToSelf(const WifiTxParameters& txParams);

    /**
     * Send Normal Ack.
     *
     * @param hdr the header of the frame soliciting the Normal Ack
     * @param dataTxVector the TXVECTOR used to transmit the frame soliciting the Normal Ack
     * @param dataSnr the SNR of the frame soliciting the Normal Ack in linear scale
     */
    void SendNormalAck(const WifiMacHeader& hdr, const WifiTxVector& dataTxVector, double dataSnr);

    /**
     * Get the next fragment of the current MSDU.
     * Only called for fragmented MSDUs.
     *
     * @return the next fragment of the current MSDU.
     */
    Ptr<WifiMpdu> GetNextFragment();

    /**
     * Take necessary actions upon a transmission success. A non-QoS station
     * transmits the next fragment, if any, or releases the channel, otherwise.
     */
    virtual void TransmissionSucceeded();

    /**
     * Take necessary actions upon a transmission failure. A non-QoS station
     * releases the channel when this method is called.
     *
     * @param forceCurrentCw whether to force the contention window to stay equal to the current
     *                       value (normally, contention window is updated upon TX failure)
     */
    virtual void TransmissionFailed(bool forceCurrentCw = false);

    /**
     * Wrapper for the GetMpdusToDropOnTxFailure function of the remote station manager that
     * additionally drops the MPDUs in the given PSDU that the remote station manager requested
     * to drop.
     *
     * @param psdu the given PSDU
     * @return an MPDU that has been dropped, if any, to be notified to the remote station manager
     *         through the appropriate function
     */
    Ptr<WifiMpdu> DropMpduIfRetryLimitReached(Ptr<WifiPsdu> psdu);

    /**
     * Called when the Ack timeout expires.
     *
     * @param mpdu the MPDU that solicited a Normal Ack response
     * @param txVector the TXVECTOR used to transmit the frame soliciting the Normal Ack
     */
    virtual void NormalAckTimeout(Ptr<WifiMpdu> mpdu, const WifiTxVector& txVector);

    /**
     * Called when the CTS timeout expires.
     *
     * @param rts the RTS that solicited a CTS response
     * @param txVector the TXVECTOR used to transmit the RTS frame
     */
    virtual void CtsTimeout(Ptr<WifiMpdu> rts, const WifiTxVector& txVector);
    /**
     * Take required actions when the CTS timer fired after sending an RTS to
     * protect the given PSDU expires.
     *
     * @param psdu the PSDU protected by the failed RTS
     */
    void DoCtsTimeout(Ptr<WifiPsdu> psdu);

    /**
     * @return whether CW shall be updated on CTS timeout
     */
    virtual bool GetUpdateCwOnCtsTimeout() const;

    /**
     * Reset this frame exchange manager.
     */
    virtual void Reset();

    /**
     * @param txVector the TXVECTOR decoded from PHY header.
     * @param psduDuration the duration of the PSDU that is about to be received.
     *
     * This method is typically invoked by the lower PHY layer to notify
     * the MAC layer that the reception of a PSDU is starting.
     * This is equivalent to the PHY-RXSTART primitive.
     * If the reception is correct for at least one MPDU of the PSDU
     * the Receive method will be called after \p psduDuration.
     */
    virtual void RxStartIndication(WifiTxVector txVector, Time psduDuration);

    /**
     * Store information about the MAC header of the MPDU being received.
     *
     * @param macHdr the MAC header of the MPDU being received
     * @param txVector the TXVECTOR used to transmit the PSDU
     * @param psduDuration the remaining duration of the PSDU
     */
    virtual void ReceivedMacHdr(const WifiMacHeader& macHdr,
                                const WifiTxVector& txVector,
                                Time psduDuration);

    /**
     * Notify the last (re)transmission of a groupcast MPDU using the GCR-UR service.
     *
     * @param mpdu the groupcast MPDU
     */
    virtual void NotifyLastGcrUrTx(Ptr<const WifiMpdu> mpdu);

  private:
    /**
     * Send the current MPDU, which can be acknowledged by a Normal Ack.
     */
    void SendMpdu();

    Ptr<WifiMpdu> m_mpdu;           //!< the MPDU being transmitted
    WifiTxParameters m_txParams;    //!< the TX parameters for the current frame
    Ptr<Packet> m_fragmentedPacket; //!< the MSDU being fragmented
    bool m_moreFragments;           //!< true if a fragment has to be sent after a SIFS
    Ptr<WifiProtectionManager> m_protectionManager; //!< Protection manager
    Ptr<WifiAckManager> m_ackManager;               //!< Acknowledgment manager

    OngoingRxInfo
        m_ongoingRxInfo{}; //!< information about the MAC header of the MPDU being received
};

} // namespace ns3

#endif /* FRAME_EXCHANGE_MANAGER_H */
