/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
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
 * Author: Davide Magrin <davide@magr.in>
 */

#ifndef TIM_H
#define TIM_H

#include "wifi-information-element.h"

#include <set>

namespace ns3
{

/**
 * \brief The Traffic Indication Map Information Element
 * \ingroup wifi
 *
 * The 802.11 Traffic Indication Map (see section 9.4.2.5 of 802.11-2020)
 *
 * Note: The current implementation does not support S1G operation, or
 * multiple BSSID.
 */
class Tim : public WifiInformationElement
{
  public:
    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /**
     * Add the provided AID value to the list contained in the Virtual Bitmap
     *
     * \param aid the AID value to add to this TIM's Virtual Bitmap
     */
    void AddAid(uint16_t aid);

    /**
     * Add the AID values in the provided iterator range to the list contained
     * in the Virtual Bitmap
     *
     * \tparam Iterator Type of iterator
     * \param begin Starting position of the iterator range
     * \param end Ending position of the iterator range
     */
    template <typename Iterator>
    void AddAid(Iterator begin, Iterator end);

    /**
     * Check whether the bit corresponding to the provided AID is set in the
     * Virtual Bitmap included in this TIM
     *
     * \param aid The AID value to look for
     * \return True if the AID value is found in the Virtual Bitmap, false otherwise
     */
    bool HasAid(uint16_t aid) const;

    /**
     * Return the AID values, greater than the given AID value, whose corresponding bits are set
     * in the virtual bitmap.
     *
     * \param aid the given AID value
     * \return the AID values, greater than the given AID value, whose corresponding bits are set
     *         in the virtual bitmap
     */
    std::set<uint16_t> GetAidSet(uint16_t aid = 0) const;

    /**
     * Get the Partial Virtual Bitmap offset, i.e., the number (denoted as N1 by the specs) of
     * the first octet included in the Partial Virtual Bitmap. Note that the Bitmap Offset
     * subfield contains the number N1/2.
     *
     * \return the Partial Virtual Bitmap offset
     */
    uint8_t GetPartialVirtualBitmapOffset() const;

    /**
     * \return the last non-zero octet in the virtual bitmap (denoted as N2 by the specs)
     */
    uint8_t GetLastNonZeroOctetIndex() const;

    uint8_t m_dtimCount{0};            //!< The DTIM Count field
    uint8_t m_dtimPeriod{0};           //!< The DTIM Period field
    bool m_hasMulticastPending{false}; //!< Whether there is Multicast / Broadcast data

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    /**
     * Obtain the index of the octet where the provided AID value should be
     * set in the Virtual Bitmap
     *
     * \param aid the provided AID value
     * \return the index of the octet where the provided AID value should be
     *         set in the Virtual Bitmap
     */
    uint8_t GetAidOctetIndex(uint16_t aid) const;

    /**
     * Obtain an octet with a set bit, corresponding to the provided AID value
     *
     * \param aid the provided AID value
     * \return an octet with a set bit, corresponding to the provided AID value
     */
    uint8_t GetAidBit(uint16_t aid) const;

    /**
     * Obtain the AID value represented by a certain octet index and bit
     * position inside the Virtual Bitmap
     *
     * \param octet the octet index in the Virtual Bitmap
     * \param position the bit position in the octet of the Virtual Bitmap
     * \return the corresponding AID value
     */
    uint16_t GetAidFromOctetIndexAndBitPosition(uint16_t octet, uint8_t position) const;

    /**
     * The Bitmap Control field is optional if the TIM is carried in an S1G PPDU, while
     * it is always present when the TIM is carried in a non-S1G PPDU.
     *
     * \return the value of the Bitmap Control field
     */
    uint8_t GetBitmapControl() const;

    /**
     * \return a vector containing the Partial Virtual Bitmap octets
     */
    std::vector<uint8_t> GetPartialVirtualBitmap() const;

    std::set<uint16_t> m_aidValues; //!< List of AID values included in this TIM
};

template <typename Iterator>
void
Tim::AddAid(Iterator begin, Iterator end)
{
    for (auto& it = begin; it != end; it++)
    {
        AddAid(*it);
    }
}

} // namespace ns3

#endif /* TIM_H */
