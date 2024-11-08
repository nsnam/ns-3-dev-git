/*
 * Copyright (c) 2018 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef PREAMBLE_DETECTION_MODEL_H
#define PREAMBLE_DETECTION_MODEL_H

#include "wifi-units.h"

#include "ns3/object.h"

namespace ns3
{

/**
 * @ingroup wifi
 * @brief the interface for Wifi's preamble detection models
 *
 */
class PreambleDetectionModel : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * A pure virtual method that must be implemented in the subclass.
     * This method returns whether the preamble detection was successful.
     *
     * @param rssi the RSSI of the received signal.
     * @param snr the SNR of the received signal in linear scale.
     * @param channelWidth the channel width of the received signal.
     *
     * @return true if the preamble has been detected,
     *         false otherwise
     */
    virtual bool IsPreambleDetected(dBm_u rssi, double snr, MHz_u channelWidth) const = 0;
};

} // namespace ns3

#endif /* PREAMBLE_DETECTION_MODEL_H */
