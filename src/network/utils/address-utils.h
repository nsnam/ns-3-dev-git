/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef ADDRESS_UTILS_H
#define ADDRESS_UTILS_H

#include "ipv4-address.h"
#include "ipv6-address.h"
#include "mac16-address.h"
#include "mac48-address.h"
#include "mac64-address.h"

#include "ns3/address.h"
#include "ns3/buffer.h"

namespace ns3
{

/**
 * @brief Write an Ipv4Address to a Buffer
 * @param i a reference to the buffer to write to
 * @param ad the Ipv4Address
 */
void WriteTo(Buffer::Iterator& i, Ipv4Address ad);

/**
 * @brief Write an Ipv4Address to a Buffer
 * @param i a reference to the buffer to write to
 * @param ad the Ipv6Address
 */
void WriteTo(Buffer::Iterator& i, Ipv6Address ad);

/**
 * @brief Write an Address to a Buffer
 * @param i a reference to the buffer to write to
 * @param ad the Address
 */
void WriteTo(Buffer::Iterator& i, const Address& ad);

/**
 * @brief Write an Mac64Address to a Buffer
 * @param i a reference to the buffer to write to
 * @param ad the Mac64Address
 */
void WriteTo(Buffer::Iterator& i, Mac64Address ad);

/**
 * @brief Write an Mac48Address to a Buffer
 * @param i a reference to the buffer to write to
 * @param ad the Mac48Address
 */
void WriteTo(Buffer::Iterator& i, Mac48Address ad);

/**
 * @brief Write an Mac16Address to a Buffer
 * @param i a reference to the buffer to write to
 * @param ad the Mac16Address
 */
void WriteTo(Buffer::Iterator& i, Mac16Address ad);

/**
 * @brief Read an Ipv4Address from a Buffer
 * @param i a reference to the buffer to read from
 * @param ad a reference to the Ipv4Address to be read
 */
void ReadFrom(Buffer::Iterator& i, Ipv4Address& ad);

/**
 * @brief Read an Ipv6Address from a Buffer
 * @param i a reference to the buffer to read from
 * @param ad a reference to the Ipv6Address to be read
 */
void ReadFrom(Buffer::Iterator& i, Ipv6Address& ad);

/**
 * @brief Read an Address from a Buffer
 * @param i a reference to the buffer to read from
 * @param ad a reference to the Address to be read
 * @param len the length of the Address
 */
void ReadFrom(Buffer::Iterator& i, Address& ad, uint32_t len);

/**
 * @brief Read a Mac64Address from a Buffer
 * @param i a reference to the buffer to read from
 * @param ad a reference to the Mac64Address to be read
 */
void ReadFrom(Buffer::Iterator& i, Mac64Address& ad);

/**
 * @brief Read a Mac48Address from a Buffer
 * @param i a reference to the buffer to read from
 * @param ad a reference to the Mac48Address to be read
 */
void ReadFrom(Buffer::Iterator& i, Mac48Address& ad);

/**
 * @brief Read a Mac16Address from a Buffer
 * @param i a reference to the buffer to read from
 * @param ad a reference to the Mac16Address to be read
 */
void ReadFrom(Buffer::Iterator& i, Mac16Address& ad);

namespace addressUtils
{

/**
 * @brief Address family-independent test for a multicast address
 * @param ad the address to test
 * @return true if the address is a multicast address, false otherwise
 */
bool IsMulticast(const Address& ad);

/**
 * @brief Convert IPv4/IPv6 address with port to a socket address
 * @param address the address to convert
 * @param port the port
 * @return the corresponding socket address
 */
Address ConvertToSocketAddress(const Address& address, uint16_t port);

}; // namespace addressUtils

}; // namespace ns3

#endif /* ADDRESS_UTILS_H */
