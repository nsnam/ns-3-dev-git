/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <stdexcept>
#include <string>

/**
 * @file
 * @defgroup log-line-test-example Core example: assembling and interrupting log lines
 * @ingroup core-examples
 * @ingroup logging
 *
 * Example program exercising how a log line is assembled and emitted:
 * the time, node, function and context prefixes, the parameters logged by
 * NS_LOG_FUNCTION, a user-defined `operator<<` which itself logs (nested
 * logging), and what is emitted when the program dies while a log line is
 * still being assembled.
 *
 * Log lines are assembled in a memory buffer and emitted with a single write
 * (see ns3::LogLineBegin), so a line interrupted before it is committed would
 * be lost; ns3::FatalImpl::FlushStreams() emits the truncated line instead.
 *
 * Run without arguments to see the uninterrupted output:
 *
 *     $ ./ns3 run log-line-test-example
 *
 * The lines are logged from an event scheduled at `--time` seconds, with the
 * Time resolution set to seconds, so the time prefix can be exercised up to
 * the largest time which converts to seconds (about 9.2e18); `--context`
 * sets the node context of that event, shown by the node prefix, and
 * `--nested` logs from within the parameter's `operator<<`:
 *
 *     $ ./ns3 run "log-line-test-example --time=9e18 --context=7 --nested"
 *
 * Run with `--mode` to interrupt the second log line:
 *
 *     $ ./ns3 run "log-line-test-example --mode=assert"
 *     $ ./ns3 run "log-line-test-example --mode=fatal --nested"
 *
 * `--mode=throw` reaches std::terminate() without going through the fatal
 * error macros, and `--mode=abort` calls `std::abort()` directly; the
 * truncated line is recovered by the terminate and crash signal handlers
 * installed by ns3::FatalImpl, as it is on a segmentation fault.
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LogLineTestExample");

/// How to interrupt the log line: `none`, `assert`, `fatal`, `throw` or `abort`.
std::string g_mode{"none"};

/// The label appended to every line by NS_LOG_APPEND_CONTEXT, when not empty.
std::string g_label;

/** Append a label to each log line, as models do to identify the logging node. */
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                                                      \
    if (!g_label.empty())                                                                          \
    {                                                                                              \
        ns3LogStream << "[" << g_label << "] ";                                                    \
    }

/**
 * @ingroup log-line-test-example
 * A parameter whose `operator<<` optionally logs, then interrupts the program.
 */
struct Parameter
{
    bool nested;    ///< Whether to log from within operator<<
    bool interrupt; ///< Whether to interrupt the program according to the mode
};

/**
 * @ingroup log-line-test-example
 * Stream a Parameter, logging and dying in the middle of the enclosing log line.
 *
 * @param [in,out] os The output stream.
 * @param [in] param The parameter to stream.
 * @return The stream.
 */
std::ostream&
operator<<(std::ostream& os, const Parameter& param)
{
    os << "PARAM-BEGIN";
    if (param.nested)
    {
        NS_LOG_INFO("nested line logged from operator<<");
    }
    os << "PARAM-END";

    if (!param.interrupt)
    {
        os << " PARAM-TAIL";
        return os;
    }

    if (g_mode == "assert")
    {
        NS_ASSERT_MSG(false, "assertion in the middle of a log line");
    }
    else if (g_mode == "fatal")
    {
        NS_FATAL_ERROR("fatal error in the middle of a log line");
    }
    else if (g_mode == "throw")
    {
        throw std::runtime_error("uncaught exception in the middle of a log line");
    }
    else if (g_mode == "abort")
    {
        std::abort();
    }

    os << " PARAM-TAIL";
    return os;
}

/**
 * @ingroup log-line-test-example
 * Log three lines, the second one interrupted according to the mode.
 *
 * @param [in] nested Whether the parameter logs from within its operator<<.
 * @param [in] count A parameter logged by NS_LOG_FUNCTION.
 */
void
LogLines(bool nested, uint32_t count)
{
    NS_LOG_FUNCTION(nested << count << (Parameter{false, false}));
    NS_LOG_INFO("line 1: logged before the interruption");
    NS_LOG_INFO("line 2: outer line with " << (Parameter{nested, true}) << ", end of outer line");
    NS_LOG_INFO("line 3: logged after the interruption");
}

int
main(int argc, char** argv)
{
    bool nested = false;
    double time = 0;
    uint32_t context = Simulator::NO_CONTEXT;

    CommandLine cmd(__FILE__);
    cmd.AddValue("mode",
                 "How to interrupt the log line: none, assert, fatal, throw or abort",
                 g_mode);
    cmd.AddValue("nested", "Log from within the parameter's operator<<", nested);
    cmd.AddValue("time", "Simulation time, in seconds, at which the lines are logged", time);
    cmd.AddValue("context", "Node context of the event logging the lines", context);
    cmd.AddValue("label", "Label appended to every line by NS_LOG_APPEND_CONTEXT", g_label);
    cmd.Parse(argc, argv);

    LogComponentEnable("LogLineTestExample", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_ALL));

    Time::SetResolution(Time::S);
    Simulator::ScheduleWithContext(context, Seconds(time), &LogLines, nested, 3);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
