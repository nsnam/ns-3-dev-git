/*
 * Copyright (c) 2007,2008,2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                               <amine.ismail@UDcast.com>
 */

#ifndef SS_MANAGER_H
#define SS_MANAGER_H

#include "cid.h"
#include "ss-record.h"

#include <stdint.h>

namespace ns3
{

/**
 * @ingroup wimax
 * @brief this class manages a list of SSrecords
 * @see SSrecord
 */
class SSManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    SSManager();
    ~SSManager() override;
    /**
     * Create SS record
     * @param macAddress the MAC address
     * @returns pointer to the SS record
     */
    SSRecord* CreateSSRecord(const Mac48Address& macAddress);
    /**
     * Get SS record
     * @param macAddress the MAC address
     * @returns pointer to the SS record
     */
    SSRecord* GetSSRecord(const Mac48Address& macAddress) const;
    /**
     * @brief returns the ssrecord which has been assigned this cid. Since
     *        different types of cids (basic, primary, transport) are assigned
     *        different values, all cids (basic, primary and transport) of the
     *        ssrecord are matched.
     * @param cid the cid to be matched
     * @return pointer to the ss record matching the cid
     */
    SSRecord* GetSSRecord(Cid cid) const;
    /**
     * Get SS records
     * @returns a vector of pointers to the SS records
     */
    std::vector<SSRecord*>* GetSSRecords() const;
    /**
     * Check if address is in record
     * @param macAddress the MAC address
     * @returns whether the address is in the record
     */
    bool IsInRecord(const Mac48Address& macAddress) const;
    /**
     * Check if address is registered
     * @param macAddress the MAC address
     * @returns whether the address is registered
     */
    bool IsRegistered(const Mac48Address& macAddress) const;
    /**
     * Delete SS record
     * @param cid the CID
     */
    void DeleteSSRecord(Cid cid);
    /**
     * Get MAC address by CID
     * @param cid the CID
     * @returns the MAC address
     */
    Mac48Address GetMacAddress(Cid cid) const;
    /**
     * Get number of SSs
     * @returns the number of SSs
     */
    uint32_t GetNSSs() const;
    /**
     * Get number of registered SSs
     * @returns the number of registered SSs
     */
    uint32_t GetNRegisteredSSs() const;

  private:
    std::vector<SSRecord*>* m_ssRecords; ///< the SS records
};

} // namespace ns3

#endif /* SS_MANAGER_H */
