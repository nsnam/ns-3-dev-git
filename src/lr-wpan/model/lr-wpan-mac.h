/*
 * Copyright (c) 2011 The Boeing Company
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
 * Authors:
 *  Gary Pei <guangyu.pei@boeing.com>
 *  kwong yin <kwong-sang.yin@boeing.com>
 *  Tom Henderson <thomas.r.henderson@boeing.com>
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 *  Alberto Gallegos Ramonet <ramonet@fc.ritsumei.ac.jp>
 */

#ifndef LR_WPAN_MAC_H
#define LR_WPAN_MAC_H

#include "lr-wpan-fields.h"
#include "lr-wpan-phy.h"

#include <ns3/event-id.h>
#include <ns3/mac16-address.h>
#include <ns3/mac64-address.h>
#include <ns3/object.h>
#include <ns3/sequence-number.h>
#include <ns3/traced-callback.h>
#include <ns3/traced-value.h>

#include <deque>
#include <memory>

namespace ns3
{

class Packet;
class LrWpanCsmaCa;

/**
 * \defgroup lr-wpan LR-WPAN models
 *
 * This section documents the API of the IEEE 802.15.4-related models.  For a generic functional
 * description, please refer to the ns-3 manual.
 */

/**
 * \ingroup lr-wpan
 *
 * Tx options
 */
enum LrWpanTxOption
{
    TX_OPTION_NONE = 0,    //!< TX_OPTION_NONE
    TX_OPTION_ACK = 1,     //!< TX_OPTION_ACK
    TX_OPTION_GTS = 2,     //!< TX_OPTION_GTS
    TX_OPTION_INDIRECT = 4 //!< TX_OPTION_INDIRECT
};

/**
 * \ingroup lr-wpan
 *
 * MAC states
 */
enum LrWpanMacState
{
    MAC_IDLE,               //!< MAC_IDLE
    MAC_CSMA,               //!< MAC_CSMA
    MAC_SENDING,            //!< MAC_SENDING
    MAC_ACK_PENDING,        //!< MAC_ACK_PENDING
    CHANNEL_ACCESS_FAILURE, //!< CHANNEL_ACCESS_FAILURE
    CHANNEL_IDLE,           //!< CHANNEL_IDLE
    SET_PHY_TX_ON,          //!< SET_PHY_TX_ON
    MAC_GTS,                //!< MAC_GTS
    MAC_INACTIVE,           //!< MAC_INACTIVE
    MAC_CSMA_DEFERRED       //!< MAC_CSMA_DEFERRED
};

/**
 * \ingroup lr-wpan
 *
 * Superframe status
 */
enum SuperframeStatus
{
    BEACON,  //!< The Beacon transmission or reception Period
    CAP,     //!< Contention Access Period
    CFP,     //!< Contention Free Period
    INACTIVE //!< Inactive Period or unslotted CSMA-CA
};

/**
 * \ingroup lr-wpan
 *
 * Superframe type
 */
enum SuperframeType
{
    OUTGOING = 0, //!< Outgoing Superframe
    INCOMING = 1  //!< Incoming Superframe
};

/**
 * \ingroup lr-wpan
 *
 * Indicates a pending MAC primitive
 */
enum PendingPrimitiveStatus
{
    MLME_NONE = 0,      //!< No pending primitive
    MLME_START_REQ = 1, //!< Pending MLME-START.request primitive
    MLME_SCAN_REQ = 2,  //!< Pending MLME-SCAN.request primitive
    MLME_ASSOC_REQ = 3, //!< Pending MLME-ASSOCIATION.request primitive
    MLME_SYNC_REQ = 4   //!< Pending MLME-SYNC.request primitive
};

namespace TracedValueCallback
{

/**
 * \ingroup lr-wpan
 * TracedValue callback signature for LrWpanMacState.
 *
 * \param [in] oldValue original value of the traced variable
 * \param [in] newValue new value of the traced variable
 */
typedef void (*LrWpanMacState)(LrWpanMacState oldValue, LrWpanMacState newValue);

/**
 * \ingroup lr-wpan
 * TracedValue callback signature for SuperframeStatus.
 *
 * \param [in] oldValue original value of the traced variable
 * \param [in] newValue new value of the traced variable
 */
typedef void (*SuperframeStatus)(SuperframeStatus oldValue, SuperframeStatus newValue);

} // namespace TracedValueCallback

/**
 * \ingroup lr-wpan
 *
 * table 80 of 802.15.4
 */
enum LrWpanAddressMode
{
    NO_PANID_ADDR = 0,
    ADDR_MODE_RESERVED = 1,
    SHORT_ADDR = 2,
    EXT_ADDR = 3
};

/**
 * \ingroup lr-wpan
 *
 * table 83 of 802.15.4
 */
enum LrWpanAssociationStatus
{
    ASSOCIATED = 0,
    PAN_AT_CAPACITY = 1,
    PAN_ACCESS_DENIED = 2,
    ASSOCIATED_WITHOUT_ADDRESS = 0xfe,
    DISASSOCIATED = 0xff
};

/**
 * \ingroup lr-wpan
 *
 * Table 30 of IEEE 802.15.4-2011
 */
enum LrWpanMlmeScanType
{
    MLMESCAN_ED = 0x00,
    MLMESCAN_ACTIVE = 0x01,
    MLMESCAN_PASSIVE = 0x02,
    MLMESCAN_ORPHAN = 0x03
};

/**
 * \ingroup lr-wpan
 *
 * Table 42 of 802.15.4-2006
 */
enum LrWpanMcpsDataConfirmStatus
{
    IEEE_802_15_4_SUCCESS = 0,
    IEEE_802_15_4_TRANSACTION_OVERFLOW = 1,
    IEEE_802_15_4_TRANSACTION_EXPIRED = 2,
    IEEE_802_15_4_CHANNEL_ACCESS_FAILURE = 3,
    IEEE_802_15_4_INVALID_ADDRESS = 4,
    IEEE_802_15_4_INVALID_GTS = 5,
    IEEE_802_15_4_NO_ACK = 6,
    IEEE_802_15_4_COUNTER_ERROR = 7,
    IEEE_802_15_4_FRAME_TOO_LONG = 8,
    IEEE_802_15_4_UNAVAILABLE_KEY = 9,
    IEEE_802_15_4_UNSUPPORTED_SECURITY = 10,
    IEEE_802_15_4_INVALID_PARAMETER = 11
};

/**
 * \ingroup lr-wpan
 *
 * Table 35 of IEEE 802.15.4-2011
 */
enum LrWpanMlmeStartConfirmStatus
{
    MLMESTART_SUCCESS = 0,
    MLMESTART_NO_SHORT_ADDRESS = 1,
    MLMESTART_SUPERFRAME_OVERLAP = 2,
    MLMESTART_TRACKING_OFF = 3,
    MLMESTART_INVALID_PARAMETER = 4,
    MLMESTART_COUNTER_ERROR = 5,
    MLMESTART_FRAME_TOO_LONG = 6,
    MLMESTART_UNAVAILABLE_KEY = 7,
    MLMESTART_UNSUPPORTED_SECURITY = 8,
    MLMESTART_CHANNEL_ACCESS_FAILURE = 9
};

/**
 * \ingroup lr-wpan
 *
 * Table 31 of IEEE 802.15.4-2011
 */
enum LrWpanMlmeScanConfirmStatus
{
    MLMESCAN_SUCCESS = 0,
    MLMESCAN_LIMIT_REACHED = 1,
    MLMESCAN_NO_BEACON = 2,
    MLMESCAN_SCAN_IN_PROGRESS = 3,
    MLMESCAN_COUNTER_ERROR = 4,
    MLMESCAN_FRAME_TOO_LONG = 5,
    MLMESCAN_UNAVAILABLE_KEY = 6,
    MLMESCAN_UNSUPPORTED_SECURITY = 7,
    MLMESCAN_INVALID_PARAMETER = 8
};

/**
 * \ingroup lr-wpan
 *
 * Table 12 of IEEE 802.15.4-2011
 */
enum LrWpanMlmeAssociateConfirmStatus
{
    MLMEASSOC_SUCCESS = 0,
    MLMEASSOC_FULL_CAPACITY = 1,
    MLMEASSOC_ACCESS_DENIED = 2,
    MLMEASSOC_CHANNEL_ACCESS_FAILURE = 3,
    MLMEASSOC_NO_ACK = 4,
    MLMEASSOC_NO_DATA = 5,
    MLMEASSOC_COUNTER_ERROR = 6,
    MLMEASSOC_FRAME_TOO_LONG = 7,
    MLMEASSOC_UNSUPPORTED_LEGACY = 8,
    MLMEASSOC_INVALID_PARAMETER = 9
};

/**
 * \ingroup lr-wpan
 *
 * Table 37 of IEEE 802.15.4-2011
 */
enum LrWpanSyncLossReason
{
    MLMESYNCLOSS_PAN_ID_CONFLICT = 0,
    MLMESYNCLOSS_REALIGMENT = 1,
    MLMESYNCLOSS_BEACON_LOST = 2,
    MLMESYNCLOSS_SUPERFRAME_OVERLAP = 3
};

/**
 * \ingroup lr-wpan
 *
 * Table 18 of IEEE 802.15.4-2011
 */
enum LrWpanMlmeCommStatus
{
    MLMECOMMSTATUS_SUCCESS = 0,
    MLMECOMMSTATUS_TRANSACTION_OVERFLOW = 1,
    MLMECOMMSTATUS_TRANSACTION_EXPIRED = 2,
    MLMECOMMSTATUS_CHANNEL_ACCESS_FAILURE = 3,
    MLMECOMMSTATUS_NO_ACK = 4,
    MLMECOMMSTATUS_COUNTER_ERROR = 5,
    MLMECOMMSTATUS_FRAME_TOO_LONG = 6,
    MLMECOMMSTATUS_INVALID_PARAMETER = 7
};

/**
 * \ingroup lr-wpan
 *
 * Table 39 of IEEE 802.15.4-2011
 */
enum LrWpanMlmePollConfirmStatus
{
    MLMEPOLL_SUCCESS = 0,
    MLMEPOLL_CHANNEL_ACCESS_FAILURE = 2,
    MLMEPOLL_NO_ACK = 3,
    MLMEPOLL_NO_DATA = 4,
    MLMEPOLL_COUNTER_ERROR = 5,
    MLMEPOLL_FRAME_TOO_LONG = 6,
    MLMEPOLL_UNAVAILABLE_KEY = 7,
    MLMEPOLL_UNSUPPORTED_SECURITY = 8,
    MLMEPOLL_INVALID_PARAMETER = 9
};

/**
 * \ingroup lr-wpan
 *
 * Table 33 of IEEE 802.15.4-2011
 */
enum LrWpanMlmeSetConfirmStatus
{
    MLMESET_SUCCESS = 0,
    MLMESET_READ_ONLY = 1,
    MLMESET_UNSUPPORTED_ATTRIBUTE = 2,
    MLMESET_INVALID_INDEX = 3,
    MLMESET_INVALID_PARAMETER = 4
};

/**
 * \ingroup lr-wpan
 *
 * Table 20 of IEEE 802.15.4-2011
 */
enum LrWpanMlmeGetConfirmStatus
{
    MLMEGET_SUCCESS = 0,
    MLMEGET_UNSUPPORTED_ATTRIBUTE = 1
};

/**
 * \ingroup lr-wpan
 *
 * IEEE802.15.4-2011 MAC PIB Attribute Identifiers Table 52 in section 6.4.2
 *
 */
enum LrWpanMacPibAttributeIdentifier
{
    macBeaconPayload = 0,
    macBeaconPayloadLength = 1,
    macShortAddress = 2,
    macExtendedAddress = 3,
    macPanId = 4,
    unsupported = 255
    // TODO: complete other MAC pib attributes
};

/**
 * \ingroup lr-wpan
 *
 * IEEE802.15.4-2011 PHY PIB Attributes Table 52 in section 6.4.2
 */
struct LrWpanMacPibAttributes : public SimpleRefCount<LrWpanMacPibAttributes>
{
    Ptr<Packet> macBeaconPayload;      //!< The contents of the beacon payload.
    uint8_t macBeaconPayloadLength{0}; //!< The length in octets of the beacon payload.
    Mac16Address macShortAddress;      //!< The 16 bit mac short address
    Mac64Address macExtendedAddress;   //!< The EUI-64 bit address
    uint16_t macPanId;                 //!< The identifier of the PAN
    // TODO: complete other MAC pib attributes
};

/**
 * \ingroup lr-wpan
 *
 * PAN Descriptor, Table 17 IEEE 802.15.4-2011
 */
struct PanDescriptor
{
    LrWpanAddressMode m_coorAddrMode{SHORT_ADDR}; //!< The coordinator addressing mode corresponding
                                                  //!< to the received beacon frame.
    uint16_t m_coorPanId{0xffff};                 //!< The PAN ID of the coordinator as specified in
                                                  //!<  the received beacon frame.
    Mac16Address m_coorShortAddr; //!< The coordinator short address as specified in the coordinator
                                  //!< address mode.
    Mac64Address m_coorExtAddr;   //!< The coordinator extended address as specified in the
                                  //!< coordinator address mode.
    uint8_t m_logCh{11};          //!< The current channel number occupied by the network.
    uint8_t m_logChPage{0};       //!< The current channel page occupied by the network.
    SuperframeField m_superframeSpec; //!< The superframe specification as specified in the received
                                      //!< beacon frame.
    bool m_gtsPermit{false};          //!< TRUE if the beacon is from the PAN coordinator
                                      //!< that is accepting GTS requests.
    uint8_t m_linkQuality{0};         //!< The LQI at which the network beacon was received.
                                      //!< Lower values represent lower LQI.
    Time m_timeStamp; //!< Beacon frame reception time. Used as Time data type in ns-3 to avoid
                      //!< precision problems.
};

/**
 * \ingroup lr-wpan
 *
 * MCPS-DATA.request params. See 7.1.1.1
 */
struct McpsDataRequestParams
{
    LrWpanAddressMode m_srcAddrMode{SHORT_ADDR}; //!< Source address mode
    LrWpanAddressMode m_dstAddrMode{SHORT_ADDR}; //!< Destination address mode
    uint16_t m_dstPanId{0};                      //!< Destination PAN identifier
    Mac16Address m_dstAddr;                      //!< Destination address
    Mac64Address m_dstExtAddr;                   //!< Destination extended address
    uint8_t m_msduHandle{0};                     //!< MSDU handle
    uint8_t m_txOptions{0};                      //!< Tx Options (bitfield)
};

/**
 * \ingroup lr-wpan
 *
 * MCPS-DATA.confirm params. See 7.1.1.2
 */
struct McpsDataConfirmParams
{
    uint8_t m_msduHandle{0}; //!< MSDU handle
    LrWpanMcpsDataConfirmStatus m_status{
        IEEE_802_15_4_INVALID_PARAMETER}; //!< The status of the last MSDU transmission
};

/**
 * \ingroup lr-wpan
 *
 * MCPS-DATA.indication params. See 7.1.1.3
 */
struct McpsDataIndicationParams
{
    uint8_t m_srcAddrMode{SHORT_ADDR}; //!< Source address mode
    uint16_t m_srcPanId{0};            //!< Source PAN identifier
    Mac16Address m_srcAddr;            //!< Source address
    Mac64Address m_srcExtAddr;         //!< Source extended address
    uint8_t m_dstAddrMode{SHORT_ADDR}; //!< Destination address mode
    uint16_t m_dstPanId{0};            //!< Destination PAN identifier
    Mac16Address m_dstAddr;            //!< Destination address
    Mac64Address m_dstExtAddr;         //!< Destination extended address
    uint8_t m_mpduLinkQuality{0};      //!< LQI value measured during reception of the MPDU
    uint8_t m_dsn{0};                  //!< The DSN of the received data frame
};

/**
 * \ingroup lr-wpan
 *
 * MLME-ASSOCIATE.indication params. See 802.15.4-2011 6.2.2.2.
 */
struct MlmeAssociateIndicationParams
{
    Mac64Address m_extDevAddr;      //!< The extended address of the device requesting association
    CapabilityField capabilityInfo; //!< The operational capabilities of
                                    //!< the device requesting association.
    uint8_t lqi{0}; //!< The link quality indicator of the received associate request command
                    //!< (Not officially supported in the standard but found in implementations)
};

/**
 * \ingroup lr-wpan
 *
 * MLME-ASSOCIATE.response params. See 802.15.4-2011 6.2.2.3.
 */
struct MlmeAssociateResponseParams
{
    Mac64Address m_extDevAddr;     //!< The extended address of the device requesting association
    Mac16Address m_assocShortAddr; //!< The short address allocated by the coordinator on successful
                                   //!< assoc. FF:FF = Unsuccessful
    LrWpanAssociationStatus m_status{DISASSOCIATED}; //!< The status of the association attempt (As
                                                     //!< defined on Table 83 IEEE 802.15.4-2006)
};

/**
 * \ingroup lr-wpan
 *
 * MLME-START.request params. See 802.15.4-2011  Section 6.2.12.1
 */
struct MlmeStartRequestParams
{
    uint16_t m_PanId{0};     //!< Pan Identifier used by the device.
    uint8_t m_logCh{11};     //!< Logical channel on which to start using the
                             //!< new superframe configuration.
    uint32_t m_logChPage{0}; //!< Logical channel page on which to start using the
                             //!< new superframe configuration.
    uint32_t m_startTime{0}; //!< Time at which to begin transmitting beacons (Used by Coordinator
                             //!< not PAN Coordinators). The time is specified in symbols.
    uint8_t m_bcnOrd{15};    //!< Beacon Order, Used to calculate the beacon interval, a value of 15
                             //!< indicates no periodic beacons will be transmitted.
    uint8_t m_sfrmOrd{15};   //!< Superframe Order, indicates the length of the CAP in time slots.
    bool m_panCoor{false};   //!< On true this device will become coordinator.
    bool m_battLifeExt{false}; //!< Flag indicating whether or not the Battery life extension (BLE)
                               //!< features are used.
    bool m_coorRealgn{false};  //!< True if a realignment request command is to be transmitted prior
                               //!< changing the superframe.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-SYNC.request params. See 802.15.4-2011  Section 6.2.13.1
 */
struct MlmeSyncRequestParams
{
    uint8_t m_logCh{11};    //!< The channel number on which to attempt coordinator synchronization.
    bool m_trackBcn{false}; //!< True if the mlme sync with the next beacon and attempts to track
                            //!< future beacons. False if mlme sync only the next beacon.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-POLL.request params. See 802.15.4-2011  Section 6.2.14.1
 */
struct MlmePollRequestParams
{
    LrWpanAddressMode m_coorAddrMode{
        SHORT_ADDR}; //!< The addressing mode of the coordinator to which the pool is intended.
    uint16_t m_coorPanId{0};      //!< The PAN id of the coordinator to which the poll is intended.
    Mac16Address m_coorShortAddr; //!< Coordinator short address.
    Mac64Address m_coorExtAddr;   //!< Coordinator extended address.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-SCAN.request params. See IEEE 802.15.4-2011  Section 6.2.10.1 Table 30
 */
struct MlmeScanRequestParams
{
    LrWpanMlmeScanType m_scanType{MLMESCAN_PASSIVE}; //!< Indicates the type of scan performed as
                                                     //!< described in IEEE 802.15.4-2011 (5.1.2.1).
    uint32_t m_scanChannels{0x7FFF800};              //!< The channel numbers to be scanned.
                                                     //!< Default: (0x7FFF800 = Ch11-Ch26)
                                                     //!< 27 LSB (b0,b1,...,b26) = channels
    uint8_t m_scanDuration{14}; //!< The factor (0-14) used to calculate the length of time
                                //!< to spend scanning.
                                //!< scanDurationSymbols =
                                //!< [aBaseSuperframeDuration * (2^m_scanDuration + 1)].
    uint32_t m_chPage{0};       //!< The channel page on which to perform scan.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-SCAN.confirm params. See IEEE 802.15.4-2011 Section 6.2.10.2
 */
struct MlmeScanConfirmParams
{
    LrWpanMlmeScanConfirmStatus m_status{MLMESCAN_INVALID_PARAMETER}; //!< The status the request.
    LrWpanMlmeScanType m_scanType{MLMESCAN_PASSIVE}; //!< Indicates the type of scan
                                                     //!<  performed (ED,ACTIVE,PASSIVE,ORPHAN).
    uint32_t m_chPage{0};                 //!< The channel page on which the scan was performed.
    std::vector<uint8_t> m_unscannedCh;   //!< A list of channels given in the request which
                                          //!<  were not scanned (Not valid for ED scans).
    uint8_t m_resultListSize{0};          //!< The number of elements returned in the appropriate
                                          //!<  result list. (Not valid for Orphan scan).
    std::vector<uint8_t> m_energyDetList; //!< A list of energy measurements, one for each
                                          //!< channel searched during ED scan
                                          //!< (Not valid for Active, Passive or Orphan Scans)
    std::vector<PanDescriptor> m_panDescList; //!< A list of PAN descriptor, one for each beacon
                                              //!< found (Not valid for ED and Orphan scans).
};

/**
 * \ingroup lr-wpan
 *
 * MLME-ASSOCIATE.request params. See 802.15.4-2011  Section 6.2.2.1
 */
struct MlmeAssociateRequestParams
{
    uint8_t m_chNum{11};  //!< The channel number on which to attempt association.
    uint32_t m_chPage{0}; //!< The channel page on which to attempt association.
    uint8_t m_coordAddrMode{
        SHORT_ADDR}; //!< The coordinator addressing mode for this primitive and subsequent MPDU.
    uint16_t m_coordPanId{0}; //!< The identifier of the PAN with which to associate.
    Mac16Address
        m_coordShortAddr; //!< The short address of the coordinator with which to associate.
    Mac64Address
        m_coordExtAddr; //!< The extended address of the coordinator with which to associate.
    CapabilityField
        m_capabilityInfo; //!< Specifies the operational capabilities of the associating device.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-ASSOCIATE.confirm params. See 802.15.4-2011  Section 6.2.2.4
 */
struct MlmeAssociateConfirmParams
{
    Mac16Address m_assocShortAddr; //!< The short address used in the association request
    LrWpanMlmeAssociateConfirmStatus m_status{
        MLMEASSOC_INVALID_PARAMETER}; //!< The status of a MLME-associate.request
};

/**
 * \ingroup lr-wpan
 *
 * MLME-START.confirm params. See  802.15.4-2011   Section 6.2.12.2
 */
struct MlmeStartConfirmParams
{
    LrWpanMlmeStartConfirmStatus m_status{
        MLMESTART_INVALID_PARAMETER}; //!< The status of a MLME-start.request
};

/**
 * \ingroup lr-wpan
 *
 * MLME-BEACON-NOTIFY.indication params. See  802.15.4-2011   Section 6.2.4.1, Table 16
 */
struct MlmeBeaconNotifyIndicationParams
{
    uint8_t m_bsn{0};              //!< The beacon sequence number.
    PanDescriptor m_panDescriptor; //!< The PAN descriptor for the received beacon.
    uint32_t m_sduLength{0};       //!< The number of octets contained in the beacon payload.
    Ptr<Packet> m_sdu;             //!< The set of octets comprising the beacon payload.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-SYNC-LOSS.indication params. See  802.15.4-2011   Section 6.2.13.2, Table 37
 */
struct MlmeSyncLossIndicationParams
{
    LrWpanSyncLossReason m_lossReason{
        MLMESYNCLOSS_PAN_ID_CONFLICT}; //!< The reason for the lost of synchronization.
    uint16_t m_panId{0}; //!< The PAN identifier with which the device lost synchronization or to
                         //!< which it was realigned.
    uint8_t m_logCh{11}; //!< The channel number on which the device lost synchronization or to
                         //!< which it was realigned.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-COMM-STATUS.indication params. See  802.15.4-2011   Section 6.2.4.2 Table 18
 */
struct MlmeCommStatusIndicationParams
{
    uint16_t m_panId{0}; //!< The PAN identifier of the device from which the frame was received or
                         //!< to which the frame was being sent.
    uint8_t m_srcAddrMode{SHORT_ADDR}; //!< The source addressing mode for this primitive
    Mac16Address m_srcShortAddr; //!< The short address of the entity from which the frame causing
                                 //!< the error originated.
    Mac64Address m_srcExtAddr; //!< The extended address of the entity from which the frame causing
                               //!< the error originated.
    uint8_t m_dstAddrMode{SHORT_ADDR}; //!< The destination addressing mode for this primitive.
    Mac16Address m_dstShortAddr;       //!< The short address of the device for
                                       //!< which the frame was intended.
    Mac64Address m_dstExtAddr;         //!< The extended address of the device for
                                       //!< which the frame was intended.
    LrWpanMlmeCommStatus m_status{MLMECOMMSTATUS_INVALID_PARAMETER}; //!< The communication status
};

/**
 * \ingroup lr-wpan
 *
 * MLME-ORPHAN.indication params. See  802.15.4-2011   Section 6.2.7.1
 */
struct MlmeOrphanIndicationParams
{
    Mac64Address m_orphanAddr; //!< The address of the orphaned device.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-ORPHAN.response params. See  802.15.4-2011   Section 6.2.7.2
 */
struct MlmeOrphanResponseParams
{
    Mac64Address m_orphanAddr; //!< The address of the orphaned device.
    Mac16Address m_shortAddr;  //!< The short address allocated.
    bool m_assocMember{false}; //!< T = allocated with this coord |  F = otherwise
};

/**
 * \ingroup lr-wpan
 *
 * MLME-START.confirm params. See  802.15.4-2011   Section 6.2.14.2
 */
struct MlmePollConfirmParams
{
    LrWpanMlmePollConfirmStatus m_status{
        MLMEPOLL_INVALID_PARAMETER}; //!< The confirmation status resulting from a
                                     //!< MLME-poll.request.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-SET.confirm params. See  802.15.4-2011   Section 6.2.11.2
 */
struct MlmeSetConfirmParams
{
    LrWpanMlmeSetConfirmStatus m_status{MLMESET_UNSUPPORTED_ATTRIBUTE}; //!< The result of
                                                                        //!< the request to write
                                                                        //!< the PIB attribute.
    LrWpanMacPibAttributeIdentifier id; //!< The id of the PIB attribute that was written.
};

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a McpsDataRequest has been called from
 * the higher layer.  It returns a status of the outcome of the
 * transmission request
 */
typedef Callback<void, McpsDataConfirmParams> McpsDataConfirmCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a Mcps has successfully received a
 *  frame and wants to deliver it to the higher layer.
 *
 *  \todo for now, we do not deliver all of the parameters in section
 *  802.15.4-2006 7.1.1.3.1 but just send up the packet.
 */
typedef Callback<void, McpsDataIndicationParams, Ptr<Packet>> McpsDataIndicationCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a MlmeStartRequest has been called from
 * the higher layer.  It returns a status of the outcome of the
 * transmission request
 */
typedef Callback<void, MlmeStartConfirmParams> MlmeStartConfirmCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a Mlme has successfully received a
 *  beacon frame and wants to deliver it to the higher layer.
 *
 *  \todo for now, we do not deliver all of the parameters in section
 *  802.15.4-2006 6.2.4.1 but just send up the packet.
 */
typedef Callback<void, MlmeBeaconNotifyIndicationParams> MlmeBeaconNotifyIndicationCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called to indicate the loss of synchronization with
 * a coordinator.
 *
 * \todo for now, we do not deliver all of the parameters in section
 *  See IEEE 802.15.4-2011 6.2.13.2.
 */
typedef Callback<void, MlmeSyncLossIndicationParams> MlmeSyncLossIndicationCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a Mlme-Poll.Request has been called from
 * the higher layer.  It returns a status of the outcome of the
 * transmission request
 */
typedef Callback<void, MlmePollConfirmParams> MlmePollConfirmCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a MlmeScanRequest has been called from
 * the higher layer.  It returns a status of the outcome of the scan.
 */
typedef Callback<void, MlmeScanConfirmParams> MlmeScanConfirmCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a MlmeAssociateRequest has been called from
 * the higher layer. It returns a status of the outcome of the
 * association request
 */
typedef Callback<void, MlmeAssociateConfirmParams> MlmeAssociateConfirmCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a Mlme has successfully received a command
 *  frame and wants to deliver it to the higher layer.
 *
 *  Security related parameters and not handle.
 *  See 802.15.4-2011 6.2.2.2.
 */
typedef Callback<void, MlmeAssociateIndicationParams> MlmeAssociateIndicationCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called by the MLME and issued to its next higher layer following
 * a transmission instigated through a response primitive.
 *
 *  Security related parameters and not handle.
 *  See 802.15.4-2011 6.2.4.2
 */
typedef Callback<void, MlmeCommStatusIndicationParams> MlmeCommStatusIndicationCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called by the MLME and issued to its next higher layer following
 * the reception of a orphan notification.
 *
 *  Security related parameters and not handle.
 *  See 802.15.4-2011 6.2.7.1
 */
typedef Callback<void, MlmeOrphanIndicationParams> MlmeOrphanIndicationCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a MlmeSetRequest has been called from
 * the higher layer to set a PIB. It returns a status of the outcome of the
 * write attempt.
 */
typedef Callback<void, MlmeSetConfirmParams> MlmeSetConfirmCallback;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a MlmeGetRequest has been called from
 * the higher layer to get a PIB. It returns a status of the outcome of the
 * write attempt.
 */
typedef Callback<void,
                 LrWpanMlmeGetConfirmStatus,
                 LrWpanMacPibAttributeIdentifier,
                 Ptr<LrWpanMacPibAttributes>>
    MlmeGetConfirmCallback;

/**
 * \ingroup lr-wpan
 *
 * Class that implements the LR-WPAN MAC state machine
 */
class LrWpanMac : public Object
{
  public:
    /**
     * Get the type ID.
     *
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Default constructor.
     */
    LrWpanMac();
    ~LrWpanMac() override;

    /**
     * Check if the receiver will be enabled when the MAC is idle.
     *
     * \return true, if the receiver is enabled during idle periods, false otherwise
     */
    bool GetRxOnWhenIdle() const;

    /**
     * Set if the receiver should be enabled when the MAC is idle.
     *
     * \param rxOnWhenIdle set to true to enable the receiver during idle periods
     */
    void SetRxOnWhenIdle(bool rxOnWhenIdle);

    // XXX these setters will become obsolete if we use the attribute system
    /**
     * Set the short address of this MAC.
     *
     * \param address the new address
     */
    void SetShortAddress(Mac16Address address);

    /**
     * Get the short address of this MAC.
     *
     * \return the short address
     */
    Mac16Address GetShortAddress() const;

    /**
     * Set the extended address of this MAC.
     *
     * \param address the new address
     */
    void SetExtendedAddress(Mac64Address address);

    /**
     * Get the extended address of this MAC.
     *
     * \return the extended address
     */
    Mac64Address GetExtendedAddress() const;

    /**
     * Set the PAN id used by this MAC.
     *
     * \param panId the new PAN id.
     */
    void SetPanId(uint16_t panId);

    /**
     * Get the PAN id used by this MAC.
     *
     * \return the PAN id.
     */
    uint16_t GetPanId() const;

    /**
     * Get the coordinator short address currently associated to this device.
     *
     * \return The coordinator short address
     */
    Mac16Address GetCoordShortAddress() const;

    /**
     * Get the coordinator extended address currently associated to this device.
     *
     * \return The coordinator extended address
     */
    Mac64Address GetCoordExtAddress() const;

    /**
     * IEEE 802.15.4-2006, section 7.1.1.1
     * MCPS-DATA.request
     * Request to transfer a MSDU.
     *
     * \param params the request parameters
     * \param p the packet to be transmitted
     */
    void McpsDataRequest(McpsDataRequestParams params, Ptr<Packet> p);

    /**
     * IEEE 802.15.4-2006, section 7.1.14.1
     * MLME-START.request
     * Request to allow a PAN coordinator to initiate
     * a new PAN or beginning a new superframe configuration.
     *
     * \param params the request parameters
     */
    void MlmeStartRequest(MlmeStartRequestParams params);

    /**
     * IEEE 802.15.4-2011, section 6.2.10.1
     * MLME-SCAN.request
     * Request primitive used to initiate a channel scan over a given list of channels.
     *
     * \param params the scan request parameters
     */
    void MlmeScanRequest(MlmeScanRequestParams params);

    /**
     * IEEE 802.15.4-2011, section 6.2.2.1
     * MLME-ASSOCIATE.request
     * Request primitive used by a device to request an association with
     * a coordinator.
     *
     * \param params the request parameters
     */
    void MlmeAssociateRequest(MlmeAssociateRequestParams params);

    /**
     * IEEE 802.15.4-2011, section 6.2.2.3
     * MLME-ASSOCIATE.response
     * Primitive used to initiate a response to an MLME-ASSOCIATE.indication
     * primitive.
     *
     * \param params the associate response parameters
     */
    void MlmeAssociateResponse(MlmeAssociateResponseParams params);

    /**
     * IEEE 802.15.4-2011, section 6.2.7.2
     * MLME-ORPHAN.response
     * Primitive used to initiatte a response to an MLME-ORPHAN.indication
     * primitive.
     *
     * \param params the orphan response parameters
     */
    void MlmeOrphanResponse(MlmeOrphanResponseParams params);

    /**
     * IEEE 802.15.4-2011, section 6.2.13.1
     * MLME-SYNC.request
     * Request to synchronize with the coordinator by acquiring and,
     * if specified, tracking beacons.
     *
     * \param params the request parameters
     */
    void MlmeSyncRequest(MlmeSyncRequestParams params);

    /**
     * IEEE 802.15.4-2011, section 6.2.14.2
     * MLME-POLL.request
     * Prompts the device to request data from the coordinator.
     *
     * \param params the request parameters
     */
    void MlmePollRequest(MlmePollRequestParams params);

    /**
     * IEEE 802.15.4-2011, section 6.2.11.1
     * MLME-SET.request
     * Attempts to write the given value to the indicated PIB attribute.
     *
     * \param id the attributed identifier
     * \param attribute the attribute value
     */
    void MlmeSetRequest(LrWpanMacPibAttributeIdentifier id, Ptr<LrWpanMacPibAttributes> attribute);

    /**
     * IEEE 802.15.4-2011, section 6.2.5.1
     * MLME-GET.request
     * Request information about a given PIB attribute.
     *
     * \param id the attribute identifier
     */
    void MlmeGetRequest(LrWpanMacPibAttributeIdentifier id);

    /**
     * Set the CSMA/CA implementation to be used by the MAC.
     *
     * \param csmaCa the CSMA/CA implementation
     */
    void SetCsmaCa(Ptr<LrWpanCsmaCa> csmaCa);

    /**
     * Set the underlying PHY for the MAC.
     *
     * \param phy the PHY
     */
    void SetPhy(Ptr<LrWpanPhy> phy);

    /**
     * Get the underlying PHY of the MAC.
     *
     * \return the PHY
     */
    Ptr<LrWpanPhy> GetPhy();

    /**
     * Set the callback for the indication of an incoming data packet.
     * The callback implements MCPS-DATA.indication SAP of IEEE 802.15.4-2006,
     * section 7.1.1.3.
     *
     * \param c the callback
     */
    void SetMcpsDataIndicationCallback(McpsDataIndicationCallback c);

    /**
     * Set the callback for the indication of an incoming associate request command.
     * The callback implements MLME-ASSOCIATE.indication SAP of IEEE 802.15.4-2011,
     * section 6.2.2.2.
     *
     * \param c the callback
     */
    void SetMlmeAssociateIndicationCallback(MlmeAssociateIndicationCallback c);

    /**
     * Set the callback for the indication to a response primitive.
     * The callback implements MLME-COMM-STATUS.indication SAP of IEEE 802.15.4-2011,
     * section 6.2.4.2.
     *
     * \param c the callback
     */
    void SetMlmeCommStatusIndicationCallback(MlmeCommStatusIndicationCallback c);

    /**
     * Set the callback for the indication to the reception of an orphan notification.
     * The callback implements MLME-ORPHAN.indication SAP of IEEE 802.15.4-2011,
     * section 6.2.7.1.
     *
     * \param c the callback
     */
    void SetMlmeOrphanIndicationCallback(MlmeOrphanIndicationCallback c);

    /**
     * Set the callback for the confirmation of a data transmission request.
     * The callback implements MCPS-DATA.confirm SAP of IEEE 802.15.4-2006,
     * section 7.1.1.2.
     *
     * \param c the callback
     */
    void SetMcpsDataConfirmCallback(McpsDataConfirmCallback c);

    /**
     * Set the callback for the confirmation of a data transmission request.
     * The callback implements MLME-START.confirm SAP of IEEE 802.15.4-2006,
     * section 7.1.14.2.
     *
     * \param c the callback
     */
    void SetMlmeStartConfirmCallback(MlmeStartConfirmCallback c);

    /**
     * Set the callback for the confirmation of a data transmission request.
     * The callback implements MLME-SCAN.confirm SAP of IEEE 802.15.4-2011,
     * section 6.2.10.2.
     *
     * \param c the callback
     */
    void SetMlmeScanConfirmCallback(MlmeScanConfirmCallback c);

    /**
     * Set the callback for the confirmation of a data transmission request.
     * The callback implements MLME-ASSOCIATE.confirm SAP of IEEE 802.15.4-2011,
     * section 6.2.2.4.
     *
     * \param c the callback
     */
    void SetMlmeAssociateConfirmCallback(MlmeAssociateConfirmCallback c);

    /**
     * Set the callback for the indication of an incoming beacon packet.
     * The callback implements MLME-BEACON-NOTIFY.indication SAP of IEEE 802.15.4-2011,
     * section 6.2.4.1.
     *
     * \param c the callback
     */
    void SetMlmeBeaconNotifyIndicationCallback(MlmeBeaconNotifyIndicationCallback c);

    /**
     * Set the callback for the loss of synchronization with a coordinator.
     * The callback implements MLME-BEACON-NOTIFY.indication SAP of IEEE 802.15.4-2011,
     * section 6.2.13.2.
     *
     * \param c the callback
     */
    void SetMlmeSyncLossIndicationCallback(MlmeSyncLossIndicationCallback c);

    /**
     * Set the callback for the confirmation of a data transmission request.
     * The callback implements MLME-POLL.confirm SAP of IEEE 802.15.4-2011,
     * section 6.2.14.2
     *
     * \param c the callback
     */
    void SetMlmePollConfirmCallback(MlmePollConfirmCallback c);

    /**
     * Set the callback for the confirmation of an attempt to write an attribute.
     * The callback implements MLME-SET.confirm SAP of IEEE 802.15.4-2011,
     * section 6.2.11.2
     *
     * \param c the callback
     */
    void SetMlmeSetConfirmCallback(MlmeSetConfirmCallback c);

    /**
     * Set the callback for the confirmation of an attempt to read an attribute.
     * The callback implements MLME-GET.confirm SAP of IEEE 802.15.4-2011,
     * section 6.2.5.2
     *
     *\param c the callback
     */
    void SetMlmeGetConfirmCallback(MlmeGetConfirmCallback c);

    // interfaces between MAC and PHY

    /**
     * IEEE 802.15.4-2006 section 6.2.1.3
     * PD-DATA.indication
     * Indicates the transfer of an MPDU from PHY to MAC (receiving)
     * \param psduLength number of bytes in the PSDU
     * \param p the packet to be transmitted
     * \param lqi Link quality (LQI) value measured during reception of the PPDU
     */
    void PdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi);

    /**
     * IEEE 802.15.4-2006 section 6.2.1.2
     * Confirm the end of transmission of an MPDU to MAC
     * \param status to report to MAC
     *        PHY PD-DATA.confirm status
     */
    void PdDataConfirm(LrWpanPhyEnumeration status);

    /**
     * IEEE 802.15.4-2006 section 6.2.2.2
     * PLME-CCA.confirm status
     * \param status TRX_OFF, BUSY or IDLE
     */
    void PlmeCcaConfirm(LrWpanPhyEnumeration status);

    /**
     * IEEE 802.15.4-2006 section 6.2.2.4
     * PLME-ED.confirm status and energy level
     * \param status SUCCESS, TRX_OFF or TX_ON
     * \param energyLevel 0x00-0xff ED level for the channel
     */
    void PlmeEdConfirm(LrWpanPhyEnumeration status, uint8_t energyLevel);

    /**
     * IEEE 802.15.4-2006 section 6.2.2.6
     * PLME-GET.confirm
     * Get attributes per definition from Table 23 in section 6.4.2
     * \param status SUCCESS or UNSUPPORTED_ATTRIBUTE
     * \param id the attributed identifier
     * \param attribute the attribute value
     */
    void PlmeGetAttributeConfirm(LrWpanPhyEnumeration status,
                                 LrWpanPibAttributeIdentifier id,
                                 Ptr<LrWpanPhyPibAttributes> attribute);

    /**
     * IEEE 802.15.4-2006 section 6.2.2.8
     * PLME-SET-TRX-STATE.confirm
     * Set PHY state
     * \param status in RX_ON,TRX_OFF,FORCE_TRX_OFF,TX_ON
     */
    void PlmeSetTRXStateConfirm(LrWpanPhyEnumeration status);

    /**
     * IEEE 802.15.4-2006 section 6.2.2.10
     * PLME-SET.confirm
     * Set attributes per definition from Table 23 in section 6.4.2
     * \param status SUCCESS, UNSUPPORTED_ATTRIBUTE, INVALID_PARAMETER, or READ_ONLY
     * \param id the attributed identifier
     */
    void PlmeSetAttributeConfirm(LrWpanPhyEnumeration status, LrWpanPibAttributeIdentifier id);

    /**
     * CSMA-CA algorithm calls back the MAC after executing channel assessment.
     *
     * \param macState indicate BUSY or IDLE channel condition
     */
    void SetLrWpanMacState(LrWpanMacState macState);

    /**
     * Get the current association status.
     *
     * \return current association status
     */
    LrWpanAssociationStatus GetAssociationStatus() const;

    /**
     * Set the current association status.
     *
     * \param status new association status
     */
    void SetAssociationStatus(LrWpanAssociationStatus status);

    /**
     * Set the max size of the transmit queue.
     *
     * \param queueSize The transmit queue size.
     */
    void SetTxQMaxSize(uint32_t queueSize);

    /**
     * Set the max size of the indirect transmit queue (Pending Transaction list)
     *
     * \param queueSize The indirect transmit queue size.
     */
    void SetIndTxQMaxSize(uint32_t queueSize);

    // MAC PIB attributes

    /**
     * The time that the device transmitted its last beacon frame.
     * It also indicates the start of the Active Period in the Outgoing superframe.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    Time m_macBeaconTxTime;

    /**
     * The time that the device received its last bit of the beacon frame.
     * It does not indicate the start of the Active Period in the Incoming superframe.
     * Not explicitly listed by the standard but its use is implied.
     * Its purpose is somehow similar to m_macBeaconTxTime
     */
    Time m_macBeaconRxTime;

    /**
     * The maximum time, in multiples of aBaseSuperframeDuration, a device
     * shall wait for a response command frame to be available following a
     * request command frame.
     */
    uint64_t m_macResponseWaitTime;

    /**
     * The maximum wait time for an association response command after the reception
     * of data request command ACK during the association process. Not explicitly
     * listed by the standard but its use is required for a device to react to the lost
     * of the association response (failure of the association: NO_DATA)
     */
    uint64_t m_assocRespCmdWaitTime;

    /**
     * The short address of the coordinator through which the device is
     * associated.
     * 0xFFFF indicates this value is unknown.
     * 0xFFFE indicates the coordinator is only using its extended address.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    Mac16Address m_macCoordShortAddress;

    /**
     * The extended address of the coordinator through which the device
     * is associated.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    Mac64Address m_macCoordExtendedAddress;

    /**
     * Symbol boundary is same as m_macBeaconTxTime.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    uint64_t m_macSyncSymbolOffset;

    /**
     * Used by a PAN coordinator or coordinator.
     * Defines how often the coordinator transmits its beacon
     * (outgoing superframe). Range 0 - 15 with 15 meaning no beacons are being sent.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    uint8_t m_macBeaconOrder;

    /**
     * Used by a PAN coordinator or coordinator. The length of the active portion
     * of the outgoing superframe, including the beacon frame.
     * 0 - 15 with 15 means the superframe will not be active after the beacon.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    uint8_t m_macSuperframeOrder;

    /**
     * The maximum time (in UNIT periods) that a transaction is stored by a
     * coordinator and indicated in its beacon. This value establish the expiration
     * time of the packets stored in the pending transaction list (indirect transmissions).
     * 1 Unit Period:
     * Beacon-enabled = aBaseSuperframeDuration * 2^BO
     * Non-beacon enabled = aBaseSuperframeDuration
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    uint16_t m_macTransactionPersistenceTime;

    /**
     * The total size of the received beacon in symbols.
     * Its value is used to calculate the end CAP time of the incoming superframe.
     */
    uint64_t m_rxBeaconSymbols;

    /**
     * Indication of the Slot where the CAP portion of the OUTGOING Superframe ends.
     */
    uint8_t m_fnlCapSlot;

    /**
     * The beaconOrder value of the INCOMING frame. Used by all devices that have a parent.
     * Specification of how often the parent coordinator transmits its beacon.
     * 0 - 15 with 15 means the parent is not currently transmitting beacons.
     */
    uint8_t m_incomingBeaconOrder;

    /**
     * Used by all devices that have a parent.
     * The length of the active portion of the INCOMING superframe, including the
     * beacon frame.
     * 0 - 15 with 15 meaning the superframe will not be active after the beacon.
     */
    uint8_t m_incomingSuperframeOrder;

    /**
     * Indication of the Slot where the CAP portion of the INCOMING Superframe ends.
     */
    uint8_t m_incomingFnlCapSlot;

    /**
     * Indicates if MAC sublayer is in receive all mode. True mean accept all
     * frames from PHY.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    bool m_macPromiscuousMode;

    /**
     * 16 bits id of PAN on which this device is operating. 0xffff means not
     * associated.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    uint16_t m_macPanId;

    /**
     * Temporally stores the value of the current m_macPanId when a MLME-SCAN.request is performed.
     * See IEEE 802.15.4-2011, section 5.1.2.1.2.
     */
    uint16_t m_macPanIdScan;

    /**
     * Sequence number added to transmitted data or MAC command frame, 00-ff.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    SequenceNumber8 m_macDsn;

    /**
     * Sequence number added to transmitted beacon frame, 00-ff.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    SequenceNumber8 m_macBsn;

    /**
     * The contents of the beacon payload.
     * This value is set directly by the MLME-SET primitive.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    Ptr<Packet> m_macBeaconPayload;

    /**
     * The length, in octets, of the beacon payload.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    uint32_t m_macBeaconPayloadLength;

    /**
     * The maximum number of retries allowed after a transmission failure.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    uint8_t m_macMaxFrameRetries;

    /**
     * Indication of whether the MAC sublayer is to enable its receiver during
     * idle periods.
     * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
     */
    bool m_macRxOnWhenIdle;

    /**
     * The minimum time forming a Long InterFrame Spacing (LIFS) period.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    uint32_t m_macLIFSPeriod;

    /**
     * The minimum time forming a Short InterFrame Spacing (SIFS) period.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    uint32_t m_macSIFSPeriod;

    /**
     * Indication of whether a coordinator is currently allowing association.
     * A value of TRUE indicates that the association is permitted.
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    bool m_macAssociationPermit;

    /**
     * Indication of whether a device automatically sends data request command
     * if its address is listed in the beacon frame.
     * TRUE = request command automatically is sent. This command also affects
     * the generation of MLME-BEACON-NOTIFY.indication (6.2.4.1)
     * See IEEE 802.15.4-2011, section 6.4.2, Table 52.
     */
    bool m_macAutoRequest;

    /**
     * The maximum energy level detected during ED scan on the current channel.
     */
    uint8_t m_maxEnergyLevel;

    /**
     * The value of the necessary InterFrame Space after the transmission of a packet.
     */
    uint32_t m_ifs;

    /**
     * Indication of whether the current device is the PAN coordinator
     */
    bool m_panCoor;

    /**
     * Indicates if the current device is a coordinator type
     */
    bool m_coor;

    /**
     * Indication of the Interval used by the coordinator to transmit beacon frames
     * expressed in symbols.
     */
    uint32_t m_beaconInterval;

    /**
     * Indication of the superframe duration in symbols.
     * (e.g. 1 symbol = 4 bits in a 250kbps O-QPSK PHY)
     */
    uint32_t m_superframeDuration;

    /**
     * Indication of the interval a node should receive a superframe
     * expressed in symbols.
     */
    uint32_t m_incomingBeaconInterval;

    /**
     * Indication of the superframe duration in symbols
     * (e.g. 1 symbol = 4 bits in a 250kbps O-QPSK PHY)
     */
    uint32_t m_incomingSuperframeDuration;

    /**
     * Indication of current device capability (FFD or RFD)
     */
    uint8_t m_deviceCapability;

    /**
     * Indication of whether the current device is tracking incoming beacons.
     */
    bool m_beaconTrackingOn;

    /**
     * The number of consecutive loss beacons in a beacon tracking operation.
     */
    uint8_t m_numLostBeacons;

    /**
     * Get the macAckWaitDuration attribute value.
     *
     * \return the maximum number symbols to wait for an acknowledgment frame
     */
    uint64_t GetMacAckWaitDuration() const;

    /**
     * Get the macMaxFrameRetries attribute value.
     *
     * \return the maximum number of retries
     */
    uint8_t GetMacMaxFrameRetries() const;

    /**
     * Print the number of elements in the packet transmit queue.
     */
    void PrintTransmitQueueSize();

    /**
     * Set the macMaxFrameRetries attribute value.
     *
     * \param retries the maximum number of retries
     */
    void SetMacMaxFrameRetries(uint8_t retries);

    /**
     * Check if the packet destination is its coordinator
     *
     * \return True if m_txPkt (packet awaiting to be sent) destination is its coordinator
     */
    bool isCoordDest();

    /**
     * Check if the packet destination is its coordinator
     *
     *\param mac The coordinator short MAC Address
     */
    void SetAssociatedCoor(Mac16Address mac);

    /**
     * Check if the packet destination is its coordinator
     *
     *\param mac The coordinator extended MAC Address
     */
    void SetAssociatedCoor(Mac64Address mac);

    /**
     * Get the size of the Interframe Space according to MPDU size (m_txPkt).
     *
     * \return the IFS size in symbols
     */
    uint32_t GetIfsSize();

    /**
     * Obtain the number of symbols in the packet which is currently being sent by the MAC layer.
     *
     *\return packet number of symbols
     * */
    uint64_t GetTxPacketSymbols();

    /**
     * Check if the packet to transmit requires acknowledgment
     *
     *\return True if the Tx packet requires acknowledgment
     * */
    bool isTxAckReq();

    /**
     * Print the Pending transaction list.
     * \param os The reference to the output stream used by this print function.
     */
    void PrintPendingTxQueue(std::ostream& os) const;

    /**
     * Print the Transmit Queue.
     * \param os The reference to the output stream used by this print function.
     */
    void PrintTxQueue(std::ostream& os) const;

    /**
     * TracedCallback signature for sent packets.
     *
     * \param [in] packet The packet.
     * \param [in] retries The number of retries.
     * \param [in] backoffs The number of CSMA backoffs.
     */
    typedef void (*SentTracedCallback)(Ptr<const Packet> packet, uint8_t retries, uint8_t backoffs);

    /**
     * TracedCallback signature for LrWpanMacState change events.
     *
     * \param [in] oldValue The original state value.
     * \param [in] newValue The new state value.
     * \deprecated The LrWpanMacState is now accessible as the
     * TracedValue \c MacStateValue. The \c MacState TracedCallback will
     * be removed in a future release.
     */
    typedef void (*StateTracedCallback)(LrWpanMacState oldState, LrWpanMacState newState);

  protected:
    // Inherited from Object.
    void DoInitialize() override;
    void DoDispose() override;

  private:
    /**
     * Helper structure for managing transmission queue elements.
     */
    struct TxQueueElement : public SimpleRefCount<TxQueueElement>
    {
        uint8_t txQMsduHandle; //!< MSDU Handle
        Ptr<Packet> txQPkt;    //!< Queued packet
    };

    /**
     * Helper structure for managing pending transaction list elements (Indirect transmissions).
     */
    struct IndTxQueueElement : public SimpleRefCount<IndTxQueueElement>
    {
        uint8_t seqNum;               //!< The sequence number of  the queued packet
        Mac16Address dstShortAddress; //!< The destination short Mac Address
        Mac64Address dstExtAddress;   //!< The destination extended Mac Address
        Ptr<Packet> txQPkt;           //!< Queued packet.
        Time expireTime; //!< The expiration time of the packet in the indirect transmission queue.
    };

    /**
     * Called to send a single beacon frame.
     */
    void SendOneBeacon();

    /**
     * Called to send an associate request command.
     */
    void SendAssocRequestCommand();

    /**
     * Used to send a data request command (i.e. Request the coordinator to send the association
     * response)
     */
    void SendDataRequestCommand();

    /**
     * Called to send an associate response command.
     *
     * \param rxDataReqPkt The received data request pkt that instigated the Association response
     * command.
     */
    void SendAssocResponseCommand(Ptr<Packet> rxDataReqPkt);

    /**
     * Called after m_assocRespCmdWaitTime timeout while waiting for an association response
     * command.
     */
    void LostAssocRespCommand();

    /**
     * Called to send a beacon request command.
     */
    void SendBeaconRequestCommand();

    /**
     * Called to send a orphan notification command. This is used by an associated device that
     * has lost synchronization with its coordinator.
     * As described in IEEE 802.15.4-2011 (Section 5.3.6)
     */
    void SendOrphanNotificationCommand();

    /**
     * Called to end a MLME-START.request after changing the page and channel number.
     */
    void EndStartRequest();

    /**
     * Called at the end of the current channel scan (Active or Passive) for a given duration.
     */
    void EndChannelScan();

    /**
     * Called at the end of one ED channel scan.
     */
    void EndChannelEnergyScan();

    /**
     * Called to end an MLME-ASSOCIATE.request after changing the page and channel number.
     */
    void EndAssociateRequest();

    /**
     * Called to begin the Contention Free Period (CFP) in a
     * beacon-enabled mode.
     *
     * \param superframeType The incoming or outgoing superframe reference
     */
    void StartCFP(SuperframeType superframeType);

    /**
     * Called to begin the Contention Access Period (CAP) in a
     * beacon-enabled mode.
     *
     * \param superframeType The incoming or outgoing superframe reference
     */
    void StartCAP(SuperframeType superframeType);

    /**
     * Start the Inactive Period in a beacon-enabled mode.
     *
     * \param superframeType The incoming or outgoing superframe reference
     *
     */
    void StartInactivePeriod(SuperframeType superframeType);

    /**
     * Called after the end of an INCOMING superframe to start the moment a
     * device waits for a new incoming beacon.
     */
    void AwaitBeacon();

    /**
     * Called if the device is unable to locate a beacon in the time set by MLME-SYNC.request.
     */
    void BeaconSearchTimeout();

    /**
     * Send an acknowledgment packet for the given sequence number.
     *
     * \param seqno the sequence number for the ACK
     */
    void SendAck(uint8_t seqno);

    /**
     * Add an element to the transmission queue.
     *
     * \param txQElement The element added to the Tx Queue.
     */
    void EnqueueTxQElement(Ptr<TxQueueElement> txQElement);

    /**
     * Remove the tip of the transmission queue, including clean up related to the
     * last packet transmission.
     */
    void RemoveFirstTxQElement();

    /**
     * Change the current MAC state to the given new state.
     *
     * \param newState the new state
     */
    void ChangeMacState(LrWpanMacState newState);

    /**
     * Handle an ACK timeout with a packet retransmission, if there are
     * retransmission left, or a packet drop.
     */
    void AckWaitTimeout();

    /**
     * After a successful transmission of a frame (beacon, data) or an ack frame reception,
     * the mac layer wait an Interframe Space (IFS) time and triggers this function
     * to continue with the MAC flow.
     *
     * \param ifsTime IFS time
     */
    void IfsWaitTimeout(Time ifsTime);

    /**
     * Check for remaining retransmissions for the packet currently being sent.
     * Drop the packet, if there are no retransmissions left.
     *
     * \return true, if the packet should be retransmitted, false otherwise.
     */
    bool PrepareRetransmission();

    /**
     * Adds a packet to the pending transactions list (Indirect transmissions).
     *
     * \param p The packet added to pending transaction list.
     */
    void EnqueueInd(Ptr<Packet> p);

    /**
     * Extracts a packet from pending transactions list (Indirect transmissions).
     * \param dst The extended address used an index to obtain an element from the pending
     * transaction list.
     * \param entry The dequeued element from the pending transaction list.
     * \return The status of the dequeue
     */
    bool DequeueInd(Mac64Address dst, Ptr<IndTxQueueElement> entry);

    /**
     * Purge expired transactions from the pending transactions list.
     */
    void PurgeInd();

    /**
     * Remove an element from the pending transaction list.
     *
     * \param p The packet to be removed from the pending transaction list.
     */
    void RemovePendTxQElement(Ptr<Packet> p);

    /**
     * Check the transmission queue. If there are packets in the transmission
     * queue and the MAC is idle, pick the first one and initiate a packet
     * transmission.
     */
    void CheckQueue();

    /**
     * Constructs a Superframe specification field from the local information,
     * the superframe Specification field is necessary to create a beacon frame.
     *
     * \returns the Superframe specification field
     */
    SuperframeField GetSuperframeField();

    /**
     * Constructs the Guaranteed Time Slots (GTS) Fields from local information.
     * The GTS Fields are part of the beacon frame.
     *
     * \returns the Guaranteed Time Slots (GTS) Fields
     */
    GtsFields GetGtsFields();

    /**
     * Constructs Pending Address Fields from the local information,
     * the Pending Address Fields are part of the beacon frame.
     *
     * \returns the Pending Address Fields
     */
    PendingAddrFields GetPendingAddrFields();

    /**
     * The trace source is fired at the end of any Interframe Space (IFS).
     */
    TracedCallback<Time> m_macIfsEndTrace;

    /**
     * The trace source fired when packets are considered as successfully sent
     * or the transmission has been given up.
     * Only non-broadcast packets are traced.
     *
     * The data should represent:
     * packet, number of retries, total number of csma backoffs
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>, uint8_t, uint8_t> m_sentPktTrace;

    /**
     * The trace source fired when packets come into the "top" of the device
     * at the L3/L2 transition, when being queued for transmission.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxEnqueueTrace;

    /**
     * The trace source fired when packets are dequeued from the
     * L3/l2 transmission queue.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxDequeueTrace;

    /**
     * The trace source fired when packets come into the "top" of the device
     * at the L3/L2 transition, when being queued for indirect transmission
     * (pending transaction list).
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macIndTxEnqueueTrace;

    /**
     * The trace source fired when packets are dequeued from the
     * L3/l2 indirect transmission queue (Pending transaction list).
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macIndTxDequeueTrace;

    /**
     * The trace source fired when packets are being sent down to L1.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxTrace;

    /**
     * The trace source fired when packets where successfully transmitted, that is
     * an acknowledgment was received, if requested, or the packet was
     * successfully sent by L1, if no ACK was requested.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxOkTrace;

    /**
     * The trace source fired when packets are dropped due to missing ACKs or
     * because of transmission failures in L1.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxDropTrace;

    /**
     * The trace source fired when packets are dropped due to indirect Tx queue
     * overflows or expiration.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macIndTxDropTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a promiscuous trace.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macPromiscRxTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a non-promiscuous trace.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macRxTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * but dropped before being forwarded up to higher layers (at the L2/L3
     * transition).
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macRxDropTrace;

    /**
     * A trace source that emulates a non-promiscuous protocol sniffer connected
     * to the device.  Unlike your average everyday sniffer, this trace source
     * will not fire on PACKET_OTHERHOST events.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device hard_start_xmit where
     * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in netif_receive_skb.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_snifferTrace;

    /**
     * A trace source that emulates a promiscuous mode protocol sniffer connected
     * to the device.  This trace source fire on packets destined for any host
     * just like your average everyday packet sniffer.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device hard_start_xmit where
     * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in netif_receive_skb.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_promiscSnifferTrace;

    /**
     * A trace source that fires when the LrWpanMac changes states.
     * Parameters are the old mac state and the new mac state.
     *
     * \deprecated This TracedCallback is deprecated and will be
     * removed in a future release,  Instead, use the \c MacStateValue
     * TracedValue.
     */
    TracedCallback<LrWpanMacState, LrWpanMacState> m_macStateLogger;

    /**
     * The PHY associated with this MAC.
     */
    Ptr<LrWpanPhy> m_phy;

    /**
     * The CSMA/CA implementation used by this MAC.
     */
    Ptr<LrWpanCsmaCa> m_csmaCa;

    /**
     * This callback is used to report the result of an attribute writing request
     * to the upper layers.
     * See IEEE 802.15.4-2011, section 6.2.11.2.
     */
    MlmeSetConfirmCallback m_mlmeSetConfirmCallback;

    /**
     * This callback is used to report the result of an attribute read request
     * to the upper layers.
     * See IEEE 802.15.4-2011, section 6.2.5.2
     */
    MlmeGetConfirmCallback m_mlmeGetConfirmCallback;

    /**
     * This callback is used to notify incoming beacon packets to the upper layers.
     * See IEEE 802.15.4-2011, section 6.2.4.1.
     */
    MlmeBeaconNotifyIndicationCallback m_mlmeBeaconNotifyIndicationCallback;

    /**
     * This callback is used to indicate the loss of synchronization with a coordinator.
     * See IEEE 802.15.4-2011, section 6.2.13.2.
     */
    MlmeSyncLossIndicationCallback m_mlmeSyncLossIndicationCallback;

    /**
     * This callback is used to report the result of a scan on a group of channels for the
     * selected channel page.
     * See IEEE 802.15.4-2011, section 6.2.10.2.
     */
    MlmeScanConfirmCallback m_mlmeScanConfirmCallback;

    /**
     * This callback is used to report the status after a device request an association with
     * a coordinator.
     * See IEEE 802.15.4-2011, section 6.2.2.4.
     */
    MlmeAssociateConfirmCallback m_mlmeAssociateConfirmCallback;

    /**
     * This callback is used to report the status after a device send data command request to
     * the coordinator to transmit data.
     * See IEEE 802.15.4-2011, section 6.2.14.2.
     */
    MlmePollConfirmCallback m_mlmePollConfirmCallback;

    /**
     * This callback is used to report the start of a new PAN or
     * the begin of a new superframe configuration.
     * See IEEE 802.15.4-2006, section 7.1.14.2.
     */
    MlmeStartConfirmCallback m_mlmeStartConfirmCallback;

    /**
     * This callback is used to notify incoming packets to the upper layers.
     * See IEEE 802.15.4-2006, section 7.1.1.3.
     */
    McpsDataIndicationCallback m_mcpsDataIndicationCallback;

    /**
     * This callback is used to indicate the reception of an association request command.
     * See IEEE 802.15.4-2011, section 6.2.2.2
     */
    MlmeAssociateIndicationCallback m_mlmeAssociateIndicationCallback;

    /**
     * This callback is instigated through a response primitive.
     * See IEEE 802.15.4-2011, section 6.2.4.2
     */
    MlmeCommStatusIndicationCallback m_mlmeCommStatusIndicationCallback;

    /**
     * This callback is used to indicate the reception of a orphan notification command.
     * See IEEE 802.15.4-2011, section 6.2.7.1
     */
    MlmeOrphanIndicationCallback m_mlmeOrphanIndicationCallback;

    /**
     * This callback is used to report data transmission request status to the
     * upper layers.
     * See IEEE 802.15.4-2006, section 7.1.1.2.
     */
    McpsDataConfirmCallback m_mcpsDataConfirmCallback;

    /**
     * The current state of the MAC layer.
     */
    TracedValue<LrWpanMacState> m_lrWpanMacState;

    /**
     * The current period of the incoming superframe.
     */
    TracedValue<SuperframeStatus> m_incSuperframeStatus;

    /**
     * The current period of the outgoing superframe.
     */
    TracedValue<SuperframeStatus> m_outSuperframeStatus;

    /**
     * The current association status of the MAC layer.
     */
    LrWpanAssociationStatus m_associationStatus;

    /**
     * The packet which is currently being sent by the MAC layer.
     */
    Ptr<Packet> m_txPkt;

    /**
     * The command request packet received. Briefly stored to proceed with operations
     * that take place after ACK messages.
     */
    Ptr<Packet> m_rxPkt;

    /**
     * The short address (16 bit address) used by this MAC. If supported,
     * the short address must be assigned to the device by the coordinator
     * during the association process.
     */
    Mac16Address m_shortAddress;

    /**
     * The extended 64 address (IEEE EUI-64) used by this MAC.
     */
    Mac64Address m_selfExt;

    /**
     * The transmit queue used by the MAC.
     */
    std::deque<Ptr<TxQueueElement>> m_txQueue;

    /**
     * The indirect transmit queue used by the MAC pending messages (The pending transaction
     * list).
     */
    std::deque<Ptr<IndTxQueueElement>> m_indTxQueue;

    /**
     * The maximum size of the transmit queue.
     */
    uint32_t m_maxTxQueueSize;

    /**
     * The maximum size of the indirect transmit queue (The pending transaction list).
     */
    uint32_t m_maxIndTxQueueSize;

    /**
     * The list of PAN descriptors accumulated during channel scans, used to select a PAN to
     * associate.
     */
    std::vector<PanDescriptor> m_panDescriptorList;

    /**
     * The list of energy measurements, one for each channel searched during an ED scan.
     */
    std::vector<uint8_t> m_energyDetectList;

    /**
     * The list of unscanned channels during a scan operation.
     */
    std::vector<uint8_t> m_unscannedChannels;

    /**
     * The parameters used during a MLME-SCAN.request. These parameters are stored here while
     * PLME-SET (set channel page, set channel number) and other operations take place.
     */
    MlmeScanRequestParams m_scanParams;

    /**
     * The parameters used during a MLME-START.request. These parameters are stored here while
     * PLME-SET operations (set channel page, set channel number) take place.
     */
    MlmeStartRequestParams m_startParams;

    /**
     * The parameters used during a MLME-ASSOCIATE.request. These parameters are stored here while
     * PLME-SET operations (set channel page, set channel number) take place.
     */
    MlmeAssociateRequestParams m_associateParams;

    /**
     * The channel list index used to obtain the current scanned channel.
     */
    uint16_t m_channelScanIndex;

    /**
     * Indicates the pending primitive when PLME.SET operation (page or channel switch) is called
     * from within another MLME primitive (e.g. Association, Scan, Sync, Start).
     */
    PendingPrimitiveStatus m_pendPrimitive;

    /**
     * The number of already used retransmission for the currently transmitted
     * packet.
     */
    uint8_t m_retransmission;

    /**
     * The number of CSMA/CA retries used for sending the current packet.
     */
    uint8_t m_numCsmacaRetry;

    /**
     * Keep track of the last received frame Link Quality Indicator
     */
    uint8_t m_lastRxFrameLqi;

    /**
     * Scheduler event for the ACK timeout of the currently transmitted data
     * packet.
     */
    EventId m_ackWaitTimeout;

    /**
     * Scheduler event for a response to a request command frame.
     */
    EventId m_respWaitTimeout;

    /**
     * Scheduler event for the lost of a association response command frame.
     */
    EventId m_assocResCmdWaitTimeout;

    /**
     * Scheduler event for a deferred MAC state change.
     */
    EventId m_setMacState;

    /**
     * Scheduler event for Interframe spacing wait time.
     */
    EventId m_ifsEvent;

    /**
     * Scheduler event for generation of one beacon.
     */
    EventId m_beaconEvent;

    /**
     * Scheduler event for the end of the outgoing superframe CAP.
     **/
    EventId m_capEvent;

    /**
     * Scheduler event for the end of the outgoing superframe CFP.
     */
    EventId m_cfpEvent;

    /**
     * Scheduler event for the end of the incoming superframe CAP.
     **/
    EventId m_incCapEvent;

    /**
     * Scheduler event for the end of the incoming superframe CFP.
     */
    EventId m_incCfpEvent;

    /**
     * Scheduler event to track the incoming beacons.
     */
    EventId m_trackingEvent;

    /**
     * Scheduler event for the end of an ACTIVE or PASSIVE channel scan.
     */
    EventId m_scanEvent;

    /**
     * Scheduler event for the end of an ORPHAN channel scan.
     */
    EventId m_scanOrphanEvent;

    /**
     * Scheduler event for the end of a ED channel scan.
     */
    EventId m_scanEnergyEvent;
};
} // namespace ns3

#endif /* LR_WPAN_MAC_H */
