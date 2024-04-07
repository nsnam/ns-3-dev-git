/*
 * Copyright (c) 2020 University of Washington
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
 * Authors: Rohan Patidar <rpatidar@uw.edu>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *          Sian Jin <sianjin@uw.edu>
 */

// This file contains table data for the TableBasedErrorRateModel.  For more
// information on the source of this data, see wifi module documentation.

#ifndef ERROR_RATE_TABLES_H
#define ERROR_RATE_TABLES_H

#include <cstdint>
#include <utility>
#include <vector>

namespace ns3
{

const uint16_t ERROR_TABLE_BCC_SMALL_FRAME_SIZE =
    32; //!< reference size (bytes) of small frames for BCC
const uint16_t ERROR_TABLE_BCC_LARGE_FRAME_SIZE =
    1458; //!< reference size (bytes) of large frames for BCC
const uint16_t ERROR_TABLE_LDPC_FRAME_SIZE = 1458; //!< reference size (bytes) for LDPC
const uint8_t ERROR_TABLE_BCC_MAX_NUM_MCS = 10;    //!< maximum number of MCSs for BCC
const uint8_t ERROR_TABLE_LDPC_MAX_NUM_MCS = 12;   //!< maximum number of MCSs for LDPC

/// Table of SNR (dB) and PER pairs
typedef std::vector<std::pair<double /* SNR (dB) */, double /* PER */>> SnrPerTable;

/// AWGN error table for BCC with reference size of 32 bytes
extern const SnrPerTable AwgnErrorTableBcc32[ERROR_TABLE_BCC_MAX_NUM_MCS];

/// AWGN error table for BCC with reference size of 1458 bytes
extern const SnrPerTable AwgnErrorTableBcc1458[ERROR_TABLE_BCC_MAX_NUM_MCS];

/// AWGN error table for LDPC with reference size of 1458 bytes
extern const SnrPerTable AwgnErrorTableLdpc1458[ERROR_TABLE_LDPC_MAX_NUM_MCS];

} // namespace ns3

#endif /* ERROR_RATE_TABLES_H */
