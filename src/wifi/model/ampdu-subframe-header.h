/*
 * Copyright (c) 2013
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ghada Badawy <gbadawy@gmail.com>
 */

#ifndef AMPDU_SUBFRAME_HEADER_H
#define AMPDU_SUBFRAME_HEADER_H

#include "ns3/header.h"

namespace ns3
{

/**
 * @ingroup wifi
 * @brief Headers for A-MPDU subframes
 */
class AmpduSubframeHeader : public Header
{
  public:
    AmpduSubframeHeader();
    ~AmpduSubframeHeader() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * Set the length field.
     *
     * @param length in bytes
     */
    void SetLength(uint16_t length);
    /**
     * Set the EOF field.
     *
     * @param eof set EOF field if true
     */
    void SetEof(bool eof);
    /**
     * Return the length field.
     *
     * @return the length field in bytes
     */
    uint16_t GetLength() const;
    /**
     * Return the EOF field.
     *
     * @return the EOF field
     */
    bool GetEof() const;
    /**
     * Return whether the pattern stored in the delimiter
     * signature field is correct, i.e. corresponds to the
     * unique pattern 0x4E.
     *
     * @return true if the signature is valid, false otherwise
     */
    bool IsSignatureValid() const;

  private:
    uint16_t m_length;   //!< length field in bytes
    bool m_eof;          //!< EOF field
    uint8_t m_signature; //!< delimiter signature (should correspond to pattern 0x4E in order to be
                         //!< assumed valid)
};

} // namespace ns3

#endif /* AMPDU_SUBFRAME_HEADER_H */
