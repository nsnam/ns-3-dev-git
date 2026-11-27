/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef ADDBA_EXTENSION_H
#define ADDBA_EXTENSION_H

#include "wifi-information-element.h"

#include "ns3/wifi-export.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * The IEEE 802.11 ADDBA Extension Element (Sec. 9.4.2.139 of 802.11-2020)
 */
class WIFI_EXPORT AddbaExtension : public WifiInformationElement
{
  public:
    /**
     * ADDBA Extended Parameter Set
     */
    struct WIFI_EXPORT ExtParamSet
    {
        /**
         * @returns the expected size of the ADDBA Extended Parameter Set field in bytes
         *
         * This method should return the number of bytes which are needed to store
         * the ADDBA Extended Parameter Set data by Serialize.
         */
        uint32_t GetSerializedSize() const;

        /**
         * @param start an iterator which points to where the ADDBA Extended Parameter Set should be
         * written
         *
         * @return Buffer::Iterator
         *
         * This method is used to store a ADDBA Extended Parameter Set into the byte buffer.
         * The data written is expected to match bit-for-bit the representation of this
         * header in a real network.
         */
        Buffer::Iterator Serialize(Buffer::Iterator start) const;

        /**
         * @param start an iterator which points to where the ADDBA Extended Parameter Set should be
         * read.
         *
         * @returns the number of bytes read.
         *
         * This method is used to re-create a ADDBA Extended Parameter Set from the byte buffer.
         * The data read is expected to match bit-for-bit the representation of this
         * header in real networks.
         */
        Buffer::Iterator Deserialize(Buffer::Iterator start);

        uint8_t noFragment : 1 {0};   //!< reserved when transmitted by HE STA to HE STA
        uint8_t heFragmentOp : 2 {0}; //!< indicates level of HE dynamic fragmentation (unsupported)
        uint8_t reserved : 2 {0};     //!< reserved
        uint8_t extBufferSize : 3 {0}; //!< extended buffer size

        /**
         * @brief Equality operator for ADDBA Extended Parameter Set
         * @param rhs right hand side
         * @return true if the ADDBA Extended Parameter Sets are equal, false otherwise
         */
        bool operator==(const ExtParamSet& rhs) const = default;
    };

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    ExtParamSet m_extParamSet{}; //!< ADDBA Extended Parameter Set field

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

/**
 * Serialize ADDBA Extended Parameter Set to the given ostream.
 *
 * @param os the output stream
 * @param params the ADDBA Extended Parameter Set to serialize
 *
 * @return std::ostream
 */
WIFI_EXPORT std::ostream& operator<<(std::ostream& os, const AddbaExtension::ExtParamSet& params);

} // namespace ns3

#endif /* ADDBA_EXTENSION_H */
