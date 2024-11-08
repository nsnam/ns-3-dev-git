/*
 * Copyright (c) 2016 Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef DSSS_PARAMETER_SET_H
#define DSSS_PARAMETER_SET_H

#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * @brief The DSSS Parameter Set
 * @ingroup wifi
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
     * @param currentChannel the CurrentChannel field in the DsssParameterSet information element
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
