/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_PHY_STATE_H
#define WIFI_PHY_STATE_H

#include "ns3/deprecated.h"
#include "ns3/fatal-error.h"

namespace ns3
{

/**
 * The state of the PHY layer.
 */
/// State enumeration
enum class WifiPhyState
{
    /**
     * The PHY layer is IDLE.
     */
    IDLE = 0,
    /**
     * The PHY layer has sense the medium busy through the CCA mechanism
     */
    CCA_BUSY,
    /**
     * The PHY layer is sending a packet.
     */
    TX,
    /**
     * The PHY layer is receiving a packet.
     */
    RX,
    /**
     * The PHY layer is switching to other channel.
     */
    SWITCHING,
    /**
     * The PHY layer is sleeping.
     */
    SLEEP,
    /**
     * The PHY layer is switched off.
     */
    OFF
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param state the state
 * @returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, WifiPhyState state)
{
    switch (state)
    {
    case WifiPhyState::IDLE:
        return (os << "IDLE");
    case WifiPhyState::CCA_BUSY:
        return (os << "CCA_BUSY");
    case WifiPhyState::TX:
        return (os << "TX");
    case WifiPhyState::RX:
        return (os << "RX");
    case WifiPhyState::SWITCHING:
        return (os << "SWITCHING");
    case WifiPhyState::SLEEP:
        return (os << "SLEEP");
    case WifiPhyState::OFF:
        return (os << "OFF");
    default:
        NS_FATAL_ERROR("Invalid state");
        return (os << "INVALID");
    }
}

} // namespace ns3

/**
 * Deprecated WifiPhyState enums.
 *
 * Use WifiPhyState class enum symbols instead.
 * @{
 */
NS_DEPRECATED_3_42("Use WifiPhyState::IDLE instead")
static constexpr auto IDLE = ns3::WifiPhyState::IDLE;
NS_DEPRECATED_3_42("Use WifiPhyState::CCA_BUSY instead")
static constexpr auto CCA_BUSY = ns3::WifiPhyState::CCA_BUSY;
NS_DEPRECATED_3_42("Use WifiPhyState::TX instead")
static constexpr auto TX = ns3::WifiPhyState::TX;
NS_DEPRECATED_3_42("Use WifiPhyState::RX instead")
static constexpr auto RX = ns3::WifiPhyState::RX;
NS_DEPRECATED_3_42("Use WifiPhyState::SWITCHING instead")
static constexpr auto SWITCHING = ns3::WifiPhyState::SWITCHING;
NS_DEPRECATED_3_42("Use WifiPhyState::SLEEP instead")
static constexpr auto SLEEP = ns3::WifiPhyState::SLEEP;
NS_DEPRECATED_3_42("Use WifiPhyState::OFF instead")
static constexpr auto OFF = ns3::WifiPhyState::OFF;
/**@}*/

#endif /* WIFI_PHY_STATE_H */
