/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@cutebugs.net>
 */

#include "raw-text-config.h"
#include "attribute-iterator.h"
#include "attribute-default-iterator.h"
#include "ns3/global-value.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/config.h"

#include <sstream>
#include <algorithm>
#include <functional>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RawTextConfig");

RawTextConfigSave::RawTextConfigSave ()
  : m_os (0)
{
  NS_LOG_FUNCTION (this);
}
RawTextConfigSave::~RawTextConfigSave ()
{
  NS_LOG_FUNCTION (this);
  if (m_os != 0)
    {
      m_os->close ();
    }
  delete m_os;
  m_os = 0;
}
void
RawTextConfigSave::SetFilename (std::string filename)
{
  NS_LOG_FUNCTION (this << filename);
  m_os = new std::ofstream ();
  m_os->open (filename.c_str (), std::ios::out);
}
void
RawTextConfigSave::Default (void)
{
  NS_LOG_FUNCTION (this);
  class RawTextDefaultIterator : public AttributeDefaultIterator
  {
public:
    RawTextDefaultIterator (std::ostream *os) {
      m_os = os;
    }
private:
    virtual void StartVisitTypeId (std::string name) {
      m_typeId = name;
    }
    virtual void DoVisitAttribute (std::string name, std::string defaultValue) {
      NS_LOG_DEBUG ("Saving " << m_typeId << "::" << name);
      *m_os << "default " << m_typeId << "::" << name << " \"" << defaultValue << "\"" << std::endl;
    }
    std::string m_typeId;
    std::ostream *m_os;
  };

  RawTextDefaultIterator iterator = RawTextDefaultIterator (m_os);
  iterator.Iterate ();
}
void
RawTextConfigSave::Global (void)
{
  NS_LOG_FUNCTION (this);
  for (GlobalValue::Iterator i = GlobalValue::Begin (); i != GlobalValue::End (); ++i)
    {
      StringValue value;
      (*i)->GetValue (value);
      NS_LOG_LOGIC ("Saving " << (*i)->GetName ());
      *m_os << "global " << (*i)->GetName () << " \"" << value.Get () << "\"" << std::endl;
    }
}
void
RawTextConfigSave::Attributes (void)
{
  NS_LOG_FUNCTION (this);
  class RawTextAttributeIterator : public AttributeIterator
  {
public:
    RawTextAttributeIterator (std::ostream *os)
      : m_os (os) {}
private:
    virtual void DoVisitAttribute (Ptr<Object> object, std::string name) {
      StringValue str;
      object->GetAttribute (name, str);
      NS_LOG_DEBUG ("Saving " << GetCurrentPath ());
      *m_os << "value " << GetCurrentPath () << " \"" << str.Get () << "\"" << std::endl;
    }
    std::ostream *m_os;
  };

  RawTextAttributeIterator iter = RawTextAttributeIterator (m_os);
  iter.Iterate ();
}

RawTextConfigLoad::RawTextConfigLoad ()
  : m_is (0)
{
  NS_LOG_FUNCTION (this);
}
RawTextConfigLoad::~RawTextConfigLoad ()
{
  NS_LOG_FUNCTION (this);
  if (m_is != 0)
    {
      m_is->close ();
      delete m_is;
      m_is = 0;
    }
}
void
RawTextConfigLoad::SetFilename (std::string filename)
{
  NS_LOG_FUNCTION (this << filename);
  m_is = new std::ifstream ();
  m_is->open (filename.c_str (), std::ios::in);
}
std::string
RawTextConfigLoad::Strip (std::string value)
{
  std::string::size_type start = value.find ("\"");
  NS_ASSERT_MSG (start != std::string::npos, "Ill-formed attribute value: " << value);

  std::string::size_type end = value.find ("\"", start + 1);

  NS_ASSERT_MSG (end != std::string::npos, "Ill-formed attribute value: " << value);
  return value.substr (start+1, end-start-1);
}

void
RawTextConfigLoad::Default (void)
{
  NS_LOG_FUNCTION (this);
  m_is->clear ();
  m_is->seekg (0);
  std::string type, name, value;
  for (std::string line; std::getline (*m_is, line);)
    {
      if (!ParseLine (line, type, name, value)) continue; // comment

      NS_LOG_DEBUG ("type=" << type << ", name=" << name << ", value=" << value);
      value = Strip (value);
      if (type == "default")
        Config::SetDefault (name, StringValue (value));
      name.clear ();
      type.clear ();
    }
}
void
RawTextConfigLoad::Global (void)
{
  NS_LOG_FUNCTION (this);
  m_is->clear ();
  m_is->seekg (0);
  std::string type, name, value;
  for (std::string line; std::getline (*m_is, line);)
    {
      if (!ParseLine (line, type, name, value)) continue; // comment

      NS_LOG_DEBUG ("type=" << type << ", name=" << name << ", value=" << value);
      value = Strip (value);
      if (type == "global")
        Config::SetGlobal (name, StringValue (value));
      name.clear ();
      type.clear ();
    }
}
void
RawTextConfigLoad::Attributes (void)
{
  NS_LOG_FUNCTION (this);
  m_is->clear ();
  m_is->seekg (0);
  std::string type, name, value;
  for (std::string line; std::getline (*m_is, line);)
    {
      if (!ParseLine (line, type, name, value)) continue;

      NS_LOG_DEBUG ("type=" << type << ", name=" << name << ", value=" << value);
      value = Strip (value);
      if (type == "value")
        Config::Set (name, StringValue (value));
      name.clear ();
      type.clear ();
    }
}

bool
RawTextConfigLoad::ParseLine (const std::string &line, std::string &type, std::string &name, std::string &value)
{
  if (line.front () == '#')
    return false;     // comment line
  if (line.empty () || line == std::string (line.size (), ' '))
    return false;  // blank line

  // for multiline values, append line to value if type and name not empty
  if (type.empty () || name.empty ())
    {
      std::istringstream iss (line);
      std::ostringstream rem;
      iss >> type >> name >> rem.rdbuf ();
      value = rem.str ();   // remaining line, includes embedded spaces
    }
  else
    {
      value.append (line);
    }

  // look for two quotes
  return std::count (value.begin (), value.end (), '"') == 2;
}


} // namespace ns3
