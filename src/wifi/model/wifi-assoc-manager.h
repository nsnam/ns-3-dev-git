/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_ASSOC_MANAGER_H
#define WIFI_ASSOC_MANAGER_H

#include "qos-utils.h"
#include "sta-wifi-mac.h"
#include <set>
#include <unordered_map>

namespace ns3  {

/**
 * \ingroup wifi
 *
 * Abstract base class for the Association Manager, which manages
 * scanning and association for single link devices and ML discovery
 * and setup for multi-link devices.
 */
class WifiAssocManager : public Object
{
  /**
  * Struct providing a function call operator to compare two ApInfo objects.
  * This struct is used as the Compare template type parameter of the set of
  * ApInfo objects maintained by the Association Manager.
  */
  struct ApInfoCompare
  {
    /**
    * Constructor.
    *
    * \param manager a pointer to the Association Manager
    */
    ApInfoCompare (const WifiAssocManager& manager);
    /**
    * Function call operator. Calls the Compare method of the Association Manager.
    *
    * \param lhs left hand side ApInfo object
    * \param rhs right hand side ApInfo object
    * \return true if the left hand side ApInfo object should be placed before the
    *         right hand side ApInfo object in the sorted list maintained by the
    *         Association Manager, false otherwise
    */
    bool operator()(const StaWifiMac::ApInfo& lhs, const StaWifiMac::ApInfo& rhs) const;

  private:
    const WifiAssocManager& m_manager;    ///< Association Manager
  };

public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual ~WifiAssocManager ();

  /**
   * Set the pointer to the STA wifi MAC.
   *
   * \param mac the pointer to the STA wifi MAC
   */
  void SetStaWifiMac (Ptr<StaWifiMac> mac);

  /**
   * Request the Association Manager to start a scanning procedure according to
   * the given scanning parameters. At subclass' discretion, stored information
   * about APs matching the given scanning parameters may be used and scanning
   * not performed.
   *
   * \param scanParams the scanning parameters
   */
  void StartScanning (WifiScanParams&& scanParams);

  /**
   * STA wifi MAC received a Beacon frame or Probe Response frame while scanning
   * and notifies us the AP information contained in the received frame.
   *
   * Note that the given ApInfo object is moved to the sorted list of ApInfo objects.
   *
   * \param apInfo the AP information contained in the received frame
   */
  virtual void NotifyApInfo (const StaWifiMac::ApInfo&& apInfo);

  /**
   * Compare two ApInfo objects for the purpose of keeping a sorted list of
   * ApInfo objects.
   *
   * \param lhs left hand side ApInfo object
   * \param rhs right hand side ApInfo object
   * \return true if the left hand side ApInfo object should be placed before the
   *         right hand side ApInfo object in the sorted list of ApInfo objects,
   *         false otherwise
   */
  virtual bool Compare (const StaWifiMac::ApInfo& lhs, const StaWifiMac::ApInfo& rhs) const = 0;

protected:
  /**
   * Constructor (protected as this is an abstract base class)
   */
  WifiAssocManager ();
  void DoDispose (void) override;

  /// typedef for the sorted list of ApInfo objects
  using SortedList = std::set<StaWifiMac::ApInfo, ApInfoCompare>;

  /**
   * \return a const reference to the sorted list of ApInfo objects.
   */
  const SortedList& GetSortedList (void) const;

  /**
   * \return the scanning parameters.
   */
  const WifiScanParams& GetScanParams (void) const;

  /**
   * Check whether the given AP information match the current scanning parameters.
   *
   * \param apInfo the given AP information
   * \return whether the given AP information match the current scanning parameters
   */
  bool MatchScanParams (const StaWifiMac::ApInfo& apInfo) const;

  /**
   * Allow subclasses to choose whether the given ApInfo shall be considered
   * and hence inserted in the sorted list of ApInfo objects.
   *
   * \param apInfo the apInfo object to insert
   * \return true if the apInfo object can be inserted, false otherwise
   */
  virtual bool CanBeInserted (const StaWifiMac::ApInfo& apInfo) const = 0;
  /**
   * Allow subclasses to choose whether the given ApInfo shall be returned or
   * discarded when the STA wifi MAC requests information on the best AP.
   *
   * \param apInfo the apInfo object to return
   * \return true if the apInfo object can be returned, false otherwise
   */
  virtual bool CanBeReturned (const StaWifiMac::ApInfo& apInfo) const = 0;

  /**
   * Extract the best AP to associate with from the sorted list and return
   * it, if any, to the STA wifi MAC along with the notification that scanning
   * is completed.
   */
  void ScanningTimeout (void);

  Ptr<StaWifiMac> m_mac;                                ///< pointer to the STA wifi MAC

private:
  /**
   * Start a scanning procedure. This method needs to schedule a call to
   * ScanningTimeout when the scanning procedure is completed.
   */
  virtual void DoStartScanning (void) = 0;

  WifiScanParams m_scanParams;              ///< scanning parameters
  SortedList m_apList;                      ///< sorted list of candidate APs
  /// hash table to help locate ApInfo objects in the sorted list based on the BSSID
  std::unordered_map<Mac48Address, SortedList::const_iterator, WifiAddressHash> m_apListIt;
};

} //namespace ns3

#endif /* WIFI_ASSOC_MANAGER_H */
