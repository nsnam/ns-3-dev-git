/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_ASSOC_MANAGER_H
#define WIFI_ASSOC_MANAGER_H

#include "qos-utils.h"
#include "sta-wifi-mac.h"

#include <optional>
#include <set>
#include <unordered_map>

namespace ns3
{

/**
 * @ingroup wifi
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
         * @param manager a pointer to the Association Manager
         */
        ApInfoCompare(const WifiAssocManager& manager);
        /**
         * Function call operator. Calls the Compare method of the Association Manager.
         *
         * @param lhs left hand side ApInfo object
         * @param rhs right hand side ApInfo object
         * @return true if the left hand side ApInfo object should be placed before the
         *         right hand side ApInfo object in the sorted list maintained by the
         *         Association Manager, false otherwise
         */
        bool operator()(const StaWifiMac::ApInfo& lhs, const StaWifiMac::ApInfo& rhs) const;

      private:
        const WifiAssocManager& m_manager; ///< Association Manager
    };

  public:
    /**
     * Struct to identify a specific TBTT Information field of a Neighbor AP Information
     * field in a Reduced Neighbor Report element.
     */
    struct RnrLinkInfo
    {
        std::size_t m_nbrApInfoId;     ///< Neighbor AP Information field index
        std::size_t m_tbttInfoFieldId; ///< TBTT Information field index
    };

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    ~WifiAssocManager() override;

    /**
     * Set the pointer to the STA wifi MAC.
     *
     * @param mac the pointer to the STA wifi MAC
     */
    void SetStaWifiMac(Ptr<StaWifiMac> mac);

    /**
     * Clear the stored information about all known APs.
     */
    void ClearStoredApInfo();

    /**
     * Request the Association Manager to start a scanning procedure according to
     * the given scanning parameters. At subclass' discretion, stored information
     * about APs matching the given scanning parameters may be used and scanning
     * not performed.
     *
     * @param scanParams the scanning parameters
     */
    void StartScanning(WifiScanParams&& scanParams);

    /**
     * STA wifi MAC received a Beacon frame or Probe Response frame while scanning
     * and notifies us the AP information contained in the received frame.
     *
     * Note that the given ApInfo object is moved to the sorted list of ApInfo objects.
     *
     * @param apInfo the AP information contained in the received frame
     */
    virtual void NotifyApInfo(const StaWifiMac::ApInfo&& apInfo);

    /**
     * Notify that the given link has completed channel switching.
     *
     * @param linkId the ID of the given link
     * @return whether the channel switch was requested by this manager in the context of its
     *         scanning/association procedures
     */
    bool NotifyChannelSwitched(uint8_t linkId);

    /**
     * Notify that ML setup has been completed and link IDs have been swapped. This method
     * updates the link IDs in the information gathered so far.
     *
     * @param swapInfo a set of pairs (from, to) each mapping a current link ID to the
     *              link ID it has to become (i.e., link 'from' becomes link 'to')
     */
    void NotifyLinkSwapped(const std::map<uint8_t, uint8_t>& swapInfo);

    /**
     * Compare two ApInfo objects for the purpose of keeping a sorted list of
     * ApInfo objects.
     *
     * @param lhs left hand side ApInfo object
     * @param rhs right hand side ApInfo object
     * @return true if the left hand side ApInfo object should be placed before the
     *         right hand side ApInfo object in the sorted list of ApInfo objects,
     *         false otherwise
     */
    virtual bool Compare(const StaWifiMac::ApInfo& lhs, const StaWifiMac::ApInfo& rhs) const = 0;

    /**
     * Search the given RNR element for APs affiliated to the same AP MLD as the
     * reporting AP. The search starts at the given Neighbor AP Information field.
     *
     * @param rnr the given RNR element
     * @param nbrApInfoId the index of the given Neighbor AP Information field
     * @return the index of the Neighbor AP Information field and the index of the
     *         TBTT Information field containing the next affiliated AP, if any.
     */
    static std::optional<WifiAssocManager::RnrLinkInfo> GetNextAffiliatedAp(
        const ReducedNeighborReport& rnr,
        std::size_t nbrApInfoId);

    /**
     * Find all the APs affiliated to the same AP MLD as the reporting AP that sent
     * the given RNR element.
     *
     * @param rnr the given RNR element
     * @return a list containing the index of the Neighbor AP Information field and the index of the
     *         TBTT Information field containing all the affiliated APs
     */
    static std::list<WifiAssocManager::RnrLinkInfo> GetAllAffiliatedAps(
        const ReducedNeighborReport& rnr);

    /**
     * @return whether a channel scanning procedure is ongoing
     */
    bool IsScanning() const;

  protected:
    /**
     * Constructor (protected as this is an abstract base class)
     */
    WifiAssocManager();
    void DoDispose() override;

    /// typedef for the sorted list of ApInfo objects
    using SortedList = std::set<StaWifiMac::ApInfo, ApInfoCompare>;

    /**
     * @return a const reference to the sorted list of ApInfo objects.
     */
    const SortedList& GetSortedList() const;

    /**
     * Get a reference to the list of the links to setup with the given AP. This method
     * allows subclasses to modify such a list.
     *
     * @param apInfo the info about the given AP
     * @return a reference to the list of the links to setup with the given AP
     */
    std::list<StaWifiMac::ApInfo::SetupLinksInfo>& GetSetupLinks(const StaWifiMac::ApInfo& apInfo);

    /**
     * @return the scanning parameters.
     */
    const WifiScanParams& GetScanParams() const;

    /**
     * Check whether the given AP information match the current scanning parameters.
     *
     * @param apInfo the given AP information
     * @return whether the given AP information match the current scanning parameters
     */
    bool MatchScanParams(const StaWifiMac::ApInfo& apInfo) const;

    /**
     * Allow subclasses to choose whether the given ApInfo shall be considered
     * and hence inserted in the sorted list of ApInfo objects.
     *
     * @param apInfo the apInfo object to insert
     * @return true if the apInfo object can be inserted, false otherwise
     */
    virtual bool CanBeInserted(const StaWifiMac::ApInfo& apInfo) const = 0;
    /**
     * Allow subclasses to choose whether the given ApInfo shall be returned or
     * discarded when the STA wifi MAC requests information on the best AP.
     *
     * @param apInfo the apInfo object to return
     * @return true if the apInfo object can be returned, false otherwise
     */
    virtual bool CanBeReturned(const StaWifiMac::ApInfo& apInfo) const = 0;

    /**
     * Check whether the channel width of the STA is compatible with the channel width of the
     * operating channel of a given AP. A channel width is compatible if the STA can advertise it to
     * the AP, or AP operates on a channel width that is equal or lower than that channel width
     *
     * @param apInfo the given AP information
     * @return true if the channel width of the STA is compatible, false otherwise
     */
    bool IsChannelWidthCompatible(const StaWifiMac::ApInfo& apInfo) const;

    /**
     * Start a scanning procedure for each of the PHYs indicated in the scanning parameters. For
     * each PHY, all frequency channels indicated in the scanning parameters for that PHY are
     * scanned.
     */
    void ScanChannels();

    /**
     * Subclasses must implement this method, which is called at the end of a channel scanning
     * procedure. This method needs to call ScanningTimeout(), which notifies the STA wifi MAC.
     */
    virtual void EndScanning() = 0;

    /**
     * Extract the best AP to associate with from the sorted list and return
     * it, if any, to the STA wifi MAC along with the notification that scanning
     * is completed.
     */
    void ScanningTimeout();

    /// typedef for an optional const reference to a ReducedNeighborReport object
    using OptRnrConstRef = std::optional<std::reference_wrapper<const ReducedNeighborReport>>;
    /// typedef for an optional const reference to a MultiLinkElement object
    using OptMleConstRef = std::optional<std::reference_wrapper<const MultiLinkElement>>;

    /**
     * Check whether 11be Multi-Link setup can be established with the current best AP.
     *
     * \param[out] mle const reference to the Multi-Link Element present in the
     *                 Beacon/Probe Response received from the best AP, if any
     * \param[out] rnr const reference to the Reduced Neighbor Report Element present
     *                 in the Beacon/Probe Response received from the best AP, if any.
     * @return whether 11be Multi-Link setup can be established with the current best AP
     */
    bool CanSetupMultiLink(OptMleConstRef& mle, OptRnrConstRef& rnr);

    Ptr<StaWifiMac> m_mac;            ///< pointer to the STA wifi MAC
    std::set<uint8_t> m_allowedLinks; /**< "Only Beacon and Probe Response frames received on a
                                            link belonging to the this set are processed */

    Time m_channelSwitchTimeout;       ///< maximum delay for channel switching
    bool m_allowAssocAllChannelWidths; ///< flag whether the check on channel width compatibility
                                       ///< with the candidate AP should be skipped

  private:
    /**
     * Request subclasses to start a scanning procedure.
     */
    virtual void DoStartScanning() = 0;

    /**
     * Notify subclasses that the given link has completed channel switching.
     *
     * @param linkId the ID of the given link
     * @return whether the channel switch was requested by the subclass in the context of its
     *         scanning/association procedures
     */
    virtual bool DoNotifyChannelSwitched(uint8_t linkId) = 0;

    /**
     * Switch the given PHY to the given channel (if needed) and start scanning.
     *
     * @param phyId the ID of the given PHY
     * @param band the band of the channel to scan
     * @param channelIt an iterator pointing to the channel to scan
     */
    void SwitchToChannelToScan(uint8_t phyId,
                               WifiPhyBand band,
                               WifiScanParams::ChannelList::mapped_type::const_iterator channelIt);

    /**
     * Scan the channel the given PHY is currently operating on using active or passive scanning
     * (depending on the scanning parameters).
     *
     * @param phyId the ID of the given PHY
     * @param band the band in which the given PHY is operating
     * @param channelIt an iterator pointing to the current channel in the scanning list
     */
    void ScanCurrentChannel(uint8_t phyId,
                            WifiPhyBand band,
                            WifiScanParams::ChannelList::mapped_type::const_iterator channelIt);

    /**
     * Switch the given PHY to the next channel to scan. If there is no other channel to scan for
     * the given PHY, call EndScanning() if scanning has been completed for all other PHYs.
     *
     * @param phyId the ID of the given PHY
     * @param band the band in which the given PHY is operating
     * @param channelNo the current channel number
     */
    void SwitchToNextChannelToScan(uint8_t phyId, WifiPhyBand band, uint8_t channelNo);

    WifiScanParams m_scanParams; ///< scanning parameters
    std::map<uint8_t, EventId>
        m_currentChannelScanEnd; ///< PHY ID-indexed map of events indicating the end of the scan of
                                 ///< the current channel
    std::map<uint8_t, EventId>
        m_switchToChannelToScanTimeout; ///< PHY ID-indexed map of events indicating the end of the
                                        ///< timer started when switching to the channel to scan
    SortedList m_apList;                ///< sorted list of candidate APs
    /// hash table to help locate ApInfo objects in the sorted list based on the BSSID
    std::unordered_map<Mac48Address, SortedList::const_iterator, WifiAddressHash> m_apListIt;
};

} // namespace ns3

#endif /* WIFI_ASSOC_MANAGER_H */
