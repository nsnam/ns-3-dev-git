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
 * \ingroup spectrum
 * Spectrum model logger for frequencies between 300 Khz 300 Ghz
 */
Ptr<SpectrumModel> SpectrumModel300Khz300GhzLog;

/**
 * \ingroup spectrum
 *
 * Static initializer class for Spectrum model logger
 * for frequencies between 300 Khz 300 Ghz
 */
class static_SpectrumModel300Khz300GhzLog_initializer
{
  public:
    static_SpectrumModel300Khz300GhzLog_initializer()
    {
        std::vector<double> freqs;
        for (double f = 3e5; f < 3e11; f = f * 2)
        {
            freqs.push_back(f);
        }
        SpectrumModel300Khz300GhzLog = Create<SpectrumModel>(freqs);
    }
};

/**
 * \ingroup spectrum
 * Static variable for analyzer initialization
 */
static_SpectrumModel300Khz300GhzLog_initializer
    g_static_SpectrumModel300Khz300GhzLog_initializer_instance;

} // namespace ns3
