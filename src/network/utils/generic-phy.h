/*
 * Copyright (c)  2010 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef GENERIC_PHY_H
#define GENERIC_PHY_H

#include "ns3/callback.h"

namespace ns3
{

class Packet;

/**
 * This method allows the MAC to instruct the PHY to start a
 * transmission of a given packet
 *
 * @param packet the Packet to be transmitted
 * @return this method returns false if the PHY will start TX,
 * true if the PHY refuses to start the TX. If false, the MAC layer
 * will expect that GenericPhyTxEndCallback is invoked at some point later.
 */
typedef Callback<bool, Ptr<Packet>> GenericPhyTxStartCallback;

/**
 * this method is invoked by the PHY to notify the MAC that the
 * transmission of a given packet has been completed.
 *
 * @param packet the Packet whose TX has been completed.
 */
typedef Callback<void, Ptr<const Packet>> GenericPhyTxEndCallback;

/**
 * This method is used by the PHY to notify the MAC that a RX
 * attempt is being started, i.e., a valid signal has been
 * recognized by the PHY.
 *
 */
typedef Callback<void> GenericPhyRxStartCallback;

/**
 * This method is used by the PHY to notify the MAC that a
 * previously started RX attempt has terminated without success.
 */
typedef Callback<void> GenericPhyRxEndErrorCallback;

/**
 * This method is used by the PHY to notify the MAC that a
 * previously started RX attempt has been successfully completed.
 *
 * @param packet the received Packet
 */
typedef Callback<void, Ptr<Packet>> GenericPhyRxEndOkCallback;

} // namespace ns3

#endif /* GENERIC_PHY_H */
