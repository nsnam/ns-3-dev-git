/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Kashif Nadeem <kshfnadeem@gmail.com>
 *          Matthieu Coudron <matthieu.coudron@lip6.fr>
 *
 */

#ifndef MPTCP_CRYPTO_H
#define MPTCP_CRYPTO_H

  /**
   * Token:  A locally unique identifier given to a multipath connection
   * by a host.  May also be referred to as a "Connection ID".
   *
   * In this specification, with only the SHA-1 algorithm
   * (bit "H") specified and selected, the token MUST be a truncated (most
   * significant 32 bits) SHA-1 hash ([4], [15]) of the key.  A different,
   * 64-bit truncation (the least significant 64 bits) of the SHA-1 hash
   * of the key MUST be used as the initial data sequence number.  Note
   * that the key MUST be hashed in network byte order.  Also note that
   * the "least significant" bits MUST be the rightmost bits of the SHA-1
   * digest, as per [4].  Future specifications of the use of the crypto
   * bits may choose to specify different algorithms for token and IDSN
   * generation.
   */
namespace ns3
{
  /**
   * \brief Only SHA1 is defined in the RFC up to now.
   */
  enum mptcp_crypto_alg_t
  {
    HMAC_SHA1 = 1 /**< Default choice */
    /* more may come in the future depending on the standardization */
  };

  /**
   * \brief This function generates the token and idsn based on the passed key
   *
   * \note This function operates in different modes depending on if the library libgcrypt
   *   was available when running ./waf config . The result conforms to the standard when libgcrypt
   *  is present, otherwise it relies on a simpler incompatible ns3 implementation.
   *
   * In the case of sha1 (only one standardized), the token MUST be a truncated (most
   * significant 32 bits) SHA-1 hash according to \rfc{6824}.
   * The least significant 64 bits of the SHA-1 hash
   * of the key MUST be used as the initial data sequence number.
   *
   * \param alg The hmac algorith m to use to generate the hash
   * \param key Given key for a connection
   * \param token Resulting token generated from the key
   * \param idsn Resulting initial data sequence number generated from the key
   */
  void
  GenerateTokenForKey( mptcp_crypto_alg_t alg, uint64_t key, uint32_t& token, uint64_t& idsn);
}

#endif
