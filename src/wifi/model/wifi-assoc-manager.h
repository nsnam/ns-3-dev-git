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
#include "wifi-phy.h"

#include "ns3/attribute-container.h"
#include "ns3/enum.h"
#include "ns3/pair.h"
#include "ns3/wifi-export.h"

#include <list>
#include <optional>
#include <set>
#include <unordered_map>

namespace ns3
{

/**
 * @ingroup wifi
 *
 * Scan type (active or passive)
 */
enum class WifiScanType : uint8_t
{
    ACTIVE = 0,
    PASSIVE
};

/**
 * @ingroup wifi
 *
 * Structure holding scan parameters
 */
struct WIFI_EXPORT WifiScanParams
{
    /// typedef for a PHY band-indexed map of channels to scan
    using ChannelList = std::map<WifiPhyBand, std::list<uint64_t>>;

    /// typedef for PHY ID-indexed map of channels to scan
    using Map = std::map<uint8_t, ChannelList>;

    WifiScanType type;   ///< indicates either active or passive scanning
    Ssid ssid;           ///< desired SSID or wildcard SSID
    Map channelList;     ///< list of channels to scan, for each PHY ID
    Time probeDelay;     ///< delay prior to transmitting a Probe Request
    Time minChannelTime; ///< minimum time to spend on each channel
    Time maxChannelTime; ///< maximum time to spend on each channel
};

/**
 * @ingroup wifi
 *
 * Enumerate protection techniques to adopt before performing an offline channel scan:
 * - NONE: no protection
 * - POWERSAVE: switch to powersave mode before starting offline channel scanning
 */
enum class WifiOfflineScanProtection : uint8_t
{
    NONE = 0,
    POWERSAVE
};

/**
 * @ingroup wifi
 *
 * Abstract base class for the Association Manager, which manages
 * scanning and association for single link devices and ML discovery
 * and setup for multi-link devices.
 */
class WIFI_EXPORT WifiAssocManager : public Object
{
    /**
     * Struct providing a function call operator to compare two ApInfo objects.
     * This struct is used as the Compare template type parameter of the set of
     * ApInfo objects maintained by the Association Manager.
     */
    struct WIFI_EXPORT ApInfoCompare
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
    struct WIFI_EXPORT RnrLinkInfo
    {
        std::size_t m_nbrApInfoId;     ///< Neighbor AP Information field index
        std::size_t m_tbttInfoFieldId; ///< TBTT Information field index
    };

    /// Structure to hold the results of a scanning procedure on a specific channel
    struct PerChanScanResults
    {
        uint8_t phyId;                 ///< ID of the PHY on which the scan has been performed
        Time scanStartTime;            ///< time at which the scan started on that channel
        Time scanEndTime;              ///< time at which the scan ended on that channel
        WifiChannelConfig scanChannel; ///< channel settings of the channel that was scanned
        std::list<StaWifiMac::ApInfo>
            apList; ///< list of APs found during the scan (sorted in the order of discovery)
    };

    /// Structure to hold the results of a scanning procedure
    struct WifiScanResults
    {
        std::vector<PerChanScanResults> perChanResults; ///< list of results for each scanned
                                                        ///< channel in the order they were scanned
    };

    /// Iterator type for per-channel scan results
    using PerChanScanResultsIterator = std::vector<PerChanScanResults>::iterator;

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
     * Start scanning using the current STA configuration.
     *
     * @param offline whether the scanning procedure is an offline channel scanning procedure
     */
    void StartScanning(bool offline = false);

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
    bool NotifyChannelSwitched(linkId_t linkId);

    /**
     * Notify that ML setup has been completed and link IDs have been swapped. This method
     * updates the link IDs in the information gathered so far.
     *
     * @param swapInfo a set of pairs (from, to) each mapping a current link ID to the
     *              link ID it has to become (i.e., link 'from' becomes link 'to')
     */
    void NotifyLinkSwapped(const std::map<uint8_t, uint8_t>& swapInfo);

    /**
     * Notify that the Power Management mode of the non-AP STA operating on the given link
     * has changed.
     *
     * @param pmMode the new PM mode
     * @param linkId the ID of the given link
     */
    void NotifyPmModeChanged(WifiPowerManagementMode pmMode, linkId_t linkId);

    /**
     * Start an offline channel scanning procedure.
     */
    void StartOfflineScanning();

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

    /**
     * Convert the data structure containing the channels to scan during scanning procedures into
     * a string formatted as described for the ScanningChannels attribute.
     *
     * @param channels the data structure containing the channels to scan
     * @return a string formatted as described for the ScanningChannels attribute
     */
    static std::string ScanningChannelsToStr(const WifiScanParams::Map& channels);

    /**
     * Return the configured channels to scan, replacing empty per-PHY maps with the current
     * primary 20 MHz channel used by the corresponding PHY.
     *
     * @return a copy of the channels to scan with the current channels filled in where needed
     */
    WifiScanParams::Map GetCurrentChannelsForScanning();

  protected:
    /**
     * Constructor (protected as this is an abstract base class)
     */
    WifiAssocManager();
    void DoDispose() override;

    /// typedef for the sorted list of ApInfo objects
    using SortedList = std::set<StaWifiMac::ApInfo, ApInfoCompare>;

    /// typedef for an attribute value storing a pair (WifiPhyBand, list of channel numbers)
    using PhyBandChannelNumbersPairValue =
        PairValue<EnumValue<WifiPhyBand>, AttributeContainerValue<UintegerValue>>;

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

    /**
     * Restore channels used before starting an offline channel scanning procedure.
     */
    void OfflineScanningTimeout();

    /// typedef for an optional const reference to a ReducedNeighborReport object
    using OptRnrConstRef = std::optional<std::reference_wrapper<const ReducedNeighborReport>>;
    /// typedef for an optional const reference to a MultiLinkElement object
    using OptMleConstRef = std::optional<std::reference_wrapper<const MultiLinkElement>>;

    /**
     * Check whether 11be Multi-Link setup can be established with the current best AP.
     *
     * @param [out] mle const reference to the Multi-Link Element present in the
     *                  Beacon/Probe Response received from the best AP, if any
     * @param [out] rnr const reference to the Reduced Neighbor Report Element present
     *                  in the Beacon/Probe Response received from the best AP, if any.
     * @return whether 11be Multi-Link setup can be established with the current best AP
     */
    bool CanSetupMultiLink(OptMleConstRef& mle, OptRnrConstRef& rnr);

    Ptr<StaWifiMac> m_mac;             ///< pointer to the STA wifi MAC
    std::set<linkId_t> m_allowedLinks; /**< Only Beacon and Probe Response frames received on a
                                            link belonging to the this set are processed */

    Time m_channelSwitchTimeout;        ///< maximum delay for channel switching
    bool m_allowAssocAllChannelWidths;  ///< flag whether the check on channel width compatibility
                                        ///< with the candidate AP should be skipped
    bool m_enableScanning;              ///< whether channel scanning is enabled
    WifiScanParams::Map m_scanChannels; ///< data structure containing the channels to scan

  private:
    std::set<linkId_t> m_activeBeforeScan; ///< IDs of links on which STAs are operating in active
                                           ///< mode before switching to PS mode for offline
                                           ///< scanning

    /**
     * Get a checker for an attribute value consisting of a WifiPhyBand-index map of the list of
     * channels to scan. This is used internally by SetScanningChannelsFromStr() and
     * GetStrFromScanningChannels().
     *
     * @return a checker for an attribute value consisting of a WifiPhyBand-index map of the list
     *         of channels to scan
     */
    static Ptr<const AttributeChecker> GetPhyBandChannelListChecker();

    /**
     * Set the channels to scan during scanning procedures starting from a string formatted as
     * described for the ScanningChannels attribute.
     *
     * @param s the string formatted as described for the ScanningChannels attribute
     */
    void SetScanningChannelsFromStr(const std::string& s);

    /**
     * Process the data structure containing the channels to scan by replacing unspecified band
     * and channel number values with their actual values, provided that PHYs have been installed.
     */
    void ExpandScanningChannelsMap();

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
    virtual bool DoNotifyChannelSwitched(linkId_t linkId) = 0;

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
     * Find the most recent scan results entry for a given PHY.
     *
     * @param phyId the ID of the PHY
     * @return optional iterator to the latest scan result for the PHY
     */
    std::optional<PerChanScanResultsIterator> GetLastPerChanScanResult(uint8_t phyId);

    /**
     * Switch the given PHY to the next channel to scan. If there is no other channel to scan for
     * the given PHY, call EndScanning() if scanning has been completed for all other PHYs.
     *
     * @param phyId the ID of the given PHY
     * @param band the band in which the given PHY is operating
     * @param channelNo the current channel number
     */
    void SwitchToNextChannelToScan(uint8_t phyId, WifiPhyBand band, uint8_t channelNo);

    /**
     * Restore state after the completion of an offline channel scanning procedure.
     */
    void HandleOfflineScanningEnded();

    /**
     * Request an offline channel scanning procedure and schedule the next one after the given
     * interval.
     *
     * @param interval the time interval after which scheduling the next offline scanning procedure
     */
    void RequestOfflineScanning(const Time& interval);

    WifiScanParams m_scanParams; ///< scanning parameters
    std::map<uint8_t, EventId>
        m_currentChannelScanEnd; ///< PHY ID-indexed map of events indicating the end of the scan of
                                 ///< the current channel
    std::map<uint8_t, EventId>
        m_switchToChannelToScanTimeout; ///< PHY ID-indexed map of events indicating the end of the
                                        ///< timer started when switching to the channel to scan
    std::map<uint8_t, WifiPhyOperatingChannel>
        m_channelsToSet; ///< PHY ID-indexed map of the frequency channels to set after a channel
                         ///< scanning procedure is completed
    std::map<uint8_t, EventId>
        m_switchToChannelToSetTimeout; ///< PHY ID-indexed map of events indicating the end of the
                                       ///< timer started when switching to the channels to set
                                       ///< at the end of a scanning procedure
    SortedList m_apList;               ///< sorted list of candidate APs
    std::optional<StaWifiMac::ApInfo> m_bestAp; ///< the best AP to return to STA wifi MAC
    WifiOfflineScanProtection
        m_offlineScanProtection;      ///< technique to protect an offline channel scan
    EventId m_nextOfflineChannelScan; ///< event associated with the next offline channel scan
    bool m_enableChannelSwitch; ///< whether to enable channel switch at the end of channel scanning

    /// hash table to help locate ApInfo objects in the sorted list based on the BSSID
    std::unordered_map<Mac48Address, SortedList::const_iterator, WifiAddressHash> m_apListIt;
    WifiScanResults m_currScanResults; ///< results of the current scanning procedure
    TracedCallback<const WifiScanResults&> m_scanEndLogger; ///< scanning procedure end logger

    /// TracedCallback signature for scanning procedure end events
    using ScanEndCallback = void (*)(const WifiScanResults&);
};

} // namespace ns3

#endif /* WIFI_ASSOC_MANAGER_H */
