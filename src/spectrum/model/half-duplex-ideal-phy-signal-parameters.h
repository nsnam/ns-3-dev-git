/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef HALF_DUPLEX_IDEAL_PHY_SPECTRUM_PARAMETERS_H
#define HALF_DUPLEX_IDEAL_PHY_SPECTRUM_PARAMETERS_H

#include "spectrum-signal-parameters.h"

namespace ns3
{

class Packet;

/**
 * @ingroup spectrum
 *
 * Signal parameters for HalfDuplexIdealPhy
 */
struct HalfDuplexIdealPhySignalParameters : public SpectrumSignalParameters
{
    Ptr<SpectrumSignalParameters> Copy() const override;

    /**
     * default constructor
     */
    HalfDuplexIdealPhySignalParameters();

    /**
     * copy constructor
     * @param p object to copy
     */
    HalfDuplexIdealPhySignalParameters(const HalfDuplexIdealPhySignalParameters& p);

    /**
     * The data packet being transmitted with this signal
     */
    Ptr<Packet> data;
};

} // namespace ns3

#endif /* HALF_DUPLEX_IDEAL_PHY_SPECTRUM_PARAMETERS_H */
