/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Modified by: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 *              Moved the Demangle function out of CallbackImplBase
 */

#include "demangle.h"

#include "assert.h"
#include "log.h"

NS_LOG_COMPONENT_DEFINE("Demangle");

#if (__GNUC__ >= 3)

#include <cstdlib>
#include <cxxabi.h>

std::string
ns3::Demangle(const std::string& mangled)
{
    NS_LOG_FUNCTION(mangled);

    int status;
    char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);

    std::string ret;
    if (status == 0)
    {
        NS_ASSERT(demangled);
        ret = demangled;
    }
    else if (status == -1)
    {
        NS_LOG_DEBUG("Demangling failed: Memory allocation failure occurred.");
        ret = mangled;
    }
    else if (status == -2)
    {
        NS_LOG_DEBUG("Demangling failed: Mangled name is not a valid under the C++ ABI "
                     "mangling rules.");
        ret = mangled;
    }
    else if (status == -3)
    {
        NS_LOG_DEBUG("Demangling failed: One of the arguments is invalid.");
        ret = mangled;
    }
    else
    {
        NS_LOG_DEBUG("Demangling failed: status " << status);
        ret = mangled;
    }

    if (demangled)
    {
        std::free(demangled);
    }
    return ret;
}

#else

std::string
ns3::Demangle(const std::string& mangled)
{
    NS_LOG_FUNCTION(mangled);
    return mangled;
}

#endif
