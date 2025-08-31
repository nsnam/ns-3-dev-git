/*
 * Copyright (c) 2026 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_MODE_H
#define UHR_MODE_H

#include <cstdint>

namespace ns3
{

namespace UhrMode
{

/**
 * Values for the Mode ID field
 * IEEE 802.11bn D1.0 Table 9-417g
 */
enum ModeId : uint8_t
{
    DPS = 0,
    NPCA = 1,
    DUO = 2,
    DSO = 3,
    P_EDCA = 4,
    ELR_RECEPTION = 5,
    AOM = 6,
    LLI = 7,
    CO_BF = 8,
    CO_SR = 9,
    EMLSR = 10,
    DBE = 11,
    MAX_MODE_ID = 12
};

} // namespace UhrMode

} // namespace ns3

#endif /* UHR_MODE_H */
