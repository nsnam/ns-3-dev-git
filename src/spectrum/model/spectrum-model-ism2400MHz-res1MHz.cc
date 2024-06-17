/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "spectrum-model-ism2400MHz-res1MHz.h"

namespace ns3
{

/**
 * \ingroup spectrum
 * Spectrum model logger for frequencies in the 2.4 GHz ISM band
 * with 1 MHz resolution.
 */
Ptr<SpectrumModel> SpectrumModelIsm2400MhzRes1Mhz;

/**
 * \ingroup spectrum
 *
 * Static initializer class for Spectrum model logger for
 * frequencies in the 2.4 GHz ISM band with 1 MHz resolution.
 */
class static_SpectrumModelIsm2400MhzRes1Mhz_initializer
{
  public:
    static_SpectrumModelIsm2400MhzRes1Mhz_initializer()
    {
        std::vector<double> freqs;
        freqs.reserve(100);
        for (int i = 0; i < 100; ++i)
        {
            freqs.push_back((i + 2400) * 1e6);
        }

        SpectrumModelIsm2400MhzRes1Mhz = Create<SpectrumModel>(freqs);
    }
};

/**
 * \ingroup spectrum
 * Static variable for analyzer initialization
 */
static_SpectrumModelIsm2400MhzRes1Mhz_initializer
    g_static_SpectrumModelIsm2400MhzRes1Mhz_initializer_instance;

} // namespace ns3
