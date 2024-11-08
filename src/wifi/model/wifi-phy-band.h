/*
 * Copyright (c) 2020
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_PHY_BAND_H
#define WIFI_PHY_BAND_H

#include <iostream>

namespace ns3
{

/**
 * @ingroup wifi
 * Identifies the PHY band.
 */
enum WifiPhyBand
{
    /** The 2.4 GHz band */
    WIFI_PHY_BAND_2_4GHZ,
    /** The 5 GHz band */
    WIFI_PHY_BAND_5GHZ,
    /** The 6 GHz band */
    WIFI_PHY_BAND_6GHZ,
    /** The 60 GHz band */
    WIFI_PHY_BAND_60GHZ,
    /** Unspecified */
    WIFI_PHY_BAND_UNSPECIFIED
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param band the band
 * @returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, WifiPhyBand band)
{
    switch (band)
    {
    case WIFI_PHY_BAND_2_4GHZ:
        return (os << "2.4GHz");
    case WIFI_PHY_BAND_5GHZ:
        return (os << "5GHz");
    case WIFI_PHY_BAND_6GHZ:
        return (os << "6GHz");
    case WIFI_PHY_BAND_60GHZ:
        return (os << "60GHz");
    default:
        return (os << "INVALID");
    }
}

} // namespace ns3

#endif /* WIFI_PHY_BAND_H */
