/*
 * Copyright (c) 2008 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ENCODE_DECODE_H
#define ENCODE_DECODE_H

#include <cstdint>
#include <string>

namespace ns3
{

std::string BufferToString(uint8_t* buffer, uint32_t len);
bool StringToBuffer(std::string s, uint8_t* buffer, uint32_t* len);

} // namespace ns3

#endif /* ENCODE_DECODE_H */
