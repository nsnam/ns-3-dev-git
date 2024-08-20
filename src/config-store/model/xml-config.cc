/*
 * Copyright (c) 2009 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@cutebugs.net>
 */

#include "xml-config.h"

#include "attribute-default-iterator.h"
#include "attribute-iterator.h"

#include "ns3/config.h"
#include "ns3/fatal-error.h"
#include "ns3/global-value.h"
#include "ns3/log.h"
#include "ns3/string.h"

#include <libxml/encoding.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("XmlConfig");

XmlConfigSave::XmlConfigSave()
    : m_writer(nullptr)
{
    NS_LOG_FUNCTION(this);
}

void
XmlConfigSave::SetFilename(std::string filename)
{
    NS_LOG_FUNCTION(filename);
    if (filename.empty())
    {
        return;
    }
    int rc;

    /* Create a new XmlWriter for uri, with no compression. */
    m_writer = xmlNewTextWriterFilename(filename.c_str(), 0);
    if (m_writer == nullptr)
    {
        NS_FATAL_ERROR("Error creating the XML writer");
    }
    rc = xmlTextWriterSetIndent(m_writer, 1);
    if (rc < 0)
    {
        NS_FATAL_ERROR("Error at xmlTextWriterSetIndent");
    }
    /* Start the document with the XML default for the version,
     * encoding utf-8 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(m_writer, nullptr, "utf-8", nullptr);
    if (rc < 0)
    {
        NS_FATAL_ERROR("Error at xmlTextWriterStartDocument");
    }

    /* Start an element named "ns3". Since this is the first
     * element, this will be the root element of the document. */
    rc = xmlTextWriterStartElement(m_writer, BAD_CAST "ns3");
    if (rc < 0)
    {
        NS_FATAL_ERROR("Error at xmlTextWriterStartElement\n");
    }
}

XmlConfigSave::~XmlConfigSave()
{
    NS_LOG_FUNCTION(this);
    if (m_writer == nullptr)
    {
        return;
    }
    int rc;
    /* Here we could close the remaining elements using the
     * function xmlTextWriterEndElement, but since we do not want to
     * write any other elements, we simply call xmlTextWriterEndDocument,
     * which will do all the work. */
    rc = xmlTextWriterEndDocument(m_writer);
    if (rc < 0)
    {
        NS_FATAL_ERROR("Error at xmlTextWriterEndDocument\n");
    }

    xmlFreeTextWriter(m_writer);
    m_writer = nullptr;
}

void
XmlConfigSave::Default()
{
    class XmlDefaultIterator : public AttributeDefaultIterator
    {
      public:
        XmlDefaultIterator(xmlTextWriterPtr writer)
        {
            m_writer = writer;
        }

      private:
        void StartVisitTypeId(std::string name) override
        {
            m_typeid = name;
        }

        void DoVisitAttribute(std::string name, std::string defaultValue) override
        {
            TypeId tid = TypeId::LookupByName(m_typeid);
            ns3::TypeId::SupportLevel supportLevel = TypeId::SupportLevel::SUPPORTED;
            std::string originalInitialValue;
            std::string valueTypeName;
            for (std::size_t i = 0; i < tid.GetAttributeN(); i++)
            {
                TypeId::AttributeInformation tmp = tid.GetAttribute(i);
                if (tmp.name == name)
                {
                    supportLevel = tmp.supportLevel;
                    originalInitialValue = tmp.originalInitialValue->SerializeToString(tmp.checker);
                    valueTypeName = tmp.checker->GetValueTypeName();
                    break;
                }
            }
            if (valueTypeName == "ns3::CallbackValue")
            {
                NS_LOG_WARN("Global attribute " << m_typeid << "::" << name
                                                << " was not saved because it is a CallbackValue");
                return;
            }
            if (supportLevel == TypeId::SupportLevel::OBSOLETE)
            {
                NS_LOG_WARN("Global attribute " << m_typeid << "::" << name
                                                << " was not saved because it is OBSOLETE");
                return;
            }
            if (supportLevel == TypeId::SupportLevel::DEPRECATED &&
                defaultValue == originalInitialValue)
            {
                NS_LOG_WARN("Global attribute "
                            << m_typeid << "::" << name
                            << " was not saved because it is DEPRECATED and its value has not "
                               "changed from the original initial value");
                return;
            }

            int rc;
            rc = xmlTextWriterStartElement(m_writer, BAD_CAST "default");
            if (rc < 0)
            {
                NS_FATAL_ERROR("Error at xmlTextWriterStartElement");
            }
            std::string fullname = m_typeid + "::" + name;
            rc = xmlTextWriterWriteAttribute(m_writer, BAD_CAST "name", BAD_CAST fullname.c_str());
            if (rc < 0)
            {
                NS_FATAL_ERROR("Error at xmlTextWriterWriteAttribute");
            }
            rc = xmlTextWriterWriteAttribute(m_writer,
                                             BAD_CAST "value",
                                             BAD_CAST defaultValue.c_str());
            if (rc < 0)
            {
                NS_FATAL_ERROR("Error at xmlTextWriterWriteAttribute");
            }
            rc = xmlTextWriterEndElement(m_writer);
            if (rc < 0)
            {
                NS_FATAL_ERROR("Error at xmlTextWriterEndElement");
            }
        }

        xmlTextWriterPtr m_writer;
        std::string m_typeid;
    };

    XmlDefaultIterator iterator = XmlDefaultIterator(m_writer);
    iterator.Iterate();
}

void
XmlConfigSave::Attributes()
{
    class XmlTextAttributeIterator : public AttributeIterator
    {
      public:
        XmlTextAttributeIterator(xmlTextWriterPtr writer)
            : m_writer(writer)
        {
        }

      private:
        void DoVisitAttribute(Ptr<Object> object, std::string name) override
        {
            StringValue str;
            TypeId tid = object->GetInstanceTypeId();

            auto [found, inTid, attr] = TypeId::FindAttribute(tid, name);

            if (found)
            {
                if (attr.checker && attr.checker->GetValueTypeName() == "ns3::CallbackValue")
                {
                    NS_LOG_WARN("Attribute " << GetCurrentPath()
                                             << " was not saved because it is a CallbackValue");
                    return;
                }
                auto supportLevel = attr.supportLevel;
                if (supportLevel == TypeId::SupportLevel::OBSOLETE)
                {
                    NS_LOG_WARN("Attribute " << GetCurrentPath()
                                             << " was not saved because it is OBSOLETE");
                    return;
                }

                std::string originalInitialValue =
                    attr.originalInitialValue->SerializeToString(attr.checker);
                object->GetAttribute(name, str, true);

                if (supportLevel == TypeId::SupportLevel::DEPRECATED &&
                    str.Get() == originalInitialValue)
                {
                    NS_LOG_WARN("Attribute "
                                << GetCurrentPath()
                                << " was not saved because it is DEPRECATED and its value has not "
                                   "changed from the original initial value");
                    return;
                }
            }

            int rc;
            rc = xmlTextWriterStartElement(m_writer, BAD_CAST "value");
            if (rc < 0)
            {
                NS_FATAL_ERROR("Error at xmlTextWriterStartElement");
            }
            rc = xmlTextWriterWriteAttribute(m_writer,
                                             BAD_CAST "path",
                                             BAD_CAST GetCurrentPath().c_str());
            if (rc < 0)
            {
                NS_FATAL_ERROR("Error at xmlTextWriterWriteAttribute");
            }
            rc =
                xmlTextWriterWriteAttribute(m_writer, BAD_CAST "value", BAD_CAST str.Get().c_str());
            if (rc < 0)
            {
                NS_FATAL_ERROR("Error at xmlTextWriterWriteAttribute");
            }
            rc = xmlTextWriterEndElement(m_writer);
            if (rc < 0)
            {
                NS_FATAL_ERROR("Error at xmlTextWriterEndElement");
            }
        }

        xmlTextWriterPtr m_writer;
    };

    XmlTextAttributeIterator iter = XmlTextAttributeIterator(m_writer);
    iter.Iterate();
}

void
XmlConfigSave::Global()
{
    int rc;
    for (auto i = GlobalValue::Begin(); i != GlobalValue::End(); ++i)
    {
        StringValue value;
        (*i)->GetValue(value);

        rc = xmlTextWriterStartElement(m_writer, BAD_CAST "global");
        if (rc < 0)
        {
            NS_FATAL_ERROR("Error at xmlTextWriterStartElement");
        }
        rc =
            xmlTextWriterWriteAttribute(m_writer, BAD_CAST "name", BAD_CAST(*i)->GetName().c_str());
        if (rc < 0)
        {
            NS_FATAL_ERROR("Error at xmlTextWriterWriteAttribute");
        }
        rc = xmlTextWriterWriteAttribute(m_writer, BAD_CAST "value", BAD_CAST value.Get().c_str());
        if (rc < 0)
        {
            NS_FATAL_ERROR("Error at xmlTextWriterWriteAttribute");
        }
        rc = xmlTextWriterEndElement(m_writer);
        if (rc < 0)
        {
            NS_FATAL_ERROR("Error at xmlTextWriterEndElement");
        }
    }
}

XmlConfigLoad::XmlConfigLoad()
{
    NS_LOG_FUNCTION(this);
}

XmlConfigLoad::~XmlConfigLoad()
{
    NS_LOG_FUNCTION(this);
}

void
XmlConfigLoad::SetFilename(std::string filename)
{
    NS_LOG_FUNCTION(filename);
    m_filename = filename;
}

void
XmlConfigLoad::Default()
{
    xmlTextReaderPtr reader = xmlNewTextReaderFilename(m_filename.c_str());
    if (reader == nullptr)
    {
        NS_FATAL_ERROR("Error at xmlReaderForFile");
    }
    int rc;
    rc = xmlTextReaderRead(reader);
    while (rc > 0)
    {
        const xmlChar* type = xmlTextReaderConstName(reader);
        if (type == nullptr)
        {
            NS_FATAL_ERROR("Invalid value");
        }
        if (std::string((char*)type) == "default")
        {
            xmlChar* name = xmlTextReaderGetAttribute(reader, BAD_CAST "name");
            if (name == nullptr)
            {
                NS_FATAL_ERROR("Error getting attribute 'name'");
            }
            xmlChar* value = xmlTextReaderGetAttribute(reader, BAD_CAST "value");
            if (value == nullptr)
            {
                NS_FATAL_ERROR("Error getting attribute 'value'");
            }
            NS_LOG_DEBUG("default=" << (char*)name << ", value=" << value);
            Config::SetDefault((char*)name, StringValue((char*)value));
            xmlFree(name);
            xmlFree(value);
        }
        rc = xmlTextReaderRead(reader);
    }
    xmlFreeTextReader(reader);
}

void
XmlConfigLoad::Global()
{
    xmlTextReaderPtr reader = xmlNewTextReaderFilename(m_filename.c_str());
    if (reader == nullptr)
    {
        NS_FATAL_ERROR("Error at xmlReaderForFile");
    }
    int rc;
    rc = xmlTextReaderRead(reader);
    while (rc > 0)
    {
        const xmlChar* type = xmlTextReaderConstName(reader);
        if (type == nullptr)
        {
            NS_FATAL_ERROR("Invalid value");
        }
        if (std::string((char*)type) == "global")
        {
            xmlChar* name = xmlTextReaderGetAttribute(reader, BAD_CAST "name");
            if (name == nullptr)
            {
                NS_FATAL_ERROR("Error getting attribute 'name'");
            }
            xmlChar* value = xmlTextReaderGetAttribute(reader, BAD_CAST "value");
            if (value == nullptr)
            {
                NS_FATAL_ERROR("Error getting attribute 'value'");
            }
            NS_LOG_DEBUG("global=" << (char*)name << ", value=" << value);
            Config::SetGlobal((char*)name, StringValue((char*)value));
            xmlFree(name);
            xmlFree(value);
        }
        rc = xmlTextReaderRead(reader);
    }
    xmlFreeTextReader(reader);
}

void
XmlConfigLoad::Attributes()
{
    xmlTextReaderPtr reader = xmlNewTextReaderFilename(m_filename.c_str());
    if (reader == nullptr)
    {
        NS_FATAL_ERROR("Error at xmlReaderForFile");
    }
    int rc;
    rc = xmlTextReaderRead(reader);
    while (rc > 0)
    {
        const xmlChar* type = xmlTextReaderConstName(reader);
        if (type == nullptr)
        {
            NS_FATAL_ERROR("Invalid value");
        }
        if (std::string((char*)type) == "value")
        {
            xmlChar* path = xmlTextReaderGetAttribute(reader, BAD_CAST "path");
            if (path == nullptr)
            {
                NS_FATAL_ERROR("Error getting attribute 'path'");
            }
            xmlChar* value = xmlTextReaderGetAttribute(reader, BAD_CAST "value");
            if (value == nullptr)
            {
                NS_FATAL_ERROR("Error getting attribute 'value'");
            }
            NS_LOG_DEBUG("path=" << (char*)path << ", value=" << (char*)value);
            Config::Set((char*)path, StringValue((char*)value));
            xmlFree(path);
            xmlFree(value);
        }
        rc = xmlTextReaderRead(reader);
    }
    xmlFreeTextReader(reader);
}

} // namespace ns3
