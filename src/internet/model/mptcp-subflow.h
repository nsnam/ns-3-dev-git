/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Sussex
 * Copyright (c) 2015 Université Pierre et Marie Curie (UPMC)
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
 *          Morteza Kheirkhah <m.kheirkhah@sussex.ac.uk>
 */
#ifndef MPTCP_SUBFLOW_H
#define MPTCP_SUBFLOW_H

#include "ns3/trace-source-accessor.h"
#include "ns3/sequence-number.h"
#include "ns3/rtt-estimator.h"
#include "ns3/event-id.h"
#include "ns3/packet.h"
#include "ns3/tcp-socket.h"
#include "ns3/ipv4-end-point.h"
#include "ns3/ipv4-address.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-header.h"
#include "ns3/mptcp-mapping.h"

using namespace std;

namespace ns3 {

class MpTcpSocketBase;
class TcpOptionMpTcpDSS;
class TcpOptionMpTcpMain;
class TcpSocketBase;
class TcpOptionMpTcpAddAddress;

/**
 * \class MpTcpSubflow
 */
class MpTcpSubflow : public TcpSocketBase
{
public:

  static TypeId
  GetTypeId(void);

  virtual TypeId GetInstanceTypeId(void) const;
  virtual uint32_t GetTxAvailable (void) const;

  /*
   * This function to pass peer token to tcpsocketbase
   */
  uint32_t PeerToken();

  /**
   * \bfief the metasocket is the socket the application is talking to.
   * \brief Every subflow is linked to that socket.
   * \param The metasocket it is linked to
   */
  MpTcpSubflow();

  MpTcpSubflow(const TcpSocketBase& sock);
  MpTcpSubflow(const MpTcpSubflow&);
  virtual ~MpTcpSubflow();
  virtual uint32_t UnAckDataCount (void) const;
  virtual uint32_t BytesInFlight (void) const;
  virtual uint32_t AvailableWindow (void) const;
  virtual uint32_t Window (void) const;          // Return the max possible number of unacked bytes

  /**
   * \brief will update the meta rwnd. Called by subflows whose
   * \return true
   */
  virtual bool UpdateWindowSize (const TcpHeader& header);

  /**
   * \return Value advertised by the meta socket
   */
  virtual uint16_t AdvertisedWindowSize (bool scale = true) const;

  /**
   * \param metaSocket
   */
  virtual void
  SetMeta(Ptr<MpTcpSocketBase> metaSocket);

  /**
   * \warning for prototyping purposes, we let the user free to advertise an IP that doesn't belong to the node
   * (in reference to MPTCP connection agility).
   * \note Maybe we should change this behavior
   */
  virtual void
  AdvertiseAddress(Ipv4Address , uint16_t port);

  /**
   * \brief Send a REM_ADDR for the specific address.
   * \see AdvertiseAddress
   * \return false if no id associated with the address which likely means it was never advertised in the first place
   */
  virtual bool
  StopAdvertisingAddress(Ipv4Address);

  /**
   * \brief This is important. This should first request data from the meta
   */
  virtual void
  NotifySend (uint32_t spaceAvailable);

  /**
   * \brief for debug
   */
  void
  DumpInfo() const;

  /**
   * \brief
   * \note A Master socket is the first to initiate the connection, thus it will use the option MP_CAPABLE
   *  during the 3WHS while any additionnal subflow must resort to the MP_JOIN option
   * \return True if this subflow is the first (should be unique) subflow attempting to connect
   */
  virtual bool
  IsMaster() const;

  /**
   * \return True if this subflow shall be used only when all the regular ones failed
   */
  virtual bool BackupSubflow() const;
  virtual uint32_t SendPendingData (bool withAck = false);

  /**
   * \brief Disabled for now.
   * SendMapping should be used instead.
   */
  virtual int Send (Ptr<Packet> p, uint32_t flags);  // Call by app to send data to network

  /**
   * \param dsn will set the dsn of the beginning of the data
   * \param only_full_mappings Set to true if you want to extract only packets that match a whole mapping
   * \param dsn returns the head DSN of the returned packet
   *
   * \return this can return an EmptyPacket if on close
   */
  virtual Ptr<Packet>
  ExtractAtMostOneMapping(uint32_t maxSize, bool only_full_mappings, SequenceNumber64& dsn);

  /**
   * \brief used in previous implementation to notify meta of subflow closing.
   * \brief See it that's worth keeping
   */
  virtual void ClosingOnEmpty(TcpHeader& header);
  virtual void DeallocateEndPoint (void);
  virtual void NewAck (SequenceNumber32 const& ack, bool resetRTO);
  virtual void TimeWait (void);
  virtual void DoRetransmit (void);
  virtual int Listen (void);
  virtual void ProcessListen (Ptr<Packet> packet, const TcpHeader& tcpHeader,
                      const Address& fromAddress, const Address& toAddress);
  /**
   * \brief Will send MP_JOIN or MP_CAPABLE depending on if it is master or not
   * \brief Updates the meta endpoint
   * \see TcpSocketBase::CompleteFork
   */
  virtual void CompleteFork (Ptr<Packet> p, const TcpHeader& tcpHeader,
                             const Address& fromAddress, const Address& toAddress);
  virtual void ProcessSynRcvd (Ptr<Packet> packet, const TcpHeader& tcpHeader,
                       const Address& fromAddress, const Address& toAddress);
  virtual void ProcessSynSent(Ptr<Packet> packet, const TcpHeader& tcpHeader);
  virtual void ProcessWait(Ptr<Packet> packet, const TcpHeader& tcpHeader);
  virtual void Retransmit(void);

  /**
   * \bfief Parse DSS essentially
   */
  virtual int ProcessOptionMpTcpDSSEstablished (const Ptr<const TcpOptionMpTcpDSS> option);
  virtual int ProcessOptionMpTcpJoin (const Ptr<const TcpOptionMpTcpMain> option);
  virtual int ProcessOptionMpTcpCapable (const Ptr<const TcpOptionMpTcpMain> option);

  /*
   * \brief Process TcpOptionTcpAddAddress 
   * \briefto add advertised address and create new subflows
   * \param option MpTcpOptions
   */
  virtual int ProcessOptionMpTcpAddAddress (const Ptr<const TcpOptionMpTcpAddAddress> option);

  /**
   * \brief Helper functions: Connection set up
   * \brief Common part of the two Bind(), i.e. set callback and remembering local addr:port
   * \returns 0 on success, -1 on failure
   */
  virtual int SetupCallback (void);

  /**
   * \brief Called by the L3 protocol when it received a packet to pass on to TCP.
   *
   * \param packet the incoming packet
   * \param header the packet's IPv4 header
   * \param port the remote port
   * \param incomingInterface the incoming interface
   */
  virtual void ForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port, Ptr<Ipv4Interface> incomingInterface);

protected:
  friend class MpTcpSocketBase;
  /**
   * \brief This is a public function in TcpSocketBase but it shouldn't be public here !
   */
  virtual int Close(void);
  virtual void CloseAndNotify(void);
  virtual void GetMappedButMissingData(
                std::set< MpTcpMapping >& missing
                );

  /**
   * \brief Depending on if this subflow is master or not, we want to
   * \brief trigger
   * \brief Callbacks being private members
   * \brief Overrides parent in order to warn meta
   */
  virtual void ConnectionSucceeded(void);
  int DoConnect();

  /**
   * \brief In fact, instead of relying on IsMaster etc...
   * \brief this should depend on meta's state , if it is wait or not
   * \brief and thus it should be pushed in meta (would also remove the need for crypto accessors)
   * \param header
   */
  virtual void AddOptionMpTcp3WHS(TcpHeader& hdr) const;

  virtual void ProcessClosing(Ptr<Packet> packet, const TcpHeader& tcpHeader);
  virtual int ProcessOptionMpTcp (const Ptr<const TcpOption> option);
  Ptr<MpTcpSocketBase> m_metaSocket;    //!< Meta
  virtual void SendPacket(TcpHeader header, Ptr<Packet> p);

public:
  /**
   * \return pointer to MpTcpSocketBase
   */
  virtual Ptr<MpTcpSocketBase> GetMeta() const;

  /**
   * \brief Not implemented
   * \return false
   */
  virtual bool IsInfiniteMappingEnabled() const;

  virtual void SendEmptyPacket (uint8_t flags); // Send a empty packet that carries a flag, e.g. ACK

protected:

  /////////////////////////////////////////////
  //// DSS Mapping handling
  /////////////////////////////////////////////

  /**
   * \brief Mapping is said "loose" because it is not tied to an SSN yet, this is the job
   * \brief of this function: it will look for the FirstUnmappedSSN() and map the DSN to it.
   *
   * Thus you should call it with increased dsn.
   * \param dsnHead
   */
  bool AddLooseMapping(SequenceNumber64 dsnHead, uint16_t length);

  /**
   * \brief If no mappings set yet, then it returns the tail ssn of the Tx buffer.
   * \return Otherwise returns the last registered mapping TailSequence
   */
  SequenceNumber32 FirstUnmappedSSN();

  /**
   * \brief Creates a DSS option if does not exist and configures it to have a dataack
   */
  virtual void AppendDSSAck();

  /**
   * \brief Corresponds to mptcp_write_dss_mapping and mptcp_write_dss_ack
   */
  virtual void AddMpTcpOptionDSS(TcpHeader& header);

  virtual void AppendDSSFin();
  virtual void AppendDSSMapping(const MpTcpMapping& mapping);
  virtual void ReceivedAck(Ptr<Packet>, const TcpHeader&); // Received an ACK packet
  virtual void ReceivedData (Ptr<Packet> packet, const TcpHeader& tcpHeader);
  virtual uint32_t SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck);
  virtual void ReTxTimeout();
  /**
   * \brief Overrides the TcpSocketBase that just handles the MP_CAPABLE option.
   * \param header
   */
  virtual void AddMpTcpOptions (TcpHeader& header);

  /**
   * \brief should be able to use parent's one little by little
   */
  virtual void CancelAllTimers(void); // Cancel all timer when endpoint is deleted

  // Use Ptr here so that we don't have to unallocate memory manually
  // MappingList
  MpTcpMappingContainer m_TxMappings;  //!< List of mappings to send
  MpTcpMappingContainer m_RxMappings;  //!< List of mappings to receive

private:
  // Delayed values to
  uint8_t m_dssFlags;           //!< used to know if AddMpTcpOptions should send a flag
  bool m_masterSocket;  //!< True if this is the first subflow established (with MP_CAPABLE)
  MpTcpMapping m_dssMapping;    //!< Pending ds configuration to be sent in next packet
  bool m_backupSubflow; //!< Priority
  uint32_t m_localNonce;  //!< Store local host token, generated during the 3-way handshake
  int m_prefixCounter;  //!< Temporary variable to help with prefix generation . To remove later

};

}
#endif /* MP_TCP_SUBFLOW */
