/*
 * Copyright (c) 2022 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef NS3_WARNINGS_H
#define NS3_WARNINGS_H

/**
 * @defgroup warnings Compiler warnings
 * @ingroup core
 *
 * Macros useful to silence compiler warnings on selected code parts.
 */

/**
 * @ingroup warnings
 * @def NS_WARNING_POP
 * Pops the diagnostic warning list from the stack, restoring it to the previous state.
 * \sa NS_WARNING_PUSH
 */

/**
 * @ingroup warnings
 * @def NS_WARNING_PUSH
 * Push the diagnostic warning list to the stack, allowing it to be restored later.
 * \sa NS_WARNING_POP
 */

/**
 * @ingroup warnings
 * @def NS_WARNING_SILENCE_DEPRECATED
 * Silences the "-Wdeprecated-declarations" warnings.
 * \sa NS_WARNING_POP
 */

/**
 * @ingroup warnings
 * @def NS_WARNING_SILENCE_MAYBE_UNINITIALIZED
 * Silences GCC "-Wmaybe-uninitialized" warnings.
 * \sa NS_WARNING_POP
 */

/**
 * @ingroup warnings
 * @def NS_WARNING_PUSH_DEPRECATED
 * Save the current warning list and disables the ones about deprecated functions and classes.
 *
 * This macro can be used to silence deprecation warnings and should be used as a last resort
 * to silence the compiler for very specific lines of code.
 * The typical pattern is:
 * @code
 *   NS_WARNING_PUSH_DEPRECATED;
 *   // call to a function or class that has been deprecated.
 *   NS_WARNING_POP;
 * @endcode
 *
 * This macro is equivalent to
 * @code
 *   NS_WARNING_PUSH;
 *   NS_WARNING_SILENCE_DEPRECATED;
 * @endcode
 *
 * Its use is, of course, not suggested unless strictly necessary.
 */

/**
 * @ingroup warnings
 * @def NS_WARNING_PUSH_MAYBE_UNINITIALIZED
 * Save the current warning list and disables the ones about possible uninitialized variables.
 *
 *
 * This macro is equivalent to
 * @code
 *   NS_WARNING_PUSH;
 *   NS_WARNING_SILENCE_MAYBE_UNINITIALIZED;
 * @endcode
 *
 * \sa NS_WARNING_PUSH_DEPRECATED
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

// GCC-specific - Apple's clang pretends to be both...
#if defined(__GNUC__) && !defined(__clang__)
#define NS_WARNING_SILENCE_MAYBE_UNINITIALIZED                                                     \
    _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#else
#define NS_WARNING_SILENCE_MAYBE_UNINITIALIZED
#endif

#define NS_WARNING_PUSH_DEPRECATED                                                                 \
    NS_WARNING_PUSH;                                                                               \
    NS_WARNING_SILENCE_DEPRECATED

#define NS_WARNING_PUSH_MAYBE_UNINITIALIZED                                                        \
    NS_WARNING_PUSH;                                                                               \
    NS_WARNING_SILENCE_MAYBE_UNINITIALIZED

#endif /* NS3_WARNINGS_H */
