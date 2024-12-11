/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_RU_H
#define WIFI_RU_H

#include "ns3/eht-ru.h"
#include "ns3/he-ru.h"

namespace ns3
{

/**
 * This class handles RU variants.
 */
class WifiRu
{
  public:
    /**
     * Get the approximate bandwidth occupied by a RU.
     *
     * @param ruType the RU type
     * @return the approximate bandwidth occupied by the RU
     */
    static MHz_u GetBandwidth(RuType ruType);

    /**
     * Get the RU corresponding to the approximate bandwidth.
     *
     * @param bandwidth the approximate bandwidth occupied by the RU
     * @return the RU type
     */
    static RuType GetRuType(MHz_u bandwidth);
};

} // namespace ns3

#endif /* WIFI_RU_H */
