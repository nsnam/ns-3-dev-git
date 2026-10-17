/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

// Injected into every ns-3 executable when NS3_CLANGCL_STLCHECKS=ON. Installs
// a CRT report hook before main() so that debug-CRT reports (checked-iterator
// assertions such as "invalid comparator", heap corruption reports, etc.) are
// printed to stderr along with a stack trace, instead of being shown in a
// message box that blocks or is invisible in non-interactive shells.

#if defined(_DEBUG) && defined(_MSC_VER)

#include "ns3/fatal-impl.h"

#include <crtdbg.h>
#include <iostream>

namespace ns3
{

namespace
{

int
CrtDebugReportHook(int reportType, char* message, int* returnValue)
{
    std::cerr << "CRT debug report: " << (message ? message : "") << std::endl;
    FatalImpl::PrintStackTrace();
    return false; // let normal report handling continue
}

/// Installs the CRT report hook upon dynamic initialization.
const struct CrtDebugReportHookInstaller
{
    CrtDebugReportHookInstaller()
    {
        _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, CrtDebugReportHook);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    }
} crtDebugReportHookInstaller;

} // namespace

} // namespace ns3

#endif
