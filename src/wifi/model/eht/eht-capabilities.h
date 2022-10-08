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

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    // TODO: add fields
};

/**
 * output stream output operator
 * \param os the output stream
 * \param ehtCapabilities the EHT capabilities
 * \returns the output stream
 */
std::ostream& operator<<(std::ostream& os, const EhtCapabilities& ehtCapabilities);

} // namespace ns3

#endif /* HE_CAPABILITY_H */
