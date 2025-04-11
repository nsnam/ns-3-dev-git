/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#ifndef WIFI_STATIC_SETUP_HELPER_H
#define WIFI_STATIC_SETUP_HELPER_H

#include "ns3/mac48-address.h"
#include "ns3/wifi-utils.h"

#include <optional>
#include <set>

namespace ns3
{

class ApWifiMac;
class MgtAssocRequestHeader;
class MgtAssocResponseHeader;
class NetDeviceContainer;
class WifiNetDevice;
class StaWifiMac;
class WifiMacHeader;

/**
 * Helper to statically setup wifi devices without actually exchanging management frames over the
 * air:
 *
 * - association/ML setup (note that scanning is disabled for this purpose)
 * - block ack agreement(s)
 * - enabling EMLSR mode on EMLSR client links
 */
class WifiStaticSetupHelper
{
  public:
    /// Bypass static capabilities exchange for input devices
    /// @param bssDev BSS advertising device (AP)
    /// @param clientDevs client devices
    static void SetStaticAssociation(Ptr<WifiNetDevice> bssDev,
                                     const NetDeviceContainer& clientDevs);

    /// Bypass static capabilities exchange for input devices
    /// @param bssDev BSS advertising device (AP)
    /// @param clientDev client device
    static void SetStaticAssociation(Ptr<WifiNetDevice> bssDev, Ptr<WifiNetDevice> clientDev);

    /// Perform static Association Request/Response exchange for input devices post initialization
    /// at runtime begin
    /// @param bssDev BSS advertising device (AP)
    /// @param clientDev client device
    static void SetStaticAssocPostInit(Ptr<WifiNetDevice> bssDev, Ptr<WifiNetDevice> clientDev);

    /// Perform static Association Request/Response exchange for input devices post initialization
    /// at runtime begin
    /// @param apMac AP MAC
    /// @param clientMac client MAC
    static void SetStaticAssocPostInit(Ptr<ApWifiMac> apMac, Ptr<StaWifiMac> clientMac);

    /// Construct non-AP MLD link ID to AP MLD link ID mapping based on PHY channel settings.
    /// It is required that for each STA affiliated with the non-AP MLD there exists an AP
    /// affiliated with the AP MLD that operates on a channel having the same primary20 as the
    /// channel on which the STA operates.
    /// @param apDev AP NetDevice
    /// @param clientDev STA NetDevice
    /// @return link ID mapping
    static std::map<linkId_t, linkId_t> GetLinkIdMap(Ptr<WifiNetDevice> apDev,
                                                     Ptr<WifiNetDevice> clientDev);

    /// Construct non-AP MLD link ID to AP MLD link ID mapping based on PHY channel settings
    /// It is required that for each STA affiliated with the non-AP MLD there exists an AP
    /// affiliated with the AP MLD that operates on a channel having the same primary20 as the
    /// channel on which the STA operates.
    /// @param apMac AP MAC
    /// @param staMac STA MAC
    /// @return link ID mapping
    static std::map<linkId_t, linkId_t> GetLinkIdMap(Ptr<ApWifiMac> apMac, Ptr<StaWifiMac> staMac);

    /// Get Association Request for input STA link address
    /// @param staMac STA MAC
    /// @param staLinkId ID of link used for Assoc request
    /// @param isMldAssoc true if MLD association, false otherwise
    /// @return association request
    static MgtAssocRequestHeader GetAssocReq(Ptr<StaWifiMac> staMac,
                                             linkId_t staLinkId,
                                             bool isMldAssoc);

    /// Get Association Response for input STA link address from
    /// AP MAC including Multi-link Element if MLD Association
    /// @param staLinkAddr STA link MAC address
    /// @param apMac AP MAC
    /// @param apLinkId ID of link used for Assoc response
    /// @param isMldAssoc true if MLD association, false otherwise
    /// @return association response
    static MgtAssocResponseHeader GetAssocResp(Mac48Address staLinkAddr,
                                               Ptr<ApWifiMac> apMac,
                                               linkId_t apLinkId,
                                               bool isMldAssoc);

    /// Get Association Response MAC Header for input STA link address from
    /// AP MAC including Multi-link Element if MLD Association
    /// @param staLinkAddr STA link MAC address
    /// @param apMac AP MAC
    /// @param apLinkId Link ID of link used for Assoc response
    /// @return MAC header
    static WifiMacHeader GetAssocRespMacHdr(Mac48Address staLinkAddr,
                                            Ptr<ApWifiMac> apMac,
                                            linkId_t apLinkId);

    /// Bypass ADDBA Request-Response exchange sequence between AP and STAs for given TIDs.
    /// Static setup will be performed in both uplink and downlink.
    /// @param apDev AP device
    /// @param clientDevs STA devices
    /// @param tids the set of TIDs corresponding to Block ACK agreements
    /// @param gcrGroupAddr MAC address of the GCR group (GCR Group Address)
    static void SetStaticBlockAck(Ptr<WifiNetDevice> apDev,
                                  const NetDeviceContainer& clientDevs,
                                  const std::set<tid_t>& tids,
                                  std::optional<Mac48Address> gcrGroupAddr = std::nullopt);

    /// Bypass ADDBA Request-Response exchange sequence between input devices for given TID.
    /// @param originatorDev originator device of Block ACK agreement
    /// @param recipientDev recipient device of Block ACK agreement
    /// @param tid TID corresponding to Block ACK agreement
    /// @param gcrGroupAddr MAC address of the GCR group (GCR Group Address)
    static void SetStaticBlockAck(Ptr<WifiNetDevice> originatorDev,
                                  Ptr<WifiNetDevice> recipientDev,
                                  tid_t tid,
                                  std::optional<Mac48Address> gcrGroupAddr = std::nullopt);

    /// Perform ADDBA Request-Response exchange sequence between input devices for given TID
    /// post initialization at runtime begin
    /// @param originatorDev originator device of Block ACK agreement
    /// @param recipientDev recipient device of Block ACK agreement
    /// @param tid TID corresponding to Block ACK agreement
    /// @param gcrGroupAddr MAC address of the GCR group (GCR Group Address)
    static void SetStaticBlockAckPostInit(Ptr<WifiNetDevice> originatorDev,
                                          Ptr<WifiNetDevice> recipientDev,
                                          tid_t tid,
                                          std::optional<Mac48Address> gcrGroupAddr = std::nullopt);

    /// Get Block ACK originator address based on devices MAC config
    /// @param originatorMac originator MAC
    /// @param recipientMac recipient MAC
    /// @return the Block Ack originator address
    static Mac48Address GetBaOriginatorAddr(Ptr<WifiMac> originatorMac, Ptr<WifiMac> recipientMac);

    /// Get Block ACK recipient address based on devices MAC config
    /// @param originatorMac originator MAC
    /// @param recipientMac recipient MAC
    /// @return the Block Ack recipient address
    static Mac48Address GetBaRecipientAddr(Ptr<WifiMac> originatorMac, Ptr<WifiMac> recipientMac);

    /// Bypass EML Operating Mode Notification exchange sequence between AP MLD
    /// and input non-AP devices
    /// @param apDev AP MLD
    /// @param clientDevs Non-AP devices
    static void SetStaticEmlsr(Ptr<WifiNetDevice> apDev, const NetDeviceContainer& clientDevs);

    /// Bypass EML Operating Mode Notification exchange sequence between AP MLD and non-AP MLD
    /// to enable EMLSR mode on the links specified via the EmlsrManager::EmlsrLinkSet attribute
    /// @param apDev AP MLD
    /// @param clientDev Non-AP MLD
    static void SetStaticEmlsr(Ptr<WifiNetDevice> apDev, Ptr<WifiNetDevice> clientDev);

    /// Perform EML Operating Mode Notification exchange sequence between AP MLD and non-AP MLD
    /// to enable EMLSR mode on the links specified via the EmlsrManager::EmlsrLinkSet attribute
    /// post initialization at runtime begin
    /// @param apDev AP MLD
    /// @param clientDev Non-AP MLD
    static void SetStaticEmlsrPostInit(Ptr<WifiNetDevice> apDev, Ptr<WifiNetDevice> clientDev);
};

} // namespace ns3

#endif // WIFI_STATIC_SETUP_HELPER_H
