/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019, WPL Inc.
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
 * Author: Jared Dulmage <jared.dulmage@wpli.net>
 */
#include "ns3/core-module.h"
#include "ns3/config-store-module.h"
#include "ns3/type-id.h"

#include <iostream>

using namespace ns3;

/**
 * \ingroup configstore-examples
 * \ingroup examples
 *
 * \brief Example class to demonstrate use of the ns-3 Config Store
 */
class ConfigExample : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::ConfigExample")
      .SetParent<Object> ()
      .AddAttribute ("TestInt16", "help text",
                     IntegerValue (-2),
                     MakeIntegerAccessor (&ConfigExample::m_int16),
                     MakeIntegerChecker<int16_t> ())
      ;
      return tid;
    }
  int16_t m_int16; ///< value to configure
};

NS_OBJECT_ENSURE_REGISTERED (ConfigExample);

// Load a ConfigStore file previously saved with config-store-save example.
// Set the filename via --ns3::ConfigStore::Filename=<filename>.
// May edit text files to explore comments and blank lines, multi-line
// values.  
// Observe behavior with NS_LOG=RawTextConfigLoad.
// 
int main (int argc, char *argv[])
{
  std::string filename;

  CommandLine cmd;
  cmd.Usage ("Basic usage: ./waf --run \"config-store-load <filename>\"");
  cmd.AddNonOption ("filename", "Relative path to config-store input file", filename);
  cmd.Parse (argc, argv);
  
  if (filename.empty ())
    {
      std::ostringstream oss;
      cmd.PrintHelp (oss);
      NS_FATAL_ERROR ("Missing required filename argument" << std::endl << std::endl << oss.str ());
      return 1;
    }
  Config::SetDefault ("ns3::ConfigStore::Filename", StringValue (filename));

  Config::SetDefault ("ns3::ConfigExample::TestInt16", IntegerValue (-5));

  Ptr<ConfigExample> a_obj = CreateObject<ConfigExample> ();
  NS_ABORT_MSG_UNLESS (a_obj->m_int16 == -5, "Cannot set ConfigExample's integer attribute via Config::SetDefault");

  Ptr<ConfigExample> b_obj = CreateObject<ConfigExample> ();
  b_obj->SetAttribute ("TestInt16", IntegerValue (-3));
  IntegerValue iv;
  b_obj->GetAttribute ("TestInt16", iv);
  NS_ABORT_MSG_UNLESS (iv.Get () == -3, "Cannot set ConfigExample's integer attribute via SetAttribute");

  // These test objects are not rooted in any ns-3 configuration namespace.
  // This is usually done automatically for ns3 nodes and channels, but
  // we can establish a new root and anchor one of them there (note; we 
  // can't use two objects of the same type as roots).  Rooting one of these
  // is necessary for it to show up in the config namespace so that
  // ConfigureAttributes() will work below.
  Config::RegisterRootNamespaceObject (b_obj);
  
  std::string::size_type pos = filename.rfind ('.');
  NS_ABORT_MSG_IF (pos == filename.npos, "Could not find filename extension for " << filename);

  std::string ext = filename.substr (pos);
  // Input config store from XML format
  if (ext == ".xml")
    {
#ifndef HAVE_LIBXML2
      NS_FATAL_ERROR ("No built-in XML library support");
#else
      Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("Xml"));
      Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Load"));
      ConfigStore inputConfig;
      inputConfig.ConfigureDefaults ();
      inputConfig.ConfigureAttributes ();
#endif /* HAVE_LIBXML2 */
    }
  else if (ext == ".txt")
    {
      // Input config store from txt format
      Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
      Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Load"));
      ConfigStore inputConfig;
      inputConfig.ConfigureDefaults ();
      inputConfig.ConfigureAttributes ();
    }
  else
    {
      NS_FATAL_ERROR ("Unsupported extension " << ext << " of filename " << filename);
    }
    
  Simulator::Run ();

  Simulator::Destroy ();
}
