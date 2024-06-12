/*
 * Copyright (c) 2020 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Rohan Patidar <rpatidar@uw.edu>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *          Sian Jin <sianjin@uw.edu>
 */

// This file contains table data for the TableBasedErrorRateModel.  For more
// information on the source of this data, see wifi module documentation.

#ifndef ERROR_RATE_TABLES_H
#define ERROR_RATE_TABLES_H

#include "ns3/wifi-units.h"

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

/// Table of SNR and PER pairs
typedef std::vector<std::pair<dB_u /* SNR */, double /* PER */>> SnrPerTable;

/// AWGN error table for BCC with reference size of 32 bytes
extern const SnrPerTable AwgnErrorTableBcc32[ERROR_TABLE_BCC_MAX_NUM_MCS];

/// AWGN error table for BCC with reference size of 1458 bytes
extern const SnrPerTable AwgnErrorTableBcc1458[ERROR_TABLE_BCC_MAX_NUM_MCS];

/// AWGN error table for LDPC with reference size of 1458 bytes
extern const SnrPerTable AwgnErrorTableLdpc1458[ERROR_TABLE_LDPC_MAX_NUM_MCS];

} // namespace ns3

#endif /* ERROR_RATE_TABLES_H */
