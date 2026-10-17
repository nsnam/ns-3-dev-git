/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "log.h"

#include "assert.h"
#include "environment-variable.h"
#include "fatal-error.h"
#include "string.h"

#include <algorithm> // transform
#include <ctype.h>   // toupper
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <io.h> // _write
#else
#include <unistd.h> // write
#endif

/**
 * @file
 * @ingroup logging
 * ns3::LogComponent and related implementations.
 */

/**
 * @ingroup logging
 * Unnamed namespace for log.cc
 */
namespace
{
/** Mapping of log level text names to values. */
const std::map<std::string, ns3::LogLevel> LOG_LABEL_LEVELS = {
    // clang-format off
        {"none",           ns3::LOG_NONE},
        {"error",          ns3::LOG_ERROR},
        {"level_error",    ns3::LOG_LEVEL_ERROR},
        {"warn",           ns3::LOG_WARN},
        {"level_warn",     ns3::LOG_LEVEL_WARN},
        {"debug",          ns3::LOG_DEBUG},
        {"level_debug",    ns3::LOG_LEVEL_DEBUG},
        {"info",           ns3::LOG_INFO},
        {"level_info",     ns3::LOG_LEVEL_INFO},
        {"function",       ns3::LOG_FUNCTION},
        {"level_function", ns3::LOG_LEVEL_FUNCTION},
        {"logic",          ns3::LOG_LOGIC},
        {"level_logic",    ns3::LOG_LEVEL_LOGIC},
        {"all",            ns3::LOG_ALL},
        {"level_all",      ns3::LOG_LEVEL_ALL},
        {"func",           ns3::LOG_PREFIX_FUNC},
        {"prefix_func",    ns3::LOG_PREFIX_FUNC},
        {"time",           ns3::LOG_PREFIX_TIME},
        {"prefix_time",    ns3::LOG_PREFIX_TIME},
        {"node",           ns3::LOG_PREFIX_NODE},
        {"prefix_node",    ns3::LOG_PREFIX_NODE},
        {"level",          ns3::LOG_PREFIX_LEVEL},
        {"prefix_level",   ns3::LOG_PREFIX_LEVEL},
        {"prefix_all",     ns3::LOG_PREFIX_ALL}
    // clang-format on
};

/** Inverse mapping of level values to log level text names. */
const std::map<ns3::LogLevel, std::string> LOG_LEVEL_LABELS = {[]() {
    std::map<ns3::LogLevel, std::string> labels;
    for (const auto& [label, lev] : LOG_LABEL_LEVELS)
    {
        // Only keep the first label for a level
        if (labels.find(lev) == labels.end())
        {
            std::string pad{label};
            // Add whitespace for alignment with "ERROR", "DEBUG" etc.
            if (pad.size() < 5)
            {
                pad.insert(pad.size(), 5 - pad.size(), ' ');
            }
            std::transform(pad.begin(), pad.end(), pad.begin(), ::toupper);
            labels[lev] = pad;
        }
    }
    return labels;
}()};

} // Unnamed namespace

namespace ns3
{

/**
 * @ingroup logging
 * The Log TimePrinter.
 * This is private to the logging implementation.
 */
static TimePrinter g_logTimePrinter = nullptr;
/**
 * @ingroup logging
 * The Log NodePrinter.
 */
static NodePrinter g_logNodePrinter = nullptr;

/**
 * @ingroup logging
 * Handler for the undocumented \c print-list token in NS_LOG
 * which triggers printing of the list of log components, then exits.
 *
 * A static instance of this class is instantiated below, so the
 * \c print-list token is handled before any other logging action
 * can take place.
 *
 * This is private to the logging implementation.
 */
class PrintList
{
  public:
    PrintList(); //<! Constructor, prints the list and exits.
};

/**
 * Invoke handler for \c print-list in NS_LOG environment variable.
 * This is private to the logging implementation.
 */
static PrintList g_printList;

/* static */
LogComponent::ComponentList*
LogComponent::GetComponentList()
{
    static LogComponent::ComponentList components;
    return &components;
}

PrintList::PrintList()
{
    auto [found, value] = EnvironmentVariable::Get("NS_LOG", "print-list", ":");
    if (found)
    {
        LogComponentPrintList();
        exit(0);
    }
}

LogComponent::LogComponent(const std::string& name,
                           const std::string& file,
                           const LogLevel mask /* = 0 */)
    : m_levels(0),
      m_mask(mask),
      m_name(name),
      m_file(file)
{
    // Check if we're mentioned in NS_LOG, and set our flags appropriately
    EnvVarCheck();

    LogComponent::ComponentList* components = GetComponentList();

    if (components->find(name) != components->end())
    {
        NS_FATAL_ERROR("Log component \"" << name << "\" has already been registered once.");
    }

    components->insert(std::make_pair(name, this));
}

LogComponent&
GetLogComponent(const std::string name)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();
    LogComponent* ret;

    try
    {
        ret = components->at(name);
    }
    catch (std::out_of_range&)
    {
        NS_FATAL_ERROR("Log component \"" << name << "\" does not exist.");
    }
    return *ret;
}

/** Unnamed namespace for log line assembly buffers. */
namespace
{

/**
 * A streambuf appending to a std::string whose capacity is reused
 * between log lines, so steady-state logging does not allocate.
 */
class LogLineBuf : public std::streambuf
{
  public:
    LogLineBuf()
    {
        m_line.reserve(1000);
    }

    /** @return The log line being assembled. */
    std::string& Line()
    {
        return m_line;
    }

  protected:
    /**
     * Append a character sequence to the line.
     *
     * @param s The characters to append.
     * @param n The number of characters to append.
     * @return The number of characters appended.
     */
    std::streamsize xsputn(const char* s, std::streamsize n) override
    {
        m_line.append(s, static_cast<std::size_t>(n));
        return n;
    }

    /**
     * Append a single character to the line.
     *
     * @param c The character to append, or EOF.
     * @return A value other than EOF on success.
     */
    int_type overflow(int_type c) override
    {
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            m_line.push_back(traits_type::to_char_type(c));
        }
        return traits_type::not_eof(c);
    }

  private:
    std::string m_line; //!< The log line being assembled.
};

/**
 * Write an assembled log line to std::clog, terminated by a newline,
 * and empty it.
 *
 * @param [in,out] line The line to emit.
 */
void
EmitLine(std::string& line)
{
    line.push_back('\n');
    std::clog.write(line.data(), static_cast<std::streamsize>(line.size()));
    std::clog.flush();
    line.clear();
}

/**
 * Whether this thread's LogLine has been destroyed.
 *
 * Trivially destructible, so it remains readable during program shutdown,
 * after the buffer's own thread_local destructor has run.  Core itself logs
 * during static destruction (e.g. Time::Clear() from ~Time of static
 * attribute defaults), so this must be handled.
 */
thread_local bool g_logLineDestroyed = false;

/** A memory buffer and the ostream assembling a log line into it. */
struct LogLine
{
    LogLineBuf buf;        //!< The line buffer.
    std::ostream os{&buf}; //!< The stream assembling the line.

    /**
     * Arm g_logLineDestroyed, emitting any partially assembled line first
     * so it is not lost.
     */
    ~LogLine()
    {
        g_logLineDestroyed = true;
        auto& partial = buf.Line();
        if (!partial.empty())
        {
            EmitLine(partial);
        }
    }
};

/**
 * The thread's log line, once created.
 *
 * A plain pointer, so a signal handler can reach the line being assembled
 * without running the thread_local initialization it may have interrupted.
 */
thread_local LogLine* g_logLine = nullptr;

/** @return The thread-local log line buffer. */
LogLine&
GetLogLine()
{
    thread_local LogLine line;
    g_logLine = &line;
    return line;
}

} // unnamed namespace

std::ostream&
LogLineBegin()
{
    if (g_logLineDestroyed)
    {
        // Logging during program shutdown, after this thread's buffer is
        // gone: stream directly to std::clog, which is kept alive by
        // std::ios_base::Init.
        return std::clog;
    }
    // The buffer is empty here except for nested logging (a user-defined
    // operator<< that itself logs while a log message is being assembled);
    // then the inner line is appended to the outer line in progress and
    // LogLineCommit() flushes both, matching the historical interleaving of
    // direct std::clog streaming.
    return GetLogLine().os;
}

void
LogLineCommit(std::ostream& os)
{
    if (&os == &std::clog)
    {
        std::clog << std::endl;
        return;
    }
    EmitLine(static_cast<LogLineBuf*>(os.rdbuf())->Line());
    // A failed insertion (e.g. an operator<< that threw) sets badbit on the
    // stream; clear it so the failure does not silence every later line.
    os.clear();
}

void
LogLineFlushPartial()
{
    if (g_logLineDestroyed)
    {
        return;
    }
    auto& line = GetLogLine().buf.Line();
    if (line.empty())
    {
        return;
    }
    EmitLine(line);
}

namespace
{

/**
 * Write to the standard error file descriptor, bypassing std::clog.
 *
 * @param [in] s The characters to write.
 * @param [in] n The number of characters to write.
 */
void
WriteStdErr(const char* s, std::size_t n)
{
#ifdef _WIN32
    _write(2, s, static_cast<unsigned int>(n));
#else
    [[maybe_unused]] auto written = write(2, s, n);
#endif
}

} // unnamed namespace

void
LogLineFlushPartialFromSignal()
{
    if (g_logLineDestroyed || g_logLine == nullptr)
    {
        return;
    }
    const auto& line = g_logLine->buf.Line();
    if (line.empty())
    {
        return;
    }
    WriteStdErr(line.data(), line.size());
    WriteStdErr("\n", 1);
}

void
LogComponent::EnvVarCheck()
{
    auto [found, value] = EnvironmentVariable::Get("NS_LOG", m_name, ":");
    if (!found)
    {
        std::tie(found, value) = EnvironmentVariable::Get("NS_LOG", "*", ":");
    }
    if (!found)
    {
        std::tie(found, value) = EnvironmentVariable::Get("NS_LOG", "***", ":");
    }

    if (!found)
    {
        return;
    }

    if (value.empty())
    {
        // Default is enable all levels, all prefixes
        value = "**";
    }

    // Got a value, might have flags
    int level = 0;
    StringVector flags = SplitString(value, "|");
    NS_ASSERT_MSG(!flags.empty(), "Unexpected empty flags from non-empty value");
    bool pre_pipe{true};

    for (const auto& lev : flags)
    {
        if (lev == "**")
        {
            level |= LOG_LEVEL_ALL | LOG_PREFIX_ALL;
        }
        else if (lev == "all" || lev == "*")
        {
            level |= (pre_pipe ? LOG_LEVEL_ALL : LOG_PREFIX_ALL);
        }
        else if (LOG_LABEL_LEVELS.find(lev) != LOG_LABEL_LEVELS.end())
        {
            level |= LOG_LABEL_LEVELS.at(lev);
        }
        pre_pipe = false;
    }
    Enable(static_cast<LogLevel>(level));
}

bool
LogComponent::IsNoneEnabled() const
{
    return m_levels == 0;
}

void
LogComponent::SetMask(const LogLevel level)
{
    m_mask |= level;
}

void
LogComponent::Enable(const LogLevel level)
{
    m_levels |= (level & ~m_mask);
}

void
LogComponent::Disable(const LogLevel level)
{
    m_levels &= ~level;
}

std::string
LogComponent::Name() const
{
    return m_name;
}

std::string
LogComponent::File() const
{
    return m_file;
}

/* static */
std::string
LogComponent::GetLevelLabel(const LogLevel level)
{
    auto it = LOG_LEVEL_LABELS.find(level);
    if (it != LOG_LEVEL_LABELS.end())
    {
        return it->second;
    }
    return "unknown";
}

void
LogComponentEnable(const std::string& name, LogLevel level)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();
    auto logComponent = components->find(name);

    if (logComponent == components->end())
    {
        NS_LOG_UNCOND("Logging component \"" << name << "\" not found.");
        LogComponentPrintList();
        NS_FATAL_ERROR("Logging component \""
                       << name << "\" not found."
                       << " See above for a list of available log components");
    }

    logComponent->second->Enable(level);
}

void
LogComponentEnableAll(LogLevel level)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();
    for (auto i = components->begin(); i != components->end(); i++)
    {
        i->second->Enable(level);
    }
}

void
LogComponentDisable(const std::string& name, LogLevel level)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();
    auto logComponent = components->find(name);

    if (logComponent != components->end())
    {
        logComponent->second->Disable(level);
    }
}

void
LogComponentDisableAll(LogLevel level)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();
    for (auto i = components->begin(); i != components->end(); i++)
    {
        i->second->Disable(level);
    }
}

void
LogComponentPrintList()
{
    // Create sorted map of components by inserting them into a map
    std::map<std::string, LogComponent*> componentsSorted;

    for (const auto& component : *LogComponent::GetComponentList())
    {
        componentsSorted.insert(component);
    }

    // Iterate through sorted components
    for (const auto& [name, component] : componentsSorted)
    {
        std::cout << name << "=";
        if (component->IsNoneEnabled())
        {
            std::cout << "0" << std::endl;
            continue;
        }
        if (component->IsEnabled(LOG_LEVEL_ALL))
        {
            std::cout << "all";
        }
        else
        {
            if (component->IsEnabled(LOG_ERROR))
            {
                std::cout << "error";
            }
            if (component->IsEnabled(LOG_WARN))
            {
                std::cout << "|warn";
            }
            if (component->IsEnabled(LOG_INFO))
            {
                std::cout << "|info";
            }
            if (component->IsEnabled(LOG_LOGIC))
            {
                std::cout << "|logic";
            }
            if (component->IsEnabled(LOG_DEBUG))
            {
                std::cout << "|debug";
            }
            if (component->IsEnabled(LOG_FUNCTION))
            {
                std::cout << "|function";
            }
        }
        if (component->IsEnabled(LOG_PREFIX_ALL))
        {
            std::cout << "|prefix_all";
        }
        else
        {
            if (component->IsEnabled(LOG_PREFIX_FUNC))
            {
                std::cout << "|func";
            }
            if (component->IsEnabled(LOG_PREFIX_TIME))
            {
                std::cout << "|time";
            }
            if (component->IsEnabled(LOG_PREFIX_NODE))
            {
                std::cout << "|node";
            }
            if (component->IsEnabled(LOG_PREFIX_LEVEL))
            {
                std::cout << "|level";
            }
        }
        std::cout << std::endl;
    }
}

/**
 * @ingroup logging
 * Check if a log component exists.
 * This is private to the logging implementation.
 *
 * @param [in] componentName The putative log component name.
 * @returns \c true if \c componentName exists.
 */
static bool
ComponentExists(std::string componentName)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();

    return components->find(componentName) != components->end();
}

/**
 * @ingroup logging
 * Parse the \c NS_LOG environment variable.
 * This is private to the logging implementation.
 */
static void
CheckEnvironmentVariables()
{
    auto dict = EnvironmentVariable::GetDictionary("NS_LOG", ":")->GetStore();

    for (auto& [component, value] : dict)
    {
        if (component != "*" && component != "***" && !ComponentExists(component))
        {
            NS_LOG_UNCOND("Invalid or unregistered component name \"" << component << "\"");
            LogComponentPrintList();
            NS_FATAL_ERROR(
                "Invalid or unregistered component name \""
                << component
                << "\" in env variable NS_LOG, see above for a list of valid components");
        }

        // No valid component or wildcard
        if (value.empty())
        {
            continue;
        }

        // We have a valid component or wildcard, check the flags present in value
        StringVector flags = SplitString(value, "|");
        for (const auto& flag : flags)
        {
            // Handle wild cards
            if (flag == "*" || flag == "**")
            {
                continue;
            }
            bool ok = LOG_LABEL_LEVELS.find(flag) != LOG_LABEL_LEVELS.end();
            if (!ok)
            {
                NS_FATAL_ERROR("Invalid log level \""
                               << flag << "\" in env variable NS_LOG for component name "
                               << component);
            }
        }
    }
}

void
LogSetTimePrinter(TimePrinter printer)
{
    g_logTimePrinter = printer;
    /**
     * @internal
     * This is the only place where we are more or less sure that all log variables
     * are registered. See \bugid{1082} for details.
     */
    CheckEnvironmentVariables();
}

TimePrinter
LogGetTimePrinter()
{
    return g_logTimePrinter;
}

void
LogSetNodePrinter(NodePrinter printer)
{
    g_logNodePrinter = printer;
}

NodePrinter
LogGetNodePrinter()
{
    return g_logNodePrinter;
}

ParameterLogger::ParameterLogger(std::ostream& os)
    : m_os(os)
{
}

std::size_t ParameterLogger::m_maxLoggedContainerElements = 10;

void
ParameterLogger::SetMaxLoggedContainerElements(std::size_t max)
{
    m_maxLoggedContainerElements = max;
}

std::size_t
ParameterLogger::GetMaxLoggedContainerElements() const
{
    return m_maxLoggedContainerElements;
}

void
ParameterLogger::CommaRest()
{
    if (m_first)
    {
        m_first = false;
    }
    else
    {
        m_os << ", ";
    }
}

} // namespace ns3
