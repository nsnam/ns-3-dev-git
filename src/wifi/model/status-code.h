/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef STATUS_CODE_H
#define STATUS_CODE_H

#include "ns3/buffer.h"
#include "ns3/wifi-export.h"

namespace ns3
{

/// Size in bytes of Status Code field, IEEE 802.11-2024 Figure 9-147
constexpr uint32_t WIFI_STATUS_CODE_SIZE_B = 2;

/// Status codes as defined in 802.11-2024, Table 9-80
enum class StatusCodeValue : uint16_t
{
    SUCCESS = 0,
    REFUSED_REASON_UNSPECIFIED = 1,
    REQUEST_DECLINED = 37,                             // IEEE 802.11-2024 Table 9-80
    REJECTED_ST_PREP_MAX_PREP_EXCEEDED = 154,          // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_ST_EXEC_EXCEEDED_PREP_TIMEOUT = 155,      // IEEE 802.11bn D2.0 Table 9-96
    DENIED_UHR_NOT_SUPPORTED = 157,                    // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_ST_EXEC_TARGET_NOT_PREPARED = 158,        // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_INVALID_SECURITY_PROFILE = 159,           // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_TARGET_AP_UNKNOWN = 160,                  // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_TARGET_AP_NOT_IN_SAME_SMD = 161,          // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_NEIGHBOR_AP_INFO_NOT_AVAILABLE = 162,     // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_ST_PREP_NO_LINK_ACCEPTED = 163,           // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_ST_PREP_TARGET_AP_ALREADY_PREPARED = 164, // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_ST_EXEC_ALREADY_PERFORMED = 165,          // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_ST_PREP_MISSING_PTK_GEN_PARAMETERS = 171, // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_MAPC_SCHEME_AP_PARAMETERS = 172,          // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_MAPC_AGREEMENT_LIMIT_EXCEEDED = 173,      // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_UNSATISFACTORY_COORDINATION = 174,        // IEEE 802.11bn D2.0 Table 9-96
    REJECTED_WITH_SUGGESTED_MAPC_PARAMETERS = 175,     // IEEE 802.11bn D2.0 Table 9-96

    // Other status codes currently not implemented
};

/**
 * Status code representation
 */
struct WIFI_EXPORT StatusCode
{
    /**
     * Constructor
     * @param code initial status code value
     */
    explicit StatusCode(StatusCodeValue code);
    StatusCode() = default;
    ~StatusCode() = default;

    /**
     * Set status code to SUCCESS
     */
    void SetSuccess();

    /**
     * Set status code to a failure value
     * @param code status code value, if not provided REFUSED_REASON_UNSPECIFIED is used
     */
    void SetFailure(StatusCodeValue code = StatusCodeValue::REFUSED_REASON_UNSPECIFIED);

    /**
     * Return whether the status code is SUCCESS.
     *
     * @return true if status code is SUCCESS,
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

    StatusCodeValue m_code{StatusCodeValue::SUCCESS}; ///< status code
};

/**
 * Serialize StatusCode to the given ostream.
 *
 * @param os the output stream
 * @param code the StatusCode
 *
 * @return std::ostream
 */
WIFI_EXPORT std::ostream& operator<<(std::ostream& os, const StatusCode& code);

} // namespace ns3

#endif /* STATUS_CODE_H */
