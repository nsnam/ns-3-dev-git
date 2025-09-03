/*
 * Copyright (c) 2025 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_APS_H
#define ZIGBEE_APS_H

#include "zigbee-aps-header.h"
#include "zigbee-aps-tables.h"
#include "zigbee-group-table.h"
#include "zigbee-nwk.h"

#include "ns3/event-id.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"
#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-callback.h"
#include "ns3/traced-value.h"

#include <cstdint>
#include <iomanip>
#include <iterator>

namespace ns3
{
namespace zigbee
{

/**
 * @ingroup zigbee
 *
 * APS Destination Address Mode,
 * Zigbee Specification r22.1.0
 * Table 2-2 APSDE-DATA.request Parameters
 * See Table 2-4 APSDE-DATA.indication Parameters
 */
enum class ApsDstAddressMode : std::uint8_t
{
    DST_ADDR_AND_DST_ENDPOINT_NOT_PRESENT = 0x00, //!< Destination address and destination endpoint
                                                  //!< not present.
    GROUP_ADDR_DST_ENDPOINT_NOT_PRESENT = 0x01,   //!< Group address or 16-bit destination address
                                                  //!< present but destination endpoint not present.
    DST_ADDR16_DST_ENDPOINT_PRESENT = 0x02,       //!< 16-bit destination address and destination
                                                  //!< endpoint present.
    DST_ADDR64_DST_ENDPOINT_PRESENT = 0x03,       //!< 64-bit destination address and destination
                                                  //!< endpoint present.
    DST_ADDR64_DST_ENDPOINT_NOT_PRESENT = 0x04    //!< 64-bit address present but destination
                                                  //!< endpoint not present.
};

/**
 * @ingroup zigbee
 *
 * APS Source Address Mode,
 * Zigbee Specification r22.1.0
 * See Table 2-4 APSDE-DATA.indication Parameters
 */
enum class ApsSrcAddressMode : std::uint8_t
{
    SRC_ADDR16_SRC_ENDPOINT_PRESENT = 0x02, //!< 16-bit source address and source endpoint present
    SRC_ADDR64_SRC_ENDPOINT_PRESENT = 0x03, //!< 64-bit source address and source endpoint present
    SRC_ADDR64_SRC_ENDPOINT_NOT_PRESENT = 0x04 //!< 64-bit source address present but source
                                               //!< endpoint not present
};

/**
 * @ingroup zigbee
 *
 * APS Security status
 * See Zigbee Specification r22.1.0, Table 2-4
 * APSDE-DATA.indication Parameters
 */
enum class ApsSecurityStatus : std::uint8_t
{
    UNSECURED = 0x00,       //!< Unsecured status
    SECURED_NWK_KEY = 0x01, //!< Use NWK secure key
    SECURED_LINK_KEY = 0x02 //!< Use link secure key
};

/**
 * @ingroup zigbee
 *
 * APS Sub-layer Status Values
 * See Zigbee Specification r22.1.0, Table 2-27
 */
enum class ApsStatus : std::uint8_t
{
    SUCCESS = 0x00,              //!< A request has been executed successfully.
    ASDU_TOO_LONG = 0xa0,        //!< A received fragmented
                                 //!< frame could not be defragmented at the current time.
    DEFRAG_DEFERRED = 0xa1,      //!< Defragmentation deferred.
    DEFRAG_UNSUPPORTED = 0xa2,   //!< Defragmentation is not supported.
    ILLEGAL_REQUEST = 0xa3,      //!< Illegal request
    INVALID_BINDING = 0xa4,      //!< Invalid binding
    INVALID_GROUP = 0xa5,        //!< Invalid group
    INVALID_PARAMETER = 0xa6,    //!< A parameter value was invalid or out of range
    NO_ACK = 0xa7,               //!< No Acknowledgment
    NO_BOUND_DEVICE = 0xa8,      //!< No bound device
    NO_SHORT_ADDRESS = 0xa9,     //!< No short address present
    NOT_SUPPORTED = 0xaa,        //!< Not supported in APS
    SECURED_LINK_KEY = 0xab,     //!< Secured link key present
    SECURED_NWK_KEY = 0xac,      //!< Secured network key present
    SECURITY_FAIL = 0xad,        //!< Security failed
    TABLE_FULL = 0xae,           //!< Binding table or group table is full
    UNSECURED = 0xaf,            //!< Unsecured
    UNSUPPORTED_ATTRIBUTE = 0xb0 //!< Unsupported attribute
};

/**
 * @ingroup zigbee
 *
 * Zigbee Specification r22.1.0, Section 2.2.4.1.1
 * APSDE-DATA.request params.
 */
struct ApsdeDataRequestParams
{
    ApsDstAddressMode m_dstAddrMode{
        ApsDstAddressMode::DST_ADDR_AND_DST_ENDPOINT_NOT_PRESENT}; //!< Destination address mode.
    Mac16Address m_dstAddr16;    //!< The destination 16-bit address
    Mac64Address m_dstAddr64;    //!< The destination 64-bit address
    uint8_t m_dstEndPoint{0};    //!< The destination endpoint
    uint16_t m_profileId{0};     //!< The application profile ID
    uint16_t m_clusterId{0};     //!< The application cluster ID
    uint8_t m_srcEndPoint{0};    //!< The source endpoint
    uint32_t m_asduLength{0};    //!< The ASDU length
    uint8_t m_txOptions{0};      //!< Transmission options
    bool m_useAlias{false};      //!< Indicates if alias is used in this transmission
    Mac16Address m_aliasSrcAddr; //!< Alias source address
    uint8_t m_aliasSeqNumb{0};   //!< Alias sequence number
    uint8_t m_radius{0};         //!< Radius (Number of hops this message travels)
};

/**
 * @ingroup zigbee
 *
 * Zigbee Specification r22.1.0, Section 2.2.4.1.2
 * APSDE-DATA.confirm params.
 */
struct ApsdeDataConfirmParams
{
    ApsDstAddressMode m_dstAddrMode{
        ApsDstAddressMode::DST_ADDR_AND_DST_ENDPOINT_NOT_PRESENT}; //!< Destination address mode.
    Mac16Address m_dstAddr16; //!< The destination 16-bit address.
    Mac64Address m_dstAddr64; //!< The destination IEEE address (64-bit address).
    uint8_t m_dstEndPoint{0}; //!< The destination endpoint.
    uint8_t m_srcEndPoint{0}; //!< The source endpoint.
    ApsStatus m_status{ApsStatus::UNSUPPORTED_ATTRIBUTE}; //!< The confirmation status.
    Time m_txTime;                                        //!< The transmission timestamp.
};

/**
 * @ingroup zigbee
 *
 *  Zigbee Specification r22.1.0, Section 2.2.4.1.3
 *  APSDE-DATA.indications params.
 */
struct ApsdeDataIndicationParams
{
    ApsDstAddressMode m_dstAddrMode{
        ApsDstAddressMode::DST_ADDR_AND_DST_ENDPOINT_NOT_PRESENT}; //!< The destination
                                                                   //!< address mode
    Mac16Address m_dstAddr16;    //!< The destination 16-bit address
    Mac64Address m_dstAddr64;    //!< The destination IEEE address (64-bit address)
    uint8_t m_dstEndPoint{0xF0}; //!< The destination endpoint
    ApsSrcAddressMode m_srcAddrMode{
        ApsSrcAddressMode::SRC_ADDR16_SRC_ENDPOINT_PRESENT}; //!< The
                                                             //!< source address mode

    Mac16Address m_srcAddress16;            //!< The 16-bit address
    Mac64Address m_srcAddress64;            //!< The IEEE source address (64-bit address)
    uint8_t m_srcEndpoint{0xF0};            //!< The application source endpoint
    uint16_t m_profileId{0xC0DE};           //!< The application profile ID
    uint16_t m_clusterId{0x0000};           //!< The application cluster ID
    uint8_t asduLength{0};                  //!< The size of the the ASDU packet
    ApsStatus m_status{ApsStatus::SUCCESS}; //!< The data indication status
    ApsSecurityStatus m_securityStatus{ApsSecurityStatus::UNSECURED}; //!< Security status
    uint8_t m_linkQuality{0}; //!< The link quality indication value
    Time m_rxTime;            //!< The reception timestamp
};

/**
 * @ingroup zigbee
 *
 * Zigbee Specification r22.1.0, Sections 2.2.4.3.1 and 2.2.4.3.3
 * APSME-BIND.request and APSME-UNBIND.request params.
 */
struct ApsmeBindRequestParams
{
    Mac64Address m_srcAddr;   //!< The source IEEE address (64-bit address)
    uint8_t m_srcEndPoint{0}; //!< The application source endpoint
    uint16_t m_clusterId{0};  //!< The application cluster ID
    ApsDstAddressModeBind m_dstAddrMode{
        ApsDstAddressModeBind::GROUP_ADDR_DST_ENDPOINT_NOT_PRESENT}; //!< Destination address mode.
    Mac16Address m_dstAddr16;    //!< The destination 16-bit address
    Mac64Address m_dstAddr64;    //!< The destination 64-bit address
    uint8_t m_dstEndPoint{0xF0}; //!< The application destination endpoint
};

/**
 * @ingroup zigbee
 *
 * Zigbee Specification r22.1.0, Sections 2.2.4.3.2 and 2.2.4.3.4
 * APSME-BIND.confirm and APSME-UNBIND.confirm params
 */
struct ApsmeBindConfirmParams
{
    ApsStatus m_status{ApsStatus::UNSUPPORTED_ATTRIBUTE}; //!< The status of the bind request
    Mac64Address m_srcAddr;                               //!< The application source address
    uint8_t m_srcEndPoint{0};                             //!< The application source endpoint
    uint16_t m_clusterId{0};                              //!< The application cluster ID
    ApsDstAddressModeBind m_dstAddrMode{
        ApsDstAddressModeBind::GROUP_ADDR_DST_ENDPOINT_NOT_PRESENT}; //!< Destination address mode.
    Mac16Address m_dstAddr16;    //!< The destination 16-bit address
    Mac64Address m_dstAddr64;    //!< The destination 64-bit address
    uint8_t m_dstEndPoint{0xF0}; //!< The application destination endpoint
};

/**
 * @ingroup zigbee
 *
 * Zigbee Specification r22.1.0, Section 2.2.4.5.1  and  2.2.4.5.3
 * APSME-ADD-GROUP.request and APSME-REMOVE-GROUP.request params
 */
struct ApsmeGroupRequestParams
{
    Mac16Address m_groupAddress; //!< The group address to add
    uint8_t m_endPoint{1};       //!< The endpoint to which the group address is associated
};

/**
 * @ingroup zigbee
 *
 * Zigbee Specification r22.1.0, Section 2.2.4.5.2 and 2.2.4.5.4
 * APSME-ADD-GROUP.confirm  and APSME-REMOVE-GROUP.confirm params
 */
struct ApsmeGroupConfirmParams
{
    ApsStatus m_status{ApsStatus::INVALID_PARAMETER}; //!< The status of the add group request
    Mac16Address m_groupAddress;                      //!< The group address being added
    uint8_t m_endPoint{1}; //!< The endpoint to which the given group is being added.
};

/**
 * @ingroup zigbee
 *
 * Zigbee Specification r22.1.0, Section 2.2.4.5.6
 * APSME-REMOVE-ALL-GROUPS.request params
 */
struct ApsmeRemoveAllGroupsConfirmParams
{
    ApsStatus m_status{
        ApsStatus::INVALID_PARAMETER}; //!< The status of the remove all groups request
    uint8_t m_endPoint{1};             //!< The endpoint from which all groups are being removed.
};

/**
 * @ingroup zigbee
 *
 * Zigbee Specification r22.1.0, Section 2.2.3
 * Class that implements the Zigbee Specification Application Support Sub-layer (APS).
 */
class ZigbeeAps : public Object
{
  public:
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Default constructor.
     */
    ZigbeeAps();
    ~ZigbeeAps() override;

    /**
     * This callback is called to confirm a successfully transmission of an ASDU.
     */
    using ApsdeDataConfirmCallback = Callback<void, ApsdeDataConfirmParams>;

    /**
     * This callback is called after a ASDU has successfully received and
     *  APS push it to deliver it to the next higher layer (typically the application framework).
     */
    using ApsdeDataIndicationCallback = Callback<void, ApsdeDataIndicationParams, Ptr<Packet>>;

    /**
     * This callback is called to confirm a successfully addition of a destination
     * into the binding table.
     */
    using ApsmeBindConfirmCallback = Callback<void, ApsmeBindConfirmParams>;

    /**
     * This callback is called to confirm a successfully  unbind request performed
     * into the binding table.
     */
    using ApsmeUnbindConfirmCallback = Callback<void, ApsmeBindConfirmParams>;

    /**
     * This callback is called to confirm a successfully addition of a group address
     * and or endPoint into the group table.
     */
    using ApsmeAddGroupConfirmCallback = Callback<void, ApsmeGroupConfirmParams>;

    /**
     * This callback is called to confirm a successfully removal of a group address
     * and or endPoint from the group table.
     */
    using ApsmeRemoveGroupConfirmCallback = Callback<void, ApsmeGroupConfirmParams>;

    /**
     * This callback is called to confirm a successfully removal of an endpoint from
     * all the the groups.
     */
    using ApsmeRemoveAllGroupsConfirmCallback = Callback<void, ApsmeRemoveAllGroupsConfirmParams>;

    /**
     * Set the underlying NWK to use in this Zigbee APS
     *
     * @param nwk The pointer to the underlying Zigbee NWK to set to this Zigbee APS
     */
    void SetNwk(Ptr<ZigbeeNwk> nwk);

    /**
     * Get the group table used by this Zigbee APS.
     *
     * @param groupTable The pointer to the group table to set.
     */
    void SetGroupTable(Ptr<ZigbeeGroupTable> groupTable);

    /**
     * Get the underlying NWK used by the current Zigbee APS.
     *
     * @return The pointer to the underlying NWK object currently connected to the Zigbee APS.
     */
    Ptr<ZigbeeNwk> GetNwk() const;

    /**
     * Zigbee Specification r22.1.0, Section 2.2.4.1.1
     * APSDE-DATA.request
     * Request the transmission of data to one or more entities.
     *
     * @param params The APSDE data request params
     * @param asdu  The packet to transmit
     */
    void ApsdeDataRequest(ApsdeDataRequestParams params, Ptr<Packet> asdu);

    /**
     * Zigbee Specification r22.1.0, Section 2.2.4.3.1
     * APSME-BIND.request
     * Bind a source entry to one or more destination entries in the binding table.
     *
     * @param params The APSDE bind request params
     */
    void ApsmeBindRequest(ApsmeBindRequestParams params);

    /**
     * Zigbee Specification r22.1.0, Section 2.2.4.3.3
     * APSME-BIND.request
     * Unbind a destination entry from a source entry in the binding table.
     *
     * @param params The APSDE bind request params
     */
    void ApsmeUnbindRequest(ApsmeBindRequestParams params);

    /**
     * Zigbee Specification r22.1.0, Section 2.2.4.5.1
     * APSME-ADD-GROUP.request
     * Request that group membership for a particular group be added
     * for a particular endpoint.
     *
     * @param params The APSME add group request params
     */
    void ApsmeAddGroupRequest(ApsmeGroupRequestParams params);

    /**
     * Zigbee Specification r22.1.0, Section 2.2.4.5.3
     * APSME-REMOVE-GROUP.request
     * Request that group membership for a particular group be removed
     * for a particular endpoint.
     *
     * @param params The APSME remove group request params
     */
    void ApsmeRemoveGroupRequest(ApsmeGroupRequestParams params);

    /**
     * Zigbee Specification r22.1.0, Section 2.2.4.5.5
     * APSME-REMOVE-ALL-GROUPS.request
     * Remove membership in all groups from an endpoint, so that no group-addressed frames
     * will be delivered to that endpoint.
     *
     * @param endPoint The endpoint from which all groups are being removed.
     */
    void ApsmeRemoveAllGroupsRequest(uint8_t endPoint);

    /**
     * Zigbee Specification r22.1.0, Section 3.2.1.2
     * NLDE-DATA.confirm
     * Used to report to the APS the transmission of data from the NWK.
     *
     * @param params The NLDE data confirm params
     */
    void NldeDataConfirm(NldeDataConfirmParams params);

    /**
     * Zigbee Specification r22.1.0, Section 3.2.1.3
     * NLDE-DATA.indication
     * Used to report to the APS the reception of data from the NWK.
     *
     * @param params The NLDE data indication params
     * @param nsdu The packet received
     */
    void NldeDataIndication(NldeDataIndicationParams params, Ptr<Packet> nsdu);

    /**
     * Zigbee Specification r22.1.0, Section 2.2.8.4.2
     * Reception and Rejection of data from the NWK.
     *
     *  @param apsHeader The APS header of the received packet
     *  @param params The NLDE data indication params
     *  @param nsdu The packet received
     */
    void ReceiveData(const ZigbeeApsHeader& apsHeader,
                     const NldeDataIndicationParams& params,
                     Ptr<Packet> nsdu);

    /**
     *  Set the callback as part of the interconnections between the APS and
     *  the next layer or service (typically the application framework). The callback
     *  implements the callback used in a APSDE-DATA.confirm
     *
     *  @param c the ApsdeDataConfirm callback
     */
    void SetApsdeDataConfirmCallback(ApsdeDataConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the APS and
     *  the next layer or service (typically the application framework). The callback
     *  implements the callback used in a APSDE-DATA.indication
     *
     *  @param c the ApsdeDataIndication callback
     */
    void SetApsdeDataIndicationCallback(ApsdeDataIndicationCallback c);

    /**
     *  Set the callback as part of the interconnections between the APS and
     *  the next layer or service (typically the application framework). The callback
     *  implements the callback used in a APSME-BIND.confirm
     *
     *  @param c the ApsmeBindConfirm callback
     */
    void SetApsmeBindConfirmCallback(ApsmeBindConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the APS and
     *  the next layer or service (typically the application framework). The callback
     *  implements the callback used in a APSDE-UNBIND.confirm
     *
     *  @param c the ApsdeUnbindConfirm callback
     */
    void SetApsmeUnbindConfirmCallback(ApsmeUnbindConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the APS and
     *  the next layer or service (typically the application framework). The callback
     *  implements the callback used in a APSME-ADD-GROUP.confirm
     *
     *  @param c the ApsmeAddGroupConfirm callback
     */
    void SetApsmeAddGroupConfirmCallback(ApsmeAddGroupConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the APS and
     *  the next layer or service (typically the application framework). The callback
     *  implements the callback used in a APSME-REMOVE-GROUP.confirm
     *
     *  @param c the ApsmeRemoveGroupConfirm callback
     */
    void SetApsmeRemoveGroupConfirmCallback(ApsmeRemoveGroupConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the APS and
     *  the next layer or service (typically the application framework). The callback
     *  implements the callback used in a APSME-REMOVE-ALL-GROUPS.confirm
     *
     *  @param c the ApsmeRemoveAllGroupsConfirm callback
     */
    void SetApsmeRemoveAllGroupsConfirmCallback(ApsmeRemoveAllGroupsConfirmCallback c);

  protected:
    void DoInitialize() override;
    void DoDispose() override;
    void NotifyConstructionCompleted() override;

  private:
    /**
     * Send a Groupcast or IEEE address destination from a list of destination in
     * the binding table.
     *
     * @param params The APSDE data request params
     * @param asdu  The packet to transmit
     */
    void SendDataWithBindingTable(ApsdeDataRequestParams params, Ptr<Packet> asdu);

    /**
     * Send a regular UCST or BCST data transmission to a known 16-bit address destination.
     *
     * @param params The APSDE data request params
     * @param asdu  The packet to transmit
     */
    void SendDataUcstBcst(ApsdeDataRequestParams params, Ptr<Packet> asdu);

    /**
     * Send a Groupcast to a group address destination.
     *
     * @param params The APSDE data request params
     * @param asdu  The packet to transmit
     */
    void SendDataGroup(ApsdeDataRequestParams params, Ptr<Packet> asdu);

    /**
     *  This callback is used to to notify the results of a data transmission
     *  request to the Application framework (AF) making the request.
     *  See Zigbee specification r22.1.0, Section 2.2.4.1.2
     */
    ApsdeDataConfirmCallback m_apsdeDataConfirmCallback;

    /**
     *  This callback is used to to notify the reception of data
     *  to the Application framework (AF).
     *  See Zigbee specification r22.1.0, Section 2.2.4.1.3
     */
    ApsdeDataIndicationCallback m_apsdeDataIndicationCallback;

    /**
     *  This callback is used to to notify the result of a binding
     *  request in the APS to the Application framework (AF).
     *  See Zigbee specification r22.1.0, Section 2.2.4.3.2
     */
    ApsmeBindConfirmCallback m_apsmeBindConfirmCallback;

    /**
     *  This callback is used to to notify the result of a unbinding
     *  request in the APS to the Application framework (AF).
     *  See Zigbee specification r22.1.0, Section 2.2.4.3.4
     */
    ApsmeUnbindConfirmCallback m_apsmeUnbindConfirmCallback;

    /**
     *  This callback is used to to notify the result of endpoint addition
     *  request in the APS to the Application framework (AF).
     *  See Zigbee specification r22.1.0, Section 2.2.4.5.2
     */
    ApsmeAddGroupConfirmCallback m_apsmeAddGroupConfirmCallback;

    /**
     *  This callback is used to to notify the result of a endpoint removal
     *  request in the APS to the Application framework (AF).
     *  See Zigbee specification r22.1.0, Section 2.2.4.5.4
     */
    ApsmeRemoveGroupConfirmCallback m_apsmeRemoveGroupConfirmCallback;

    /**
     *  This callback is used to to notify the result of a endpoint removal
     *  request in the APS to the Application framework (AF).
     *  See Zigbee specification r22.1.0, Section 2.2.4.5.5
     */
    ApsmeRemoveAllGroupsConfirmCallback m_apsmeRemoveAllGroupsConfirmCallback;

    /**
     * The underlying Zigbee NWK connected to this Zigbee APS.
     */
    Ptr<ZigbeeNwk> m_nwk;

    /**
     * The group table used by this Zigbee APS.
     * The group table is a shared resource with the NWK layer.
     * Zigbee Specification r22.1.0, Section 2.2.7.2
     */
    Ptr<ZigbeeGroupTable> m_apsGroupTable;

    /**
     * The sequence number used in packet Tx with APS headers.
     * Zigbee Specification r22.1.0, Section 2.2.7.2
     */
    SequenceNumber8 m_apsCounter;

    /**
     * The binding table used by this Zigbee APS.
     * Zigbee Specification r22.1.0, Section 2.2.7.2
     */
    BindingTable m_apsBindingTable;

    /**
     * The APS non-member radius, used to limit the number of hops
     * for non-member multicast group devices when using NWK layer multicast.
     * Valid range 0x00 to 0x07.
     * Zigbee Specification r22.1.0, Section 2.2.7.2
     */
    uint8_t m_apsNonMemberRadius;
};

/**
 * @ingroup zigbee
 *
 * Helper class used to craft the transmission options bitmap used  by the
 * APSDE-DATA.request.
 */
class ZigbeeApsTxOptions
{
  public:
    /**
     * The constructor of the Tx options class.
     *
     * @param value The value to set in the Tx options.
     */
    ZigbeeApsTxOptions(uint8_t value = 0);

    /**
     * Set the security enable bit of the TX options.
     *
     * @param enable True if security is enabled.
     */
    void SetSecurityEnabled(bool enable);

    /**
     * Set the use network key bit of the TX options.
     *
     * @param enable True if Network key should be used.
     */
    void SetUseNwkKey(bool enable);

    /**
     * Set the Acknowledgement required bit of the Tx options.
     *
     * @param enable True if ACK is required.
     */
    void SetAckRequired(bool enable);

    /**
     * Set the fragmentation bit of the Tx options
     *
     * @param enable True if fragmentation is allowed in the transmission.
     */
    void SetFragmentationPermitted(bool enable);

    /**
     * Set the include extended nonce bit of the Tx options
     *
     * @param enable True if the frame should include the extended nonce
     */
    void SetIncludeExtendedNonce(bool enable);

    /**
     * Show if the security enable bit of the Tx options is present.
     *
     * @return True if the bit is active
     */
    bool IsSecurityEnabled() const;

    /**
     * Show if the use network key bit of the Tx options is present.
     *
     * @return True if the bit is active
     */
    bool IsUseNwkKey() const;

    /**
     * Show if the ACK bit of the Tx options is present.
     *
     * @return True if the bit is active
     */
    bool IsAckRequired() const;

    /**
     * Show if the fragmentation permitted bit of the Tx options is present.
     *
     * @return True if the bit is active
     */
    bool IsFragmentationPermitted() const;

    /**
     * Show if the include extended nonce bit of the Tx options is present.
     *
     * @return True if the bit is active
     */
    bool IsIncludeExtendedNonce() const;

    /**
     * Get the complete bitmap containing the Tx options
     *
     * @return The Tx options bitmap.
     */
    uint8_t GetTxOptions() const;

  private:
    /**
     * Set a bit value into a position in the uint8_t representint the Tx options.
     *
     * @param pos Position to shift
     * @param value Value to set
     */
    void SetBit(int pos, bool value);

    /**
     * Get the value of the bit at the position indicated.
     *
     * @param pos The position in the uint8_t Tx options
     * @return True if the bit value was obtained
     */
    bool GetBit(int pos) const;

    uint8_t m_txOptions; //!< the bitmap representing the Tx options
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_APS_H */
