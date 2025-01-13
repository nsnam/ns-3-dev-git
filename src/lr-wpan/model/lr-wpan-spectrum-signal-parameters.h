/*
 * Copyright (c) 2012 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gary Pei <guangyu.pei@boeing.com>
 */

#ifndef LR_WPAN_SPECTRUM_SIGNAL_PARAMETERS_H
#define LR_WPAN_SPECTRUM_SIGNAL_PARAMETERS_H

#include "ns3/spectrum-signal-parameters.h"

namespace ns3
{

class PacketBurst;

namespace lrwpan
{

/**
 * @ingroup lr-wpan
 *
 * Signal parameters for LrWpan.
 */
struct LrWpanSpectrumSignalParameters : public SpectrumSignalParameters
{
    Ptr<SpectrumSignalParameters> Copy() const override;

    /**
     * default constructor
     */
    LrWpanSpectrumSignalParameters();

    /**
     * copy constructor
     * @param p the object to copy from.
     */
    LrWpanSpectrumSignalParameters(const LrWpanSpectrumSignalParameters& p);

    /**
     * The packet burst being transmitted with this signal
     */
    Ptr<PacketBurst> packetBurst;
};

} // namespace lrwpan
} // namespace ns3

#endif /* LR_WPAN_SPECTRUM_SIGNAL_PARAMETERS_H */
