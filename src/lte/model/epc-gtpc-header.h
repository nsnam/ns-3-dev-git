/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef EPC_GTPC_HEADER_H
#define EPC_GTPC_HEADER_H

#include "ns3/header.h"
#include "ns3/epc-tft.h"
#include "ns3/eps-bearer.h"

namespace ns3 {

/**
 * \ingroup lte
 *
 * \brief Header of the GTPv2-C protocol
 *
 * Implementation of the GPRS Tunnelling Protocol for Control Plane (GTPv2-C) header
 * according to the 3GPP TS 29.274 document
 */
class GtpcHeader : public Header
{
public:
  GtpcHeader ();
  virtual ~GtpcHeader ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  virtual uint32_t GetMessageSize (void) const;

  /**
   * Get message type
   * \returns the message type
   */
  uint8_t GetMessageType () const;
  /**
   * Get message length
   * \returns the message length
   */
  uint16_t GetMessageLength () const;
  /**
   * Get TEID
   * \returns the TEID
   */
  uint32_t GetTeid () const;
  /**
   * Get sequence number
   * \returns the sequence number
   */
  uint32_t GetSequenceNumber () const;

  /**
   * Set message type
   * \param messageType the message type
   */
  void SetMessageType (uint8_t messageType);
  /**
   * Set message length
   * \param messageLength the message length
   */
  void SetMessageLength (uint16_t messageLength);
  /**
   * Set TEID
   * \param teid the TEID
   */
  void SetTeid (uint32_t teid);
  /**
   * Set sequence number
   * \param sequenceNumber the sequence number
   */
  void SetSequenceNumber (uint32_t sequenceNumber);
  /**
   * Set IEs length. It is used to compute the message length
   * \param iesLength the IEs length
   */
  void SetIesLength (uint16_t iesLength);

  void ComputeMessageLength (void);

  /// Interface Type enumeration
  enum InterfaceType_t
  {
    S1U_ENB_GTPU  = 0,
    S5_SGW_GTPU   = 4,
    S5_PGW_GTPU   = 5,
    S5_SGW_GTPC   = 6,
    S5_PGW_GTPC   = 7,
    S11_MME_GTPC  = 10,
  };

  /// FTEID structure
  struct Fteid_t
  {
    InterfaceType_t interfaceType;
    Ipv4Address addr;
    uint32_t teid;
  };

  /// Message Type enumeration
  enum MessageType_t
  {
    Reserved                  = 0,
    CreateSessionRequest      = 32,
    CreateSessionResponse     = 33,
    ModifyBearerRequest       = 34,
    ModifyBearerResponse      = 35,
    DeleteSessionRequest      = 36,
    DeleteSessionResponse     = 37,
    DeleteBearerCommand       = 66,
    DeleteBearerRequest       = 99,
    DeleteBearerResponse      = 100,
  };


private:
  /**
   * TEID flag.
   * This flag indicates if TEID field is present or not
   */
  bool m_teidFlag;
  /**
   * Message type field.
   * It can be one of the values of MessageType_t
   */
  uint8_t m_messageType;
  /**
   * Message length field.
   * This field indicates the length of the message in octets excluding
   * the mandatory part of the GTP-C header (the first 4 octets)
   */
  uint16_t m_messageLength;
  /**
   * Tunnel Endpoint Identifier (TEID) field
   */
  uint32_t m_teid;
  /**
   * GTP Sequence number field
   */
  uint32_t m_sequenceNumber;


protected:
  /**
   * Serialize the GTP-C header in the GTP-C messages
   * \param i the buffer iterator
   */
  void PreSerialize (Buffer::Iterator &i) const;
  /**
   * Deserialize the GTP-C header in the GTP-C messages
   * \param i the buffer iterator
   */
  uint32_t PreDeserialize (Buffer::Iterator &i);

};


class GtpcIes
{
public:
  enum Cause_t
  {
    RESERVED          = 0,
    REQUEST_ACCEPTED  = 16,
  };

  const uint32_t serializedSizeImsi = 12;
  const uint32_t serializedSizeCause = 6;
  const uint32_t serializedSizeEbi = 5;
  const uint32_t serializedSizeBearerQos = 26;
  const uint32_t serializedSizePacketFilter = 3 + 9 + 9 + 5 + 5 + 3;
  uint32_t GetSerializedSizeBearerTft (std::list<EpcTft::PacketFilter> packetFilters) const;
  const uint32_t serializedSizeUliEcgi = 12;
  const uint32_t serializedSizeFteid = 13;
  const uint32_t serializedSizeBearerContextHeader = 4;


  void SerializeImsi (Buffer::Iterator &i, uint64_t imsi) const;
  uint32_t DeserializeImsi (Buffer::Iterator &i, uint64_t &imsi);

  void SerializeCause (Buffer::Iterator &i, Cause_t cause) const;
  uint32_t DeserializeCause (Buffer::Iterator &i, Cause_t &cause);

  void SerializeEbi (Buffer::Iterator &i, uint8_t epsBearerId) const;
  uint32_t DeserializeEbi (Buffer::Iterator &i, uint8_t &epsBearerId);

  void WriteHtonU40 (Buffer::Iterator &i, uint64_t data) const;
  uint64_t ReadNtohU40 (Buffer::Iterator &i);

  void SerializeBearerQos (Buffer::Iterator &i, EpsBearer bearerQos) const;
  uint32_t DeserializeBearerQos (Buffer::Iterator &i, EpsBearer &bearerQos);

  void SerializeBearerTft (Buffer::Iterator &i, std::list<EpcTft::PacketFilter> packetFilters) const;
  uint32_t DeserializeBearerTft (Buffer::Iterator &i, Ptr<EpcTft> epcTft);

  void SerializeUliEcgi (Buffer::Iterator &i, uint32_t uliEcgi) const;
  uint32_t DeserializeUliEcgi (Buffer::Iterator &i, uint32_t &uliEcgi);

  void SerializeFteid (Buffer::Iterator &i, GtpcHeader::Fteid_t fteid) const;
  uint32_t DeserializeFteid (Buffer::Iterator &i, GtpcHeader::Fteid_t &fteid);

  void SerializeBearerContextHeader (Buffer::Iterator &i, uint16_t length) const;
  uint32_t DeserializeBearerContextHeader (Buffer::Iterator &i, uint16_t &length);
};


class GtpcCreateSessionRequestMessage : public GtpcHeader, public GtpcIes
{
public:
  GtpcCreateSessionRequestMessage ();
  virtual ~GtpcCreateSessionRequestMessage ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetMessageSize (void) const;

  uint64_t GetImsi () const;
  void SetImsi (uint64_t imsi);

  uint32_t GetUliEcgi () const;
  void SetUliEcgi (uint32_t uliEcgi);

  GtpcHeader::Fteid_t GetSenderCpFteid () const;
  void SetSenderCpFteid (GtpcHeader::Fteid_t fteid);

  struct BearerContextToBeCreated
  {
    GtpcHeader::Fteid_t sgwS5uFteid; ///< FTEID
    uint8_t epsBearerId; ///< EPS bearer ID
    Ptr<EpcTft> tft; ///< traffic flow template
    EpsBearer bearerLevelQos; ///< bearer QOS level
  };

  std::list<BearerContextToBeCreated> GetBearerContextsToBeCreated () const;
  void SetBearerContextsToBeCreated (std::list<BearerContextToBeCreated> bearerContexts);

private:
  uint64_t m_imsi;
  uint32_t m_uliEcgi;
  GtpcHeader::Fteid_t m_senderCpFteid;

  std::list<BearerContextToBeCreated> m_bearerContextsToBeCreated;
};


class GtpcCreateSessionResponseMessage : public GtpcHeader, public GtpcIes
{
public:
  GtpcCreateSessionResponseMessage ();
  virtual ~GtpcCreateSessionResponseMessage ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetMessageSize (void) const;

  Cause_t GetCause () const;
  void SetCause (Cause_t cause);

  GtpcHeader::Fteid_t GetSenderCpFteid () const;
  void SetSenderCpFteid (GtpcHeader::Fteid_t fteid);

  struct BearerContextCreated
  {
    uint8_t epsBearerId; ///< EPS bearer ID
    uint8_t cause; ///< Cause
    Ptr<EpcTft> tft; ///< Bearer traffic flow template
    GtpcHeader::Fteid_t fteid; ///< FTEID
    EpsBearer bearerLevelQos; ///< Bearer QOS level
  };

  std::list<BearerContextCreated> GetBearerContextsCreated () const;
  void SetBearerContextsCreated (std::list<BearerContextCreated> bearerContexts);

private:
  Cause_t m_cause;
  GtpcHeader::Fteid_t m_senderCpFteid;

  std::list<BearerContextCreated> m_bearerContextsCreated;
};


class GtpcModifyBearerRequestMessage : public GtpcHeader, public GtpcIes
{
public:
  GtpcModifyBearerRequestMessage ();
  virtual ~GtpcModifyBearerRequestMessage ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetMessageSize (void) const;

  uint8_t GetImsi () const;
  void SetImsi (uint8_t imsi);

  uint32_t GetUliEcgi () const;
  void SetUliEcgi (uint32_t uliEcgi);

  struct BearerContextToBeModified
  {
    uint8_t epsBearerId; ///< EPS bearer ID
    GtpcHeader::Fteid_t fteid; ///< FTEID
  };

  std::list<BearerContextToBeModified> GetBearerContextsToBeModified () const;
  void SetBearerContextsToBeModified (std::list<BearerContextToBeModified> bearerContexts);

private:
  uint64_t m_imsi;
  uint32_t m_uliEcgi;

  std::list<BearerContextToBeModified> m_bearerContextsToBeModified;
};


class GtpcModifyBearerResponseMessage : public GtpcHeader, public GtpcIes
{
public:
  GtpcModifyBearerResponseMessage ();
  virtual ~GtpcModifyBearerResponseMessage ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetMessageSize (void) const;

  Cause_t GetCause () const;
  void SetCause (Cause_t cause);

private:
  Cause_t m_cause;
};


class GtpcDeleteBearerCommandMessage : public GtpcHeader, public GtpcIes
{
public:
  GtpcDeleteBearerCommandMessage ();
  virtual ~GtpcDeleteBearerCommandMessage ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetMessageSize (void) const;

  struct BearerContext
  {
    uint8_t m_epsBearerId; ///< EPS bearer ID
  };

  std::list<BearerContext> GetBearerContexts () const;
  void SetBearerContexts (std::list<BearerContext> bearerContexts);

private:
  std::list<BearerContext> m_bearerContexts;
};


class GtpcDeleteBearerRequestMessage : public GtpcHeader, public GtpcIes
{
public:
  GtpcDeleteBearerRequestMessage ();
  virtual ~GtpcDeleteBearerRequestMessage ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetMessageSize (void) const;

  std::list<uint8_t> GetEpsBearerIds () const;
  void SetEpsBearerIds (std::list<uint8_t> epsBearerIds);

private:
  std::list<uint8_t> m_epsBearerIds;
};


class GtpcDeleteBearerResponseMessage : public GtpcHeader, public GtpcIes
{
public:
  GtpcDeleteBearerResponseMessage ();
  virtual ~GtpcDeleteBearerResponseMessage ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetMessageSize (void) const;

  Cause_t GetCause () const;
  void SetCause (Cause_t cause);

  std::list<uint8_t> GetEpsBearerIds () const;
  void SetEpsBearerIds (std::list<uint8_t> epsBearerIds);

private:
  Cause_t m_cause;
  std::list<uint8_t> m_epsBearerIds;
};

} // namespace ns3

#endif // EPC_GTPC_HEADER_H
