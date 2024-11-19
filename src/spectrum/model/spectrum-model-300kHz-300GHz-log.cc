/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "spectrum-model-300kHz-300GHz-log.h"

namespace ns3
{

/**
 * @ingroup spectrum
 * Spectrum model logger for frequencies between 300 KHz 300 GHz
 */
Ptr<SpectrumModel>
SpectrumModel300Khz300GhzLog()
{
    std::vector<double> freqs;
    for (double f = 3e5; f < 3e11; f = f * 2)
    {
        freqs.push_back(f);
    }
    static Ptr<SpectrumModel> model = Create<SpectrumModel>(freqs);
    return model;
}

} // namespace ns3
