/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2014 University of Washington
 *               2015 Universita' degli Studi di Napoli Federico II
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
 */

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "queue-disc.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QueueDisc");

QueueDiscItem::QueueDiscItem (Ptr<Packet> p, const Ipv4Header& hdr, const Address& addr, uint16_t protocol, uint8_t txq)
  : QueueItem (p),
    m_ipv4Header (hdr),
    m_headerType (IPV4_HEADER),
    m_address (addr),
    m_protocol (protocol),
    m_txq (txq)
{
}

QueueDiscItem::QueueDiscItem (Ptr<Packet> p, const Ipv6Header& hdr, const Address& addr, uint16_t protocol, uint8_t txq)
  : QueueItem (p),
    m_ipv6Header (hdr),
    m_headerType (IPV6_HEADER),
    m_address (addr),
    m_protocol (protocol),
    m_txq (txq)
{
}

QueueDiscItem::~QueueDiscItem()
{
  NS_LOG_FUNCTION (this);
}

QueueDiscItem::HeaderType
QueueDiscItem::GetHeaderType (void) const
{
  return m_headerType;
}

const Header &
QueueDiscItem::GetHeader (void) const
{
  if (m_headerType == IPV4_HEADER)
    {
      return m_ipv4Header;
    }
  if (m_headerType == IPV6_HEADER)
    {
      return m_ipv6Header;
    }
  NS_ASSERT (false);
}

Address
QueueDiscItem::GetAddress (void) const
{
  return m_address;
}

uint16_t
QueueDiscItem::GetProtocol (void) const
{
  return m_protocol;
}

uint16_t
QueueDiscItem::GetTxq (void) const
{
  return m_txq;
}

void
QueueDiscItem::Print (std::ostream& os) const
{
  os << GetPacket () << " "
     << "Dst addr " << m_address << " "
     << "proto " << m_protocol << " "
     << "txq " << m_txq
  ;
}


NS_OBJECT_ENSURE_REGISTERED (QueueDisc);

TypeId QueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QueueDisc")
    .SetParent<Object> ()
    .AddAttribute ("Quota", "The maximum number of packets dequeued in a qdisc run",
                   UintegerValue (DEFAULT_QUOTA),
                   MakeUintegerAccessor (&QueueDisc::SetQuota,
                                         &QueueDisc::GetQuota),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Enqueue", "Enqueue a packet in the queue disc",
                     MakeTraceSourceAccessor (&QueueDisc::m_traceEnqueue),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("Dequeue", "Dequeue a packet from the queue disc",
                     MakeTraceSourceAccessor (&QueueDisc::m_traceDequeue),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("Requeue", "Requeue a packet in the queue disc",
                     MakeTraceSourceAccessor (&QueueDisc::m_traceRequeue),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("Drop", "Drop a packet stored in the queue disc",
                     MakeTraceSourceAccessor (&QueueDisc::m_traceDrop),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PacketsInQueue",
                     "Number of packets currently stored in the queue disc",
                     MakeTraceSourceAccessor (&QueueDisc::m_nPackets),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("BytesInQueue",
                     "Number of bytes currently stored in the queue disc",
                     MakeTraceSourceAccessor (&QueueDisc::m_nBytes),
                     "ns3::TracedValueCallback::Uint32")
  ;
  return tid;
}

QueueDisc::QueueDisc ()
  :  m_nPackets (0),
     m_nBytes (0),
     m_nTotalReceivedPackets (0),
     m_nTotalReceivedBytes (0),
     m_nTotalDroppedPackets (0),
     m_nTotalDroppedBytes (0),
     m_nTotalRequeuedPackets (0),
     m_nTotalRequeuedBytes (0),
     m_running (false)
{
  NS_LOG_FUNCTION (this);
}

uint32_t
QueueDisc::GetNPackets () const
{
  NS_LOG_FUNCTION (this);
  return m_nPackets;
}

uint32_t
QueueDisc::GetNBytes (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nBytes;
}

uint32_t
QueueDisc::GetTotalReceivedPackets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nTotalReceivedPackets;
}

uint32_t
QueueDisc::GetTotalReceivedBytes (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nTotalReceivedBytes;
}

uint32_t
QueueDisc::GetTotalDroppedPackets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nTotalDroppedPackets;
}

uint32_t
QueueDisc:: GetTotalDroppedBytes (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nTotalDroppedBytes;
}

uint32_t
QueueDisc::GetTotalRequeuedPackets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nTotalRequeuedPackets;
}

uint32_t
QueueDisc:: GetTotalRequeuedBytes (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nTotalRequeuedBytes;
}

void
QueueDisc::SetNetDeviceQueue (Ptr<NetDeviceQueue> devQueue)
{
  NS_LOG_FUNCTION (this << devQueue);
  m_devQueue = devQueue;
}

Ptr<NetDeviceQueue>
QueueDisc::GetNetDeviceQueue (void) const
{
  NS_LOG_FUNCTION (this);
  return m_devQueue;
}

void
QueueDisc::SetWakeCbOnTxQueue (void)
{
  if (m_devQueue == 0)
    {
      NS_LOG_WARN ("Cannot set the wake callback because m_devQueue is not set.");
      return;
    }

  m_devQueue->SetWakeCallback (MakeCallback (&QueueDisc::Run, this));
}

void
QueueDisc::SetWakeCbOnAllTxQueues (void)
{
  if (m_devQueue == 0)
    {
      NS_LOG_WARN ("Cannot set the wake callback because m_devQueue is not set.");
      return;
    }

  Ptr<NetDeviceQueue> devQueue;
  for (uint8_t i = 0; i < m_devQueue->GetDevice ()->GetTxQueuesN (); i++)
    {
      devQueue = m_devQueue->GetDevice ()->GetTxQueue (i);
      devQueue->SetWakeCallback (MakeCallback (&QueueDisc::Run, this));
    }
}

void
QueueDisc::SetQuota (const uint32_t quota)
{
  NS_LOG_FUNCTION (this << quota);
  m_quota = quota;
}

uint32_t
QueueDisc::GetQuota (void) const
{
  NS_LOG_FUNCTION (this);
  return m_quota;
}

void
QueueDisc::Drop (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  m_nTotalDroppedPackets++;
  m_nTotalDroppedBytes += p->GetSize ();

  NS_LOG_LOGIC ("m_traceDrop (p)");
  m_traceDrop (p);
}

bool
QueueDisc::Enqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);
  bool ret = DoEnqueue (item);
  if (ret)
    {
      Ptr<Packet> p = item->GetPacket ();
      m_nPackets++;
      m_nBytes += p->GetSize ();
      m_nTotalReceivedPackets++;
      m_nTotalReceivedBytes += p->GetSize ();

      NS_LOG_LOGIC ("m_traceEnqueue (p)");
      m_traceEnqueue (p);
    }

  return ret;
}

Ptr<QueueDiscItem>
QueueDisc::Dequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item;
  item = DoDequeue ();

  if (item != 0)
    {
      Ptr<Packet> p = item->GetPacket ();
      m_nPackets--;
      m_nBytes -= p->GetSize ();

      NS_LOG_LOGIC ("m_traceDequeue (p)");
      m_traceDequeue (p);
    }

  return item;
}

Ptr<const QueueDiscItem>
QueueDisc::Peek (void) const
{
  NS_LOG_FUNCTION (this);
  return DoPeek ();
}

void
QueueDisc::Run (void)
{
  NS_LOG_FUNCTION (this);

  if (RunBegin ())
    {
      uint32_t quota = m_quota;
      while (Restart ())
        {
          quota -= 1;
          if (quota <= 0)
            {
              /// \todo netif_schedule (q);
              break;
            }
        }
      RunEnd ();
    }
}

bool
QueueDisc::RunBegin (void)
{
  NS_LOG_FUNCTION (this);
  if (m_running)
    {
      return false;
    }

  m_running = true;
  return true;
}

void
QueueDisc::RunEnd (void)
{
  NS_LOG_FUNCTION (this);
  m_running = false;
}

bool
QueueDisc::Restart (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<QueueDiscItem> item = DequeuePacket();
  if (item == 0)
    {
      NS_LOG_LOGIC ("No packet to send");
      return false;
    }

  return Transmit (item);
}

Ptr<QueueDiscItem>
QueueDisc::DequeuePacket ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_devQueue);
  Ptr<QueueDiscItem> item;

  // First check if there is a requeued packet
  if (m_requeued != 0)
    {
        // If the queue where the requeued packet is destined to is not stopped, return
        // the requeued packet; otherwise, return an empty packet.
        // Note that this function is only called on root queue discs and hence
        // m_devQueue does not point to the transmission queue associated to this
        // queue disc.
        if (!m_devQueue->GetDevice ()->GetTxQueue (m_requeued->GetTxq ())->IsStopped ())
          {
            item = m_requeued;
            m_requeued = 0;

            m_nPackets--;
            m_nBytes -= item->GetPacket ()->GetSize ();

            NS_LOG_LOGIC ("m_traceDequeue (p)");
            m_traceDequeue (item->GetPacket ());
          }
    }
  else
    {
      // If the device is multi-queue (actually, Linux checks if the queue disc has
      // multiple queues), ask the queue disc to dequeue a packet (a multi-queue aware
      // queue disc should try not to dequeue a packet destined to a stopped queue).
      // Otherwise, ask the queue disc to dequeue a packet only if the (unique) queue
      // is not stopped.
      if (m_devQueue->GetDevice ()->GetTxQueuesN ()>1 || !m_devQueue->IsStopped ())
        {
          item = Dequeue ();
          // If the item is not null, add the IP header to the packet.
          if (item != 0)
            {
              item->GetPacket ()->AddHeader (item->GetHeader ());
            }
          // Here, Linux tries bulk dequeues
        }
    }
  return item;
}

void
QueueDisc::Requeue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);
  m_requeued = item;
  /// \todo netif_schedule (q);

  Ptr<Packet> p = item->GetPacket ();
  m_nPackets++;       // it's still part of the queue
  m_nBytes += p->GetSize ();
  m_nTotalRequeuedPackets++;
  m_nTotalRequeuedBytes += p->GetSize ();

  NS_LOG_LOGIC ("m_traceRequeue (p)");
  m_traceRequeue (p);
}

bool
QueueDisc::Transmit (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);
  NS_ASSERT (m_devQueue);
  bool ret = false;

  if (!m_devQueue->GetDevice ()->GetTxQueue (item->GetTxq ())->IsStopped ())
    {
      ret = m_devQueue->GetDevice ()->Send (item->GetPacket (), item->GetAddress (), item->GetProtocol ());
    }

  // If the transmission did not happen or failed, requeue the item
  if (!ret)
    {
      Requeue (item);
    }

  // If the transmission succeeded but now the queue is stopped, return false
  if (ret && m_devQueue->GetDevice ()->GetTxQueue (item->GetTxq ())->IsStopped ())
    {
      ret = false;
    }

  return ret;
}

} // namespace ns3
