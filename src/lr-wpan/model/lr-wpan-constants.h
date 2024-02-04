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
 *  Erwan Livolant <erwan.livolant@inria.fr>
 *  Alberto Gallegos Ramonet <ramonet@fc.ritsumei.ac.jp>
 */

#ifndef LR_WPAN_CONSTANTS_H
#define LR_WPAN_CONSTANTS_H

#include <cstdint>

namespace ns3
{
namespace lrwpan
{

/**
 * \defgroup LrWpanConstants LR-WPAN common parameters
 * \ingroup lr-wpan
 *
 * Contains common parameters about LR-WPAN that can be reused in multiple files.
 * These parameters are defined in the IEEE 802.15.4-2011 standard and adopt the
 * same names as in the standard.
 *
 * @{
 */

///////////////////
// PHY constants //
///////////////////

/**
 * The maximum packet size accepted by the PHY.
 * See Table 22 in section 6.4.1 of IEEE 802.15.4-2006
 */
constexpr uint32_t aMaxPhyPacketSize{127};

/**
 * The turnaround time in symbol periods for switching the transceiver from RX to TX or
 * vice-versa.
 * See Table 22 in section 6.4.1 of IEEE 802.15.4-2006
 */
constexpr uint32_t aTurnaroundTime{12};

///////////////////
// MAC constants //
///////////////////

/**
 * The minimum number of octets added by the MAC sublayer to the PSDU.
 * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
 */
constexpr uint32_t aMinMPDUOverhead{9};

/**
 * Length of a superframe slot in symbols. Defaults to 60 symbols in each superframe slot.
 * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
 */
constexpr uint32_t aBaseSlotDuration{60};

/**
 * Number of a superframe slots per superframe. Defaults to 16.
 * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
 */
constexpr uint32_t aNumSuperframeSlots{16};

/**
 * Length of a superframe in symbols. Defaults to aBaseSlotDuration * aNumSuperframeSlots in
 * symbols.
 * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
 */
constexpr uint32_t aBaseSuperframeDuration{aBaseSlotDuration * aNumSuperframeSlots};

/**
 * The number of consecutive lost beacons that will cause the MAC sublayer of a receiving device to
 * declare a loss of synchronization.
 * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
 */
constexpr uint32_t aMaxLostBeacons{4};

/**
 * The maximum size of an MPDU, in octets, that can be followed by a Short InterFrame Spacing (SIFS)
 * period.
 * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
 */
constexpr uint32_t aMaxSIFSFrameSize{18};

/**
 * Number of symbols per CSMA/CA time unit, default 20 symbols.
 */
constexpr uint32_t aUnitBackoffPeriod{20};

/**
 * The maximum number of octets added by the MAC sublayer to the MAC payload o a a beacon frame.
 * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
 */
constexpr uint32_t aMaxBeaconOverhead{75};

/**
 * The maximum size, in octets, of a beacon payload.
 * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
 */
constexpr uint32_t aMaxBeaconPayloadLength{aMaxPhyPacketSize - aMaxBeaconOverhead};

/** @} */

} // namespace lrwpan
} // namespace ns3

#endif // LR_WPAN_CONSTANTS_H
