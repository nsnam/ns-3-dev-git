/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Gary Pei <guangyu.pei@boeing.com>
 *  kwong yin <kwong-sang.yin@boeing.com>
 *  Tom Henderson <thomas.r.henderson@boeing.com>
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 *  Alberto Gallegos Ramonet <ramonet@fc.ritsumei.ac.jp>
 */

#ifndef LR_WPAN_MAC_H
#define LR_WPAN_MAC_H

#include "lr-wpan-fields.h"
#include "lr-wpan-mac-base.h"
#include "lr-wpan-phy.h"

#include "ns3/event-id.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-callback.h"
#include "ns3/traced-value.h"

#include <deque>
#include <memory>

namespace ns3
{
class Packet;

namespace lrwpan
{

class LrWpanCsmaCa;

/**
 * @defgroup lr-wpan LR-WPAN models
 *
 * This section documents the API of the IEEE 802.15.4-related models.  For a generic functional
 * description, please refer to the ns-3 manual.
 */

/**
 * @ingroup lr-wpan
 *
 * Tx options
 */
enum TxOption
{
    TX_OPTION_NONE = 0,    //!< TX_OPTION_NONE
    TX_OPTION_ACK = 1,     //!< TX_OPTION_ACK
    TX_OPTION_GTS = 2,     //!< TX_OPTION_GTS
    TX_OPTION_INDIRECT = 4 //!< TX_OPTION_INDIRECT
};

/**
 * @ingroup lr-wpan
 *
 * MAC states
 */
enum MacState
{
    MAC_IDLE,               //!< MAC_IDLE
    MAC_CSMA,               //!< MAC_CSMA
    MAC_SENDING,            //!< MAC_SENDING
    MAC_ACK_PENDING,        //!< MAC_ACK_PENDING
    CHANNEL_ACCESS_FAILURE, //!< CHANNEL_ACCESS_FAILURE
    CHANNEL_IDLE,           //!< CHANNEL_IDLE
    SET_PHY_TX_ON,          //!< SET_PHY_TX_ON
    MAC_GTS,                //!< MAC_GTS
    MAC_INACTIVE,           //!< MAC_INACTIVE
    MAC_CSMA_DEFERRED       //!< MAC_CSMA_DEFERRED
};

/**
 *  Overloaded operator to print the value of a MacState.
 *
 *  @param os The output stream
 *  @param state The text value of the PHY state
 *  @return The output stream with text value of the MAC state
 */
std::ostream& operator<<(std::ostream& os, const MacState& state);

/**
 * @ingroup lr-wpan
 *
 * Superframe status
 */
enum SuperframeStatus
{
    BEACON,  //!< The Beacon transmission or reception Period
    CAP,     //!< Contention Access Period
    CFP,     //!< Contention Free Period
    INACTIVE //!< Inactive Period or unslotted CSMA-CA
};

/**
 * @ingroup lr-wpan
 *
 * Superframe type
 */
enum SuperframeType
{
    OUTGOING = 0, //!< Outgoing Superframe
    INCOMING = 1  //!< Incoming Superframe
};

/**
 * @ingroup lr-wpan
 *
 * Indicates a pending MAC primitive
 */
enum PendingPrimitiveStatus
{
    MLME_NONE = 0,      //!< No pending primitive
    MLME_START_REQ = 1, //!< Pending MLME-START.request primitive
    MLME_SCAN_REQ = 2,  //!< Pending MLME-SCAN.request primitive
    MLME_ASSOC_REQ = 3, //!< Pending MLME-ASSOCIATION.request primitive
    MLME_SYNC_REQ = 4   //!< Pending MLME-SYNC.request primitive
};

namespace TracedValueCallback
{

/**
 * @ingroup lr-wpan
 * TracedValue callback signature for MacState.
 *
 * @param [in] oldValue original value of the traced variable
 * @param [in] newValue new value of the traced variable
 */
typedef void (*MacState)(MacState oldValue, MacState newValue);

/**
 * @ingroup lr-wpan
 * TracedValue callback signature for SuperframeStatus.
 *
 * @param [in] oldValue original value of the traced variable
 * @param [in] newValue new value of the traced variable
 */
typedef void (*SuperframeStatus)(SuperframeStatus oldValue, SuperframeStatus newValue);

} // namespace TracedValueCallback

/**
 * @ingroup lr-wpan
 *
 * Class that implements the LR-WPAN MAC state machine
 */
class LrWpanMac : public LrWpanMacBase
{
  public:
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Default constructor.
     */
    LrWpanMac();
    ~LrWpanMac() override;

    /**
     * Check if the receiver will be enabled when the MAC is idle.
     *
     * @return true, if the receiver is enabled during idle periods, false otherwise
     */
    bool GetRxOnWhenIdle() const;

    /**
     * Set if the receiver should be enabled when the MAC is idle.
     *
     * @param rxOnWhenIdle set to true to enable the receiver during idle periods
     */
    void SetRxOnWhenIdle(bool rxOnWhenIdle);

    // XXX these setters will become obsolete if we use the attribute system
    /**
     * Set the short address of this MAC.
     *
     * @param address the new address
     */
    void SetShortAddress(Mac16Address address);

    /**
     * Get the short address of this MAC.
     *
     * @return the short address
     */
    Mac16Address GetShortAddress() const;

    /**
     * Set the extended address of this MAC.
     *
     * @param address the new address
     */
    void SetExtendedAddress(Mac64Address address);

    /**
     * Get the extended address of this MAC.
     *
     * @return the extended address
     */
    Mac64Address GetExtendedAddress() const;

    /**
     * Set the PAN id used by this MAC.
     *
     * @param panId the new PAN id.
     */
    void SetPanId(uint16_t panId);

    /**
     * Get the PAN id used by this MAC.
     *
     * @return the PAN id.
     */
    uint16_t GetPanId() const;

    /**
     * Get the coordinator short address currently associated to this device.
     *
     * @return The coordinator short address
     */
    Mac16Address GetCoordShortAddress() const;

    /**
     * Get the coordinator extended address currently associated to this device.
     *
     * @return The coordinator extended address
     */
    Mac64Address GetCoordExtAddress() const;

    void McpsDataRequest(McpsDataRequestParams params, Ptr<Packet> p) override;

    void MlmeStartRequest(MlmeStartRequestParams params) override;

    void MlmeScanRequest(MlmeScanRequestParams params) override;

    void MlmeAssociateRequest(MlmeAssociateRequestParams params) override;

    void MlmeAssociateResponse(MlmeAssociateResponseParams params) override;

    void MlmeOrphanResponse(MlmeOrphanResponseParams params) override;

    void MlmeSyncRequest(MlmeSyncRequestParams params) override;

    void MlmePollRequest(MlmePollRequestParams params) override;

    void MlmeSetRequest(MacPibAttributeIdentifier id, Ptr<MacPibAttributes> attribute) override;

    void MlmeGetRequest(MacPibAttributeIdentifier id) override;

    /**
     * Set the CSMA/CA implementation to be used by the MAC.
     *
     * @param csmaCa the CSMA/CA implementation
     */
    void SetCsmaCa(Ptr<LrWpanCsmaCa> csmaCa);

    /**
     * Set the underlying PHY for the MAC.
     *
     * @param phy the PHY
     */
    void SetPhy(Ptr<LrWpanPhy> phy);

    /**
     * Get the underlying PHY of the MAC.
     *
     * @return the PHY
     */
    Ptr<LrWpanPhy> GetPhy();

    ////////////////////////////////////
    // Interfaces between MAC and PHY //
    ////////////////////////////////////

    /**
     * IEEE 802.15.4-2006 section 6.2.1.3
     * PD-DATA.indication
     * Indicates the transfer of an MPDU from PHY to MAC (receiving)
     * @param psduLength number of bytes in the PSDU
     * @param p the packet to be transmitted
     * @param lqi Link quality (LQI) value measured during reception of the PPDU
     */
    void PdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi);

    /**
     * IEEE 802.15.4-2006 section 6.2.1.2
     * Confirm the end of transmission of an MPDU to MAC
     * @param status to report to MAC
     *        PHY PD-DATA.confirm status
     */
    void PdDataConfirm(PhyEnumeration status);

    /**
     * IEEE 802.15.4-2006 section 6.2.2.2
     * PLME-CCA.confirm status
     * @param status TRX_OFF, BUSY or IDLE
     */
    void PlmeCcaConfirm(PhyEnumeration status);

    /**
     * IEEE 802.15.4-2006 section 6.2.2.4
     * PLME-ED.confirm status and energy level
     * @param status SUCCESS, TRX_OFF or TX_ON
     * @param energyLevel 0x00-0xff ED level for the channel
     */
    void PlmeEdConfirm(PhyEnumeration status, uint8_t energyLevel);

    /**
     * IEEE 802.15.4-2006 section 6.2.2.6
     * PLME-GET.confirm
     * Get attributes per definition from Table 23 in section 6.4.2
     * @param status SUCCESS or UNSUPPORTED_ATTRIBUTE
     * @param id the attributed identifier
     * @param attribute the attribute value
     */
    void PlmeGetAttributeConfirm(PhyEnumeration status,
                                 PhyPibAttributeIdentifier id,
                                 Ptr<PhyPibAttributes> attribute);

    /**
     * IEEE 802.15.4-2006 section 6.2.2.8
     * PLME-SET-TRX-STATE.confirm
     * Set PHY state
     * @param status in RX_ON,TRX_OFF,FORCE_TRX_OFF,TX_ON
     */
    void PlmeSetTRXStateConfirm(PhyEnumeration status);

    /**
     * IEEE 802.15.4-2006 section 6.2.2.10
     * PLME-SET.confirm
     * Set attributes per definition from Table 23 in section 6.4.2
     * @param status SUCCESS, UNSUPPORTED_ATTRIBUTE, INVALID_PARAMETER, or READ_ONLY
     * @param id the attributed identifier
     */
    void PlmeSetAttributeConfirm(PhyEnumeration status, PhyPibAttributeIdentifier id);

    /**
     * CSMA-CA algorithm calls back the MAC after executing channel assessment.
     *
     * @param macState indicate BUSY or IDLE channel condition
     */
    void SetLrWpanMacState(MacState macState);

    /**
     * Set the max size of the transmit queue.
     *
     * @param queueSize The transmit queue size.
     */
    void SetTxQMaxSize(uint32_t queueSize);

    /**
     * Set the max size of the indirect transmit queue (Pending Transaction list)
     *
     * @param queueSize The indirect transmit queue size.
     */
    void SetIndTxQMaxSize(uint32_t queueSize);

    // MAC PIB attributes

    /**
     * The time that the device transmitted its last beacon frame.
     * It also indicates the start of the Active Period in the Outgoing superframe.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    Time m_macBeaconTxTime;

    /**
     * The time that the device received its last bit of the beacon frame.
     * It does not indicate the start of the Active Period in the Incoming superframe.
     * Not explicitly listed by the standard but its use is implied.
     * Its purpose is somehow similar to m_macBeaconTxTime
     */
    Time m_macBeaconRxTime;

    /**
     * The maximum time, in multiples of aBaseSuperframeDuration, a device
     * shall wait for a response command frame to be available following a
     * request command frame.
     */
    uint64_t m_macResponseWaitTime;

    /**
     * The maximum wait time for an association response command after the reception
     * of data request command ACK during the association process. Not explicitly
     * listed by the standard but its use is required for a device to react to the lost
     * of the association response (failure of the association: NO_DATA)
     */
    uint64_t m_assocRespCmdWaitTime;

    /**
     * The short address of the coordinator through which the device is
     * associated.
     * 0xFFFF indicates this value is unknown.
     * 0xFFFE indicates the coordinator is only using its extended address.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    Mac16Address m_macCoordShortAddress;

    /**
     * The extended address of the coordinator through which the device
     * is associated.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    Mac64Address m_macCoordExtendedAddress;

    /**
     * Symbol boundary is same as m_macBeaconTxTime.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    uint64_t m_macSyncSymbolOffset;

    /**
     * Used by a PAN coordinator or coordinator.
     * Defines how often the coordinator transmits its beacon
     * (outgoing superframe). Range 0 - 15 with 15 meaning no beacons are being sent.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    uint8_t m_macBeaconOrder;

    /**
     * Used by a PAN coordinator or coordinator. The length of the active portion
     * of the outgoing superframe, including the beacon frame.
     * 0 - 15 with 15 means the superframe will not be active after the beacon.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    uint8_t m_macSuperframeOrder;

    /**
     * The maximum time (in UNIT periods) that a transaction is stored by a
     * coordinator and indicated in its beacon. This value establish the expiration
     * time of the packets stored in the pending transaction list (indirect transmissions).
     * 1 Unit Period:
     * Beacon-enabled = aBaseSuperframeDuration * 2^BO
     * Non-beacon enabled = aBaseSuperframeDuration
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    uint16_t m_macTransactionPersistenceTime;

    /**
     * The total size of the received beacon in symbols.
     * Its value is used to calculate the end CAP time of the incoming superframe.
     */
    uint64_t m_rxBeaconSymbols;

    /**
     * Indication of the Slot where the CAP portion of the OUTGOING Superframe ends.
     */
    uint8_t m_fnlCapSlot;

    /**
     * The beaconOrder value of the INCOMING frame. Used by all devices that have a parent.
     * Specification of how often the parent coordinator transmits its beacon.
     * 0 - 15 with 15 means the parent is not currently transmitting beacons.
     */
    uint8_t m_incomingBeaconOrder;

    /**
     * Used by all devices that have a parent.
     * The length of the active portion of the INCOMING superframe, including the
     * beacon frame.
     * 0 - 15 with 15 meaning the superframe will not be active after the beacon.
     */
    uint8_t m_incomingSuperframeOrder;

    /**
     * Indication of the Slot where the CAP portion of the INCOMING Superframe ends.
     */
    uint8_t m_incomingFnlCapSlot;

    /**
     * Indicates if MAC sublayer is in receive all mode. True mean accept all
     * frames from PHY.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    bool m_macPromiscuousMode;

    /**
     * 16 bits id of PAN on which this device is operating. 0xffff means not
     * associated.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    uint16_t m_macPanId;

    /**
     * Temporarily stores the value of the current m_macPanId when a MLME-SCAN.request is performed.
     * See IEEE 802.15.4-2011, section 5.1.2.1.2.
     */
    uint16_t m_macPanIdScan;

    /**
     * Sequence number added to transmitted data or MAC command frame, 00-ff.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    SequenceNumber8 m_macDsn;

    /**
     * Sequence number added to transmitted beacon frame, 00-ff.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    SequenceNumber8 m_macBsn;

    /**
     * The set with the contents of the beacon payload.
     * This value is set directly by the MLME-SET primitive.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    std::vector<uint8_t> m_macBeaconPayload;

    /**
     * The length, in octets, of the beacon payload.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    uint32_t m_macBeaconPayloadLength;

    /**
     * The maximum number of retries allowed after a transmission failure.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    uint8_t m_macMaxFrameRetries;

    /**
     * Indication of whether the MAC sublayer is to enable its receiver during
     * idle periods.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    bool m_macRxOnWhenIdle;

    /**
     * The minimum time forming a Long InterFrame Spacing (LIFS) period.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    uint32_t m_macLIFSPeriod;

    /**
     * The minimum time forming a Short InterFrame Spacing (SIFS) period.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    uint32_t m_macSIFSPeriod;

    /**
     * Indication of whether a coordinator is currently allowing association.
     * A value of TRUE indicates that the association is permitted.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    bool m_macAssociationPermit;

    /**
     * Indication of whether a device automatically sends data request command
     * if its address is listed in the beacon frame.
     * TRUE = request command automatically is sent. This command also affects
     * the generation of MLME-BEACON-NOTIFY.indication (6.2.4.1)
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    bool m_macAutoRequest;

    /**
     * The maximum energy level detected during ED scan on the current channel.
     */
    uint8_t m_maxEnergyLevel;

    /**
     * The value of the necessary InterFrame Space after the transmission of a packet.
     */
    uint32_t m_ifs;

    /**
     * Indication of whether the current device is the PAN coordinator
     */
    bool m_panCoor;

    /**
     * Indicates if the current device is a coordinator type
     */
    bool m_coor;

    /**
     * Indication of the Interval used by the coordinator to transmit beacon frames
     * expressed in symbols.
     */
    uint32_t m_beaconInterval;

    /**
     * Indication of the superframe duration in symbols.
     * (e.g. 1 symbol = 4 bits in a 250kbps O-QPSK PHY)
     */
    uint32_t m_superframeDuration;

    /**
     * Indication of the interval a node should receive a superframe
     * expressed in symbols.
     */
    uint32_t m_incomingBeaconInterval;

    /**
     * Indication of the superframe duration in symbols
     * (e.g. 1 symbol = 4 bits in a 250kbps O-QPSK PHY)
     */
    uint32_t m_incomingSuperframeDuration;

    /**
     * Indication of current device capability (FFD or RFD)
     */
    uint8_t m_deviceCapability;

    /**
     * Indication of whether the current device is tracking incoming beacons.
     */
    bool m_beaconTrackingOn;

    /**
     * The number of consecutive loss beacons in a beacon tracking operation.
     */
    uint8_t m_numLostBeacons;

    /**
     * Get the macAckWaitDuration attribute value.
     *
     * @return the maximum number symbols to wait for an acknowledgment frame
     */
    uint64_t GetMacAckWaitDuration() const;

    /**
     * Get the macMaxFrameRetries attribute value.
     *
     * @return the maximum number of retries
     */
    uint8_t GetMacMaxFrameRetries() const;

    /**
     * Print the number of elements in the packet transmit queue.
     */
    void PrintTransmitQueueSize();

    /**
     * Set the macMaxFrameRetries attribute value.
     *
     * @param retries the maximum number of retries
     */
    void SetMacMaxFrameRetries(uint8_t retries);

    /**
     * Check if the packet destination is its coordinator
     *
     * @return True if m_txPkt (packet awaiting to be sent) destination is its coordinator
     */
    bool IsCoordDest();

    /**
     * Check if the packet destination is its coordinator
     *
     * @param mac The coordinator short MAC Address
     */
    void SetAssociatedCoor(Mac16Address mac);

    /**
     * Check if the packet destination is its coordinator
     *
     * @param mac The coordinator extended MAC Address
     */
    void SetAssociatedCoor(Mac64Address mac);

    /**
     * Get the size of the Interframe Space according to MPDU size (m_txPkt).
     *
     * @return the IFS size in symbols
     */
    uint32_t GetIfsSize();

    /**
     * Obtain the number of symbols in the packet which is currently being sent by the MAC layer.
     *
     * @return packet number of symbols
     */
    uint64_t GetTxPacketSymbols();

    /**
     * Check if the packet to transmit requires acknowledgment
     *
     * @return True if the Tx packet requires acknowledgment
     */
    bool IsTxAckReq();

    /**
     * Print the Pending transaction list.
     * @param os The reference to the output stream used by this print function.
     */
    void PrintPendingTxQueue(std::ostream& os) const;

    /**
     * Print the Transmit Queue.
     * @param os The reference to the output stream used by this print function.
     */
    void PrintTxQueue(std::ostream& os) const;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams that have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

    /**
     * TracedCallback signature for sent packets.
     *
     * @param [in] packet The packet.
     * @param [in] retries The number of retries.
     * @param [in] backoffs The number of CSMA backoffs.
     */
    typedef void (*SentTracedCallback)(Ptr<const Packet> packet, uint8_t retries, uint8_t backoffs);

    /**
     * TracedCallback signature for MacState change events.
     *
     * @param [in] oldValue The original state value.
     * @param [in] newValue The new state value.
     * @deprecated The MacState is now accessible as the
     * TracedValue \c MacStateValue. The \c MacState TracedCallback will
     * be removed in a future release.
     */
    // NS_DEPRECATED() - tag for future removal
    typedef void (*StateTracedCallback)(MacState oldState, MacState newState);

  protected:
    // Inherited from Object.
    void DoInitialize() override;
    void DoDispose() override;

  private:
    /**
     * Helper structure for managing transmission queue elements.
     */
    struct TxQueueElement : public SimpleRefCount<TxQueueElement>
    {
        uint8_t txQMsduHandle; //!< MSDU Handle
        Ptr<Packet> txQPkt;    //!< Queued packet
    };

    /**
     * Helper structure for managing pending transaction list elements (Indirect transmissions).
     */
    struct IndTxQueueElement : public SimpleRefCount<IndTxQueueElement>
    {
        uint8_t seqNum;               //!< The sequence number of  the queued packet
        Mac16Address dstShortAddress; //!< The destination short Mac Address
        Mac64Address dstExtAddress;   //!< The destination extended Mac Address
        Ptr<Packet> txQPkt;           //!< Queued packet.
        Time expireTime; //!< The expiration time of the packet in the indirect transmission queue.
    };

    /**
     * Called to send a single beacon frame.
     */
    void SendOneBeacon();

    /**
     * Called to send an associate request command.
     */
    void SendAssocRequestCommand();

    /**
     * Used to send a data request command (i.e. Request the coordinator to send the association
     * response)
     */
    void SendDataRequestCommand();

    /**
     * Called to send an associate response command.
     *
     * @param rxDataReqPkt The received data request pkt that instigated the Association response
     * command.
     */
    void SendAssocResponseCommand(Ptr<Packet> rxDataReqPkt);

    /**
     * Called after m_assocRespCmdWaitTime timeout while waiting for an association response
     * command.
     */
    void LostAssocRespCommand();

    /**
     * Called to send a beacon request command.
     */
    void SendBeaconRequestCommand();

    /**
     * Called to send a orphan notification command. This is used by an associated device that
     * has lost synchronization with its coordinator.
     * As described in IEEE 802.15.4-2011 (Section 5.3.6)
     */
    void SendOrphanNotificationCommand();

    /**
     * Called to end a MLME-START.request after changing the page and channel number.
     */
    void EndStartRequest();

    /**
     * Called at the end of the current channel scan (Active or Passive) for a given duration.
     */
    void EndChannelScan();

    /**
     * Called at the end of one ED channel scan.
     */
    void EndChannelEnergyScan();

    /**
     * Called to end an MLME-ASSOCIATE.request after changing the page and channel number.
     */
    void EndAssociateRequest();

    /**
     * Called to begin the Contention Free Period (CFP) in a
     * beacon-enabled mode.
     *
     * @param superframeType The incoming or outgoing superframe reference
     */
    void StartCFP(SuperframeType superframeType);

    /**
     * Called to begin the Contention Access Period (CAP) in a
     * beacon-enabled mode.
     *
     * @param superframeType The incoming or outgoing superframe reference
     */
    void StartCAP(SuperframeType superframeType);

    /**
     * Start the Inactive Period in a beacon-enabled mode.
     *
     * @param superframeType The incoming or outgoing superframe reference
     *
     */
    void StartInactivePeriod(SuperframeType superframeType);

    /**
     * Called after the end of an INCOMING superframe to start the moment a
     * device waits for a new incoming beacon.
     */
    void AwaitBeacon();

    /**
     * Called if the device is unable to locate a beacon in the time set by MLME-SYNC.request.
     */
    void BeaconSearchTimeout();

    /**
     * Used to process the reception of a beacon packet.
     *
     * @param lqi The value of the link quality indicator (LQI) of the received packet
     * @param p The packet containing the MAC header and the beacon payload information
     */
    void ReceiveBeacon(uint8_t lqi, Ptr<Packet> p);

    /**
     * Send an acknowledgment packet for the given sequence number.
     *
     * @param seqno the sequence number for the ACK
     */
    void SendAck(uint8_t seqno);

    /**
     * Add an element to the transmission queue.
     *
     * @param txQElement The element added to the Tx Queue.
     */
    void EnqueueTxQElement(Ptr<TxQueueElement> txQElement);

    /**
     * Remove the tip of the transmission queue, including clean up related to the
     * last packet transmission.
     */
    void RemoveFirstTxQElement();

    /**
     * Change the current MAC state to the given new state.
     *
     * @param newState the new state
     */
    void ChangeMacState(MacState newState);

    /**
     * Handle an ACK timeout with a packet retransmission, if there are
     * retransmission left, or a packet drop.
     */
    void AckWaitTimeout();

    /**
     * After a successful transmission of a frame (beacon, data) or an ack frame reception,
     * the mac layer wait an Interframe Space (IFS) time and triggers this function
     * to continue with the MAC flow.
     *
     * @param ifsTime IFS time
     */
    void IfsWaitTimeout(Time ifsTime);

    /**
     * Check for remaining retransmissions for the packet currently being sent.
     * Drop the packet, if there are no retransmissions left.
     *
     * @return true, if the packet should be retransmitted, false otherwise.
     */
    bool PrepareRetransmission();

    /**
     * Adds a packet to the pending transactions list (Indirect transmissions).
     *
     * @param p The packet added to pending transaction list.
     */
    void EnqueueInd(Ptr<Packet> p);

    /**
     * Extracts a packet from pending transactions list (Indirect transmissions).
     * @param dst The extended address used an index to obtain an element from the pending
     * transaction list.
     * @param entry The dequeued element from the pending transaction list.
     * @return The status of the dequeue
     */
    bool DequeueInd(Mac64Address dst, Ptr<IndTxQueueElement> entry);

    /**
     * Purge expired transactions from the pending transactions list.
     */
    void PurgeInd();

    /**
     * Remove an element from the pending transaction list.
     *
     * @param p The packet to be removed from the pending transaction list.
     */
    void RemovePendTxQElement(Ptr<Packet> p);

    /**
     * Check the transmission queue. If there are packets in the transmission
     * queue and the MAC is idle, pick the first one and initiate a packet
     * transmission.
     */
    void CheckQueue();

    /**
     * Constructs a Superframe specification field from the local information,
     * the superframe Specification field is necessary to create a beacon frame.
     *
     * @returns the Superframe specification field (bitmap)
     */
    uint16_t GetSuperframeField();

    /**
     * Constructs the Guaranteed Time Slots (GTS) Fields from local information.
     * The GTS Fields are part of the beacon frame.
     *
     * @returns the Guaranteed Time Slots (GTS) Fields
     */
    GtsFields GetGtsFields();

    /**
     * Constructs Pending Address Fields from the local information,
     * the Pending Address Fields are part of the beacon frame.
     *
     * @returns the Pending Address Fields
     */
    PendingAddrFields GetPendingAddrFields();

    /**
     * The trace source is fired at the end of any Interframe Space (IFS).
     */
    TracedCallback<Time> m_macIfsEndTrace;

    /**
     * The trace source fired when packets are considered as successfully sent
     * or the transmission has been given up.
     * Only non-broadcast packets are traced.
     *
     * The data should represent:
     * packet, number of retries, total number of csma backoffs
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>, uint8_t, uint8_t> m_sentPktTrace;

    /**
     * The trace source fired when packets come into the "top" of the device
     * at the L3/L2 transition, when being queued for transmission.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxEnqueueTrace;

    /**
     * The trace source fired when packets are dequeued from the
     * L3/l2 transmission queue.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxDequeueTrace;

    /**
     * The trace source fired when packets come into the "top" of the device
     * at the L3/L2 transition, when being queued for indirect transmission
     * (pending transaction list).
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macIndTxEnqueueTrace;

    /**
     * The trace source fired when packets are dequeued from the
     * L3/l2 indirect transmission queue (Pending transaction list).
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macIndTxDequeueTrace;

    /**
     * The trace source fired when packets are being sent down to L1.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxTrace;

    /**
     * The trace source fired when packets where successfully transmitted, that is
     * an acknowledgment was received, if requested, or the packet was
     * successfully sent by L1, if no ACK was requested.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxOkTrace;

    /**
     * The trace source fired when packets are dropped due to missing ACKs or
     * because of transmission failures in L1.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxDropTrace;

    /**
     * The trace source fired when packets are dropped due to indirect Tx queue
     * overflows or expiration.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macIndTxDropTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a promiscuous trace.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macPromiscRxTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a non-promiscuous trace.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macRxTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * but dropped before being forwarded up to higher layers (at the L2/L3
     * transition).
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macRxDropTrace;

    /**
     * A trace source that emulates a non-promiscuous protocol sniffer connected
     * to the device.  Unlike your average everyday sniffer, this trace source
     * will not fire on PACKET_OTHERHOST events.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device hard_start_xmit where
     * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in netif_receive_skb.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_snifferTrace;

    /**
     * A trace source that emulates a promiscuous mode protocol sniffer connected
     * to the device.  This trace source fire on packets destined for any host
     * just like your average everyday packet sniffer.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device hard_start_xmit where
     * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in netif_receive_skb.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_promiscSnifferTrace;

    /**
     * A trace source that fires when the MAC changes states.
     * Parameters are the old mac state and the new mac state.
     *
     * @deprecated This TracedCallback is deprecated and will be
     * removed in a future release,  Instead, use the \c MacStateValue
     * TracedValue.
     */
    // NS_DEPRECATED() - tag for future removal
    TracedCallback<MacState, MacState> m_macStateLogger;

    /**
     * The PHY associated with this MAC.
     */
    Ptr<LrWpanPhy> m_phy;

    /**
     * The CSMA/CA implementation used by this MAC.
     */
    Ptr<LrWpanCsmaCa> m_csmaCa;

    /**
     * The current state of the MAC layer.
     */
    TracedValue<MacState> m_macState;

    /**
     * The current period of the incoming superframe.
     */
    TracedValue<SuperframeStatus> m_incSuperframeStatus;

    /**
     * The current period of the outgoing superframe.
     */
    TracedValue<SuperframeStatus> m_outSuperframeStatus;

    /**
     * The packet which is currently being sent by the MAC layer.
     */
    Ptr<Packet> m_txPkt;

    /**
     * The command request packet received. Briefly stored to proceed with operations
     * that take place after ACK messages.
     */
    Ptr<Packet> m_rxPkt;

    /**
     * The short address (16 bit address) used by this MAC. If supported,
     * the short address must be assigned to the device by the coordinator
     * during the association process.
     */
    Mac16Address m_shortAddress;

    /**
     * The extended 64 address (IEEE EUI-64) used by this MAC.
     */
    Mac64Address m_macExtendedAddress;

    /**
     * The transmit queue used by the MAC.
     */
    std::deque<Ptr<TxQueueElement>> m_txQueue;

    /**
     * The indirect transmit queue used by the MAC pending messages (The pending transaction
     * list).
     */
    std::deque<Ptr<IndTxQueueElement>> m_indTxQueue;

    /**
     * The maximum size of the transmit queue.
     */
    uint32_t m_maxTxQueueSize;

    /**
     * The maximum size of the indirect transmit queue (The pending transaction list).
     */
    uint32_t m_maxIndTxQueueSize;

    /**
     * The list of PAN descriptors accumulated during channel scans, used to select a PAN to
     * associate.
     */
    std::vector<PanDescriptor> m_panDescriptorList;

    /**
     * The list of energy measurements, one for each channel searched during an ED scan.
     */
    std::vector<uint8_t> m_energyDetectList;

    /**
     * The list of unscanned channels during a scan operation.
     */
    std::vector<uint8_t> m_unscannedChannels;

    /**
     * The parameters used during a MLME-SCAN.request. These parameters are stored here while
     * PLME-SET (set channel page, set channel number) and other operations take place.
     */
    MlmeScanRequestParams m_scanParams;

    /**
     * The parameters used during a MLME-START.request. These parameters are stored here while
     * PLME-SET operations (set channel page, set channel number) take place.
     */
    MlmeStartRequestParams m_startParams;

    /**
     * The parameters used during a MLME-ASSOCIATE.request. These parameters are stored here while
     * PLME-SET operations (set channel page, set channel number) take place.
     */
    MlmeAssociateRequestParams m_associateParams;

    /**
     * The channel list index used to obtain the current scanned channel.
     */
    uint16_t m_channelScanIndex;

    /**
     * Indicates the pending primitive when PLME.SET operation (page or channel switch) is called
     * from within another MLME primitive (e.g. Association, Scan, Sync, Start).
     */
    PendingPrimitiveStatus m_pendPrimitive;

    /**
     * The number of already used retransmission for the currently transmitted
     * packet.
     */
    uint8_t m_retransmission;

    /**
     * The number of CSMA/CA retries used for sending the current packet.
     */
    uint8_t m_numCsmacaRetry;

    /**
     * Keep track of the last received frame Link Quality Indicator
     */
    uint8_t m_lastRxFrameLqi;

    /**
     * This flag informs the MAC that an association response command was received
     * before the acknowledgment (ACK) for the data request command that
     * should precede it. This situation typically occurs due to network saturation.
     */

    bool m_ignoreDataCmdAck;

    /**
     * Scheduler event for the ACK timeout of the currently transmitted data
     * packet.
     */
    EventId m_ackWaitTimeout;

    /**
     * Scheduler event for a response to a request command frame.
     */
    EventId m_respWaitTimeout;

    /**
     * Scheduler event for the lost of a association response command frame.
     */
    EventId m_assocResCmdWaitTimeout;

    /**
     * Scheduler event for a deferred MAC state change.
     */
    EventId m_setMacState;

    /**
     * Scheduler event for Interframe spacing wait time.
     */
    EventId m_ifsEvent;

    /**
     * Scheduler event for generation of one beacon.
     */
    EventId m_beaconEvent;

    /**
     * Scheduler event for the end of the outgoing superframe CAP.
     **/
    EventId m_capEvent;

    /**
     * Scheduler event for the end of the outgoing superframe CFP.
     */
    EventId m_cfpEvent;

    /**
     * Scheduler event for the end of the incoming superframe CAP.
     **/
    EventId m_incCapEvent;

    /**
     * Scheduler event for the end of the incoming superframe CFP.
     */
    EventId m_incCfpEvent;

    /**
     * Scheduler event to track the incoming beacons.
     */
    EventId m_trackingEvent;

    /**
     * Scheduler event for the end of an ACTIVE or PASSIVE channel scan.
     */
    EventId m_scanEvent;

    /**
     * Scheduler event for the end of an ORPHAN channel scan.
     */
    EventId m_scanOrphanEvent;

    /**
     * Scheduler event for the end of a ED channel scan.
     */
    EventId m_scanEnergyEvent;

    /**
     * The uniform random variable used in this mac layer
     */
    Ptr<UniformRandomVariable> m_uniformVar;
};
} // namespace lrwpan
} // namespace ns3

#endif /* LR_WPAN_MAC_H */
