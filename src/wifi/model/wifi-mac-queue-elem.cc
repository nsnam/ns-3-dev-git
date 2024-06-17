/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-mac-queue-elem.h"

#include "wifi-mpdu.h"

namespace ns3
{

WifiMacQueueElem::WifiMacQueueElem(Ptr<WifiMpdu> item)
    : mpdu(item),
      expiryTime(0),
      ac(AC_UNDEF),
      expired(false)
{
}

WifiMacQueueElem::~WifiMacQueueElem()
{
    deleter(mpdu);
    inflights.clear();
}

} // namespace ns3
