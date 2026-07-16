/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef NS3_LOG_MACROS_ENABLED_H
#define NS3_LOG_MACROS_ENABLED_H

/**
 * @file
 * @ingroup logging
 * NS_LOG and related logging macro definitions.
 */

#ifdef NS3_LOG_ENABLE

/**
 * @ingroup logging
 * Append the simulation time to a log message.
 * @internal
 * Logging implementation macro; should not be called directly.
 * NS_FATAL_ERROR_IMPL_NO_MSG (fatal-error.h) emits the same prefix
 * directly to std::clog; keep the two formats in sync.
 */
#define NS_LOG_APPEND_TIME_PREFIX                                                                  \
    if (g_log.IsEnabled(ns3::LOG_PREFIX_TIME))                                                     \
    {                                                                                              \
        ns3::TimePrinter printer = ns3::LogGetTimePrinter();                                       \
        if (printer != 0)                                                                          \
        {                                                                                          \
            (*printer)(ns3LogStream);                                                              \
            ns3LogStream << " ";                                                                   \
        }                                                                                          \
    }

/**
 * @ingroup logging
 * Append the simulation node id to a log message.
 * @internal
 * Logging implementation macro; should not be called directly.
 * NS_FATAL_ERROR_IMPL_NO_MSG (fatal-error.h) emits the same prefix
 * directly to std::clog; keep the two formats in sync.
 */
#define NS_LOG_APPEND_NODE_PREFIX                                                                  \
    if (g_log.IsEnabled(ns3::LOG_PREFIX_NODE))                                                     \
    {                                                                                              \
        ns3::NodePrinter printer = ns3::LogGetNodePrinter();                                       \
        if (printer != 0)                                                                          \
        {                                                                                          \
            (*printer)(ns3LogStream);                                                              \
            ns3LogStream << " ";                                                                   \
        }                                                                                          \
    }

/**
 * @ingroup logging
 * Append the function name to a log message.
 * @internal
 * Logging implementation macro; should not be called directly.
 */
#define NS_LOG_APPEND_FUNC_PREFIX                                                                  \
    if (g_log.IsEnabled(ns3::LOG_PREFIX_FUNC))                                                     \
    {                                                                                              \
        ns3LogStream << g_log.Name() << ":" << __FUNCTION__ << "(): ";                             \
    }

/**
 * @ingroup logging
 * Append the log severity level to a log message.
 * @internal
 * Logging implementation macro; should not be called directly.
 */
#define NS_LOG_APPEND_LEVEL_PREFIX(level)                                                          \
    if (g_log.IsEnabled(ns3::LOG_PREFIX_LEVEL))                                                    \
    {                                                                                              \
        ns3LogStream << "[" << g_log.GetLevelLabel(level) << "] ";                                 \
    }

#ifndef NS_LOG_APPEND_CONTEXT
/**
 * @ingroup logging
 * Append the node id (or other file-local programmatic context, such as
 * MPI rank) to a log message.
 *
 * This is implemented locally in `.cc` files because
 * the relevant variable is only known there.
 *
 * The macro is expanded inside the NS_LOG_* macros where a local
 * `std::ostream& ns3LogStream` is in scope.  Redefinitions must stream the
 * context into `ns3LogStream` (not directly to `std::clog`) so the whole
 * log line can be emitted with a single write operation.
 *
 * Preferred format is something like (assuming the node id is
 * accessible from `var`:
 * @code
 *   if (var)
 *     {
 *       ns3LogStream << "[node " << var->GetObject<Node> ()->GetId () << "] ";
 *     }
 * @endcode
 */
#define NS_LOG_APPEND_CONTEXT
#endif /* NS_LOG_APPEND_CONTEXT */

#ifndef NS_LOG_CONDITION
/**
 * @ingroup logging
 * Limit logging output based on some file-local condition,
 * such as MPI rank.
 *
 * This is implemented locally in `.cc` files because
 * the relevant condition variable is only known there.
 *
 * Since this appears immediately before the `do { ... } while false`
 * construct of \c NS_LOG(level, msg), it must have the form
 * @code
 *   #define NS_LOG_CONDITION    if (condition)
 * @endcode
 */
#define NS_LOG_CONDITION
#endif

/**
 * @ingroup logging
 *
 * This macro allows you to log an arbitrary message at a specific
 * log level.
 *
 * The log message is expected to be a C++ ostream
 * message such as "my string" << aNumber << "my oth stream".
 * The message and all prefixes (time, node, context, function, level) are
 * assembled in a reusable memory buffer (`ns3LogStream`) and emitted to
 * `std::clog` with a single write operation
 * (see ns3::LogLineBegin() / ns3::LogLineCommit()).
 *
 * Typical usage looks like:
 * @code
 * NS_LOG (LOG_DEBUG, "a number="<<aNumber<<", anotherNumber="<<anotherNumber);
 * @endcode
 *
 * @param [in] level The log level
 * @param [in] msg The message to log
 * @internal
 * Logging implementation macro; should not be called directly.
 */
#define NS_LOG(level, msg)                                                                         \
    NS_LOG_CONDITION                                                                               \
    do                                                                                             \
    {                                                                                              \
        if (g_log.IsEnabled(level))                                                                \
        {                                                                                          \
            std::ostream& ns3LogStream = ns3::LogLineBegin();                                      \
            NS_LOG_APPEND_TIME_PREFIX;                                                             \
            NS_LOG_APPEND_NODE_PREFIX;                                                             \
            NS_LOG_APPEND_CONTEXT;                                                                 \
            NS_LOG_APPEND_FUNC_PREFIX;                                                             \
            NS_LOG_APPEND_LEVEL_PREFIX(level);                                                     \
            auto flags = ns3LogStream.setf(std::ios_base::boolalpha);                              \
            ns3LogStream << msg;                                                                   \
            ns3LogStream.flags(flags);                                                             \
            ns3::LogLineCommit(ns3LogStream);                                                      \
        }                                                                                          \
    } while (false)

/**
 * @ingroup logging
 *
 * Output the name of the function.
 *
 * This should be used only in static functions without arguments; most member functions
 * should instead use NS_LOG_FUNCTION().
 */
#define NS_LOG_FUNCTION_NOARGS()                                                                   \
    NS_LOG_CONDITION                                                                               \
    do                                                                                             \
    {                                                                                              \
        if (g_log.IsEnabled(ns3::LOG_FUNCTION))                                                    \
        {                                                                                          \
            std::ostream& ns3LogStream = ns3::LogLineBegin();                                      \
            NS_LOG_APPEND_TIME_PREFIX;                                                             \
            NS_LOG_APPEND_NODE_PREFIX;                                                             \
            NS_LOG_APPEND_CONTEXT;                                                                 \
            ns3LogStream << g_log.Name() << ":" << __FUNCTION__ << "()";                           \
            ns3::LogLineCommit(ns3LogStream);                                                      \
        }                                                                                          \
    } while (false)

/**
 * @ingroup logging
 *
 * If log level LOG_FUNCTION is enabled, this macro will output
 * all input parameters separated by ", ".
 *
 * Typical usage looks like:
 * @code
 * NS_LOG_FUNCTION (aNumber<<anotherNumber);
 * @endcode
 * And the output will look like:
 * @code
 * Component:Function (aNumber, anotherNumber)
 * @endcode
 *
 * To facilitate function tracing, most functions should begin with
 * (at least) NS_LOG_FUNCTION(this).
 *
 * Static functions should use NS_LOG_FUNCTION(args) when they have arguments,
 * and NS_LOG_FUNCTION_NOARGS() when they have no arguments.
 *
 * @param [in] parameters The parameters to output.
 */
#define NS_LOG_FUNCTION(parameters)                                                                \
    NS_LOG_CONDITION                                                                               \
    do                                                                                             \
    {                                                                                              \
        if (g_log.IsEnabled(ns3::LOG_FUNCTION))                                                    \
        {                                                                                          \
            std::ostream& ns3LogStream = ns3::LogLineBegin();                                      \
            NS_LOG_APPEND_TIME_PREFIX;                                                             \
            NS_LOG_APPEND_NODE_PREFIX;                                                             \
            NS_LOG_APPEND_CONTEXT;                                                                 \
            ns3LogStream << g_log.Name() << ":" << __FUNCTION__ << "(";                            \
            auto flags = ns3LogStream.setf(std::ios_base::boolalpha);                              \
            ns3::ParameterLogger(ns3LogStream) << parameters;                                      \
            ns3LogStream.flags(flags);                                                             \
            ns3LogStream << ")";                                                                   \
            ns3::LogLineCommit(ns3LogStream);                                                      \
        }                                                                                          \
    } while (false)

/**
 * @ingroup logging
 *
 * Output the requested message unconditionally.
 *
 * @param [in] msg The message to log
 */
#define NS_LOG_UNCOND(msg)                                                                         \
    NS_LOG_CONDITION                                                                               \
    do                                                                                             \
    {                                                                                              \
        std::ostream& ns3LogStream = ns3::LogLineBegin();                                          \
        auto flags = ns3LogStream.setf(std::ios_base::boolalpha);                                  \
        ns3LogStream << msg;                                                                       \
        ns3LogStream.flags(flags);                                                                 \
        ns3::LogLineCommit(ns3LogStream);                                                          \
    } while (false)

#endif /* NS3_LOG_ENABLE */

#endif /* NS3_LOG_MACROS_ENABLED_H */
