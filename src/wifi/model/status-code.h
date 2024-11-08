/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef STATUS_CODE_H
#define STATUS_CODE_H

#include "ns3/buffer.h"

namespace ns3
{

/**
 * Status code for association response.
 */
class StatusCode
{
  public:
    StatusCode();
    /**
     * Set success bit to 0 (success).
     */
    void SetSuccess();
    /**
     * Set success bit to 1 (failure).
     */
    void SetFailure();

    /**
     * Return whether the status code is success.
     *
     * @return true if success,
     *         false otherwise
     */
    bool IsSuccess() const;

    /**
     * @returns the expected size of the status code.
     *
     * This method should return the number of bytes which are needed to store
     * the status code data by Serialize.
     */
    uint32_t GetSerializedSize() const;
    /**
     * @param start an iterator which points to where the status code should be written
     *
     * @return Buffer::Iterator
     *
     * This method is used to store a status code into the byte buffer.
     * The data written is expected to match bit-for-bit the representation of this
     * header in a real network.
     */
    Buffer::Iterator Serialize(Buffer::Iterator start) const;
    /**
     * @param start an iterator which points to where the status code should be read.
     *
     * @returns the number of bytes read.
     *
     * This method is used to re-create a status code from the byte buffer.
     * The data read is expected to match bit-for-bit the representation of this
     * header in real networks.
     */
    Buffer::Iterator Deserialize(Buffer::Iterator start);

  private:
    uint16_t m_code; ///< status code
};

/**
 * Serialize StatusCode to the given ostream.
 *
 * @param os the output stream
 * @param code the StatusCode
 *
 * @return std::ostream
 */
std::ostream& operator<<(std::ostream& os, const StatusCode& code);

} // namespace ns3

#endif /* STATUS_CODE_H */
