/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Sébastien Deronne
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-phy-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DsssSigHeader);

DsssSigHeader::DsssSigHeader ()
  : m_rate (0b00001010),
    m_length (0)
{
}

DsssSigHeader::~DsssSigHeader ()
{
}

TypeId
DsssSigHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DsssSigHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DsssSigHeader> ()
  ;
  return tid;
}

TypeId
DsssSigHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
DsssSigHeader::Print (std::ostream &os) const
{
  os << "SIGNAL=" << GetRate ()
     << " LENGTH=" << m_length;
}

uint32_t
DsssSigHeader::GetSerializedSize (void) const
{
  return 6;
}

void
DsssSigHeader::SetRate (uint64_t rate)
{
  /* Here is the binary representation for a given rate:
   * 1 Mbit/s: 00001010
   * 2 Mbit/s: 00010100
   * 5.5 Mbit/s: 00110111
   * 11 Mbit/s: 01101110
   */
  switch (rate)
    {
      case 1000000:
        m_rate = 0b00001010;
        break;
      case 2000000:
        m_rate = 0b00010100;
        break;
      case 5500000:
        m_rate = 0b00110111;
        break;
      case 11000000:
        m_rate = 0b01101110;
        break;
      default:
        NS_ASSERT_MSG (false, "Invalid rate");
        break;
    }
}

uint64_t
DsssSigHeader::GetRate (void) const
{
  uint64_t rate = 0;
  switch (m_rate)
    {
      case 0b00001010:
        rate = 1000000;
        break;
      case 0b00010100:
        rate = 2000000;
        break;
      case 0b00110111:
        rate = 5500000;
        break;
      case 0b01101110:
        rate = 11000000;
        break;
      default:
        NS_ASSERT_MSG (false, "Invalid rate");
        break;
    }
  return rate;
}

void
DsssSigHeader::SetLength (uint16_t length)
{
  m_length = length;
}

uint16_t
DsssSigHeader::GetLength (void) const
{
  return m_length;
}

void
DsssSigHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU8 (m_rate);
  start.WriteU8 (0); /* SERVICE */
  start.WriteU16 (m_length);
  start.WriteU16 (0); /* CRC */
}

uint32_t
DsssSigHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_rate = i.ReadU8 ();
  i.ReadU8 (); /* SERVICE */
  m_length =  i.ReadU16 ();
  i.ReadU16 (); /* CRC */
  return i.GetDistanceFrom (start);
}

NS_OBJECT_ENSURE_REGISTERED (LSigHeader);

LSigHeader::LSigHeader ()
  : m_rate (0b1101),
    m_length (0)
{
}

LSigHeader::~LSigHeader ()
{
}

TypeId
LSigHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LSigHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<LSigHeader> ()
  ;
  return tid;
}

TypeId
LSigHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
LSigHeader::Print (std::ostream &os) const
{
  os << "SIGNAL=" << GetRate ()
     << " LENGTH=" << m_length;
}

uint32_t
LSigHeader::GetSerializedSize (void) const
{
  return 3;
}

void
LSigHeader::SetRate (uint64_t rate, uint16_t channelWidth)
{
  if (channelWidth < 20)
    {
      //conversion for 5 MHz and 10 MHz
      rate *= (20 / channelWidth);
    }
  /* Here is the binary representation for a given rate:
   * 6 Mbit/s: 1101
   * 9 Mbit/s: 1111
   * 12 Mbit/s: 0101
   * 18 Mbit/s: 0111
   * 24 Mbit/s: 1001
   * 36 Mbit/s: 1011
   * 48 Mbit/s: 0001
   * 54 Mbit/s: 0011
   */
  switch (rate)
    {
      case 6000000:
        m_rate = 0b1101;
        break;
      case 9000000:
        m_rate = 0b1111;
        break;
      case 12000000:
        m_rate = 0b0101;
        break;
      case 18000000:
        m_rate = 0b0111;
        break;
      case 24000000:
        m_rate = 0b1001;
        break;
      case 36000000:
        m_rate = 0b1011;
        break;
      case 48000000:
        m_rate = 0b0001;
        break;
      case 54000000:
        m_rate = 0b0011;
        break;
      default:
        NS_ASSERT_MSG (false, "Invalid rate");
        break;
    }
}

uint64_t
LSigHeader::GetRate (uint16_t channelWidth) const
{
  uint64_t rate = 0;
  switch (m_rate)
    {
      case 0b1101:
        rate = 6000000;
        break;
      case 0b1111:
        rate = 9000000;
        break;
      case 0b0101:
        rate = 12000000;
        break;
      case 0b0111:
        rate = 18000000;
        break;
      case 0b1001:
        rate = 24000000;
        break;
      case 0b1011:
        rate = 36000000;
        break;
      case 0b0001:
        rate = 48000000;
        break;
      case 0b0011:
        rate = 54000000;
        break;
      default:
        NS_ASSERT_MSG (false, "Invalid rate");
        break;
    }
  if (channelWidth == 5)
    {
      rate /= 4; //compute corresponding 5 MHz rate
    }
  else if (channelWidth == 10)
    {
      rate /= 2; //compute corresponding 10 MHz rate
    }
  return rate;
}

void
LSigHeader::SetLength (uint16_t length)
{
  NS_ASSERT_MSG (length < 4096, "Invalid length");
  m_length = length;
}

uint16_t
LSigHeader::GetLength (void) const
{
  return m_length;
}

void
LSigHeader::Serialize (Buffer::Iterator start) const
{
  uint8_t byte = 0;
  uint16_t bytes = 0;

  byte |= m_rate;
  byte |= (m_length & 0x07) << 5;
  start.WriteU8 (byte);

  bytes |= (m_length & 0x0ff8) >> 3;
  start.WriteU16 (bytes);
}

uint32_t
LSigHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  uint8_t byte = i.ReadU8 ();
  m_rate = byte & 0x0f;
  m_length = (byte >> 5) & 0x07;

  uint16_t bytes = i.ReadU16 ();
  m_length |= (bytes << 3) & 0x0ff8;

  return i.GetDistanceFrom (start);
}

NS_OBJECT_ENSURE_REGISTERED (HtSigHeader);

HtSigHeader::HtSigHeader ()
  : m_mcs (0),
    m_cbw20_40 (0),
    m_htLength (0),
    m_aggregation (0),
    m_sgi (0)
{
}

HtSigHeader::~HtSigHeader ()
{
}

TypeId
HtSigHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HtSigHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<HtSigHeader> ()
  ;
  return tid;
}

TypeId
HtSigHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
HtSigHeader::Print (std::ostream &os) const
{
  os << "MCS=" << +m_mcs
     << " HT_LENGTH=" << m_htLength
     << " CHANNEL_WIDTH=" << GetChannelWidth ()
     << " FEC_CODING=" << (m_fecCoding ? "LDPC" : "BCC")
     << " SGI=" << +m_sgi
     << " AGGREGATION=" << +m_aggregation;
}

uint32_t
HtSigHeader::GetSerializedSize (void) const
{
  return 6;
}

void
HtSigHeader::SetMcs (uint8_t mcs)
{
  NS_ASSERT (mcs <= 31);
  m_mcs = mcs;
}

uint8_t
HtSigHeader::GetMcs (void) const
{
  return m_mcs;
}

void
HtSigHeader::SetChannelWidth (uint16_t channelWidth)
{
  m_cbw20_40 = (channelWidth > 20) ? 1 : 0;
}

uint16_t
HtSigHeader::GetChannelWidth (void) const
{
  return m_cbw20_40 ? 40 : 20;
}

void
HtSigHeader::SetHtLength (uint16_t length)
{
  m_htLength = length;
}

uint16_t
HtSigHeader::GetHtLength (void) const
{
  return m_htLength;
}

void
HtSigHeader::SetAggregation (bool aggregation)
{
  m_aggregation = aggregation ? 1 : 0;
}

bool
HtSigHeader::GetAggregation (void) const
{
  return m_aggregation ? true : false;
}

void
HtSigHeader::SetFecCoding (bool ldpc)
{
  m_fecCoding = ldpc ? 1 : 0;
}

bool
HtSigHeader::IsLdpcFecCoding (void) const
{
  return m_fecCoding ? true : false;
}

void
HtSigHeader::SetShortGuardInterval (bool sgi)
{
  m_sgi = sgi ? 1 : 0;
}

bool
HtSigHeader::GetShortGuardInterval (void) const
{
  return m_sgi ? true : false;
}

void
HtSigHeader::Serialize (Buffer::Iterator start) const
{
  uint8_t byte = m_mcs;
  byte |= ((m_cbw20_40 & 0x01) << 7);
  start.WriteU8 (byte);
  start.WriteU16 (m_htLength);
  byte = (0x01 << 2); //Set Reserved bit #2 to 1
  byte |= ((m_aggregation & 0x01) << 3);
  byte |= ((m_fecCoding & 0x01) << 6);
  byte |= ((m_sgi & 0x01) << 7);
  start.WriteU8 (byte);
  start.WriteU16 (0);
}

uint32_t
HtSigHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t byte = i.ReadU8 ();
  m_mcs = byte & 0x7f;
  m_cbw20_40 = ((byte >> 7) & 0x01);
  m_htLength = i.ReadU16 ();
  byte = i.ReadU8 ();
  m_aggregation = ((byte >> 3) & 0x01);
  m_fecCoding = ((byte >> 6) & 0x01);
  m_sgi = ((byte >> 7) & 0x01);
  i.ReadU16 ();
  return i.GetDistanceFrom (start);
}

NS_OBJECT_ENSURE_REGISTERED (VhtSigHeader);

VhtSigHeader::VhtSigHeader ()
  : m_bw (0),
    m_nsts (0),
    m_sgi (0),
    m_sgi_disambiguation (0),
    m_suMcs (0),
    m_mu (false)
{
}

VhtSigHeader::~VhtSigHeader ()
{
}

TypeId
VhtSigHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VhtSigHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<VhtSigHeader> ()
  ;
  return tid;
}

TypeId
VhtSigHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
VhtSigHeader::Print (std::ostream &os) const
{
  os << "SU_MCS=" << +m_suMcs
     << " CHANNEL_WIDTH=" << GetChannelWidth ()
     << " SGI=" << +m_sgi
     << " NSTS=" << +m_nsts
     << " CODING=" << (m_coding ? "LDPC" : "BCC")
     << " MU=" << +m_mu;
}

uint32_t
VhtSigHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 3; //VHT-SIG-A1
  size += 3; //VHT-SIG-A2
  if (m_mu)
    {
      size += 4; //VHT-SIG-B
    }
  return size;
}

void
VhtSigHeader::SetMuFlag (bool mu)
{
  m_mu = mu;
}

void
VhtSigHeader::SetChannelWidth (uint16_t channelWidth)
{
  if (channelWidth == 160)
    {
      m_bw = 3;
    }
  else if (channelWidth == 80)
    {
      m_bw = 2;
    }
  else if (channelWidth == 40)
    {
      m_bw = 1;
    }
  else
    {
      m_bw = 0;
    }
}

uint16_t
VhtSigHeader::GetChannelWidth (void) const
{
  if (m_bw == 3)
    {
      return 160;
    }
  else if (m_bw == 2)
    {
      return 80;
    }
  else if (m_bw == 1)
    {
      return 40;
    }
  else
    {
      return 20;
    }
}

void
VhtSigHeader::SetNStreams (uint8_t nStreams)
{
  NS_ASSERT (nStreams <= 8);
  m_nsts = (nStreams - 1);
}

uint8_t
VhtSigHeader::GetNStreams (void) const
{
  return (m_nsts + 1);
}

void
VhtSigHeader::SetShortGuardInterval (bool sgi)
{
  m_sgi = sgi ? 1 : 0;
}

bool
VhtSigHeader::GetShortGuardInterval (void) const
{
  return m_sgi ? true : false;
}

void
VhtSigHeader::SetShortGuardIntervalDisambiguation (bool disambiguation)
{
  m_sgi_disambiguation = disambiguation ? 1 : 0;
}
  
bool
VhtSigHeader::GetShortGuardIntervalDisambiguation (void) const
{
  return m_sgi_disambiguation ? true : false;
}

void
VhtSigHeader::SetCoding (bool ldpc)
{
  m_coding = ldpc ? 1 : 0;
}

bool
VhtSigHeader::IsLdpcCoding (void) const
{
  return m_coding ? true : false;

}

void
VhtSigHeader::SetSuMcs (uint8_t mcs)
{
  NS_ASSERT (mcs <= 9);
  m_suMcs = mcs;
}

uint8_t
VhtSigHeader::GetSuMcs (void) const
{
  return m_suMcs;
}

void
VhtSigHeader::Serialize (Buffer::Iterator start) const
{
  //VHT-SIG-A1
  uint8_t byte = m_bw;
  byte |= (0x01 << 2); //Set Reserved bit #2 to 1
  start.WriteU8 (byte);
  uint16_t bytes = (m_nsts & 0x07) << 2;
  bytes |= (0x01 << (23 - 8)); //Set Reserved bit #23 to 1
  start.WriteU16 (bytes);

  //VHT-SIG-A2
  byte = m_sgi & 0x01;
  byte |= ((m_sgi_disambiguation & 0x01) << 1);
  byte |= ((m_coding & 0x01) << 2);
  byte |= ((m_suMcs & 0x0f) << 4);
  start.WriteU8 (byte);
  bytes = (0x01 << (9 - 8)); //Set Reserved bit #9 to 1
  start.WriteU16 (bytes);

  if (m_mu)
    {
      //VHT-SIG-B
      start.WriteU32 (0);
    }
}

uint32_t
VhtSigHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  //VHT-SIG-A1
  uint8_t byte = i.ReadU8 ();
  m_bw = byte & 0x03;
  uint16_t bytes = i.ReadU16 ();
  m_nsts = ((bytes >> 2) & 0x07);

  //VHT-SIG-A2
  byte = i.ReadU8 ();
  m_sgi = byte & 0x01;
  m_sgi_disambiguation = ((byte >> 1) & 0x01);
  m_coding = ((byte >> 2) & 0x01);
  m_suMcs = ((byte >> 4) & 0x0f);
  i.ReadU16 ();

  if (m_mu)
    {
      //VHT-SIG-B
      i.ReadU32 ();
    }

  return i.GetDistanceFrom (start);
}


NS_OBJECT_ENSURE_REGISTERED (HeSigHeader);

HeSigHeader::HeSigHeader ()
  : m_format (1),
    m_bssColor (0),
    m_ul_dl (0),
    m_mcs (0),
    m_spatialReuse (0),
    m_bandwidth (0),
    m_gi_ltf_size (0),
    m_nsts (0),
    m_mu (false)
{
}

HeSigHeader::~HeSigHeader ()
{
}

TypeId
HeSigHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HeSigHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<HeSigHeader> ()
  ;
  return tid;
}

TypeId
HeSigHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
HeSigHeader::Print (std::ostream &os) const
{
  os << "MCS=" << +m_mcs
     << " CHANNEL_WIDTH=" << GetChannelWidth ()
     << " GI=" << GetGuardInterval ()
     << " NSTS=" << +m_nsts
     << " BSSColor=" << +m_bssColor
     << " CODING=" << (m_coding ? "LDPC" : "BCC")
     << " MU=" << +m_mu;
}

uint32_t
HeSigHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 4; //HE-SIG-A1
  size += 4; //HE-SIG-A2
  if (m_mu)
    {
      size += 1; //HE-SIG-B
    }
  return size;
}

void
HeSigHeader::SetMuFlag (bool mu)
{
  m_mu = mu;
}

void
HeSigHeader::SetMcs (uint8_t mcs)
{
  NS_ASSERT (mcs <= 11);
  m_mcs = mcs;
}

uint8_t
HeSigHeader::GetMcs (void) const
{
  return m_mcs;
}

void
HeSigHeader::SetBssColor (uint8_t bssColor)
{
  NS_ASSERT (bssColor < 64);
  m_bssColor = bssColor;
}

uint8_t
HeSigHeader::GetBssColor (void) const
{
  return m_bssColor;
}

void
HeSigHeader::SetChannelWidth (uint16_t channelWidth)
{
  if (channelWidth == 160)
    {
      m_bandwidth = 3;
    }
  else if (channelWidth == 80)
    {
      m_bandwidth = 2;
    }
  else if (channelWidth == 40)
    {
      m_bandwidth = 1;
    }
  else
    {
      m_bandwidth = 0;
    }
}

uint16_t
HeSigHeader::GetChannelWidth (void) const
{
  if (m_bandwidth == 3)
    {
      return 160;
    }
  else if (m_bandwidth == 2)
    {
      return 80;
    }
  else if (m_bandwidth == 1)
    {
      return 40;
    }
  else
    {
      return 20;
    }
}

void
HeSigHeader::SetGuardIntervalAndLtfSize (uint16_t gi, uint8_t ltf)
{
  if (gi == 800 && ltf == 1)
    {
      m_gi_ltf_size = 0;
    }
  else if (gi == 800 && ltf == 2)
    {
      m_gi_ltf_size = 1;
    }
  else if (gi == 1600 && ltf == 2)
    {
      m_gi_ltf_size = 2;
    }
  else
    {
      m_gi_ltf_size = 3;
    }
}

uint16_t
HeSigHeader::GetGuardInterval (void) const
{
  if (m_gi_ltf_size == 3)
    {
      //we currently do not consider DCM nor STBC fields
      return 3200;
    }
  else if (m_gi_ltf_size == 2)
    {
      return 1600;
    }
  else
    {
      return 800;
    }
}

void
HeSigHeader::SetNStreams (uint8_t nStreams)
{
  NS_ASSERT (nStreams <= 8);
  m_nsts = (nStreams - 1);
}

uint8_t
HeSigHeader::GetNStreams (void) const
{
  return (m_nsts + 1);
}

void
HeSigHeader::SetCoding (bool ldpc)
{
  m_coding = ldpc ? 1 : 0;
}

bool
HeSigHeader::IsLdpcCoding (void) const
{
  return m_coding ? true : false;

}

void
HeSigHeader::Serialize (Buffer::Iterator start) const
{
  //HE-SIG-A1
  uint8_t byte = m_format & 0x01;
  byte |= ((m_ul_dl & 0x01) << 2);
  byte |= ((m_mcs & 0x0f) << 3);
  start.WriteU8 (byte);
  uint16_t bytes = (m_bssColor & 0x3f);
  bytes |= (0x01 << 6); //Reserved set to 1
  bytes |= ((m_spatialReuse & 0x0f) << 7);
  bytes |= ((m_bandwidth & 0x03) << 11);
  bytes |= ((m_gi_ltf_size & 0x03) << 13);
  bytes |= ((m_nsts & 0x01) << 15);
  start.WriteU16 (bytes);
  start.WriteU8 ((m_nsts >> 1) & 0x03);

  //HE-SIG-A2
  uint32_t sigA2 = 0;
  sigA2 |= ((m_coding & 0x01) << 7);
  sigA2 |= (0x01 << 14); //Set Reserved bit #14 to 1
  start.WriteU32 (sigA2);

  if (m_mu)
    {
      //HE-SIG-B
      start.WriteU8 (0);
    }
}

uint32_t
HeSigHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  //HE-SIG-A1
  uint8_t byte = i.ReadU8 ();
  m_format = (byte & 0x01);
  m_ul_dl = ((byte >> 2) & 0x01);
  m_mcs = ((byte >> 3) & 0x0f);
  uint16_t bytes = i.ReadU16 ();
  m_bssColor = (bytes & 0x3f);
  m_spatialReuse = ((bytes >> 7) & 0x0f);
  m_bandwidth = ((bytes >> 11) & 0x03);
  m_gi_ltf_size = ((bytes >> 13) & 0x03);
  m_nsts = ((bytes >> 15) & 0x01);
  byte = i.ReadU8 ();
  m_nsts |= (byte & 0x03) << 1;

  //HE-SIG-A2
  uint32_t sigA2 = i.ReadU32 ();
  m_coding = ((sigA2 >> 7) & 0x01);

  if (m_mu)
    {
      //HE-SIG-B
      i.ReadU8 ();
    }

  return i.GetDistanceFrom (start);
}

} //namespace ns3
