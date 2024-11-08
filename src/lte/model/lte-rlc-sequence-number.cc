/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "lte-rlc-sequence-number.h"

namespace ns3
{

/**
 * Ostream output function
 * @param os the output stream
 * @param val the sequence number
 * @returns the os
 */
std::ostream&
operator<<(std::ostream& os, const SequenceNumber10& val)
{
    os << val.m_value;
    return os;
}

} // namespace ns3
