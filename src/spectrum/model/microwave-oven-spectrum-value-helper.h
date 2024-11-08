/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef MICROWAVE_OVEN_SPECTRUM_VALUE_HELPER_H
#define MICROWAVE_OVEN_SPECTRUM_VALUE_HELPER_H

#include "spectrum-value.h"

namespace ns3
{

/**
 * @ingroup spectrum
 *
 * This class provides methods for the creation of SpectrumValue
 * instances that mimic the Power Spectral Density of commercial
 * microwave ovens based on the measurements reported in the following paper:
 * Tanim M. Taher, Matthew J. Misurac, Joseph L. LoCicero, and Donald R. Ucci,
 * "MICROWAVE OVEN SIGNAL MODELING", in Proc. of IEEE WCNC, 2008
 *
 */
class MicrowaveOvenSpectrumValueHelper
{
  public:
    /**
     *
     * @return the Power Spectral Density of Micro Wave Oven #1 in the
     * cited paper
     */
    static Ptr<SpectrumValue> CreatePowerSpectralDensityMwo1();

    /**
     *
     * @return the Power Spectral Density of Micro Wave Oven #2 in the
     * cited paper
     */
    static Ptr<SpectrumValue> CreatePowerSpectralDensityMwo2();
};

} // namespace ns3

#endif /*  MICROWAVE_OVEN_SPECTRUM_VALUE_HELPER_H */
