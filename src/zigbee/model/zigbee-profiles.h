/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

namespace ns3
{
namespace zigbee
{
/**
 * Well-known Zigbee application profile identifiers.
 *
 * These profile identifiers are carried in the APS header to identify the
 * application profile associated with an APS frame.
 *
 * There is no unified reference list of Zigbee Profiles, instead,
 * profile identifiers are assigned by the Zigbee Alliance (now the
 * Connectivity Standards Alliance) and defined by their respective profile
 * specifications. The present enumeration is a collection of those numbers
 * found on each specification.
 */
enum ZigbeeProfiles : uint16_t
{
    ZDP_PROFILE =
        0x0000, //!< Zigbee Device Profile (ZDP). Indicates the implementation of the Zigbee Device
                //!< Object (ZDO) on endpoint 0. Source: Zigbee Specification.
    IPM_PROFILE = 0x0101, //!< Industrial Plant Monitoring (IPM) profile. Source: Industrial Plant
                          //!< Monitoring Profile Specification.
    ZHA_PROFILE =
        0x0104, //!< Zigbee Home Automation (ZHA) profile. Also used by Zigbee 3.0 devices for
                //!< interoperability. Source: Zigbee Home Automation Profile Specification.
    CBA_PROFILE = 0x0105,   //!< Commercial Building Automation (CBA) profile. Source: Commercial
                            //!< Building Automation Profile Specification.
    TA_PROFILE = 0x0107,    //!< Telecom Applications (TA) profile. Source: Telecom Applications
                            //!< Profile Specification.
    PHHC_PROFILE = 0x0108,  //!< Personal Home and Hospital Care (PHHC) profile. Source: Personal
                            //!< Home and Hospital Care Profile Specification.
    AMI_PROFILE = 0x0109,   //!< Advanced Metering Initiative (AMI) profile. Predecessor to Smart
                            //!< Energy. Source: Advanced Metering Initiative Profile Specification.
    SE_PROFILE = 0x010A,    //!< Smart Energy (SE) profile (also known as Zigbee Smart Energy, ZSE).
                            //!< Source: Smart Energy Profile Specification.
    RF4CE_PROFILE = 0x010D, //!< Radio Frequency for Consumer Electronics (RF4CE) profile. Intended
                            //!< for remote-control applications.
    GP_PROFILE = 0xA1E0,  //!< Green Power (GP) profile. Supports ultra-low-power energy-harvesting
                          //!< devices. Source: Green Power Specification.
    ZLL_PROFILE = 0xC05E, //!< Zigbee Light Link (ZLL) profile. Legacy lighting profile merged into
                          //!< Zigbee 3.0. Source: Zigbee Light Link Profile Specification.
};
} // namespace zigbee
} // namespace ns3
