/*
 * Copyright (c) 2005,2006 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 * \param [in] mangled The mangled string
 * \return The demangled form of mangled
 */
std::string Demangle(const std::string& mangled);
}; // namespace ns3

#endif // NS3_DEMANGLE_H
