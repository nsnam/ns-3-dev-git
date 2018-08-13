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
#ifndef MPTCP_SOCKET_BASE_H
#define MPTCP_SOCKET_BASE_H

#include "ns3/callback.h"
#include "ns3/mptcp-mapping.h"
#include "ns3/tcp-socket.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/inet-socket-address.h"
#include "ns3/mptcp-scheduler-round-robin.h"
#include "ns3/mptcp-fullmesh.h"
#include "ns3/mptcp-ndiffports.h"

namespace ns3 {

class Ipv4EndPoint;
class Ipv6EndPoint;
class Node;
class Packet;
class TcpL4Protocol;
class MpTcpSubflow;
class TcpOptionMpTcpDSS;
class TcpOptionMpTcpJoin;
class OutputStreamWrapper;
class TcpSocketBase;

/**
 * \ingroup socket
 * \class MpTcpSocketBase
 *
 * \brief A base class for implementation of a stream socket using MPTCP.
 * This is the MPTCP meta socket the application talks with
 * this socket. New subflows, as well as the first one (the master
 * socket) are linked to this meta socket.
 * Every data transfer happens on a subflow.
 * Following the linux kernel from UCL (http://multipath-tcp.org) convention,
 * the first established subflow is called the "master" subflow.
 *
 * This inherits TcpSocketBase so that it can be  used as any other TCP variant:
 * this is the backward compability feature that is required in RFC.
 * Also doing so allows to run TCP tests with MPTCP via for instance the command
 * Config::SetDefault ("ns3::TcpL4Protocol::SocketType", "ns3::MpTcpOlia");
 *
 * But to make sure some inherited functions are not improperly used, we need to redefine them so that they
 * launch an assert. You can notice those via the comments "//! Disabled"
 *
 * As such many inherited (protected) functions are overriden & left empty.
 *
 * As in linux, the meta should return the m_endPoint information of the master,
 * even if that subflow got closed during the MpTcpConnection.
 *
 * ConnectionSucceeded may be called twice; once when it goes to established
 * and the second time when it sees
 * Simulator::ScheduleNow(&MpTcpSocketBase::ConnectionSucceeded, this);
 */

class MpTcpSocketBase  : public TcpSocketBase
{
public:

  typedef std::vector< Ptr<MpTcpSubflow> > SubflowList;

  static TypeId GetTypeId(void);

  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * \brief Path managers to initiate subslows
   * \param Default default path manager
   * \param FullMesh fullmesh path manager
   * \param nDiffPorts ndiffports path manager
   */
  enum PathManagerMode
   {
    Default,
    FullMesh,
    nDiffPorts
   };

  MpTcpSocketBase();

  MpTcpSocketBase(const TcpSocketBase& sock);
  MpTcpSocketBase(const MpTcpSocketBase&);
  virtual ~MpTcpSocketBase();

  /**
   * \brief Should be called only by subflows when they update their receiver window
   */
  virtual bool UpdateWindowSize(const TcpHeader& header);

protected:
   ////////////////////////////////////////////
   /// List of overriden callbacks
   ////////////////////////////////////////////
  /**
   * This is callback called by subflow NotifyNewConnectionCreated. If
   * the calling subflow is the master, then the call is forwarded through meta's
   * NotifyNewConnectionCreated, else it is forward to the JoinCreatedCallback
   *
   * \see Socket::NotifyNewConnectionCreated
   */
  virtual void OnSubflowCreated (Ptr<Socket> socket, const Address &from);
  virtual void OnSubflowConnectionFailure (Ptr<Socket> socket);
  virtual void OnSubflowConnectionSuccess (Ptr<Socket> socket);

public:

  virtual void SetPathManager(PathManagerMode); 
  /**
   * Create a subflow for ndiffports path manager
   * Initiate a single new subflow between given IP addresses
   * \param randomSourcePort random source port
   * \param randomDestinationPort random destination port
   */
  bool CreateSingleSubflow(uint16_t randomSourcePort, uint16_t randomDestinationPort);

  /**
   * \brief Create a subflow for fullmesh path manager
   * \brief Initiate s subflow between the received address/port
   * \param destinationIp address of the destination
   * \param destinationPort destination port
   * \param id
   */
   bool CreateSubflowsForMesh();

  /**
   * \brief these callbacks will be passed on to
   * \see Socket::Set
   */
  virtual void
  SetSubflowAcceptCallback(Callback<bool, Ptr<MpTcpSocketBase>, const Address &, const Address & > connectionRequest,
                           Callback<void, Ptr<MpTcpSubflow> > connectionCreated
                           );

  virtual void
  SetSubflowConnectCallback(Callback<void, Ptr<MpTcpSubflow> > connectionSucceeded,
                           Callback<void, Ptr<MpTcpSubflow> > connectionFailure
                            );

  /**
   * Triggers callback registered by SetSubflowAcceptCallback
   */
  void NotifySubflowCreated(Ptr<MpTcpSubflow> sf);

  /**
   * Triggers callback registered by SetSubflowConnectCallback
   */
  void NotifySubflowConnected(Ptr<MpTcpSubflow> sf);

  /**
   * Called when a subflow TCP state is updated.
   * It detects such events by tracing its subflow m_state.
   *
   */
  virtual void  OnSubflowNewCwnd(std::string context, uint32_t oldCwnd, uint32_t newCwnd);

  /**
   * Initiates a new subflow with MP_JOIN
   *
   * Wrapper that just creates a subflow, bind it to a specific address
   * and then establishes the connection
   */
  virtual int ConnectNewSubflow(const Address &local, const Address &remote);
  virtual void DoRetransmit (void);

  /**
   * Called when a subflow TCP state is updated.
   * It detects such events by tracing its subflow m_state.
   *
   * \param context
   * \param sf Subflow that changed state
   * \param oldState previous TCP state of the subflow
   * \param newState new TCP state of the subflow
   */
  virtual void
  OnSubflowNewState(
    std::string context,
    Ptr<MpTcpSubflow> sf,
    TcpStates_t  oldState,
    TcpStates_t newState
    );

  // Window Management
  virtual uint32_t BytesInFlight (void) const;  // Return total bytes in flight of a subflow

  /* 
   * This function is added to access m_nextTxSequence in 
   * MpTcpSchedulerRoundRobin class
   * directly accessing value of m_nextTxSequence in MpTcpSechulerRoundRobin
   * it gives zero value
   */
  SequenceNumber32 Getvalue_nextTxSeq();
  void DumpRxBuffers(Ptr<MpTcpSubflow> sf) const;

  /* Sum congestio nwindows across subflows to compute global cwin
   * It does not take flows that are closing yet so that may be a weakness depending on the scenario
   * to update
   */
  virtual uint32_t ComputeTotalCWND();
  virtual uint16_t AdvertisedWindowSize (bool scale = true) const;
  virtual uint32_t AvailableWindow (void) const;
  virtual void CloseAndNotify(void);

  /* Limit the size of in-flight data by cwnd and receiver's rxwin */

  virtual uint32_t Window (void) const;
  virtual void PersistTimeout (void);

  /* \brief equivalent to PeerClose
   * \param finalDsn
   * OnDataFin
   */
  virtual void PeerClose( SequenceNumber32 fin_seq, Ptr<MpTcpSubflow> sf); 

  /* equivalent to TCP Rst */
  virtual void SendFastClose(Ptr<MpTcpSubflow> sf);

  /**
   * \return return true if connected
   */
  bool IsConnected() const;

  // Public interface for MPTCP
  virtual int Bind(void);
  virtual int Bind(const Address &address);

  /**
   * \brief Same as MpTcpSocketBase::Close
   *
   * The default behavior is to do nothing until all the data is transmitted.
   * Only then are
   * RFC 6824:
   * - When an application calls close() on a socket, this indicates that it
   * has no more data to send; for regular TCP, this would result in a FIN
   * on the connection.  For MPTCP, an equivalent mechanism is needed, and
   * this is referred to as the DATA_FIN.
   *
   * - A DATA_FIN has the semantics and behavior as a regular TCP FIN, but
   * at the connection level.  Notably, it is only DATA_ACKed once all
   * data has been successfully received at the connection level
   */
  // socket function member
  virtual int Close(void);

  /** \brief called by subflow when it sees a DSS with the DATAFIN flag
   * \Params are ignored
   * \param packet Ignored
   */
  virtual void PeerClose (Ptr<Packet> p, const TcpHeader& tcpHeader); // Received a FIN from peer, notify rx buffer
  virtual void DoPeerClose (void); // FIN is in sequence, notify app and respond with a FIN
  virtual int DoClose(void);
  virtual uint32_t GetTxAvailable() const;
  virtual uint32_t GetRxAvailable(void) const;
  virtual void ForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port, Ptr<Ipv4Interface> incomingInterface);

  /**
   * \return Number of connected subflows (that is that ran the 3whs)
   */
  SubflowList::size_type GetNActiveSubflows() const;

  /**
   *
   * \return an established subflow
   */
  virtual Ptr<MpTcpSubflow> GetSubflow(uint8_t) const;
  virtual void ClosingOnEmpty(TcpHeader& header);

  /**
   * The connection is considered fully established
   * when it can create new subflows, i.e., when it received
   * a first dss ack
   */
  virtual bool FullyEstablished() const;

  /**
   * This retriggers Connection success callback
   * You have to check in the callback if it fully estalbished or not
   */
  virtual void BecomeFullyEstablished();

  /* \brief Superseed into TcpSocketBase
   * Here it sends MP_FIN
   */
  virtual void SendFin();

  /**
   * \brief Initiate
   * \param sf subflow to setup
   */
  virtual void AddSubflow(Ptr<MpTcpSubflow> sf);

  virtual void Destroy (void);

  /**
   * \return 0 In case of success
   */
  uint64_t GetPeerKey() const;

  /**
   * \brief Generated during the initial 3 WHS
   */
  uint64_t GetLocalKey() const;

  /**
   * \return Hash of the local key
   */
  uint32_t GetLocalToken() const;

  /**
   * \return Hash of the peer key
   */
  uint32_t GetPeerToken() const;

  /**
  * \brief For now it looks there is no way to know that an ip interface went up so we will assume until
  * \brief further notice that IPs of the client don't change.
  * \param -1st callback called on receiving an ADD_ADDR
  * -param 2nd callback called on receiving REM_ADDR
  */
  void SetNewAddrCallback(Callback<bool, Ptr<Socket>, Address, uint8_t> remoteAddAddrCb,
                          Callback<void, uint8_t> remoteRemAddrCb);
  /*
   * Advertise addresses to the peer
   * to create new subflows
   */
  void AdvertiseAddresses();

  /*
   * Add local addresses to the container for future
   * to create new subflows when asked by application
   */
  void AddLocalAddresses(); 

public:
  typedef enum {
    Established = 0, /* contains ESTABLISHED/CLOSE_WAIT */
    Others = 1,     /**< Composed of SYN_RCVD, SYN_SENT*/
    Closing,        /**< CLOSE_WAIT, FIN_WAIT */
    Maximum         /**< keep it last, used to decalre array */
  } mptcp_container_t;

protected:
  friend class Tcp;
  friend class MpTcpSubflow;
  /**
   * \brief Expects InetXSocketAddress
   */
  virtual bool NotifyJoinRequest (const Address &from, const Address & toAddress);
  /**
   * \brief Expects Ipv4 (6 not supported yet)
   */
  bool OwnIP(const Address& address) const;

  /**
   * Should be called after having sent a dataFIN
   * Should send a RST on all subflows in state Other
   * and a FIN for Established subflows
   */
  virtual void CloseAllSubflows();

  // MPTCP connection and subflow set up
  virtual int SetupCallback(void);  // Setup SetRxCallback & SetRxCallback call back for a host

  /**
   * \param sf
   * \param reset True if closing due to reset
   */
  void OnSubflowClosed(Ptr<MpTcpSubflow> sf, bool reset);
  void OnSubflowDupAck(Ptr<MpTcpSubflow> sf);

  /**
   * \brief Currently used as callback for subflows
   * \param dataSeq Used to reconstruct the mapping
   */
  virtual void OnSubflowRecv (Ptr<MpTcpSubflow> sf);

  /*
   * \brief close all subflows
   */
  virtual void TimeWait (void);

  /**
   * \brief Called when a subflow that initiated the connection
   * \brief gets established
   */
  virtual void OnSubflowEstablishment(Ptr<MpTcpSubflow>);

  /**
   * \brief Called when a subflow that received a connection33
   * request gets established
   */
  virtual void OnSubflowEstablished(Ptr<MpTcpSubflow> subflow);

  /**
   * \brief Should be called when subflows enters FIN_WAIT or LAST_ACK
   */
  virtual void OnSubflowClosing(Ptr<MpTcpSubflow>);

  void NotifyRemoteAddAddr(Address address);
  void NotifyRemoteRemAddr(uint8_t addrId);

  /**
   * /brief note Setting a remote key has the sideeffect of enabling MPTCP on the socket
   */
  void SetPeerKey(uint64_t );

  virtual void ConnectionSucceeded (void); // Schedule-friendly wrapper for Socket::NotifyConnectionSucceeded()

  /** Inherit from Socket class: Return data to upper-layer application. Parameter flags
   is not used. Data is returned as a packet of size no larger than maxSize */
  virtual Ptr<Packet> Recv(uint32_t maxSize, uint32_t flags);
  virtual int Send(Ptr<Packet> p, uint32_t flags);

  /**
   * Sending data via subflows with available window size. It sends data only to ESTABLISHED subflows.
   * It sends data by calling SendDataPacket() function.
   * This one is really different from parent class
   *
   * Called by functions: ReceveidAck, NewAck
   * send as  much as possible
   * \return true if it send mappings
   */
  virtual uint32_t SendPendingData(bool withAck = false);
  virtual void ReTxTimeout (void);
  virtual void Retransmit();
  // MPTCP specfic version
  virtual void ReceivedAck (
    SequenceNumber32 dack
  , Ptr<MpTcpSubflow> sf
  , bool count_dupacks
  );

  /**
   * \brief Part of the logic was implemented but this is non-working.
   * \return Always false
   */
  virtual bool IsInfiniteMappingEnabled() const;

  virtual bool DoChecksum() const;
  virtual void OnSubflowDupack (Ptr<MpTcpSubflow> sf, MpTcpMapping mapping);
  virtual void OnSubflowRetransmit (Ptr<MpTcpSubflow> sf) ;

  /**
   *  inherited from parent: update buffers
   * @brief Called from subflows when they receive DATA-ACK. For now calls parent fct
   */
  virtual void NewAck(SequenceNumber32 const& dataLevelSeq, bool resetRTO);

  virtual void OnTimeWaitTimeOut();
  std::vector< std::pair<const Ipv4Address, uint16_t > > RemoteAddressInfo;  //!< Ipv4/v6 address and its port
  std::vector< std::pair<const Ipv4Address, uint16_t > > LocalAddressInfo;  //!< Ipv4/v6 address and its port

protected:
  friend class TcpL4Protocol;
  friend class MpTcpSchedulerRoundRobin;
  friend class MpTcpSchedulerFastestRTT;
  friend class MpTcpNdiffPorts;
  /**
   * \brief called by TcpL4protocol when receiving an MP_JOIN taht does not fit
   * \brief to any Ipv4endpoint. Thus twe create one.
   */
  virtual Ipv4EndPoint*
  NewSubflowRequest(
    Ptr<Packet> p,
    const TcpHeader & header,
    const Address & fromAddress,
    const Address & toAddress,
    Ptr<const TcpOptionMpTcpJoin> join
    );

  /**
   * \brief should accept a stream
   */
  virtual void DumpSubflows() const;

  SubflowList m_subflows[Maximum];
  Callback<bool, Ptr<Socket>, Address, uint8_t > m_onRemoteAddAddr;  //!< return true to create a subflow
  Callback<void, uint8_t > m_onAddrDeletion;    // return true to create a subflow
protected:
  virtual void CreateScheduler(TypeId schedulerTypeId);

  /**
   * \brief the scheduler is so closely
   */
  Ptr<MpTcpScheduler> m_scheduler;  //!<
  uint32_t m_peerToken;
  PathManagerMode m_pathManager {FullMesh};

private:
  uint64_t m_peerKey; //!< Store remote host token
  bool     m_doChecksum;  //!< Compute the checksum. Negociated during 3WHS. Unused
  bool     m_receivedDSS;  //!< True if we received at least one DSS
  bool     m_multipleSubflows; //!< true if required number of subflows have been created [KN]
  double   fLowStartTime;

  /* \brief Utility function used when a subflow changes state
   * \brief Research of the subflow is done
   * \warn This function does not check if the destination container is
   */
  void MoveSubflow(Ptr<MpTcpSubflow> sf, mptcp_container_t to);

  /**
   * \brief Asserts if from == to
   */
  void MoveSubflow(Ptr<MpTcpSubflow> sf, mptcp_container_t from, mptcp_container_t to);

  Callback<void, Ptr<MpTcpSubflow> > m_subflowConnectionSucceeded;  //!< connection succeeded callback
  Callback<void, Ptr<MpTcpSubflow> > m_subflowConnectionFailure;     //!< connection failed callback
  Callback<bool, Ptr<MpTcpSocketBase>, const Address &, const Address & >       m_joinRequest;    //!< connection request callback
  Callback<void, Ptr<MpTcpSubflow> >    m_subflowCreated; //!< connection created callback

  //!
  TypeId m_subflowTypeId;
  TypeId m_schedulerTypeId;
};

}   //namespace ns3

#endif /* MP_TCP_SOCKET_BASE_H */
