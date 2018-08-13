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

#ifndef TCP_OPTION_MPTCP_H
#define TCP_OPTION_MPTCP_H

#include "tcp-option.h"
#include "tcp-header.h"
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/mptcp-crypto.h"
#include "ns3/sequence-number.h"
#include <vector>

/**
 * \ingroup mptcp
 *
 * Here are a few notations/acronyms used in the multipath tcp group
 * - 3WHS = three way handshake (SYN/SYN+ACK/ACK)
 * - DSN = Data Sequence Number
 * - DSS = Data Sequence Signaling
 */

namespace ns3 {

/**
 * \brief Defines the TCP option of kind 30 (Multipath TCP) as in \RFC{6824}
 *
 * MPTCP signaling messages are all encoded under the same TCP option number 30.
 * MPTCP then uses a subtype
 */
class TcpOptionMpTcpMain : public TcpOption
{
public:
  /**
   * List the different subtypes of MPTCP options
   */
  enum SubType
  {
    MP_CAPABLE,
    MP_JOIN,
    MP_DSS,
    MP_ADD_ADDR,
    MP_REMOVE_ADDR,
    MP_PRIO,
    MP_FAIL,
    MP_FASTCLOSE
  };

  TcpOptionMpTcpMain (void);
  virtual ~TcpOptionMpTcpMain (void);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;

  /**
   * \brief Converts a list of mptcp subtypes
   * \return Human readable string of subtypes
   */
  static std::string
  SubTypeToString (const uint8_t& flags, const std::string& delimiter);

  /**
   * \brief Calls CreateObject with the template parameter with the class matching the given subtype
   * \param Subtype of the MPTCP option to create
   * \return An allocated but unconfigured option
   */
  static Ptr<TcpOption>
  CreateMpTcpOption (const uint8_t& kind);

  virtual uint32_t
  GetSerializedSize (void) const = 0;

  /** Get the subtype number assigned to this MPTCP option */
  virtual uint8_t
  GetKind (void) const;

  virtual void
  Serialize (Buffer::Iterator start) const = 0;

  /**
   * \return the MPTCP subtype of this class
   */
  virtual TcpOptionMpTcpMain::SubType
  GetSubType (void) const = 0;

protected:
  /**
   * \brief Serialize TCP option type & length of the option
   *
   * Let children write the subtype since Buffer iterators
   * can't write less than 1 byte
   * Should be called at the start of every subclass Serialize call
   */
  virtual void
  SerializeRef (Buffer::Iterator& i) const;

  /**
   * \brief Factorizes ation reading/subtype check that each subclass should do.
   * Should be called at the start of every subclass Deserialize
   * \return length of the option
   */
  uint32_t
  DeserializeRef (Buffer::Iterator& i) const;
};

  /**
   * \tparam SUBTYPE should be an integer
   */
template<TcpOptionMpTcpMain::SubType SUBTYPE>
class TcpOptionMpTcp : public TcpOptionMpTcpMain
{
public:
  TcpOptionMpTcp () : TcpOptionMpTcpMain ()
  {
  }

  virtual ~TcpOptionMpTcp (void)
  {
  }

  /**
   * \return MPTCP option type
   */
  virtual TcpOptionMpTcpMain::SubType
  GetSubType (void) const
  {
    return SUBTYPE;
  }
};

/**
 * \brief The MP_CAPABLE option is carried on the SYN, SYN/ACK, and ACK packets of the first TCP connection
 *  established by a MPTCP connection.
 *
 * This first subflow is sometimes called the *master* subflow. It embeds a key that will be used by later TCP connections (subflows)
 * to authenticate themselves as they want to join the MPTCP connection.
 *
 * Here is how the initial 3WHS must look like:
 *
 * \verbatim
Host A                                  Host B
------                                  ------
MP_CAPABLE            ->
[A's key, flags]
                    <-                MP_CAPABLE
                                      [B's key, flags]
ACK + MP_CAPABLE      ->
[A's key, B's key, flags]

\endverbatim

Here is the format as defined in \RFC{6824}, flags C to H refer to the crypto algorithm.
Only sha1 is defined and supported in the standard (same for ns3).
\verbatim

 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------+---------------+-------+-------+---------------+
|     Kind      |    Length     |Subtype|Version|A|B|C|D|E|F|G|H|
+---------------+---------------+-------+-------+---------------+
|                   Option Sender's Key (64 bits)               |
|                                                               |
|                                                               |
+---------------------------------------------------------------+
|                  Option Receiver's Key (64 bits)              |
|                     (if option Length == 20)                  |
|                                                               |
+---------------------------------------------------------------+
\endverbatim
 */
class TcpOptionMpTcpCapable : public TcpOptionMpTcp<TcpOptionMpTcpMain::MP_CAPABLE>
{
public:
  TcpOptionMpTcpCapable (void);
  virtual ~TcpOptionMpTcpCapable (void);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  bool operator== (const TcpOptionMpTcpCapable&) const;

  /**
   * \brief Only version 0 is standardized so far
   * \return 0
   */
  virtual uint8_t GetVersion (void) const;

  /**
   * \note MPTCP Checksums are not used in ns3
   * \return True if checksum is required
   */
  virtual bool IsChecksumRequired (void) const;

  /**
   * Set sender key. Useful for each step of the 3WHS
   *
   * \param senderKey sender key
   */
  virtual void SetSenderKey (const uint64_t& senderKey);

  /**
   * Set remote key. Useful only for the last ACK of the 3WHS.
   *
   * \param remoteKey remote key that was received in the SYN/ACK
   */
  virtual void SetPeerKey (const uint64_t& remoteKey);

  /**
   * \brief Can tell if the option contain the peer key based on the option length
   * \return True if peer key available
   */
  virtual bool HasReceiverKey (void) const;

  /**
   * \return Sender's key
   */
  virtual uint64_t GetSenderKey (void) const;

  /**
   * \warn
   * \return Sender's key
   */
  virtual uint64_t GetPeerKey (void) const;

  //! Inherited
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

protected:
  uint8_t m_version;    /**< MPTCP version (4 bytes) */
  uint8_t m_flags;      /**< 8 bits bitfield (unused in the standard for now) */
  uint64_t m_senderKey; /**< Sender key */
  uint64_t m_remoteKey; /**< Peer key */
  uint32_t m_length;    /**< Stores the length of the option */

private:
  //! Defined and unimplemented to avoid misuse
  TcpOptionMpTcpCapable (const TcpOptionMpTcpCapable&);
  TcpOptionMpTcpCapable& operator= (const TcpOptionMpTcpCapable&);
};

/**
 * \brief The MP_JOIN option is used to add new subflows to an existing MPTCP connection
 *
 * Once cryptographic keys have been exchanged and the first MPTCP Data Ack (in a DSS option) was received from the server (i.e. 2 RTTs),
 * an MPTCP socket can decide to open/close subflows.
 * The opening of new subflows is similar to the MP_CAPABLE 3WHS but it uses MP_JOIN option instead.
 * The MP_JOIN embeds cryptographic data generated from a nonce and the keys sent through the initial MP_CAPABLE in
 * order to be recognized by the remote host as valid connections (to prevent connection hijacking).
 *
 * An MP_JOIN is exchanged during the 3 modes of the 3WHS: Syn/SynAck/Ack and depending on the mode,
 * the structure of the option is very different as can be seen in the following:
 *
 * MP_JOIN Option for Initial SYN:
 * \verbatim
                     1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------+---------------+-------+-----+-+---------------+
|     Kind      |  Length = 12  |Subtype|     |B|   Address ID  |
+---------------+---------------+-------+-----+-+---------------+
|                   Receiver's Token (32 bits)                  |
+---------------------------------------------------------------+
|                Sender's Random Number (32 bits)               |
+---------------------------------------------------------------+
\endverbatim

MP_JOIN for responding SYN/ACK
\verbatim
                     1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------+---------------+-------+-----+-+---------------+
|     Kind      |  Length = 16  |Subtype|     |B|   Address ID  |
+---------------+---------------+-------+-----+-+---------------+
|                                                               |
|                Sender's Truncated HMAC (64 bits)              |
|                                                               |
+---------------------------------------------------------------+
|                Sender's Random Number (32 bits)               |
+---------------------------------------------------------------+

\endverbatim
MP_JOIN Option for third ACK:
\verbatim
                     1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------+---------------+-------+-----------------------+
|     Kind      |  Length = 24  |Subtype|      (reserved)       |
+---------------+---------------+-------+-----------------------+
|                                                               |
|                                                               |
|                   Sender's HMAC (160 bits)                    |
|                                                               |
|                                                               |
+---------------------------------------------------------------+
\endverbatim

To sum up a subflow 3WHS :
\verbatim
Host A                          Host B
 |   SYN + MP_JOIN(Token-B, R-A)  |
 |------------------------------->|
 |<-------------------------------|
 | SYN/ACK + MP_JOIN(HMAC-B, R-B) |
 |                                |
 |     ACK + MP_JOIN(HMAC-A)      |
 |------------------------------->|
 |<-------------------------------|
 |             ACK                |

HMAC-A = HMAC(Key=(Key-A+Key-B), Msg=(R-A+R-B))
HMAC-B = HMAC(Key=(Key-B+Key-A), Msg=(R-B+R-A))
\endverbatim
or
\verbatim
  Host A                                  Host B
  ------                                  ------
  MP_JOIN               ->
  [B's token, A's nonce,
   A's Address ID, flags]
                        <-                MP_JOIN
                                          [B's HMAC, B's nonce,
                                           B's Address ID, flags]
  ACK + MP_JOIN         ->
  [A's HMAC]

                        <-                ACK
\endverbatim
**/
class TcpOptionMpTcpJoin : public TcpOptionMpTcp<TcpOptionMpTcpMain::MP_JOIN>
{

public:
  /**
   * \enum State
   * \brief The MPTCP standard assigns only one MP_JOIN subtype but depending on its rank
   * during the 3 WHS, it can have very different meanings. In order to prevent misuse by the user,
   * this option is very defensive and won't accept setter or getter calls for a rank that is
   * not its current rank
   * The enum values match the size (in bytes) of the option.
   */
  enum Mode
  {
    Uninitialized = 0,
    Syn    = 12,  /**< peer token + nonce */
    SynAck = 16,  /**< peer truncated hmac + nonce */
    Ack    = 24   /**< Send sender full hmac */
  };

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  TcpOptionMpTcpJoin (void);
  virtual ~TcpOptionMpTcpJoin (void);

  virtual bool operator== (const TcpOptionMpTcpJoin&) const;

  /**
   * \return Mode (internal value) in which the option is configured
   */
  virtual Mode
  GetMode (void) const;

  /**
   * \brief Available in Syn and SynAck modes
   * \return nonce
   */
  virtual uint32_t
  GetNonce (void) const;

  /**
   * \param nonce Random number used to prevent malicious subflow establishement from
   * malicious nodes
   */
  virtual void
  SetNonce (const uint32_t& nonce);

  /**
   * \note Available in SynAck mode only
   */
  virtual void SetTruncatedHmac (const uint64_t& );

  /**
   * \note Available in SynAck mode
   */
  virtual uint64_t GetTruncatedHmac (void) const;

  /**
   * \brief Returns hmac generated by the peer.
   * \warning Available in Ack mode only
   * \return null for now (not implemented)
   */
  virtual const uint8_t* GetHmac (void) const;

  /**
   * \brief Available only in Ack mode. Sets Hmac computed from the nonce, tokens previously exchanged
   * \warning Available in Ack mode only
   * \param hmac
   * \todo not implemented
   */
  virtual void SetHmac (uint8_t hmac[20]);

  /**
   * \brief Set token computed from peer's key
   * Used in the SYN message of the MP_JOIN 3WHS
   */
  virtual void
  SetPeerToken (const uint32_t& token);

  /**
   * \return Peer token (the token being a hash of the key)
   */
  virtual uint32_t
  GetPeerToken (void) const;

  /**
   * \brief Unique id assigned by the remote host to this subflow combination (ip,port).
   * \return Identifier of the subflow
   */
  virtual uint8_t
  GetAddressId (void) const;

  /**
   * \brief When a subflow joins a connection, it should advertise a unique identifier
   * \param addrId unique identifier associated with this very subflow
   */
  virtual void
  SetAddressId (const uint8_t& addrId);

  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

  /**
   * \brief When creating from scratch a, MP_CAPABLE option, to call first. Once a mode has been set,
   * it is not possible to change. Depending on the chosen mode, some functions will trigger a fatal error
   * to prevent ns3 from doing meaningless actions.
   * \param mode Mode in which we want to configure the option: Syn or SynAck or Ack
   */
  virtual void SetMode (Mode s);

protected:
  Mode m_mode;          /**<  3 typs of MP_JOIN. Members will throw an exception if used in wrong mode. */
  uint8_t m_addressId;  /**< Unique identifier assigend to this subflow */
  uint8_t m_flags;      /**< Unused so far. */
  uint32_t m_buffer[5]; /**< Memory buffer to register the data of the different modes */

private:
  //! Defined and unimplemented to avoid misuse
  TcpOptionMpTcpJoin (const TcpOptionMpTcpJoin&);
  TcpOptionMpTcpJoin& operator= (const TcpOptionMpTcpJoin&);
};

/**
 * \brief DSS option (Data Sequence Signaling)
 *
 * This option can transport 3 different optional semantic information.
 * First it is important to understand that MPTCP uses an additional sequence number space
 * on top of TCP sequence numbers in order to reorder the bytes at the receiver.
 * The DSS option is in charge of carrying MPTCP sequence number related information between
 * the hosts.
 * MPTCP sequence numbers are mapped to TCP sequence numbers through this option and
 * are acked through this option as well. It it also this option that signals the end of
 * the MPTCP connection (similar to the TCP FIN flag).
 *
 * \warn Note that members asserts that flag is enabled before returning any value.
 *
 * \note MPTCP sequence numbers are 8 bytes long but when conveyed through a TCP option,
 *       they can be trimmed to 4 bytes in order to save TCP option space.
 *
 * \warn ns3 should allow to carry both 4 and 8 bytes but the initial implementation relied on 4 bytes
 *       so you may find bugs
 *
 * \warn checksum are not supported since they make little sense in ns3 case
 *
 * Data Sequence Signal (DSS) Option
\verbatim
                      1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +---------------+---------------+-------+----------------------+
 |     Kind      |    Length     |Subtype| (reserved) |F|m|M|a|A|
 +---------------+---------------+-------+----------------------+
 |           Data ACK (4 or 8 octets, depending on flags)       |
 +--------------------------------------------------------------+
 |   Data sequence number (4 or 8 octets, depending on flags)   |
 +--------------------------------------------------------------+
 |              Subflow Sequence Number (4 octets)              |
 +-------------------------------+------------------------------+
 |  Data-Level Length (2 octets) |      Checksum (2 octets)     |
 +-------------------------------+------------------------------+

\endverbatim
*/
class TcpOptionMpTcpDSS : public TcpOptionMpTcp<TcpOptionMpTcpMain::MP_DSS>
{

public:
  /**
   * \brief Each value represents the offset of a flag in LSB order
   * \see TcpOptionMpTcpDSS
   */
  enum FLAG
  {
    DataAckPresent  = 1,       //!< matches the "A" in previous packet format
    DataAckOf8Bytes = 2,       //!< a
    DSNMappingPresent = 4,     //!< M bit
    DSNOfEightBytes   = 8,     //!< m bit
    DataFin           = 16,    //!< F . set to indicate end of communication
    CheckSumPresent   = 32     //!< Not computed for now

  };

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  TcpOptionMpTcpDSS (void);
  virtual ~TcpOptionMpTcpDSS (void);

  /**
   * \brief Upon detecting an error, an MPTCP connection can fallback to legacy TCP.
   * \return False (not implemented)
   */
  virtual bool IsInfiniteMapping () const;

  /**
   * \brief when DataFin is set, the data level length is increased by one.
   * \return True if the mapping present is just because of the datafin
   */
  virtual bool DataFinMappingOnly () const;

  virtual void TruncateDSS(bool truncate);

  /**
   * \brief This returns a copy
   * \warning Asserts if flags
   * \
   */
  virtual void GetMapping (uint64_t& dsn, uint32_t& ssn, uint16_t& length) const;

  /**
   * \brief
   * \param trunc_to_32bits Set to true to send a 32bit DSN
   * \warn Mapping can be set only once, otherwise it will crash ns3
   */
  virtual void SetMapping (uint64_t headDsn, uint32_t headSsn, uint16_t length, bool enable_dfin);

  /**
   * \brief A DSS length depends on what content it embeds. This is defined by the flags.
   * \return All flags
   */
  virtual uint8_t GetFlags (void) const;

  virtual bool operator== (const TcpOptionMpTcpDSS&) const;

  /**
   * \brief Set seq nb of acked data at MPTP level
   * \param dack Sequence number of the dataack
   * \param send_as_32bits Decides if the DACK should be sent as a 32 bits number
   */
  virtual void SetDataAck (const uint64_t& dack, const bool& send_as_32bits = true);

  /**
   * \brief Get data ack value
   * \warning  check the flags to know if the returned value is a 32 or 64 bits DSN
   */
  virtual uint64_t GetDataAck (void) const;

  /**
   * \brief Unimplemented
   */
  virtual void SetChecksum (const uint16_t&);

  /**
   * \brief Unimplemented
   */
  virtual uint16_t GetChecksum (void) const;

  /**
  * \return If DFIN is set, returns its associated DSN
  *
  * \warning check the flags to know if it returns a 32 or 64 bits DSN
  */
  virtual uint64_t GetDataFinDSN () const;

  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator ) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

  /**
  * \brief the DSS option size can change a lot
  * The DSS size depends if it embeds a DataAck, a mapping, in 32 bits
  *  or in 64 bits. This can compute the size depending on the flags
  * \param flags flags of the DSS option
  */
  static uint32_t GetSizeFromFlags (uint16_t flags);

protected:

  bool m_hasChecksum;   //!< true if checksums enabled
  uint16_t m_checksum;  //!< valeu of the checksum
  uint8_t m_flags;  //!< bitfield

  // In fact for now we use only 32 LSB
  uint64_t m_dataAck;           /**< Can be On 32 bits dependings on the flags **/
  uint64_t m_dsn;               /**< Data Sequence Number (Can be On 32 bits dependings on the flags) */
  uint32_t m_ssn;               /**< Subflow Sequence Number, always 32bits */
  uint16_t m_dataLevelLength;   /**< Length of the mapping and/or +1 if DFIN */

private:
  //! Defined and unimplemented to avoid misuse
  TcpOptionMpTcpDSS (const TcpOptionMpTcpDSS&);
  TcpOptionMpTcpDSS& operator= (const TcpOptionMpTcpDSS&);
};

/**
 * \brief Used to advertise new subflow possibilities
 *
 * This option signals to the peer an IP or a couple (IP, port) to which the peer could
 * open a new subflow.
 * Every combination (IP, port) must have a unique ID ("Address ID"), whose allocation is not specified
 * in the standard.
 *
 * \see TcpOptionMpTcpRemoveAddress
 *
 * \note Only supports IPv4 version for now
 *
 * \note Though the port is optional in the RFC, ns3 implementation always include it, even if
 * it's 0 for the sake of simplicity.
 *
 * Add Address (ADD_ADDR) option:
\verbatim
                     1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------+---------------+-------+-------+---------------+
|     Kind      |     Length    |Subtype| IPVer |  Address ID   |
+---------------+---------------+-------+-------+---------------+
|          Address (IPv4 - 4 octets / IPv6 - 16 octets)         |
+-------------------------------+-------------------------------+
|   Port (2 octets, optional)   |
+-------------------------------+
\endverbatim
 */
class TcpOptionMpTcpAddAddress : public TcpOptionMpTcp<TcpOptionMpTcpMain::MP_ADD_ADDR>
{

public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  TcpOptionMpTcpAddAddress (void);
  virtual ~TcpOptionMpTcpAddAddress (void);

  /**
   * \brief we always send the port, even if it's 0 ?
   * \brief Expects InetXSocketAddress
   * "port is specified, MPTCP SHOULD attempt to connect to the specified
   * address on the same port as is already in use by the subflow on which
   * the MP_ADD_ADDR signal was sent"
   */
  virtual void SetAddress (const Address& address, uint8_t addrId);

  virtual bool operator== (const TcpOptionMpTcpAddAddress&) const;

  /**
   * \note Only IPv4 is supported so far
   * \return IP version (i.e., 4 or 6)
   */
  virtual uint8_t GetAddressVersion (void) const;

  /**
   * \brief Return advertised InetSocketAddress.
   * If port unset, ns3 sets it to 0
   * \return
   */
  virtual InetSocketAddress GetAddress (void) const;

  /**
   * \see GetAddress
   */
  virtual Inet6SocketAddress GetAddress6 (void) const;

  /**
   * \return Address id assigned to the combination
   */
  virtual uint8_t GetAddressId (void) const;

  //! Inherited
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

protected:
  uint8_t m_addressVersion; /**< IPversion (4 or 6) */
  uint8_t m_addrId;
  uint16_t m_port; /**< Optional value */ // changed from uint8_t to uint16_t
  Ipv4Address m_address;  /**< Advertised IPv4 address */
  Ipv6Address m_address6; //!< unused

private:
  //! Defined and unimplemented to avoid misuse
  TcpOptionMpTcpAddAddress (const TcpOptionMpTcpAddAddress&);
  TcpOptionMpTcpAddAddress& operator= (const TcpOptionMpTcpAddAddress&);
};

/**
 * \see TcpOptionMpTcpAddAddress
 *
 * \brief Allow to remove joinable addresses
 *
 * In case ADD_ADDR was used to advertise an IP that is not valid anymore (e.g., because of roaming),
 * this option allows to remove it from peer memory (and can remove a batch of them).
 * The subflow should not exist yet (else you must use TCP FIN to close it).
 *
 * REMOVE_ADDR option:
 * \verbatim
                     1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------+---------------+-------+-------+---------------+
|     Kind      |  Length = 3+n |Subtype|(resvd)|   Address ID  | ...
+---------------+---------------+-------+-------+---------------+
            (followed by n-1 Address IDs, if required)

\endverbatim
 */
class TcpOptionMpTcpRemoveAddress : public TcpOptionMpTcp<TcpOptionMpTcpMain::MP_REMOVE_ADDR>
{

public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  TcpOptionMpTcpRemoveAddress (void);
  virtual ~TcpOptionMpTcpRemoveAddress (void);

  /**
   * As we do not know in advance the number of records, we pass a vector
   * \param Returns the addresses into a vector. Empty it before use.
   */
  void GetAddresses (std::vector<uint8_t>& addresses);

  /**
   * Append an association id to remove from peer memory
   * \param addressId The association (IP,port) id
   */
  void AddAddressId (uint8_t addressId);

  virtual bool operator== (const TcpOptionMpTcpRemoveAddress&) const;

  //! Inherited
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

protected:
  std::vector<uint8_t> m_addressesId; /**< List of removed association ids */

private:
  //! Defined and unimplemented to avoid misuse
  TcpOptionMpTcpRemoveAddress (const TcpOptionMpTcpRemoveAddress&);
  TcpOptionMpTcpRemoveAddress& operator= (const TcpOptionMpTcpRemoveAddress&);
};

/**
 * \brief Allow to (un)block emission from peer on this subflow
 *
 * MPTCP allows a host to advertise their preference about the links they would prefer to use.
 * It is advertised as a boolean (B) that means "Please don't send data on this connection, except if
 * there is a problem".
 * The peer is free to ignore this preference.
 * This option is unidirectional, i.e, an emitter may ask not to receive data
 * on a subflow while transmitting on it.
 *
 * \warn This option is not taken into account by ns3 yet.
 * MP_PRIO option:
\verbatim
                     1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------+---------------+-------+-----+-+--------------+
|     Kind      |     Length    |Subtype|     |B| AddrID (opt) |
+---------------+---------------+-------+-----+-+--------------+

\endverbatim
 *
 */
class TcpOptionMpTcpChangePriority : public TcpOptionMpTcp<TcpOptionMpTcpMain::MP_PRIO>
{

public:
  /**
   * Only one flag standardized
   */
  enum Flags
  {
    Backup = 0 /**< Set this flag if you prefer not to receive data on path addressId*/
  };

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  TcpOptionMpTcpChangePriority (void);
  virtual ~TcpOptionMpTcpChangePriority (void);

  /**
   * \brief All flags should be set at once.
   *
   * You can append flags by using SetFlags( option.GetFlags() | NewFlag)
   *
   * \example SetFlags(TcpOptionMpTcpChangePriority::Backup);
   */
  virtual void SetFlags (const uint8_t& flags);

  /**
   * \brief Optional. If you don't set addrId, the receiver considers the subflow on which the
   * option was received
   */
  virtual void SetAddressId (const uint8_t& addrId);

  /**
   * \return True if an address id was set in the packet.
   * \note Result depends on length of the option
   */
  virtual bool EmbeddedAddressId (void) const;

  /**
   * \return association id
   * \warning will assert if addressId was not set. check it with \see EmbeddedAddressId
   */
  virtual uint8_t GetAddressId () const;

  /**
   * \return flags, i.e., the 4 LSB
   */
  virtual uint8_t GetFlags (void) const;

  virtual bool operator== (const TcpOptionMpTcpChangePriority& ) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * \brief Length may be 3 or 4 (if addrId present)
  */
  virtual uint32_t GetSerializedSize (void) const;

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  TcpOptionMpTcpChangePriority (const TcpOptionMpTcpChangePriority&);
  TcpOptionMpTcpChangePriority& operator= (const TcpOptionMpTcpChangePriority&);

  uint8_t m_length;   /**< Length of this option */
  uint8_t m_addrId;   //!< May be unset
  uint8_t m_flags;    //!< On 4 bits
};

/**
 * \brief MP_FASTCLOSE is the equivalent of TCP RST at the MPTCP level
 *
 * For example, if the operating system is running out of resources, MPTCP could send an
 * MP_FASTCLOSE.
 * This should not be very useful in ns3 scenarii.
 *
 * MP_FASTCLOSE option:
\verbatim
                    1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------+---------------+-------+-----------------------+
|     Kind      |    Length     |Subtype|      (reserved)       |
+---------------+---------------+-------+-----------------------+
|                      Option Receiver's Key                    |
|                            (64 bits)                          |
|                                                               |
+---------------------------------------------------------------+

\endverbatim
**/
class TcpOptionMpTcpFastClose : public TcpOptionMpTcp<TcpOptionMpTcpMain::MP_PRIO>
{

public:
  TcpOptionMpTcpFastClose (void);
  virtual ~TcpOptionMpTcpFastClose (void);
  virtual bool operator== (const TcpOptionMpTcpFastClose&) const;

  /**
  * \brief Set peer key to prevent spoofing a MP_FastClose.
  * \param remoteKey key exchanged during the 3WHS.
  */
  virtual void SetPeerKey (const uint64_t& remoteKey);

  /**
  * \return peer's key
  */
  virtual uint64_t GetPeerKey (void) const;

  //! Inherited
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

private:
  uint64_t m_peerKey; /**< Key of the remote host so that it accepts the RST */

  //! Defined and unimplemented to avoid misuse
  TcpOptionMpTcpFastClose (const TcpOptionMpTcpFastClose&);
  TcpOptionMpTcpFastClose& operator= (const TcpOptionMpTcpFastClose&);
};

/**
 * \brief Option used when a problem is noticed at any point during a connection (payload changed etc).
 * A subflow would sends RST and MP_FAIL option
 *
 *
 * Note that the MP_FAIL option requires the use of the full 64-bit sequence number, even if 32-bit sequence numbers are
 * normally in use in the DSS signals on the path.
 *
 * Such a situation is likely to never happen in ns3 and thus this option is implemented as a reference only.
 *
 MP_FAIL option:
\verbatim
                     1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------+---------------+-------+----------------------+
|     Kind      |   Length=12   |Subtype|      (reserved)      |
+---------------+---------------+-------+----------------------+
|                                                              |
|                 Data Sequence Number (8 octets)              |
|                                                              |
+--------------------------------------------------------------+

\endverbatim
*/
class TcpOptionMpTcpFail : public TcpOptionMpTcp<TcpOptionMpTcpMain::MP_FAIL>
{

public:
  TcpOptionMpTcpFail (void);
  virtual ~TcpOptionMpTcpFail (void);

  virtual bool operator== (const TcpOptionMpTcpFail& ) const;

  /**
   * \brief Set Data Sequence Number (DSN) until which the communication was fine
   * \param dsn Last in-order ACK-1 received
   */
  virtual void SetDSN (const uint64_t& dsn);

  /**
   * \return DSN for which error was detected
   */
  virtual uint64_t GetDSN (void) const;

  //! Inherited
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

private:
  //! Defined and unimplemented to avoid misuse
  TcpOptionMpTcpFail (const TcpOptionMpTcpFail&);
  TcpOptionMpTcpFail& operator= (const TcpOptionMpTcpFail&);

  uint64_t m_dsn; /**< Last acked dsn */
};

/**
 * \brief Like GetMpTcpOption but if does not find the option then it creates one
 *       and append it to the the header
 *
 * \see GetMpTcpOption
 * \return false if it had to create the option
**/
template<class T>
bool
GetOrCreateMpTcpOption (TcpHeader& header, Ptr<T>& ret)
{
  if (!GetTcpOption (header,ret))
    {
      ret = Create<T> ();
      header.AppendOption (ret);
      return false;
    }
  return true;
}

} // namespace ns3

#endif /* TCP_OPTION_MPTCP */
