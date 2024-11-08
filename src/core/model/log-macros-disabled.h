/*
 * Copyright (c) 2014 Andrey Mazo
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Andrey Mazo <ahippo@yandex.com>
 */

/**
 * @file
 * @ingroup logging
 * Definition of empty logging macros and the NS_LOG_NOOP_INTERNAL macro.
 */

#ifndef NS3_LOG_MACROS_DISABLED_H
#define NS3_LOG_MACROS_DISABLED_H

#ifndef NS3_LOG_ENABLE
/*
  Implementation Note:

  std::clog << msg
  This expression is required for forms like NS_LOG_LOGIC (... << std::endl ...)


  ns3::ParameterLogger (std::clog) << msg
  This expression is required for NS_LOG_FUNCTION (... << vector ...)

  IMO ParameterLogger should work for both cases, but it fails
  with the error:

    note: in expansion of macro NS_LOG_LOGIC (std::endl...
    error: no match for operator<< (operand types are ns3::ParameterLogger
    and <unresolved overloaded function type>)

    note: candidate: template<class T> ns3::ParameterLogger&
      ns3::ParameterLogger::operator<<(T)
    note:   template argument deduction/substitution failed
    note:   couldn't deduce template parameter T

  Note that std::endl is templated manipulator function, which needs the
  target stream to resolve it's own template parameters.  The compiler
  should deduce this from the ParameterLogger::operator<< <T> ()
  implementation, but evidently fails to do so.

*/

/**
 * @ingroup logging
 * Empty logging macro implementation, used when logging is disabled.
 */
#define NS_LOG_NOOP_INTERNAL(msg)                                                                  \
    do                                                                                             \
        if (false)                                                                                 \
        {                                                                                          \
            std::clog << msg;                                                                      \
        }                                                                                          \
    while (false)

#define NS_LOG(level, msg) NS_LOG_NOOP_INTERNAL(msg)

#define NS_LOG_FUNCTION_NOARGS()

/**
 * @ingroup logging
 * Empty logging macro implementation, used when logging is disabled.
 */
#define NS_LOG_NOOP_FUNC_INTERNAL(msg)                                                             \
    do                                                                                             \
        if (false)                                                                                 \
        {                                                                                          \
            ns3::ParameterLogger(std::clog) << msg;                                                \
        }                                                                                          \
    while (false)

#define NS_LOG_FUNCTION(parameters) NS_LOG_NOOP_FUNC_INTERNAL(parameters)

#define NS_LOG_UNCOND(msg) NS_LOG_NOOP_INTERNAL(msg)

#endif /* !NS3_LOG_ENABLE */

#endif /* NS3_LOG_MACROS_DISABLED_H */
