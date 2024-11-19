/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef FREQS_300KHZ_300GHZ_LOG_H
#define FREQS_300KHZ_300GHZ_LOG_H

#include "spectrum-value.h"

namespace ns3
{

/**
 * @brief Spectrum model logger for frequencies between 300 KHz 300 GHz
 * @return Spectrum model with a range of 300 KHz to 300 GHz for center frequencies
 */
Ptr<SpectrumModel> SpectrumModel300Khz300GhzLog();

} // namespace ns3

#endif /*  FREQS_300KHZ_300GHZ_LOG_H */
