/*
 * Copyright (c) 2023 Tokushima University, Japan.
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
 *  Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef LR_WPAN_MAC_BASE_H
#define LR_WPAN_MAC_BASE_H

#include <ns3/callback.h>
#include <ns3/mac16-address.h>
#include <ns3/mac64-address.h>
#include <ns3/nstime.h>
#include <ns3/object.h>
#include <ns3/packet.h>
#include <ns3/ptr.h>

#include <cstdint>

namespace ns3
{
namespace lrwpan
{

/**
 * \ingroup lr-wpan
 *
 * The status of a confirm or an indication primitive as a result of a previous request.
 * Represent the value of status in IEEE 802.15.4-2011 primitives.
 * (Tables 6, 12, 18, 20, 31, 33, 35, 37, 39, 47)
 *
 * Status codes values only appear in IEEE 802.15.4-2006, Table 78
 * See also NXP JN5169 IEEE 802.15.4 Stack User Guide
 * (Table 6: Status Enumerations and Section 6.1.23)
 *
 */
enum class MacStatus : std::uint8_t
{
    SUCCESS = 0,              //!< The operation was completed successfully.
    FULL_CAPACITY = 0x01,     //!< PAN at capacity. Association Status field (std. 2006, Table 83)
    ACCESS_DENIED = 0x02,     //!< PAN access denied. Association Status field (std. 2006, Table 83)
    COUNTER_ERROR = 0xdb,     //!< The frame counter of the received frame is invalid.
    IMPROPER_KEY_TYPE = 0xdc, //!< The key is not allowed to be used with that frame type.
    IMPROPER_SECURITY_LEVEL = 0xdd, //!< Insufficient security level expected by the recipient.
    UNSUPPORTED_LEGACY = 0xde,      //!< Deprecated security used in IEEE 802.15.4-2003
    UNSUPPORTED_SECURITY = 0xdf,    //!< The security applied is not supported.
    BEACON_LOSS = 0xe0,             //!< The beacon was lost following a synchronization request.
    CHANNEL_ACCESS_FAILURE = 0xe1,  //!< A Tx could not take place due to activity in the CH.
    DENIED = 0xe2,                  //!< The GTS request has been denied by the PAN coordinator.
    DISABLE_TRX_FAILURE = 0xe3,     //!< The attempt to disable the transceier has failed.
    SECURITY_ERROR = 0xe4,      // Cryptographic process of the frame failed(FAILED_SECURITY_CHECK).
    FRAME_TOO_LONG = 0xe5,      //!< Frame more than aMaxPHYPacketSize or too large for CAP or GTS.
    INVALID_GTS = 0xe6,         //!< Missing GTS transmit or undefined direction.
    INVALID_HANDLE = 0xe7,      //!< When purging from TX queue handle was not found.
    INVALID_PARAMETER = 0xe8,   //!< Primitive parameter not supported or out of range.
    NO_ACK = 0xe9,              //!< No acknowledgment was received after macMaxFrameRetries.
    NO_BEACON = 0xea,           //!< A scan operation failed to find any network beacons.
    NO_DATA = 0xeb,             //!<  No response data were available following a request.
    NO_SHORT_ADDRESS = 0xec,    //!< Failure due to unallocated 16-bit short address.
    OUT_OF_CAP = 0xed,          //!< (Deprecated) See IEEE 802.15.4-2003
    PAN_ID_CONFLICT = 0xee,     //!<  PAN id conflict detected and informed to the coordinator.
    REALIGMENT = 0xef,          //!< A coordinator realigment command has been received.
    TRANSACTION_EXPIRED = 0xf0, //!< The transaction expired and its information discarded.
    TRANSACTION_OVERFLOW = 0xf1,  //!< There is no capacity to store the transaction.
    TX_ACTIVE = 0xf2,             //!< The transceiver was already enabled.
    UNAVAILABLE_KEY = 0xf3,       //!< Unavailable key, unknown or blacklisted.
    UNSUPPORTED_ATTRIBUTE = 0xf4, //!< SET/GET request issued with a non supported ID.
    INVALID_ADDRESS = 0xf5,       //!< Invalid source or destination address.
    ON_TIME_TOO_LONG = 0xf6,      //!< RX enable request fail due to syms. longer than Bcn. interval
    PAST_TIME = 0xf7,             //!< Rx enable request fail due to lack of time in superframe.
    TRACKING_OFF = 0xf8,          //!< This device is currently not tracking beacons.
    INVALID_INDEX = 0xf9,     //!< A MAC PIB write failed because specified index is out of range.
    LIMIT_REACHED = 0xfa,     //!< PAN descriptors stored reached max capacity.
    READ_ONLY = 0xfb,         //!< SET/GET request issued for a read only attribute.
    SCAN_IN_PROGRESS = 0xfc,  //!< Scan failed because already performing another scan.
    SUPERFRAME_OVERLAP = 0xfd //!< Coordinator superframe and this device superframe tx overlap.
};

/**
 * \ingroup lr-wpan
 *
 * table 80 of 802.15.4
 */
enum AddressMode
{
    NO_PANID_ADDR = 0,
    ADDR_MODE_RESERVED = 1,
    SHORT_ADDR = 2,
    EXT_ADDR = 3
};

/**
 * \ingroup lr-wpan
 *
 * Table 30 of IEEE 802.15.4-2011
 */
enum MlmeScanType
{
    MLMESCAN_ED = 0x00,
    MLMESCAN_ACTIVE = 0x01,
    MLMESCAN_PASSIVE = 0x02,
    MLMESCAN_ORPHAN = 0x03
};

/**
 * \ingroup lr-wpan
 *
 * MCPS-DATA.request params. See 7.1.1.1
 */
struct McpsDataRequestParams
{
    AddressMode m_srcAddrMode{SHORT_ADDR}; //!< Source address mode
    AddressMode m_dstAddrMode{SHORT_ADDR}; //!< Destination address mode
    uint16_t m_dstPanId{0};                //!< Destination PAN identifier
    Mac16Address m_dstAddr;                //!< Destination address
    Mac64Address m_dstExtAddr;             //!< Destination extended address
    uint8_t m_msduHandle{0};               //!< MSDU handle
    uint8_t m_txOptions{0};                //!< Tx Options (bitfield)
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
 * MLME-SCAN.request params. See IEEE 802.15.4-2011  Section 6.2.10.1 Table 30
 */
struct MlmeScanRequestParams
{
    MlmeScanType m_scanType{MLMESCAN_PASSIVE}; //!< Indicates the type of scan performed as
                                               //!< described in IEEE 802.15.4-2011 (5.1.2.1).
    uint32_t m_scanChannels{0x7FFF800};        //!< The channel numbers to be scanned.
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
 * MLME-ASSOCIATE.request params. See 802.15.4-2011  Section 6.2.2.1
 */
struct MlmeAssociateRequestParams
{
    uint8_t m_chNum{11};                 //!< The channel number on which to attempt association.
    uint32_t m_chPage{0};                //!< The channel page on which to attempt association.
    uint8_t m_coordAddrMode{SHORT_ADDR}; //!< The coordinator addressing mode for this
                                         //!< primitive and subsequent MPDU.
    uint16_t m_coordPanId{0};            //!< The identifier of the PAN with which to associate.
    Mac16Address m_coordShortAddr;       //!< The short address of the coordinator
                                         //!< with which to associate.
    Mac64Address m_coordExtAddr;         //!< The extended address of the coordinator
                                         //!< with which to associate.
    uint8_t m_capabilityInfo{0};         //!< Specifies the operational capabilities
                                         //!< of the associating device (bitmap).
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
    MacStatus m_status{
        MacStatus::ACCESS_DENIED}; //!< The status of the association
                                   //!< attempt (As defined on Table 83 IEEE 802.15.4-2006)
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
    AddressMode m_coorAddrMode{SHORT_ADDR}; //!< The addressing mode of the coordinator
                                            //!< to which the pool is intended.
    uint16_t m_coorPanId{0};      //!< The PAN id of the coordinator to which the poll is intended.
    Mac16Address m_coorShortAddr; //!< Coordinator short address.
    Mac64Address m_coorExtAddr;   //!< Coordinator extended address.
};

/**
 * \ingroup lr-wpan
 *
 * IEEE 802.15.4-2006 PHY and MAC PIB Attribute Identifiers Table 23 and Table 86.
 * Note: Attribute identifiers use standardized values.
 */
enum MacPibAttributeIdentifier
{
    pCurrentChannel = 0x00,      //!< RF channel used for transmissions and receptions.
    pCurrentPage = 0x04,         //!< The current channel page.
    macAckWaitDuration = 0x40,   //!< Maximum number of symbols to wait for an acknowledgment.
    macAssociationPermit = 0x41, //!< Indication of whether a coordinator is allowing association.
    macAutoRequest = 0x42, //!< Indication of whether a device automatically sends a data request
                           //!< command if its address is listed in a beacon frame.
    macBattLifeExt = 0x43, //!< Indication of whether BLE, through the reduction of coordinator
                           //!< receiver operation time during the CAP, is enabled.
    macBattLifeExtPeriods = 0x44,  //!< In BLE mode, the number of backoff periods during which the
                                   //!< the receiver is enabled after the IFS following a beacon.
    macBeaconPayload = 0x45,       //!< The contents of the beacon payload.
    macBeaconPayloadLength = 0x46, //!< The length in octets of the beacon payload.
    macBeaconOrder = 0x47,  //!< Specification of how often the coordinator transmits its beacon.
    macBeaconTxTime = 0x48, //!< The time that the device transmitted its last beacome frame,
                            //!< in symbol periods.
    macBsn = 0x49,          //!< The sequence number added to the transmitted beacon frame.
    macCoordExtendedAddress = 0x4a, //!< The 64-bit address of the coordinator through which
                                    //!< the device is associated.
    macCoordShortAddress = 0x4b, //!< The 16-bit short address assigned to the coordinator through
                                 //!< which the device is associated. 0xFFFE = Ext address mode
                                 //!< 0xFFFF = Unknown.
    macDSN = 0x4c, //!< The sequence number added to the transmitted data or MAC command frame.
    macGTSPermit = 0x4d,      //!< True if the PAN coordinator is to accept GTS requests.
    macMaxCSMABackoff = 0x4e, //!< The maximum number of backoffs the CSMA-CA algorithm
                              //!< will attempt before declaring a channel access failure.
    macMinBE = 0x4f, //!< The minimum value of the backoff exponent (BE) in the CSMA-CA algorithm.
    macExtendedAddress = 0x6f, //!< The extended address of the device (64 bit address). The id
                               //!< is not compliant for 2003 and 2006 versions, but this attribute
                               //!< is later on added to the Pib attributes in 2011 and subsequent
                               //!< editions of the standard.
    macPanId = 0x50,           //!< The 16-bit identifier of the Personal Area Network (PAN).
    macPromiscuousMode = 0x51, //!< Indication of whether the MAC sublayer is in a promiscuous
                               //!< mode. True indicates that the MAC sublayer accepts all frames.
    macRxOnWhenIdle = 0x52,    //!< Indication of whether the MAC is enabled during idle periods.
    macShortAddress = 0x53,    //!< The short address of the device (16 bit address).
    macSuperframeOrder = 0x54, //!< The length of the active portion of the outgoing superframe,
                               //!< including the beacon frame.
    macTransactionPersistenceTime = 0x55, //!< The maximum time (in unit periods) that a
                                          //!< transaction is stored by a coordinator and
                                          //!< indicated in its beacon.
    macMaxFrameRetries = 0x59,  //!< The maximum number of retries allowed after a transmission
                                //!< failure.
    macResponseWaitTime = 0x5a, //!< The maximum time in multiples of aBaseSuperframeDuration, a
                                //!< device shall wait for a response command frame to be
                                //!< available following a request command frame.
    unsupported = 255
    // TODO: complete other MAC pib attributes
};

/**
 * \ingroup lr-wpan
 *
 * IEEE802.15.4-2011 PHY PIB Attributes Table 52 in section 6.4.2
 */
struct MacPibAttributes : public SimpleRefCount<MacPibAttributes>
{
    Ptr<Packet> macBeaconPayload;      //!< The contents of the beacon payload.
    uint8_t macBeaconPayloadLength{0}; //!< The length in octets of the beacon payload.
    Mac16Address macShortAddress;      //!< The 16 bit mac short address
    Mac64Address macExtendedAddress;   //!< The EUI-64 bit address
    uint16_t macPanId{0xffff};         //!< The identifier of the PAN
    uint8_t pCurrentChannel{11};       //!< The current logical channel in used in the PHY
    uint8_t pCurrentPage{0};           //!< The current logical page in use in the PHY
    // TODO: complete other MAC pib attributes
};

/**
 * \ingroup lr-wpan
 *
 * MCPS-DATA.confirm params. See 7.1.1.2
 */
struct McpsDataConfirmParams
{
    uint8_t m_msduHandle{0};                          //!< MSDU handle
    MacStatus m_status{MacStatus::INVALID_PARAMETER}; //!< The status
                                                      //!< of the last MSDU transmission
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
    Mac64Address m_extDevAddr; //!< The extended address of the device requesting association
    uint8_t capabilityInfo{0}; //!< The operational capabilities of
                               //!< the device requesting association.
    uint8_t lqi{0}; //!< The link quality indicator of the received associate request command
                    //!< (Not officially supported in the standard but found in implementations)
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
    MacStatus m_status{MacStatus::INVALID_PARAMETER}; //!< The communication status
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
 * MLME-START.confirm params. See  802.15.4-2011   Section 6.2.12.2
 */
struct MlmeStartConfirmParams
{
    MacStatus m_status{MacStatus::INVALID_PARAMETER}; //!< The status of
                                                      //!< a MLME-start.request
};

/**
 * \ingroup lr-wpan
 *
 * PAN Descriptor, Table 17 IEEE 802.15.4-2011
 */
struct PanDescriptor
{
    AddressMode m_coorAddrMode{SHORT_ADDR}; //!< The coordinator addressing mode corresponding
                                            //!< to the received beacon frame.
    uint16_t m_coorPanId{0xffff};           //!< The PAN ID of the coordinator as specified in
                                            //!<  the received beacon frame.
    Mac16Address m_coorShortAddr; //!< The coordinator short address as specified in the coordinator
                                  //!< address mode.
    Mac64Address m_coorExtAddr;   //!< The coordinator extended address as specified in the
                                  //!< coordinator address mode.
    uint8_t m_logCh{11};          //!< The current channel number occupied by the network.
    uint8_t m_logChPage{0};       //!< The current channel page occupied by the network.
    uint16_t m_superframeSpec{0}; //!< The superframe specification as specified in the received
                                  //!< beacon frame.
    bool m_gtsPermit{false};      //!< TRUE if the beacon is from the PAN coordinator
                                  //!< that is accepting GTS requests.
    uint8_t m_linkQuality{0};     //!< The LQI at which the network beacon was received.
                                  //!< Lower values represent lower LQI.
    Time m_timeStamp; //!< Beacon frame reception time. Used as Time data type in ns-3 to avoid
                      //!< precision problems.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-SCAN.confirm params. See IEEE 802.15.4-2011 Section 6.2.10.2
 */
struct MlmeScanConfirmParams
{
    MacStatus m_status{MacStatus::INVALID_PARAMETER}; //!< The status of the scan request.
    uint8_t m_scanType{MLMESCAN_PASSIVE};             //!< Indicates the type of scan
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
 * MLME-ASSOCIATE.confirm params. See 802.15.4-2011  Section 6.2.2.4
 */
struct MlmeAssociateConfirmParams
{
    Mac16Address m_assocShortAddr; //!< The short address used in the association request
    MacStatus m_status{MacStatus::INVALID_PARAMETER}; //!< The status of
                                                      //!< a MLME-associate.request
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
    MacStatus m_lossReason{MacStatus::PAN_ID_CONFLICT}; //!< The reason for the lost
                                                        //!< of synchronization.
    uint16_t m_panId{0}; //!< The PAN identifier with which the device lost synchronization or to
                         //!< which it was realigned.
    uint8_t m_logCh{11}; //!< The channel number on which the device lost synchronization or to
                         //!< which it was realigned.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-SET.confirm params. See  802.15.4-2011   Section 6.2.11.2
 */
struct MlmeSetConfirmParams
{
    MacStatus m_status{MacStatus::UNSUPPORTED_ATTRIBUTE}; //!< The result of
                                                          //!< the request to write
                                                          //!< the PIB attribute.
    MacPibAttributeIdentifier id; //!< The id of the PIB attribute that was written.
};

/**
 * \ingroup lr-wpan
 *
 * MLME-START.confirm params. See  802.15.4-2011   Section 6.2.14.2
 */
struct MlmePollConfirmParams
{
    MacStatus m_status{MacStatus::INVALID_PARAMETER}; //!< The confirmation
                                                      //!< status resulting from a
                                                      //!< MLME-poll.request.
};

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a McpsDataRequest has been called from
 * the higher layer.  It returns a status of the outcome of the
 * transmission request
 */
using McpsDataConfirmCallback = Callback<void, McpsDataConfirmParams>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a Mcps has successfully received a
 *  frame and wants to deliver it to the higher layer.
 *
 *  \todo for now, we do not deliver all of the parameters in section
 *  802.15.4-2006 7.1.1.3.1 but just send up the packet.
 */
using McpsDataIndicationCallback = Callback<void, McpsDataIndicationParams, Ptr<Packet>>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a Mlme has successfully received a command
 *  frame and wants to deliver it to the higher layer.
 *
 *  Security related parameters and not handle.
 *  See 802.15.4-2011 6.2.2.2.
 */
using MlmeAssociateIndicationCallback = Callback<void, MlmeAssociateIndicationParams>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called by the MLME and issued to its next higher layer following
 * a transmission instigated through a response primitive.
 *
 *  Security related parameters and not handle.
 *  See 802.15.4-2011 6.2.4.2
 */
using MlmeCommStatusIndicationCallback = Callback<void, MlmeCommStatusIndicationParams>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called by the MLME and issued to its next higher layer following
 * the reception of a orphan notification.
 *
 *  Security related parameters and not handle.
 *  See 802.15.4-2011 6.2.7.1
 */
using MlmeOrphanIndicationCallback = Callback<void, MlmeOrphanIndicationParams>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a MlmeStartRequest has been called from
 * the higher layer.  It returns a status of the outcome of the
 * transmission request
 */
using MlmeStartConfirmCallback = Callback<void, MlmeStartConfirmParams>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a MlmeScanRequest has been called from
 * the higher layer.  It returns a status of the outcome of the scan.
 */
using MlmeScanConfirmCallback = Callback<void, MlmeScanConfirmParams>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a MlmeAssociateRequest has been called from
 * the higher layer. It returns a status of the outcome of the
 * association request
 */
using MlmeAssociateConfirmCallback = Callback<void, MlmeAssociateConfirmParams>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a Mlme has successfully received a
 *  beacon frame and wants to deliver it to the higher layer.
 *
 *  \todo for now, we do not deliver all of the parameters in section
 *  802.15.4-2006 6.2.4.1 but just send up the packet.
 */
using MlmeBeaconNotifyIndicationCallback = Callback<void, MlmeBeaconNotifyIndicationParams>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called to indicate the loss of synchronization with
 * a coordinator.
 *
 * \todo for now, we do not deliver all of the parameters in section
 *  See IEEE 802.15.4-2011 6.2.13.2.
 */
using MlmeSyncLossIndicationCallback = Callback<void, MlmeSyncLossIndicationParams>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a MlmeSetRequest has been called from
 * the higher layer to set a PIB. It returns a status of the outcome of the
 * write attempt.
 */
using MlmeSetConfirmCallback = Callback<void, MlmeSetConfirmParams>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a MlmeGetRequest has been called from
 * the higher layer to get a PIB. It returns a status of the outcome of the
 * write attempt.
 */
using MlmeGetConfirmCallback =
    Callback<void, MacStatus, MacPibAttributeIdentifier, Ptr<MacPibAttributes>>;

/**
 * \ingroup lr-wpan
 *
 * This callback is called after a Mlme-Poll.Request has been called from
 * the higher layer.  It returns a status of the outcome of the
 * transmission request
 */
using MlmePollConfirmCallback = Callback<void, MlmePollConfirmParams>;

/**
 * \ingroup netdevice
 *
 * \brief Lr-wpan MAC layer abstraction
 *
 * This class defines the interface functions (primitives) used by a IEEE 802.15.4-2011 compliant
 * MAC layer. Any lr-wpan MAC should extend from this class and implement the
 * behavior of the basic MAC interfaces (primitives).
 *
 */
class LrWpanMacBase : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    ~LrWpanMacBase() override;

    /**
     * IEEE 802.15.4-2006, section 7.1.1.1
     * MCPS-DATA.request
     * Request to transfer a MSDU.
     *
     * \param params the request parameters
     * \param p the packet to be transmitted
     */
    virtual void McpsDataRequest(McpsDataRequestParams params, Ptr<Packet> p) = 0;

    /**
     * IEEE 802.15.4-2006, section 7.1.14.1
     * MLME-START.request
     * Request to allow a PAN coordinator to initiate
     * a new PAN or beginning a new superframe configuration.
     *
     * \param params the request parameters
     */
    virtual void MlmeStartRequest(MlmeStartRequestParams params) = 0;

    /**
     * IEEE 802.15.4-2011, section 6.2.10.1
     * MLME-SCAN.request
     * Request primitive used to initiate a channel scan over a given list of channels.
     *
     * \param params the scan request parameters
     */
    virtual void MlmeScanRequest(MlmeScanRequestParams params) = 0;

    /**
     * IEEE 802.15.4-2011, section 6.2.2.1
     * MLME-ASSOCIATE.request
     * Request primitive used by a device to request an association with
     * a coordinator.
     *
     * \param params the request parameters
     */
    virtual void MlmeAssociateRequest(MlmeAssociateRequestParams params) = 0;

    /**
     * IEEE 802.15.4-2011, section 6.2.2.3
     * MLME-ASSOCIATE.response
     * Primitive used to initiate a response to an MLME-ASSOCIATE.indication
     * primitive.
     *
     * \param params the associate response parameters
     */
    virtual void MlmeAssociateResponse(MlmeAssociateResponseParams params) = 0;

    /**
     * IEEE 802.15.4-2011, section 6.2.13.1
     * MLME-SYNC.request
     * Request to synchronize with the coordinator by acquiring and,
     * if specified, tracking beacons.
     *
     * \param params the request parameters
     */
    virtual void MlmeSyncRequest(MlmeSyncRequestParams params) = 0;

    /**
     * IEEE 802.15.4-2011, section 6.2.14.2
     * MLME-POLL.request
     * Prompts the device to request data from the coordinator.
     *
     * \param params the request parameters
     */
    virtual void MlmePollRequest(MlmePollRequestParams params) = 0;

    /**
     * IEEE 802.15.4-2011, section 6.2.7.2
     * MLME-ORPHAN.response
     * Primitive used to initiatte a response to an MLME-ORPHAN.indication
     * primitive.
     *
     * \param params the orphan response parameters
     */
    virtual void MlmeOrphanResponse(MlmeOrphanResponseParams params) = 0;

    /**
     * IEEE 802.15.4-2011, section 6.2.11.1
     * MLME-SET.request
     * Attempts to write the given value to the indicated PIB attribute.
     *
     * \param id the attributed identifier
     * \param attribute the attribute value
     */
    virtual void MlmeSetRequest(MacPibAttributeIdentifier id, Ptr<MacPibAttributes> attribute) = 0;

    /**
     * IEEE 802.15.4-2006, section 7.1.6.1
     * MLME-GET.request
     * Request information about a given PIB attribute.
     * Note: The PibAttributeIndex parameter is not included because
     * attributes that represent tables are not supported.
     *
     * \param id the attribute identifier
     */
    virtual void MlmeGetRequest(MacPibAttributeIdentifier id) = 0;

    /**
     * Set the callback for the confirmation of a data transmission request.
     * The callback implements MCPS-DATA.confirm SAP of IEEE 802.15.4-2006,
     * section 7.1.1.2.
     *
     * \param c the callback
     */
    void SetMcpsDataConfirmCallback(McpsDataConfirmCallback c);

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
     * The callback implements MLME-SYNC-LOSS.indication SAP of IEEE 802.15.4-2011,
     * section 6.2.13.2.
     *
     * \param c the callback
     */
    void SetMlmeSyncLossIndicationCallback(MlmeSyncLossIndicationCallback c);

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

    /**
     * Set the callback for the confirmation of a data transmission request.
     * The callback implements MLME-POLL.confirm SAP of IEEE 802.15.4-2011,
     * section 6.2.14.2
     *
     * \param c the callback
     */
    void SetMlmePollConfirmCallback(MlmePollConfirmCallback c);

  protected:
    /**
     * This callback is used to report data transmission request status to the
     * upper layers.
     * See IEEE 802.15.4-2006, section 7.1.1.2.
     */
    McpsDataConfirmCallback m_mcpsDataConfirmCallback;

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
     * This callback is used to report the start of a new PAN or
     * the begin of a new superframe configuration.
     * See IEEE 802.15.4-2006, section 7.1.14.2.
     */
    MlmeStartConfirmCallback m_mlmeStartConfirmCallback;

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
     * This callback is used to report the status after a device send data command request to
     * the coordinator to transmit data.
     * See IEEE 802.15.4-2011, section 6.2.14.2.
     */
    MlmePollConfirmCallback m_mlmePollConfirmCallback;
};

} // namespace lrwpan
} // namespace ns3

#endif /* LR_WPAN_MAC_BASE_H*/
