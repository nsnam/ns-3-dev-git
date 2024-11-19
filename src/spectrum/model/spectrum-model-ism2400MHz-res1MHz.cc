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
 * @ingroup spectrum
 * Spectrum model logger for frequencies in the 2.4 GHz ISM band
 * with 1 MHz resolution.
 */
Ptr<SpectrumModel>
SpectrumModelIsm2400MhzRes1Mhz()
{
    std::vector<double> freqs(100);
    for (int i = 0; i < 100; ++i)
    {
        freqs[i] = ((i + 2400) * 1e6);
    }
    static Ptr<SpectrumModel> model = Create<SpectrumModel>(freqs);
    return model;
}

} // namespace ns3
