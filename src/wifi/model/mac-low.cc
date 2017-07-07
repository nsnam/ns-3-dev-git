/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/socket.h"
#include "mac-low.h"
#include "edca-txop-n.h"
#include "wifi-mac-trailer.h"
#include "snr-tag.h"
#include "ampdu-tag.h"
#include "wifi-mac-queue.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[mac=" << m_self << "] "

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MacLow");

MacLowTransmissionParameters::MacLowTransmissionParameters ()
  : m_nextSize (0),
    m_waitAck (ACK_NONE),
    m_sendRts (false),
    m_overrideDurationId (Seconds (0))
{
}
void
MacLowTransmissionParameters::EnableNextData (uint32_t size)
{
  m_nextSize = size;
}
void
MacLowTransmissionParameters::DisableNextData (void)
{
  m_nextSize = 0;
}
void
MacLowTransmissionParameters::EnableOverrideDurationId (Time durationId)
{
  m_overrideDurationId = durationId;
}
void
MacLowTransmissionParameters::DisableOverrideDurationId (void)
{
  m_overrideDurationId = Seconds (0);
}
void
MacLowTransmissionParameters::EnableSuperFastAck (void)
{
  m_waitAck = ACK_SUPER_FAST;
}
void
MacLowTransmissionParameters::EnableBasicBlockAck (void)
{
  m_waitAck = BLOCK_ACK_BASIC;
}
void
MacLowTransmissionParameters::EnableCompressedBlockAck (void)
{
  m_waitAck = BLOCK_ACK_COMPRESSED;
}
void
MacLowTransmissionParameters::EnableMultiTidBlockAck (void)
{
  m_waitAck = BLOCK_ACK_MULTI_TID;
}
void
MacLowTransmissionParameters::EnableFastAck (void)
{
  m_waitAck = ACK_FAST;
}
void
MacLowTransmissionParameters::EnableAck (void)
{
  m_waitAck = ACK_NORMAL;
}
void
MacLowTransmissionParameters::DisableAck (void)
{
  m_waitAck = ACK_NONE;
}
void
MacLowTransmissionParameters::EnableRts (void)
{
  m_sendRts = true;
}
void
MacLowTransmissionParameters::DisableRts (void)
{
  m_sendRts = false;
}
bool
MacLowTransmissionParameters::MustWaitAck (void) const
{
  return (m_waitAck != ACK_NONE);
}
bool
MacLowTransmissionParameters::MustWaitNormalAck (void) const
{
  return (m_waitAck == ACK_NORMAL);
}
bool
MacLowTransmissionParameters::MustWaitFastAck (void) const
{
  return (m_waitAck == ACK_FAST);
}
bool
MacLowTransmissionParameters::MustWaitSuperFastAck (void) const
{
  return (m_waitAck == ACK_SUPER_FAST);
}
bool
MacLowTransmissionParameters::MustWaitBasicBlockAck (void) const
{
  return (m_waitAck == BLOCK_ACK_BASIC) ? true : false;
}
bool
MacLowTransmissionParameters::MustWaitCompressedBlockAck (void) const
{
  return (m_waitAck == BLOCK_ACK_COMPRESSED) ? true : false;
}
bool
MacLowTransmissionParameters::MustWaitMultiTidBlockAck (void) const
{
  return (m_waitAck == BLOCK_ACK_MULTI_TID) ? true : false;
}
bool
MacLowTransmissionParameters::MustSendRts (void) const
{
  return m_sendRts;
}
bool
MacLowTransmissionParameters::HasDurationId (void) const
{
  return (!m_overrideDurationId.IsZero ());
}
Time
MacLowTransmissionParameters::GetDurationId (void) const
{
  NS_ASSERT (!m_overrideDurationId.IsZero ());
  return m_overrideDurationId;
}
bool
MacLowTransmissionParameters::HasNextPacket (void) const
{
  return (m_nextSize != 0);
}
uint32_t
MacLowTransmissionParameters::GetNextPacketSize (void) const
{
  NS_ASSERT (HasNextPacket ());
  return m_nextSize;
}

std::ostream &operator << (std::ostream &os, const MacLowTransmissionParameters &params)
{
  os << "["
     << "send rts=" << params.m_sendRts << ", "
     << "next size=" << params.m_nextSize << ", "
     << "dur=" << params.m_overrideDurationId << ", "
     << "ack=";
  switch (params.m_waitAck)
    {
    case MacLowTransmissionParameters::ACK_NONE:
      os << "none";
      break;
    case MacLowTransmissionParameters::ACK_NORMAL:
      os << "normal";
      break;
    case MacLowTransmissionParameters::ACK_FAST:
      os << "fast";
      break;
    case MacLowTransmissionParameters::ACK_SUPER_FAST:
      os << "super-fast";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_BASIC:
      os << "basic-block-ack";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_COMPRESSED:
      os << "compressed-block-ack";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_MULTI_TID:
      os << "multi-tid-block-ack";
      break;
    }
  os << "]";
  return os;
}


/**
 * Listener for PHY events. Forwards to MacLow
 */
class PhyMacLowListener : public ns3::WifiPhyListener
{
public:
  /**
   * Create a PhyMacLowListener for the given MacLow.
   *
   * \param macLow
   */
  PhyMacLowListener (ns3::MacLow *macLow)
    : m_macLow (macLow)
  {
  }
  virtual ~PhyMacLowListener ()
  {
  }
  void NotifyRxStart (Time duration)
  {
  }
  void NotifyRxEndOk (void)
  {
  }
  void NotifyRxEndError (void)
  {
  }
  void NotifyTxStart (Time duration, double txPowerDbm)
  {
  }
  void NotifyMaybeCcaBusyStart (Time duration)
  {
  }
  void NotifySwitchingStart (Time duration)
  {
    m_macLow->NotifySwitchingStartNow (duration);
  }
  void NotifySleep (void)
  {
    m_macLow->NotifySleepNow ();
  }
  void NotifyWakeup (void)
  {
  }

private:
  ns3::MacLow *m_macLow; ///< the MAC
};


MacLow::MacLow ()
  : m_normalAckTimeoutEvent (),
    m_fastAckTimeoutEvent (),
    m_superFastAckTimeoutEvent (),
    m_fastAckFailedTimeoutEvent (),
    m_blockAckTimeoutEvent (),
    m_ctsTimeoutEvent (),
    m_sendCtsEvent (),
    m_sendAckEvent (),
    m_sendDataEvent (),
    m_waitIfsEvent (),
    m_endTxNoAckEvent (),
    m_currentPacket (0),
    m_currentDca (0),
    m_lastNavStart (Seconds (0)),
    m_lastNavDuration (Seconds (0)),
    m_promisc (false),
    m_ampdu (false),
    m_phyMacLowListener (0),
    m_ctsToSelfSupported (false)
{
  NS_LOG_FUNCTION (this);
  for (uint8_t i = 0; i < 8; i++)
    {
      m_aggregateQueue[i] = CreateObject<WifiMacQueue> ();
    }
}

MacLow::~MacLow ()
{
  NS_LOG_FUNCTION (this);
}

/* static */
TypeId
MacLow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MacLow")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<MacLow> ()
  ;
  return tid;
}

void
MacLow::SetupPhyMacLowListener (const Ptr<WifiPhy> phy)
{
  m_phyMacLowListener = new PhyMacLowListener (this);
  phy->RegisterListener (m_phyMacLowListener);
}

void
MacLow::RemovePhyMacLowListener (Ptr<WifiPhy> phy)
{
  if (m_phyMacLowListener != 0 )
    {
      phy->UnregisterListener (m_phyMacLowListener);
      delete m_phyMacLowListener;
      m_phyMacLowListener = 0;
    }
}

void
MacLow::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_normalAckTimeoutEvent.Cancel ();
  m_fastAckTimeoutEvent.Cancel ();
  m_superFastAckTimeoutEvent.Cancel ();
  m_fastAckFailedTimeoutEvent.Cancel ();
  m_blockAckTimeoutEvent.Cancel ();
  m_ctsTimeoutEvent.Cancel ();
  m_sendCtsEvent.Cancel ();
  m_sendAckEvent.Cancel ();
  m_sendDataEvent.Cancel ();
  m_waitIfsEvent.Cancel ();
  m_endTxNoAckEvent.Cancel ();
  m_phy = 0;
  m_stationManager = 0;
  if (m_phyMacLowListener != 0)
    {
      delete m_phyMacLowListener;
      m_phyMacLowListener = 0;
    }
  for (uint8_t i = 0; i < 8; i++)
    {
      m_aggregateQueue[i] = 0;
    }
  m_ampdu = false;
}

void
MacLow::CancelAllEvents (void)
{
  NS_LOG_FUNCTION (this);
  bool oneRunning = false;
  if (m_normalAckTimeoutEvent.IsRunning ())
    {
      m_normalAckTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_fastAckTimeoutEvent.IsRunning ())
    {
      m_fastAckTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_superFastAckTimeoutEvent.IsRunning ())
    {
      m_superFastAckTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_fastAckFailedTimeoutEvent.IsRunning ())
    {
      m_fastAckFailedTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_blockAckTimeoutEvent.IsRunning ())
    {
      m_blockAckTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_ctsTimeoutEvent.IsRunning ())
    {
      m_ctsTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_sendCtsEvent.IsRunning ())
    {
      m_sendCtsEvent.Cancel ();
      oneRunning = true;
    }
  if (m_sendAckEvent.IsRunning ())
    {
      m_sendAckEvent.Cancel ();
      oneRunning = true;
    }
  if (m_sendDataEvent.IsRunning ())
    {
      m_sendDataEvent.Cancel ();
      oneRunning = true;
    }
  if (m_waitIfsEvent.IsRunning ())
    {
      m_waitIfsEvent.Cancel ();
      oneRunning = true;
    }
  if (m_endTxNoAckEvent.IsRunning ())
    {
      m_endTxNoAckEvent.Cancel ();
      oneRunning = true;
    }
  if (oneRunning && m_currentDca != 0)
    {
      m_currentDca->Cancel ();
      m_currentDca = 0;
    }
}

void
MacLow::SetPhy (const Ptr<WifiPhy> phy)
{
  m_phy = phy;
  m_phy->SetReceiveOkCallback (MakeCallback (&MacLow::DeaggregateAmpduAndReceive, this));
  m_phy->SetReceiveErrorCallback (MakeCallback (&MacLow::ReceiveError, this));
  SetupPhyMacLowListener (phy);
}

Ptr<WifiPhy>
MacLow::GetPhy (void) const
{
  return m_phy;
}

void
MacLow::ResetPhy (void)
{
  m_phy->SetReceiveOkCallback (MakeNullCallback<void, Ptr<Packet>, double, WifiTxVector> ());
  m_phy->SetReceiveErrorCallback (MakeNullCallback<void, Ptr<Packet>, double> ());
  RemovePhyMacLowListener (m_phy);
  m_phy = 0;
}

void
MacLow::SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> manager)
{
  m_stationManager = manager;
}

void
MacLow::SetAddress (Mac48Address ad)
{
  m_self = ad;
}

void
MacLow::SetAckTimeout (Time ackTimeout)
{
  m_ackTimeout = ackTimeout;
}

void
MacLow::SetBasicBlockAckTimeout (Time blockAckTimeout)
{
  m_basicBlockAckTimeout = blockAckTimeout;
}

void
MacLow::SetCompressedBlockAckTimeout (Time blockAckTimeout)
{
  m_compressedBlockAckTimeout = blockAckTimeout;
}

void
MacLow::SetCtsToSelfSupported (bool enable)
{
  m_ctsToSelfSupported = enable;
}

bool
MacLow::GetCtsToSelfSupported () const
{
  return m_ctsToSelfSupported;
}

void
MacLow::SetCtsTimeout (Time ctsTimeout)
{
  m_ctsTimeout = ctsTimeout;
}

void
MacLow::SetSifs (Time sifs)
{
  m_sifs = sifs;
}

void
MacLow::SetSlotTime (Time slotTime)
{
  m_slotTime = slotTime;
}

void
MacLow::SetPifs (Time pifs)
{
  m_pifs = pifs;
}

void
MacLow::SetRifs (Time rifs)
{
  m_rifs = rifs;
}

void
MacLow::SetBssid (Mac48Address bssid)
{
  m_bssid = bssid;
}

void
MacLow::SetPromisc (void)
{
  m_promisc = true;
}

Mac48Address
MacLow::GetAddress (void) const
{
  return m_self;
}

Time
MacLow::GetAckTimeout (void) const
{
  return m_ackTimeout;
}

Time
MacLow::GetBasicBlockAckTimeout () const
{
  return m_basicBlockAckTimeout;
}

Time
MacLow::GetCompressedBlockAckTimeout () const
{
  return m_compressedBlockAckTimeout;
}

Time
MacLow::GetCtsTimeout (void) const
{
  return m_ctsTimeout;
}

Time
MacLow::GetSifs (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sifs;
}

Time
MacLow::GetRifs (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rifs;
}

Time
MacLow::GetSlotTime (void) const
{
  return m_slotTime;
}

Time
MacLow::GetPifs (void) const
{
  return m_pifs;
}

Mac48Address
MacLow::GetBssid (void) const
{
  return m_bssid;
}

bool
MacLow::IsPromisc (void) const
{
  return m_promisc;
}

void
MacLow::SetRxCallback (Callback<void, Ptr<Packet>, const WifiMacHeader *> callback)
{
  m_rxCallback = callback;
}

void
MacLow::RegisterDcf (Ptr<DcfManager> dcf)
{
  m_dcfManagers.push_back (dcf);
}

bool
MacLow::IsAmpdu (Ptr<const Packet> packet, const WifiMacHeader hdr)
{
  uint32_t size, actualSize;
  WifiMacTrailer fcs;
  size = packet->GetSize () + hdr.GetSize () + fcs.GetSerializedSize ();
  Ptr<Packet> p = AggregateToAmpdu (packet, hdr);
  actualSize = p->GetSize ();
  if (actualSize > size)
    {
      m_currentPacket = p;
      return true;
    }
  else
    {
      return false;
    }
}

void
MacLow::StartTransmission (Ptr<const Packet> packet,
                           const WifiMacHeader* hdr,
                           MacLowTransmissionParameters params,
                           Ptr<DcaTxop> dca)
{
  NS_LOG_FUNCTION (this << packet << hdr << params << dca);
  /* m_currentPacket is not NULL because someone started
   * a transmission and was interrupted before one of:
   *   - ctsTimeout
   *   - sendDataAfterCTS
   * expired. This means that one of these timers is still
   * running. They are all cancelled below anyway by the
   * call to CancelAllEvents (because of at least one
   * of these two timers) which will trigger a call to the
   * previous listener's cancel method.
   *
   * This typically happens because the high-priority
   * QapScheduler has taken access to the channel from
   * one of the Edca of the QAP.
   */
  m_currentPacket = packet->Copy ();
  // remove the priority tag attached, if any
  SocketPriorityTag priorityTag;
  m_currentPacket->RemovePacketTag (priorityTag);
  m_currentHdr = *hdr;
  m_currentDca = dca;
  CancelAllEvents ();
  m_txParams = params;
  m_currentTxVector = GetDataTxVector (m_currentPacket, &m_currentHdr);

  if (NeedRts ())
    {
      m_txParams.EnableRts ();
    }
  else
    {
      m_txParams.DisableRts ();
    }

  if (m_currentHdr.IsMgt ()
      || (!m_currentHdr.IsQosData ()
          && !m_currentHdr.IsBlockAck ()
          && !m_currentHdr.IsBlockAckReq ()))
    {
      //This is mainly encountered when a higher priority control or management frame is
      //sent between A-MPDU transmissions. It avoids to unexpectedly flush the aggregate
      //queue when previous RTS request has failed.
      m_ampdu = false;
    }
  else if (m_currentHdr.IsQosData () && !m_aggregateQueue[GetTid (packet, *hdr)]->IsEmpty ())
    {
      //m_aggregateQueue > 0 occurs when a RTS/CTS exchange failed before an A-MPDU transmission.
      //In that case, we transmit the same A-MPDU as previously.
      uint8_t sentMpdus = m_aggregateQueue[GetTid (packet, *hdr)]->GetNPackets ();
      m_ampdu = true;
      if (sentMpdus > 1)
        {
          m_txParams.EnableCompressedBlockAck ();
        }
      else if (m_currentHdr.IsQosData ())
        {
          //VHT/HE single MPDUs are followed by normal ACKs
          m_txParams.EnableAck ();
        }
      AcIndex ac = QosUtilsMapTidToAc (GetTid (packet, *hdr));
      std::map<AcIndex, Ptr<EdcaTxopN> >::const_iterator edcaIt = m_edca.find (ac);
      Ptr<Packet> aggregatedPacket = Create<Packet> ();
      for (uint32_t i = 0; i < sentMpdus; i++)
        {
          Ptr<Packet> newPacket = (m_txPackets[GetTid (packet, *hdr)].at (i).packet)->Copy ();
          newPacket->AddHeader (m_txPackets[GetTid (packet, *hdr)].at (i).hdr);
          WifiMacTrailer fcs;
          newPacket->AddTrailer (fcs);
          edcaIt->second->GetMpduAggregator ()->Aggregate (newPacket, aggregatedPacket);
        }
      m_currentPacket = aggregatedPacket;
      m_currentHdr = (m_txPackets[GetTid (packet, *hdr)].at (0).hdr);
      m_currentTxVector = GetDataTxVector (m_currentPacket, &m_currentHdr);
    }
  else
    {
      //Perform MPDU aggregation if possible
      m_ampdu = IsAmpdu (m_currentPacket, m_currentHdr);
      if (m_ampdu)
        {
          AmpduTag ampdu;
          m_currentPacket->PeekPacketTag (ampdu);
          if (ampdu.GetRemainingNbOfMpdus () > 0)
            {
              m_txParams.EnableCompressedBlockAck ();
            }
          else if (m_currentHdr.IsQosData ())
            {
              //VHT/HE single MPDUs are followed by normal ACKs
              m_txParams.EnableAck ();
            }
        }
    }

  NS_LOG_DEBUG ("startTx size=" << GetSize (m_currentPacket, &m_currentHdr, m_ampdu) <<
                ", to=" << m_currentHdr.GetAddr1 () << ", dca=" << m_currentDca);

  if (m_txParams.MustSendRts ())
    {
      SendRtsForPacket ();
    }
  else
    {
      if ((m_ctsToSelfSupported || m_stationManager->GetUseNonErpProtection ()) && NeedCtsToSelf ())
        {
          SendCtsToSelf ();
        }
      else
        {
          SendDataPacket ();
        }
    }

  /* When this method completes, we have taken ownership of the medium. */
  NS_ASSERT (m_phy->IsStateTx ());
}

bool
MacLow::NeedRts (void) const
{
  WifiTxVector dataTxVector = GetDataTxVector (m_currentPacket, &m_currentHdr);
  return m_stationManager->NeedRts (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                    m_currentPacket, dataTxVector);
}

bool
MacLow::NeedCtsToSelf (void) const
{
  WifiTxVector dataTxVector = GetDataTxVector (m_currentPacket, &m_currentHdr);
  return m_stationManager->NeedCtsToSelf (dataTxVector);
}

void
MacLow::ReceiveError (Ptr<Packet> packet, double rxSnr)
{
  NS_LOG_FUNCTION (this << packet << rxSnr);
  NS_LOG_DEBUG ("rx failed");
  if (m_txParams.MustWaitFastAck ())
    {
      NS_ASSERT (m_fastAckFailedTimeoutEvent.IsExpired ());
      m_fastAckFailedTimeoutEvent = Simulator::Schedule (GetSifs (),
                                                         &MacLow::FastAckFailedTimeout, this);
    }
  return;
}

void
MacLow::NotifySwitchingStartNow (Time duration)
{
  NS_LOG_DEBUG ("switching channel. Cancelling MAC pending events");
  m_stationManager->Reset ();
  CancelAllEvents ();
  if (m_navCounterResetCtsMissed.IsRunning ())
    {
      m_navCounterResetCtsMissed.Cancel ();
    }
  m_lastNavStart = Simulator::Now ();
  m_lastNavDuration = Seconds (0);
  m_currentPacket = 0;
  m_currentDca = 0;
}

void
MacLow::NotifySleepNow (void)
{
  NS_LOG_DEBUG ("Device in sleep mode. Cancelling MAC pending events");
  CancelAllEvents ();
  if (m_navCounterResetCtsMissed.IsRunning ())
    {
      m_navCounterResetCtsMissed.Cancel ();
    }
  m_lastNavStart = Simulator::Now ();
  m_lastNavDuration = Seconds (0);
  m_currentPacket = 0;
  m_currentDca = 0;
}

void
MacLow::ReceiveOk (Ptr<Packet> packet, double rxSnr, WifiTxVector txVector, bool ampduSubframe)
{
  NS_LOG_FUNCTION (this << packet << rxSnr << txVector.GetMode () << txVector.GetPreambleType ());
  /* A packet is received from the PHY.
   * When we have handled this packet,
   * we handle any packet present in the
   * packet queue.
   */
  WifiMacHeader hdr;
  packet->RemoveHeader (hdr);
  m_lastReceivedHdr = hdr;

  bool isPrevNavZero = IsNavZero ();
  NS_LOG_DEBUG ("duration/id=" << hdr.GetDuration ());
  NotifyNav (packet, hdr, txVector.GetPreambleType ());
  if (hdr.IsRts ())
    {
      /* see section 9.2.5.7 802.11-1999
       * A STA that is addressed by an RTS frame shall transmit a CTS frame after a SIFS
       * period if the NAV at the STA receiving the RTS frame indicates that the medium is
       * idle. If the NAV at the STA receiving the RTS indicates the medium is not idle,
       * that STA shall not respond to the RTS frame.
       */
      if (ampduSubframe)
        {
          NS_FATAL_ERROR ("Received RTS as part of an A-MPDU");
        }
      else
        {
          if (isPrevNavZero
              && hdr.GetAddr1 () == m_self)
            {
              NS_LOG_DEBUG ("rx RTS from=" << hdr.GetAddr2 () << ", schedule CTS");
              NS_ASSERT (m_sendCtsEvent.IsExpired ());
              m_stationManager->ReportRxOk (hdr.GetAddr2 (), &hdr,
                                            rxSnr, txVector.GetMode ());
              m_sendCtsEvent = Simulator::Schedule (GetSifs (),
                                                    &MacLow::SendCtsAfterRts, this,
                                                    hdr.GetAddr2 (),
                                                    hdr.GetDuration (),
                                                    txVector,
                                                    rxSnr);
            }
          else
            {
              NS_LOG_DEBUG ("rx RTS from=" << hdr.GetAddr2 () << ", cannot schedule CTS");
            }
        }
    }
  else if (hdr.IsCts ()
           && hdr.GetAddr1 () == m_self
           && m_ctsTimeoutEvent.IsRunning ()
           && m_currentPacket != 0)
    {
      if (ampduSubframe)
        {
          NS_FATAL_ERROR ("Received CTS as part of an A-MPDU");
        }

      NS_LOG_DEBUG ("received cts from=" << m_currentHdr.GetAddr1 ());

      SnrTag tag;
      packet->RemovePacketTag (tag);
      m_stationManager->ReportRxOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                    rxSnr, txVector.GetMode ());
      m_stationManager->ReportRtsOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                     rxSnr, txVector.GetMode (), tag.Get ());

      m_ctsTimeoutEvent.Cancel ();
      NotifyCtsTimeoutResetNow ();
      NS_ASSERT (m_sendDataEvent.IsExpired ());
      m_sendDataEvent = Simulator::Schedule (GetSifs (),
                                             &MacLow::SendDataAfterCts, this,
                                             hdr.GetAddr1 (),
                                             hdr.GetDuration ());
    }
  else if (hdr.IsAck ()
           && hdr.GetAddr1 () == m_self
           && (m_normalAckTimeoutEvent.IsRunning ()
               || m_fastAckTimeoutEvent.IsRunning ()
               || m_superFastAckTimeoutEvent.IsRunning ())
           && m_txParams.MustWaitAck ())
    {
      NS_LOG_DEBUG ("receive ack from=" << m_currentHdr.GetAddr1 ());
      SnrTag tag;
      packet->RemovePacketTag (tag);
      //When fragmentation is used, only update manager when the last fragment is acknowledged
      if (!m_txParams.HasNextPacket ())
        {
          m_stationManager->ReportRxOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                        rxSnr, txVector.GetMode ());
          m_stationManager->ReportDataOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                          rxSnr, txVector.GetMode (), tag.Get ());
        }
      bool gotAck = false;
      if (m_txParams.MustWaitNormalAck ()
          && m_normalAckTimeoutEvent.IsRunning ())
        {
          m_normalAckTimeoutEvent.Cancel ();
          NotifyAckTimeoutResetNow ();
          gotAck = true;
        }
      if (m_txParams.MustWaitFastAck ()
          && m_fastAckTimeoutEvent.IsRunning ())
        {
          m_fastAckTimeoutEvent.Cancel ();
          NotifyAckTimeoutResetNow ();
          gotAck = true;
        }
      if (gotAck)
        {
          m_currentDca->GotAck ();
        }
      if (m_txParams.HasNextPacket () && (!m_currentHdr.IsQosData () || m_currentDca->GetTxopLimit ().IsZero () || m_currentDca->HasTxop ()))
        {
          if (m_stationManager->GetRifsPermitted ())
            {
              m_waitIfsEvent = Simulator::Schedule (GetRifs (), &MacLow::WaitIfsAfterEndTxFragment, this);
            }
          else
            {
              m_waitIfsEvent = Simulator::Schedule (GetSifs (), &MacLow::WaitIfsAfterEndTxFragment, this);
            }
        }
      else if (m_currentHdr.IsQosData () && m_currentDca->HasTxop ())
        {
          if (m_stationManager->GetRifsPermitted ())
            {
              m_waitIfsEvent = Simulator::Schedule (GetRifs (), &MacLow::WaitIfsAfterEndTxPacket, this);
            }
          else
            {
              m_waitIfsEvent = Simulator::Schedule (GetSifs (), &MacLow::WaitIfsAfterEndTxPacket, this);
            }
        }
      m_ampdu = false;
      if (m_currentHdr.IsQosData ())
        {
          FlushAggregateQueue (m_currentHdr.GetQosTid ());
        }
    }
  else if (hdr.IsBlockAck () && hdr.GetAddr1 () == m_self
           && (m_txParams.MustWaitBasicBlockAck () || m_txParams.MustWaitCompressedBlockAck ())
           && m_blockAckTimeoutEvent.IsRunning ())
    {
      NS_LOG_DEBUG ("got block ack from " << hdr.GetAddr2 ());
      SnrTag tag;
      packet->RemovePacketTag (tag);
      FlushAggregateQueue (GetTid (packet, hdr));
      CtrlBAckResponseHeader blockAck;
      packet->RemoveHeader (blockAck);
      m_blockAckTimeoutEvent.Cancel ();
      NotifyAckTimeoutResetNow ();
      m_currentDca->GotBlockAck (&blockAck, hdr.GetAddr2 (), rxSnr, txVector.GetMode (), tag.Get ());
      m_ampdu = false;
      if (m_currentHdr.IsQosData () && m_currentDca->HasTxop ())
        {
          if (m_stationManager->GetRifsPermitted ())
            {
              m_waitIfsEvent = Simulator::Schedule (GetRifs (), &MacLow::WaitIfsAfterEndTxPacket, this);
            }
          else
            {
              m_waitIfsEvent = Simulator::Schedule (GetSifs (), &MacLow::WaitIfsAfterEndTxPacket, this);
            }
        }
    }
  else if (hdr.IsBlockAckReq () && hdr.GetAddr1 () == m_self)
    {
      CtrlBAckRequestHeader blockAckReq;
      packet->RemoveHeader (blockAckReq);
      if (!blockAckReq.IsMultiTid ())
        {
          uint8_t tid = blockAckReq.GetTidInfo ();
          AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), tid));
          if (it != m_bAckAgreements.end ())
            {
              //Update block ack cache
              BlockAckCachesI i = m_bAckCaches.find (std::make_pair (hdr.GetAddr2 (), tid));
              NS_ASSERT (i != m_bAckCaches.end ());
              (*i).second.UpdateWithBlockAckReq (blockAckReq.GetStartingSequence ());

              //NS_ASSERT (m_sendAckEvent.IsExpired ());
              m_sendAckEvent.Cancel ();
              /* See section 11.5.3 in IEEE 802.11 for mean of this timer */
              ResetBlockAckInactivityTimerIfNeeded (it->second.first);
              if ((*it).second.first.IsImmediateBlockAck ())
                {
                  NS_LOG_DEBUG ("rx blockAckRequest/sendImmediateBlockAck from=" << hdr.GetAddr2 ());
                  m_sendAckEvent = Simulator::Schedule (GetSifs (),
                                                        &MacLow::SendBlockAckAfterBlockAckRequest, this,
                                                        blockAckReq,
                                                        hdr.GetAddr2 (),
                                                        hdr.GetDuration (),
                                                        txVector.GetMode (),
                                                        rxSnr);
                }
              else
                {
                  NS_FATAL_ERROR ("Delayed block ack not supported.");
                }
            }
          else
            {
              NS_LOG_DEBUG ("There's not a valid agreement for this block ack request.");
            }
        }
      else
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
    }
  else if (hdr.IsCtl ())
    {
      NS_LOG_DEBUG ("rx drop " << hdr.GetTypeString ());
    }
  else if (hdr.GetAddr1 () == m_self)
    {
      m_stationManager->ReportRxOk (hdr.GetAddr2 (), &hdr,
                                    rxSnr, txVector.GetMode ());
      if (hdr.IsQosData () && ReceiveMpdu (packet, hdr))
        {
          /* From section 9.10.4 in IEEE 802.11:
             Upon the receipt of a QoS data frame from the originator for which
             the Block Ack agreement exists, the recipient shall buffer the MSDU
             regardless of the value of the Ack Policy subfield within the
             QoS Control field of the QoS data frame. */
          if (hdr.IsQosAck () && !ampduSubframe)
            {
              NS_LOG_DEBUG ("rx QoS unicast/sendAck from=" << hdr.GetAddr2 ());
              AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));

              RxCompleteBufferedPacketsWithSmallerSequence (it->second.first.GetStartingSequenceControl (),
                                                            hdr.GetAddr2 (), hdr.GetQosTid ());
              RxCompleteBufferedPacketsUntilFirstLost (hdr.GetAddr2 (), hdr.GetQosTid ());
              NS_ASSERT (m_sendAckEvent.IsExpired ());
              m_sendAckEvent = Simulator::Schedule (GetSifs (),
                                                    &MacLow::SendAckAfterData, this,
                                                    hdr.GetAddr2 (),
                                                    hdr.GetDuration (),
                                                    txVector.GetMode (),
                                                    rxSnr);
            }
          else if (hdr.IsQosBlockAck ())
            {
              AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
              /* See section 11.5.3 in IEEE 802.11 for mean of this timer */
              ResetBlockAckInactivityTimerIfNeeded (it->second.first);
            }
          return;
        }
      else if (hdr.IsQosData () && hdr.IsQosBlockAck ())
        {
          /* This happens if a packet with ack policy Block Ack is received and a block ack
             agreement for that packet doesn't exist.

             From section 11.5.3 in IEEE 802.11e:
             When a recipient does not have an active Block ack for a TID, but receives
             data MPDUs with the Ack Policy subfield set to Block Ack, it shall discard
             them and shall send a DELBA frame using the normal access
             mechanisms. */
          AcIndex ac = QosUtilsMapTidToAc (hdr.GetQosTid ());
          m_edca[ac]->SendDelbaFrame (hdr.GetAddr2 (), hdr.GetQosTid (), false);
          return;
        }
      else if (hdr.IsQosData () && hdr.IsQosNoAck ())
        {
          if (ampduSubframe)
            {
              NS_LOG_DEBUG ("rx Ampdu with No Ack Policy from=" << hdr.GetAddr2 ());
            }
          else
            {
              NS_LOG_DEBUG ("rx unicast/noAck from=" << hdr.GetAddr2 ());
            }
        }
      else if (hdr.IsData () || hdr.IsMgt ())
        {
          if (hdr.IsProbeResp ())
            {
              // Apply SNR tag for probe response quality measurements
              SnrTag tag;
              tag.Set (rxSnr);
              packet->AddPacketTag (tag);
            }
          if (hdr.IsMgt () && ampduSubframe)
            {
              NS_FATAL_ERROR ("Received management packet as part of an A-MPDU");
            }
          else
            {
              NS_LOG_DEBUG ("rx unicast/sendAck from=" << hdr.GetAddr2 ());
              NS_ASSERT (m_sendAckEvent.IsExpired ());
              m_sendAckEvent = Simulator::Schedule (GetSifs (),
                                                    &MacLow::SendAckAfterData, this,
                                                    hdr.GetAddr2 (),
                                                    hdr.GetDuration (),
                                                    txVector.GetMode (),
                                                    rxSnr);
            }
        }
      goto rxPacket;
    }
  else if (hdr.GetAddr1 ().IsGroup ())
    {
      if (ampduSubframe)
        {
          NS_FATAL_ERROR ("Received group addressed packet as part of an A-MPDU");
        }
      else
        {
          if (hdr.IsData () || hdr.IsMgt ())
            {
              NS_LOG_DEBUG ("rx group from=" << hdr.GetAddr2 ());
              if (hdr.IsBeacon ())
                {
                  // Apply SNR tag for beacon quality measurements
                  SnrTag tag;
                  tag.Set (rxSnr);
                  packet->AddPacketTag (tag);
                }
              goto rxPacket;
            }
        }
    }
  else if (m_promisc)
    {
      NS_ASSERT (hdr.GetAddr1 () != m_self);
      if (hdr.IsData ())
        {
          goto rxPacket;
        }
    }
  else
    {
      NS_LOG_DEBUG ("rx not for me from=" << hdr.GetAddr2 ());
    }
  return;
rxPacket:
  WifiMacTrailer fcs;
  packet->RemoveTrailer (fcs);
  m_rxCallback (packet, &hdr);
  return;
}

uint32_t
MacLow::GetAckSize (void)
{
  WifiMacHeader ack;
  ack.SetType (WIFI_MAC_CTL_ACK);
  return ack.GetSize () + 4;
}

uint32_t
MacLow::GetBlockAckSize (BlockAckType type)
{
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_BACKRESP);
  CtrlBAckResponseHeader blockAck;
  if (type == BASIC_BLOCK_ACK)
    {
      blockAck.SetType (BASIC_BLOCK_ACK);
    }
  else if (type == COMPRESSED_BLOCK_ACK)
    {
      blockAck.SetType (COMPRESSED_BLOCK_ACK);
    }
  else if (type == MULTI_TID_BLOCK_ACK)
    {
      //Not implemented
      NS_ASSERT (false);
    }
  return hdr.GetSize () + blockAck.GetSerializedSize () + 4;
}

uint32_t
MacLow::GetRtsSize (void)
{
  WifiMacHeader rts;
  rts.SetType (WIFI_MAC_CTL_RTS);
  return rts.GetSize () + 4;
}

Time
MacLow::GetAckDuration (Mac48Address to, WifiTxVector dataTxVector) const
{
  WifiTxVector ackTxVector = GetAckTxVectorForData (to, dataTxVector.GetMode ());
  return GetAckDuration (ackTxVector);
}

Time
MacLow::GetAckDuration (WifiTxVector ackTxVector) const
{
  NS_ASSERT (ackTxVector.GetMode ().GetModulationClass () != WIFI_MOD_CLASS_HT); //ACK should always use non-HT PPDU (HT PPDU cases not supported yet)
  return m_phy->CalculateTxDuration (GetAckSize (), ackTxVector, m_phy->GetFrequency ());
}

Time
MacLow::GetBlockAckDuration (Mac48Address to, WifiTxVector blockAckReqTxVector, BlockAckType type) const
{
  /*
   * For immediate Basic BlockAck we should transmit the frame with the same WifiMode
   * as the BlockAckReq.
   */
  return m_phy->CalculateTxDuration (GetBlockAckSize (type), blockAckReqTxVector, m_phy->GetFrequency ());
}

Time
MacLow::GetCtsDuration (Mac48Address to, WifiTxVector rtsTxVector) const
{
  WifiTxVector ctsTxVector = GetCtsTxVectorForRts (to, rtsTxVector.GetMode ());
  return GetCtsDuration (ctsTxVector);
}

Time
MacLow::GetCtsDuration (WifiTxVector ctsTxVector) const
{
  NS_ASSERT (ctsTxVector.GetMode ().GetModulationClass () != WIFI_MOD_CLASS_HT); //CTS should always use non-HT PPDU (HT PPDU cases not supported yet)
  return m_phy->CalculateTxDuration (GetCtsSize (), ctsTxVector, m_phy->GetFrequency ());
}

uint32_t
MacLow::GetCtsSize (void)
{
  WifiMacHeader cts;
  cts.SetType (WIFI_MAC_CTL_CTS);
  return cts.GetSize () + 4;
}

uint32_t
MacLow::GetSize (Ptr<const Packet> packet, const WifiMacHeader *hdr, bool isAmpdu)
{
  uint32_t size;
  WifiMacTrailer fcs;
  if (isAmpdu)
    {
      size = packet->GetSize ();
    }
  else
    {
      size = packet->GetSize () + hdr->GetSize () + fcs.GetSerializedSize ();
    }
  return size;
}

WifiTxVector
MacLow::GetCtsToSelfTxVector (Ptr<const Packet> packet, const WifiMacHeader *hdr) const
{
  return m_stationManager->GetCtsToSelfTxVector (hdr, packet);
}

WifiTxVector
MacLow::GetRtsTxVector (Ptr<const Packet> packet, const WifiMacHeader *hdr) const
{
  Mac48Address to = hdr->GetAddr1 ();
  return m_stationManager->GetRtsTxVector (to, hdr, packet);
}

WifiTxVector
MacLow::GetDataTxVector (Ptr<const Packet> packet, const WifiMacHeader *hdr) const
{
  Mac48Address to = hdr->GetAddr1 ();
  return m_stationManager->GetDataTxVector (to, hdr, packet);
}

WifiTxVector
MacLow::GetCtsTxVector (Mac48Address to, WifiMode rtsTxMode) const
{
  return m_stationManager->GetCtsTxVector (to, rtsTxMode);
}

WifiTxVector
MacLow::GetAckTxVector (Mac48Address to, WifiMode dataTxMode) const
{
  return m_stationManager->GetAckTxVector (to, dataTxMode);
}

WifiTxVector
MacLow::GetBlockAckTxVector (Mac48Address to, WifiMode dataTxMode) const
{
  return m_stationManager->GetBlockAckTxVector (to, dataTxMode);
}

WifiTxVector
MacLow::GetCtsTxVectorForRts (Mac48Address to, WifiMode rtsTxMode) const
{
  return GetCtsTxVector (to, rtsTxMode);
}

WifiTxVector
MacLow::GetAckTxVectorForData (Mac48Address to, WifiMode dataTxMode) const
{
  return GetAckTxVector (to, dataTxMode);
}

Time
MacLow::CalculateOverallTxTime (Ptr<const Packet> packet,
                                const WifiMacHeader* hdr,
                                const MacLowTransmissionParameters& params) const
{
  Time txTime = Seconds (0);
  if (params.MustSendRts ())
    {
      WifiTxVector rtsTxVector = GetRtsTxVector (packet, hdr);
      txTime += m_phy->CalculateTxDuration (GetRtsSize (), rtsTxVector, m_phy->GetFrequency ());
      txTime += GetCtsDuration (hdr->GetAddr1 (), rtsTxVector);
      txTime += Time (GetSifs () * 2);
    }
  WifiTxVector dataTxVector = GetDataTxVector (packet, hdr);
  uint32_t dataSize = GetSize (packet, hdr, m_ampdu);
  txTime += m_phy->CalculateTxDuration (dataSize, dataTxVector, m_phy->GetFrequency ());
  txTime += GetSifs ();
  if (params.MustWaitAck ())
    {
      txTime += GetAckDuration (hdr->GetAddr1 (), dataTxVector);
    }
  return txTime;
}

Time
MacLow::CalculateOverallTxFragmentTime (Ptr<const Packet> packet,
                                        const WifiMacHeader* hdr,
                                        const MacLowTransmissionParameters& params,
                                        uint32_t fragmentSize) const
{
  Time txTime = Seconds (0);
  if (params.MustSendRts ())
    {
      WifiTxVector rtsTxVector = GetRtsTxVector (packet, hdr);
      txTime += m_phy->CalculateTxDuration (GetRtsSize (), rtsTxVector, m_phy->GetFrequency ());
      txTime += GetCtsDuration (hdr->GetAddr1 (), rtsTxVector);
      txTime += Time (GetSifs () * 2);
    }
  WifiTxVector dataTxVector = GetDataTxVector (packet, hdr);
  Ptr<const Packet> fragment = Create<Packet> (fragmentSize);
  uint32_t dataSize = GetSize (fragment, hdr, m_ampdu);
  txTime += m_phy->CalculateTxDuration (dataSize, dataTxVector, m_phy->GetFrequency ());
  txTime += GetSifs ();
  if (params.MustWaitAck ())
    {
      txTime += GetAckDuration (hdr->GetAddr1 (), dataTxVector);
    }
  return txTime;
}

Time
MacLow::CalculateTransmissionTime (Ptr<const Packet> packet,
                                   const WifiMacHeader* hdr,
                                   const MacLowTransmissionParameters& params) const
{
  Time txTime = CalculateOverallTxTime (packet, hdr, params);
  if (params.HasNextPacket ())
    {
      WifiTxVector dataTxVector = GetDataTxVector (packet, hdr);
      txTime += GetSifs ();
      txTime += m_phy->CalculateTxDuration (params.GetNextPacketSize (), dataTxVector, m_phy->GetFrequency ());
    }
  return txTime;
}

void
MacLow::NotifyNav (Ptr<const Packet> packet,const WifiMacHeader &hdr, WifiPreamble preamble)
{
  NS_ASSERT (m_lastNavStart <= Simulator::Now ());
  Time duration = hdr.GetDuration ();

  if (hdr.IsCfpoll ()
      && hdr.GetAddr2 () == m_bssid)
    {
      //see section 9.3.2.2 802.11-1999
      DoNavResetNow (duration);
      return;
    }
  /// \todo We should also handle CF_END specially here
  /// but we don't for now because we do not generate them.
  else if (hdr.GetAddr1 () != m_self)
    {
      // see section 9.2.5.4 802.11-1999
      bool navUpdated = DoNavStartNow (duration);
      if (hdr.IsRts () && navUpdated)
        {
          /**
           * A STA that used information from an RTS frame as the most recent basis to update its NAV setting
           * is permitted to reset its NAV if no PHY-RXSTART.indication is detected from the PHY during a
           * period with a duration of (2 * aSIFSTime) + (CTS_Time) + (2 * aSlotTime) starting at the
           * PHY-RXEND.indication corresponding to the detection of the RTS frame. The “CTS_Time” shall
           * be calculated using the length of the CTS frame and the data rate at which the RTS frame
           * used for the most recent NAV update was received.
           */
          WifiMacHeader cts;
          cts.SetType (WIFI_MAC_CTL_CTS);
          WifiTxVector txVector = GetRtsTxVector (packet, &hdr);
          Time navCounterResetCtsMissedDelay =
            m_phy->CalculateTxDuration (cts.GetSerializedSize (), txVector, m_phy->GetFrequency ()) +
            Time (2 * GetSifs ()) + Time (2 * GetSlotTime ());
          m_navCounterResetCtsMissed = Simulator::Schedule (navCounterResetCtsMissedDelay,
                                                            &MacLow::NavCounterResetCtsMissed, this,
                                                            Simulator::Now ());
        }
    }
}

void
MacLow::NavCounterResetCtsMissed (Time rtsEndRxTime)
{
  if (m_phy->GetLastRxStartTime () < rtsEndRxTime)
    {
      DoNavResetNow (Seconds (0.0));
    }
}

void
MacLow::DoNavResetNow (Time duration)
{
  for (DcfManagersCI i = m_dcfManagers.begin (); i != m_dcfManagers.end (); i++)
    {
      (*i)->NotifyNavResetNow (duration);
    }
  m_lastNavStart = Simulator::Now ();
  m_lastNavDuration = duration;
}

bool
MacLow::DoNavStartNow (Time duration)
{
  for (DcfManagersCI i = m_dcfManagers.begin (); i != m_dcfManagers.end (); i++)
    {
      (*i)->NotifyNavStartNow (duration);
    }
  Time newNavEnd = Simulator::Now () + duration;
  Time oldNavEnd = m_lastNavStart + m_lastNavDuration;
  if (newNavEnd > oldNavEnd)
    {
      m_lastNavStart = Simulator::Now ();
      m_lastNavDuration = duration;
      return true;
    }
  return false;
}

void
MacLow::NotifyAckTimeoutStartNow (Time duration)
{
  for (DcfManagersCI i = m_dcfManagers.begin (); i != m_dcfManagers.end (); i++)
    {
      (*i)->NotifyAckTimeoutStartNow (duration);
    }
}

void
MacLow::NotifyAckTimeoutResetNow ()
{
  for (DcfManagersCI i = m_dcfManagers.begin (); i != m_dcfManagers.end (); i++)
    {
      (*i)->NotifyAckTimeoutResetNow ();
    }
}

void
MacLow::NotifyCtsTimeoutStartNow (Time duration)
{
  for (DcfManagersCI i = m_dcfManagers.begin (); i != m_dcfManagers.end (); i++)
    {
      (*i)->NotifyCtsTimeoutStartNow (duration);
    }
}

void
MacLow::NotifyCtsTimeoutResetNow ()
{
  for (DcfManagersCI i = m_dcfManagers.begin (); i != m_dcfManagers.end (); i++)
    {
      (*i)->NotifyCtsTimeoutResetNow ();
    }
}

void
MacLow::ForwardDown (Ptr<const Packet> packet, const WifiMacHeader* hdr, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << packet << hdr << txVector);
  NS_LOG_DEBUG ("send " << hdr->GetTypeString () <<
                ", to=" << hdr->GetAddr1 () <<
                ", size=" << packet->GetSize () <<
                ", mode=" << txVector.GetMode  () <<
                ", preamble=" << txVector.GetPreambleType () <<
                ", duration=" << hdr->GetDuration () <<
                ", seq=0x" << std::hex << m_currentHdr.GetSequenceControl () << std::dec);
  if (!m_ampdu || hdr->IsAck () || hdr->IsRts () || hdr->IsCts () || hdr->IsBlockAck () || hdr->IsMgt ())
    {
      m_phy->SendPacket (packet, txVector);
    }
  else
    {
      Ptr<Packet> newPacket;
      Ptr <WifiMacQueueItem> dequeuedItem;
      WifiMacHeader newHdr;
      WifiMacTrailer fcs;
      uint8_t queueSize = m_aggregateQueue[GetTid (packet, *hdr)]->GetNPackets ();
      bool singleMpdu = false;
      bool last = false;
      MpduType mpdutype = NORMAL_MPDU;

      uint8_t tid = GetTid (packet, *hdr);
      AcIndex ac = QosUtilsMapTidToAc (tid);
      std::map<AcIndex, Ptr<EdcaTxopN> >::const_iterator edcaIt = m_edca.find (ac);

      if (queueSize == 1)
        {
          singleMpdu = true;
        }

      //Add packet tag
      AmpduTag ampdutag;
      ampdutag.SetAmpdu (true);
      Time delay = Seconds (0);
      Time remainingAmpduDuration = m_phy->CalculateTxDuration (packet->GetSize (), txVector, m_phy->GetFrequency ());
      if (queueSize > 1 || singleMpdu)
        {
          txVector.SetAggregation (true);
        }
      for (; queueSize > 0; queueSize--)
        {
          dequeuedItem = m_aggregateQueue[GetTid (packet, *hdr)]->Dequeue ();
          newHdr = dequeuedItem->GetHeader ();
          newPacket = dequeuedItem->GetPacket ()->Copy ();
          newHdr.SetDuration (hdr->GetDuration ());
          newPacket->AddHeader (newHdr);
          AddWifiMacTrailer (newPacket);
          if (queueSize == 1)
            {
              last = true;
              mpdutype = LAST_MPDU_IN_AGGREGATE;
            }

          edcaIt->second->GetMpduAggregator ()->AddHeaderAndPad (newPacket, last, singleMpdu);

          if (delay.IsZero ())
            {
              if (!singleMpdu)
                {
                  NS_LOG_DEBUG ("Sending MPDU as part of A-MPDU");
                  mpdutype = MPDU_IN_AGGREGATE;
                }
              else
                {
                  NS_LOG_DEBUG ("Sending S-MPDU");
                  mpdutype = NORMAL_MPDU;
                }
            }

          Time mpduDuration = m_phy->CalculateTxDuration (newPacket->GetSize (), txVector, m_phy->GetFrequency (), mpdutype, 0);
          remainingAmpduDuration -= mpduDuration;

          ampdutag.SetRemainingNbOfMpdus (queueSize - 1);
          if (queueSize > 1)
            {
              ampdutag.SetRemainingAmpduDuration (remainingAmpduDuration);
            }
          else
            {
              ampdutag.SetRemainingAmpduDuration (NanoSeconds (0));
            }
          newPacket->AddPacketTag (ampdutag);

          if (delay.IsZero ())
            {
              m_phy->SendPacket (newPacket, txVector, mpdutype);
            }
          else
            {
              Simulator::Schedule (delay, &MacLow::SendMpdu, this, newPacket, txVector, mpdutype);
            }
          if (queueSize > 1)
            {
              NS_ASSERT (remainingAmpduDuration > 0);
              delay = delay + mpduDuration;
            }

          txVector.SetPreambleType (WIFI_PREAMBLE_NONE);
        }
    }
}

void
MacLow::SendMpdu (Ptr<const Packet> packet, WifiTxVector txVector, MpduType mpdutype)
{
  NS_LOG_DEBUG ("Sending MPDU as part of A-MPDU");
  m_phy->SendPacket (packet, txVector, mpdutype);
}

void
MacLow::CtsTimeout (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("cts timeout");
  /// \todo should check that there was no rx start before now.
  /// we should restart a new cts timeout now until the expected
  /// end of rx if there was a rx start before now.
  m_stationManager->ReportRtsFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
  Ptr<DcaTxop> dca = m_currentDca;
  m_currentDca = 0;
  m_ampdu = false;
  dca->MissedCts ();
}

void
MacLow::NormalAckTimeout (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("normal ack timeout");
  /// \todo should check that there was no rx start before now.
  /// we should restart a new ack timeout now until the expected
  /// end of rx if there was a rx start before now.
  m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
  Ptr<DcaTxop> dca = m_currentDca;
  m_currentDca = 0;
  m_ampdu = false;
  if (m_currentHdr.IsQosData ())
    {
      FlushAggregateQueue (GetTid (m_currentPacket, m_currentHdr));
    }
  dca->MissedAck ();
}

void
MacLow::FastAckTimeout (void)
{
  NS_LOG_FUNCTION (this);
  m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
  Ptr<DcaTxop> dca = m_currentDca;
  m_currentDca = 0;
  if (m_phy->IsStateIdle ())
    {
      NS_LOG_DEBUG ("fast Ack idle missed");
      dca->MissedAck ();
    }
  else
    {
      NS_LOG_DEBUG ("fast Ack ok");
    }
}

void
MacLow::BlockAckTimeout (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("block ack timeout");
  Ptr<DcaTxop> dca = m_currentDca;
  m_currentDca = 0;
  m_ampdu = false;
  uint8_t tid = GetTid (m_currentPacket, m_currentHdr);
  uint8_t nTxMpdus = m_aggregateQueue[tid]->GetNPackets ();
  FlushAggregateQueue (tid);
  dca->MissedBlockAck (nTxMpdus);
}

void
MacLow::SuperFastAckTimeout ()
{
  NS_LOG_FUNCTION (this);
  m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
  Ptr<DcaTxop> dca = m_currentDca;
  m_currentDca = 0;
  if (m_phy->IsStateIdle ())
    {
      NS_LOG_DEBUG ("super fast Ack failed");
      dca->MissedAck ();
    }
  else
    {
      NS_LOG_DEBUG ("super fast Ack ok");
      dca->GotAck ();
    }
}

void
MacLow::SendRtsForPacket (void)
{
  NS_LOG_FUNCTION (this);
  /* send an RTS for this packet. */
  WifiMacHeader rts;
  rts.SetType (WIFI_MAC_CTL_RTS);
  rts.SetDsNotFrom ();
  rts.SetDsNotTo ();
  rts.SetNoRetry ();
  rts.SetNoMoreFragments ();
  rts.SetAddr1 (m_currentHdr.GetAddr1 ());
  rts.SetAddr2 (m_self);
  WifiTxVector rtsTxVector = GetRtsTxVector (m_currentPacket, &m_currentHdr);
  Time duration = Seconds (0);

  if (m_txParams.HasDurationId ())
    {
      duration += m_txParams.GetDurationId ();
    }
  else
    {
      duration += GetSifs ();
      duration += GetCtsDuration (m_currentHdr.GetAddr1 (), rtsTxVector);
      duration += GetSifs ();
      duration += m_phy->CalculateTxDuration (GetSize (m_currentPacket, &m_currentHdr, m_ampdu),
                                              m_currentTxVector, m_phy->GetFrequency ());
      duration += GetSifs ();
      if (m_txParams.MustWaitBasicBlockAck ())
        {
          WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
          duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, BASIC_BLOCK_ACK);
        }
      else if (m_txParams.MustWaitCompressedBlockAck ())
        {
          WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
          duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, COMPRESSED_BLOCK_ACK);
        }
      else if (m_txParams.MustWaitAck ())
        {
          duration += GetAckDuration (m_currentHdr.GetAddr1 (), m_currentTxVector);
        }
      if (m_txParams.HasNextPacket ())
        {
          duration += m_phy->CalculateTxDuration (m_txParams.GetNextPacketSize (),
                                                  m_currentTxVector, m_phy->GetFrequency ());
          if (m_txParams.MustWaitAck ())
            {
              duration += GetSifs ();
              duration += GetAckDuration (m_currentHdr.GetAddr1 (), m_currentTxVector);
            }
        }
    }
  rts.SetDuration (duration);

  Time txDuration = m_phy->CalculateTxDuration (GetRtsSize (), rtsTxVector, m_phy->GetFrequency ());
  Time timerDelay = txDuration + GetCtsTimeout ();

  NS_ASSERT (m_ctsTimeoutEvent.IsExpired ());
  NotifyCtsTimeoutStartNow (timerDelay);
  m_ctsTimeoutEvent = Simulator::Schedule (timerDelay, &MacLow::CtsTimeout, this);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (rts);
  AddWifiMacTrailer (packet);

  ForwardDown (packet, &rts, rtsTxVector);
}

void
MacLow::StartDataTxTimers (WifiTxVector dataTxVector)
{
  Time txDuration = m_phy->CalculateTxDuration (GetSize (m_currentPacket, &m_currentHdr, m_ampdu), dataTxVector, m_phy->GetFrequency ());
  if (m_txParams.MustWaitNormalAck ())
    {
      Time timerDelay = txDuration + GetAckTimeout ();
      NS_ASSERT (m_normalAckTimeoutEvent.IsExpired ());
      NotifyAckTimeoutStartNow (timerDelay);
      m_normalAckTimeoutEvent = Simulator::Schedule (timerDelay, &MacLow::NormalAckTimeout, this);
    }
  else if (m_txParams.MustWaitFastAck ())
    {
      Time timerDelay = txDuration + GetPifs ();
      NS_ASSERT (m_fastAckTimeoutEvent.IsExpired ());
      NotifyAckTimeoutStartNow (timerDelay);
      m_fastAckTimeoutEvent = Simulator::Schedule (timerDelay, &MacLow::FastAckTimeout, this);
    }
  else if (m_txParams.MustWaitSuperFastAck ())
    {
      Time timerDelay = txDuration + GetPifs ();
      NS_ASSERT (m_superFastAckTimeoutEvent.IsExpired ());
      NotifyAckTimeoutStartNow (timerDelay);
      m_superFastAckTimeoutEvent = Simulator::Schedule (timerDelay,
                                                        &MacLow::SuperFastAckTimeout, this);
    }
  else if (m_txParams.MustWaitBasicBlockAck ())
    {
      Time timerDelay = txDuration + GetBasicBlockAckTimeout ();
      NS_ASSERT (m_blockAckTimeoutEvent.IsExpired ());
      NotifyAckTimeoutStartNow (timerDelay);
      m_blockAckTimeoutEvent = Simulator::Schedule (timerDelay, &MacLow::BlockAckTimeout, this);
    }
  else if (m_txParams.MustWaitCompressedBlockAck ())
    {
      Time timerDelay = txDuration + GetCompressedBlockAckTimeout ();
      NS_ASSERT (m_blockAckTimeoutEvent.IsExpired ());
      NotifyAckTimeoutStartNow (timerDelay);
      m_blockAckTimeoutEvent = Simulator::Schedule (timerDelay, &MacLow::BlockAckTimeout, this);
    }
  else if (m_txParams.HasNextPacket ())
    {
      NS_ASSERT (m_waitIfsEvent.IsExpired ());
      Time delay = txDuration;
      if (m_stationManager->GetRifsPermitted ())
        {
          delay += GetRifs ();
        }
      else
        {
          delay += GetSifs ();
        }
      m_waitIfsEvent = Simulator::Schedule (delay, &MacLow::WaitIfsAfterEndTxFragment, this);
    }
  else if (m_currentHdr.IsQosData () && m_currentHdr.IsQosBlockAck () && m_currentDca->HasTxop ())
    {
      Time delay = txDuration;
      if (m_stationManager->GetRifsPermitted ())
        {
          delay += GetRifs ();
        }
      else
        {
          delay += GetSifs ();
        }
      m_waitIfsEvent = Simulator::Schedule (delay, &MacLow::WaitIfsAfterEndTxPacket, this);
    }
  else
    {
      // since we do not expect any timer to be triggered.
      Simulator::Schedule (txDuration, &MacLow::EndTxNoAck, this);
    }
}

void
MacLow::SendDataPacket (void)
{
  NS_LOG_FUNCTION (this);
  /* send this packet directly. No RTS is needed. */
  StartDataTxTimers (m_currentTxVector);

  Time duration = Seconds (0.0);
  if (m_txParams.HasDurationId ())
    {
      duration += m_txParams.GetDurationId ();
    }
  else
    {
      if (m_txParams.MustWaitBasicBlockAck ())
        {
          duration += GetSifs ();
          WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
          duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, BASIC_BLOCK_ACK);
        }
      else if (m_txParams.MustWaitCompressedBlockAck ())
        {
          duration += GetSifs ();
          WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
          duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, COMPRESSED_BLOCK_ACK);
        }
      else if (m_txParams.MustWaitAck ())
        {
          duration += GetSifs ();
          duration += GetAckDuration (m_currentHdr.GetAddr1 (), m_currentTxVector);
        }
      if (m_txParams.HasNextPacket ())
        {
          if (m_stationManager->GetRifsPermitted ())
            {
              duration += GetRifs ();
            }
          else
            {
              duration += GetSifs ();
            }
          duration += m_phy->CalculateTxDuration (m_txParams.GetNextPacketSize (),
                                                  m_currentTxVector, m_phy->GetFrequency ());
          if (m_txParams.MustWaitAck ())
            {
              duration += GetSifs ();
              duration += GetAckDuration (m_currentHdr.GetAddr1 (), m_currentTxVector);
            }
        }
    }
  m_currentHdr.SetDuration (duration);
  Ptr <Packet> packet = m_currentPacket->Copy ();
  if (m_ampdu)
    {
      NS_ASSERT (m_currentHdr.IsQosData ());
    }
  else
    {
      packet->AddHeader (m_currentHdr);
      AddWifiMacTrailer (packet);
    }

  ForwardDown (packet, &m_currentHdr, m_currentTxVector);
}

bool
MacLow::IsNavZero (void) const
{
  if (m_lastNavStart + m_lastNavDuration < Simulator::Now ())
    {
      return true;
    }
  else
    {
      return false;
    }
}

void
MacLow::SendCtsToSelf (void)
{
  WifiMacHeader cts;
  cts.SetType (WIFI_MAC_CTL_CTS);
  cts.SetDsNotFrom ();
  cts.SetDsNotTo ();
  cts.SetNoMoreFragments ();
  cts.SetNoRetry ();
  cts.SetAddr1 (m_self);

  WifiTxVector ctsTxVector = GetRtsTxVector (m_currentPacket, &m_currentHdr);
  Time duration = Seconds (0);

  if (m_txParams.HasDurationId ())
    {
      duration += m_txParams.GetDurationId ();
    }
  else
    {
      duration += GetSifs ();
      duration += m_phy->CalculateTxDuration (GetSize (m_currentPacket, &m_currentHdr, m_ampdu),
                                              m_currentTxVector, m_phy->GetFrequency ());
      if (m_txParams.MustWaitBasicBlockAck ())
        {

          duration += GetSifs ();
          WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
          duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, BASIC_BLOCK_ACK);
        }
      else if (m_txParams.MustWaitCompressedBlockAck ())
        {
          duration += GetSifs ();
          WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
          duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, COMPRESSED_BLOCK_ACK);
        }
      else if (m_txParams.MustWaitAck ())
        {
          duration += GetSifs ();
          duration += GetAckDuration (m_currentHdr.GetAddr1 (), m_currentTxVector);
        }
      if (m_txParams.HasNextPacket ())
        {
          duration += GetSifs ();
          duration += m_phy->CalculateTxDuration (m_txParams.GetNextPacketSize (),
                                                  m_currentTxVector, m_phy->GetFrequency ());
          if (m_txParams.MustWaitCompressedBlockAck ())
            {
              duration += GetSifs ();
              WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
              duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, COMPRESSED_BLOCK_ACK);
            }
          else if (m_txParams.MustWaitAck ())
            {
              duration += GetSifs ();
              duration += GetAckDuration (m_currentHdr.GetAddr1 (), m_currentTxVector);
            }
        }
    }

  cts.SetDuration (duration);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (cts);
  AddWifiMacTrailer (packet);

  ForwardDown (packet, &cts, ctsTxVector);

  Time txDuration = m_phy->CalculateTxDuration (GetCtsSize (), ctsTxVector, m_phy->GetFrequency ());
  txDuration += GetSifs ();
  NS_ASSERT (m_sendDataEvent.IsExpired ());

  m_sendDataEvent = Simulator::Schedule (txDuration,
                                         &MacLow::SendDataAfterCts, this,
                                         cts.GetAddr1 (),
                                         duration);

}

void
MacLow::SendCtsAfterRts (Mac48Address source, Time duration, WifiTxVector rtsTxVector, double rtsSnr)
{
  NS_LOG_FUNCTION (this << source << duration << rtsTxVector.GetMode () << rtsSnr);
  /* send a CTS when you receive a RTS
   * right after SIFS.
   */
  WifiTxVector ctsTxVector = GetCtsTxVector (source, rtsTxVector.GetMode ());
  WifiMacHeader cts;
  cts.SetType (WIFI_MAC_CTL_CTS);
  cts.SetDsNotFrom ();
  cts.SetDsNotTo ();
  cts.SetNoMoreFragments ();
  cts.SetNoRetry ();
  cts.SetAddr1 (source);
  duration -= GetCtsDuration (source, rtsTxVector);
  duration -= GetSifs ();
  NS_ASSERT (duration.IsPositive ());
  cts.SetDuration (duration);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (cts);
  AddWifiMacTrailer (packet);

  SnrTag tag;
  tag.Set (rtsSnr);
  packet->AddPacketTag (tag);

  //CTS should always use non-HT PPDU (HT PPDU cases not supported yet)
  ForwardDown (packet, &cts, ctsTxVector);
}

void
MacLow::SendDataAfterCts (Mac48Address source, Time duration)
{
  NS_LOG_FUNCTION (this);
  /* send the third step in a
   * RTS/CTS/DATA/ACK hanshake
   */
  NS_ASSERT (m_currentPacket != 0);

  if (m_currentHdr.IsQosData ())
    {
      uint8_t tid = GetTid (m_currentPacket, m_currentHdr);
      if (!m_aggregateQueue[GetTid (m_currentPacket, m_currentHdr)]->IsEmpty ())
        {
          for (std::vector<Item>::size_type i = 0; i != m_txPackets[tid].size (); i++)
            {
              AcIndex ac = QosUtilsMapTidToAc (tid);
              std::map<AcIndex, Ptr<EdcaTxopN> >::const_iterator edcaIt = m_edca.find (ac);
              edcaIt->second->CompleteMpduTx (m_txPackets[tid].at (i).packet, m_txPackets[tid].at (i).hdr, m_txPackets[tid].at (i).timestamp);
            }
        }
    }

  StartDataTxTimers (m_currentTxVector);
  Time newDuration = Seconds (0);
  if (m_txParams.MustWaitBasicBlockAck ())
    {
      newDuration += GetSifs ();
      WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
      newDuration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, BASIC_BLOCK_ACK);
    }
  else if (m_txParams.MustWaitCompressedBlockAck ())
    {
      newDuration += GetSifs ();
      WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
      newDuration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, COMPRESSED_BLOCK_ACK);
    }
  else if (m_txParams.MustWaitAck ())
    {
      newDuration += GetSifs ();
      newDuration += GetAckDuration (m_currentHdr.GetAddr1 (), m_currentTxVector);
    }
  if (m_txParams.HasNextPacket ())
    {
      if (m_stationManager->GetRifsPermitted ())
        {
          newDuration += GetRifs ();
        }
      else
        {
          newDuration += GetSifs ();
        }      newDuration += m_phy->CalculateTxDuration (m_txParams.GetNextPacketSize (), m_currentTxVector, m_phy->GetFrequency ());
      if (m_txParams.MustWaitCompressedBlockAck ())
        {
          newDuration += GetSifs ();
          WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
          newDuration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, COMPRESSED_BLOCK_ACK);
        }
      else if (m_txParams.MustWaitAck ())
        {
          newDuration += GetSifs ();
          newDuration += GetAckDuration (m_currentHdr.GetAddr1 (), m_currentTxVector);
        }
    }

  Time txDuration = m_phy->CalculateTxDuration (GetSize (m_currentPacket, &m_currentHdr, m_ampdu), m_currentTxVector, m_phy->GetFrequency ());
  duration -= txDuration;
  duration -= GetSifs ();

  duration = std::max (duration, newDuration);
  NS_ASSERT (duration.IsPositive ());
  m_currentHdr.SetDuration (duration);
  Ptr <Packet> packet = m_currentPacket->Copy ();
  if (m_ampdu)
    {
      NS_ASSERT (m_currentHdr.IsQosData ());
    }
  else
    {
      packet->AddHeader (m_currentHdr);
      AddWifiMacTrailer (packet);
    }

  ForwardDown (packet, &m_currentHdr, m_currentTxVector);
}

void
MacLow::WaitIfsAfterEndTxFragment (void)
{
  m_currentDca->StartNextFragment ();
}

void
MacLow::WaitIfsAfterEndTxPacket (void)
{
  m_currentDca->StartNextPacket ();
}

void
MacLow::EndTxNoAck (void)
{
  Ptr<DcaTxop> dca = m_currentDca;
  m_currentDca = 0;
  dca->EndTxNoAck ();
}

void
MacLow::FastAckFailedTimeout (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<DcaTxop> dca = m_currentDca;
  m_currentDca = 0;
  dca->MissedAck ();
  NS_LOG_DEBUG ("fast Ack busy but missed");
}

void
MacLow::SendAckAfterData (Mac48Address source, Time duration, WifiMode dataTxMode, double dataSnr)
{
  NS_LOG_FUNCTION (this);
  // send an ACK, after SIFS, when you receive a packet
  WifiTxVector ackTxVector = GetAckTxVector (source, dataTxMode);
  WifiMacHeader ack;
  ack.SetType (WIFI_MAC_CTL_ACK);
  ack.SetDsNotFrom ();
  ack.SetDsNotTo ();
  ack.SetNoRetry ();
  ack.SetNoMoreFragments ();
  ack.SetAddr1 (source);
  // 802.11-2012, Section 8.3.1.4:  Duration/ID is received duration value
  // minus the time to transmit the ACK frame and its SIFS interval
  duration -= GetAckDuration (ackTxVector);
  duration -= GetSifs ();
  NS_ASSERT_MSG (duration.IsPositive (), "Please provide test case to maintainers if this assert is hit.");
  ack.SetDuration (duration);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (ack);
  AddWifiMacTrailer (packet);

  SnrTag tag;
  tag.Set (dataSnr);
  packet->AddPacketTag (tag);

  //ACK should always use non-HT PPDU (HT PPDU cases not supported yet)
  ForwardDown (packet, &ack, ackTxVector);
}

bool
MacLow::IsInWindow (uint16_t seq, uint16_t winstart, uint16_t winsize)
{
  return ((seq - winstart + 4096) % 4096) < winsize;
}

bool
MacLow::ReceiveMpdu (Ptr<Packet> packet, WifiMacHeader hdr)
{
  if (m_stationManager->HasHtSupported () || m_stationManager->HasVhtSupported ())
    {
      Mac48Address originator = hdr.GetAddr2 ();
      uint8_t tid = 0;
      if (hdr.IsQosData ())
        {
          tid = hdr.GetQosTid ();
        }
      uint16_t seqNumber = hdr.GetSequenceNumber ();
      AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
      if (it != m_bAckAgreements.end ())
        {
          //Implement HT immediate Block Ack support for HT Delayed Block Ack is not added yet
          if (!QosUtilsIsOldPacket ((*it).second.first.GetStartingSequence (), seqNumber))
            {
              StoreMpduIfNeeded (packet, hdr);
              if (!IsInWindow (hdr.GetSequenceNumber (), (*it).second.first.GetStartingSequence (), (*it).second.first.GetBufferSize ()))
                {
                  uint16_t delta = (seqNumber - (*it).second.first.GetWinEnd () + 4096) % 4096;
                  if (delta > 1)
                    {
                      (*it).second.first.SetWinEnd (seqNumber);
                      int16_t winEnd = (*it).second.first.GetWinEnd ();
                      int16_t bufferSize = (*it).second.first.GetBufferSize ();
                      uint16_t sum = ((uint16_t)(std::abs (winEnd - bufferSize + 1))) % 4096;
                      (*it).second.first.SetStartingSequence (sum);
                      RxCompleteBufferedPacketsWithSmallerSequence ((*it).second.first.GetStartingSequenceControl (), originator, tid);
                    }
                }
              RxCompleteBufferedPacketsUntilFirstLost (originator, tid); //forwards up packets starting from winstart and set winstart to last +1
              (*it).second.first.SetWinEnd (((*it).second.first.GetStartingSequence () + (*it).second.first.GetBufferSize () - 1) % 4096);
            }
          return true;
        }
      return false;
    }
  else
    {
      return StoreMpduIfNeeded (packet, hdr);
    }
}

bool
MacLow::StoreMpduIfNeeded (Ptr<Packet> packet, WifiMacHeader hdr)
{
  AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
  if (it != m_bAckAgreements.end ())
    {
      WifiMacTrailer fcs;
      packet->RemoveTrailer (fcs);
      BufferedPacket bufferedPacket (packet, hdr);

      uint16_t endSequence = ((*it).second.first.GetStartingSequence () + 2047) % 4096;
      uint16_t mappedSeqControl = QosUtilsMapSeqControlToUniqueInteger (hdr.GetSequenceControl (), endSequence);

      BufferedPacketI i = (*it).second.second.begin ();
      for (; i != (*it).second.second.end ()
           && QosUtilsMapSeqControlToUniqueInteger ((*i).second.GetSequenceControl (), endSequence) < mappedSeqControl; i++)
        {
        }
      (*it).second.second.insert (i, bufferedPacket);

      //Update block ack cache
      BlockAckCachesI j = m_bAckCaches.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
      NS_ASSERT (j != m_bAckCaches.end ());
      (*j).second.UpdateWithMpdu (&hdr);
      return true;
    }
  return false;
}

void
MacLow::CreateBlockAckAgreement (const MgtAddBaResponseHeader *respHdr, Mac48Address originator,
                                 uint16_t startingSeq)
{
  NS_LOG_FUNCTION (this);
  uint8_t tid = respHdr->GetTid ();
  BlockAckAgreement agreement (originator, tid);
  if (respHdr->IsImmediateBlockAck ())
    {
      agreement.SetImmediateBlockAck ();
    }
  else
    {
      agreement.SetDelayedBlockAck ();
    }
  agreement.SetAmsduSupport (respHdr->IsAmsduSupported ());
  agreement.SetBufferSize (respHdr->GetBufferSize () + 1);
  agreement.SetTimeout (respHdr->GetTimeout ());
  agreement.SetStartingSequence (startingSeq);

  std::list<BufferedPacket> buffer (0);
  AgreementKey key (originator, respHdr->GetTid ());
  AgreementValue value (agreement, buffer);
  m_bAckAgreements.insert (std::make_pair (key, value));

  BlockAckCache cache;
  cache.Init (startingSeq, respHdr->GetBufferSize () + 1);
  m_bAckCaches.insert (std::make_pair (key, cache));

  if (respHdr->GetTimeout () != 0)
    {
      AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, respHdr->GetTid ()));
      Time timeout = MicroSeconds (1024 * agreement.GetTimeout ());

      AcIndex ac = QosUtilsMapTidToAc (agreement.GetTid ());

      it->second.first.m_inactivityEvent = Simulator::Schedule (timeout,
                                                                &EdcaTxopN::SendDelbaFrame,
                                                                m_edca[ac], originator, tid, false);
    }
}

void
MacLow::DestroyBlockAckAgreement (Mac48Address originator, uint8_t tid)
{
  NS_LOG_FUNCTION (this);
  AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
  if (it != m_bAckAgreements.end ())
    {
      RxCompleteBufferedPacketsWithSmallerSequence (it->second.first.GetStartingSequenceControl (), originator, tid);
      RxCompleteBufferedPacketsUntilFirstLost (originator, tid);
      m_bAckAgreements.erase (it);

      BlockAckCachesI i = m_bAckCaches.find (std::make_pair (originator, tid));
      NS_ASSERT (i != m_bAckCaches.end ());
      m_bAckCaches.erase (i);
    }
}

void
MacLow::RxCompleteBufferedPacketsWithSmallerSequence (uint16_t seq, Mac48Address originator, uint8_t tid)
{
  AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
  if (it != m_bAckAgreements.end ())
    {
      uint16_t endSequence = ((*it).second.first.GetStartingSequence () + 2047) % 4096;
      uint16_t mappedStart = QosUtilsMapSeqControlToUniqueInteger (seq, endSequence);
      BufferedPacketI last = (*it).second.second.begin ();
      uint16_t guard = 0;
      if (last != (*it).second.second.end ())
        {
          guard = (*it).second.second.begin ()->second.GetSequenceControl ();
        }
      BufferedPacketI i = (*it).second.second.begin ();
      for (; i != (*it).second.second.end ()
           && QosUtilsMapSeqControlToUniqueInteger ((*i).second.GetSequenceControl (), endSequence) < mappedStart; )
        {
          if (guard == (*i).second.GetSequenceControl ())
            {
              if (!(*i).second.IsMoreFragments ())
                {
                  while (last != i)
                    {
                      m_rxCallback ((*last).first, &(*last).second);
                      last++;
                    }
                  m_rxCallback ((*last).first, &(*last).second);
                  last++;
                  /* go to next packet */
                  while (i != (*it).second.second.end () && guard == (*i).second.GetSequenceControl ())
                    {
                      i++;
                    }
                  if (i != (*it).second.second.end ())
                    {
                      guard = (*i).second.GetSequenceControl ();
                      last = i;
                    }
                }
              else
                {
                  guard++;
                }
            }
          else
            {
              /* go to next packet */
              while (i != (*it).second.second.end () && guard == (*i).second.GetSequenceControl ())
                {
                  i++;
                }
              if (i != (*it).second.second.end ())
                {
                  guard = (*i).second.GetSequenceControl ();
                  last = i;
                }
            }
        }
      (*it).second.second.erase ((*it).second.second.begin (), i);
    }
}

void
MacLow::RxCompleteBufferedPacketsUntilFirstLost (Mac48Address originator, uint8_t tid)
{
  AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
  if (it != m_bAckAgreements.end ())
    {
      uint16_t guard = (*it).second.first.GetStartingSequenceControl ();
      BufferedPacketI lastComplete = (*it).second.second.begin ();
      BufferedPacketI i = (*it).second.second.begin ();
      for (; i != (*it).second.second.end () && guard == (*i).second.GetSequenceControl (); i++)
        {
          if (!(*i).second.IsMoreFragments ())
            {
              while (lastComplete != i)
                {
                  m_rxCallback ((*lastComplete).first, &(*lastComplete).second);
                  lastComplete++;
                }
              m_rxCallback ((*lastComplete).first, &(*lastComplete).second);
              lastComplete++;
            }
          guard = (*i).second.IsMoreFragments () ? (guard + 1) : ((guard + 16) & 0xfff0);
        }
      (*it).second.first.SetStartingSequenceControl (guard);
      /* All packets already forwarded to WifiMac must be removed from buffer:
      [begin (), lastComplete) */
      (*it).second.second.erase ((*it).second.second.begin (), lastComplete);
    }
}
void
MacLow::SendBlockAckResponse (const CtrlBAckResponseHeader* blockAck, Mac48Address originator, bool immediate,
                              Time duration, WifiMode blockAckReqTxMode, double rxSnr)
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (*blockAck);

  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_BACKRESP);
  hdr.SetAddr1 (originator);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoRetry ();
  hdr.SetNoMoreFragments ();

  WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (originator, blockAckReqTxMode);

  if (immediate)
    {
      m_txParams.DisableAck ();
      duration -= GetSifs ();
      if (blockAck->IsBasic ())
        {
          duration -= GetBlockAckDuration (originator, blockAckReqTxVector, BASIC_BLOCK_ACK);
        }
      else if (blockAck->IsCompressed ())
        {
          duration -= GetBlockAckDuration (originator, blockAckReqTxVector, COMPRESSED_BLOCK_ACK);
        }
      else if (blockAck->IsMultiTid ())
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
    }
  else
    {
      m_txParams.EnableAck ();
      duration += GetSifs ();
      duration += GetAckDuration (originator, blockAckReqTxVector);
    }
  m_txParams.DisableNextData ();

  if (!immediate)
    {
      StartDataTxTimers (blockAckReqTxVector);
    }

  NS_ASSERT (duration.IsPositive ());
  hdr.SetDuration (duration);
  //here should be present a control about immediate or delayed block ack
  //for now we assume immediate
  packet->AddHeader (hdr);
  AddWifiMacTrailer (packet);
  SnrTag tag;
  tag.Set (rxSnr);
  packet->AddPacketTag (tag);
  ForwardDown (packet, &hdr, blockAckReqTxVector);
}

void
MacLow::SendBlockAckAfterAmpdu (uint8_t tid, Mac48Address originator, Time duration, WifiTxVector blockAckReqTxVector, double rxSnr)
{
  NS_LOG_FUNCTION (this);
  if (!m_phy->IsStateTx () && !m_phy->IsStateRx ())
    {
      NS_LOG_FUNCTION (this << (uint16_t) tid << originator << duration.As (Time::S) << blockAckReqTxVector << rxSnr);
      CtrlBAckResponseHeader blockAck;
      uint16_t seqNumber = 0;
      BlockAckCachesI i = m_bAckCaches.find (std::make_pair (originator, tid));
      NS_ASSERT (i != m_bAckCaches.end ());
      seqNumber = (*i).second.GetWinStart ();

      bool immediate = true;
      AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
      blockAck.SetStartingSequence (seqNumber);
      blockAck.SetTidInfo (tid);
      immediate = (*it).second.first.IsImmediateBlockAck ();
      blockAck.SetType (COMPRESSED_BLOCK_ACK);
      NS_LOG_DEBUG ("Got Implicit block Ack Req with seq " << seqNumber);
      (*i).second.FillBlockAckBitmap (&blockAck);

      SendBlockAckResponse (&blockAck, originator, immediate, duration, blockAckReqTxVector.GetMode (), rxSnr);
    }
  else
    {
      NS_LOG_DEBUG ("Skip block ack response!");
    }
}

void
MacLow::SendBlockAckAfterBlockAckRequest (const CtrlBAckRequestHeader reqHdr, Mac48Address originator,
                                          Time duration, WifiMode blockAckReqTxMode, double rxSnr)
{
  NS_LOG_FUNCTION (this);
  CtrlBAckResponseHeader blockAck;
  uint8_t tid = 0;
  bool immediate = false;
  if (!reqHdr.IsMultiTid ())
    {
      tid = reqHdr.GetTidInfo ();
      AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
      if (it != m_bAckAgreements.end ())
        {
          blockAck.SetStartingSequence (reqHdr.GetStartingSequence ());
          blockAck.SetTidInfo (tid);
          immediate = (*it).second.first.IsImmediateBlockAck ();
          if (reqHdr.IsBasic ())
            {
              blockAck.SetType (BASIC_BLOCK_ACK);
            }
          else if (reqHdr.IsCompressed ())
            {
              blockAck.SetType (COMPRESSED_BLOCK_ACK);
            }
          BlockAckCachesI i = m_bAckCaches.find (std::make_pair (originator, tid));
          NS_ASSERT (i != m_bAckCaches.end ());
          (*i).second.FillBlockAckBitmap (&blockAck);
          NS_LOG_DEBUG ("Got block Ack Req with seq " << reqHdr.GetStartingSequence ());

          if (!m_stationManager->HasHtSupported () && !m_stationManager->HasVhtSupported ())
            {
              /* All packets with smaller sequence than starting sequence control must be passed up to Wifimac
               * See 9.10.3 in IEEE 802.11e standard.
               */
              RxCompleteBufferedPacketsWithSmallerSequence (reqHdr.GetStartingSequenceControl (), originator, tid);
              RxCompleteBufferedPacketsUntilFirstLost (originator, tid);
            }
          else
            {
              if (!QosUtilsIsOldPacket ((*it).second.first.GetStartingSequence (), reqHdr.GetStartingSequence ()))
                {
                  (*it).second.first.SetStartingSequence (reqHdr.GetStartingSequence ());
                  (*it).second.first.SetWinEnd (((*it).second.first.GetStartingSequence () + (*it).second.first.GetBufferSize () - 1) % 4096);
                  RxCompleteBufferedPacketsWithSmallerSequence (reqHdr.GetStartingSequenceControl (), originator, tid);
                  RxCompleteBufferedPacketsUntilFirstLost (originator, tid);
                  (*it).second.first.SetWinEnd (((*it).second.first.GetStartingSequence () + (*it).second.first.GetBufferSize () - 1) % 4096);
                }
            }
        }
      else
        {
          NS_LOG_DEBUG ("there's not a valid block ack agreement with " << originator);
        }
    }
  else
    {
      NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
    }

  SendBlockAckResponse (&blockAck, originator, immediate, duration, blockAckReqTxMode, rxSnr);
}

void
MacLow::ResetBlockAckInactivityTimerIfNeeded (BlockAckAgreement &agreement)
{
  if (agreement.GetTimeout () != 0)
    {
      NS_ASSERT (agreement.m_inactivityEvent.IsRunning ());
      agreement.m_inactivityEvent.Cancel ();
      Time timeout = MicroSeconds (1024 * agreement.GetTimeout ());
      AcIndex ac = QosUtilsMapTidToAc (agreement.GetTid ());
      agreement.m_inactivityEvent = Simulator::Schedule (timeout,
                                                         &EdcaTxopN::SendDelbaFrame,
                                                         m_edca[ac], agreement.GetPeer (),
                                                         agreement.GetTid (), false);
    }
}

void
MacLow::RegisterEdcaForAc (AcIndex ac, Ptr<EdcaTxopN> edca)
{
  m_edca.insert (std::make_pair (ac, edca));
}

void
MacLow::DeaggregateAmpduAndReceive (Ptr<Packet> aggregatedPacket, double rxSnr, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  AmpduTag ampdu;
  bool normalAck = false;
  bool ampduSubframe = false; //flag indicating the packet belongs to an A-MPDU and is not a VHT/HE single MPDU
  if (aggregatedPacket->RemovePacketTag (ampdu))
    {
      ampduSubframe = true;
      MpduAggregator::DeaggregatedMpdus packets = MpduAggregator::Deaggregate (aggregatedPacket);
      MpduAggregator::DeaggregatedMpdusCI n = packets.begin ();

      WifiMacHeader firsthdr;
      (*n).first->PeekHeader (firsthdr);
      NS_LOG_DEBUG ("duration/id=" << firsthdr.GetDuration ());
      WifiPreamble preamble = txVector.GetPreambleType ();
      NotifyNav ((*n).first, firsthdr, preamble);

      if (firsthdr.GetAddr1 () == m_self)
        {
          bool singleMpdu = (*n).second.GetEof ();
          if (singleMpdu)
            {
              //If the MPDU is sent as a VHT/HE single MPDU (EOF=1 in A-MPDU subframe header), then the responder sends an ACK.
              NS_LOG_DEBUG ("Receive S-MPDU");
              ampduSubframe = false;
            }
          else if (preamble != WIFI_PREAMBLE_NONE || !m_sendAckEvent.IsRunning ())
            {
              m_sendAckEvent = Simulator::Schedule (ampdu.GetRemainingAmpduDuration () + GetSifs (),
                                                    &MacLow::SendBlockAckAfterAmpdu, this,
                                                    firsthdr.GetQosTid (),
                                                    firsthdr.GetAddr2 (),
                                                    firsthdr.GetDuration (),
                                                    txVector,
                                                    rxSnr);
            }

          if (firsthdr.IsAck () || firsthdr.IsBlockAck () || firsthdr.IsBlockAckReq ())
            {
              ReceiveOk ((*n).first, rxSnr, txVector, ampduSubframe);
            }
          else if (firsthdr.IsData () || firsthdr.IsQosData ())
            {
              NS_LOG_DEBUG ("Deaggregate packet from " << firsthdr.GetAddr2 () << " with sequence=" << firsthdr.GetSequenceNumber ());
              ReceiveOk ((*n).first, rxSnr, txVector, ampduSubframe);
              if (firsthdr.IsQosAck ())
                {
                  NS_LOG_DEBUG ("Normal Ack");
                  normalAck = true;
                }
            }
          else
            {
              NS_FATAL_ERROR ("Received A-MPDU with invalid first MPDU type");
            }

          if (ampdu.GetRemainingNbOfMpdus () == 0 && !singleMpdu)
            {
              if (normalAck)
                {
                  //send block Ack
                  if (firsthdr.IsBlockAckReq ())
                    {
                      NS_FATAL_ERROR ("Sending a BlockAckReq with QosPolicy equal to Normal Ack");
                    }
                  uint8_t tid = firsthdr.GetQosTid ();
                  AgreementsI it = m_bAckAgreements.find (std::make_pair (firsthdr.GetAddr2 (), tid));
                  if (it != m_bAckAgreements.end ())
                    {
                      /* See section 11.5.3 in IEEE 802.11 for mean of this timer */
                      ResetBlockAckInactivityTimerIfNeeded (it->second.first);
                      NS_LOG_DEBUG ("rx A-MPDU/sendImmediateBlockAck from=" << firsthdr.GetAddr2 ());
                      NS_ASSERT (m_sendAckEvent.IsRunning ());
                    }
                  else
                    {
                      NS_LOG_DEBUG ("There's not a valid agreement for this block ack request.");
                    }
                }
            }
        }
    }
  else
    {
      ReceiveOk (aggregatedPacket, rxSnr, txVector, ampduSubframe);
    }
}

bool
MacLow::StopMpduAggregation (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHdr, Ptr<Packet> aggregatedPacket, uint16_t size) const
{
  if (peekedPacket == 0)
    {
      NS_LOG_DEBUG ("no more packets in queue");
      return true;
    }

  Time aPPDUMaxTime = MicroSeconds (5484);
  uint8_t tid = GetTid (peekedPacket, peekedHdr);
  AcIndex ac = QosUtilsMapTidToAc (tid);
  std::map<AcIndex, Ptr<EdcaTxopN> >::const_iterator edcaIt = m_edca.find (ac);

  if (m_phy->GetGreenfield ())
    {
      aPPDUMaxTime = MicroSeconds (10000);
    }

  //A STA shall not transmit a PPDU that has a duration that is greater than aPPDUMaxTime
  if (m_phy->CalculateTxDuration (aggregatedPacket->GetSize () + peekedPacket->GetSize () + peekedHdr.GetSize () + WIFI_MAC_FCS_LENGTH, m_currentTxVector, m_phy->GetFrequency ()) > aPPDUMaxTime)
    {
      NS_LOG_DEBUG ("no more packets can be aggregated to satisfy PPDU <= aPPDUMaxTime");
      return true;
    }

  if (!edcaIt->second->GetMpduAggregator ()->CanBeAggregated (peekedPacket->GetSize () + peekedHdr.GetSize () + WIFI_MAC_FCS_LENGTH, aggregatedPacket, size))
    {
      NS_LOG_DEBUG ("no more packets can be aggregated because the maximum A-MPDU size has been reached");
      return true;
    }

  return false;
}

Ptr<Packet>
MacLow::AggregateToAmpdu (Ptr<const Packet> packet, const WifiMacHeader hdr)
{
  bool isAmpdu = false;
  Ptr<Packet> newPacket, tempPacket;
  WifiMacHeader peekedHdr;
  newPacket = packet->Copy ();
  Ptr<Packet> currentAggregatedPacket;
  CtrlBAckRequestHeader blockAckReq;

  if (hdr.IsBlockAckReq ())
    {
      //Workaround to avoid BlockAckReq to be part of an A-MPDU. The standard says that
      //BlockAckReq is not present in A-MPDU if any QoS data frames for that TID are present.
      //Since an A-MPDU in non-PSMP frame exchanges aggregates MPDUs from one TID, this means
      //we should stop aggregation here for single-TID A-MPDUs. Once PSMP and multi-TID A-MPDUs
      //are supported, the condition of entering here should be changed.
      return newPacket;
    }

  //missing hdr.IsAck() since we have no means of knowing the Tid of the Ack yet
  if (hdr.IsQosData () || hdr.IsBlockAck ()|| hdr.IsBlockAckReq ())
    {
      Time tstamp;
      uint8_t tid = GetTid (packet, hdr);
      Ptr<WifiMacQueue> queue;
      AcIndex ac = QosUtilsMapTidToAc (tid);
      std::map<AcIndex, Ptr<EdcaTxopN> >::const_iterator edcaIt = m_edca.find (ac);
      NS_ASSERT (edcaIt != m_edca.end ());
      queue = edcaIt->second->GetQueue ();

      if (!hdr.GetAddr1 ().IsBroadcast () && edcaIt->second->GetMpduAggregator () != 0)
        {
          //Have to make sure that their exist a block Ack agreement before sending an AMPDU (BlockAck Manager)
          if (edcaIt->second->GetBaAgreementExists (hdr.GetAddr1 (), tid))
            {
              /* here is performed mpdu aggregation */
              /* MSDU aggregation happened in edca if the user asked for it so m_currentPacket may contains a normal packet or a A-MSDU*/
              currentAggregatedPacket = Create<Packet> ();
              peekedHdr = hdr;
              uint16_t startingSequenceNumber = 0;
              uint16_t currentSequenceNumber = 0;
              uint8_t qosPolicy = 0;
              uint16_t blockAckSize = 0;
              bool aggregated = false;
              int i = 0;
              Ptr<Packet> aggPacket = newPacket->Copy ();

              if (!hdr.IsBlockAckReq ())
                {
                  if (!hdr.IsBlockAck ())
                    {
                      startingSequenceNumber = peekedHdr.GetSequenceNumber ();
                      peekedHdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
                    }
                  currentSequenceNumber = peekedHdr.GetSequenceNumber ();
                  newPacket->AddHeader (peekedHdr);
                  AddWifiMacTrailer (newPacket);

                  aggregated = edcaIt->second->GetMpduAggregator ()->Aggregate (newPacket, currentAggregatedPacket);

                  if (aggregated)
                    {
                      NS_LOG_DEBUG ("Adding packet with sequence number " << currentSequenceNumber << " to A-MPDU, packet size = " << newPacket->GetSize () << ", A-MPDU size = " << currentAggregatedPacket->GetSize ());
                      i++;
                      m_aggregateQueue[tid]->Enqueue (Create<WifiMacQueueItem> (aggPacket, peekedHdr));
                    }
                }
              else if (hdr.IsBlockAckReq ())
                {
                  blockAckSize = packet->GetSize () + hdr.GetSize () + WIFI_MAC_FCS_LENGTH;
                  qosPolicy = 3; //if the last subrame is block ack req then set ack policy of all frames to blockack
                  packet->PeekHeader (blockAckReq);
                  startingSequenceNumber = blockAckReq.GetStartingSequence ();
                }
              /// \todo We should also handle Ack and BlockAck
              bool retry = false;
              //looks for other packets to the same destination with the same Tid need to extend that to include MSDUs
              Ptr<const Packet> peekedPacket = edcaIt->second->PeekNextRetransmitPacket (peekedHdr, peekedHdr.GetAddr1 (), tid, &tstamp);
              if (peekedPacket == 0)
                {
                  Ptr<const WifiMacQueueItem> item = queue->PeekByTidAndAddress (tid,
                                                                                 WifiMacHeader::ADDR1,
                                                                                 hdr.GetAddr1 ());
                  if (item)
                    {
                      peekedPacket = item->GetPacket ();
                      peekedHdr = item->GetHeader ();
                      tstamp = item->GetTimeStamp ();
                    }
                  currentSequenceNumber = edcaIt->second->PeekNextSequenceNumberFor (&peekedHdr);

                  /* here is performed MSDU aggregation (two-level aggregation) */
                  if (peekedPacket != 0 && edcaIt->second->GetMsduAggregator () != 0)
                    {
                      tempPacket = PerformMsduAggregation (peekedPacket, &peekedHdr, &tstamp, currentAggregatedPacket, blockAckSize);
                      if (tempPacket != 0)  //MSDU aggregation
                        {
                          peekedPacket = tempPacket->Copy ();
                        }
                    }
                }
              else
                {
                  retry = true;
                  currentSequenceNumber = peekedHdr.GetSequenceNumber ();
                }

              while (IsInWindow (currentSequenceNumber, startingSequenceNumber, 64) && !StopMpduAggregation (peekedPacket, peekedHdr, currentAggregatedPacket, blockAckSize))
                {
                  //for now always send AMPDU with normal ACK
                  if (retry == false)
                    {
                      currentSequenceNumber = edcaIt->second->GetNextSequenceNumberFor (&peekedHdr);
                      peekedHdr.SetSequenceNumber (currentSequenceNumber);
                      peekedHdr.SetFragmentNumber (0);
                      peekedHdr.SetNoMoreFragments ();
                      peekedHdr.SetNoRetry ();
                    }
                  if (qosPolicy == 0)
                    {
                      peekedHdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
                    }
                  else
                    {
                      peekedHdr.SetQosAckPolicy (WifiMacHeader::BLOCK_ACK);
                    }

                  newPacket = peekedPacket->Copy ();
                  Ptr<Packet> aggPacket = newPacket->Copy ();

                  newPacket->AddHeader (peekedHdr);
                  AddWifiMacTrailer (newPacket);
                  aggregated = edcaIt->second->GetMpduAggregator ()->Aggregate (newPacket, currentAggregatedPacket);
                  if (aggregated)
                    {
                      m_aggregateQueue[tid]->Enqueue (Create<WifiMacQueueItem> (aggPacket, peekedHdr));
                      if (i == 1 && hdr.IsQosData ())
                        {
                          if (!m_txParams.MustSendRts ())
                            {
                              edcaIt->second->CompleteMpduTx (packet, hdr, tstamp);
                            }
                          else
                            {
                              InsertInTxQueue (packet, hdr, tstamp, tid);
                            }
                        }
                      NS_LOG_DEBUG ("Adding packet with sequence number " << peekedHdr.GetSequenceNumber () << " to A-MPDU, packet size = " << newPacket->GetSize () << ", A-MPDU size = " << currentAggregatedPacket->GetSize ());
                      i++;
                      isAmpdu = true;
                      if (!m_txParams.MustSendRts ())
                        {
                          edcaIt->second->CompleteMpduTx (peekedPacket, peekedHdr, tstamp);
                        }
                      else
                        {
                          InsertInTxQueue (peekedPacket, peekedHdr, tstamp, tid);
                        }
                      if (retry)
                        {
                          edcaIt->second->RemoveRetransmitPacket (tid, hdr.GetAddr1 (), peekedHdr.GetSequenceNumber ());
                        }
                      else
                        {
                          queue->Remove (peekedPacket);
                        }
                      newPacket = 0;
                    }
                  else
                    {
                      break;
                    }
                  if (retry == true)
                    {
                      peekedPacket = edcaIt->second->PeekNextRetransmitPacket (peekedHdr, hdr.GetAddr1 (), tid, &tstamp);
                      if (peekedPacket == 0)
                        {
                          //I reached the first packet that I added to this A-MPDU
                          retry = false;
                          Ptr<const WifiMacQueueItem> item = queue->PeekByTidAndAddress (tid,
                                                                                         WifiMacHeader::ADDR1,
                                                                                         hdr.GetAddr1 ());
                          if (item != 0)
                            {
                              peekedPacket = item->GetPacket ();
                              peekedHdr = item->GetHeader ();
                              tstamp = item->GetTimeStamp ();
                              //find what will the sequence number be so that we don't send more than 64 packets apart
                              currentSequenceNumber = edcaIt->second->PeekNextSequenceNumberFor (&peekedHdr);

                              if (edcaIt->second->GetMsduAggregator () != 0)
                                {
                                  tempPacket = PerformMsduAggregation (peekedPacket, &peekedHdr, &tstamp, currentAggregatedPacket, blockAckSize);
                                  if (tempPacket != 0) //MSDU aggregation
                                    {
                                      peekedPacket = tempPacket->Copy ();
                                    }
                                }
                            }
                        }
                      else
                        {
                          currentSequenceNumber = peekedHdr.GetSequenceNumber ();
                        }
                    }
                  else
                    {
                      Ptr<const WifiMacQueueItem> item = queue->PeekByTidAndAddress (tid,
                                                                 WifiMacHeader::ADDR1, hdr.GetAddr1 ());
                      if (item != 0)
                        {
                          peekedPacket = item->GetPacket ();
                          peekedHdr = item->GetHeader ();
                          tstamp = item->GetTimeStamp ();
                          //find what will the sequence number be so that we don't send more than 64 packets apart
                          currentSequenceNumber = edcaIt->second->PeekNextSequenceNumberFor (&peekedHdr);

                          if (edcaIt->second->GetMsduAggregator () != 0 && IsInWindow (currentSequenceNumber, startingSequenceNumber, 64))
                            {
                              tempPacket = PerformMsduAggregation (peekedPacket, &peekedHdr, &tstamp, currentAggregatedPacket, blockAckSize);
                              if (tempPacket != 0) //MSDU aggregation
                                {
                                  peekedPacket = tempPacket->Copy ();
                                }
                            }
                        }
                      else
                        {
                          peekedPacket = 0;
                        }
                    }
                }

              if (isAmpdu)
                {
                  if (hdr.IsBlockAckReq ())
                    {
                      newPacket = packet->Copy ();
                      peekedHdr = hdr;
                      Ptr<Packet> aggPacket = newPacket->Copy ();
                      m_aggregateQueue[tid]->Enqueue (Create<WifiMacQueueItem> (aggPacket, peekedHdr));
                      newPacket->AddHeader (peekedHdr);
                      AddWifiMacTrailer (newPacket);
                      edcaIt->second->GetMpduAggregator ()->Aggregate (newPacket, currentAggregatedPacket);
                      currentAggregatedPacket->AddHeader (blockAckReq);
                    }

                  if (qosPolicy == 0)
                    {
                      edcaIt->second->CompleteAmpduTransfer (hdr.GetAddr1 (), tid);
                    }

                  //Add packet tag
                  AmpduTag ampdutag;
                  ampdutag.SetAmpdu (true);
                  ampdutag.SetRemainingNbOfMpdus (i - 1);
                  newPacket = currentAggregatedPacket;
                  newPacket->AddPacketTag (ampdutag);

                  NS_LOG_DEBUG ("tx unicast A-MPDU");
                  edcaIt->second->SetAmpduExist (hdr.GetAddr1 (), true);
                }
              else
                {
                  uint8_t queueSize = m_aggregateQueue[tid]->GetNPackets ();
                  NS_ASSERT (queueSize <= 2); //since it is not an A-MPDU then only 2 packets should have been added to the queue no more
                  if (queueSize >= 1)
                    {
                      //remove any packets that we added to the aggregate queue
                      FlushAggregateQueue (tid);
                    }
                }
            }
          // VHT/HE single MPDU operation
          WifiTxVector dataTxVector = GetDataTxVector (m_currentPacket, &m_currentHdr);
          if (!isAmpdu
              && hdr.IsQosData ()
              && (dataTxVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_VHT
                  || dataTxVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HE))
            {
              peekedHdr = hdr;
              peekedHdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);

              currentAggregatedPacket = Create<Packet> ();
              edcaIt->second->GetMpduAggregator ()->AggregateSingleMpdu (packet, currentAggregatedPacket);
              m_aggregateQueue[tid]->Enqueue (Create<WifiMacQueueItem> (packet, peekedHdr));
              if (m_txParams.MustSendRts ())
                {
                  InsertInTxQueue (packet, peekedHdr, tstamp, tid);
                }
              if (edcaIt->second->GetBaAgreementExists (hdr.GetAddr1 (), tid))
                {
                  edcaIt->second->CompleteAmpduTransfer (peekedHdr.GetAddr1 (), tid);
                }

              //Add packet tag
              AmpduTag ampdutag;
              ampdutag.SetAmpdu (true);
              newPacket = currentAggregatedPacket;
              newPacket->AddHeader (peekedHdr);
              AddWifiMacTrailer (newPacket);
              newPacket->AddPacketTag (ampdutag);

              NS_LOG_DEBUG ("tx unicast S-MPDU with sequence number " << hdr.GetSequenceNumber ());
              edcaIt->second->SetAmpduExist (hdr.GetAddr1 (), true);
            }
        }
    }
  return newPacket;
}

void
MacLow::FlushAggregateQueue (uint8_t tid)
{
  if (!m_aggregateQueue[tid]->IsEmpty ())
    {
      NS_LOG_DEBUG ("Flush aggregate queue");
      m_aggregateQueue[tid]->Flush ();
    }
  m_txPackets[tid].clear ();
}

void
MacLow::InsertInTxQueue (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time tStamp, uint8_t tid)
{
  NS_LOG_FUNCTION (this);
  Item item;
  item.packet = packet;
  item.hdr = hdr;
  item.timestamp = tStamp;
  m_txPackets[tid].push_back (item);
}

Ptr<Packet>
MacLow::PerformMsduAggregation (Ptr<const Packet> packet, WifiMacHeader *hdr, Time *tstamp, Ptr<Packet> currentAmpduPacket, uint16_t blockAckSize)
{
  bool msduAggregation = false;
  bool isAmsdu = false;
  Ptr<Packet> currentAmsduPacket = Create<Packet> ();
  Ptr<Packet> tempPacket = Create<Packet> ();

  Ptr<WifiMacQueue> queue;
  AcIndex ac = QosUtilsMapTidToAc (GetTid (packet, *hdr));
  std::map<AcIndex, Ptr<EdcaTxopN> >::const_iterator edcaIt = m_edca.find (ac);
  NS_ASSERT (edcaIt != m_edca.end ());
  queue = edcaIt->second->GetQueue ();

  Ptr<const WifiMacQueueItem> peekedItem = queue->DequeueByTidAndAddress (hdr->GetQosTid (),
                                                                          WifiMacHeader::ADDR1,
                                                                          hdr->GetAddr1 ());
  if (peekedItem)
    {
      *hdr = peekedItem->GetHeader ();
    }

  edcaIt->second->GetMsduAggregator ()->Aggregate (packet, currentAmsduPacket,
                                                   edcaIt->second->MapSrcAddressForAggregation (*hdr),
                                                   edcaIt->second->MapDestAddressForAggregation (*hdr));

  peekedItem = queue->PeekByTidAndAddress (hdr->GetQosTid (), WifiMacHeader::ADDR1, hdr->GetAddr1 ());
  while (peekedItem != 0)
    {
      *hdr = peekedItem->GetHeader ();
      *tstamp = peekedItem->GetTimeStamp ();
      tempPacket = currentAmsduPacket;

      msduAggregation = edcaIt->second->GetMsduAggregator ()->Aggregate (peekedItem->GetPacket (), tempPacket,
                                                                         edcaIt->second->MapSrcAddressForAggregation (*hdr),
                                                                         edcaIt->second->MapDestAddressForAggregation (*hdr));

      if (msduAggregation && !StopMpduAggregation (tempPacket, *hdr, currentAmpduPacket, blockAckSize))
        {
          isAmsdu = true;
          currentAmsduPacket = tempPacket;
          queue->Remove (peekedItem->GetPacket ());
        }
      else
        {
          break;
        }
      peekedItem = queue->PeekByTidAndAddress (hdr->GetQosTid (), WifiMacHeader::ADDR1, hdr->GetAddr1 ());
    }

  if (isAmsdu)
    {
      NS_LOG_DEBUG ("A-MSDU with size = " << currentAmsduPacket->GetSize ());
      hdr->SetQosAmsdu ();
      hdr->SetAddr3 (GetBssid ());
      return currentAmsduPacket;
    }
  else
    {
      queue->PushFront (Create<WifiMacQueueItem> (packet, *hdr));
      return 0;
    }
}

void
MacLow::AddWifiMacTrailer (Ptr<Packet> packet)
{
  WifiMacTrailer fcs;
  packet->AddTrailer (fcs);
}

} //namespace ns3
