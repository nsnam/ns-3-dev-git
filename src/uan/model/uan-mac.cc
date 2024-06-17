/*
 * Copyright (c) 2011 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mitch Watrous <watrous@u.washington.edu>
 */

#include "uan-mac.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(UanMac);

UanMac::UanMac()
    : m_txModeIndex(0)
{
}

TypeId
UanMac::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UanMac").SetParent<Object>().SetGroupName("Uan");
    return tid;
}

void
UanMac::SetTxModeIndex(uint32_t txModeIndex)
{
    m_txModeIndex = txModeIndex;
}

uint32_t
UanMac::GetTxModeIndex() const
{
    return m_txModeIndex;
}

Address
UanMac::GetAddress()
{
    return m_address;
}

void
UanMac::SetAddress(Mac8Address addr)
{
    m_address = addr;
}

Address
UanMac::GetBroadcast() const
{
    Mac8Address broadcast = Mac8Address(255);
    return broadcast;
}

} // namespace ns3
