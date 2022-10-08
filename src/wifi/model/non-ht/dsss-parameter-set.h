/*
 * Copyright (c) 2016 Sébastien Deronne
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

#ifndef DSSS_PARAMETER_SET_H
#define DSSS_PARAMETER_SET_H

#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * \brief The DSSS Parameter Set
 * \ingroup wifi
 *
 * This class knows how to serialise and deserialise the DSSS Parameter Set.
 */
class DsssParameterSet : public WifiInformationElement
{
  public:
    DsssParameterSet();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;

    /**
     * Set the Current Channel field in the DsssParameterSet information element.
     *
     * \param currentChannel the CurrentChannel field in the DsssParameterSet information element
     */
    void SetCurrentChannel(uint8_t currentChannel);

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    uint8_t m_currentChannel; ///< current channel number
};

} // namespace ns3

#endif /* DSSS_PARAMETER_SET_H */
