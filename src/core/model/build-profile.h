/*
 * Copyright (c) 2015 LLNL
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#ifndef NS3_BUILD_PROFILE_H
#define NS3_BUILD_PROFILE_H

/**
 * @file
 * @ingroup debugging
 * NS_BUILD_DEBUG, NS_BUILD_RELEASE, and NS_BUILD_OPTIMIZED
 * macro definitions.
 */

/**
 * @ingroup debugging
 * Build profile no-op macro.
 * @param [in] code The code to skip.
 */
#define NS_BUILD_PROFILE_NOOP(code)                                                                \
    do                                                                                             \
        if (false)                                                                                 \
        {                                                                                          \
            code;                                                                                  \
        }                                                                                          \
    while (false)

/**
 * @ingroup debugging
 * Build profile macro to execute a code snippet.
 * @param [in] code The code to execute.
 */
#define NS_BUILD_PROFILE_OP(code)                                                                  \
    do                                                                                             \
    {                                                                                              \
        code;                                                                                      \
    } while (false)

#ifdef NS3_BUILD_PROFILE_DEBUG
/**
 * @ingroup debugging
 * Execute a code snippet in debug builds.
 * @param [in] code The code to execute.
 */
#define NS_BUILD_DEBUG(code) NS_BUILD_PROFILE_OP(code)
#else
#define NS_BUILD_DEBUG(code) NS_BUILD_PROFILE_NOOP(code)
#endif

#ifdef NS3_BUILD_PROFILE_RELEASE
/**
 * @ingroup debugging
 * Execute a code snippet in release builds.
 * @param [in] code The code to execute.
 */
#define NS_BUILD_RELEASE(code) NS_BUILD_PROFILE_OP(code)
#else
#define NS_BUILD_RELEASE(code) NS_BUILD_PROFILE_NOOP(code)
#endif

#ifdef NS3_BUILD_PROFILE_OPTIMIZED
/**
 * @ingroup debugging
 * Execute a code snippet in optimized builds.
 * @param [in] code The code to execute.
 */
#define NS_BUILD_OPTIMIZED(code) NS_BUILD_PROFILE_OP(code)
#else
#define NS_BUILD_OPTIMIZED(code) NS_BUILD_PROFILE_NOOP(code)
#endif

#endif /* NS3_BUILD_PROFILE_H */
