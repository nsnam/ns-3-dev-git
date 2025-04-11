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
};

} // namespace ns3

#endif // WIFI_STATIC_SETUP_HELPER_H
