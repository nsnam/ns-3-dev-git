/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "command-line.h"

#include "config.h"
#include "des-metrics.h"
#include "environment-variable.h"
#include "global-value.h"
#include "log.h"
#include "string.h"
#include "system-path.h"
#include "type-id.h"

#if defined(ENABLE_BUILD_VERSION)
#include "version.h"
#endif

#include <algorithm> // transform
#include <cctype>    // tolower
#include <cstdlib>   // exit
#include <cstring>   // strlen
#include <iomanip>   // setw, boolalpha
#include <set>
#include <sstream>

/**
 * @file
 * @ingroup commandline
 * ns3::CommandLine implementation.
 */

/** CommandLine anonymous namespace. */
namespace
{
/**
 * HTML-encode a string, for PrintDoxygenUsage().
 * Usage and help strings, which are intended for text-only display,
 * can contain illegal characters for HTML.  This function
 * encodes '&', '\"', '\'',  and '<'.
 * @param [in] source The original string.
 * @returns The HTML-encoded version.
 */
std::string
Encode(const std::string& source)
{
    std::string buffer;
    buffer.reserve(1.1 * source.size());

    for (size_t pos = 0; pos != source.size(); ++pos)
    {
        switch (source[pos])
        {
        case '&':
            buffer.append("&amp;");
            break;
        case '\"':
            buffer.append("&quot;");
            break;
        case '\'':
            buffer.append("&apos;");
            break;
            // case '>':  buffer.append ("&gt;");           break;

        case '<':
            // Special case:
            // "...blah <file..." is not allowed
            // "...foo<bar..."  is allowed
            if (buffer.empty() || buffer.back() == ' ')
            {
                buffer.append("&lt;");
            }
            else
            {
                buffer.append("<");
            }

            break;

        default:
            buffer.append(&source[pos], 1);
            break;
        }
    }
    return buffer;
}

} // anonymous namespace

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CommandLine");

CommandLine::CommandLine()
    : m_NNonOptions(0),
      m_nonOptionCount(0),
      m_usage(),
      m_shortName()
{
    NS_LOG_FUNCTION(this);
}

CommandLine::CommandLine(const std::string& filename)
    : m_NNonOptions(0),
      m_nonOptionCount(0),
      m_usage()
{
    NS_LOG_FUNCTION(this << filename);
    std::string basename = SystemPath::Split(filename).back();
    m_shortName = basename.substr(0, basename.rfind(".cc"));
    m_shortName = m_shortName.substr(basename.find_last_of('/') + 1);
}

CommandLine::CommandLine(const CommandLine& cmd)
{
    Copy(cmd);
}

CommandLine&
CommandLine::operator=(const CommandLine& cmd)
{
    Clear();
    Copy(cmd);
    return *this;
}

CommandLine::~CommandLine()
{
    NS_LOG_FUNCTION(this);
    Clear();
}

void
CommandLine::Copy(const CommandLine& cmd)
{
    NS_LOG_FUNCTION(&cmd);

    std::copy(cmd.m_options.begin(), cmd.m_options.end(), m_options.end());
    std::copy(cmd.m_nonOptions.begin(), cmd.m_nonOptions.end(), m_nonOptions.end());

    m_NNonOptions = cmd.m_NNonOptions;
    m_nonOptionCount = 0;
    m_usage = cmd.m_usage;
    m_shortName = cmd.m_shortName;
}

void
CommandLine::Clear()
{
    NS_LOG_FUNCTION(this);

    m_options.clear();
    m_nonOptions.clear();
    m_NNonOptions = 0;
    m_usage = "";
    m_shortName = "";
}

void
CommandLine::Usage(const std::string& usage)
{
    m_usage = usage;
}

std::string
CommandLine::GetName() const
{
    return m_shortName;
}

CommandLine::Item::~Item()
{
    NS_LOG_FUNCTION(this);
}

void
CommandLine::Parse(std::vector<std::string> args)
{
    NS_LOG_FUNCTION(this << args.size() << args);

    PrintDoxygenUsage();

    m_nonOptionCount = 0;

    if (!args.empty())
    {
        args.erase(args.begin()); // discard the program name

        HandleHardOptions(args);

        for (const auto& param : args)
        {
            if (HandleOption(param))
            {
                continue;
            }
            if (HandleNonOption(param))
            {
                continue;
            }

            // is this possible?
            NS_ASSERT_MSG(false,
                          "unexpected error parsing command line parameter: '" << param << "'");
        }
    }

#ifdef ENABLE_DES_METRICS
    DesMetrics::Get()->Initialize(args);
#endif
}

CommandLine::HasOptionName
CommandLine::GetOptionName(const std::string& param) const
{
    // remove leading "--" or "-"
    std::string arg = param;
    std::string::size_type cur = arg.find("--");
    if (cur == 0)
    {
        arg = arg.substr(2, arg.size() - 2);
    }
    else
    {
        cur = arg.find('-');
        if (cur == 0)
        {
            arg = arg.substr(1, arg.size() - 1);
        }
        else
        {
            // non-option argument?
            return {false, param, ""};
        }
    }

    // find any value following '='
    cur = arg.find('=');
    std::string name;
    std::string value;
    if (cur == std::string::npos)
    {
        name = arg;
        value = "";
    }
    else
    {
        name = arg.substr(0, cur);
        value = arg.substr(cur + 1, arg.size() - (cur + 1));
    }

    return {true, name, value};
}

void
CommandLine::HandleHardOptions(const std::vector<std::string>& args) const
{
    NS_LOG_FUNCTION(this << args.size() << args);

    for (const auto& param : args)
    {
        auto [isOpt, name, value] = GetOptionName(param);
        if (!isOpt)
        {
            continue;
        }

        // Hard-coded options
        if (name == "PrintHelp" || name == "help")
        {
            // method below never returns.
            PrintHelp(std::cout);
            std::exit(0);
        }
        if (name == "PrintVersion" || name == "version")
        {
            // Print the version, then exit the program
            PrintVersion(std::cout);
            std::exit(0);
        }
        else if (name == "PrintGroups")
        {
            // method below never returns.
            PrintGroups(std::cout);
            std::exit(0);
        }
        else if (name == "PrintTypeIds")
        {
            // method below never returns.
            PrintTypeIds(std::cout);
            std::exit(0);
        }
        else if (name == "PrintGlobals")
        {
            // method below never returns.
            PrintGlobals(std::cout);
            std::exit(0);
        }
        else if (name == "PrintGroup")
        {
            // method below never returns.
            PrintGroup(std::cout, value);
            std::exit(0);
        }
        else if (name == "PrintAttributes")
        {
            // method below never returns.
            PrintAttributes(std::cout, value);
            std::exit(0);
        }
    }
}

bool
CommandLine::HandleOption(const std::string& param) const
{
    auto [isOpt, name, value] = GetOptionName(param);
    if (!isOpt)
    {
        return false;
    }

    HandleArgument(name, value);

    return true;
}

bool
CommandLine::HandleNonOption(const std::string& value)
{
    NS_LOG_FUNCTION(this << value);

    if (m_nonOptionCount == m_nonOptions.size())
    {
        // Add an unspecified non-option as a string
        NS_LOG_LOGIC("adding StringItem, NOCount:" << m_nonOptionCount
                                                   << ", NOSize:" << m_nonOptions.size());
        auto item = std::make_shared<StringItem>();
        item->m_name = "extra-non-option-argument";
        item->m_help = "Extra non-option argument encountered.";
        item->m_value = value;
        m_nonOptions.push_back(item);
    }

    auto i = m_nonOptions[m_nonOptionCount];
    if (!i->Parse(value))
    {
        std::cerr << "Invalid non-option argument value " << value << " for " << i->m_name
                  << std::endl;
        PrintHelp(std::cerr);
        std::exit(1);
    }
    ++m_nonOptionCount;
    return true;
}

void
CommandLine::Parse(int argc, char* argv[])
{
    NS_LOG_FUNCTION(this << argc);
    std::vector<std::string> args(argv, argv + argc);
    Parse(args);
}

void
CommandLine::PrintHelp(std::ostream& os) const
{
    NS_LOG_FUNCTION(this);

    // Hack to show just the declared non-options
    Items nonOptions(m_nonOptions.begin(), m_nonOptions.begin() + m_NNonOptions);
    os << m_shortName << (!m_options.empty() ? " [Program Options]" : "")
       << (!nonOptions.empty() ? " [Program Arguments]" : "") << " [General Arguments]"
       << std::endl;

    if (!m_usage.empty())
    {
        os << std::endl;
        os << m_usage << std::endl;
    }

    std::size_t width = 0;
    auto max_width = [&width](const std::shared_ptr<Item> item) {
        width = std::max(width, item->m_name.size());
    };
    std::for_each(m_options.begin(), m_options.end(), max_width);
    std::for_each(nonOptions.begin(), nonOptions.end(), max_width);
    width += 3; // room for ":  " between option and help

    auto optionsHelp = [&os, width](const std::string& head, bool option, const Items& items) {
        os << "\n" << head << "\n";
        for (const auto& item : items)
        {
            os << "    " << (option ? "--" : "  ") << std::left << std::setw(width)
               << (item->m_name + ":") << std::right << item->m_help;

            if (item->HasDefault())
            {
                os << " [" << item->GetDefault() << "]";
            }
            os << "\n";
        }
    };

    if (!m_options.empty())
    {
        optionsHelp("Program Options:", true, m_options);
    }

    if (!nonOptions.empty())
    {
        optionsHelp("Program Arguments:", false, nonOptions);
    }

    os << std::endl;
    os << "General Arguments:\n"
       << "    --PrintGlobals:              Print the list of globals.\n"
       << "    --PrintGroups:               Print the list of groups.\n"
       << "    --PrintGroup=[group]:        Print all TypeIds of group.\n"
       << "    --PrintTypeIds:              Print all TypeIds.\n"
       << "    --PrintAttributes=[typeid]:  Print all attributes of typeid.\n"
       << "    --PrintVersion:              Print the ns-3 version.\n"
       << "    --PrintHelp:                 Print this help message.\n"
       << std::endl;
}

std::string
CommandLine::GetVersion() const
{
#if defined(ENABLE_BUILD_VERSION)
    return Version::LongVersion();
#else
    return std::string{"Build version support is not enabled, reconfigure with "
                       "--enable-build-version flag"};
#endif
}

void
CommandLine::PrintVersion(std::ostream& os) const
{
    os << GetVersion() << std::endl;
}

void
CommandLine::PrintDoxygenUsage() const
{
    NS_LOG_FUNCTION(this);

    auto [found, path] = EnvironmentVariable::Get("NS_COMMANDLINE_INTROSPECTION");
    if (!found)
    {
        return;
    }

    if (m_shortName.empty())
    {
        NS_FATAL_ERROR("No file name on example-to-run; forgot to use CommandLine var (__FILE__)?");
        return;
    }

    // Hack to show just the declared non-options
    Items nonOptions(m_nonOptions.begin(), m_nonOptions.begin() + m_NNonOptions);

    std::string outf = SystemPath::Append(path, m_shortName + ".command-line");

    NS_LOG_INFO("Writing CommandLine doxy to " << outf);

    std::fstream os(outf, std::fstream::out);

    os << "/**\n \\file " << m_shortName << ".cc\n"
       << "<h3>Usage</h3>\n"
       << "<code>$ ./ns3 run \"" << m_shortName << (!m_options.empty() ? " [Program Options]" : "")
       << (!nonOptions.empty() ? " [Program Arguments]" : "") << "\"</code>\n";

    if (!m_usage.empty())
    {
        os << Encode(m_usage) << "\n";
    }

    auto listOptions = [&os](const std::string& head, const Items& items, std::string pre) {
        os << "\n<h3>" << head << "</h3>\n<dl>\n";
        for (const auto& i : items)
        {
            os << "  <dt>" << pre << i->m_name << " </dt>\n"
               << "    <dd>" << Encode(i->m_help);

            if (i->HasDefault())
            {
                os << " [" << Encode(i->GetDefault()) << "]";
            }
            os << " </dd>\n";
        }
        os << "</dl>\n";
    };

    if (!m_options.empty())
    {
        listOptions("Program Options", m_options, "\\c --");
    }

    if (!nonOptions.empty())
    {
        listOptions("Program Arguments", nonOptions, "\\c ");
    }

    os << "*/" << std::endl;

    // All done, don't need to actually run the example
    os.close();
    std::exit(0);
}

void
CommandLine::PrintGlobals(std::ostream& os) const
{
    NS_LOG_FUNCTION(this);

    os << "Global values:" << std::endl;

    // Sort output
    std::vector<std::string> globals;

    for (auto i = GlobalValue::Begin(); i != GlobalValue::End(); ++i)
    {
        std::stringstream ss;
        ss << "    --" << (*i)->GetName() << "=[";
        Ptr<const AttributeChecker> checker = (*i)->GetChecker();
        StringValue v;
        (*i)->GetValue(v);
        ss << v.Get() << "]" << std::endl;
        ss << "        " << (*i)->GetHelp() << std::endl;
        globals.push_back(ss.str());
    }
    std::sort(globals.begin(), globals.end());
    for (const auto& s : globals)
    {
        os << s;
    }
}

void
CommandLine::PrintAttributeList(std::ostream& os, const TypeId tid, std::stringstream& header) const
{
    NS_LOG_FUNCTION(this);

    if (!tid.GetAttributeN())
    {
        return;
    }
    os << header.str() << "\n";
    // To sort output
    std::vector<std::string> attributes;

    for (uint32_t i = 0; i < tid.GetAttributeN(); ++i)
    {
        std::stringstream ss;
        ss << "    --" << tid.GetAttributeFullName(i) << "=[";
        TypeId::AttributeInformation info = tid.GetAttribute(i);
        ss << info.initialValue->SerializeToString(info.checker) << "]\n"
           << "        " << info.help << "\n";
        attributes.push_back(ss.str());
    }
    std::sort(attributes.begin(), attributes.end());
    for (const auto& s : attributes)
    {
        os << s;
    }
}

void
CommandLine::PrintAttributes(std::ostream& os, const std::string& type) const
{
    NS_LOG_FUNCTION(this);

    TypeId tid;
    if (!TypeId::LookupByNameFailSafe(type, &tid))
    {
        NS_FATAL_ERROR("Unknown type=" << type << " in --PrintAttributes");
    }

    std::stringstream header;
    header << "Attributes for TypeId " << tid.GetName();
    PrintAttributeList(os, tid, header);
    header.str("");

    // Parent Attributes
    if (tid.GetParent() != tid)
    {
        TypeId tmp = tid.GetParent();
        while (tmp.GetParent() != tmp)
        {
            header << "Attributes defined in parent class " << tmp.GetName();
            PrintAttributeList(os, tmp, header);
            header.str("");
            tmp = tmp.GetParent();
        }
    }
}

void
CommandLine::PrintGroup(std::ostream& os, const std::string& group) const
{
    NS_LOG_FUNCTION(this);

    os << "TypeIds in group " << group << ":" << std::endl;

    // Sort output
    std::vector<std::string> groupTypes;

    for (uint16_t i = 0; i < TypeId::GetRegisteredN(); ++i)
    {
        std::stringstream ss;
        TypeId tid = TypeId::GetRegistered(i);
        if (tid.GetGroupName() == group)
        {
            ss << "    " << tid.GetName() << std::endl;
        }
        groupTypes.push_back(ss.str());
    }
    std::sort(groupTypes.begin(), groupTypes.end());
    for (const auto& s : groupTypes)
    {
        os << s;
    }
}

void
CommandLine::PrintTypeIds(std::ostream& os) const
{
    NS_LOG_FUNCTION(this);
    os << "Registered TypeIds:" << std::endl;

    // Sort output
    std::vector<std::string> types;

    for (uint16_t i = 0; i < TypeId::GetRegisteredN(); ++i)
    {
        std::stringstream ss;
        TypeId tid = TypeId::GetRegistered(i);
        ss << "    " << tid.GetName() << std::endl;
        types.push_back(ss.str());
    }
    std::sort(types.begin(), types.end());
    for (const auto& s : types)
    {
        os << s;
    }
}

void
CommandLine::PrintGroups(std::ostream& os) const
{
    NS_LOG_FUNCTION(this);

    std::set<std::string> groups;
    for (uint16_t i = 0; i < TypeId::GetRegisteredN(); ++i)
    {
        TypeId tid = TypeId::GetRegistered(i);
        groups.insert(tid.GetGroupName());
    }

    os << "Registered TypeId groups:" << std::endl;
    // Sets are already sorted
    for (const auto& s : groups)
    {
        os << "    " << s << std::endl;
    }
}

bool
CommandLine::HandleArgument(const std::string& name, const std::string& value) const
{
    NS_LOG_FUNCTION(this << name << value);

    NS_LOG_DEBUG("Handle arg name=" << name << " value=" << value);

    auto errorExit = [this, name, value]() {
        std::cerr << "Invalid command-line argument: --" << name;
        if (!value.empty())
        {
            std::cerr << "=" << value;
        }
        std::cerr << std::endl;
        this->PrintHelp(std::cerr);
        std::exit(1);
    };

    auto item = std::find_if(m_options.begin(), m_options.end(), [name](std::shared_ptr<Item> it) {
        return it->m_name == name;
    });
    if (item != m_options.end())
    {
        if (!(*item)->Parse(value))
        {
            errorExit();
        }
        return true;
    }

    // Global or ConfigPath options
    if (!HandleAttribute(name, value))
    {
        errorExit();
    }
    return true;
}

bool
CommandLine::CallbackItem::HasDefault() const
{
    return !m_default.empty();
}

std::string
CommandLine::CallbackItem::GetDefault() const
{
    return m_default;
}

bool
CommandLine::CallbackItem::Parse(const std::string& value) const
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("CommandLine::CallbackItem::Parse \"" << value << "\"");
    return m_callback(value);
}

void
CommandLine::AddValue(const std::string& name,
                      const std::string& help,
                      char* value,
                      std::size_t num)
{
    NS_LOG_FUNCTION(this << name << help << value << num);
    auto item = std::make_shared<CharStarItem>();
    item->m_name = name;
    item->m_help = help;
    item->m_buffer = value;
    item->m_size = num;
    item->m_default.assign(value);
    m_options.push_back(item);
}

void
CommandLine::AddValue(const std::string& name,
                      const std::string& help,
                      ns3::Callback<bool, const std::string&> callback,
                      const std::string& defaultValue /* = "" */)

{
    NS_LOG_FUNCTION(this << &name << &help << &callback);
    auto item = std::make_shared<CallbackItem>();
    item->m_name = name;
    item->m_help = help;
    item->m_callback = callback;
    item->m_default = defaultValue;
    m_options.push_back(item);
}

void
CommandLine::AddValue(const std::string& name, const std::string& attributePath)
{
    NS_LOG_FUNCTION(this << name << attributePath);
    // Attribute name is last token
    std::size_t colon = attributePath.rfind("::");
    const std::string typeName = attributePath.substr(0, colon);
    NS_LOG_DEBUG("typeName: '" << typeName << "', colon: " << colon);

    TypeId tid;
    if (!TypeId::LookupByNameFailSafe(typeName, &tid))
    {
        NS_FATAL_ERROR("Unknown type=" << typeName);
    }

    const std::string attrName = attributePath.substr(colon + 2);
    TypeId::AttributeInformation info;
    if (!tid.LookupAttributeByName(attrName, &info))
    {
        NS_FATAL_ERROR("Attribute not found: " << attributePath);
    }

    std::stringstream ss;
    ss << info.help << " (" << attributePath << ") ["
       << info.initialValue->SerializeToString(info.checker) << "]";

    AddValue(name, ss.str(), MakeBoundCallback(CommandLine::HandleAttribute, attributePath));
}

std::string
CommandLine::GetExtraNonOption(std::size_t i) const
{
    std::string value;

    if (m_nonOptions.size() >= i + m_NNonOptions)
    {
        auto ip = std::dynamic_pointer_cast<StringItem>(m_nonOptions[i + m_NNonOptions]);
        if (ip != nullptr)
        {
            value = ip->m_value;
        }
    }
    return value;
}

std::size_t
CommandLine::GetNExtraNonOptions() const
{
    if (m_nonOptions.size() > m_NNonOptions)
    {
        return m_nonOptions.size() - m_NNonOptions;
    }
    else
    {
        return 0;
    }
}

/* static */
bool
CommandLine::HandleAttribute(const std::string& name, const std::string& value)
{
    return Config::SetGlobalFailSafe(name, StringValue(value)) ||
           Config::SetDefaultFailSafe(name, StringValue(value));
}

bool
CommandLine::Item::HasDefault() const
{
    return false;
}

bool
CommandLine::StringItem::Parse(const std::string& value) const
{
    m_value = value; // mutable
    return true;
}

bool
CommandLine::StringItem::HasDefault() const
{
    return false;
}

std::string
CommandLine::StringItem::GetDefault() const
{
    return "";
}

bool
CommandLine::CharStarItem::Parse(const std::string& value) const
{
    if (value.size() > m_size - 1)
    {
        std::cerr << "Value \"" << value << "\" (" << value.size() << " bytes) is too long for "
                  << m_name << " buffer (" << m_size << " bytes, including terminating null)."
                  << std::endl;
        return false;
    }

    std::strncpy(m_buffer, value.c_str(), m_size);
    return true;
}

bool
CommandLine::CharStarItem::HasDefault() const
{
    return true;
}

std::string
CommandLine::CharStarItem::GetDefault() const
{
    return m_default;
}

template <>
std::string
CommandLineHelper::GetDefault<bool>(const std::string& defaultValue)
{
    bool value;
    std::istringstream iss(defaultValue);
    iss >> value;
    std::ostringstream oss;
    oss << std::boolalpha << value;
    return oss.str();
}

template <>
bool
CommandLineHelper::UserItemParse<bool>(const std::string& value, bool& dest)
{
    // No new value, so just toggle it
    if (value.empty())
    {
        dest = !dest;
        return true;
    }

    std::string src = value;
    std::transform(src.begin(), src.end(), src.begin(), [](char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (src == "true" || src == "t")
    {
        dest = true;
        return true;
    }
    else if (src == "false" || src == "f")
    {
        dest = false;
        return true;
    }
    else
    {
        std::istringstream iss;
        iss.str(src);
        iss >> dest;
        return !iss.bad() && !iss.fail();
    }
}

template <>
std::string
CommandLineHelper::GetDefault<Time>(const std::string& defaultValue)
{
    std::ostringstream oss;
    oss << Time(defaultValue).As();
    return oss.str();
}

template <>
bool
CommandLineHelper::UserItemParse<uint8_t>(const std::string& value, uint8_t& dest)
{
    uint8_t oldDest = dest;
    int newDest;

    try
    {
        newDest = std::stoi(value);
    }
    catch (std::invalid_argument& ia)
    {
        NS_LOG_WARN("invalid argument: " << ia.what());
        dest = oldDest;
        return false;
    }
    catch (std::out_of_range& oor)
    {
        NS_LOG_WARN("out of range: " << oor.what());
        dest = oldDest;
        return false;
    }
    if (newDest < 0 || newDest > 255)
    {
        return false;
    }
    dest = newDest;
    return true;
}

std::ostream&
operator<<(std::ostream& os, const CommandLine& cmd)
{
    cmd.PrintHelp(os);
    return os;
}

} // namespace ns3
