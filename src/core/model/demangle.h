/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Modified by: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 *              Moved the Demangle function out of CallbackImplBase
 */

#ifndef NS3_DEMANGLE_H
#define NS3_DEMANGLE_H

#include <string>

namespace ns3
{
/**
 * @param [in] mangled The mangled string
 * @return The demangled form of mangled
 */
std::string Demangle(const std::string& mangled);
}; // namespace ns3

#endif // NS3_DEMANGLE_H
