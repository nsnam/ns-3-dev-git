/*
 * Copyright (c) 2022 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef NS3_WARNINGS_H
#define NS3_WARNINGS_H

/**
 * \ingroup core
 * \def NS_WARNING_POP
 * Pops the diagnostic warning list from the stack, restoring it to the previous state.
 * \sa NS_WARNING_PUSH
 */

/**
 * \ingroup core
 * \def NS_WARNING_PUSH
 * Push the diagnostic warning list to the stack, allowing it to be restored later.
 * \sa NS_WARNING_POP
 */

/**
 * \ingroup core
 * \def NS_WARNING_SILENCE_DEPRECATED
 * Silences the "-Wdeprecated-declarations" warnings.
 * \sa NS_WARNING_POP
 */

/**
 * \ingroup core
 * \def NS_WARNING_PUSH_DEPRECATED
 * Save the current warning list and disables the ones about deprecated functions and classes.
 *
 * This macro can be used to silence deprecation warnings and should be used as a last resort
 * to silence the compiler for very specific lines of code.
 * The typical pattern is:
 * \code
 *   NS_WARNING_PUSH_DEPRECATED;
 *   // call to a function or class that has been deprecated.
 *   NS_WARNING_POP;
 * \endcode
 *
 * Its use is, of course, not suggested unless strictly necessary.
 */

#if defined(_MSC_VER)
// You can find the MSC warning codes at
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warnings-c4000-c5999
#define NS_WARNING_PUSH __pragma(warning(push))
#define NS_WARNING_SILENCE_DEPRECATED __pragma(warning(disable : 4996))
#define NS_WARNING_POP __pragma(warning(pop))

#elif defined(__GNUC__) || defined(__clang__)
// Clang seems to understand these GCC pragmas
#define NS_WARNING_PUSH _Pragma("GCC diagnostic push")
#define NS_WARNING_SILENCE_DEPRECATED                                                              \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define NS_WARNING_POP _Pragma("GCC diagnostic pop")

#else
#define NS_WARNING_PUSH
#define NS_WARNING_SILENCE_DEPRECATED
#define NS_WARNING_POP

#endif

#define NS_WARNING_PUSH_DEPRECATED                                                                 \
    NS_WARNING_PUSH;                                                                               \
    NS_WARNING_SILENCE_DEPRECATED

#endif /* NS3_WARNINGS_H */
