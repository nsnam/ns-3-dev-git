/*
 * Copyright (c) 2006,2007 INRIA, INESC Porto
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Gustavo Carneiro <gjc@inescporto.pt>
 */

#include "breakpoint.h"

#include "log.h"

#include "ns3/core-config.h"
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

namespace ns3
{

/**
 * @file
 * @ingroup breakpoint
 * ns3::BreakpointFallback function implementation.
 */

NS_LOG_COMPONENT_DEFINE("Breakpoint");

#if defined(HAVE_SIGNAL_H) && defined(SIGTRAP)

void
BreakpointFallback()
{
    NS_LOG_FUNCTION_NOARGS();

    raise(SIGTRAP);
}

#else

void
BreakpointFallback()
{
    NS_LOG_FUNCTION_NOARGS();

    int* a = nullptr;
    /**
     * we test here to allow a debugger to change the value of
     * the variable 'a' to allow the debugger to avoid the
     * subsequent segfault.
     */
    if (a == nullptr)
    {
        *a = 0;
    }
}

#endif // HAVE_SIGNAL_H

} // namespace ns3
