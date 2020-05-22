/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef TXOP_H
#define TXOP_H

#include "ns3/traced-value.h"
#include "mac-low-transmission-parameters.h"
#include "wifi-mac-header.h"

namespace ns3 {

class Packet;
class ChannelAccessManager;
class MacTxMiddle;
class MacLow;
class WifiMode;
class WifiMacQueue;
class WifiMacQueueItem;
class UniformRandomVariable;
class CtrlBAckResponseHeader;
class WifiRemoteStationManager;
class WifiTxVector;

/**
 * \brief Handle packet fragmentation and retransmissions
 * for data and management frames.
 * \ingroup wifi
 *
 * This class implements the packet fragmentation and
 * retransmission policy for data and management frames.
 * It uses the ns3::MacLow and ns3::ChannelAccessManager helper
 * classes to respectively send packets and decide when
 * to send them. Packets are stored in a ns3::WifiMacQueue
 * until they can be sent.
 *
 * The policy currently implemented uses a simple fragmentation
 * threshold: any packet bigger than this threshold is fragmented
 * in fragments whose size is smaller than the threshold.
 *
 * The retransmission policy is also very simple: every packet is
 * retransmitted until it is either successfully transmitted or
 * it has been retransmitted up until the SSRC or SLRC thresholds.
 *
 * The RTS/CTS policy is similar to the fragmentation policy: when
 * a packet is bigger than a threshold, the RTS/CTS protocol is used.
 */

class Txop : public Object
{
public:
  /// allow MacLowTransmissionListener class access
  friend class MacLowTransmissionListener;

  Txop ();
  virtual ~Txop ();

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * typedef for a callback to invoke when a
   * packet transmission was completed successfully.
   */
  typedef Callback <void, const WifiMacHeader&> TxOk;
  /**
   * typedef for a callback to invoke when a
   * packet transmission was failed.
   */
  typedef Callback <void, const WifiMacHeader&> TxFailed;
  /**
   * typedef for a callback to invoke when a
   * packet is dropped.
   */
  typedef Callback <void, Ptr<const Packet> > TxDropped;

  /**
   * Check for QoS TXOP.
   *
   * \returns true if QoS TXOP.
   */
  virtual bool IsQosTxop () const;

  /**
   * Set MacLow associated with this Txop.
   *
   * \param low MacLow to associate.
   */
  void SetMacLow (const Ptr<MacLow> low);
  /**
   * Set ChannelAccessManager this Txop is associated to.
   *
   * \param manager ChannelAccessManager to associate.
   */
  void SetChannelAccessManager (const Ptr<ChannelAccessManager> manager);
  /**
   * Set WifiRemoteStationsManager this Txop is associated to.
   *
   * \param remoteManager WifiRemoteStationManager to associate.
   */
  virtual void SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> remoteManager);
  /**
   * Set MacTxMiddle this Txop is associated to.
   *
   * \param txMiddle MacTxMiddle to associate.
   */
  void SetTxMiddle (const Ptr<MacTxMiddle> txMiddle);

  /**
   * \param callback the callback to invoke when a
   * packet transmission was completed successfully.
   */
  void SetTxOkCallback (TxOk callback);
  /**
   * \param callback the callback to invoke when a
   *        packet transmission was completed unsuccessfully.
   */
  void SetTxFailedCallback (TxFailed callback);
  /**
   * \param callback the callback to invoke when a
   *        packet is dropped.
   */
  void SetTxDroppedCallback (TxDropped callback);

  /**
   * Return the MacLow associated with this Txop.
   *
   * \return the associated MacLow
   */
  Ptr<MacLow> GetLow (void) const;

  /**
   * Return the packet queue associated with this Txop.
   *
   * \return the associated WifiMacQueue
   */
  Ptr<WifiMacQueue > GetWifiMacQueue () const;

  /**
   * Set the minimum contention window size.
   *
   * \param minCw the minimum contention window size.
   */
  void SetMinCw (uint32_t minCw);
  /**
   * Set the maximum contention window size.
   *
   * \param maxCw the maximum contention window size.
   */
  void SetMaxCw (uint32_t maxCw);
  /**
   * Set the number of slots that make up an AIFS.
   *
   * \param aifsn the number of slots that make up an AIFS.
   */
  void SetAifsn (uint8_t aifsn);
  /**
   * Set the TXOP limit.
   *
   * \param txopLimit the TXOP limit.
   *        Value zero corresponds to default Txop.
   */
  void SetTxopLimit (Time txopLimit);
  /**
   * Return the minimum contention window size.
   *
   * \return the minimum contention window size.
   */
  uint32_t GetMinCw (void) const;
  /**
   * Return the maximum contention window size.
   *
   * \return the maximum contention window size.
   */
  uint32_t GetMaxCw (void) const;
  /**
   * Return the number of slots that make up an AIFS.
   *
   * \return the number of slots that make up an AIFS.
   */
  uint8_t GetAifsn (void) const;
  /**
   * Return the TXOP limit.
   *
   * \return the TXOP limit.
   */
  Time GetTxopLimit (void) const;

  /**
   * When a channel switching occurs, enqueued packets are removed.
   */
  virtual void NotifyChannelSwitching (void);
  /**
   * When sleep operation occurs, if there is a pending packet transmission,
   * it will be reinserted to the front of the queue.
   */
  virtual void NotifySleep (void);
  /**
   * When off operation occurs, the queue gets cleaned up.
   */
  virtual void NotifyOff (void);
  /**
   * When wake up operation occurs, channel access will be restarted.
   */
  virtual void NotifyWakeUp (void);
  /**
   * When on operation occurs, channel access will be started.
   */
  virtual void NotifyOn (void);

  /* Event handlers */
  /**
   * \param packet packet to send.
   * \param hdr header of packet to send.
   *
   * Store the packet in the internal queue until it
   * can be sent safely.
   */
  virtual void Queue (Ptr<Packet> packet, const WifiMacHeader &hdr);

  /**
   * Sends CF frame to STA with address <i>addr</i>.
   *
   * \param frameType the type of frame to be transmitted.
   * \param addr address of the recipient.
   */
  void SendCfFrame (WifiMacType frameType, Mac48Address addr);

  /* Event handlers */
  /**
   * Event handler when a CTS timeout has occurred.
   */
  virtual void MissedCts (void);
  /**
   * Event handler when an Ack is received.
   */
  virtual void GotAck (void);
  /**
   * Event handler when an Ack is missed.
   */
  virtual void MissedAck (void);
  /**
   * Event handler when a CF-END frame is received.
   */
  void GotCfEnd (void);
  /**
   * Event handler when a response to a CF-POLL frame is missed.
   *
   * \param expectedCfAck flag to indicate whether a CF-Ack was expected in the response.
   */
  void MissedCfPollResponse (bool expectedCfAck);
  /**
   * Event handler when a BlockAck is received.
   *
   * \param blockAck BlockAck header.
   * \param recipient address of the recipient.
   * \param rxSnr SNR of the BlockAck itself in linear scale.
   * \param dataSnr reported data SNR from the peer in linear scale.
   * \param dataTxVector TXVECTOR used to send the Data.
   */
  virtual void GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient,
                            double rxSnr, double dataSnr, WifiTxVector dataTxVector);
  /**
   * Event handler when a BlockAck timeout has occurred.
   * \param nMpdus the number of MPDUs sent in the A-MPDU transmission that results in a BlockAck timeout.
   */
  virtual void MissedBlockAck (uint8_t nMpdus);

  /**
   * Start transmission for the next fragment.
   * This is called for fragment only.
   */
  virtual void StartNextFragment (void);
  /**
   * Cancel the transmission.
   */
  virtual void Cancel (void);
  /**
   * Start transmission for the next packet if allowed by the TxopLimit.
   */
  virtual void StartNextPacket (void);
  /**
   * Event handler when a transmission that
   * does not require an Ack has completed.
   */
  virtual void EndTxNoAck (void);

  /**
   * Return the remaining duration in the current TXOP.
   *
   * \return the remaining duration in the current TXOP.
   */
  virtual Time GetTxopRemaining (void) const;
  /**
   * Update backoff and restart access if needed.
   */
  virtual void TerminateTxop (void);

  /**
   * Check if the next PCF transmission can fit in the remaining CFP duration.
   *
   * \return true if the next PCF transmission can fit in the remaining CFP duration,
   *         false otherwise
   */
  bool CanStartNextPolling (void) const;


  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use.
   *
   * \return the number of stream indices assigned by this model.
   */
  int64_t AssignStreams (int64_t stream);

  /**
   * \returns true if access has been requested for this function and
   *          has not been granted already, false otherwise.
   */
  virtual bool IsAccessRequested (void) const;

  /**
   * \param nSlots the number of slots of the backoff.
   *
   * Start a backoff by initializing the backoff counter to the number of
   * slots specified.
   */
  void StartBackoffNow (uint32_t nSlots);

protected:
  ///< ChannelAccessManager associated class
  friend class ChannelAccessManager;

  virtual void DoDispose (void);
  virtual void DoInitialize (void);

  /* Txop notifications forwarded here */
  /**
   * Notify that access request has been received.
   */
  virtual void NotifyAccessRequested (void);
  /**
   * Notify the Txop that access has been granted.
   */
  virtual void NotifyAccessGranted (void);
  /**
   * Notify the Txop that internal collision has occurred.
   */
  virtual void NotifyInternalCollision (void);

  /**
   * Check if the Txop has frames to transmit.
   * \return true if the Txop has frames to transmit.
   */
  virtual bool HasFramesToTransmit (void);
  /**
   * Generate a new backoff now.
   */
  virtual void GenerateBackoff (void);
  /**
   * Restart access request if needed.
   */
  virtual void RestartAccessIfNeeded (void);
  /**
   * Request access from Txop if needed.
   */
  virtual void StartAccessIfNeeded (void);

  /**
   * \returns the current value of the CW variable. The initial value is
   *          minCW.
   */
  uint32_t GetCw (void) const;
  /**
   * Update the value of the CW variable to take into account
   * a transmission success or a transmission abort (stop transmission
   * of a packet after the maximum number of retransmissions has been
   * reached). By default, this resets the CW variable to minCW.
   */
  void ResetCw (void);
  /**
   * Update the value of the CW variable to take into account
   * a transmission failure. By default, this triggers a doubling
   * of CW (capped by maxCW).
   */
  void UpdateFailedCw (void);
  /**
   * Return the current number of backoff slots.
   *
   * \return the current number of backoff slots
   */
  uint32_t GetBackoffSlots (void) const;
  /**
   * Return the time when the backoff procedure started.
   *
   * \return the time when the backoff procedure started
   */
  Time GetBackoffStart (void) const;
  /**
   * Update backoff slots that nSlots has passed.
   *
   * \param nSlots the number of slots to decrement
   * \param backoffUpdateBound the time at which backoff should start
   */
  void UpdateBackoffSlotsNow (uint32_t nSlots, Time backoffUpdateBound);

  /**
   * Check if RTS should be re-transmitted if CTS was missed.
   *
   * \param packet current packet being transmitted.
   * \param hdr current header being transmitted.
   * \return true if RTS should be re-transmitted,
   *         false otherwise.
   */
  bool NeedRtsRetransmission (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * Check if Data should be re-transmitted if Ack was missed.
   *
   * \param packet current packet being transmitted.
   * \param hdr current header being transmitted.
   * \return true if Data should be re-transmitted,
   *         false otherwise.
   */
  bool NeedDataRetransmission (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * Check if the current packet should be fragmented.
   *
   * \return true if the current packet should be fragmented,
   *         false otherwise
   */
  virtual bool NeedFragmentation (void) const;

  /**
   * Continue to the next fragment. This method simply
   * increments the internal variable that keep track
   * of the current fragment number.
   */
  void NextFragment (void);
  /**
   * Get the next fragment from the packet with
   * appropriate Wifi header for the fragment.
   *
   * \param hdr Wi-Fi header.
   *
   * \return the fragment with the current fragment number.
   */
  virtual Ptr<Packet> GetFragmentPacket (WifiMacHeader *hdr);
  /**
   * Calculate the size of the next fragment.
   *
   * \return the size of the next fragment in bytes.
   */
  virtual uint32_t GetNextFragmentSize (void) const;
  /**
   * Calculate the size of the current fragment.
   *
   * \return the size of the current fragment in bytes.
   */
  virtual uint32_t GetFragmentSize (void) const;
  /**
   * Calculate the offset for the current fragment.
   *
   * \return the offset for the current fragment in bytes.
   */
  virtual uint32_t GetFragmentOffset (void) const;
  /**
   * Check if the current fragment is the last fragment.
   *
   * \return true if the current fragment is the last fragment,
   *         false otherwise.
   */
  virtual bool IsLastFragment (void) const;
  /**
   *
   * Pass the packet included in the wifi MAC queue item to the
   * packet dropped callback.
   *
   * \param item the wifi MAC queue item.
   */
  void TxDroppedPacket (Ptr<const WifiMacQueueItem> item);

  Ptr<ChannelAccessManager> m_channelAccessManager; //!< the channel access manager
  TxOk m_txOkCallback;                              //!< the transmit OK callback
  TxFailed m_txFailedCallback;                      //!< the transmit failed callback
  TxDropped m_txDroppedCallback;                    //!< the packet dropped callback
  Ptr<WifiMacQueue> m_queue;                        //!< the wifi MAC queue
  Ptr<MacTxMiddle> m_txMiddle;                      //!< the MacTxMiddle
  Ptr <MacLow> m_low;                               //!< the MacLow
  Ptr<WifiRemoteStationManager> m_stationManager;   //!< the wifi remote station manager
  Ptr<UniformRandomVariable> m_rng;                 //!< the random stream

  uint32_t m_cwMin;        //!< the minimum contention window
  uint32_t m_cwMax;        //!< the maximum contention window
  uint32_t m_cw;           //!< the current contention window
  uint32_t m_backoff;      //!< the current backoff
  bool m_accessRequested;  //!< flag whether channel access is already requested
  uint32_t m_backoffSlots; //!< the number of backoff slots
  /**
   * the backoffStart variable is used to keep track of the
   * time at which a backoff was started or the time at which
   * the backoff counter was last updated.
   */
  Time m_backoffStart;

  uint8_t m_aifsn;        //!< the AIFSN
  Time m_txopLimit;       //!< the TXOP limit time

  Ptr<const Packet> m_currentPacket;            //!< the current packet
  WifiMacHeader m_currentHdr;                   //!< the current header
  MacLowTransmissionParameters m_currentParams; //!< current transmission parameters
  uint8_t m_fragmentNumber;                     //!< the fragment number
  TracedCallback<uint32_t> m_backoffTrace;      //!< backoff trace value
  TracedValue<uint32_t> m_cwTrace;              //!< CW trace value
};

} //namespace ns3

#endif /* TXOP_H */
