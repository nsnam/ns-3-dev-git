/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 The Boeing Company
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
 * Author: Gary Pei <guangyu.pei@boeing.com>
 *
 * Updated by Tom Henderson, Rohan Patidar, Hao Yin and Sébastien Deronne
 */

// This program conducts a Bianchi analysis of a wifi network.
// It currently only supports 11a/b/g, and will be later extended
// to support 11n/ac/ax, including frame aggregation settings.

#include <fstream>
#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/gnuplot.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "ns3/command-line.h"
#include "ns3/node-list.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/queue-size.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/mobility-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-server.h"
#include "ns3/application-container.h"

#define PI 3.1415926535

NS_LOG_COMPONENT_DEFINE ("WifiBianchi");

using namespace ns3;

std::ofstream cwTraceFile;         ///< File that traces CW over time
std::ofstream backoffTraceFile;    ///< File that traces backoff over time
std::ofstream phyTxTraceFile;      ///< File that traces PHY transmissions  over time
std::ofstream macTxTraceFile;      ///< File that traces MAC transmissions  over time
std::ofstream macRxTraceFile;      ///< File that traces MAC receptions  over time
std::ofstream socketSendTraceFile; ///< File that traces packets transmitted by the application  over time

std::map<Mac48Address, uint64_t> packetsReceived;              ///< Map that stores the total packets received per STA (and addressed to that STA)
std::map<Mac48Address, uint64_t> bytesReceived;                ///< Map that stores the total bytes received per STA (and addressed to that STA)
std::map<Mac48Address, uint64_t> packetsTransmitted;           ///< Map that stores the total packets transmitted per STA
std::map<Mac48Address, uint64_t> psduFailed;                   ///< Map that stores the total number of unsuccessfully received PSDUS (for which the PHY header was successfully received)  per STA (including PSDUs not addressed to that STA)
std::map<Mac48Address, uint64_t> psduSucceeded;                ///< Map that stores the total number of successfully received PSDUs per STA (including PSDUs not addressed to that STA)
std::map<Mac48Address, uint64_t> phyHeaderFailed;              ///< Map that stores the total number of unsuccessfully received PHY headers per STA
std::map<Mac48Address, uint64_t> rxEventWhileTxing;            ///< Map that stores the number of reception events per STA that occured while PHY was already transmitting a PPDU
std::map<Mac48Address, uint64_t> rxEventWhileRxing;            ///< Map that stores the number of reception events per STA that occured while PHY was already receiving a PPDU
std::map<Mac48Address, uint64_t> rxEventWhileDecodingPreamble; ///< Map that stores the number of reception events per STA that occured while PHY was already decoding a preamble
std::map<Mac48Address, uint64_t> rxEventAbortedByTx;           ///< Map that stores the number of reception events aborted per STA because the PHY has started to transmit

std::map<Mac48Address, Time> timeFirstReceived;    ///< Map that stores the time at which the first packet was received per STA (and the packet is addressed to that STA)
std::map<Mac48Address, Time> timeLastReceived;     ///< Map that stores the time at which the last packet was received per STA (and the packet is addressed to that STA)
std::map<Mac48Address, Time> timeFirstTransmitted; ///< Map that stores the time at which the first packet was transmitted per STA
std::map<Mac48Address, Time> timeLastTransmitted;  ///< Map that stores the time at which the last packet was transmitted per STA

std::set<uint32_t> associated; ///< Contains the IDs of the STAs that successfully associated to the access point (in infrastructure mode only)

bool tracing = false;    ///< Flag to enable/disable generation of tracing files
uint32_t pktSize = 1500; ///< packet size used for the simulation (in bytes)

std::map<unsigned int /* rate (bit/s)*/, std::map<unsigned int /* number of nodes */, double /* calculated throughput */> > bianchiResultsEifs =
{
/* 11b */
    {1000000, {
        {5, 0.8418},
        {10, 0.7831},
        {15, 0.7460},
        {20, 0.7186},
        {25, 0.6973},
        {30, 0.6802},
        {35, 0.6639},
        {40, 0.6501},
        {45, 0.6386},
        {50, 0.6285},
    }},
    {2000000, {
        {5, 1.6170},
        {10, 1.5075},
        {15, 1.4371},
        {20, 1.3849},
        {25, 1.3442},
        {30, 1.3115},
        {35, 1.2803},
        {40, 1.2538},
        {45, 1.2317},
        {50, 1.2124},
    }},
    {5500000, {
        {5, 3.8565},
        {10, 3.6170},
        {15, 3.4554},
        {20, 3.3339},
        {25, 3.2385},
        {30, 3.1613},
        {35, 3.0878},
        {40, 3.0249},
        {45, 2.9725},
        {50, 2.9266},
    }},
    {11000000, {
        {5, 6.3821},
        {10, 6.0269},
        {15, 5.7718},
        {20, 5.5765},
        {25, 5.4217},
        {30, 5.2958},
        {35, 5.1755},
        {40, 5.0722},
        {45, 4.9860},
        {50, 4.9103},
    }},
/* 11a/g */
    {6000000, {
        {5, 4.6899},
        {10, 4.3197},
        {15, 4.1107},
        {20, 3.9589},
        {25, 3.8478},
        {30, 3.7490},
        {35, 3.6618},
        {40, 3.5927},
        {45, 3.5358},
        {50, 3.4711},
    }},
    {9000000, {
        {5, 6.8188},
        {10, 6.2885},
        {15, 5.9874},
        {20, 5.7680},
        {25, 5.6073},
        {30, 5.4642},
        {35, 5.3378},
        {40, 5.2376},
        {45, 5.1551},
        {50, 5.0612},
    }},
    {12000000, {
        {5, 8.8972},
        {10, 8.2154},
        {15, 7.8259},
        {20, 7.5415},
        {25, 7.3329},
        {30, 7.1469},
        {35, 6.9825},
        {40, 6.8521},
        {45, 6.7447},
        {50, 6.6225},
    }},
    {18000000, {
        {5, 12.6719},
        {10, 11.7273},
        {15, 11.1814},
        {20, 10.7810},
        {25, 10.4866},
        {30, 10.2237},
        {35, 9.9910},
        {40, 9.8061},
        {45, 9.6538},
        {50, 9.4804},
    }},
    {24000000, {
        {5, 16.0836},
        {10, 14.9153},
        {15, 14.2327},
        {20, 13.7300},
        {25, 13.3595},
        {30, 13.0281},
        {35, 12.7343},
        {40, 12.5008},
        {45, 12.3083},
        {50, 12.0889},
    }},
    {36000000, {
        {5, 22.0092},
        {10, 20.4836},
        {15, 19.5743},
        {20, 18.8997},
        {25, 18.4002},
        {30, 17.9524},
        {35, 17.5545},
        {40, 17.2377},
        {45, 16.9760},
        {50, 16.6777},
    }},
    {48000000, {
        {5, 26.8382},
        {10, 25.0509},
        {15, 23.9672},
        {20, 23.1581},
        {25, 22.5568},
        {30, 22.0165},
        {35, 21.5355},
        {40, 21.1519},
        {45, 20.8348},
        {50, 20.4729},
    }},
    {54000000, {
        {5, 29.2861},
        {10, 27.3763},
        {15, 26.2078},
        {20, 25.3325},
        {25, 24.6808},
        {30, 24.0944},
        {35, 23.5719},
        {40, 23.1549},
        {45, 22.8100},
        {50, 22.4162},
    }},
};

std::map<unsigned int /* rate (bit/s)*/, std::map<unsigned int /* number of nodes */, double /* calculated throughput */> > bianchiResultsDifs =
{
/* 11b */
    {1000000, {
        {5, 0.8437},
        {10, 0.7861},
        {15, 0.7496},
        {20, 0.7226},
        {25, 0.7016},
        {30, 0.6847},
        {35, 0.6686},
        {40, 0.6549},
        {45, 0.6435},
        {50, 0.6336},
    }},
    {2000000, {
        {5, 1.6228},
        {10, 1.5168},
        {15, 1.4482},
        {20, 1.3972},
        {25, 1.3574},
        {30, 1.3253},
        {35, 1.2947},
        {40, 1.2687},
        {45, 1.2469},
        {50, 1.2279},
    }},
    {5500000, {
        {5, 3.8896},
        {10, 3.6707},
        {15, 3.5203},
        {20, 3.4063},
        {25, 3.3161},
        {30, 3.2429},
        {35, 3.1729},
        {40, 3.1128},
        {45, 3.0625},
        {50, 3.0184},
    }},
    {11000000, {
        {5, 6.4734},
        {10, 6.1774},
        {15, 5.9553},
        {20, 5.7819},
        {25, 5.6429},
        {30, 5.5289},
        {35, 5.4191},
        {40, 5.3243},
        {45, 5.2446},
        {50, 5.1745},
    }},
/* 11a/g */
    {6000000, {
        {5, 4.7087},
        {10, 4.3453},
        {15, 4.1397},
        {20, 3.9899},
        {25, 3.8802},
        {30, 3.7824},
        {35, 3.6961},
        {40, 3.6276},
        {45, 3.5712},
        {50, 3.5071},
    }},
    {9000000, {
        {5, 6.8586},
        {10, 6.3431},
        {15, 6.0489},
        {20, 5.8340},
        {25, 5.6762},
        {30, 5.5355},
        {35, 5.4110},
        {40, 5.3122},
        {45, 5.2307},
        {50, 5.1380},
    }},
    {12000000, {
        {5, 8.9515},
        {10, 8.2901},
        {15, 7.9102},
        {20, 7.6319},
        {25, 7.4274},
        {30, 7.2447},
        {35, 7.0829},
        {40, 6.9544},
        {45, 6.8485},
        {50, 6.7278},
    }},
    {18000000, {
        {5, 12.7822},
        {10, 11.8801},
        {15, 11.3543},
        {20, 10.9668},
        {25, 10.6809},
        {30, 10.4249},
        {35, 10.1978},
        {40, 10.0171},
        {45, 9.8679},
        {50, 9.6978},
    }},
    {24000000, {
        {5, 16.2470},
        {10, 15.1426},
        {15, 14.4904},
        {20, 14.0072},
        {25, 13.6496},
        {30, 13.3288},
        {35, 13.0436},
        {40, 12.8164},
        {45, 12.6286},
        {50, 12.4144},
    }},
    {36000000, {
        {5, 22.3164},
        {10, 20.9147},
        {15, 20.0649},
        {20, 19.4289},
        {25, 18.9552},
        {30, 18.5284},
        {35, 18.1476},
        {40, 17.8434},
        {45, 17.5915},
        {50, 17.3036},
    }},
    {48000000, {
        {5, 27.2963},
        {10, 25.6987},
        {15, 24.7069},
        {20, 23.9578},
        {25, 23.3965},
        {30, 22.8891},
        {35, 22.4350},
        {40, 22.0713},
        {45, 21.7696},
        {50, 21.4243},
    }},
    {54000000, {
        {5, 29.8324},
        {10, 28.1519},
        {15, 27.0948},
        {20, 26.2925},
        {25, 25.6896},
        {30, 25.1434},
        {35, 24.6539},
        {40, 24.2613},
        {45, 23.9353},
        {50, 23.5618},
    }},
};

// Parse context strings of the form "/NodeList/x/DeviceList/x/..." to extract the NodeId integer
uint32_t
ContextToNodeId (std::string context)
{
  std::string sub = context.substr (10);
  uint32_t pos = sub.find ("/Device");
  return atoi (sub.substr (0, pos).c_str ());
}

// Parse context strings of the form "/NodeList/x/DeviceList/x/..." and fetch the Mac address
Mac48Address
ContextToMac (std::string context)
{
  std::string sub = context.substr (10);
  uint32_t pos = sub.find ("/Device");
  uint32_t nodeId = atoi (sub.substr (0, pos).c_str ());
  Ptr<Node> n = NodeList::GetNode (nodeId);
  Ptr<WifiNetDevice> d;
  for (uint32_t i = 0; i < n->GetNDevices (); i++)
    {
      d = n->GetDevice (i)->GetObject<WifiNetDevice> ();
      if (d)
        {
          break;
        }
    }
  return Mac48Address::ConvertFrom (d->GetAddress ());
}

// Functions for tracing.

void
IncrementCounter (std::map<Mac48Address, uint64_t> & counter, Mac48Address addr, uint64_t increment = 1)
{
  auto it = counter.find (addr);
  if (it != counter.end ())
   {
     it->second += increment;
   }
  else
   {
     counter.insert (std::make_pair (addr, increment));
   }
}

void
TracePacketReception (std::string context, Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, SignalNoiseDbm signalNoise, uint16_t staId)
{
  WifiMacHeader hdr;
  packet->PeekHeader (hdr);
  // hdr.GetAddr1() is the receiving MAC address
  if (hdr.GetAddr1 () != ContextToMac (context))
    { 
      return;
    }
  // hdr.GetAddr2() is the sending MAC address
  if (packet->GetSize () == (pktSize + 36)) // ignore non-data frames
    {
      IncrementCounter (packetsReceived, hdr.GetAddr2 ());
      IncrementCounter (bytesReceived, hdr.GetAddr2 (), pktSize);
      auto itTimeFirstReceived = timeFirstReceived.find (hdr.GetAddr2 ());
      if (itTimeFirstReceived == timeFirstReceived.end ())
        {
          timeFirstReceived.insert (std::make_pair (hdr.GetAddr2 (), Simulator::Now ()));
        }
      auto itTimeLastReceived = timeLastReceived.find (hdr.GetAddr2 ());
      if (itTimeLastReceived != timeLastReceived.end ())
        {
          itTimeLastReceived->second = Simulator::Now ();
        }
      else
        {
          timeLastReceived.insert (std::make_pair (hdr.GetAddr2 (), Simulator::Now ()));
        }
    }
}

void
CwTrace (std::string context, uint32_t oldVal, uint32_t newVal)
{
  NS_LOG_INFO ("CW time=" << Simulator::Now () << " node=" << ContextToNodeId (context) << " val=" << newVal);
  if (tracing)
    {
      cwTraceFile << Simulator::Now ().GetSeconds () << " " << ContextToNodeId (context) << " " << newVal << std::endl;
    }
}

void
BackoffTrace (std::string context, uint32_t newVal)
{
  NS_LOG_INFO ("Backoff time=" << Simulator::Now () << " node=" << ContextToNodeId (context) << " val=" << newVal);
  if (tracing)
    {
      backoffTraceFile << Simulator::Now ().GetSeconds () << " " << ContextToNodeId (context) << " " << newVal << std::endl;
    }
}

void
PhyRxTrace (std::string context, Ptr<const Packet> p, RxPowerWattPerChannelBand power)
{
  NS_LOG_INFO ("PHY-RX-START time=" << Simulator::Now () << " node=" << ContextToNodeId (context) << " size=" << p->GetSize ());
}

void
PhyRxPayloadTrace (std::string context, WifiTxVector txVector, Time psduDuration)
{
  NS_LOG_INFO ("PHY-RX-PAYLOAD-START time=" << Simulator::Now () << " node=" << ContextToNodeId (context) << " psduDuration=" << psduDuration);
}

void
PhyRxDropTrace (std::string context, Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
  NS_LOG_INFO ("PHY-RX-DROP time=" << Simulator::Now () << " node=" << ContextToNodeId (context) << " size=" << p->GetSize () << " reason=" << reason);
  Mac48Address addr = ContextToMac (context);
  switch (reason)
    {
    case UNSUPPORTED_SETTINGS:
      NS_FATAL_ERROR ("RX packet with unsupported settings!");
      break;
    case CHANNEL_SWITCHING:
      NS_FATAL_ERROR ("Channel is switching!");
      break;
    case BUSY_DECODING_PREAMBLE:
    {
      if (p->GetSize () == (pktSize + 36)) // ignore non-data frames
        {
          IncrementCounter (rxEventWhileDecodingPreamble, addr);
        }
      break;
    }
    case RXING:
    {
      if (p->GetSize () == (pktSize + 36)) // ignore non-data frames
        {
          IncrementCounter (rxEventWhileRxing, addr);
        }
      break;
    }
    case TXING:
    {
      if (p->GetSize () == (pktSize + 36)) // ignore non-data frames
        {
          IncrementCounter (rxEventWhileTxing, addr);
        }
      break;
    }
    case SLEEPING:
      NS_FATAL_ERROR ("Device is sleeping!");
      break;
    case PREAMBLE_DETECT_FAILURE:
      NS_FATAL_ERROR ("Preamble should always be detected!");
      break;
    case RECEPTION_ABORTED_BY_TX:
    {
      if (p->GetSize () == (pktSize + 36)) // ignore non-data frames
        {
          IncrementCounter (rxEventAbortedByTx, addr);
        }
      break;
    }
    case L_SIG_FAILURE:
    {
      if (p->GetSize () == (pktSize + 36)) // ignore non-data frames
        {
          IncrementCounter (phyHeaderFailed, addr);
        }
      break;
    }
    case SIG_A_FAILURE:
      NS_FATAL_ERROR ("Unexpected PHY header failure!");
    case PREAMBLE_DETECTION_PACKET_SWITCH:
      NS_FATAL_ERROR ("All devices should send with same power, so no packet switch during preamble detection should occur!");
      break;
    case FRAME_CAPTURE_PACKET_SWITCH:
      NS_FATAL_ERROR ("Frame capture should be disabled!");
      break;
    case OBSS_PD_CCA_RESET:
      NS_FATAL_ERROR ("Unexpected CCA reset!");
      break;
    case UNKNOWN:
    default:
      NS_FATAL_ERROR ("Unknown drop reason!");
      break;
    }
}

void
PhyRxDoneTrace (std::string context, Ptr<const Packet> p)
{
  NS_LOG_INFO ("PHY-RX-END time=" << Simulator::Now () << " node=" << ContextToNodeId (context) << " size=" << p->GetSize ());
}

void
PhyRxOkTrace (std::string context, Ptr<const Packet> p, double snr, WifiMode mode, WifiPreamble preamble)
{
  NS_LOG_INFO ("PHY-RX-OK time=" << Simulator::Now () << " node=" << ContextToNodeId (context) << " size=" << p->GetSize () << " snr=" << snr << " mode=" << mode << " preamble=" << preamble);
  if (p->GetSize () == (pktSize + 36)) // ignore non-data frames
    {
      Mac48Address addr = ContextToMac (context);
      IncrementCounter (psduSucceeded, addr);
    }
}

void
PhyRxErrorTrace (std::string context, Ptr<const Packet> p, double snr)
{
  NS_LOG_INFO ("PHY-RX-ERROR time=" << Simulator::Now () << " node=" << ContextToNodeId (context) << " size=" << p->GetSize () << " snr=" << snr);
  if (p->GetSize () == (pktSize + 36)) // ignore non-data frames
    {
      Mac48Address addr = ContextToMac (context);
      IncrementCounter (psduFailed, addr);
    }
}

void
PhyTxTrace (std::string context, Ptr<const Packet> p, double txPowerW)
{
  NS_LOG_INFO ("PHY-TX-START time=" << Simulator::Now () << " node=" << ContextToNodeId (context) << " size=" << p->GetSize () << " " << txPowerW);
  if (tracing)
    {
      phyTxTraceFile << Simulator::Now ().GetSeconds () << " " << ContextToNodeId (context) << " size=" << p->GetSize () << " " << txPowerW << std::endl;
    }
  if (p->GetSize () == (pktSize + 36)) // ignore non-data frames
    {
      Mac48Address addr = ContextToMac (context);
      IncrementCounter (packetsTransmitted, addr);
    }
}

void
PhyTxDoneTrace (std::string context, Ptr<const Packet> p)
{
  NS_LOG_INFO ("PHY-TX-END time=" << Simulator::Now () << " node=" << ContextToNodeId (context) << " " << p->GetSize ());
}

void
MacTxTrace (std::string context, Ptr<const Packet> p)
{
  if (tracing)
    {
      macTxTraceFile << Simulator::Now ().GetSeconds () << " " << ContextToNodeId (context) << " " << p->GetSize () << std::endl;
    }
}

void
MacRxTrace (std::string context, Ptr<const Packet> p)
{
  if (tracing)
    {
      macRxTraceFile << Simulator::Now ().GetSeconds () << " " << ContextToNodeId (context) << " " << p->GetSize () << std::endl;
    }
}

void
SocketSendTrace (std::string context, Ptr<const Packet> p, const Address &addr)
{
  if (tracing)
    {
      socketSendTraceFile << Simulator::Now ().GetSeconds () << " " << ContextToNodeId (context) << " " << p->GetSize () << " " << addr << std::endl;
    }
}

void
AssociationLog (std::string context, Mac48Address address)
{
  uint32_t nodeId = ContextToNodeId (context);
  auto it = associated.find (nodeId);
  if (it == associated.end ())
    {
      NS_LOG_DEBUG ("Association: time=" << Simulator::Now () << " node=" << nodeId);
      associated.insert (it, nodeId);
    }
  else
    {
      NS_FATAL_ERROR (nodeId << " is already associated!");
    }
}

void
DisassociationLog (std::string context, Mac48Address address)
{
  uint32_t nodeId = ContextToNodeId (context);
  NS_LOG_DEBUG ("Disassociation: time=" << Simulator::Now () << " node=" << nodeId);
  NS_FATAL_ERROR ("Device should not disassociate!");
}

void
RestartCalc ()
{
  bytesReceived.clear ();
  packetsReceived.clear ();
  packetsTransmitted.clear ();
  psduFailed.clear ();
  psduSucceeded.clear ();
  phyHeaderFailed.clear ();
  timeFirstReceived.clear ();
  timeLastReceived.clear ();
  rxEventWhileDecodingPreamble.clear ();
  rxEventWhileRxing.clear ();
  rxEventWhileTxing.clear ();
  rxEventAbortedByTx.clear ();
}

class Experiment
{
public:
  Experiment ();
  int Run (const WifiHelper &wifi, const YansWifiPhyHelper &wifiPhy, const WifiMacHelper &wifiMac, const YansWifiChannelHelper &wifiChannel,
           uint32_t trialNumber, uint32_t networkSize, double duration, bool pcap, bool infra);
};

Experiment::Experiment ()
{
}

int
Experiment::Run (const WifiHelper &helper, const YansWifiPhyHelper &wifiPhy, const WifiMacHelper &wifiMac, const YansWifiChannelHelper &wifiChannel,
                 uint32_t trialNumber, uint32_t networkSize, double duration, bool pcap, bool infra)
{
  NodeContainer wifiNodes;
  if (infra)
    {
      wifiNodes.Create (networkSize + 1);
    }
  else
    {
      wifiNodes.Create (networkSize); 
    }

  YansWifiPhyHelper phy = wifiPhy;
  phy.SetErrorRateModel ("ns3::NistErrorRateModel");
  phy.SetChannel (wifiChannel.Create ());
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  WifiMacHelper mac = wifiMac;
  WifiHelper wifi = helper;
  NetDeviceContainer devices;
  uint32_t nNodes = wifiNodes.GetN ();
  if (infra)
    {
      Ssid ssid = Ssid ("wifi-bianchi");
      uint64_t beaconInterval = std::min<uint64_t> ((ceil ((duration * 1000000) / 1024) * 1024), (65535 * 1024)); //beacon interval needs to be a multiple of time units (1024 us)
      mac.SetType ("ns3::ApWifiMac",
                   "BeaconInterval", TimeValue (MicroSeconds (beaconInterval)),
                   "Ssid", SsidValue (ssid));
      devices = wifi.Install (phy, mac, wifiNodes.Get (0));

      mac.SetType ("ns3::StaWifiMac",
                   "MaxMissedBeacons", UintegerValue (std::numeric_limits<uint32_t>::max ()),
                   "Ssid", SsidValue (ssid));
      for (uint32_t i = 1; i < nNodes; ++i)
        {
          devices.Add (wifi.Install (phy, mac, wifiNodes.Get (i)));
        }
    }
  else
    {
      mac.SetType ("ns3::AdhocWifiMac");
      devices = wifi.Install (phy, mac, wifiNodes);
    }

  wifi.AssignStreams (devices, trialNumber);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // Set postion for AP
  positionAlloc->Add (Vector (1.0, 1.0, 0.0));

  // Set postion for STAs
  double angle = (static_cast<double> (360) / (nNodes - 1));
  for (uint32_t i = 0; i < (nNodes - 1); ++i)
    {
      positionAlloc->Add (Vector (1.0 + (0.001 * cos ((i * angle * PI) / 180)), 1.0 + (0.001 * sin ((i * angle * PI) / 180)), 0.0));
    }

  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (wifiNodes);

  PacketSocketHelper packetSocket;
  packetSocket.Install (wifiNodes);
  
  ApplicationContainer apps;
  Ptr<UniformRandomVariable> startTime = CreateObject<UniformRandomVariable> ();
  startTime->SetAttribute ("Stream", IntegerValue (trialNumber));
  startTime->SetAttribute ("Max", DoubleValue (5.0));

  uint32_t i = infra ? 1 : 0;
  for (; i < nNodes; ++i)
    {
      uint32_t j = infra ? 0 : (i + 1) % nNodes;
      PacketSocketAddress socketAddr;
      socketAddr.SetSingleDevice (devices.Get (i)->GetIfIndex ());
      socketAddr.SetPhysicalAddress (devices.Get (j)->GetAddress ());
      socketAddr.SetProtocol (1);

      Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient> ();
      client->SetRemote (socketAddr);
      wifiNodes.Get (i)->AddApplication (client);
      client->SetAttribute ("PacketSize", UintegerValue (pktSize));
      client->SetAttribute ("MaxPackets", UintegerValue (0));
      client->SetAttribute ("Interval", TimeValue (MilliSeconds (1))); //TODO: reduce it for lower rates, increase it for higher rates
      double start = startTime->GetValue ();
      NS_LOG_DEBUG ("Client " << i << " starting at " << start);
      client->SetStartTime (Seconds (start));

      Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer> ();
      server->SetLocal (socketAddr);
      wifiNodes.Get (j)->AddApplication (server);
    }

  // Log packet receptions
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/MonitorSnifferRx", MakeCallback (&TracePacketReception));

  // Log association and disassociation
  if (infra)
    {
      Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/Assoc", MakeCallback (&AssociationLog));
      Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/DeAssoc", MakeCallback (&DisassociationLog));
    }

  // Trace CW evolution
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/Txop/CwTrace", MakeCallback (&CwTrace));
  // Trace backoff evolution
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/Txop/BackoffTrace", MakeCallback (&BackoffTrace));
  // Trace PHY Tx start events
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyTxBegin", MakeCallback (&PhyTxTrace));
  // Trace PHY Tx end events
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyTxEnd", MakeCallback (&PhyTxDoneTrace));
  // Trace PHY Rx start events
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxBegin", MakeCallback (&PhyRxTrace));
  // Trace PHY Rx payload start events
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxPayloadBegin", MakeCallback (&PhyRxPayloadTrace));
  // Trace PHY Rx drop events
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxDrop", MakeCallback (&PhyRxDropTrace));
  // Trace PHY Rx end events
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxEnd", MakeCallback (&PhyRxDoneTrace));
  // Trace PHY Rx error events
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/State/RxError", MakeCallback (&PhyRxErrorTrace));
  // Trace PHY Rx success events
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/State/RxOk", MakeCallback (&PhyRxOkTrace));
  // Trace packet transmission by the device
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback (&MacTxTrace));
  // Trace packet receptions to the device
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback (&MacRxTrace));
  // Trace packets transmitted by the application
  Config::Connect ("/NodeList/*/$ns3::Node/ApplicationList/*/$ns3::PacketSocketClient/Tx", MakeCallback (&SocketSendTrace));

  Simulator::Schedule (Seconds (10), &RestartCalc);
  Simulator::Stop (Seconds (10 + duration));

  if (pcap)
    {
      phy.EnablePcap ("wifi_bianchi_pcap", devices);
    }

  Simulator::Run ();
  Simulator::Destroy ();

  if (tracing)
    {
      cwTraceFile.flush ();
      backoffTraceFile.flush ();
      phyTxTraceFile.flush ();
      macTxTraceFile.flush ();
      macRxTraceFile.flush ();
      socketSendTraceFile.flush ();
    }
  
  return 0;
}

uint64_t
GetCount (const std::map<Mac48Address, uint64_t> & counter, Mac48Address addr)
{
  uint64_t count = 0;
  auto it = counter.find (addr);
  if (it != counter.end ())
    {
      count = it->second;
    }
  return count;
}

int main (int argc, char *argv[])
{
  uint32_t nMinStas = 5;           ///< Minimum number of STAs to start with
  uint32_t nMaxStas = 50;          ///< Maximum number of STAs to end with
  uint32_t nStepSize = 5;          ///< Number of stations to add at each step
  uint32_t verbose = 0;            ///< verbosity level that increases the number of debugging traces
  double duration = 100;           ///< duration (in seconds) of each simulation run (i.e. per trial and per number of stations)
  uint32_t trials = 1;             ///< Number of runs per point in the plot
  bool pcap = false;               ///< Flag to enable/disable PCAP files generation
  bool infra = false;              ///< Flag to enable infrastructure model, ring adhoc network if not set
  std::string workDir = "./";      ///< the working directory to store generated files
  double phyRate = 54;             ///< the constant PHY rate used to transmit Data frames (in Mbps)
  std::string standard ("11a");    ///< the 802.11 standard
  bool validate = false;           ///< Flag used for regression in order to verify ns-3 results are in the expected boundaries
  uint32_t plotBianchiModel = 0x1; ///< First bit corresponds to the DIFS model, second bit to the EIFS model
  double maxRelativeError = 0.015; ///< Maximum relative error tolerated between ns-3 results and the Bianchi model (used for regression, i.e. when the validate flag is set)

  // Disable fragmentation and RTS/CTS
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("22000"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("22000"));
  // Disable short retransmission failure (make retransmissions persistent)
  Config::SetDefault ("ns3::WifiRemoteStationManager::MaxSlrc", UintegerValue (std::numeric_limits<uint32_t>::max ()));
  Config::SetDefault ("ns3::WifiRemoteStationManager::MaxSsrc", UintegerValue (std::numeric_limits<uint32_t>::max ()));
  // Set maximum queue size to the largest value and set maximum queue delay to be larger than the simulation time
  Config::SetDefault ("ns3::WifiMacQueue::MaxSize", QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, std::numeric_limits<uint32_t>::max ())));
  Config::SetDefault ("ns3::WifiMacQueue::MaxDelay", TimeValue (Seconds (2 * duration)));

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "Logging level (0: no log - 1: simulation script logs - 2: all logs)", verbose);
  cmd.AddValue ("tracing", "Generate trace files", tracing);
  cmd.AddValue ("pktSize", "The packet size in bytes", pktSize);
  cmd.AddValue ("trials", "The maximal number of runs per network size", trials);
  cmd.AddValue ("duration", "Time duration for each trial in seconds", duration);
  cmd.AddValue ("pcap", "Enable/disable PCAP tracing", pcap);
  cmd.AddValue ("infra", "True to use infrastructure mode, false to use ring adhoc mode", infra);
  cmd.AddValue ("workDir", "The working directory used to store generated files", workDir);
  cmd.AddValue ("phyRate", "Set the constant PHY rate in Mbps used to transmit Data frames", phyRate);
  cmd.AddValue ("standard", "Set the standard (11a, 11b, 11g, 11n, 11ac, 11ax)", standard);
  cmd.AddValue ("nMinStas", "Minimum number of stations to start with", nMinStas);
  cmd.AddValue ("nMaxStas", "Maximum number of stations to start with", nMaxStas);
  cmd.AddValue ("nStepSize", "Number of stations to add at each step", nStepSize);
  cmd.AddValue ("validate", "Enable/disable validation of the ns-3 simulations against the Bianchi model", validate);
  cmd.AddValue ("maxRelativeError", "The maximum relative error tolerated between ns-3 results and the Bianchi model (used for regression, i.e. when the validate flag is set)", maxRelativeError);
  cmd.Parse (argc, argv);

  if (tracing)
    {
      cwTraceFile.open ("wifi-bianchi-cw-trace.out");
      if (!cwTraceFile.is_open ())
        {
          NS_FATAL_ERROR ("Failed to open file wifi-bianchi-cw-trace.out");
        }
      backoffTraceFile.open ("wifi-bianchi-backoff-trace.out");
      if (!backoffTraceFile.is_open ())
        {
          NS_FATAL_ERROR ("Failed to open file wifi-bianchi-backoff-trace.out");
        }
      phyTxTraceFile.open ("wifi-bianchi-phy-tx-trace.out");
      if (!phyTxTraceFile.is_open ())
        {
          NS_FATAL_ERROR ("Failed to open file wifi-bianchi-phy-tx-trace.out");
        }
      macTxTraceFile.open ("wifi-bianchi-mac-tx-trace.out");
      if (!macTxTraceFile.is_open ())
        {
          NS_FATAL_ERROR ("Failed to open file wifi-bianchi-mac-tx-trace.out");
        }
      macRxTraceFile.open ("wifi-bianchi-mac-rx-trace.out");
      if (!macRxTraceFile.is_open ())
        {
          NS_FATAL_ERROR ("Failed to open file wifi-bianchi-mac-rx-trace.out");
        }
      socketSendTraceFile.open ("wifi-bianchi-socket-send-trace.out");
      if (!socketSendTraceFile.is_open ())
        {
          NS_FATAL_ERROR ("Failed to open file wifi-bianchi-socket-send-trace.out");
        }
    }

  if (verbose >= 1)
    {
      LogComponentEnable ("WifiBianchi", LOG_LEVEL_ALL);
    }
  if (verbose >= 2)
    {
      WifiHelper::EnableLogComponents ();
    }

  std::stringstream ss;
  ss << "wifi-"<< standard << "-p-" << pktSize << (infra ? "-infrastructure" : "-adhoc") << "-r-" << phyRate << "-min-" << nMinStas << "-max-" << nMaxStas << "-step-" << nStepSize << "-throughput.plt";
  std::ofstream throughputPlot (ss.str ().c_str ());
  ss.str ("");
  ss << "wifi-" << standard << "-p-" << pktSize << (infra ? "-infrastructure" : "-adhoc") <<"-r-" << phyRate << "-min-" << nMinStas << "-max-" << nMaxStas << "-step-" << nStepSize << "-throughput.eps";
  Gnuplot gnuplot = Gnuplot (ss.str ());

  WifiStandard wifiStandard;
  std::stringstream phyRateStr;
  if (standard == "11a")
    {
      wifiStandard = WIFI_STANDARD_80211a;
      if ((phyRate != 6) && (phyRate != 9) && (phyRate != 12) && (phyRate != 18) && (phyRate != 24) && (phyRate != 36) && (phyRate != 48) && (phyRate != 54))
        {
          NS_FATAL_ERROR ("Selected PHY rate " << phyRate << " is not defined in " << standard);
        }
      phyRateStr << "OfdmRate" << phyRate << "Mbps";
    }
  else if (standard == "11b")
    {
      wifiStandard = WIFI_STANDARD_80211b;
      if ((phyRate != 1) && (phyRate != 2) && (phyRate != 5.5) && (phyRate != 11))
        {
          NS_FATAL_ERROR ("Selected PHY rate " << phyRate << " is not defined in " << standard);
        }
      if (phyRate == 5.5)
        {
          phyRateStr << "DsssRate5_5Mbps";
        }
      else
        {
          phyRateStr << "DsssRate" << phyRate << "Mbps";
        }
    }
  else if (standard == "11g")
    {
      wifiStandard = WIFI_STANDARD_80211g;
      if ((phyRate != 6) && (phyRate != 9) && (phyRate != 12) && (phyRate != 18) && (phyRate != 24) && (phyRate != 36) && (phyRate != 48) && (phyRate != 54))
        {
          NS_FATAL_ERROR ("Selected PHY rate " << phyRate << " is not defined in " << standard);
        }
      phyRateStr << "ErpOfdmRate" << phyRate << "Mbps";
    }
  else
    {
      NS_FATAL_ERROR ("Unsupported standard: " << standard);
    }

  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  wifiPhy.DisablePreambleDetectionModel ();

  WifiMacHelper wifiMac;

  WifiHelper wifi;
  wifi.SetStandard (wifiStandard);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyRateStr.str()));

  Gnuplot2dDataset dataset;
  Gnuplot2dDataset datasetBianchiEifs;
  Gnuplot2dDataset datasetBianchiDifs;
  dataset.SetErrorBars (Gnuplot2dDataset::Y);
  dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);
  datasetBianchiEifs.SetStyle (Gnuplot2dDataset::LINES_POINTS);
  datasetBianchiDifs.SetStyle (Gnuplot2dDataset::LINES_POINTS);

  Experiment experiment;
  double averageThroughput, throughputArray[trials];
  for (uint32_t n = nMinStas; n <= nMaxStas; n += nStepSize)
    {
      averageThroughput = 0;
      double throughput;
      for (uint32_t runIndex = 0; runIndex < trials; runIndex++)
        {
          packetsReceived.clear ();
          bytesReceived.clear ();
          packetsTransmitted.clear ();
          psduFailed.clear ();
          psduSucceeded.clear ();
          phyHeaderFailed.clear ();
          timeFirstReceived.clear ();
          timeLastReceived.clear ();
          rxEventWhileDecodingPreamble.clear ();
          rxEventWhileRxing.clear ();
          rxEventWhileTxing.clear ();
          rxEventAbortedByTx.clear ();
          associated.clear ();
          throughput = 0;
          std::cout << "Trial " << runIndex + 1 << " of " << trials << "; "<< phyRate << " Mbps for " << n << " nodes " << std::endl;
          if (tracing)
            {
              cwTraceFile << "# Trial " << runIndex + 1 << " of " << trials << "; "<< phyRate << " Mbps for " << n << " nodes" << std::endl;
              backoffTraceFile << "# Trial " << runIndex + 1 << " of " << trials << "; "<< phyRate << " Mbps for " << n << " nodes" << std::endl;
              phyTxTraceFile << "# Trial " << runIndex + 1 << " of " << trials << "; " << phyRate << " Mbps for " << n << " nodes" << std::endl;
              macTxTraceFile << "# Trial " << runIndex + 1 << " of " << trials << "; " << phyRate << " Mbps for " << n << " nodes" << std::endl;
              macRxTraceFile << "# Trial " << runIndex + 1 << " of " << trials << "; " << phyRate << " Mbps for " << n << " nodes" << std::endl;
              socketSendTraceFile << "# Trial " << runIndex + 1 << " of " << trials << "; " << phyRate << " Mbps for " << n << " nodes" << std::endl;
            }
          experiment.Run (wifi, wifiPhy, wifiMac, wifiChannel, runIndex, n, duration, pcap, infra);
          uint32_t k = 0;
          if (bytesReceived.size () != n)
            {
              NS_FATAL_ERROR ("Not all stations got traffic!");
            }
          for (auto it = bytesReceived.begin (); it != bytesReceived.end (); it++, k++)
            {
              Time first = timeFirstReceived.find (it->first)->second;
              Time last = timeLastReceived.find (it->first)->second;
              Time dataTransferDuration = last - first;
              double nodeThroughput = (it->second * 8 / static_cast<double> (dataTransferDuration.GetMicroSeconds ()));
              throughput += nodeThroughput;
              uint64_t nodeTxPackets = GetCount (packetsTransmitted, it->first);
              uint64_t nodeRxPackets = GetCount (packetsReceived, it->first);
              uint64_t nodePhyHeaderFailures = GetCount (phyHeaderFailed, it->first);
              uint64_t nodePsduFailures = GetCount (psduFailed, it->first);
              uint64_t nodePsduSuccess = GetCount (psduSucceeded, it->first);
              uint64_t nodeRxEventWhileDecodingPreamble = GetCount (rxEventWhileDecodingPreamble, it->first);
              uint64_t nodeRxEventWhileRxing = GetCount (rxEventWhileRxing, it->first);
              uint64_t nodeRxEventWhileTxing = GetCount (rxEventWhileTxing, it->first);
              uint64_t nodeRxEventAbortedByTx = GetCount (rxEventAbortedByTx, it->first);
              uint64_t nodeRxEvents =
                  nodePhyHeaderFailures + nodePsduFailures + nodePsduSuccess + nodeRxEventWhileDecodingPreamble +
                  nodeRxEventWhileRxing + nodeRxEventWhileTxing + nodeRxEventAbortedByTx;
              std::cout << "Node " << it->first
                        << ": TX packets " << nodeTxPackets
                        << "; RX packets " << nodeRxPackets
                        << "; PHY header failures " << nodePhyHeaderFailures
                        << "; PSDU failures " << nodePsduFailures
                        << "; PSDU success " << nodePsduSuccess
                        << "; RX events while decoding preamble " << nodeRxEventWhileDecodingPreamble
                        << "; RX events while RXing " << nodeRxEventWhileRxing
                        << "; RX events while TXing " << nodeRxEventWhileTxing
                        << "; RX events aborted by TX " << nodeRxEventAbortedByTx
                        << "; total RX events " << nodeRxEvents
                        << "; total events " << nodeTxPackets + nodeRxEvents
                        << "; time first RX " << first
                        << "; time last RX " << last
                        << "; dataTransferDuration " << dataTransferDuration
                        << "; throughput " << nodeThroughput  << " Mbps" << std::endl;
            }
          std::cout << "Total throughput: " << throughput << " Mbps" << std::endl;
          averageThroughput += throughput;
          throughputArray[runIndex] = throughput;
        }
      averageThroughput = averageThroughput / trials;

      bool rateFound = false;
      double relativeErrorDifs = 0;
      double relativeErrorEifs = 0;
      auto itDifs = bianchiResultsDifs.find (static_cast<unsigned int> (phyRate * 1000000));
      if (itDifs != bianchiResultsDifs.end ())
        {
          rateFound = true;
          auto it = itDifs->second.find (n);
          if (it != itDifs->second.end ())
            {
              relativeErrorDifs = (std::abs (averageThroughput - it->second) / it->second);
              std::cout << "Relative error (DIFS): " << 100 * relativeErrorDifs << "%" << std::endl;
            }
          else if (validate)
            {
              NS_FATAL_ERROR ("No Bianchi results (DIFS) calculated for that number of stations!");
            }
        }
      auto itEifs = bianchiResultsEifs.find (static_cast<unsigned int> (phyRate * 1000000));
      if (itEifs != bianchiResultsEifs.end ())
        {
          rateFound = true;
          auto it = itEifs->second.find (n);
          if (it != itEifs->second.end ())
            {
              relativeErrorEifs = (std::abs (averageThroughput - it->second) / it->second);
              std::cout << "Relative error (EIFS): " << 100 * relativeErrorEifs << "%" << std::endl;
            }
          else if (validate)
            {
              NS_FATAL_ERROR ("No Bianchi results (EIFS) calculated for that number of stations!");
            }
        }
      if (!rateFound && validate)
        {
          NS_FATAL_ERROR ("No Bianchi results calculated for that rate!");
        }
      double relativeError = std::min (relativeErrorDifs, relativeErrorEifs);
      if (validate && (relativeError > maxRelativeError))
        {
          NS_FATAL_ERROR ("Relative error is too high!");
        }

      double stDev = 0;
      for (uint32_t i = 0; i < trials; ++i)
        {
          stDev += pow (throughputArray[i] - averageThroughput, 2);
        }
      stDev = sqrt (stDev / (trials - 1));
      dataset.Add (n, averageThroughput, stDev);
    }
  dataset.SetTitle ("ns-3");

  auto itDifs = bianchiResultsDifs.find (static_cast<unsigned int> (phyRate * 1000000));
  if (itDifs != bianchiResultsDifs.end ())
    {
      for (uint32_t i = nMinStas; i <= nMaxStas; i += nStepSize)
        {
          double value = 0.0;
          auto it = itDifs->second.find (i);
          if (it != itDifs->second.end ())
            {
              value = it->second;
            }
          datasetBianchiDifs.Add (i, value);
        }
    }
  else
    {
      for (uint32_t i = nMinStas; i <= nMaxStas; i += nStepSize)
        {
          datasetBianchiDifs.Add (i, 0.0);
        }
    }

  auto itEifs = bianchiResultsEifs.find (static_cast<unsigned int> (phyRate * 1000000));
  if (itEifs != bianchiResultsEifs.end ())
    {
      for (uint32_t i = nMinStas; i <= nMaxStas; i += nStepSize)
        {
          double value = 0.0;
          auto it = itEifs->second.find (i);
          if (it != itEifs->second.end ())
            {
              value = it->second;
            }
          datasetBianchiEifs.Add (i, value);
        }
    }
  else
    {
      for (uint32_t i = nMinStas; i <= nMaxStas; i += nStepSize)
        {
          datasetBianchiEifs.Add (i, 0.0);
        }
    }

  datasetBianchiEifs.SetTitle ("Bianchi (EIFS - lower bound)");
  datasetBianchiDifs.SetTitle ("Bianchi (DIFS - upper bound)");
  gnuplot.AddDataset (dataset);
  gnuplot.SetTerminal ("postscript eps color enh \"Times-BoldItalic\"");
  gnuplot.SetLegend ("Number of competing stations", "Throughput (Mbps)");
  ss.str ("");
  ss << "Frame size " << pktSize << " bytes";
  gnuplot.SetTitle (ss.str ());
  ss.str ("");
  ss << "set xrange [" << nMinStas << ":" << nMaxStas << "]\n"
     << "set xtics " << nStepSize << "\n"
     << "set grid xtics ytics\n"
     << "set mytics\n"
     << "set style line 1 linewidth 5\n"
     << "set style line 2 linewidth 5\n"
     << "set style line 3 linewidth 5\n"
     << "set style line 4 linewidth 5\n"
     << "set style line 5 linewidth 5\n"
     << "set style line 6 linewidth 5\n"
     << "set style line 7 linewidth 5\n"
     << "set style line 8 linewidth 5\n"
     << "set style increment user";
  gnuplot.SetExtra (ss.str ());
  if (plotBianchiModel & 0x01)
    {
      datasetBianchiDifs.SetTitle ("Bianchi");
      gnuplot.AddDataset (datasetBianchiDifs);
    }
  if (plotBianchiModel & 0x02)
    {
      datasetBianchiEifs.SetTitle ("Bianchi");
      gnuplot.AddDataset (datasetBianchiEifs);
    }
  if (plotBianchiModel == 0x03)
    {
      datasetBianchiEifs.SetTitle ("Bianchi (EIFS - lower bound)");
      datasetBianchiDifs.SetTitle ("Bianchi (DIFS - upper bound)");
    }
  gnuplot.GenerateOutput (throughputPlot);
  throughputPlot.close ();

  if (tracing)
    {
      cwTraceFile.close ();
      backoffTraceFile.close ();
      phyTxTraceFile.close ();
      macTxTraceFile.close ();
      macRxTraceFile.close ();
      socketSendTraceFile.close ();
    }

  return 0;
}
