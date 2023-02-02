/*
 * Copyright (c) 2007 INRIA
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
 */

/**
 * \file
 * \ingroup utils
 * Generate documentation from the TypeId database.
 */

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/global-value.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/object-vector.h"
#include "ns3/object.h"
#include "ns3/pointer.h"
#include "ns3/simple-channel.h"
#include "ns3/string.h"
#include "ns3/system-path.h"

#include <algorithm>
#include <climits> // CHAR_BIT
#include <iomanip>
#include <iostream>
#include <map>
#include <utility> // as_const

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PrintIntrospectedDoxygen");

namespace
{
/** Are we generating text or Doxygen? */
bool outputText = false;

/**
 * Markup tokens.
 * @{
 */
std::string anchor;        ///< hyperlink anchor
std::string argument;      ///< function argument
std::string boldStart;     ///< start of bold span
std::string boldStop;      ///< end of bold span
std::string breakBoth;     ///< linebreak
std::string breakHtmlOnly; ///< linebreak for html output only
std::string breakTextOnly; ///< linebreak for text output only
std::string brief;         ///< brief tag
std::string classStart;    ///< start of a class
std::string classStop;     ///< end of a class
std::string codeWord;      ///< format next word as source code
std::string commentStart;  ///< start of code comment
std::string commentStop;   ///< end of code comment
std::string copyDoc;       ///< copy (or refer) to docs elsewhere
std::string file;          ///< file
std::string flagSpanStart; ///< start of Attribute flag value
std::string flagSpanStop;  ///< end of Attribute flag value
std::string functionStart; ///< start of a method/function
std::string functionStop;  ///< end of a method/function
std::string headingStart;  ///< start of section heading (h3)
std::string headingStop;   ///< end of section heading (h3)
// Linking:  [The link text displayed](\ref TheTarget)
std::string hrefStart;        ///< start of a link
std::string hrefMid;          ///< middle part of a link
std::string hrefStop;         ///< end of a link
std::string indentHtmlOnly;   ///< small indent
std::string listLineStart;    ///< start unordered list item
std::string listLineStop;     ///< end unordered list item
std::string listStart;        ///< start unordered list
std::string listStop;         ///< end unordered list
std::string note;             ///< start a note section
std::string page;             ///< start a separate page
std::string reference;        ///< reference tag
std::string referenceNo;      ///< block automatic references
std::string returns;          ///< the return value
std::string sectionStart;     ///< start of a section or group
std::string seeAlso;          ///< Reference to other docs
std::string subSectionStart;  ///< start a new subsection
std::string templArgDeduced;  ///< template argument deduced from function
std::string templArgExplicit; ///< template argument required
std::string templateArgument; ///< template argument
std::string variable;         ///< variable or class member

/** @} */

/**
 * Alphabetize the AttributeInformation for a TypeId by the Attribute name
 * \param tid The TypeId to process.
 * \return The ordered list of Attributes.
 */
std::map<std::string, ns3::TypeId::AttributeInformation>
SortedAttributeInfo(const TypeId tid)
{
    std::map<std::string, ns3::TypeId::AttributeInformation> index;
    for (uint32_t j = 0; j < tid.GetAttributeN(); j++)
    {
        struct TypeId::AttributeInformation info = tid.GetAttribute(j);
        index[info.name] = info;
    }
    return index;
}

/**
 * Alphabetize the TraceSourceInformation for a TypeId by the
 * TraceSource name.
 * \param tid The TypeId to process.
 * \return The ordered list of TraceSourceInformation
 */
std::map<std::string, ns3::TypeId::TraceSourceInformation>
SortedTraceSourceInfo(const TypeId tid)
{
    std::map<std::string, ns3::TypeId::TraceSourceInformation> index;
    for (uint32_t j = 0; j < tid.GetTraceSourceN(); j++)
    {
        struct TypeId::TraceSourceInformation info = tid.GetTraceSource(j);
        index[info.name] = info;
    }
    return index;
}

} // unnamed namespace

/**
 * Initialize the markup strings, for either doxygen or text.
 */
void
SetMarkup()
{
    NS_LOG_FUNCTION(outputText);
    if (outputText)
    {
        anchor = "";
        argument = "  Arg: ";
        boldStart = "";
        boldStop = "";
        breakBoth = "\n";
        breakHtmlOnly = "";
        breakTextOnly = "\n";
        brief = "";
        classStart = "";
        classStop = "\n\n";
        codeWord = " ";
        commentStart = "===============================================================\n";
        commentStop = "";
        copyDoc = "  See: ";
        file = "File: introspected-doxygen.txt";
        flagSpanStart = "";
        flagSpanStop = "";
        functionStart = "";
        functionStop = "\n\n";
        headingStart = "";
        headingStop = "";
        // Linking:  The link text displayed (see TheTarget)
        hrefStart = "";
        hrefMid = "(see ";
        hrefStop = ")";
        indentHtmlOnly = "";
        listLineStart = "    * ";
        listLineStop = "";
        listStart = "";
        listStop = "";
        note = "Note: ";
        page = "Page ";
        reference = " ";
        referenceNo = " ";
        returns = "  Returns: ";
        sectionStart = "Section:  ";
        seeAlso = "  See: ";
        subSectionStart = "Subsection ";
        templArgDeduced = "[deduced]  ";
        templArgExplicit = "[explicit] ";
        templateArgument = "Template Arg: ";
        variable = "Variable: ";
    }
    else
    {
        anchor = "\\anchor ";
        argument = "\\param ";
        boldStart = "<b>";
        boldStop = "</b>";
        breakBoth = "<br>";
        breakHtmlOnly = "<br>";
        breakTextOnly = "";
        brief = "\\brief ";
        classStart = "\\class ";
        classStop = "";
        codeWord = "\\p ";
        commentStart = "/*!\n";
        commentStop = "*/\n";
        copyDoc = "\\copydoc ";
        file = "\\file";
        flagSpanStart = "<span class=\"mlabel\">";
        flagSpanStop = "</span>";
        functionStart = "\\fn ";
        functionStop = "";
        headingStart = "<h3>";
        headingStop = "</h3>";
        // Linking:  [The link text displayed](\ref TheTarget)
        hrefStart = "[";
        hrefMid = "](\\ref ";
        hrefStop = ")";
        indentHtmlOnly = "  ";
        listLineStart = "<li>";
        listLineStop = "</li>";
        listStart = "<ul>";
        listStop = "</ul>";
        note = "\\note ";
        page = "\\page ";
        reference = " \\ref ";
        referenceNo = " %";
        returns = "\\returns ";
        sectionStart = "\\ingroup ";
        seeAlso = "\\see ";
        subSectionStart = "\\addtogroup ";
        templArgDeduced = "\\deduced ";
        templArgExplicit = "\\explicit ";
        templateArgument = "\\tparam ";
        variable = "\\var ";
    }
} // SetMarkup ()

/***************************************************************
 *        Aggregation and configuration paths
 ***************************************************************/

/**
 * Gather aggregation and configuration path information from registered types.
 */
class StaticInformation
{
  public:
    /**
     * Record the a -> b aggregation relation.
     *
     * \param a [in] the source(?) TypeId name
     * \param b [in] the destination(?) TypeId name
     */
    void RecordAggregationInfo(std::string a, std::string b);
    /**
     * Gather aggregation and configuration path information for tid
     *
     * \param tid [in] the TypeId to gather information from
     */
    void Gather(TypeId tid);
    /**
     * Print output in "a -> b" form on std::cout
     */
    void Print() const;

    /**
     * \return the configuration paths for tid
     *
     * \param tid [in] the TypeId to return information for
     */
    std::vector<std::string> Get(TypeId tid) const;

    /**
     * \return the type names we couldn't aggregate.
     */
    std::vector<std::string> GetNoTypeIds() const;

  private:
    /**
     * \return the current configuration path
     */
    std::string GetCurrentPath() const;
    /**
     * Gather attribute, configuration path information for tid
     *
     * \param tid [in] the TypeId to gather information from
     */
    void DoGather(TypeId tid);
    /**
     *  Record the current config path for tid.
     *
     * \param tid [in] the TypeId to record.
     */
    void RecordOutput(TypeId tid);
    /**
     * \return whether the tid has already been processed
     *
     * \param tid [in] the TypeId to check.
     */
    bool HasAlreadyBeenProcessed(TypeId tid) const;
    /**
     * Configuration path for each TypeId
     */
    std::vector<std::pair<TypeId, std::string>> m_output;
    /**
     * Current configuration path
     */
    std::vector<std::string> m_currentPath;
    /**
     * List of TypeIds we've already processed
     */
    std::vector<TypeId> m_alreadyProcessed;
    /**
     * List of aggregation relationships.
     */
    std::vector<std::pair<TypeId, TypeId>> m_aggregates;
    /**
     * List of type names without TypeIds, because those modules aren't enabled.
     *
     * This is mutable because GetNoTypeIds sorts and uniquifies this list
     * before returning it.
     */
    mutable std::vector<std::string> m_noTids;

}; // class StaticInformation

void
StaticInformation::RecordAggregationInfo(std::string a, std::string b)
{
    NS_LOG_FUNCTION(this << a << b);
    TypeId aTid;
    bool found = TypeId::LookupByNameFailSafe(a, &aTid);
    if (!found)
    {
        m_noTids.push_back(a);
        return;
    }
    TypeId bTid;
    found = TypeId::LookupByNameFailSafe(b, &bTid);
    if (!found)
    {
        m_noTids.push_back(b);
        return;
    }

    m_aggregates.emplace_back(aTid, bTid);
}

void
StaticInformation::Print() const
{
    NS_LOG_FUNCTION(this);
    for (const auto& item : m_output)
    {
        std::cout << item.first.GetName() << " -> " << item.second << std::endl;
    }
}

std::string
StaticInformation::GetCurrentPath() const
{
    NS_LOG_FUNCTION(this);
    std::ostringstream oss;
    for (const auto& item : m_currentPath)
    {
        oss << "/" << item;
    }
    return oss.str();
}

void
StaticInformation::RecordOutput(TypeId tid)
{
    NS_LOG_FUNCTION(this << tid);
    m_output.emplace_back(tid, GetCurrentPath());
}

bool
StaticInformation::HasAlreadyBeenProcessed(TypeId tid) const
{
    NS_LOG_FUNCTION(this << tid);
    for (const auto& it : m_alreadyProcessed)
    {
        if (it == tid)
        {
            return true;
        }
    }
    return false;
}

std::vector<std::string>
StaticInformation::Get(TypeId tid) const
{
    NS_LOG_FUNCTION(this << tid);
    std::vector<std::string> paths;
    for (const auto& item : m_output)
    {
        if (item.first == tid)
        {
            paths.push_back(item.second);
        }
    }
    return paths;
}

/**
 * Helper to keep only the unique items in a container.
 *
 * The container is modified in place; the elements end up sorted.
 *
 * The container must support \c begin(), \c end() and \c erase(),
 * which, among the STL containers, limits this to
 * \c std::vector, \c std::dequeue and \c std::list.
 *
 * The container elements must support \c operator< (for \c std::sort)
 * and \c operator== (for \c std::unique).
 *
 * \tparam T \deduced The container type.
 * \param t The container.
 */
template <typename T>
void
Uniquefy(T t)
{
    std::sort(t.begin(), t.end());
    t.erase(std::unique(t.begin(), t.end()), t.end());
}

std::vector<std::string>
StaticInformation::GetNoTypeIds() const
{
    NS_LOG_FUNCTION(this);
    Uniquefy(m_noTids);
    return m_noTids;
}

void
StaticInformation::Gather(TypeId tid)
{
    NS_LOG_FUNCTION(this << tid);
    DoGather(tid);
    Uniquefy(m_output);
}

void
StaticInformation::DoGather(TypeId tid)
{
    NS_LOG_FUNCTION(this << tid);
    if (HasAlreadyBeenProcessed(tid))
    {
        return;
    }
    RecordOutput(tid);
    for (uint32_t i = 0; i < tid.GetAttributeN(); ++i)
    {
        struct TypeId::AttributeInformation info = tid.GetAttribute(i);
        const PointerChecker* ptrChecker =
            dynamic_cast<const PointerChecker*>(PeekPointer(info.checker));
        if (ptrChecker != nullptr)
        {
            TypeId pointee = ptrChecker->GetPointeeTypeId();

            // See if this is a pointer to an Object.
            Ptr<Object> object = CreateObject<Object>();
            TypeId objectTypeId = object->GetTypeId();
            if (objectTypeId == pointee)
            {
                // Stop the recursion at this attribute if it is a
                // pointer to an Object, which create too many spurious
                // paths in the list of attribute paths because any
                // Object can be in that part of the path.
                continue;
            }

            m_currentPath.push_back(info.name);
            m_alreadyProcessed.push_back(tid);
            DoGather(pointee);
            m_alreadyProcessed.pop_back();
            m_currentPath.pop_back();
            continue;
        }
        // attempt to cast to an object vector.
        const ObjectPtrContainerChecker* vectorChecker =
            dynamic_cast<const ObjectPtrContainerChecker*>(PeekPointer(info.checker));
        if (vectorChecker != nullptr)
        {
            TypeId item = vectorChecker->GetItemTypeId();
            m_currentPath.push_back(info.name + "/[i]");
            m_alreadyProcessed.push_back(tid);
            DoGather(item);
            m_alreadyProcessed.pop_back();
            m_currentPath.pop_back();
            continue;
        }
    }
    for (uint32_t j = 0; j < TypeId::GetRegisteredN(); j++)
    {
        TypeId child = TypeId::GetRegistered(j);
        if (child.IsChildOf(tid))
        {
            std::string childName = "$" + child.GetName();
            m_currentPath.push_back(childName);
            m_alreadyProcessed.push_back(tid);
            DoGather(child);
            m_alreadyProcessed.pop_back();
            m_currentPath.pop_back();
        }
    }
    for (const auto& item : m_aggregates)
    {
        if (item.first == tid || item.second == tid)
        {
            TypeId other;
            if (item.first == tid)
            {
                other = item.second;
            }
            if (item.second == tid)
            {
                other = item.first;
            }
            std::string name = "$" + other.GetName();
            m_currentPath.push_back(name);
            m_alreadyProcessed.push_back(tid);
            DoGather(other);
            m_alreadyProcessed.pop_back();
            m_currentPath.pop_back();
        }
    }
} // StaticInformation::DoGather ()

/// Register aggregation relationships that are not automatically
/// detected by this introspection program.  Statements added here
/// result in more configuration paths being added to the doxygen.
/// \return instance of StaticInformation with the registered information
StaticInformation
GetTypicalAggregations()
{
    NS_LOG_FUNCTION_NOARGS();

    static StaticInformation info;
    static bool mapped = false;

    if (mapped)
    {
        return info;
    }

    // Short circuit next call
    mapped = true;

    // The below statements register typical aggregation relationships
    // in ns-3 programs, that otherwise aren't picked up automatically
    // by the creation of the above node.  To manually list other common
    // aggregation relationships that you would like to see show up in
    // the list of configuration paths in the doxygen, add additional
    // statements below.
    info.RecordAggregationInfo("ns3::Node", "ns3::TcpSocketFactory");
    info.RecordAggregationInfo("ns3::Node", "ns3::UdpSocketFactory");
    info.RecordAggregationInfo("ns3::Node", "ns3::PacketSocketFactory");
    info.RecordAggregationInfo("ns3::Node", "ns3::MobilityModel");
    info.RecordAggregationInfo("ns3::Node", "ns3::Ipv4L3Protocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::Ipv4NixVectorRouting");
    info.RecordAggregationInfo("ns3::Node", "ns3::Icmpv4L4Protocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::ArpL3Protocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::Icmpv4L4Protocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::UdpL4Protocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::Ipv6L3Protocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::Icmpv6L4Protocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::TcpL4Protocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::RipNg");
    info.RecordAggregationInfo("ns3::Node", "ns3::GlobalRouter");
    info.RecordAggregationInfo("ns3::Node", "ns3::aodv::RoutingProtocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::dsdv::RoutingProtocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::dsr::DsrRouting");
    info.RecordAggregationInfo("ns3::Node", "ns3::olsr::RoutingProtocol");
    info.RecordAggregationInfo("ns3::Node", "ns3::EnergyHarvesterContainer");
    info.RecordAggregationInfo("ns3::Node", "ns3::EnergySourceContainer");

    // Create a channel object so that channels appear in the namespace
    // paths that will be generated here.
    Ptr<SimpleChannel> simpleChannel;
    simpleChannel = CreateObject<SimpleChannel>();

    for (uint32_t i = 0; i < Config::GetRootNamespaceObjectN(); ++i)
    {
        Ptr<Object> object = Config::GetRootNamespaceObject(i);
        info.Gather(object->GetInstanceTypeId());
    }

    return info;

} // GetTypicalAggregations ()

/// Map from TypeId name to tid
typedef std::map<std::string, int32_t> NameMap;
typedef NameMap::const_iterator NameMapIterator; ///< NameMap iterator

/**
 * Create a map from the class names to their index in the vector of
 * TypeId's so that the names will end up in alphabetical order.
 *
 * \returns NameMap
 */
NameMap
GetNameMap()
{
    NS_LOG_FUNCTION_NOARGS();

    static NameMap nameMap;
    static bool mapped = false;

    if (mapped)
    {
        return nameMap;
    }

    // Short circuit next call
    mapped = true;

    // Get typical aggregation relationships.
    StaticInformation info = GetTypicalAggregations();

    // Registered types
    for (uint32_t i = 0; i < TypeId::GetRegisteredN(); i++)
    {
        TypeId tid = TypeId::GetRegistered(i);
        if (tid.MustHideFromDocumentation())
        {
            continue;
        }

        // Capitalize all of letters in the name so that it sorts
        // correctly in the map.
        std::string name = tid.GetName();
        std::transform(name.begin(), name.end(), name.begin(), ::toupper);

        // Save this name's index.
        nameMap[name] = i;
    }

    // Type names without TypeIds
    std::vector<std::string> noTids = info.GetNoTypeIds();
    for (const auto& item : noTids)
    {
        nameMap[item] = -1;
    }

    return nameMap;
} // GetNameMap ()

/***************************************************************
 *        Docs for a single TypeId
 ***************************************************************/

/**
 * Print config paths
 * \param os the output stream
 * \param tid the type ID
 */
void
PrintConfigPaths(std::ostream& os, const TypeId tid)
{
    NS_LOG_FUNCTION(tid);
    std::vector<std::string> paths = GetTypicalAggregations().Get(tid);

    // Config --------------
    if (paths.empty())
    {
        os << "Introspection did not find any typical Config paths." << breakBoth << std::endl;
    }
    else
    {
        os << headingStart << "Config Paths" << headingStop << std::endl;
        os << std::endl;
        os << tid.GetName() << " is accessible through the following paths"
           << " with Config::Set and Config::Connect:" << std::endl;
        os << listStart << std::endl;
        for (const auto& path : paths)
        {
            os << listLineStart << "\"" << path << "\"" << listLineStop << breakTextOnly
               << std::endl;
        }
        os << listStop << std::endl;
    }
} // PrintConfigPaths ()

/**
 * Print direct Attributes for this TypeId.
 *
 * Only attributes defined directly by this TypeId will be printed.
 *
 * \param [in,out] os The output stream.
 * \param [in] tid The TypeId to print.
 */
void
PrintAttributesTid(std::ostream& os, const TypeId tid)
{
    NS_LOG_FUNCTION(tid);

    auto index = SortedAttributeInfo(tid);

    os << listStart << std::endl;
    for (const auto& [name, info] : index)
    {
        os << listLineStart << boldStart << name << boldStop << ": " << info.help << std::endl;
        os << indentHtmlOnly << listStart << std::endl;
        os << "    " << listLineStart << "Set with class: " << reference
           << info.checker->GetValueTypeName() << listLineStop << std::endl;

        std::string underType;
        if (info.checker->HasUnderlyingTypeInformation())
        {
            os << "    " << listLineStart << "Underlying type: ";

            std::string valType = info.checker->GetValueTypeName();
            underType = info.checker->GetUnderlyingTypeInformation();
            bool handled = false;
            if ((valType != "ns3::EnumValue") && (underType != "std::string"))
            {
                // Indirect cases to handle
                if (valType == "ns3::PointerValue")
                {
                    const PointerChecker* ptrChecker =
                        dynamic_cast<const PointerChecker*>(PeekPointer(info.checker));
                    if (ptrChecker != nullptr)
                    {
                        os << reference << "ns3::Ptr"
                           << "< " << reference << ptrChecker->GetPointeeTypeId().GetName() << ">";
                        handled = true;
                    }
                }
                else if (valType == "ns3::ObjectPtrContainerValue")
                {
                    const ObjectPtrContainerChecker* ptrChecker =
                        dynamic_cast<const ObjectPtrContainerChecker*>(PeekPointer(info.checker));
                    if (ptrChecker != nullptr)
                    {
                        os << reference << "ns3::Ptr"
                           << "< " << reference << ptrChecker->GetItemTypeId().GetName() << ">";
                        handled = true;
                    }
                }

                // Helper to match first part of string
                auto match = [&uType = std::as_const(underType)](const std::string& s) {
                    return uType.rfind(s, 0) == 0; // only checks position 0
                };

                if (match("bool") || match("double") || match("int8_t") || match("uint8_t") ||
                    match("int16_t") || match("uint16_t") || match("int32_t") ||
                    match("uint32_t") || match("int64_t") || match("uint64_t"))
                {
                    os << underType;
                    handled = true;
                }
            }
            if (!handled)
            {
                os << codeWord << underType;
            }
            os << listLineStop << std::endl;
        }
        if (info.flags & TypeId::ATTR_CONSTRUCT && info.accessor->HasSetter())
        {
            std::string value = info.initialValue->SerializeToString(info.checker);
            if (underType == "std::string" && value.empty())
            {
                value = "\"\"";
            }
            os << "    " << listLineStart << "Initial value: " << value << listLineStop
               << std::endl;
        }
        bool moreFlags{false};
        os << "    " << listLineStart << "Flags: ";
        if (info.flags & TypeId::ATTR_CONSTRUCT && info.accessor->HasSetter())
        {
            os << flagSpanStart << "construct" << flagSpanStop;
            moreFlags = true;
        }
        if (info.flags & TypeId::ATTR_SET && info.accessor->HasSetter())
        {
            os << (outputText && moreFlags ? ", " : "") << flagSpanStart << "write" << flagSpanStop;
            moreFlags = true;
        }
        if (info.flags & TypeId::ATTR_GET && info.accessor->HasGetter())
        {
            os << (outputText && moreFlags ? ", " : "") << flagSpanStart << "read" << flagSpanStop;
            moreFlags = true;
        }
        os << listLineStop << std::endl;
        os << indentHtmlOnly << listStop << std::endl;
    }
    os << listStop << std::endl;
} // PrintAttributesTid ()

/**
 * Print the Attributes block for tid,
 * including Attributes declared in base classes.
 *
 * All Attributes of this TypeId will be printed,
 * including those defined in parent classes.
 *
 * \param [in,out] os The output stream.
 * \param [in] tid The TypeId to print.
 */
void
PrintAttributes(std::ostream& os, const TypeId tid)
{
    NS_LOG_FUNCTION(tid);
    if (tid.GetAttributeN() == 0)
    {
        os << "No Attributes are defined for this type." << breakBoth << std::endl;
    }
    else
    {
        os << headingStart << "Attributes" << headingStop << std::endl;
        PrintAttributesTid(os, tid);
    }

    // Attributes from base classes
    TypeId tmp = tid.GetParent();
    while (tmp.GetParent() != tmp)
    {
        if (tmp.GetAttributeN() != 0)
        {
            os << headingStart << "Attributes defined in parent class " << tmp.GetName()
               << headingStop << std::endl;
            PrintAttributesTid(os, tmp);
        }
        tmp = tmp.GetParent();

    } // Attributes
} // PrintAttributes ()

/**
 * Print direct Trace sources for this TypeId.
 *
 * Only Trace sources defined directly by this TypeId will be printed.
 *
 * \param [in,out] os The output stream.
 * \param [in] tid The TypeId to print.
 */
void
PrintTraceSourcesTid(std::ostream& os, const TypeId tid)
{
    NS_LOG_FUNCTION(tid);

    auto index = SortedTraceSourceInfo(tid);

    os << listStart << std::endl;
    for (const auto& [name, info] : index)
    {
        os << listLineStart << boldStart << name << boldStop << ": " << info.help << breakBoth;
        if (!outputText)
        {
            //    '%' prevents doxygen from linking to the Callback class...
            os << "%";
        }
        os << "Callback signature: " << info.callback << std::endl;
        os << listLineStop << std::endl;
    }
    os << listStop << std::endl;
} // PrintTraceSourcesTid ()

/**
 * Print the Trace sources block for tid,
 * including Trace sources declared in base classes.
 *
 * All Trace sources of this TypeId will be printed,
 * including those defined in parent classes.
 *
 * \param [in,out] os The output stream.
 * \param [in] tid The TypeId to print.
 */
void
PrintTraceSources(std::ostream& os, const TypeId tid)
{
    NS_LOG_FUNCTION(tid);
    if (tid.GetTraceSourceN() == 0)
    {
        os << "No TraceSources are defined for this type." << breakBoth << std::endl;
    }
    else
    {
        os << headingStart << "TraceSources" << headingStop << std::endl;
        PrintTraceSourcesTid(os, tid);
    }

    // Trace sources from base classes
    TypeId tmp = tid.GetParent();
    while (tmp.GetParent() != tmp)
    {
        if (tmp.GetTraceSourceN() != 0)
        {
            os << headingStart << "TraceSources defined in parent class " << tmp.GetName()
               << headingStop << std::endl;
            PrintTraceSourcesTid(os, tmp);
        }
        tmp = tmp.GetParent();
    }

} // PrintTraceSources ()

/**
 * Print the size of the type represented by this tid.
 *
 * \param [in,out] os The output stream.
 * \param [in] tid The TypeId to print.
 */
void
PrintSize(std::ostream& os, const TypeId tid)
{
    NS_LOG_FUNCTION(tid);
    NS_ASSERT_MSG(CHAR_BIT != 0, "CHAR_BIT is zero");

    std::size_t arch = (sizeof(void*) * CHAR_BIT);

    os << boldStart << "Size" << boldStop << " of this type is " << tid.GetSize() << " bytes (on a "
       << arch << "-bit architecture)." << std::endl;
} // PrintSize ()

/**
 * Print the doxy block for each TypeId
 *
 * \param [in,out] os The output stream.
 */
void
PrintTypeIdBlocks(std::ostream& os)
{
    NS_LOG_FUNCTION_NOARGS();

    NameMap nameMap = GetNameMap();

    // Iterate over the map, which will print the class names in
    // alphabetical order.
    for (const auto& item : nameMap)
    {
        // Handle only real TypeIds
        if (item.second < 0)
        {
            continue;
        }
        // Get the class's index out of the map;
        TypeId tid = TypeId::GetRegistered(item.second);
        std::string name = tid.GetName();

        std::cout << commentStart << std::endl;

        std::cout << classStart << name << std::endl;
        std::cout << std::endl;

        PrintConfigPaths(std::cout, tid);
        PrintAttributes(std::cout, tid);
        PrintTraceSources(std::cout, tid);
        PrintSize(std::cout, tid);

        std::cout << commentStop << std::endl;
    } // for class documentation

} // PrintTypeIdBlocks

/***************************************************************
 *        Lists of All things
 ***************************************************************/

/**
 * Print the list of all TypeIds
 *
 * \param [in,out] os The output stream.
 */
void
PrintAllTypeIds(std::ostream& os)
{
    NS_LOG_FUNCTION_NOARGS();
    os << commentStart << page << "TypeIdList All ns3::TypeId's\n" << std::endl;
    os << "This is a list of all" << reference << "ns3::TypeId's.\n"
       << "For more information see the" << reference << "ns3::TypeId "
       << "section of this API documentation and the" << referenceNo << "TypeId section "
       << "in the Configuration and " << referenceNo << "Attributes chapter of the Manual.\n"
       << std::endl;

    os << listStart << std::endl;

    NameMap nameMap = GetNameMap();
    // Iterate over the map, which will print the class names in
    // alphabetical order.
    for (const auto& item : nameMap)
    {
        // Handle only real TypeIds
        if (item.second < 0)
        {
            continue;
        }
        // Get the class's index out of the map;
        TypeId tid = TypeId::GetRegistered(item.second);

        os << indentHtmlOnly << listLineStart << boldStart << tid.GetName() << boldStop
           << listLineStop << std::endl;
    }
    os << listStop << std::endl;
    os << commentStop << std::endl;

} // PrintAllTypeIds ()

/**
 * Print the list of all Attributes.
 *
 * \param [in,out] os The output stream.
 *
 * \todo Print this sorted by class (the current version)
 * as well as by Attribute name.
 */
void
PrintAllAttributes(std::ostream& os)
{
    NS_LOG_FUNCTION_NOARGS();
    os << commentStart << page << "AttributeList All Attributes\n" << std::endl;
    os << "This is a list of all" << reference << "attributes classes.  "
       << "For more information see the" << reference << "attributes "
       << "section of this API documentation and the Attributes sections "
       << "in the Tutorial and Manual.\n"
       << std::endl;

    NameMap nameMap = GetNameMap();
    // Iterate over the map, which will print the class names in
    // alphabetical order.
    for (const auto& item : nameMap)
    {
        // Handle only real TypeIds
        if (item.second < 0)
        {
            continue;
        }
        // Get the class's index out of the map;
        TypeId tid = TypeId::GetRegistered(item.second);

        if (tid.GetAttributeN() == 0)
        {
            continue;
        }

        auto index = SortedAttributeInfo(tid);

        os << boldStart << tid.GetName() << boldStop << breakHtmlOnly << std::endl;
        os << listStart << std::endl;
        for (const auto& [name, info] : index)
        {
            os << listLineStart << boldStart << name << boldStop << ": " << info.help
               << listLineStop << std::endl;
        }
        os << listStop << std::endl;
    }
    os << commentStop << std::endl;

} // PrintAllAttributes ()

/**
 * Print the list of all global variables.
 *
 * \param [in,out] os The output stream.
 */
void
PrintAllGlobals(std::ostream& os)
{
    NS_LOG_FUNCTION_NOARGS();
    os << commentStart << page << "GlobalValueList All GlobalValues\n" << std::endl;
    os << "This is a list of all" << reference << "ns3::GlobalValue instances.\n"
       << "See ns3::GlobalValue for how to set these." << std::endl;

    os << listStart << std::endl;
    for (GlobalValue::Iterator i = GlobalValue::Begin(); i != GlobalValue::End(); ++i)
    {
        StringValue val;
        (*i)->GetValue(val);
        os << indentHtmlOnly << listLineStart << boldStart << hrefStart << (*i)->GetName()
           << hrefMid << "GlobalValue" << (*i)->GetName() << hrefStop << boldStop << ": "
           << (*i)->GetHelp() << ".  Default value: " << val.Get() << "." << listLineStop
           << std::endl;
    }
    os << listStop << std::endl;
    os << commentStop << std::endl;

} // PrintAllGlobals ()

/**
 * Print the list of all LogComponents.
 *
 * \param [in,out] os The output stream.
 */
void
PrintAllLogComponents(std::ostream& os)
{
    NS_LOG_FUNCTION_NOARGS();
    os << commentStart << page << "LogComponentList All LogComponents\n" << std::endl;
    os << "This is a list of all" << reference << "ns3::LogComponent instances.\n" << std::endl;

    /**
     * \todo Switch to a border-less table, so the file links align
     * See https://www.doxygen.nl/manual/htmlcmds.html
     */
    LogComponent::ComponentList* logs = LogComponent::GetComponentList();
    // Find longest log name
    std::size_t widthL = std::string("Log Component").size();
    std::size_t widthR = std::string("file").size();
    for (const auto& it : (*logs))
    {
        widthL = std::max(widthL, it.first.size());
        std::string file = it.second->File();
        // Strip leading "../" related to depth in build directory
        // since doxygen only sees the path starting with "src/", etc.
        while (file.find("../") == 0)
        {
            file = file.substr(3);
        }
        widthR = std::max(widthR, file.size());
    }
    const std::string tLeft("| ");
    const std::string tMid(" | ");
    const std::string tRight(" |");

    // Header line has to be padded to same length as separator line
    os << tLeft << std::setw(widthL) << std::left << "Log Component" << tMid << std::setw(widthR)
       << std::left << "File" << tRight << std::endl;
    os << tLeft << ":" << std::string(widthL - 1, '-') << tMid << ":"
       << std::string(widthR - 1, '-') << tRight << std::endl;

    LogComponent::ComponentList::const_iterator it;
    for (const auto& it : (*logs))
    {
        std::string file = it.second->File();
        // Strip leading "../" related to depth in build directory
        // since doxygen only sees the path starting with "src/", etc.
        while (file.find("../") == 0)
        {
            file = file.substr(3);
        }

        os << tLeft << std::setw(widthL) << std::left << it.first << tMid << std::setw(widthR)
           << file << tRight << std::endl;
    }
    os << std::right << std::endl;
    os << commentStop << std::endl;
} // PrintAllLogComponents ()

/**
 * Print the list of all Trace sources.
 *
 * \param [in,out] os The output stream.
 *
 * \todo Print this sorted by class (the current version)
 * as well as by TraceSource name.
 */
void
PrintAllTraceSources(std::ostream& os)
{
    NS_LOG_FUNCTION_NOARGS();
    os << commentStart << page << "TraceSourceList All TraceSources\n" << std::endl;
    os << "This is a list of all" << reference << "tracing sources.  "
       << "For more information see the " << reference << "tracing "
       << "section of this API documentation and the Tracing sections "
       << "in the Tutorial and Manual.\n"
       << std::endl;

    NameMap nameMap = GetNameMap();

    // Iterate over the map, which will print the class names in
    // alphabetical order.
    for (const auto& item : nameMap)
    {
        // Handle only real TypeIds
        if (item.second < 0)
        {
            continue;
        }
        // Get the class's index out of the map;
        TypeId tid = TypeId::GetRegistered(item.second);

        if (tid.GetTraceSourceN() == 0)
        {
            continue;
        }

        auto index = SortedTraceSourceInfo(tid);

        os << boldStart << tid.GetName() << boldStop << breakHtmlOnly << std::endl;

        os << listStart << std::endl;
        for (const auto& [name, info] : index)
        {
            os << listLineStart << boldStart << name << boldStop << ": " << info.help
               << listLineStop << std::endl;
        }
        os << listStop << std::endl;
    }
    os << commentStop << std::endl;

} // PrintAllTraceSources ()

/***************************************************************
 *        Docs for Attribute classes
 ***************************************************************/

/**
 * Print the section definition for an AttributeValue.
 *
 * In doxygen form this will print a comment block with
 * \verbatim
 *   \ingroup attributes
 *   \defgroup attribute_<name>Value <name>Value
 * \endverbatim
 *
 * \param [in,out] os The output stream.
 * \param [in] name The base name of the resulting AttributeValue type.
 * \param [in] seeBase Print a "see also" pointing to the base class.
 */
void
PrintAttributeValueSection(std::ostream& os, const std::string& name, const bool seeBase = true)
{
    NS_LOG_FUNCTION(name);
    std::string section = "attribute_" + name;

    // \ingroup attributes
    // \defgroup attribute_<name>Value <name> Attribute
    os << commentStart << sectionStart << "attributes\n"
       << subSectionStart << "attribute_" << name << " " << name << " Attribute\n"
       << "AttributeValue implementation for " << name << "\n";
    if (seeBase)
    {
        // Some classes don't live in ns3::.  Yuck
        if (name != "IeMeshId")
        {
            os << seeAlso << "ns3::" << name << "\n";
        }
        else
        {
            os << seeAlso << "ns3::dot11s::" << name << "\n";
        }
    }
    os << commentStop;

} // PrintAttributeValueSection ()

/**
 * Print the AttributeValue documentation for a class.
 *
 * This will print documentation for the \pname{AttributeValue} class and methods.
 *
 * \param [in,out] os The output stream.
 * \param [in] name The token to use in defining the accessor name.
 * \param [in] type The underlying type name.
 * \param [in] header The header file which contains this declaration.
 */
void
PrintAttributeValueWithName(std::ostream& os,
                            const std::string& name,
                            const std::string& type,
                            const std::string& header)
{
    NS_LOG_FUNCTION(name << type << header);
    std::string sectAttr = sectionStart + "attribute_" + name;

    // \ingroup attribute_<name>Value
    // \class ns3::<name>Value "header"
    std::string valClass = name + "Value";
    std::string qualClass = " ns3::" + valClass;

    os << commentStart << sectAttr << std::endl;
    os << classStart << qualClass << " \"" << header << "\"" << std::endl;
    os << "AttributeValue implementation for " << name << "." << std::endl;
    os << seeAlso << "AttributeValue" << std::endl;
    os << commentStop;

    // Copy ctor: <name>Value::<name>Value
    os << commentStart << functionStart << name << qualClass << "::" << valClass;
    if ((name == "EmptyAttribute") || (name == "ObjectPtrContainer"))
    {
        // Just default constructors.
        os << "()\n";
    }
    else
    {
        // Copy constructors
        os << "(const " << type << " & value)\n"
           << "Copy constructor.\n"
           << argument << "[in] value The " << name << " value to copy.\n";
    }
    os << commentStop;

    // <name>Value::Get () const
    os << commentStart << functionStart << type << qualClass << "::Get () const\n"
       << returns << "The " << name << " value.\n"
       << commentStop;

    // <name>Value::GetAccessor (T & value) const
    os << commentStart << functionStart << "bool" << qualClass
       << "::GetAccessor (T & value) const\n"
       << "Access the " << name << " value as type " << codeWord << "T.\n"
       << templateArgument << "T " << templArgExplicit << "The type to cast to.\n"
       << argument << "[out] value The " << name << " value, as type " << codeWord << "T.\n"
       << returns << "true.\n"
       << commentStop;

    // <name>Value::Set (const name & value)
    if (type != "Callback") // Yuck
    {
        os << commentStart << functionStart << "void" << qualClass << "::Set (const " << type
           << " & value)\n"
           << "Set the value.\n"
           << argument << "[in] value The value to adopt.\n"
           << commentStop;
    }

    // <name>Value::m_value
    os << commentStart << variable << type << qualClass << "::m_value\n"
       << "The stored " << name << " instance.\n"
       << commentStop << std::endl;

} // PrintAttributeValueWithName ()

/**
 * Print the AttributeValue MakeAccessor documentation for a class.
 *
 * This will print documentation for the \pname{Make<name>Accessor} functions.
 *
 * \param [in,out] os The output stream.
 * \param [in] name The token to use in defining the accessor name.
 */
void
PrintMakeAccessors(std::ostream& os, const std::string& name)
{
    NS_LOG_FUNCTION(name);
    std::string sectAttr = sectionStart + "attribute_" + name + "\n";
    std::string make = "ns3::Make" + name + "Accessor ";

    // \ingroup attribute_<name>Value
    // Make<name>Accessor (T1 a1)
    os << commentStart << sectAttr << functionStart << "ns3::Ptr<const ns3::AttributeAccessor> "
       << make << "(T1 a1)\n"
       << copyDoc << "ns3::MakeAccessorHelper(T1)\n"
       << seeAlso << "AttributeAccessor\n"
       << commentStop;

    // \ingroup attribute_<name>Value
    // Make<name>Accessor (T1 a1)
    os << commentStart << sectAttr << functionStart << "ns3::Ptr<const ns3::AttributeAccessor> "
       << make << "(T1 a1, T2 a2)\n"
       << copyDoc << "ns3::MakeAccessorHelper(T1,T2)\n"
       << seeAlso << "AttributeAccessor\n"
       << commentStop;
} // PrintMakeAccessors ()

/**
 * Print the AttributeValue MakeChecker documentation for a class.
 *
 * This will print documentation for the \pname{Make<name>Checker} function.
 *
 * \param [in,out] os The output stream.
 * \param [in] name The token to use in defining the accessor name.
 * \param [in] header The header file which contains this declaration.
 */
void
PrintMakeChecker(std::ostream& os, const std::string& name, const std::string& header)
{
    NS_LOG_FUNCTION(name << header);
    std::string sectAttr = sectionStart + "attribute_" + name + "\n";
    std::string make = "ns3::Make" + name + "Checker ";

    // \ingroup attribute_<name>Value
    // class <name>Checker
    os << commentStart << sectAttr << std::endl;
    os << classStart << " ns3::" << name << "Checker"
       << " \"" << header << "\"" << std::endl;
    os << "AttributeChecker implementation for " << name << "Value." << std::endl;
    os << seeAlso << "AttributeChecker" << std::endl;
    os << commentStop;

    // \ingroup attribute_<name>Value
    // Make<name>Checker ()
    os << commentStart << sectAttr << functionStart << "ns3::Ptr<const ns3::AttributeChecker> "
       << make << "()\n"
       << returns << "The AttributeChecker.\n"
       << seeAlso << "AttributeChecker\n"
       << commentStop;
} // PrintMakeChecker ()

/**Descriptor for an AttributeValue. */
struct AttributeDescriptor
{
    const std::string m_name;   //!< The base name of the resulting AttributeValue type.
    const std::string m_type;   //!< The name of the underlying type.
    const bool m_seeBase;       //!< Print a "see also" pointing to the base class.
    const std::string m_header; //!< The header file name.
};

/**
 * Print documentation corresponding to use of the
 * ATTRIBUTE_HELPER_HEADER macro or
 * ATTRIBUTE_VALUE_DEFINE_WITH_NAME macro.
 *
 * \param [in,out] os The output stream.
 * \param [in] attr The AttributeDescriptor.
 */
void
PrintAttributeHelper(std::ostream& os, const AttributeDescriptor& attr)
{
    NS_LOG_FUNCTION(attr.m_name << attr.m_type << attr.m_seeBase << attr.m_header);
    PrintAttributeValueSection(os, attr.m_name, attr.m_seeBase);
    PrintAttributeValueWithName(os, attr.m_name, attr.m_type, attr.m_header);
    PrintMakeAccessors(os, attr.m_name);
    PrintMakeChecker(os, attr.m_name, attr.m_header);
} // PrintAttributeHelper ()

/**
 * Print documentation for Attribute implementations.
 * \param os The stream to print on.
 */
void
PrintAttributeImplementations(std::ostream& os)
{
    NS_LOG_FUNCTION_NOARGS();

    // clang-format off
  const AttributeDescriptor attributes [] =
    {
      // Name             Type             see Base  header-file
      // Users of ATTRIBUTE_HELPER_HEADER
      //
      { "Address",        "Address",        true,  "address.h"          },
      { "Box",            "Box",            true,  "box.h"              },
      { "DataRate",       "DataRate",       true,  "data-rate.h"        },
      { "Length",         "Length",         true,  "length.h"           },
      { "IeMeshId",       "IeMeshId",       true,  "ie-dot11s-id.h"     },
      { "Ipv4Address",    "Ipv4Address",    true,  "ipv4-address.h"     },
      { "Ipv4Mask",       "Ipv4Mask",       true,  "ipv4-address.h"     },
      { "Ipv6Address",    "Ipv6Address",    true,  "ipv6-address.h"     },
      { "Ipv6Prefix",     "Ipv6Prefix",     true,  "ipv6-address.h"     },
      { "Mac16Address",   "Mac16Address",   true,  "mac16-address.h"    },
      { "Mac48Address",   "Mac48Address",   true,  "mac48-address.h"    },
      { "Mac64Address",   "Mac64Address",   true,  "mac64-address.h"    },
      { "ObjectFactory",  "ObjectFactory",  true,  "object-factory.h"   },
      { "OrganizationIdentifier",
                          "OrganizationIdentifier",
                                            true,  "vendor-specific-action.h" },
      { "Priomap",        "Priomap",        true,  "prio-queue-disc.h"  },
      { "QueueSize",      "QueueSize",      true,  "queue-size.h"       },
      { "Rectangle",      "Rectangle",      true,  "rectangle.h"        },
      { "Ssid",           "Ssid",           true,  "ssid.h"             },
      { "TypeId",         "TypeId",         true,  "type-id.h"          },
      { "UanModesList",   "UanModesList",   true,  "uan-tx-mode.h"      },
      { "ValueClassTest", "ValueClassTest", false, "attribute-test-suite.cc" /* core/test/ */  },
      { "Vector",         "Vector",         true,  "vector.h"           },
      { "Vector2D",       "Vector2D",       true,  "vector.h"           },
      { "Vector3D",       "Vector3D",       true,  "vector.h"           },
      { "Waypoint",       "Waypoint",       true,  "waypoint.h"         },
      { "WifiMode",       "WifiMode",       true,  "wifi-mode.h"        },

      // All three (Value, Access and Checkers) defined, but custom
      { "Boolean",        "bool",           false, "boolean.h"          },
      { "Callback",       "Callback",       true,  "callback.h"         },
      { "Double",         "double",         false, "double.h"           },
      { "Enum",           "int",            false, "enum.h"             },
      { "Integer",        "int64_t",        false, "integer.h"          },
      { "Pointer",        "Pointer",        false, "pointer.h"          },
      { "String",         "std::string",    false, "string.h"           },
      { "Time",           "Time",           true,  "nstime.h"           },
      { "Uinteger",       "uint64_t",       false, "uinteger.h"         },
      { "",               "",               false, "last placeholder"   }
    };
    // clang-format on

    int i = 0;
    while (!attributes[i].m_name.empty())
    {
        PrintAttributeHelper(os, attributes[i]);
        ++i;
    }

    // Special cases
    PrintAttributeValueSection(os, "EmptyAttribute", false);
    PrintAttributeValueWithName(os, "EmptyAttribute", "EmptyAttribute", "attribute.h");

    PrintAttributeValueSection(os, "ObjectPtrContainer", false);
    PrintAttributeValueWithName(os,
                                "ObjectPtrContainer",
                                "ObjectPtrContainer",
                                "object-ptr-container.h");
    PrintMakeChecker(os, "ObjectPtrContainer", "object-ptr-container.h");

    PrintAttributeValueSection(os, "ObjectVector", false);
    PrintMakeAccessors(os, "ObjectVector");
    PrintMakeChecker(os, "ObjectVector", "object-vector.h");

    PrintAttributeValueSection(os, "ObjectMap", false);
    PrintMakeAccessors(os, "ObjectMap");
    PrintMakeChecker(os, "ObjectMap", "object-map.h");

    PrintAttributeValueSection(os, "Pair", false);
    PrintAttributeValueWithName(os, "Pair", "std::pair<A, B>", "pair.h");
    PrintMakeChecker(os, "Pair", "pair.h");

    PrintAttributeValueSection(os, "Tuple", false);
    PrintAttributeValueWithName(os, "Tuple", "std::tuple<Args...>", "tuple.h");
    PrintMakeChecker(os, "Tuple", "tuple.h");

    // AttributeContainer is already documented.
    // PrintAttributeValueSection  (os, "AttributeContainer", false);
    // PrintAttributeValueWithName (os, "AttributeContainer", "AttributeContainer",
    // "attribute-container.h");
    PrintMakeChecker(os, "AttributeContainer", "attribute-container.h");
} // PrintAttributeImplementations ()

/***************************************************************
 *        Main
 ***************************************************************/

int
main(int argc, char* argv[])
{
    NS_LOG_FUNCTION_NOARGS();

    CommandLine cmd(__FILE__);
    cmd.Usage("Generate documentation for all ns-3 registered types, "
              "trace sources, attributes and global variables.");
    cmd.AddValue("output-text", "format output as plain text", outputText);
    cmd.Parse(argc, argv);

    SetMarkup();

    // Create a Node, to force linking and instantiation of our TypeIds
    NodeContainer c;
    c.Create(1);

    std::cout << std::endl;
    std::cout << commentStart << file << "\n"
              << sectionStart << "utils\n"
              << "Doxygen docs generated from the TypeId database.\n"
              << note << "This file is automatically generated by " << codeWord
              << "print-introspected-doxygen.cc. Do not edit this file! "
              << "Edit that file instead.\n"
              << commentStop << std::endl;

    PrintTypeIdBlocks(std::cout);

    PrintAllTypeIds(std::cout);
    PrintAllAttributes(std::cout);
    PrintAllGlobals(std::cout);
    PrintAllLogComponents(std::cout);
    PrintAllTraceSources(std::cout);
    PrintAttributeImplementations(std::cout);

    return 0;
}
