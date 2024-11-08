/*
 * Copyright (c) 2007,2008,2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                               <amine.ismail@UDcast.com>
 */

#ifndef CID_H
#define CID_H

#include <ostream>
#include <stdint.h>

namespace ns3
{

/**
 * @ingroup wimax
 * @class Cid
 * @brief Cid class
 */
class Cid
{
  public:
    /// Type enumeration
    enum Type
    {
        BROADCAST = 1,
        INITIAL_RANGING,
        BASIC,
        PRIMARY,
        TRANSPORT,
        MULTICAST,
        PADDING
    };

    /// Create a cid of unknown type
    Cid();
    /**
     * Constructor
     *
     * @param cid
     */
    Cid(uint16_t cid);
    ~Cid();
    /**
     * @return the identifier of the cid
     */
    uint16_t GetIdentifier() const;
    /**
     * @return true if the cid is a multicast cid, false otherwise
     */
    bool IsMulticast() const;
    /**
     * @return true if the cid is a broadcast cid, false otherwise
     */
    bool IsBroadcast() const;
    /**
     * @return true if the cid is a padding cid, false otherwise
     */
    bool IsPadding() const;
    /**
     * @return true if the cid is an initial ranging cid, false otherwise
     */
    bool IsInitialRanging() const;
    /**
     * @return the broadcast cid
     */
    static Cid Broadcast();
    /**
     * @return the padding cid
     */
    static Cid Padding();
    /**
     * @return the initial ranging cid
     */
    static Cid InitialRanging();

  private:
    /// allow CidFactory class friend access
    friend class CidFactory;
    /// equality operator
    friend bool operator==(const Cid& lhs, const Cid& rhs);
    uint16_t m_identifier; ///< identiifier
};

bool operator==(const Cid& lhs, const Cid& rhs);
bool operator!=(const Cid& lhs, const Cid& rhs);

std::ostream& operator<<(std::ostream& os, const Cid& cid);

} // namespace ns3

#endif /* CID_H */
