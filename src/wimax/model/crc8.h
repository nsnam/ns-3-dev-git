/*
 * Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */
#ifndef CRC8_H
#define CRC8_H
#include <stdint.h>

namespace ns3
{

/**
 * @param data buffer to calculate the checksum for
 * @param length the length of the buffer (bytes)
 * @returns the computed crc.
 *
 */
uint8_t CRC8Calculate(const uint8_t* data, int length);

} // namespace ns3

#endif
