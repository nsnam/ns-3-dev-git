/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Joe Kopena <tjkopena@cs.drexel.edu>
 */

#ifndef TIMESTAMP_TAG_H
#define TIMESTAMP_TAG_H

#include "ns3/nstime.h"
#include "ns3/tag-buffer.h"
#include "ns3/tag.h"
#include "ns3/type-id.h"

#include <iostream>

namespace ns3
{

/**
 * Timestamp tag for associating a timestamp with a packet.
 *
 * It would have been more realistic to include this info in
 * a header. Here we show how to avoid the extra overhead in
 * a simulation.
 */
class TimestampTag : public Tag
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Construct a new TimestampTag object
     */
    TimestampTag();

    /**
     * @brief Construct a new TimestampTag object with the given timestamp
     * @param timestamp The timestamp
     */
    TimestampTag(Time timestamp);

    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

    /**
     * @brief Get the Timestamp object
     * @return Time for this tag
     */
    Time GetTimestamp() const;

    /**
     * @brief Set the Timestamp object
     * @param timestamp Timestamp to assign to tag
     */
    void SetTimestamp(Time timestamp);

  private:
    Time m_timestamp{0}; //!< Timestamp
};

} // namespace ns3

#endif // TIMESTAMP_TAG_H
