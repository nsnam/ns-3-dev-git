/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#ifndef MAC_LOW_H
#define MAC_LOW_H

#include <map>
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "channel-access-manager.h"
#include "block-ack-cache.h"
#include "mac-low-transmission-parameters.h"
#include "qos-utils.h"
#include "wifi-mac-header.h"
#include "wifi-tx-vector.h"
#include "block-ack-type.h"
#include "wifi-mpdu-type.h"

namespace ns3 {

class WifiMac;
class WifiPhy;
class Txop;
class QosTxop;
class WifiMacQueueItem;
class WifiMacQueue;
class WifiPsdu;
class BlockAckAgreement;
class MgtAddBaResponseHeader;
class WifiRemoteStationManager;
class CtrlBAckRequestHeader;
class CtrlBAckResponseHeader;
class MsduAggregator;
class MpduAggregator;

/**
 * \ingroup wifi
 * \brief handle RTS/CTS/Data/Ack transactions.
 */
class MacLow : public Object
{
public:

  /**
   * typedef for a callback for MacLowRx
   */
  typedef Callback<void, Ptr<WifiMacQueueItem>> MacLowRxCallback;

  MacLow ();
  virtual ~MacLow ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Set up WifiPhy associated with this MacLow.
   *
   * \param phy WifiPhy associated with this MacLow
   */
  void SetPhy (const Ptr<WifiPhy> phy);
  /**
   * \return current attached PHY device
   */
  Ptr<WifiPhy> GetPhy (void) const;
  /**
   * Remove WifiPhy associated with this MacLow.
   */
  void ResetPhy (void);
  /**
   * \param tid the Traffic ID
   * \return the QosTxop corresponding to the given TID
   */
  Ptr<QosTxop> GetEdca (uint8_t tid) const;
  /**
   * Set up WifiMac associated with this MacLow.
   *
   * \param mac WifiMac associated with this MacLow
   */
  void SetMac (const Ptr<WifiMac> mac);
  /**
   * Set up WifiRemoteStationManager associated with this MacLow.
   *
   * \param manager WifiRemoteStationManager associated with this MacLow
   */
  void SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> manager);
  /**
   * Set MAC address of this MacLow.
   *
   * \param ad Mac48Address of this MacLow
   */
  void SetAddress (Mac48Address ad);
  /**
   * Enable or disable CTS-to-self capability.
   *
   * \param enable Enable or disable CTS-to-self capability
   */
  void SetCtsToSelfSupported (bool enable);
  /**
   * \param interval the expected interval between two beacon transmissions.
   */
  void SetBeaconInterval (Time interval);
  /**
   * \param duration the maximum duration for the CF period.
   */
  void SetCfpMaxDuration (Time duration);
  /**
   * Set the Basic Service Set Identification.
   *
   * \param ad the BSSID
   */
  void SetBssid (Mac48Address ad);
  /**
   * Enable promiscuous mode.
   */
  void SetPromisc (void);
  /**
   * Return whether CTS-to-self capability is supported.
   *
   * \return true if CTS-to-self is supported, false otherwise
   */
  bool GetCtsToSelfSupported (void) const;
  /**
   * Return the MAC address of this MacLow.
   *
   * \return Mac48Address of this MacLow
   */
  Mac48Address GetAddress (void) const;
  /**
   * Return Short Interframe Space (SIFS) of this MacLow.
   *
   * \return SIFS
   */
  Time GetSifs (void) const;
  /**
   * Return slot duration of this MacLow.
   *
   * \return slot duration
   */
  Time GetSlotTime (void) const;
  /**
   * Return PCF Interframe Space (PIFS) of this MacLow.
   *
   * \return PIFS
   */
  Time GetPifs (void) const;
  /**
   * \return the expected interval between two beacon transmissions.
   */
  Time GetBeaconInterval (void) const;
  /**
   * \return the maximum duration for the CF period.
   */
  Time GetCfpMaxDuration (void) const;
  /**
   * \return the remaining duration for the CF period.
   */
  Time GetRemainingCfpDuration (void) const;
  /**
   * Return the Basic Service Set Identification.
   *
   * \return BSSID
   */
  Mac48Address GetBssid (void) const;
  /**
   * Check if MacLow is operating in promiscuous mode.
   *
   * \return true if MacLow is operating in promiscuous mode,
   *         false otherwise
   */
  bool IsPromisc (void) const;

  /**
   * \param callback the callback which receives every incoming packet.
   *
   * This callback typically forwards incoming packets to
   * an instance of ns3::MacRxMiddle.
   */
  void SetRxCallback (Callback<void, Ptr<WifiMacQueueItem>> callback);
  /**
   * \param channelAccessManager pointer to ChannelAccessManager in order to listen to NAV events
   *        for every incoming and outgoing packet.
   */
  void RegisterChannelAccessManager (Ptr<ChannelAccessManager> channelAccessManager);

  /**
   * Check whether the given MPDU, if transmitted according to the given TX vector,
   * meets the constraint on the maximum A-MPDU size (by assuming that the frame
   * has to be aggregated to an existing A-MPDU of the given size) and its
   * transmission time exceeds neither the max PPDU duration (depending on the
   * PPDU format) nor the given PPDU duration limit (if strictly positive).
   * The given MPDU needs to be a QoS Data frame.
   *
   * \param mpdu the MPDU.
   * \param txVector the TX vector used to transmit the MPDU
   * \param ampduSize the size of the existing A-MPDU in bytes, if any
   * \param ppduDurationLimit the limit on the PPDU duration
   * \returns true if constraints on size and duration limit are met.
   */
  bool IsWithinSizeAndTimeLimits (Ptr<const WifiMacQueueItem> mpdu, WifiTxVector txVector,
                                  uint32_t ampduSize, Time ppduDurationLimit);
  /**
   * Check whether an MPDU of the given size, destined to the given receiver and
   * belonging to the given TID, if transmitted according to the given TX vector,
   * meets the constraint on the maximum A-MPDU size (by assuming that the frame
   * has to be aggregated to an existing A-MPDU of the given size) and its
   * transmission time exceeds neither the max PPDU duration (depending on the
   * PPDU format) nor the given PPDU duration limit (if strictly positive).
   *
   * \param mpduSize the MPDU size.
   * \param receiver the receiver
   * \param tid the TID
   * \param txVector the TX vector used to transmit the MPDU
   * \param ampduSize the size of the existing A-MPDU in bytes, if any
   * \param ppduDurationLimit the limit on the PPDU duration
   * \returns true if constraints on size and duration limit are met.
   */
  bool IsWithinSizeAndTimeLimits (uint32_t mpduSize, Mac48Address receiver, uint8_t tid,
                                  WifiTxVector txVector, uint32_t ampduSize, Time ppduDurationLimit);
  /**
   * \param packet to send (does not include the 802.11 MAC header and checksum)
   * \param hdr header associated to the packet to send.
   * \param parameters transmission parameters of packet.
   * \return the transmission time that includes the time for the next packet transmission
   *
   * This transmission time includes the time required for
   * the next packet transmission if one was selected.
   */
  Time CalculateTransmissionTime (Ptr<const Packet> packet,
                                  const WifiMacHeader* hdr,
                                  const MacLowTransmissionParameters& parameters) const;

  /**
   * \param packet to send (does not include the 802.11 MAC header and checksum)
   * \param hdr header associated to the packet to send.
   * \param params transmission parameters of packet.
   * \param fragmentSize the packet fragment size (if fragmentation is used) in bytes
   * \return the transmission time that includes the time for the next packet transmission
   *
   * This transmission time does not include the time required for
   * the next packet transmission if one was selected.
   */
  Time CalculateOverallTxTime (Ptr<const Packet> packet,
                               const WifiMacHeader* hdr,
                               const MacLowTransmissionParameters& params,
                               uint32_t fragmentSize = 0) const;

  /**
   * \param item packet to send (does not include the 802.11 MAC header and checksum)
   * \param params transmission parameters of packet.
   * \return the transmission time that does not include the time required to transmit the frame
   *
   * This transmission time  only includes the time for the RTS/CTS exchange (if any)
   * and for the Ack frame (if any).
   */
  Time CalculateOverheadTxTime (Ptr<const WifiMacQueueItem> item,
                               const MacLowTransmissionParameters& params) const;

  /**
   * \param mpdu packet to send
   * \param parameters the transmission parameters to use for this packet.
   * \param txop pointer to the calling Txop.
   *
   * Start the transmission of the input packet and notify the listener
   * of transmission events.
   */
  virtual void StartTransmission (Ptr<WifiMacQueueItem> mpdu,
                                  MacLowTransmissionParameters parameters,
                                  Ptr<Txop> txop);

  /**
   * \param txVector the TXVECTOR decoded from PHY header.
   * \param psduDuration the duration of the PSDU that is about to be received.
   *
   * This method is typically invoked by the lower PHY layer to notify
   * the MAC layer that the reception of a PSDU is starting.
   * This is equivalent to the PHY-RXSTART primitive.
   * If the reception is correct for at least one MPDU of the PSDU
   * the DeaggregateAmpduAndReceive will be called after \p psduDuration.
   * Otherwise, ReceiveError will be called after the same duration.
   */
  void RxStartIndication (WifiTxVector txVector, Time psduDuration);

  /**
   * \param mpdu MPDU received
   * \param rxSnr snr of MPDU received in linear scale
   * \param txVector TXVECTOR of MPDU received
   * \param ampduSubframe true if this MPDU is part of an A-MPDU
   *
   * This method is typically invoked by the lower PHY layer to notify
   * the MAC layer that an MPDU was successfully received.
   */
  void ReceiveOk (Ptr<WifiMacQueueItem> mpdu, double rxSnr, WifiTxVector txVector, bool ampduSubframe);
  /**
   * \param psdu PSDU received.
   *
   * This method is typically invoked by the lower PHY layer to notify
   * the MAC layer that a PSDU was unsuccessfully received.
   */
  void ReceiveError (Ptr<WifiPsdu> psdu);
  /**
   * \param duration switching delay duration.
   *
   * This method is typically invoked by the PhyMacLowListener to notify
   * the MAC layer that a channel switching occurred. When a channel switching
   * occurs, pending MAC transmissions (RTS, CTS, Data and Ack) are cancelled.
   */
  void NotifySwitchingStartNow (Time duration);
  /**
   * This method is typically invoked by the PhyMacLowListener to notify
   * the MAC layer that the device has been put into sleep mode. When the device is put
   * into sleep mode, pending MAC transmissions (RTS, CTS, Data and Ack) are cancelled.
   */
  void NotifySleepNow (void);
  /**
   * This method is typically invoked by the PhyMacLowListener to notify
   * the MAC layer that the device has been put into off mode. When the device is put
   * into off mode, pending MAC transmissions (RTS, CTS, Data and Ack) are cancelled.
   */
  void NotifyOffNow (void);
  /**
   * \param respHdr Add block ack response from originator (action
   *        frame).
   * \param originator Address of peer station involved in block ack
   *        mechanism.
   * \param startingSeq Sequence number of the first MPDU of all
   *        packets for which block ack was negotiated.
   *
   * This function is typically invoked only by ns3::RegularWifiMac
   * when the STA (which may be non-AP in ESS, or in an IBSS) has
   * received an ADDBA Request frame and is transmitting an ADDBA
   * Response frame. At this point MacLow must allocate buffers to
   * collect all correctly received packets belonging to the category
   * for which block ack was negotiated.
   */
  void CreateBlockAckAgreement (const MgtAddBaResponseHeader *respHdr,
                                Mac48Address originator,
                                uint16_t startingSeq);
  /**
   * \param originator Address of peer participating in block ack mechanism.
   * \param tid TID for which block ack was created.
   *
   * Checks if exists an established block ack agreement with <i>originator</i>
   * for TID <i>tid</i>. If the agreement exists, tears down it. This function is typically
   * invoked when a DELBA frame is received from <i>originator</i>.
   */
  void DestroyBlockAckAgreement (Mac48Address originator, uint8_t tid);
  /**
   * \param ac Access class managed by the queue.
   * \param edca the QosTxop for the queue.
   *
   * The lifetime of the registered QosTxop is typically equal to the lifetime of the queue
   * associated to this AC.
   */
  void RegisterEdcaForAc (AcIndex ac, Ptr<QosTxop> edca);
  /**
   * \param aggregatedPacket which is the current A-MPDU
   * \param rxSnr SNR of packet received in linear scale
   * \param txVector TXVECTOR of packet received
   * \param statusPerMpdu reception status per MPDU
   *
   * This function de-aggregates an A-MPDU and decide if each MPDU is received correctly or not
   *
   */
  void DeaggregateAmpduAndReceive (Ptr<WifiPsdu> aggregatedPacket, double rxSnr, WifiTxVector txVector,
                                   std::vector<bool> statusPerMpdu);

  /**
   * Return a TXVECTOR for the Data frame given the destination.
   * The function consults WifiRemoteStationManager, which controls the rate
   * to different destinations.
   *
   * \param item the item being asked for TXVECTOR
   * \return TXVECTOR for the given item
   */
  virtual WifiTxVector GetDataTxVector (Ptr<const WifiMacQueueItem> item) const;
  /**
   * Start NAV with the given duration.
   *
   * \param duration the duration
   * \return true if NAV is reset
   */
  bool DoNavStartNow (Time duration);
  /**
   * This function indicates whether Simulator::Now is in the CF period.
   *
   * \return true if Simulator::Now is in CF period,
   *         false otherwise
   */
  virtual bool IsCfPeriod (void) const;
  /**
   * This function decides if a CF frame can be transmitted in the current CFP.
   *
   * \return true if a CF frame can be transmitted in the current CFP,
   *         false otherwise
   */
  bool CanTransmitNextCfFrame (void) const;

  /**
   * Returns the aggregator used to construct A-MSDU subframes.
   *
   * \return the aggregator used to construct A-MSDU subframes.
   */
  Ptr<MsduAggregator> GetMsduAggregator (void) const;
  /**
   * Returns the aggregator used to construct A-MPDU subframes.
   *
   * \return the aggregator used to construct A-MPDU subframes.
   */
  Ptr<MpduAggregator> GetMpduAggregator (void) const;

  /**
   * Set the aggregator used to construct A-MSDU subframes.
   *
   * \param aggr pointer to the MSDU aggregator.
   */
  void SetMsduAggregator (const Ptr<MsduAggregator> aggr);
  /**
   * Set the aggregator used to construct A-MPDU subframes.
   *
   * \param aggr pointer to the MPDU aggregator.
   */
  void SetMpduAggregator (const Ptr<MpduAggregator> aggr);


private:
  /**
   * Cancel all scheduled events. Called before beginning a transmission
   * or switching channel.
   */
  void CancelAllEvents (void);
  /**
   * Return the total CF-END size (including FCS trailer).
   *
   * \return the total CF-END size in bytes
   */
  uint32_t GetCfEndSize (void) const;
  /**
   * Forward a PSDU down to WifiPhy for transmission.
   *
   * \param psdu the PSDU
   * \param txVector the transmit vector
   */
  void ForwardDown (Ptr<const WifiPsdu> psdu, WifiTxVector txVector);
  /**
   * Return a TXVECTOR for the RTS frame given the destination.
   * The function consults WifiRemoteStationManager, which controls the rate
   * to different destinations.
   *
   * \param item the item being asked for RTS TXVECTOR
   * \return TXVECTOR for the RTS of the given item
   */
  WifiTxVector GetRtsTxVector (Ptr<const WifiMacQueueItem> item) const;
  /**
   * Return a TXVECTOR for the CTS frame given the destination and the mode of the RTS
   * used by the sender.
   * The function consults WifiRemoteStationManager, which controls the rate
   * to different destinations.
   *
   * \param to the MAC address of the CTS receiver
   * \param rtsTxMode the mode of the RTS used by the sender
   * \return TXVECTOR for the CTS
   */
  WifiTxVector GetCtsTxVector (Mac48Address to, WifiMode rtsTxMode) const;
  /**
   * Return a TXVECTOR for the Ack frame given the destination and the mode of the Data
   * used by the sender.
   * The function consults WifiRemoteStationManager, which controls the rate
   * to different destinations.
   *
   * \param to the MAC address of the Ack receiver
   * \param dataTxMode the mode of the Data used by the sender
   * \return TXVECTOR for the Ack
   */
  WifiTxVector GetAckTxVector (Mac48Address to, WifiMode dataTxMode) const;
  /**
   * Return a TXVECTOR for the BlockAck frame given the destination and the mode of the Data
   * used by the sender.
   * The function consults WifiRemoteStationManager, which controls the rate
   * to different destinations.
   *
   * \param to the MAC address of the BlockAck receiver
   * \param dataTxMode the mode of the Data used by the sender
   * \return TXVECTOR for the BlockAck
   */
  WifiTxVector GetBlockAckTxVector (Mac48Address to, WifiMode dataTxMode) const;
  /**
   * Return a TXVECTOR for the CTS frame given the destination and the mode of the RTS
   * used by the sender.
   * The function consults WifiRemoteStationManager, which controls the rate
   * to different destinations.
   *
   * \param to the MAC address of the CTS receiver
   * \param rtsTxMode the mode of the RTS used by the sender
   * \return TXVECTOR for the CTS
   */
  WifiTxVector GetCtsTxVectorForRts (Mac48Address to, WifiMode rtsTxMode) const;
  /**
   * Return a TXVECTOR for the BlockAck frame given the destination and the mode of the Data
   * used by the sender.
   * The function consults WifiRemoteStationManager, which controls the rate
   * to different destinations.
   *
   * \param to the MAC address of the BlockAck receiver
   * \param dataTxMode the mode of the Data used by the sender
   * \return TXVECTOR for the BlockAck
   */
  WifiTxVector GetAckTxVectorForData (Mac48Address to, WifiMode dataTxMode) const;
  /**
   * Get control answer mode function.
   *
   * \param reqMode request mode
   *
   * \return control answer mode
   */
  WifiMode GetControlAnswerMode (WifiMode reqMode) const;
  /**
   * Return the time required to transmit the CTS (including preamble and FCS).
   *
   * \param ctsTxVector the TXVECTOR used to transmit the CTS
   * \return the time required to transmit the CTS (including preamble and FCS)
   */
  Time GetCtsDuration (WifiTxVector ctsTxVector) const;
  /**
   * Return the time required to transmit the CTS to the specified address
   * given the TXVECTOR of the RTS (including preamble and FCS).
   *
   * \param to the receiver MAC address
   * \param rtsTxVector the TXVECTOR used to transmit the RTS
   * \return the time required to transmit the CTS (including preamble and FCS)
   */
  Time GetCtsDuration (Mac48Address to, WifiTxVector rtsTxVector) const;
  /**
   * Return the time required to transmit the Ack (including preamble and FCS).
   *
   * \param ackTxVector the TXVECTOR used to transmit the Ack
   * \return the time required to transmit the Ack (including preamble and FCS)
   */
  Time GetAckDuration (WifiTxVector ackTxVector) const;
  /**
   * Return the time required to transmit the Ack to the specified address
   * given the TXVECTOR of the Data (including preamble and FCS).
   *
   * \param to the receiver MAC address
   * \param dataTxVector the TXVECTOR used to transmit the Data
   * \return the time required to transmit the Ack (including preamble and FCS)
   */
  Time GetAckDuration (Mac48Address to, WifiTxVector dataTxVector) const;
  /**
   * Return the time required to transmit the BlockAck to the specified address
   * given the TXVECTOR of the BAR (including preamble and FCS).
   *
   * \param blockAckReqTxVector the TXVECTOR used to transmit the BAR
   * \param type the BlockAck type
   * \return the time required to transmit the BlockAck (including preamble and FCS)
   */
  Time GetBlockAckDuration (WifiTxVector blockAckReqTxVector, BlockAckType type) const;
  /**
   * Return the time required to transmit the BlockAckRequest to the specified address
   * given the TXVECTOR (including preamble and FCS).
   *
   * \param blockAckReqTxVector the TX vector used to transmit the BAR
   * \param type the BlockAckRequest type
   * \return the time required to transmit the BlockAckRequest (including preamble and FCS)
   */
  Time GetBlockAckRequestDuration (WifiTxVector blockAckReqTxVector, BlockAckType type) const;
  /**
   * Return the time required to transmit the response frames (Ack or BAR+BA
   * following the policy configured in the transmit parameters).
   *
   * \param params the transmission parameters
   * \param dataTxVector the TX vector used to transmit the data frame
   * \param receiver the station from which a response is expected
   * \return the time required to transmit the response (Ack or BAR+BA)
   */
  Time GetResponseDuration (const MacLowTransmissionParameters& params,
                            WifiTxVector dataTxVector, Mac48Address receiver) const;
  /**
   * Check if CTS-to-self mechanism should be used for the current packet.
   *
   * \return true if CTS-to-self mechanism should be used for the current packet,
   *         false otherwise
   */
  bool NeedCtsToSelf (void) const;

  /**
   * Notify NAV function.
   *
   * \param packet the packet
   * \param hdr the header
   */
  void NotifyNav (Ptr<const Packet> packet,const WifiMacHeader &hdr);
  /**
   * Reset NAV with the given duration.
   *
   * \param duration the duration to set
   */
  void DoNavResetNow (Time duration);
  /**
   * Check if NAV is zero.
   *
   * \return true if NAV is zero,
   *         false otherwise
   */
  bool IsNavZero (void) const;
  /**
   * Notify ChannelAccessManager that Ack timer should be started for the given duration.
   *
   * \param duration the duration of the timer
   */
  void NotifyAckTimeoutStartNow (Time duration);
  /**
   * Notify ChannelAccessManager that Ack timer should be reset.
   */
  void NotifyAckTimeoutResetNow (void);
  /**
   * Notify ChannelAccessManager that CTS timer should be started for the given duration.
   *
   * \param duration the duration of the timer
   */
  void NotifyCtsTimeoutStartNow (Time duration);
  /**
   * Notify ChannelAccessManager that CTS timer should be reset.
   */
  void NotifyCtsTimeoutResetNow (void);
  /* Event handlers */
  /**
   * Event handler when normal Ack timeout occurs.
   */
  void NormalAckTimeout (void);
  /**
   * Event handler when BlockAck timeout occurs.
   */
  void BlockAckTimeout (void);
  /**
   * Event handler when CTS timeout occurs.
   */
  void CtsTimeout (void);
  /**
   * Event handler when CF-Poll timeout occurs.
   */
  void CfPollTimeout (void);
  /**
   * Send CTS for a CTS-to-self mechanism.
   */
  void SendCtsToSelf (void);
  /**
   * Send CTS after receiving RTS.
   *
   * \param source the transmitter of the RTS
   * \param duration the NAV of the RTS
   * \param rtsTxVector the TXVECTOR used to transmit the RTS
   * \param rtsSnr the SNR of the RTS in linear scale
   */
  void SendCtsAfterRts (Mac48Address source, Time duration, WifiTxVector rtsTxVector, double rtsSnr);
  /**
   * Send Ack after receiving Data.
   *
   * \param source the transmitter of the Data
   * \param duration the NAV of the Data
   * \param dataTxMode the TXVECTOR used to transmit the Data
   * \param dataSnr the SNR of the Data in linear scale
   */
  void SendAckAfterData (Mac48Address source, Time duration, WifiMode dataTxMode, double dataSnr);
  /**
   * Send Data after receiving CTS.
   *
   * \param duration the NAV of the CTS
   */
  void SendDataAfterCts (Time duration);

  /**
   * Event handler that is usually scheduled to fired at the appropriate time
   * after completing transmissions.
   */
  void WaitIfsAfterEndTxFragment (void);
  /**
   * Event handler that is usually scheduled to fired at the appropriate time
   * after sending a packet.
   */
  void WaitIfsAfterEndTxPacket (void);

  /**
   * A transmission that does not require an Ack has completed.
   */
  void EndTxNoAck (void);
  /**
   * Send RTS to begin RTS-CTS-Data-Ack transaction.
   */
  void SendRtsForPacket (void);
  /**
   * Send Data packet, which can be Data-Ack or
   * RTS-CTS-Data-Ack transaction.
   */
  void SendDataPacket (void);
  /**
   * Start a Data timer by scheduling appropriate
   * Ack timeout.
   *
   * \param dataTxVector the TXVECTOR used to transmit the Data
   */
  void StartDataTxTimers (WifiTxVector dataTxVector);

  void DoDispose (void);

  /**
   * \param originator Address of peer participating in block ack mechanism.
   * \param tid TID for which block ack was created.
   * \param seq Starting sequence control
   *
   * This function forward up all completed "old" packets with sequence number
   * smaller than <i>seq</i>. All comparison are performed circularly modulo 4096.
   */
  void RxCompleteBufferedPacketsWithSmallerSequence (uint16_t seq, Mac48Address originator, uint8_t tid);
  /**
   * \param originator Address of peer participating in block ack mechanism.
   * \param tid TID for which block ack was created.
   *
   * This method is typically invoked when a MPDU with ack policy
   * subfield set to Normal Ack is received and a block ack agreement
   * for that packet exists.
   * This happens when the originator of block ack has only few MPDUs to send.
   * All completed MSDUs starting with starting sequence number of block ack
   * agreement are forward up to WifiMac until there is an incomplete or missing MSDU.
   * See section 9.10.4 in IEEE 802.11 standard for more details.
   */
  void RxCompleteBufferedPacketsUntilFirstLost (Mac48Address originator, uint8_t tid);
  /**
   * \param mpdu the MPDU
   * \returns true if MPDU received
   *
   * This method updates the reorder buffer and the scoreboard when an MPDU is received in an HT station
   * and stores the MPDU if needed when an MPDU is received in an non-HT Station (implements HT
   * immediate BlockAck)
   */
  bool ReceiveMpdu (Ptr<WifiMacQueueItem> mpdu);
  /**
   * \param mpdu the MPDU
   * \returns true if the MPDU stored
   *
   * This method checks if exists a valid established block ack agreement.
   * If there is, store the packet without pass it up to WifiMac. The packet is buffered
   * in order of increasing sequence control field. All comparison are performed
   * circularly modulo 2^12.
   */
  bool StoreMpduIfNeeded (Ptr<WifiMacQueueItem> mpdu);
  /**
   * Invoked after that a BlockAckRequest has been received. Looks for corresponding
   * block ack agreement and creates a block ack bitmap on a received packets basis.
   *
   * \param reqHdr the BAR header
   * \param originator the transmitter of the BAR
   * \param duration the NAV of the BAR
   * \param blockAckReqTxMode the TXVECTOR used to transmit the BAR
   * \param rxSnr the SNR of the BAR in linear scale
   */
  void SendBlockAckAfterBlockAckRequest (const CtrlBAckRequestHeader reqHdr, Mac48Address originator,
                                         Time duration, WifiMode blockAckReqTxMode, double rxSnr);
  /**
   * Invoked after an A-MPDU has been received. Looks for corresponding
   * block ack agreement and creates a block ack bitmap on a received packets basis.
   *
   * \param tid the Traffic ID
   * \param originator the originator MAC address
   * \param duration the remaining NAV duration
   * \param blockAckReqTxVector the transmit vector
   * \param rxSnr the receive SNR in linear scale
   */
  void SendBlockAckAfterAmpdu (uint8_t tid, Mac48Address originator, Time duration,
                               WifiTxVector blockAckReqTxVector, double rxSnr);
  /**
   * This method creates BlockAck frame with header equals to <i>blockAck</i> and start its transmission.
   *
   * \param blockAck the BA response header to send
   * \param originator the station to send a BA to
   * \param immediate use immediate BA policy if true
   * \param duration the NAV duration
   * \param blockAckReqTxMode the TXVECTOR used to transmit the BAR
   * \param rxSnr the received SNR in linear scale
   */
  void SendBlockAckResponse (const CtrlBAckResponseHeader* blockAck, Mac48Address originator, bool immediate,
                             Time duration, WifiMode blockAckReqTxMode, double rxSnr);
  /**
   * Every time that a BlockAckRequest or a packet with Ack Policy equals to <i>Block Ack</i>
   * are received, if a relative block ack agreement exists and the value of inactivity timeout
   * is not 0, the timer is reset.
   * see section 11.5.3 in IEEE 802.11e for more details.
   *
   * \param agreement the BA agreement
   */
  void ResetBlockAckInactivityTimerIfNeeded (BlockAckAgreement &agreement);

  /**
   * Set up WifiPhy listener for this MacLow.
   *
   * \param phy the WifiPhy this MacLow is connected to
   */
  void SetupPhyMacLowListener (const Ptr<WifiPhy> phy);
  /**
   * Remove current WifiPhy listener for this MacLow.
   *
   * \param phy the WifiPhy this MacLow is connected to
   */
  void RemovePhyMacLowListener (Ptr<WifiPhy> phy);

  Ptr<WifiPhy> m_phy; //!< Pointer to WifiPhy (actually send/receives frames)
  Ptr<WifiMac> m_mac; //!< Pointer to WifiMac (to fetch configuration)
  Ptr<WifiRemoteStationManager> m_stationManager; //!< Pointer to WifiRemoteStationManager (rate control)
  MacLowRxCallback m_rxCallback; //!< Callback to pass packet up

  /**
   * A struct that holds information about Ack piggybacking (CF-Ack).
   */
  struct CfAckInfo
  {
    bool appendCfAck;     //!< Flag used for PCF to indicate whether a CF-Ack should be appended
    bool expectCfAck;     //!< Flag used for PCF to indicate whether a CF-Ack should be expected
    Mac48Address address; //!< Address of the station to be acknowledged
  };

  /**
   * typedef for an iterator for a list of ChannelAccessManager.
   */
  typedef std::vector<Ptr<ChannelAccessManager> >::const_iterator ChannelAccessManagersCI;
  /**
   * typedef for a list of ChannelAccessManager.
   */
  typedef std::vector<Ptr<ChannelAccessManager> > ChannelAccessManagers;
  ChannelAccessManagers m_channelAccessManagers; //!< List of ChannelAccessManager

  Ptr<MsduAggregator> m_msduAggregator;             //!< A-MSDU aggregator
  Ptr<MpduAggregator> m_mpduAggregator;             //!< A-MPDU aggregator

  EventId m_normalAckTimeoutEvent;      //!< Normal Ack timeout event
  EventId m_blockAckTimeoutEvent;       //!< BlockAck timeout event
  EventId m_ctsTimeoutEvent;            //!< CTS timeout event
  EventId m_sendCtsEvent;               //!< Event to send CTS
  EventId m_sendAckEvent;               //!< Event to send Ack
  EventId m_sendDataEvent;              //!< Event to send Data
  EventId m_waitIfsEvent;               //!< Wait for IFS event
  EventId m_endTxNoAckEvent;            //!< Event for finishing transmission that does not require Ack
  EventId m_navCounterResetCtsMissed;   //!< Event to reset NAV when CTS is not received

  Ptr<WifiPsdu> m_currentPacket;            //!< Current packet transmitted/to be transmitted
  Ptr<Txop> m_currentTxop;                  //!< Current TXOP
  MacLowTransmissionParameters m_txParams;  //!< Transmission parameters of the current packet
  Mac48Address m_self;                      //!< Address of this MacLow (Mac48Address)
  Mac48Address m_bssid;                     //!< BSSID address (Mac48Address)

  Time m_beaconInterval;   //!< Expected interval between two beacon transmissions
  Time m_cfpMaxDuration;   //!< CFP max duration

  Time m_lastNavStart;     //!< The time when the latest NAV started
  Time m_lastNavDuration;  //!< The duration of the latest NAV

  Time m_cfpStart;          //!< The time when the latest CF period started
  Time m_lastBeacon;        //!< The time when the last beacon frame transmission started
  Time m_cfpForeshortening; //!< The delay the current CF period should be foreshortened

  bool m_promisc;  //!< Flag if the device is operating in promiscuous mode

  class PhyMacLowListener * m_phyMacLowListener; //!< Listener needed to monitor when a channel switching occurs.

  /*
   * BlockAck data structures.
   */
  typedef std::list<Ptr<WifiMacQueueItem>>::iterator BufferedPacketI; //!< buffered packet iterator typedef

  typedef std::pair<Mac48Address, uint8_t> AgreementKey; //!< agreement key typedef
  typedef std::pair<BlockAckAgreement, std::list<Ptr<WifiMacQueueItem>> > AgreementValue; //!< agreement value typedef

  typedef std::map<AgreementKey, AgreementValue> Agreements; //!< agreements
  typedef std::map<AgreementKey, AgreementValue>::iterator AgreementsI; //!< agreements iterator

  typedef std::map<AgreementKey, BlockAckCache> BlockAckCaches; //!< block ack caches typedef
  typedef std::map<AgreementKey, BlockAckCache>::iterator BlockAckCachesI; //!< block ack caches iterator typedef

  Agreements m_bAckAgreements; //!< block ack agreements
  BlockAckCaches m_bAckCaches; //!< block ack caches

  typedef std::map<AcIndex, Ptr<QosTxop> > QueueEdcas; //!< EDCA queues typedef
  QueueEdcas m_edca; //!< EDCA queues

  bool m_ctsToSelfSupported;             //!< Flag whether CTS-to-self is supported
  WifiTxVector m_currentTxVector;        //!< TXVECTOR used for the current packet transmission

  CfAckInfo m_cfAckInfo; //!< Info about piggyback Acks used in PCF
};

} //namespace ns3

#endif /* MAC_LOW_H */
