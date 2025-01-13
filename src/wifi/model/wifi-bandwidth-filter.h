/*
 * Copyright (c) 2022 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef WIFI_BANDWIDTH_FILTER_H
#define WIFI_BANDWIDTH_FILTER_H

#include "ns3/spectrum-transmit-filter.h"

namespace ns3
{

class WifiBandwidthFilter : public SpectrumTransmitFilter
{
  public:
    WifiBandwidthFilter();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Ignore the signal being received if it is a Wi-Fi PPDU whose TX band
     *        (including guard bands) does not overlap the current operating channel.
     *
     * @param params the parameters of the signals being received
     * @param receiverPhy the SpectrumPhy of the receiver
     * @return whether the signal being received should be ignored
     */

    bool DoFilter(Ptr<const SpectrumSignalParameters> params,
                  Ptr<const SpectrumPhy> receiverPhy) override;

  protected:
    int64_t DoAssignStreams(int64_t stream) override;
};

} // namespace ns3

#endif
