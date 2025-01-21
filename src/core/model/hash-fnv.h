/*
 * Copyright (c) 2012 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#ifndef HASH_FNV_H
#define HASH_FNV_H

#include "hash-function.h"

/**
 * @file
 * @ingroup hash
 * @brief ns3::Hash::Function::Fnv1a declaration.
 */

namespace ns3
{

namespace Hash
{

namespace Function
{

/**
 *  @ingroup hash
 *
 *  @brief Fnv1a hash function implementation
 *
 *  This is the venerable Fowler-Noll-Vo hash, version 1A.  (See the
 *  <a href="http://isthe.com/chongo/tech/comp/fnv/">FNV page</a>.)
 *
 *  The implementation here is taken directly from the published FNV
 *  <a href="http://isthe.com/chongo/tech/comp/fnv/#FNV-reference-source">
 *  reference code</a>,
 *  with minor modifications to wrap into this class.  See the
 *  hash-fnv.cc file for details.
 *
 */
class Fnv1a : public Implementation
{
  public:
    /**
     * Constructor
     */
    Fnv1a();
    /**
     * Compute 32-bit hash of a byte buffer
     *
     * Call clear () between calls to GetHash32() to reset the
     * internal state and hash each buffer separately.
     *
     * If you don't call clear() between calls to GetHash32,
     * you can hash successive buffers.  The final return value
     * will be the cumulative hash across all calls.
     *
     * @param [in] buffer pointer to the beginning of the buffer
     * @param [in] size length of the buffer, in bytes
     * @return 32-bit hash of the buffer
     */
    uint32_t GetHash32(const char* buffer, const size_t size) override;
    /**
     * Compute 64-bit hash of a byte buffer.
     *
     * Call clear () between calls to GetHash64() to reset the
     * internal state and hash each buffer separately.
     *
     * If you don't call clear() between calls to GetHash64,
     * you can hash successive buffers.  The final return value
     * will be the cumulative hash across all calls.
     *
     * @param [in] buffer pointer to the beginning of the buffer
     * @param [in] size length of the buffer, in bytes
     * @return 64-bit hash of the buffer
     */
    uint64_t GetHash64(const char* buffer, const size_t size) override;
    /**
     * Restore initial state
     */
    void clear() override;

  private:
    /**
     * Seed value
     */
    static constexpr auto SEED{0x8BADF00D}; // Ate bad food

    /** Cache last hash value, for incremental hashing. */
    /**@{*/
    uint32_t m_hash32;
    uint64_t m_hash64;
    /**@}*/
};

} // namespace Function

} // namespace Hash

} // namespace ns3

#endif /* HASH_FNV_H */
