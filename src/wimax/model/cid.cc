/*
 * Copyright (c) 2007,2008,2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                               <amine.ismail@UDcast.com>
 */

#include "cid.h"

// 0 will match IR CID, -1 will match broadcast CID 0xFFFF, hence 60000
#define CID_UNINITIALIZED 60000

namespace ns3
{

Cid::Cid()
{
    m_identifier = CID_UNINITIALIZED;
}

Cid::Cid(uint16_t identifier)
{
    m_identifier = identifier;
}

Cid::~Cid()
{
}

uint16_t
Cid::GetIdentifier() const
{
    return m_identifier;
}

bool
Cid::IsMulticast() const
{
    return m_identifier >= 0xff00 && m_identifier <= 0xfffd;
}

bool
Cid::IsBroadcast() const
{
    return *this == Broadcast();
}

bool
Cid::IsPadding() const
{
    return *this == Padding();
}

bool
Cid::IsInitialRanging() const
{
    return *this == InitialRanging();
}

Cid
Cid::Broadcast()
{
    return 0xffff;
}

Cid
Cid::Padding()
{
    return 0xfffe;
}

Cid
Cid::InitialRanging()
{
    return 0;
}

/**
 * @brief equality operator
 * @param lhs left hand side
 * @param rhs right hand side
 * @returns true if equal
 */
bool
operator==(const Cid& lhs, const Cid& rhs)
{
    return lhs.m_identifier == rhs.m_identifier;
}

/**
 * @brief inequality operator
 * @param lhs left hand side
 * @param rhs right hand side
 * @returns true if not equal
 */
bool
operator!=(const Cid& lhs, const Cid& rhs)
{
    return !(lhs == rhs);
}

/**
 * @brief output stream output operator
 * @param os output stream
 * @param cid CID
 * @returns output stream
 */
std::ostream&
operator<<(std::ostream& os, const Cid& cid)
{
    os << cid.GetIdentifier();
    return os;
}

} // namespace ns3
