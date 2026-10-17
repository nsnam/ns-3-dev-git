/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_ZDO_H
#define ZIGBEE_ZDO_H

#include "ns3/event-id.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"
#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-callback.h"
#include "ns3/traced-value.h"
#include "ns3/zigbee-aps.h"
#include "ns3/zigbee-nwk.h"

namespace ns3
{
namespace zigbee
{

/**
 *  Zigbee Device Profile (ZDP) enumeration of status codes.
 *  See Zigbee Specification 2023, Section 2.4.5, Table 2-129
 *  Note: A standardized list of status codes is not defined in
 *  the Zigbee Specification 2017, that is why we used the
 *  definitions later introduced on the Zigbee Specification 2023.
 */
enum class ZdpStatus : uint8_t
{
    SUCCESS = 0x00,            //!< The request was successful.
    INV_REQUESTTYPE = 0x80,    //!< The request type is invalid or not supported.
    DEVICE_NOT_FOUND = 0x81,   //!< The device with the specified IEEE address was
                               //!< not found in the network.
    INVALID_EP = 0x82,         //!< The endpoint specified in the request is invalid
                               //!< or not supported.
    NOT_ACTIVE = 0x83,         //!< The device is not currently active in the network.
    NOT_SUPPORTED = 0x84,      //!< The requested operation is not supported by the device
                               //!< or the network.
    TIMEOUT = 0x85,            //!< The request timed out.
    NO_MATCH = 0x86,           //!< No matching entry was found for the request.
    NO_ENTRY = 0x88,           //!< The requested entry does not exist in the device's
                               //!< table or cache.
    NO_DESCRIPTOR = 0x89,      //!< The requested descriptor (e.g., node, power, simple)
                               //!< is not available or not found.
    INSUFFICIENT_SPACE = 0x8A, //!< The device does not have enough space to store
                               //!< the requested information or entry.
    NOT_PERMITTED = 0x8B,      //!< The requested operation is not permitted due to
                               //!< security or policy restrictions.
    TABLE_FULL = 0x8C,         //!< The device's table (e.g., neighbor, routing, binding)
                               //!< is full and cannot accommodate new entries.
    NOT_AUTHORIZED = 0x8D,     //!< The device is not authorized to perform the requested
                               //!< operation, possibly due to security or access control
                               //!< restrictions.
    INVALID_INDEX = 0x8F,      //!< The index specified in the request is invalid or out of range.
    FRAME_TOO_LARGE = 0x90,    //!< The frame size of the request or response exceeds the maximum
                               //!< allowed size for the device or network.
    BAD_KEY_NEGOTIATION_METHOD = 0x91, //!< The key negotiation method used in the request
                                       //!< is not supported or is invalid.
    TEMPORARY_FAILURE = 0x92           //!< A temporary failure occurred while processing
                                       //!< the request, and the operation may be retried later.
};

/**
 * @ingroup zigbee
 *
 *  Zigbee Device Profile (ZDP) enumeration of request types for NWK_addr_req command.
 */
enum class NwkAddrRequestType : std::uint8_t
{
    SINGLE_DEVICE_RESPONSE = 0x00, //!< Request type for a single device response.
    EXTENDED_RESPONSE = 0x02,      //!< Request type for an extended response,
                                   //!< not currently supported.
};

/**
 * @ingroup zigbee
 *
 *  ZDO Network Address Request Parameters.
 */
struct NwkAddrRequestParams
{
    Mac16Address m_dstAddr;  //!< The destination of the request, unicast or broadcast (0xFFFF).
    Mac64Address m_ieeeAddr; //!< The known IEEE Address(64-bit address) of the device that we are
                             //!< request its network address (16-bit address).
    NwkAddrRequestType m_requestType{NwkAddrRequestType::SINGLE_DEVICE_RESPONSE}; //!< Request type
                                                                                  //!< of this
                                                                                  //!< command
    uint8_t m_startIndex{0}; //!< Placeholder, not supported.
};

/**
 * @ingroup zigbee
 *
 *  ZDO Network Address Response Parameters.
 */
struct NwkAddrResponseParams
{
    Mac16Address m_dstAddr; //!< The destination of the response
                            //!< (the device that issued the NWK_addr_req).
    uint8_t m_tsn{0};       //!< The transaction sequence number, must match the one used in the
                            //!< corresponding NWK_addr_req.
    ZdpStatus m_status{ZdpStatus::INV_REQUESTTYPE}; //!< The status of the NWK_addr_req
                                                    //!< being responded to.
    Mac64Address m_ieeeAddrRemoteDev; //!< The IEEE address (64-bit address) of the remote device
                                      //!< matching the request.
    Mac16Address m_nwkAddrRemoteDev;  //!< The network address (16-bit address) of the remote device
                                      //!< matching the request.
    // TODO Add startIndex, numAssocDev, and associatedDevList when implementing extended response
};

/**
 * @ingroup zigbee
 *
 *  ZDO Network Address Response Indication Parameters.
 *  This structure or even its parameters are not standardized in the Zigbee Specification,
 *  however, these were chosen based on the parameters is by existing Zigbee stacks like TI's
 * Z-Stack. See:
 * https://software-dl.ti.com/simplelink/esd/simplelink_cc13x2_26x2_sdk/3.20.00.68/exports/docs/zigbee/doxygen/zigbee/html/struct__zstack__zdonwkaddrrspind__t.html
 */
struct NwkAddrRespIndParams
{
    ZdpStatus m_status{ZdpStatus::INV_REQUESTTYPE}; //!< The status of the NWK_addr_req
                                                    //!< being responded to.
    Mac64Address m_ieeeAddrRemoteDev; //!< The IEEE address (64-bit address) of the remote device
                                      //!< matching the request.
    Mac16Address m_nwkAddrRemoteDev;  //!< The network address (16-bit address) of the remote device
                                      //!< matching the request.
    // TODO Add startIndex, numAssocDev, and associatedDevList when implementing extended response
};

class ZigbeeZdo : public Object
{
  public:
    /**
     * Get the TypeId of ZigbeeZdo class.
     *
     * @return The TypeId of ZigbeeZdo class.
     */
    static TypeId GetTypeId();
    ZigbeeZdo();
    ~ZigbeeZdo() override;

    /**
     * This callback is called after a ASDU has successfully received and
     *  APS push it to deliver it to the next higher layer (typically the application framework).
     */
    using ZdoNwkAddrRspIndCallback = Callback<void, NwkAddrRespIndParams>;

    /**
     * Set the underlying APS to use in this Zigbee ZDO
     *
     * @param aps The pointer to the underlying Zigbee APS to set to this Zigbee ZDO
     */
    void SetAps(Ptr<ZigbeeAps> aps);

    /**
     * Get the underlying APS used by the current Zigbee ZDO.
     *
     * @return The pointer to the underlying APS object currently connected to the Zigbee ZDO.
     */
    Ptr<ZigbeeAps> GetAps() const;

    /**
     * Set the underlying NWK to use in this Zigbee ZDO
     *
     * @param nwk The pointer to the underlying Zigbee APS to set to this Zigbee ZDO
     */
    void SetNwk(Ptr<ZigbeeNwk> nwk);

    /**
     * Get the underlying APS used by the current Zigbee ZDO.
     *
     * @return The pointer to the underlying NWK object currently connected to the Zigbee ZDO.
     */
    Ptr<ZigbeeNwk> GetNwk() const;

    /**
     * This function is called when the Zigbee Stack receives an APSDE-DATA.indication
     * with destination endpoint 0, which is the endpoint used by the Zigbee Device Object (ZDO).
     *
     * @param params The parameters of the APSDE-DATA.indication.
     * @param p The packet received.
     */
    void ApsDataIndication(ApsdeDataIndicationParams params, Ptr<Packet> p);

    /**
     * This function is used to send a Network Address (16-bit address)Request to a remote device
     * when the IEEE address (64-bit address) of the device is known.
     *
     * @param params The parameters of the Network Address Request.
     */
    void NwkAddrRequest(NwkAddrRequestParams params);

    /**
     * This function is used to send a Network Address (16-bit address) Response to a remote
     * device in reply to a previously received NWK_addr_req.
     *
     * @param params The parameters of the Network Address Response.
     */
    void NwkAddrResponse(NwkAddrResponseParams params);

    /**
     * Effect on Receipt (NWK_addr_req)
     * See Zigbee Specification 2017, Section 2.4.3.1.1
     *
     * @param asdu The received packet containing the NWK_addr_req command.
     * @param dataParams APSDE-DATA.indication parameters that carried the NWK_addr_req command.
     */
    void HandleNwkAddrRequest(Ptr<Packet> asdu, ApsdeDataIndicationParams dataParams);

    /**
     * Effect on Receipt (NWK_addr_rsp)
     * See Zigbee Specification 2017, Section 2.4.4.2.1
     *
     * @param asdu The received packet containing the NWK_addr_rsp command.
     * @param dataParams APSDE-DATA.indication parameters that carried the NWK_addr_rsp command.
     */
    void HandleNwkAddrResponse(Ptr<Packet> asdu, ApsdeDataIndicationParams dataParams);

    /**
     * Set the indication callback to be called when a response (NWK_addr_rsp) to
     * an NWK_addr_req is received.
     *
     * @param c The callback to be called when a NWK_addr_rsp is received.
     */
    void SetZdoNwkAddrRspIndCallback(ZdoNwkAddrRspIndCallback c);

  protected:
    void DoInitialize() override;
    void DoDispose() override;
    void NotifyConstructionCompleted() override;

  private:
    /**
     * This callback is used to to notify the result of a NWK_addr_req
     * request in the ZDO to the Application framework (AF).
     * This callback is not standardized in the Zigbee Specification, but
     * similar solutions are often used in ZDO implementations.
     */
    ZdoNwkAddrRspIndCallback m_zdoNwkAddrRspIndCallback;

    Ptr<ZigbeeNwk> m_nwk;  //!< Pointer to the Zigbee Network Layer (NWK) object.
    Ptr<ZigbeeAps> m_aps;  //!< Pointer to the Zigbee Application Support Sub-layer (APS) object.
    SequenceNumber8 m_tsn; //!< The transaction sequence number
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_ZDO_H */
