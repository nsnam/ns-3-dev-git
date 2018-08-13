/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Matthieu Coudron
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
 * Author:  Kashif Nadeem <kshfnadeem@gmail.com>
 *          Matthieu Coudron <matthieu.coudron@lip6.fr>
 */

#include "tcp-option-mptcp.h"
#include "ns3/log.h"

static inline
uint64_t TRUNC_TO_32(uint64_t seq) {
  return static_cast<uint32_t>(seq);
}

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpTcpCapable);
NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpTcpAddAddress );
NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpTcpRemoveAddress );
NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpTcpJoin);
NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpTcpChangePriority );
NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpTcpDSS);
NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpTcpFail);
NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpTcpFastClose);

/**
\note This is a global MPTCP option logger
*/
NS_LOG_COMPONENT_DEFINE ("TcpOptionMpTcp");

/////////////////////////////////////////////////////////
////////  Base for MPTCP options
/////////////////////////////////////////////////////////
TcpOptionMpTcpMain::TcpOptionMpTcpMain ()
  : TcpOption ()
{
  NS_LOG_FUNCTION (this);
}

TcpOptionMpTcpMain::~TcpOptionMpTcpMain ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TcpOptionMpTcpMain::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint8_t
TcpOptionMpTcpMain::GetKind (void) const
{
  return TcpOption::MPTCP;
}

TypeId
TcpOptionMpTcpMain::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpTcpMain")
    .SetParent<TcpOption> ()
  ;
  return tid;
}

void
TcpOptionMpTcpMain::Print (std::ostream &os) const
{
  NS_ASSERT_MSG (false, " You should override TcpOptionMpTcp::Print function");
}

std::string
TcpOptionMpTcpMain::SubTypeToString (const uint8_t& flags, const std::string& delimiter)
{
  static const char* flagNames[8] = {
    "CAPABLE",
    "JOIN",
    "DSS",
    "ADD_ADDR",
    "REM_ADDR",
    "CHANGE_PRIORITY",
    "MP_FAIL",
    "MP_FASTCLOSE"
  };

  std::string flagsDescription = "";

  for (int i = 0; i < 8; ++i)
    {
      if ( flags & (1 << i) )
        {
          if (flagsDescription.length () > 0)
            {
              flagsDescription += delimiter;
            }
          flagsDescription.append ( flagNames[i] );

        }
    }
  return flagsDescription;
}

Ptr<TcpOption>
TcpOptionMpTcpMain::CreateMpTcpOption (const uint8_t& subtype)
{
  NS_LOG_FUNCTION_NOARGS();
  switch (subtype)
    {
    case MP_CAPABLE:
      return CreateObject<TcpOptionMpTcpCapable>();
    case MP_JOIN:
      return CreateObject<TcpOptionMpTcpJoin>();
    case MP_DSS:
      return CreateObject<TcpOptionMpTcpDSS>();
    case MP_FAIL:
      return CreateObject<TcpOptionMpTcpFail>();
    case MP_FASTCLOSE:
      return CreateObject<TcpOptionMpTcpFastClose>();
    case MP_PRIO:
      return CreateObject<TcpOptionMpTcpChangePriority>();
    case MP_REMOVE_ADDR:
      return CreateObject<TcpOptionMpTcpRemoveAddress>();
    case MP_ADD_ADDR:
      return CreateObject<TcpOptionMpTcpAddAddress>();
    default:
      break;
    }

  NS_FATAL_ERROR ("Unsupported MPTCP suboption" << subtype);
  return 0;
}

void
TcpOptionMpTcpMain::SerializeRef (Buffer::Iterator& i) const
{
  i.WriteU8 (GetKind ());
  i.WriteU8 (GetSerializedSize ());
}

uint32_t
TcpOptionMpTcpMain::DeserializeRef (Buffer::Iterator& i) const
{
  uint8_t kind = i.ReadU8 ();
  uint32_t length = 0;

  NS_ASSERT (kind == GetKind ());

  length = static_cast<uint32_t>(i.ReadU8 ());
  return length;
}

/////////////////////////////////////////////////////////
////////  MP_CAPABLE
/////////////////////////////////////////////////////////
TcpOptionMpTcpCapable::TcpOptionMpTcpCapable ()
  : TcpOptionMpTcp (),
    m_version (0),
    m_flags ( HMAC_SHA1 ),
    m_senderKey (0),
    m_remoteKey (0),
    m_length (12)
{
  NS_LOG_FUNCTION (this);
}

TcpOptionMpTcpCapable::~TcpOptionMpTcpCapable ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

TypeId
TcpOptionMpTcpCapable::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpTcpCapable")
    .SetParent<TcpOptionMpTcpMain> ()
    .AddConstructor<TcpOptionMpTcpCapable> ()
  ;
  return tid;
}

TypeId
TcpOptionMpTcpCapable::GetInstanceTypeId (void) const
{
  return TcpOptionMpTcpCapable::GetTypeId ();
}

bool
TcpOptionMpTcpCapable::operator== (const TcpOptionMpTcpCapable& opt) const
{
  return (GetPeerKey () == opt.GetPeerKey () && GetSenderKey () == opt.GetSenderKey () );
}

void
TcpOptionMpTcpCapable::SetSenderKey (const uint64_t& senderKey)
{
  NS_LOG_FUNCTION (this);
  m_senderKey = senderKey;
}

void
TcpOptionMpTcpCapable::SetPeerKey (const uint64_t& remoteKey)
{
  NS_LOG_FUNCTION (this);
  m_length = 20;
  m_remoteKey = remoteKey;
}

void
TcpOptionMpTcpCapable::Print (std::ostream &os) const
{
  os << "MP_CAPABLE:"
     << " flags=" << (int)m_flags << "]"
     << " Sender's Key :[" << GetSenderKey () << "]";
  if ( HasReceiverKey () )
    {
      os << " Peer's Key [" << GetPeerKey () << "]";
    }

}

bool
TcpOptionMpTcpCapable::IsChecksumRequired () const
{
  return (m_flags >> 7);
}

void
TcpOptionMpTcpCapable::Serialize (Buffer::Iterator i) const
{
  TcpOptionMpTcp::SerializeRef (i);

  i.WriteU8 ( (GetSubType () << 4) + (0x0f & GetVersion ()) ); // Kind
  i.WriteU8 ( m_flags ); //
  i.WriteHtonU64 ( GetSenderKey () );
  if ( HasReceiverKey () )
    {
      i.WriteHtonU64 ( GetPeerKey () );
    }
}

uint32_t
TcpOptionMpTcpCapable::Deserialize (Buffer::Iterator i)
{
  uint32_t length = TcpOptionMpTcpMain::DeserializeRef (i);
  NS_ASSERT ( length == 12 || length == 20 );

  uint8_t subtype_and_version = i.ReadU8 ();
  NS_ASSERT ( subtype_and_version >> 4 == GetSubType () );
  m_flags = i.ReadU8 ();

  SetSenderKey ( i.ReadNtohU64 () );

  if (length == 20)
    {
      SetPeerKey ( i.ReadNtohU64 () );
    }
  return length;
}

uint32_t
TcpOptionMpTcpCapable::GetSerializedSize (void) const
{
  return (m_length);
}

uint8_t
TcpOptionMpTcpCapable::GetVersion (void) const
{
  return 0;
}

uint64_t
TcpOptionMpTcpCapable::GetSenderKey (void) const
{
  return m_senderKey;
}

uint64_t
TcpOptionMpTcpCapable::GetPeerKey (void) const
{
  return m_remoteKey;
}

bool
TcpOptionMpTcpCapable::HasReceiverKey (void) const
{
  return GetSerializedSize () == 20;
}

/////////////////////////////////////////////////////////
////////  MP_JOIN Initial SYN
/////////////////////////////////////////////////////////

TypeId
TcpOptionMpTcpJoin::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpTcpJoin")
    .SetParent<TcpOptionMpTcpMain> ()
    .AddConstructor<TcpOptionMpTcpJoin> ()
  ;
  return tid;
}

TypeId
TcpOptionMpTcpJoin::GetInstanceTypeId (void) const
{
  return TcpOptionMpTcpJoin::GetTypeId ();
}

TcpOptionMpTcpJoin::TcpOptionMpTcpJoin ()
  : TcpOptionMpTcp (),
    m_mode (Uninitialized),
    m_addressId (0),
    m_flags (0)
{
  NS_LOG_FUNCTION (this);
  memset(&m_buffer[0], 0, sizeof(uint32_t) * 5);
}

TcpOptionMpTcpJoin::~TcpOptionMpTcpJoin ()
{
  NS_LOG_FUNCTION (this);
}

void
TcpOptionMpTcpJoin::SetPeerToken (const uint32_t& token)
{
  NS_ASSERT ( m_mode & Syn);
  m_buffer[0] = token;
}

void
TcpOptionMpTcpJoin::Print (std::ostream &os) const
{
  os << "MP_JOIN: ";
  switch (m_mode)
    {
    case Uninitialized:
      os << "Uninitialized";
      return;
    case Syn:
      os << "[Syn] with token=" << GetPeerToken () << ", nonce=" << GetNonce ();
      return;
    case SynAck:
      os << "[SynAck] with nonce=" << GetNonce ()<< ", HMAC-B=" << GetTruncatedHmac ();
      return;
    case Ack:
      os << "[Ack] with hash";
      return;
    }
}

void
TcpOptionMpTcpJoin::SetAddressId (const uint8_t& addrId)
{
  NS_ASSERT (m_mode & (SynAck | Ack) );
  m_addressId =  addrId;
}

uint32_t
TcpOptionMpTcpJoin::GetPeerToken () const
{
  NS_ASSERT (m_mode & Syn );
  return m_buffer[0];
}

void
TcpOptionMpTcpJoin::SetMode (Mode s)
{
  NS_ASSERT_MSG (m_mode == Uninitialized, "Can't change state once initialized.");
  m_mode = s;
}

uint8_t
TcpOptionMpTcpJoin::GetAddressId () const
{
  NS_ASSERT_MSG (m_mode & (SynAck | Ack), "AddressId only available in states SynAck and Ack");
  return m_addressId;
}

bool
TcpOptionMpTcpJoin::operator== (const TcpOptionMpTcpJoin& opt) const
{

  if ( m_mode != opt.m_mode)
    {
      return false;
    }

  /* depending on mode, operator does not check same fields */
  switch (m_mode)
    {
    case Uninitialized:
      return true;
    case Syn:
      return (
               GetPeerToken () == opt.GetPeerToken ()
               && GetNonce () == opt.GetNonce ()
               && GetAddressId () == opt.GetAddressId ()
               );
    case SynAck:
      return (
               GetNonce () == opt.GetNonce ()
               && GetAddressId ()  == opt.GetAddressId ()
               );
    case Ack:
      return true;
    }

  NS_FATAL_ERROR ( "This should never trigger. Contact ns3 team");
  return false;
}

TcpOptionMpTcpJoin::Mode
TcpOptionMpTcpJoin::GetMode (void) const
{
  return m_mode;
}

uint32_t
TcpOptionMpTcpJoin::GetNonce () const
{
  NS_ASSERT_MSG (m_mode & (Syn | SynAck), "Nonce only available in Syn and SynAck modes");
  return m_buffer[0];
}

void
TcpOptionMpTcpJoin::SetNonce (const uint32_t& nonce)
{
  if (m_mode == Syn)
    {
      m_buffer[1] = nonce;
    }
  else if (m_mode == SynAck)
    {
      m_buffer[2] = nonce;
    }
  else
    {
      NS_FATAL_ERROR ("Unavailable command in this mode");
    }
}

void
TcpOptionMpTcpJoin::Serialize (Buffer::Iterator i) const
{
  TcpOptionMpTcp::SerializeRef (i);
  i.WriteU8 ( GetSubType () << 4 );
  if (m_mode & (Syn | SynAck))
    {
      i.WriteU8 (GetAddressId ());
    }
  else
    {
      i.WriteU8 ( 0 );
    }

  switch ( m_mode )
    {
    case Uninitialized:
      NS_FATAL_ERROR ("Uninitialized option");

    case Syn:
      i.WriteHtonU32 ( GetPeerToken () );
      i.WriteHtonU32 ( GetNonce () );
      break;
    case SynAck:
      {
        uint64_t hmac = GetTruncatedHmac ();
        i.WriteHtonU64 (hmac);
      }
      i.WriteHtonU32 ( GetNonce () );
      break;
    case Ack:
      // +=4 cos' amount of bytes we write
      for (int j = 0; j < m_mode / 4 - 1; j++)
        {
          i.WriteHtonU32 ( m_buffer[j]);
        }
      break;
    default:
      NS_FATAL_ERROR("Unhandled case");
    }
}

const uint8_t*
TcpOptionMpTcpJoin::GetHmac () const
{
  NS_ASSERT_MSG (m_mode == Ack, "Only available in Ack mode");
  return 0;
}

uint32_t
TcpOptionMpTcpJoin::Deserialize (Buffer::Iterator i)
{
  NS_ASSERT (m_mode == Uninitialized);

  uint32_t length = TcpOptionMpTcpMain::DeserializeRef (i);
  uint8_t subtype_and_flags = i.ReadU8 ();
  NS_ASSERT ( (subtype_and_flags >> 4) == GetSubType () );
  m_mode = static_cast<Mode> ( length );
  m_addressId = i.ReadU8 ();

  switch ( m_mode )
    {
    case Uninitialized:
      NS_FATAL_ERROR ("Unitialized option, this case should not happen." );
    case Syn:
      SetPeerToken ( i.ReadNtohU32 () );
      SetNonce ( i.ReadNtohU32 () );
      break;
    case SynAck:
      SetTruncatedHmac ( i.ReadNtohU64 () );
      SetNonce ( i.ReadNtohU32 () );  // read nonce
      break;
    case Ack:
      i.Read ( (uint8_t*)&m_buffer, 20);
      break;
    }

  return m_mode;
}

void
TcpOptionMpTcpJoin::SetHmac (uint8_t hmac[20])
{
  NS_LOG_ERROR ("Not implemented");
}

uint32_t
TcpOptionMpTcpJoin::GetSerializedSize (void) const
{
  NS_ASSERT (m_mode != Uninitialized);
  return (m_mode);
}

void
TcpOptionMpTcpJoin::SetTruncatedHmac (const uint64_t& hmac)
{
  NS_ASSERT_MSG (m_mode == SynAck, "Wrong mode");
  m_buffer[0] = hmac >> 32;
  m_buffer[1] = (hmac);
}

uint64_t
TcpOptionMpTcpJoin::GetTruncatedHmac () const
{
  NS_ASSERT_MSG (m_mode == SynAck, "Wrong mode");
  uint64_t temp = 0;
  temp = m_buffer[0];
  temp = temp << 32;
  temp |= m_buffer[1];

  return temp;
}

///////////////////////////////////////:
//// MP_DSS
TcpOptionMpTcpDSS::TcpOptionMpTcpDSS ()
  : TcpOptionMpTcp (),
    m_hasChecksum (false),
    m_checksum (0),
    m_flags (0),
    m_dataAck (0),
    m_dsn (0),
    m_ssn (0),
    m_dataLevelLength (0)
{
  NS_LOG_FUNCTION (this);
}

TcpOptionMpTcpDSS::~TcpOptionMpTcpDSS ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TcpOptionMpTcpDSS::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpTcpDSS")
    .SetParent<TcpOptionMpTcpMain> ()
    .AddConstructor<TcpOptionMpTcpDSS> ()
  ;
  return tid;
}

TypeId
TcpOptionMpTcpDSS::GetInstanceTypeId (void) const
{
  return TcpOptionMpTcpDSS::GetTypeId ();
}

void
TcpOptionMpTcpDSS::TruncateDSS(bool truncate)
{
  NS_ASSERT_MSG(m_flags & DSNMappingPresent, "Call it only after setting the mapping");

  if (truncate)
    { 
      m_flags &=  ~(0xff & DSNOfEightBytes);
    }
  else
   {
     m_flags |= DSNOfEightBytes;
   }
}

void
TcpOptionMpTcpDSS::SetMapping (uint64_t headDsn, uint32_t headSsn, uint16_t length, bool enable_dfin)
{
  NS_ASSERT_MSG ( !(m_flags & DataFin), "For now you can't set mapping after enabling datafin");
  m_dsn = headDsn;
  m_ssn = headSsn;
  // += in case there is a datafin
  m_dataLevelLength = length;
  m_flags |= DSNMappingPresent;

  if(enable_dfin)
  {
    m_flags |= DataFin;
  }
}

void
TcpOptionMpTcpDSS::GetMapping (uint64_t& dsn, uint32_t& ssn, uint16_t& length) const
{
  NS_ASSERT ( (m_flags & DSNMappingPresent) && !IsInfiniteMapping () );
  ssn = m_ssn;
  dsn = m_dsn;
  length = m_dataLevelLength;
  if (GetFlags () & DataFin)
    {
      length--;
    }
}

uint32_t
TcpOptionMpTcpDSS::GetSerializedSize (void) const
{
  uint32_t len = GetSizeFromFlags (m_flags) + ((m_hasChecksum) ? 2 : 0);
  return len;
}

uint64_t
TcpOptionMpTcpDSS::GetDataAck (void) const
{
  NS_ASSERT_MSG ( m_flags & DataAckPresent, "Can't request DataAck value when DataAck flag was not set. Check for its presence first" );
  return m_dataAck;
}

void
TcpOptionMpTcpDSS::SetChecksum (const uint16_t& checksum)
{
  m_hasChecksum = checksum;
}

uint16_t
TcpOptionMpTcpDSS::GetChecksum (void) const
{
  NS_ASSERT (m_hasChecksum);
  return m_checksum;
}

void
TcpOptionMpTcpDSS::Print (std::ostream& os) const
{
  os << " MP_DSS: ";
  if (GetFlags () & DataAckPresent)
    {
      os << "Acknowledges [" << GetDataAck () << "] ";
      if (GetFlags () & DataAckOf8Bytes)
        {
          os << "(8bytes DACK)";
        }
    }

  if (GetFlags () & DSNMappingPresent)
    {

      if (IsInfiniteMapping ())
        {
          os << " Infinite Mapping";
        }
      else if (GetFlags () & DataFin)
        {
          os << "Has datafin for seq [" << GetDataFinDSN () << "]";
        }
      os << " DSN:" << m_dsn << " length=" << m_dataLevelLength;
      if (GetFlags () & DSNOfEightBytes)
        {
          os << "(8bytes mapping)";
        }
    }
}

void
TcpOptionMpTcpDSS::Serialize (Buffer::Iterator i) const
{
  TcpOptionMpTcp::SerializeRef (i);
  i.WriteU8 ( GetSubType () << 4);
  i.WriteU8 ( m_flags );

  if ( m_flags & DataAckPresent)
    {
      if ( m_flags & DataAckOf8Bytes)
        {
          i.WriteHtonU64 ( m_dataAck );
        }
      else
        {
          i.WriteHtonU32 ( static_cast<uint32_t>(m_dataAck) );
        }
    }

  if (m_flags & DSNMappingPresent)
    {

      if ( m_flags & DSNOfEightBytes)
        {
          i.WriteHtonU64 ( m_dsn );
        }
      else
        {
          i.WriteHtonU32 ( m_dsn );
        }

      // Write relative SSN
      i.WriteHtonU32 ( m_ssn );
      i.WriteHtonU16 ( m_dataLevelLength );
    }
  if (m_hasChecksum)
    {
      i.WriteHtonU16 ( m_checksum );
    }
}

uint32_t
TcpOptionMpTcpDSS::GetSizeFromFlags (uint16_t flags)
{
  uint32_t length = 4;

  if ( flags & DataAckPresent)
    {
      length += 4;
      if ( flags & DataAckOf8Bytes)
        {
          length += 4;
        }
    }
  if ( flags & DSNMappingPresent)
    {
      length += 10; /* data length (2) + ssn (4) + DSN min size (4) */
      if ( flags & DSNOfEightBytes)
        {
          length += 4;
        }
    }
  return length;
}

uint32_t
TcpOptionMpTcpDSS::Deserialize (Buffer::Iterator i)
{
  uint32_t length =  TcpOptionMpTcpMain::DeserializeRef (i);
  uint8_t subtype_and_reserved = i.ReadU8 ();

  NS_ASSERT ( (subtype_and_reserved >> 4) == GetSubType ()  );
  m_flags = i.ReadU8 ();
  uint32_t shouldBeLength = GetSizeFromFlags (m_flags);
  NS_ASSERT (shouldBeLength == length || shouldBeLength + 2 == length);

  if (shouldBeLength + 2 == length)
    {
      m_hasChecksum = true;
    }
  if ( m_flags & DataAckPresent)
    {
      if ( m_flags & DataAckOf8Bytes)
        {
          m_dataAck = i.ReadNtohU64 ();
        }
      else
        {
          m_dataAck = i.ReadNtohU32 ();
        }
    }
  // Read mapping
  if (m_flags & DSNMappingPresent)
    {

      if ( m_flags & DSNOfEightBytes)
        {
          m_dsn = i.ReadNtohU64 ();
        }
      else
        {
          m_dsn = i.ReadNtohU32 ();
        }
      m_ssn = i.ReadNtohU32 ();
      m_dataLevelLength = i.ReadNtohU16 ();
    }

  if (m_hasChecksum)
    {
      m_checksum = i.ReadNtohU16 ( );
    }

  return length;
}

uint8_t
TcpOptionMpTcpDSS::GetFlags (void) const
{
  return m_flags;
}

/*
Note that when the DATA_FIN is not attached to a TCP segment
containing data, the Data Sequence Signal MUST have a subflow
sequence number of 0, a Data-Level Length of 1, and the data sequence
number that corresponds with the DATA_FIN itself
*/
bool
TcpOptionMpTcpDSS::DataFinMappingOnly () const
{
  return (m_flags & DataFin) && m_dataLevelLength == 1 && m_ssn == 0;
}

bool
TcpOptionMpTcpDSS::IsInfiniteMapping () const
{
  // The checksum, in such a case, will also be set to zero
  return (GetFlags () & DSNMappingPresent) && m_dataLevelLength == 0;
}

uint64_t
TcpOptionMpTcpDSS::GetDataFinDSN () const
{
  NS_ASSERT ( GetFlags () & DataFin);

  if (DataFinMappingOnly ())
    {
      return m_dsn;
    }
  else
    {
      return m_dsn + m_dataLevelLength;
    }
}

void
TcpOptionMpTcpDSS::SetDataAck (const uint64_t& dack, const bool& send_as_32bits)
{
  NS_LOG_LOGIC (this << dack);

  m_dataAck = dack;
  m_flags |= DataAckPresent;

  if (send_as_32bits)
    {
      m_dataAck = TRUNC_TO_32 (m_dataAck);
    }
  else
    {
      m_flags |= DataAckOf8Bytes;
    }
}

bool
TcpOptionMpTcpDSS::operator== (const TcpOptionMpTcpDSS& opt) const
{
  bool ret = m_flags == opt.m_flags;
  ret &= opt.m_checksum == m_checksum;
  ret &= opt.m_dsn == m_dsn;
  ret &= opt.m_ssn == m_ssn;
  ret &= opt.m_dataAck == m_dataAck;
  return ( ret );
}

///////////////////////////////////////:
//// ADD_ADDR
////
TcpOptionMpTcpAddAddress::TcpOptionMpTcpAddAddress ()
  : TcpOptionMpTcp (),
    m_addressVersion (0),
    m_addrId (0)
{
  NS_LOG_FUNCTION (this);
}

TcpOptionMpTcpAddAddress::~TcpOptionMpTcpAddAddress ()
{
  NS_LOG_FUNCTION (this);

}

TypeId
TcpOptionMpTcpAddAddress::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpTcpAddAddress")
    .SetParent<TcpOptionMpTcpMain> ()
    .AddConstructor<TcpOptionMpTcpAddAddress> ()
  ;
  return tid;
}

TypeId
TcpOptionMpTcpAddAddress::GetInstanceTypeId (void) const
{
  return TcpOptionMpTcpAddAddress::GetTypeId ();
}

void
TcpOptionMpTcpAddAddress::SetAddress (const Address& _address, uint8_t addrId)
{
  if (InetSocketAddress::IsMatchingType (_address) )
    {
      m_addressVersion = 4;
      InetSocketAddress address =  InetSocketAddress::ConvertFrom (_address);
      m_address = address.GetIpv4 ();
      m_port = address.GetPort ();
    }
  else
    {
      NS_ASSERT_MSG (Inet6SocketAddress::IsMatchingType (_address), "Address of unsupported type");
      m_addressVersion = 6;
      Inet6SocketAddress address6 =  Inet6SocketAddress::ConvertFrom (_address);
      m_address6 = address6.GetIpv6 ();
      m_port = address6.GetPort ();
    }

  m_addrId  = addrId;
}

void
TcpOptionMpTcpAddAddress::Print (std::ostream &os) const
{
  os << "ADD_ADDR: address id=" << GetAddressId ()
     << " associated to IP:port [";
  if (m_addressVersion == 4)
    {
      os << m_address<<":";
      os << m_port;
    }
  else
    {
      os << m_address6;
    }
  os << "]";
}

InetSocketAddress
TcpOptionMpTcpAddAddress::GetAddress () const
{
  NS_ASSERT (m_addressVersion == 4);
  return InetSocketAddress (m_address, m_port);
}

Inet6SocketAddress
TcpOptionMpTcpAddAddress::GetAddress6 () const
{
  NS_ASSERT (m_addressVersion == 6);
  return Inet6SocketAddress (m_address6,m_port);
}

uint8_t
TcpOptionMpTcpAddAddress::GetAddressId () const
{
  return m_addrId;
}

void
TcpOptionMpTcpAddAddress::Serialize (Buffer::Iterator i) const
{
  TcpOptionMpTcp::SerializeRef (i);

  NS_ASSERT_MSG (m_addressVersion == 4 || m_addressVersion == 6, "Set an IP before serializing");

  i.WriteU8 ( (GetSubType () << 4) + (uint8_t) m_addressVersion );
  i.WriteU8 ( GetAddressId () );

  if (m_addressVersion == 4)
    {
      i.WriteHtonU32 ( m_address.Get () );
    }
  else
    {
      NS_ASSERT_MSG (m_addressVersion == 6, "You should set an IP address before serializing MPTCP option ADD_ADDR");

      uint8_t     buf[16];
      m_address6.GetBytes ( buf );
      for (int j = 0; j < 16; ++j)
        {
          i.WriteU8 ( buf[j] );
        }
    }

  i.WriteHtonU16 (m_port);
}

uint32_t
TcpOptionMpTcpAddAddress::Deserialize (Buffer::Iterator i)
{
  uint32_t length =  TcpOptionMpTcpMain::DeserializeRef (i);
  NS_ASSERT ( length == 10 || length == 22 );

  uint8_t subtype_and_ipversion = i.ReadU8 ();
  NS_ASSERT ( subtype_and_ipversion >> 4 == GetSubType ()  );

  m_addressVersion = subtype_and_ipversion  & 0x0f;
  NS_ASSERT_MSG (m_addressVersion == 4 || m_addressVersion == 6, "Unsupported address version");

  m_addrId =  i.ReadU8 ();

  if ( m_addressVersion == 4)
    {
      m_address.Set ( i.ReadNtohU32 () );
      m_port = i.ReadNtohU16 () ; 
    }
  else
    {
      NS_FATAL_ERROR ("IPv6 not supported yet");
    }
  return length;
}

uint8_t
TcpOptionMpTcpAddAddress::GetAddressVersion (void) const
{
  return m_addressVersion;
}

uint32_t
TcpOptionMpTcpAddAddress::GetSerializedSize (void) const
{
  if ( GetAddressVersion () == 4)
    {
      return 10;
    }
  NS_ASSERT_MSG ( GetAddressVersion ()  == 6,"Wrong IP version. Maybe you didn't set an address to the MPTCP ADD_ADDR option ?");
  return 22;
}

bool
TcpOptionMpTcpAddAddress::operator== (const TcpOptionMpTcpAddAddress& opt) const
{
  return (GetAddressId () == opt.GetAddressId ()
          && m_address == opt.m_address
          && m_address6 == opt.m_address6
          );
}

///////////////////////////////////////:
//// DEL_ADDR Remove address
////
TcpOptionMpTcpRemoveAddress::TcpOptionMpTcpRemoveAddress ()
  : TcpOptionMpTcp ()
{
  NS_LOG_FUNCTION (this);
}

TcpOptionMpTcpRemoveAddress::~TcpOptionMpTcpRemoveAddress ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TcpOptionMpTcpRemoveAddress::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpTcpRemoveAddress")
    .SetParent<TcpOptionMpTcpMain> ()
    .AddConstructor<TcpOptionMpTcpRemoveAddress> ()
  ;
  return tid;
}

TypeId
TcpOptionMpTcpRemoveAddress::GetInstanceTypeId (void) const
{
  return TcpOptionMpTcpRemoveAddress::GetTypeId ();
}

void
TcpOptionMpTcpRemoveAddress::GetAddresses (std::vector<uint8_t>& addresses)
{
  addresses = m_addressesId;
}

void
TcpOptionMpTcpRemoveAddress::AddAddressId ( uint8_t addrId )
{
  NS_ASSERT_MSG (m_addressesId.size () < 5, "5 is a random limit but it \
            should be weird that you remove more than 5 addresses at once. \
                  Maybe increase it");

  m_addressesId.push_back ( addrId );
}

void
TcpOptionMpTcpRemoveAddress::Serialize (Buffer::Iterator i) const
{
  TcpOptionMpTcp::SerializeRef (i);

  i.WriteU8 ( (GetSubType () << 4) );
  for (
    std::vector<uint8_t>::const_iterator it = m_addressesId.begin ();
    it != m_addressesId.end ();
    it++
    )
    {
      i.WriteU8 ( *it );
    }
}

uint32_t
TcpOptionMpTcpRemoveAddress::Deserialize (Buffer::Iterator i)
{
  uint32_t length =  TcpOptionMpTcpMain::DeserializeRef (i);
  NS_ASSERT_MSG ( length > 3, "You probably forgot to add AddrId to the MPTCP Remove option");
  uint8_t subtype_and_resvd = i.ReadU8 ();
  NS_ASSERT ( subtype_and_resvd >> 4 == GetSubType ()  );
  m_addressesId.clear ();

  for (uint32_t j = 3; j < length; ++j)
    {
      m_addressesId.push_back ( i.ReadU8 () );
    }

  return length;
}

uint32_t
TcpOptionMpTcpRemoveAddress::GetSerializedSize (void) const
{
  return ( 3 + m_addressesId.size () );
}

void
TcpOptionMpTcpRemoveAddress::Print (std::ostream &os) const
{
  os << "REMOVE_ADDR. Removing addresses:";
  for (
    std::vector<uint8_t>::const_iterator it = m_addressesId.begin ();
    it != m_addressesId.end ();
    it++
    )
    {
      os << *it << "/";
    }
}

bool
TcpOptionMpTcpRemoveAddress::operator== (const TcpOptionMpTcpRemoveAddress& opt) const
{
  return (m_addressesId == opt.m_addressesId);

}

///////////////////////////////////////:
//// MP_PRIO change priority
////
TcpOptionMpTcpChangePriority::TcpOptionMpTcpChangePriority ()
  : TcpOptionMpTcp (),
    m_length (3),
    m_addrId (0),
    m_flags (false)
{
  NS_LOG_FUNCTION (this);
}

TcpOptionMpTcpChangePriority::~TcpOptionMpTcpChangePriority (void)
{
  NS_LOG_FUNCTION (this);
}

TypeId
TcpOptionMpTcpChangePriority::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpTcpChangePriority")
    .SetParent<TcpOptionMpTcpMain> ()
    .AddConstructor<TcpOptionMpTcpChangePriority> ()
  ;
  return tid;
}

TypeId
TcpOptionMpTcpChangePriority::GetInstanceTypeId (void) const
{
  return TcpOptionMpTcpChangePriority::GetTypeId ();
}

void
TcpOptionMpTcpChangePriority::Print (std::ostream &os) const
{
  os << "MP_Prio: address with id [";

  if ( EmbeddedAddressId () )
    {
      os << m_addrId;
    }
  else
    {
      os << "Not set";
    }
  os << "] to flags ["  << static_cast<int>(GetFlags ()) << "]";
}

void
TcpOptionMpTcpChangePriority::SetAddressId (const uint8_t& addrId)
{
  m_addrId = addrId;
  m_length = 4;
}

uint8_t
TcpOptionMpTcpChangePriority::GetAddressId () const
{
  NS_ASSERT_MSG (EmbeddedAddressId (),"Use EmbeddedAddressId to check beforehand if this option carries an id");
  return m_addrId;
}

void
TcpOptionMpTcpChangePriority::Serialize (Buffer::Iterator i) const
{
  TcpOptionMpTcp::SerializeRef (i);

  i.WriteU8 ( (GetSubType () << 4) + (uint8_t)m_flags );
  if ( EmbeddedAddressId () )
    {
      i.WriteU8 ( m_addrId );
    }
}

uint32_t
TcpOptionMpTcpChangePriority::Deserialize (Buffer::Iterator i)
{

  uint32_t length =  TcpOptionMpTcpMain::DeserializeRef (i);
  NS_ASSERT ( length == 3 || length == 4 );

  uint8_t subtype_and_flags = i.ReadU8 ();
  NS_ASSERT ( subtype_and_flags >> 4 == GetSubType ()  );
  SetFlags (subtype_and_flags);

  if (length == 4)
    {
      SetAddressId (i.ReadU8 ());
    }

  return m_length;
}

uint32_t
TcpOptionMpTcpChangePriority::GetSerializedSize (void) const
{
  return m_length;
}

void
TcpOptionMpTcpChangePriority::SetFlags (const uint8_t& flags)
{
  NS_LOG_FUNCTION (flags);

  /* only save the LSB bits */
  m_flags = 0x0f & flags;
}

uint8_t
TcpOptionMpTcpChangePriority::GetFlags (void) const
{
  return m_flags & 0x0f;
}

bool
TcpOptionMpTcpChangePriority::EmbeddedAddressId () const
{
  return (GetSerializedSize () == 4);
}

bool
TcpOptionMpTcpChangePriority::operator== (const TcpOptionMpTcpChangePriority& opt) const
{
  return (
           GetFlags () == opt.GetFlags ()
           && m_addrId == opt.m_addrId
         );
}

///////////////////////////////////////////////////
//// MP_FASTCLOSE to totally stop a flow of data
////
TcpOptionMpTcpFastClose::TcpOptionMpTcpFastClose ()
  : TcpOptionMpTcp (),
    m_peerKey (0)
{
  NS_LOG_FUNCTION (this);
}

TcpOptionMpTcpFastClose::~TcpOptionMpTcpFastClose (void)
{
  NS_LOG_FUNCTION (this);
}

void
TcpOptionMpTcpFastClose::SetPeerKey (const uint64_t& remoteKey)
{
  m_peerKey = remoteKey;
}

uint64_t
TcpOptionMpTcpFastClose::GetPeerKey (void) const
{
  return m_peerKey;
}

void
TcpOptionMpTcpFastClose::Print (std::ostream &os) const
{
  os << "MP_FastClose: Receiver key set to ["
     << GetPeerKey () << "]";
}

bool
TcpOptionMpTcpFastClose::operator== (const TcpOptionMpTcpFastClose& opt) const
{

  return ( GetPeerKey () == opt.GetPeerKey () );
}

void
TcpOptionMpTcpFastClose::Serialize (Buffer::Iterator i) const
{
  TcpOptionMpTcp::SerializeRef (i);

  i.WriteU8 ( (GetSubType () << 4) + (uint8_t)0 );
  i.WriteHtonU64 ( GetPeerKey () );
}

uint32_t
TcpOptionMpTcpFastClose::Deserialize (Buffer::Iterator i)
{

  uint32_t length =  TcpOptionMpTcpMain::DeserializeRef (i);
  NS_ASSERT ( length == GetSerializedSize() );
  uint8_t subtype_and_flags = i.ReadU8 ();
  NS_ASSERT ( subtype_and_flags >> 4 == GetSubType ()  );

  SetPeerKey ( i.ReadNtohU64 () );
  return GetSerializedSize();
}

uint32_t
TcpOptionMpTcpFastClose::GetSerializedSize (void) const
{
  return 12;
}

///////////////////////////////////////////////////
//// MP_FAIL to totally stop a flow of data
////
TcpOptionMpTcpFail::TcpOptionMpTcpFail ()
  : TcpOptionMpTcp (),
    m_dsn (0)
{
  NS_LOG_FUNCTION (this);
}

TcpOptionMpTcpFail::~TcpOptionMpTcpFail (void)
{
  NS_LOG_FUNCTION (this);
}

void
TcpOptionMpTcpFail::SetDSN (const uint64_t& dsn)
{
  NS_LOG_FUNCTION (dsn);
  m_dsn = dsn;
}

uint64_t
TcpOptionMpTcpFail::GetDSN (void) const
{
  return m_dsn;
}

void
TcpOptionMpTcpFail::Print (std::ostream &os) const
{
  os << "MP_FAIL for DSN=" << GetDSN ();
}

bool
TcpOptionMpTcpFail::operator== (const TcpOptionMpTcpFail& opt) const
{
  return (GetDSN () == opt.GetDSN ());
}

void
TcpOptionMpTcpFail::Serialize (Buffer::Iterator i) const
{
  TcpOptionMpTcp::SerializeRef (i);

  i.WriteU8 ( (GetSubType () << 4) + (uint8_t)0 );
  i.WriteHtonU64 ( GetDSN () );
}

uint32_t
TcpOptionMpTcpFail::Deserialize (Buffer::Iterator i)
{
  uint32_t length = TcpOptionMpTcpMain::DeserializeRef (i);
  NS_ASSERT ( length == 12 );

  uint8_t subtype_and_flags = i.ReadU8 ();
  NS_ASSERT ( subtype_and_flags >> 4 == GetSubType ()  );
  SetDSN ( i.ReadNtohU64 () );

  return 12;
}

uint32_t
TcpOptionMpTcpFail::GetSerializedSize (void) const
{
  return 12;
}

} // namespace ns3
