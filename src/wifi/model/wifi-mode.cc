/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include <cmath>
#include "ns3/log.h"
#include "wifi-mode.h"
#include "wifi-tx-vector.h"
#include "he-ru.h"

namespace ns3 {

bool operator == (const WifiMode &a, const WifiMode &b)
{
  return a.GetUid () == b.GetUid ();
}

bool operator < (const WifiMode &a, const WifiMode &b)
{
  return a.GetUid () < b.GetUid ();
}

std::ostream & operator << (std::ostream & os, const WifiMode &mode)
{
  os << mode.GetUniqueName ();
  return os;
}

std::istream & operator >> (std::istream &is, WifiMode &mode)
{
  std::string str;
  is >> str;
  mode = WifiModeFactory::GetFactory ()->Search (str);
  return is;
}

bool
WifiMode::IsAllowed (uint16_t channelWidth, uint8_t nss) const
{
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  return item->IsModeAllowedCallback (channelWidth, nss);
}

uint64_t
WifiMode::GetPhyRate (uint16_t channelWidth, uint16_t guardInterval, uint8_t nss) const
{
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  return item->GetPhyRateCallback (channelWidth, guardInterval, nss);
}

uint64_t
WifiMode::GetPhyRate (WifiTxVector txVector) const
{
  return GetPhyRate (txVector.GetChannelWidth (), txVector.GetGuardInterval (), txVector.GetNss ());
}

uint64_t
WifiMode::GetDataRate (uint16_t channelWidth) const
{
  return GetDataRate (channelWidth, 800, 1);
}

uint64_t
WifiMode::GetDataRate (WifiTxVector txVector, uint16_t staId) const
{
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  return item->GetDataRateFromTxVectorCallback (txVector, staId);
}

uint64_t
WifiMode::GetDataRate (uint16_t channelWidth, uint16_t guardInterval, uint8_t nss) const
{
  NS_ASSERT (nss <= 8);
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  return item->GetDataRateCallback (channelWidth, guardInterval, nss);
}

WifiCodeRate
WifiMode::GetCodeRate (void) const
{
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  return item->GetCodeRateCallback ();
}

uint16_t
WifiMode::GetConstellationSize (void) const
{
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  return item->GetConstellationSizeCallback ();
}

std::string
WifiMode::GetUniqueName (void) const
{
  //needed for ostream printing of the invalid mode
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  return item->uniqueUid;
}

bool
WifiMode::IsMandatory (void) const
{
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  return item->isMandatory;
}

uint8_t
WifiMode::GetMcsValue (void) const
{
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  if (item->modClass >= WIFI_MOD_CLASS_HT)
    {
      return item->mcsValue;
    }
  else
    {
      //We should not go here!
      NS_ASSERT (false);
      return 0;
    }
}

uint32_t
WifiMode::GetUid (void) const
{
  return m_uid;
}

WifiModulationClass
WifiMode::GetModulationClass () const
{
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  return item->modClass;
}

uint64_t
WifiMode::GetNonHtReferenceRate (void) const
{
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  NS_ASSERT_MSG (!item->GetNonHtReferenceRateCallback.IsNull (), "Trying to get HT reference rate for a non-HT rate");
  return item->GetNonHtReferenceRateCallback ();
}

bool
WifiMode::IsHigherCodeRate (WifiMode mode) const
{
  WifiCodeRate codeRate = mode.GetCodeRate ();
  switch (GetCodeRate ())
    {
    case WIFI_CODE_RATE_1_2:
      return false; //This is the smallest code rate.
    case WIFI_CODE_RATE_2_3:
      return (codeRate == WIFI_CODE_RATE_1_2);
    case WIFI_CODE_RATE_3_4:
      return (codeRate == WIFI_CODE_RATE_1_2 || codeRate == WIFI_CODE_RATE_2_3);
    case WIFI_CODE_RATE_5_6:
      return (codeRate == WIFI_CODE_RATE_1_2 || codeRate == WIFI_CODE_RATE_2_3 || codeRate == WIFI_CODE_RATE_3_4);
    default:
      NS_FATAL_ERROR ("Wifi Code Rate not defined");
      return false;
    }
}

bool
WifiMode::IsHigherDataRate (WifiMode mode) const
{
  WifiModeFactory::WifiModeItem *item = WifiModeFactory::GetFactory ()->Get (m_uid);
  switch (item->modClass)
    {
    case WIFI_MOD_CLASS_DSSS:
      if (mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS)
        {
          return (GetConstellationSize () > mode.GetConstellationSize ());
        }
      else
        {
          return false;
        }
    case WIFI_MOD_CLASS_HR_DSSS:
      if (mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS)
        {
          return true;
        }
      else
        {
          return (GetConstellationSize () > mode.GetConstellationSize ());
        }
    case WIFI_MOD_CLASS_ERP_OFDM:
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
    case WIFI_MOD_CLASS_HE:
      if (mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS)
        {
          return true;
        }
      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS)
        {
          return (mode.GetConstellationSize () > GetConstellationSize ());
        }
      else
        {
          if (GetConstellationSize () > mode.GetConstellationSize ())
            {
              return true;
            }
          else if (GetConstellationSize () == mode.GetConstellationSize ())
            {
              return IsHigherCodeRate (mode);
            }
          else
            {
              return false;
            }
        }
    default:
      NS_FATAL_ERROR ("Modulation class not defined");
      return false;
    }
}

WifiMode::WifiMode ()
  : m_uid (0)
{
}

WifiMode::WifiMode (uint32_t uid)
  : m_uid (uid)
{
}

WifiMode::WifiMode (std::string name)
{
  *this = WifiModeFactory::GetFactory ()->Search (name);
}

ATTRIBUTE_HELPER_CPP (WifiMode);

WifiModeFactory::WifiModeFactory ()
{
}

WifiMode
WifiModeFactory::CreateWifiMode (std::string uniqueName,
                                 WifiModulationClass modClass,
                                 bool isMandatory,
                                 CodeRateCallback codeRateCallback,
                                 ConstellationSizeCallback constellationSizeCallback,
                                 PhyRateCallback phyRateCallback,
                                 DataRateCallback dataRateCallback,
                                 DataRateFromTxVectorCallback dataRateFromTxVectorCallback,
                                 ModeAllowedCallback isModeAllowedCallback)
{
  WifiModeFactory *factory = GetFactory ();
  uint32_t uid = factory->AllocateUid (uniqueName);
  WifiModeItem *item = factory->Get (uid);
  item->uniqueUid = uniqueName;
  item->modClass = modClass;
  //The modulation class for this WifiMode must be valid.
  NS_ASSERT (modClass != WIFI_MOD_CLASS_UNKNOWN);

  //Check for compatibility between modulation class and coding
  //rate. If modulation class is DSSS then coding rate must be
  //undefined, and vice versa. I could have done this with an
  //assertion, but it seems better to always give the error (i.e.,
  //not only in non-optimised builds) and the cycles that extra test
  //here costs are only suffered at simulation setup.
  if ((codeRateCallback () == WIFI_CODE_RATE_UNDEFINED) && modClass != WIFI_MOD_CLASS_DSSS && modClass != WIFI_MOD_CLASS_HR_DSSS)
    {
      NS_FATAL_ERROR ("Error in creation of WifiMode named " << uniqueName << std::endl
                                                             << "Code rate must be WIFI_CODE_RATE_UNDEFINED iff Modulation Class is WIFI_MOD_CLASS_DSSS or WIFI_MOD_CLASS_HR_DSSS");
    }

  item->isMandatory = isMandatory;
  item->GetCodeRateCallback = codeRateCallback;
  item->GetConstellationSizeCallback = constellationSizeCallback;
  item->GetPhyRateCallback = phyRateCallback;
  item->GetDataRateCallback = dataRateCallback;
  item->GetDataRateFromTxVectorCallback = dataRateFromTxVectorCallback;
  item->GetNonHtReferenceRateCallback = MakeNullCallback<uint64_t> ();
  item->IsModeAllowedCallback = isModeAllowedCallback;

  NS_ASSERT (modClass < WIFI_MOD_CLASS_HT);
  //fill unused MCS item with a dummy value
  item->mcsValue = 0;

  return WifiMode (uid);
}

WifiMode
WifiModeFactory::CreateWifiMcs (std::string uniqueName,
                                uint8_t mcsValue,
                                WifiModulationClass modClass,
                                CodeRateCallback codeRateCallback,
                                ConstellationSizeCallback constellationSizeCallback,
                                PhyRateCallback phyRateCallback,
                                DataRateCallback dataRateCallback,
                                DataRateFromTxVectorCallback dataRateFromTxVectorCallback,
                                NonHtReferenceRateCallback nonHtReferenceRateCallback,
                                ModeAllowedCallback isModeAllowedCallback)
{
  WifiModeFactory *factory = GetFactory ();
  uint32_t uid = factory->AllocateUid (uniqueName);
  WifiModeItem *item = factory->Get (uid);
  item->uniqueUid = uniqueName;
  item->modClass = modClass;

  NS_ASSERT (modClass >= WIFI_MOD_CLASS_HT);

  item->mcsValue = mcsValue;
  item->GetCodeRateCallback = codeRateCallback;
  item->GetConstellationSizeCallback = constellationSizeCallback;
  item->GetPhyRateCallback = phyRateCallback;
  item->GetDataRateCallback = dataRateCallback;
  item->GetDataRateFromTxVectorCallback = dataRateFromTxVectorCallback;
  item->GetNonHtReferenceRateCallback = nonHtReferenceRateCallback;
  item->IsModeAllowedCallback = isModeAllowedCallback;

  //fill unused items with dummy values
  item->isMandatory = false;

  return WifiMode (uid);
}

WifiMode
WifiModeFactory::Search (std::string name) const
{
  WifiModeItemList::const_iterator i;
  uint32_t j = 0;
  for (i = m_itemList.begin (); i != m_itemList.end (); i++)
    {
      if (i->uniqueUid == name)
        {
          return WifiMode (j);
        }
      j++;
    }

  //If we get here then a matching WifiMode was not found above. This
  //is a fatal problem, but we try to be helpful by displaying the
  //list of WifiModes that are supported.
  NS_LOG_UNCOND ("Could not find match for WifiMode named \""
                 << name << "\". Valid options are:");
  for (i = m_itemList.begin (); i != m_itemList.end (); i++)
    {
      NS_LOG_UNCOND ("  " << i->uniqueUid);
    }
  //Empty fatal error to die. We've already unconditionally logged
  //the helpful information.
  NS_FATAL_ERROR ("");

  //This next line is unreachable because of the fatal error
  //immediately above, and that is fortunate, because we have no idea
  //what is in WifiMode (0), but we do know it is not what our caller
  //has requested by name. It's here only because it's the safest
  //thing that'll give valid code.
  return WifiMode (0);
}

uint32_t
WifiModeFactory::AllocateUid (std::string uniqueUid)
{
  uint32_t j = 0;
  for (WifiModeItemList::const_iterator i = m_itemList.begin ();
       i != m_itemList.end (); i++)
    {
      if (i->uniqueUid == uniqueUid)
        {
          return j;
        }
      j++;
    }
  uint32_t uid = static_cast<uint32_t> (m_itemList.size ());
  m_itemList.push_back (WifiModeItem ());
  return uid;
}

WifiModeFactory::WifiModeItem *
WifiModeFactory::Get (uint32_t uid)
{
  NS_ASSERT (uid < m_itemList.size ());
  return &m_itemList[uid];
}

WifiModeFactory *
WifiModeFactory::GetFactory (void)
{
  static bool isFirstTime = true;
  static WifiModeFactory factory;
  if (isFirstTime)
    {
      uint32_t uid = factory.AllocateUid ("Invalid-WifiMode");
      WifiModeItem *item = factory.Get (uid);
      item->uniqueUid = "Invalid-WifiMode";
      item->modClass = WIFI_MOD_CLASS_UNKNOWN;
      item->isMandatory = false;
      item->mcsValue = 0;
      item->GetCodeRateCallback = MakeNullCallback<WifiCodeRate> ();
      item->GetConstellationSizeCallback = MakeNullCallback<uint16_t> ();
      item->GetPhyRateCallback = MakeNullCallback<uint64_t, uint16_t, uint16_t, uint8_t> ();
      item->GetDataRateCallback = MakeNullCallback<uint64_t, uint16_t, uint16_t, uint8_t> ();
      item->GetDataRateFromTxVectorCallback = MakeNullCallback<uint64_t, WifiTxVector, uint16_t> ();
      item->GetNonHtReferenceRateCallback = MakeNullCallback<uint64_t> ();
      item->IsModeAllowedCallback = MakeNullCallback<bool, uint16_t, uint8_t> ();
      isFirstTime = false;
    }
  return &factory;
}

} //namespace ns3
