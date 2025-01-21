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

#include "ns3/core-config.h"

#include <algorithm> // transform
#include <cstring>   // strlen
#include <iostream>
#include <list>
#include <locale> // toupper
#include <map>
#include <numeric> // accumulate
#include <stdexcept>
#include <utility>

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
    Enable((LogLevel)level);
}

bool
LogComponent::IsEnabled(const LogLevel level) const
{
    //  LogComponentEnableEnvVar ();
    return level & m_levels;
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
            if (component->IsEnabled(LOG_DEBUG))
            {
                std::cout << "|debug";
            }
            if (component->IsEnabled(LOG_INFO))
            {
                std::cout << "|info";
            }
            if (component->IsEnabled(LOG_FUNCTION))
            {
                std::cout << "|function";
            }
            if (component->IsEnabled(LOG_LOGIC))
            {
                std::cout << "|logic";
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
