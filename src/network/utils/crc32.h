/*
 * Copyright (c) 2013 PIOTR JURKIEWICZ
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Piotr Jurkiewicz <piotr.jerzy.jurkiewicz@gmail.com>
 */
#ifndef CRC32_H
#define CRC32_H
#include <stdint.h>

namespace ns3
{

/**
 * Calculates the CRC-32 for a given input
 *
 * @param data buffer to calculate the checksum for
 * @param length the length of the buffer (bytes)
 * @returns the computed crc-32.
 *
 */
uint32_t CRC32Calculate(const uint8_t* data, int length);

} // namespace ns3

#endif
