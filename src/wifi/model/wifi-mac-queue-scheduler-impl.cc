/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-mac-queue-scheduler-impl.h"

namespace ns3
{

/// typedef for WifiSchedPrecedence<Time>
using WifiSchedPrecedenceTime = WifiSchedPrecedence<Time>;

/// typedef for std::less<WifiSchedPrecedence<Time>>
using LessWifiSchedPrecedenceTime = std::less<WifiSchedPrecedence<Time>>;

NS_OBJECT_TEMPLATE_CLASS_TWO_DEFINE(WifiMacQueueSchedulerImpl,
                                    WifiSchedPrecedenceTime,
                                    LessWifiSchedPrecedenceTime);

} // namespace ns3
