/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef WIFI_STANDARD_H
#define WIFI_STANDARD_H

#include "wifi-phy-band.h"
#include "wifi-types.h"
#include "wifi-units.h"

#include "ns3/abort.h"

#include <list>
#include <map>

namespace ns3
{

/**
 * @ingroup wifi
 * Identifies the IEEE 802.11 specifications that a Wifi device can be configured to use.
 */
enum WifiStandard
{
    WIFI_STANDARD_UNSPECIFIED,
    WIFI_STANDARD_80211a,
    WIFI_STANDARD_80211b,
    WIFI_STANDARD_80211g,
    WIFI_STANDARD_80211p,
    WIFI_STANDARD_80211n,
    WIFI_STANDARD_80211ac,
    WIFI_STANDARD_80211ad,
    WIFI_STANDARD_80211ax,
    WIFI_STANDARD_80211be,
    WIFI_STANDARD_COUNT
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param standard the standard
 * @returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, WifiStandard standard)
{
    switch (standard)
    {
    case WIFI_STANDARD_80211a:
        return (os << "802.11a");
    case WIFI_STANDARD_80211b:
        return (os << "802.11b");
    case WIFI_STANDARD_80211g:
        return (os << "802.11g");
    case WIFI_STANDARD_80211p:
        return (os << "802.11p");
    case WIFI_STANDARD_80211n:
        return (os << "802.11n");
    case WIFI_STANDARD_80211ac:
        return (os << "802.11ac");
    case WIFI_STANDARD_80211ad:
        return (os << "802.11ad");
    case WIFI_STANDARD_80211ax:
        return (os << "802.11ax");
    case WIFI_STANDARD_80211be:
        return (os << "802.11be");
    default:
        return (os << "UNSPECIFIED");
    }
}

/**
 * @brief map a given standard configured by the user to the allowed PHY bands
 */
extern const std::map<WifiStandard, std::list<WifiPhyBand>> wifiStandards;

/**
 * Get the type of the frequency channel for the given standard
 *
 * @param standard the standard
 * @return the type of the frequency channel for the given standard
 */
inline FrequencyChannelType
GetFrequencyChannelType(WifiStandard standard)
{
    switch (standard)
    {
    case WIFI_STANDARD_80211b:
        return FrequencyChannelType::DSSS;
    case WIFI_STANDARD_80211p:
        return FrequencyChannelType::CH_80211P;
    default:
        return FrequencyChannelType::OFDM;
    }
}

/**
 * Get the default channel width for the given PHY standard and band.
 *
 * @param standard the given standard
 * @param band the given PHY band
 * @return the default channel width for the given standard
 */
inline MHz_u
GetDefaultChannelWidth(WifiStandard standard, WifiPhyBand band)
{
    switch (standard)
    {
    case WIFI_STANDARD_80211b:
        return MHz_u{22};
    case WIFI_STANDARD_80211p:
        return MHz_u{10};
    case WIFI_STANDARD_80211ac:
        return MHz_u{80};
    case WIFI_STANDARD_80211ad:
        return MHz_u{2160};
    case WIFI_STANDARD_80211ax:
    case WIFI_STANDARD_80211be:
        return (band == WIFI_PHY_BAND_2_4GHZ ? MHz_u{20} : MHz_u{80});
    default:
        return MHz_u{20};
    }
}

/**
 * Get the default PHY band for the given standard.
 *
 * @param standard the given standard
 * @return the default PHY band for the given standard
 */
inline WifiPhyBand
GetDefaultPhyBand(WifiStandard standard)
{
    switch (standard)
    {
    case WIFI_STANDARD_80211p:
    case WIFI_STANDARD_80211a:
    case WIFI_STANDARD_80211ac:
    case WIFI_STANDARD_80211ax:
    case WIFI_STANDARD_80211be:
        return WIFI_PHY_BAND_5GHZ;
    case WIFI_STANDARD_80211ad:
        return WIFI_PHY_BAND_60GHZ;
    default:
        return WIFI_PHY_BAND_2_4GHZ;
    }
}

/**
 * Get the TypeId name for the FrameExchangeManager corresponding to the given standard.
 *
 * @param standard the given standard
 * @param qosSupported whether QoS is supported (ignored if standard is at least HT)
 * @return the TypeId name for the FrameExchangeManager corresponding to the given standard
 */
inline std::string
GetFrameExchangeManagerTypeIdName(WifiStandard standard, bool qosSupported)
{
    if (standard >= WIFI_STANDARD_80211be)
    {
        return "ns3::EhtFrameExchangeManager";
    }
    if (standard >= WIFI_STANDARD_80211ax)
    {
        return "ns3::HeFrameExchangeManager";
    }
    if (standard >= WIFI_STANDARD_80211ac)
    {
        return "ns3::VhtFrameExchangeManager";
    }
    if (standard >= WIFI_STANDARD_80211n)
    {
        return "ns3::HtFrameExchangeManager";
    }
    if (qosSupported)
    {
        return "ns3::QosFrameExchangeManager";
    }
    return "ns3::FrameExchangeManager";
}

} // namespace ns3

#endif /* WIFI_STANDARD_H */
