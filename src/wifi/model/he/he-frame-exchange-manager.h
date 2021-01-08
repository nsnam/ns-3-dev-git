/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#include "ns3/vht-frame-exchange-manager.h"
#include <map>
#include <unordered_map>

namespace ns3 {

class MultiUserScheduler;
class ApWifiMac;
class StaWifiMac;
class CtrlTriggerHeader;

typedef std::unordered_map <uint16_t /* staId */, Ptr<WifiPsdu> /* PSDU */> WifiPsduMap;
typedef std::unordered_map <uint16_t /* staId */, Ptr<const WifiPsdu> /* PSDU */> WifiConstPsduMap;

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
  static TypeId GetTypeId (void);
  HeFrameExchangeManager ();
  virtual ~HeFrameExchangeManager ();

  // Overridden from VhtFrameExchangeManager
  virtual uint16_t GetSupportedBaBufferSize (void) const override;
  virtual bool StartFrameExchange (Ptr<QosTxop> edca, Time availableTime, bool initialFrame) override;
  virtual void SetWifiMac (const Ptr<RegularWifiMac> mac) override;
  virtual void CalculateAcknowledgmentTime (WifiAcknowledgment* acknowledgment) const override;

  /**
   * Set the Multi-user Scheduler associated with this Frame Exchange Manager.
   *
   * \param muScheduler the Multi-user Scheduler associated with this Frame Exchange Manager
   */
  void SetMultiUserScheduler (const Ptr<MultiUserScheduler> muScheduler);

  /**
   * Get the PSDU in the given PSDU map that is addressed to the given MAC address,
   * if any, or a null pointer, otherwise.
   *
   * \param to the MAC address
   * \param psduMap the PSDU map
   */
  static Ptr<WifiPsdu> GetPsduTo (Mac48Address to, const WifiPsduMap& psduMap);

  /**
   * Set the UL Target RSSI subfield of every User Info fields of the given
   * Trigger Frame to the most recent RSSI observed from the corresponding
   * station.
   *
   * \param trigger the given Trigger Frame
   */
  virtual void SetTargetRssi (CtrlTriggerHeader& trigger) const;

protected:
  virtual void DoDispose () override;

  // Overridden from VhtFrameExchangeManager
  virtual void ReceiveMpdu (Ptr<WifiMacQueueItem> mpdu, RxSignalInfo rxSignalInfo,
                            const WifiTxVector& txVector, bool inAmpdu) override;
  virtual void EndReceiveAmpdu (Ptr<const WifiPsdu> psdu, const RxSignalInfo& rxSignalInfo,
                                const WifiTxVector& txVector, const std::vector<bool>& perMpduStatus) override;
  virtual Time GetTxDuration (uint32_t ppduPayloadSize, Mac48Address receiver,
                              const WifiTxParameters& txParams) const override;
  virtual bool SendMpduFromBaManager (Ptr<QosTxop> edca, Time availableTime, bool initialFrame) override;

  /**
   * Send a map of PSDUs as a DL MU PPDU.
   * Note that both <i>psduMap</i> and <i>txParams</i> are moved to m_psduMap and
   * m_txParams, respectively, and hence are left in an undefined state.
   *
   * \param psduMap the map of PSDUs to send
   * \param txParams the TX parameters to use to transmit the PSDUs
   */
  void SendPsduMapWithProtection (WifiPsduMap psduMap, WifiTxParameters& txParams);

  /**
   * Forward a map of PSDUs down to the PHY layer.
   *
   * \param psduMap the map of PSDUs to transmit
   * \param txVector the TXVECTOR used to transmit the MU PPDU
   */
  void ForwardPsduMapDown (WifiConstPsduMap psduMap, WifiTxVector& txVector);

  /**
   * Take the necessary actions after that some BlockAck frames are missing
   * in response to a DL MU PPDU. This method must not be called if all the
   * expected BlockAck frames were received.
   *
   * \param psduMap a pointer to PSDU map transmitted in a DL MU PPDU
   * \param staMissedBlockAckFrom set of stations we missed a BlockAck frame from
   * \param nSolicitedStations the number of stations solicited to send a TB PPDU
   */
  virtual void BlockAcksInTbPpduTimeout (WifiPsduMap* psduMap,
                                         const std::set<Mac48Address>* staMissedBlockAckFrom,
                                         std::size_t nSolicitedStations);

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
  WifiTxVector GetHeTbTxVector (CtrlTriggerHeader trigger, Mac48Address triggerSender) const;

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
  Ptr<WifiMacQueueItem> PrepareMuBar (const WifiTxVector& responseTxVector,
                                      std::map<uint16_t, CtrlBAckRequestHeader> recipients) const;

  Ptr<ApWifiMac> m_apMac;                             //!< MAC pointer (null if not an AP)
  Ptr<StaWifiMac> m_staMac;                           //!< MAC pointer (null if not a STA)

private:
  /**
   * Send the current PSDU map as a DL MU PPDU.
   */
  void SendPsduMap (void);

  WifiPsduMap m_psduMap;                              //!< the A-MPDU being transmitted
  WifiTxParameters m_txParams;                        //!< the TX parameters for the current PPDU
  Ptr<MultiUserScheduler> m_muScheduler;              //!< Multi-user Scheduler (HE APs only)
  Ptr<WifiMacQueueItem> m_triggerFrame;               //!< Trigger Frame being sent
  std::set<Mac48Address> m_staExpectTbPpduFrom;       //!< set of stations expected to send a TB PPDU
  bool m_triggerFrameInAmpdu;                         //!< True if the received A-MPDU contains an MU-BAR
};

} //namespace ns3

#endif /* HE_FRAME_EXCHANGE_MANAGER_H */
