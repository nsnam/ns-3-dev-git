/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef TAP_ENCODE_DECODE_H
#define TAP_ENCODE_DECODE_H

#include <cstdint>
#include <string>

namespace ns3
{

std::string TapBufferToString(uint8_t* buffer, uint32_t len);
bool TapStringToBuffer(std::string s, uint8_t* buffer, uint32_t* len);

} // namespace ns3

#endif /* TAP_ENCODE_DECODE_H */
