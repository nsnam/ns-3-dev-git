/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef EHT_CAPABILITIES_H
#define EHT_CAPABILITIES_H

#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * EHT MAC Capabilities Info subfield.
 * See IEEE 802.11be D1.5 9.4.2.313.2 EHT MAC Capabilities Information subfield
 */
struct EhtMacCapabilities
{
    uint8_t epcsPriorityAccessSupported : 1;      //!< EPCS Priority Access Supported
    uint8_t ehtOmControlSupport : 1;              //!< EHT OM Control Support
    uint8_t triggeredTxopSharingMode1Support : 1; //!< Triggered TXOP Sharing Mode 1 Support
    uint8_t triggeredTxopSharingMode2Support : 1; //!< Triggered TXOP Sharing Mode 2 Support
    uint8_t restrictedTwtSupport : 1;             //!< Restricted TWT Support
    uint8_t scsTrafficDescriptionSupport : 1;     //!< SCS Traffic Description Support
    uint8_t maxMpduLength : 2;                    //!< Maximum MPDU Length
    uint8_t maxAmpduLengthExponentExtension : 1;  //!< Maximum A-MPDU length exponent extension

    /**
     * Get the size of the serialized EHT MAC capabilities subfield
     *
     * \return the size of the serialized EHT MAC capabilities subfield
     */
    uint16_t GetSize() const;
    /**
     * Serialize the EHT MAC capabilities subfield
     *
     * \param start iterator pointing to where the EHT MAC capabilities subfield should be written
     * to
     */
    void Serialize(Buffer::Iterator& start) const;
    /**
     * Deserialize the EHT MAC capabilities subfield
     *
     * \param start iterator pointing to where the EHT MAC capabilities subfield should be read from
     * \return the number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator start);
};

/**
 * \ingroup wifi
 *
 * The IEEE 802.11be EHT Capabilities
 */
class EhtCapabilities : public WifiInformationElement
{
  public:
    EhtCapabilities();
    // Implementations of pure virtual methods, or overridden from base class.
    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;

    EhtMacCapabilities m_macCapabilities; //!< EHT MAC Capabilities Info subfield

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    friend std::ostream& operator<<(std::ostream& os, const EhtCapabilities& ehtCapabilities);
};

/**
 * output stream output operator
 * \param os the output stream
 * \param ehtCapabilities the EHT capabilities
 * \returns the output stream
 */
std::ostream& operator<<(std::ostream& os, const EhtCapabilities& ehtCapabilities);

} // namespace ns3

#endif /* EHT_CAPABILITIES_H */
