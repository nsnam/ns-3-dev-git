/*
 * Copyright (c) 2005,2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef WIFI_REMOTE_STATION_MANAGER_H
#define WIFI_REMOTE_STATION_MANAGER_H

#include "extended-capabilities.h"
#include "qos-utils.h"
#include "wifi-mode.h"
#include "wifi-remote-station-info.h"
#include "wifi-utils.h"

#include "ns3/common-info-basic-mle.h"
#include "ns3/data-rate.h"
#include "ns3/eht-capabilities.h"
#include "ns3/he-6ghz-band-capabilities.h"
#include "ns3/he-capabilities.h"
#include "ns3/ht-capabilities.h"
#include "ns3/mac48-address.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"
#include "ns3/vht-capabilities.h"

#include <array>
#include <list>
#include <memory>
#include <optional>
#include <unordered_map>

namespace ns3
{

class WifiPhy;
class WifiMac;
class WifiMacHeader;
class Packet;
class WifiMpdu;
class WifiPsdu;
class WifiTxVector;
class WifiTxParameters;

struct WifiRemoteStationState;
struct RxSignalInfo;

/**
 * @brief hold per-remote-station state.
 *
 * The state in this class is used to keep track
 * of association status if we are in an infrastructure
 * network and to perform the selection of TX parameters
 * on a per-packet basis.
 *
 * This class is typically subclassed and extended by
 * rate control implementations
 */
struct WifiRemoteStation
{
    virtual ~WifiRemoteStation()
    {
    }

    WifiRemoteStationState* m_state; //!< Remote station state
    std::pair<dBm_u, Time>
        m_rssiAndUpdateTimePair; //!< RSSI of the most recent packet received from
                                 //!< the remote station along with update time
};

/**
 * A struct that holds information about each remote station.
 */
struct WifiRemoteStationState
{
    /**
     * State of the station
     */
    enum
    {
        BRAND_NEW,
        DISASSOC,
        WAIT_ASSOC_TX_OK,
        GOT_ASSOC_TX_OK,
        ASSOC_REFUSED
    } m_state;

    /**
     * This member is the list of WifiMode objects that comprise the
     * OperationalRateSet parameter for this remote station. This list
     * is constructed through calls to
     * WifiRemoteStationManager::AddSupportedMode(), and an API that
     * allows external access to it is available through
     * WifiRemoteStationManager::GetNSupported() and
     * WifiRemoteStationManager::GetSupported().
     */
    WifiModeList m_operationalRateSet; //!< operational rate set
    WifiModeList m_operationalMcsSet;  //!< operational MCS set
    Mac48Address m_address;            //!< Mac48Address of the remote station
    uint16_t m_aid;                    /**< AID of the remote station (unused if this object
                                            is installed on a non-AP station) */
    WifiRemoteStationInfo m_info;      //!< remote station info
    bool m_dsssSupported;              //!< Flag if DSSS is supported by the remote station
    bool m_erpOfdmSupported;           //!< Flag if ERP OFDM is supported by the remote station
    bool m_ofdmSupported;              //!< Flag if OFDM is supported by the remote station
    Ptr<const HtCapabilities> m_htCapabilities; //!< remote station HT capabilities
    Ptr<const ExtendedCapabilities>
        m_extendedCapabilities;                   //!< remote station extended capabilities
    Ptr<const VhtCapabilities> m_vhtCapabilities; //!< remote station VHT capabilities
    Ptr<const HeCapabilities> m_heCapabilities;   //!< remote station HE capabilities
    Ptr<const He6GhzBandCapabilities>
        m_he6GhzBandCapabilities;                 //!< remote station HE 6GHz band capabilities
    Ptr<const EhtCapabilities> m_ehtCapabilities; //!< remote station EHT capabilities
    /// remote station Multi-Link Element Common Info
    std::shared_ptr<CommonInfoBasicMle> m_mleCommonInfo;
    bool m_emlsrEnabled; //!< whether EMLSR mode is enabled on this link

    MHz_u m_channelWidth; //!< Channel width supported by the remote station
    Time m_guardInterval; //!< HE Guard interval durationsupported by the remote station
    uint8_t m_ness;       //!< Number of extended spatial streams of the remote station
    bool m_aggregation;   //!< Flag if MPDU aggregation is used by the remote station
    bool m_shortPreamble; //!< Flag if short PHY preamble is supported by the remote station
    bool m_shortSlotTime; //!< Flag if short ERP slot time is supported by the remote station
    bool m_qosSupported;  //!< Flag if QoS is supported by the station
    bool m_isInPsMode;    //!< Flag if the STA is currently in PS mode
};

/**
 * @ingroup wifi
 * @brief hold a list of per-remote-station state.
 *
 * \sa ns3::WifiRemoteStation.
 */
class WifiRemoteStationManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    WifiRemoteStationManager();
    ~WifiRemoteStationManager() override;

    /// ProtectionMode enumeration
    enum ProtectionMode
    {
        RTS_CTS,
        CTS_TO_SELF
    };

    /**
     * A map of WifiRemoteStations with Mac48Address as key
     */
    using Stations = std::unordered_map<Mac48Address, WifiRemoteStation*, WifiAddressHash>;
    /**
     * A map of WifiRemoteStationStates with Mac48Address as key
     */
    using StationStates =
        std::unordered_map<Mac48Address, std::shared_ptr<WifiRemoteStationState>, WifiAddressHash>;

    /**
     * Set up PHY associated with this device since it is the object that
     * knows the full set of transmit rates that are supported.
     *
     * @param phy the PHY of this device
     */
    virtual void SetupPhy(const Ptr<WifiPhy> phy);
    /**
     * Set up MAC associated with this device since it is the object that
     * knows the full set of timing parameters (e.g. IFS).
     *
     * @param mac the MAC of this device
     */
    virtual void SetupMac(const Ptr<WifiMac> mac);

    /**
     * Set the ID of the link this Remote Station Manager is associated with.
     *
     * @param linkId the ID of the link this Remote Station Manager is associated with
     */
    void SetLinkId(uint8_t linkId);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    virtual int64_t AssignStreams(int64_t stream);

    /**
     * Sets the maximum STA short retry count (SSRC).
     *
     * @param maxSsrc the maximum SSRC
     */
    void SetMaxSsrc(uint32_t maxSsrc);
    /**
     * Sets the maximum STA long retry count (SLRC).
     *
     * @param maxSlrc the maximum SLRC
     */
    void SetMaxSlrc(uint32_t maxSlrc);
    /**
     * Sets the RTS threshold.
     *
     * @param threshold the RTS threshold
     */
    void SetRtsCtsThreshold(uint32_t threshold);

    /**
     * Return the fragmentation threshold.
     *
     * @return the fragmentation threshold
     */
    uint32_t GetFragmentationThreshold() const;
    /**
     * Sets a fragmentation threshold. The method calls a private method
     * DoSetFragmentationThreshold that checks the validity of the value given.
     *
     * @param threshold the fragmentation threshold
     */
    void SetFragmentationThreshold(uint32_t threshold);

    /**
     * Record the AID of a remote station. Should only be called by APs.
     *
     * @param remoteAddress the MAC address of the remote station
     * @param aid the Association ID
     */
    void SetAssociationId(Mac48Address remoteAddress, uint16_t aid);
    /**
     * Records QoS support of the remote station.
     *
     * @param from the address of the station being recorded
     * @param qosSupported whether the station supports QoS
     */
    void SetQosSupport(Mac48Address from, bool qosSupported);
    /**
     * @param from the address of the station being recorded
     * @param emlsrEnabled whether EMLSR mode is enabled for the station on this link
     */
    void SetEmlsrEnabled(const Mac48Address& from, bool emlsrEnabled);
    /**
     * Records HT capabilities of the remote station.
     *
     * @param from the address of the station being recorded
     * @param htCapabilities the HT capabilities of the station
     */
    void AddStationHtCapabilities(Mac48Address from, const HtCapabilities& htCapabilities);
    /**
     * Records extended capabilities of the remote station.
     *
     * @param from the address of the station being recorded
     * @param extendedCapabilities the extended capabilities of the station
     */
    void AddStationExtendedCapabilities(Mac48Address from,
                                        const ExtendedCapabilities& extendedCapabilities);
    /**
     * Records VHT capabilities of the remote station.
     *
     * @param from the address of the station being recorded
     * @param vhtCapabilities the VHT capabilities of the station
     */
    void AddStationVhtCapabilities(Mac48Address from, const VhtCapabilities& vhtCapabilities);
    /**
     * Records HE capabilities of the remote station.
     *
     * @param from the address of the station being recorded
     * @param heCapabilities the HE capabilities of the station
     */
    void AddStationHeCapabilities(Mac48Address from, const HeCapabilities& heCapabilities);
    /**
     * Records HE 6 GHz Band Capabilities of a remote station
     *
     * @param from the address of the remote station
     * @param he6GhzCapabilities the HE 6 GHz Band Capabilities of the remote station
     */
    void AddStationHe6GhzCapabilities(const Mac48Address& from,
                                      const He6GhzBandCapabilities& he6GhzCapabilities);
    /**
     * Records EHT capabilities of the remote station.
     *
     * @param from the address of the station being recorded
     * @param ehtCapabilities the EHT capabilities of the station
     */
    void AddStationEhtCapabilities(Mac48Address from, const EhtCapabilities& ehtCapabilities);
    /**
     * Records the Common Info field advertised by the given remote station in a Multi-Link
     * Element. It includes the MLD address of the remote station.
     *
     * @param from the address of the station being recorded
     * @param mleCommonInfo the MLE Common Info advertised by the station
     */
    void AddStationMleCommonInfo(Mac48Address from,
                                 const std::shared_ptr<CommonInfoBasicMle>& mleCommonInfo);
    /**
     * Return the HT capabilities sent by the remote station.
     *
     * @param from the address of the remote station
     * @return the HT capabilities sent by the remote station
     */
    Ptr<const HtCapabilities> GetStationHtCapabilities(Mac48Address from);
    /**
     * Return the extended capabilities sent by the remote station.
     *
     * @param from the address of the remote station
     * @return the extended capabilities sent by the remote station
     */
    Ptr<const ExtendedCapabilities> GetStationExtendedCapabilities(const Mac48Address& from);
    /**
     * Return the VHT capabilities sent by the remote station.
     *
     * @param from the address of the remote station
     * @return the VHT capabilities sent by the remote station
     */
    Ptr<const VhtCapabilities> GetStationVhtCapabilities(Mac48Address from);
    /**
     * Return the HE capabilities sent by the remote station.
     *
     * @param from the address of the remote station
     * @return the HE capabilities sent by the remote station
     */
    Ptr<const HeCapabilities> GetStationHeCapabilities(Mac48Address from);
    /**
     * Return the HE 6 GHz Band Capabilities sent by a remote station.
     *
     * @param from the address of the remote station
     * @return the HE 6 GHz Band capabilities sent by the remote station
     */
    Ptr<const He6GhzBandCapabilities> GetStationHe6GhzCapabilities(const Mac48Address& from) const;
    /**
     * Return the EHT capabilities sent by the remote station.
     *
     * @param from the address of the remote station
     * @return the EHT capabilities sent by the remote station
     */
    Ptr<const EhtCapabilities> GetStationEhtCapabilities(Mac48Address from);
    /**
     * @param from the (MLD or link) address of the remote non-AP MLD
     * @return the EML Capabilities advertised by the remote non-AP MLD
     */
    std::optional<std::reference_wrapper<CommonInfoBasicMle::EmlCapabilities>>
    GetStationEmlCapabilities(const Mac48Address& from);
    /**
     * @param from the (MLD or link) address of the remote non-AP MLD
     * @return the MLD Capabilities advertised by the remote non-AP MLD
     */
    std::optional<std::reference_wrapper<CommonInfoBasicMle::MldCapabilities>>
    GetStationMldCapabilities(const Mac48Address& from);
    /**
     * Return whether the device has HT capability support enabled on the link this manager is
     * associated with. Note that this means that this function returns false if this is a
     * 6 GHz link.
     *
     * @return true if HT capability support is enabled, false otherwise
     */
    bool GetHtSupported() const;
    /**
     * Return whether the device has VHT capability support enabled on the link this manager is
     * associated with. Note that this means that this function returns false if this is a
     * 2.4 or 6 GHz link.
     *
     * @return true if VHT capability support is enabled, false otherwise
     */
    bool GetVhtSupported() const;
    /**
     * Return whether the device has HE capability support enabled.
     *
     * @return true if HE capability support is enabled, false otherwise
     */
    bool GetHeSupported() const;
    /**
     * Return whether the device has EHT capability support enabled.
     *
     * @return true if EHT capability support is enabled, false otherwise
     */
    bool GetEhtSupported() const;
    /**
     * Return whether the device has LDPC support enabled.
     *
     * @return true if LDPC support is enabled, false otherwise
     */
    bool GetLdpcSupported() const;
    /**
     * Return whether the device has SGI support enabled.
     *
     * @return true if SGI support is enabled, false otherwise
     */
    bool GetShortGuardIntervalSupported() const;
    /**
     * Return the shortest supported HE guard interval duration.
     *
     * @return the shortest supported HE guard interval duration
     */
    Time GetGuardInterval() const;
    /**
     * Enable or disable protection for non-ERP stations.
     *
     * @param enable enable or disable protection for non-ERP stations
     */
    void SetUseNonErpProtection(bool enable);
    /**
     * Return whether the device supports protection of non-ERP stations.
     *
     * @return true if protection for non-ERP stations is enabled,
     *         false otherwise
     */
    bool GetUseNonErpProtection() const;
    /**
     * Enable or disable protection for non-HT stations.
     *
     * @param enable enable or disable protection for non-HT stations
     */
    void SetUseNonHtProtection(bool enable);
    /**
     * Return whether the device supports protection of non-HT stations.
     *
     * @return true if protection for non-HT stations is enabled,
     *         false otherwise
     */
    bool GetUseNonHtProtection() const;
    /**
     * Enable or disable short PHY preambles.
     *
     * @param enable enable or disable short PHY preambles
     */
    void SetShortPreambleEnabled(bool enable);
    /**
     * Return whether the device uses short PHY preambles.
     *
     * @return true if short PHY preambles are enabled,
     *         false otherwise
     */
    bool GetShortPreambleEnabled() const;
    /**
     * Enable or disable short slot time.
     *
     * @param enable enable or disable short slot time
     */
    void SetShortSlotTimeEnabled(bool enable);
    /**
     * Return whether the device uses short slot time.
     *
     * @return true if short slot time is enabled,
     *         false otherwise
     */
    bool GetShortSlotTimeEnabled() const;

    /**
     * Reset the station, invoked in a STA upon dis-association or in an AP upon reboot.
     */
    void Reset();

    /**
     * Invoked in a STA upon association to store the set of rates which belong to the
     * BSSBasicRateSet of the associated AP and which are supported locally.
     * Invoked in an AP to configure the BSSBasicRateSet.
     *
     * @param mode the WifiMode to be added to the basic mode set
     */
    void AddBasicMode(WifiMode mode);
    /**
     * Return the default transmission mode.
     *
     * @return WifiMode the default transmission mode
     */
    WifiMode GetDefaultMode() const;
    /**
     * Return the number of basic modes we support.
     *
     * @return the number of basic modes we support
     */
    uint8_t GetNBasicModes() const;
    /**
     * Return a basic mode from the set of basic modes.
     *
     * @param i index of the basic mode in the basic mode set
     *
     * @return the basic mode at the given index
     */
    WifiMode GetBasicMode(uint8_t i) const;
    /**
     * Return the number of non-ERP basic modes we support.
     *
     * @return the number of basic modes we support
     */
    uint32_t GetNNonErpBasicModes() const;
    /**
     * Return a basic mode from the set of basic modes that is not an ERP mode.
     *
     * @param i index of the basic mode in the basic mode set
     *
     * @return the basic mode at the given index
     */
    WifiMode GetNonErpBasicMode(uint8_t i) const;
    /**
     * Return whether the station supports LDPC or not.
     *
     * @param address the address of the station
     *
     * @return true if LDPC is supported by the station,
     *         false otherwise
     */
    bool GetLdpcSupported(Mac48Address address) const;
    /**
     * Return whether the station supports short PHY preamble or not.
     *
     * @param address the address of the station
     *
     * @return true if short PHY preamble is supported by the station,
     *         false otherwise
     */
    bool GetShortPreambleSupported(Mac48Address address) const;
    /**
     * Return whether the station supports short ERP slot time or not.
     *
     * @param address the address of the station
     *
     * @return true if short ERP slot time is supported by the station,
     *         false otherwise
     */
    bool GetShortSlotTimeSupported(Mac48Address address) const;
    /**
     * Return whether the given station is QoS capable.
     *
     * @param address the address of the station
     *
     * @return true if the station has QoS capabilities,
     *         false otherwise
     */
    bool GetQosSupported(Mac48Address address) const;
    /**
     * Get the AID of a remote station. Should only be called by APs.
     *
     * @param remoteAddress the MAC address of the remote station
     * @return the Association ID if the station is associated, SU_STA_ID otherwise
     */
    uint16_t GetAssociationId(Mac48Address remoteAddress) const;
    /**
     * Add a given Modulation and Coding Scheme (MCS) index to
     * the set of basic MCS.
     *
     * @param mcs the WifiMode to be added to the basic MCS set
     */
    void AddBasicMcs(WifiMode mcs);
    /**
     * Return the default Modulation and Coding Scheme (MCS) index.
     *
     * @return the default WifiMode
     */
    WifiMode GetDefaultMcs() const;
    /**
     * Return the default MCS to use to transmit frames to the given station.
     *
     * @param st the given station
     * @return the default MCS to use to transmit frames to the given station
     */
    WifiMode GetDefaultModeForSta(const WifiRemoteStation* st) const;
    /**
     * Return the number of basic MCS index.
     *
     * @return the number of basic MCS index
     */
    uint8_t GetNBasicMcs() const;
    /**
     * Return the MCS at the given <i>list</i> index.
     *
     * @param i the position in the list
     *
     * @return the basic MCS at the given list index
     */
    WifiMode GetBasicMcs(uint8_t i) const;
    /**
     * Record the MCS index supported by the station.
     *
     * @param address the address of the station
     * @param mcs the WifiMode supported by the station
     */
    void AddSupportedMcs(Mac48Address address, WifiMode mcs);
    /**
     * Return the channel width supported by the station.
     *
     * @param address the address of the station
     *
     * @return the channel width supported by the station
     */
    MHz_u GetChannelWidthSupported(Mac48Address address) const;
    /**
     * Return whether the station supports HT/VHT short guard interval.
     *
     * @param address the address of the station
     *
     * @return true if the station supports HT/VHT short guard interval,
     *         false otherwise
     */
    bool GetShortGuardIntervalSupported(Mac48Address address) const;
    /**
     * Return the number of spatial streams supported by the station.
     *
     * @param address the address of the station
     *
     * @return the number of spatial streams supported by the station
     */
    uint8_t GetNumberOfSupportedStreams(Mac48Address address) const;
    /**
     * Return the number of MCS supported by the station.
     *
     * @param address the address of the station
     *
     * @return the number of MCS supported by the station
     */
    uint8_t GetNMcsSupported(Mac48Address address) const;
    /**
     * Return whether the station supports DSSS or not.
     *
     * @param address the address of the station
     *
     * @return true if DSSS is supported by the station,
     *         false otherwise
     */
    bool GetDsssSupported(const Mac48Address& address) const;
    /**
     * Return whether the station supports ERP OFDM or not.
     *
     * @param address the address of the station
     *
     * @return true if ERP OFDM is supported by the station,
     *         false otherwise
     */
    bool GetErpOfdmSupported(const Mac48Address& address) const;
    /**
     * Return whether the station supports OFDM or not.
     *
     * @param address the address of the station
     *
     * @return true if OFDM is supported by the station,
     *         false otherwise
     */
    bool GetOfdmSupported(const Mac48Address& address) const;
    /**
     * Return whether the station supports HT or not.
     *
     * @param address the address of the station
     *
     * @return true if HT is supported by the station,
     *         false otherwise
     */
    bool GetHtSupported(Mac48Address address) const;
    /**
     * Return whether the station supports VHT or not.
     *
     * @param address the address of the station
     *
     * @return true if VHT is supported by the station,
     *         false otherwise
     */
    bool GetVhtSupported(Mac48Address address) const;
    /**
     * Return whether the station supports HE or not.
     *
     * @param address the address of the station
     *
     * @return true if HE is supported by the station,
     *         false otherwise
     */
    bool GetHeSupported(Mac48Address address) const;
    /**
     * Return whether the station supports EHT or not.
     *
     * @param address the address of the station
     *
     * @return true if EHT is supported by the station,
     *         false otherwise
     */
    bool GetEhtSupported(Mac48Address address) const;
    /**
     * @param address the (MLD or link) address of the non-AP MLD
     * @return whether the non-AP MLD supports EMLSR
     */
    bool GetEmlsrSupported(const Mac48Address& address) const;
    /**
     * @param address the (MLD or link) address of the non-AP MLD
     * @return whether EMLSR mode is enabled for the non-AP MLD on this link
     */
    bool GetEmlsrEnabled(const Mac48Address& address) const;

    /**
     * Return a mode for non-unicast packets.
     *
     * @return WifiMode for non-unicast packets
     */
    WifiMode GetNonUnicastMode() const;

    /**
     * Return the TXVECTOR to use for a groupcast packet.
     *
     * @param header the MAC header of the groupcast packet
     * @param allowedWidth the allowed width in MHz to send this packet
     * @return the TXVECTOR to use to send the groupcast packet
     */
    WifiTxVector GetGroupcastTxVector(const WifiMacHeader& header, MHz_u allowedWidth);

    /**
     * Invoked in a STA or AP to store the set of
     * modes supported by a destination which is
     * also supported locally.
     * The set of supported modes includes
     * the BSSBasicRateSet.
     *
     * @param address the address of the station being recorded
     * @param mode the WifiMode supports by the station
     */
    void AddSupportedMode(Mac48Address address, WifiMode mode);
    /**
     * Invoked in a STA or AP to store all of the modes supported
     * by a destination which is also supported locally.
     * The set of supported modes includes the BSSBasicRateSet.
     *
     * @param address the address of the station being recorded
     */
    void AddAllSupportedModes(Mac48Address address);
    /**
     * Invoked in a STA or AP to store all of the MCS supported
     * by a destination which is also supported locally.
     *
     * @param address the address of the station being recorded
     */
    void AddAllSupportedMcs(Mac48Address address);
    /**
     * Invoked in a STA or AP to delete all of the supported MCS by a destination.
     *
     * @param address the address of the station being recorded
     */
    void RemoveAllSupportedMcs(Mac48Address address);
    /**
     * Record whether the short PHY preamble is supported by the station.
     *
     * @param address the address of the station
     * @param isShortPreambleSupported whether or not short PHY preamble is supported by the station
     */
    void AddSupportedPhyPreamble(Mac48Address address, bool isShortPreambleSupported);
    /**
     * Record whether the short ERP slot time is supported by the station.
     *
     * @param address the address of the station
     * @param isShortSlotTimeSupported whether or not short ERP slot time is supported by the
     * station
     */
    void AddSupportedErpSlotTime(Mac48Address address, bool isShortSlotTimeSupported);
    /**
     * Return whether the station state is brand new.
     *
     * @param address the address of the station
     *
     * @return true if the state of the station is brand new,
     *         false otherwise
     */
    bool IsBrandNew(Mac48Address address) const;
    /**
     * Return whether the station associated.
     *
     * @param address the address of the station
     *
     * @return true if the station is associated,
     *         false otherwise
     */
    bool IsAssociated(Mac48Address address) const;
    /**
     * Return whether we are waiting for an ACK for
     * the association response we sent.
     *
     * @param address the address of the station
     *
     * @return true if the station is associated,
     *         false otherwise
     */
    bool IsWaitAssocTxOk(Mac48Address address) const;
    /**
     * Records that we are waiting for an ACK for
     * the association response we sent.
     *
     * @param address the address of the station
     */
    void RecordWaitAssocTxOk(Mac48Address address);
    /**
     * Records that we got an ACK for
     * the association response we sent.
     *
     * @param address the address of the station
     */
    void RecordGotAssocTxOk(Mac48Address address);
    /**
     * Records that we missed an ACK for
     * the association response we sent.
     *
     * @param address the address of the station
     */
    void RecordGotAssocTxFailed(Mac48Address address);
    /**
     * Records that the STA was disassociated.
     *
     * @param address the address of the station
     */
    void RecordDisassociated(Mac48Address address);
    /**
     * Return whether we refused an association request from the given station
     *
     * @param address the address of the station
     * @return true if we refused an association request, false otherwise
     */
    bool IsAssocRefused(Mac48Address address) const;
    /**
     * Records that association request was refused
     *
     * @param address the address of the station
     */
    void RecordAssocRefused(Mac48Address address);

    /**
     * Return whether the STA is currently in Power Save mode.
     *
     * @param address the address of the station
     *
     * @return true if the station is in Power Save mode, false otherwise
     */
    bool IsInPsMode(const Mac48Address& address) const;
    /**
     * Register whether the STA is in Power Save mode or not.
     *
     * @param address the address of the station
     * @param isInPsMode whether the STA is in PS mode or not
     */
    void SetPsMode(const Mac48Address& address, bool isInPsMode);

    /**
     * Get the address of the MLD the given station is affiliated with, if any.
     * Note that an MLD address is only present if an ML discovery/setup was performed
     * with the given station (which requires both this station and the given
     * station to be MLDs).
     *
     * @param address the MAC address of the remote station
     * @return the address of the MLD the given station is affiliated with, if any
     */
    std::optional<Mac48Address> GetMldAddress(const Mac48Address& address) const;
    /**
     * Get the address of the remote station operating on this link and affiliated
     * with the MLD having the given MAC address, if any.
     *
     * @param mldAddress the MLD MAC address
     * @return the address of the remote station operating on this link and
     *         affiliated with the MLD, if any
     */
    std::optional<Mac48Address> GetAffiliatedStaAddress(const Mac48Address& mldAddress) const;

    /**
     * @param header MAC header
     * @param allowedWidth the allowed width to send this packet
     * @return the TXVECTOR to use to send this packet
     */
    WifiTxVector GetDataTxVector(const WifiMacHeader& header, MHz_u allowedWidth);
    /**
     * @param address remote address
     * @param allowedWidth the allowed width for the data frame being protected
     *
     * @return the TXVECTOR to use to send the RTS prior to the
     *         transmission of the data packet itself.
     */
    WifiTxVector GetRtsTxVector(Mac48Address address, MHz_u allowedWidth);
    /**
     * Return a TXVECTOR for the CTS frame given the destination and the mode of the RTS
     * used by the sender.
     *
     * @param to the MAC address of the CTS receiver
     * @param rtsTxMode the mode of the RTS used by the sender
     * @return TXVECTOR for the CTS
     */
    WifiTxVector GetCtsTxVector(Mac48Address to, WifiMode rtsTxMode) const;
    /**
     * Since CTS-to-self parameters are not dependent on the station,
     * it is implemented in wifi remote station manager
     *
     * @return the transmission mode to use to send the CTS-to-self prior to the
     *         transmission of the data packet itself.
     */
    WifiTxVector GetCtsToSelfTxVector();
    /**
     * Adjust the TXVECTOR for an initial Control frame to ensure that the modulation class
     * is non-HT and the rate is 6 Mbps, 12 Mbps or 24 Mbps.
     *
     * @param txVector the TXVECTOR to adjust
     */
    void AdjustTxVectorForIcf(WifiTxVector& txVector) const;
    /**
     * Return a TXVECTOR for the Ack frame given the destination and the mode of the Data
     * used by the sender.
     *
     * @param to the MAC address of the Ack receiver
     * @param dataTxVector the TXVECTOR of the Data used by the sender
     * @return TXVECTOR for the Ack
     */
    WifiTxVector GetAckTxVector(Mac48Address to, const WifiTxVector& dataTxVector) const;
    /**
     * Return a TXVECTOR for the BlockAck frame given the destination and the mode of the Data
     * used by the sender.
     *
     * @param to the MAC address of the BlockAck receiver
     * @param dataTxVector the TXVECTOR of the Data used by the sender
     * @return TXVECTOR for the BlockAck
     */
    WifiTxVector GetBlockAckTxVector(Mac48Address to, const WifiTxVector& dataTxVector) const;
    /**
     * Get control answer mode function.
     *
     * @param reqMode request mode
     * @return control answer mode
     */
    WifiMode GetControlAnswerMode(WifiMode reqMode) const;

    /**
     * Should be invoked whenever the RtsTimeout associated to a transmission
     * attempt expires.
     *
     * @param header MAC header of the DATA packet
     */
    void ReportRtsFailed(const WifiMacHeader& header);
    /**
     * Should be invoked whenever the AckTimeout associated to a transmission
     * attempt expires.
     *
     * @param mpdu the MPDU whose transmission failed
     */
    void ReportDataFailed(Ptr<const WifiMpdu> mpdu);
    /**
     * Should be invoked whenever we receive the CTS associated to an RTS
     * we just sent. Note that we also get the SNR of the RTS we sent since
     * the receiver put a SnrTag in the CTS.
     *
     * @param header MAC header of the DATA packet
     * @param ctsSnr the SNR of the CTS we received
     * @param ctsMode the WifiMode the receiver used to send the CTS
     * @param rtsSnr the SNR of the RTS we sent
     */
    void ReportRtsOk(const WifiMacHeader& header, double ctsSnr, WifiMode ctsMode, double rtsSnr);
    /**
     * Should be invoked whenever we receive the ACK associated to a data packet
     * we just sent.
     *
     * @param mpdu the MPDU
     * @param ackSnr the SNR of the ACK we received
     * @param ackMode the WifiMode the receiver used to send the ACK
     * @param dataSnr the SNR of the DATA we sent
     * @param dataTxVector the TXVECTOR of the DATA we sent
     */
    void ReportDataOk(Ptr<const WifiMpdu> mpdu,
                      double ackSnr,
                      WifiMode ackMode,
                      double dataSnr,
                      WifiTxVector dataTxVector);
    /**
     * Should be invoked after calling ReportRtsFailed if frames are dropped
     *
     * @param header MAC header of the DATA packet
     */
    void ReportFinalRtsFailed(const WifiMacHeader& header);
    /**
     * Should be invoked after calling ReportDataFailed if frames are dropped
     *
     * @param mpdu the MPDU which was discarded
     */
    void ReportFinalDataFailed(Ptr<const WifiMpdu> mpdu);
    /**
     * Typically called per A-MPDU, either when a Block ACK was successfully
     * received or when a BlockAckTimeout has elapsed.
     *
     * @param address the address of the receiver
     * @param nSuccessfulMpdus number of successfully transmitted MPDUs
     * A value of 0 means that the Block ACK was missed.
     * @param nFailedMpdus number of unsuccessfuly transmitted MPDUs
     * @param rxSnr received SNR of the block ack frame itself
     * @param dataSnr data SNR reported by remote station
     * @param dataTxVector the TXVECTOR of the MPDUs we sent
     */
    void ReportAmpduTxStatus(Mac48Address address,
                             uint16_t nSuccessfulMpdus,
                             uint16_t nFailedMpdus,
                             double rxSnr,
                             double dataSnr,
                             WifiTxVector dataTxVector);

    /**
     * @param address remote address
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the TXVECTOR used for the packet received
     *
     * Should be invoked whenever a packet is successfully received.
     */
    void ReportRxOk(Mac48Address address, RxSignalInfo rxSignalInfo, const WifiTxVector& txVector);

    /**
     * Increment the retry count for all the MPDUs (if needed) in the given PSDU and find the
     * MPDUs to drop based on the frame retry count.
     *
     * @param psdu the given PSDU, whose transmission failed
     * @return the list of MPDUs that have to be dropped
     */
    std::list<Ptr<WifiMpdu>> GetMpdusToDropOnTxFailure(Ptr<WifiPsdu> psdu);

    /**
     * @param header MAC header of the data frame to send
     * @param txParams the TX parameters for the data frame to send
     *
     * @return true if we want to use an RTS/CTS handshake for this
     *         frame before sending it, false otherwise.
     */
    bool NeedRts(const WifiMacHeader& header, const WifiTxParameters& txParams);
    /**
     * Return if we need to do CTS-to-self before sending a DATA.
     *
     * @param txVector the TXVECTOR of the DATA
     * @param header the MAC header of the DATA
     * @return true if CTS-to-self is needed,
     *         false otherwise
     */
    bool NeedCtsToSelf(const WifiTxVector& txVector, const WifiMacHeader& header);

    /**
     * @param mpdu the MPDU to send
     *
     * @return true if this packet should be fragmented,
     *         false otherwise.
     */
    bool NeedFragmentation(Ptr<const WifiMpdu> mpdu);
    /**
     * @param mpdu the MPDU to send
     * @param fragmentNumber the fragment index of the next fragment to send (starts at zero).
     *
     * @return the size of the corresponding fragment.
     */
    uint32_t GetFragmentSize(Ptr<const WifiMpdu> mpdu, uint32_t fragmentNumber);
    /**
     * @param mpdu the packet to send
     * @param fragmentNumber the fragment index of the next fragment to send (starts at zero).
     *
     * @return the offset within the original packet where this fragment starts.
     */
    uint32_t GetFragmentOffset(Ptr<const WifiMpdu> mpdu, uint32_t fragmentNumber);
    /**
     * @param mpdu the packet to send
     * @param fragmentNumber the fragment index of the next fragment to send (starts at zero).
     *
     * @return true if this is the last fragment, false otherwise.
     */
    bool IsLastFragment(Ptr<const WifiMpdu> mpdu, uint32_t fragmentNumber);

    /**
     * @return the default transmission power
     */
    uint8_t GetDefaultTxPowerLevel() const;
    /**
     * @param address of the remote station
     *
     * @return information regarding the remote station associated with the given address
     */
    WifiRemoteStationInfo GetInfo(Mac48Address address);
    /**
     * @param address of the remote station
     *
     * @return the RSSI of the most recent packet received from the remote station (irrespective of
     * TID)
     *
     * This method is typically used when the device needs
     * to estimate the target UL RSSI info to put in the
     * Trigger frame to send to the remote station.
     */
    std::optional<dBm_u> GetMostRecentRssi(Mac48Address address) const;
    /**
     * Set the default transmission power level
     *
     * @param txPower the default transmission power level
     */
    void SetDefaultTxPowerLevel(uint8_t txPower);
    /**
     * @return the number of antennas supported by the PHY layer
     */
    uint8_t GetNumberOfAntennas() const;
    /**
     * @return the maximum number of spatial streams supported by the PHY layer
     */
    uint8_t GetMaxNumberOfTransmitStreams() const;
    /**
     * @returns whether LDPC should be used for a given destination address.
     *
     * @param dest the destination address
     *
     * @return whether LDPC should be used for a given destination address
     */
    bool UseLdpcForDestination(Mac48Address dest) const;

    /**
     * TracedCallback signature for power change events.
     *
     * @param [in] oldPower The previous power (in dBm).
     * @param [in] newPower The new power (in dBm).
     * @param [in] address The remote station MAC address.
     */
    typedef void (*PowerChangeTracedCallback)(double oldPower,
                                              double newPower,
                                              Mac48Address remoteAddress);

    /**
     * TracedCallback signature for rate change events.
     *
     * @param [in] oldRate The previous data rate.
     * @param [in] newRate The new data rate.
     * @param [in] address The remote station MAC address.
     */
    typedef void (*RateChangeTracedCallback)(DataRate oldRate,
                                             DataRate newRate,
                                             Mac48Address remoteAddress);

    /**
     * Return the WifiPhy.
     *
     * @return a pointer to the WifiPhy
     */
    Ptr<WifiPhy> GetPhy() const;
    /**
     * Return the WifiMac.
     *
     * @return a pointer to the WifiMac
     */
    Ptr<WifiMac> GetMac() const;

  protected:
    void DoDispose() override;
    /**
     * Return whether mode associated with the specified station at the specified index.
     *
     * @param station the station being queried
     * @param i the index
     *
     * @return WifiMode at the given index of the specified station
     */
    WifiMode GetSupported(const WifiRemoteStation* station, uint8_t i) const;
    /**
     * Return the number of modes supported by the given station.
     *
     * @param station the station being queried
     *
     * @return the number of modes supported by the given station
     */
    uint8_t GetNSupported(const WifiRemoteStation* station) const;
    /**
     * Return whether the given station is QoS capable.
     *
     * @param station the station being queried
     *
     * @return true if the station has QoS capabilities,
     *         false otherwise
     */
    bool GetQosSupported(const WifiRemoteStation* station) const;
    /**
     * Return whether the given station is HT capable.
     *
     * @param station the station being queried
     *
     * @return true if the station has HT capabilities,
     *         false otherwise
     */
    bool GetHtSupported(const WifiRemoteStation* station) const;
    /**
     * Return whether the given station is VHT capable.
     *
     * @param station the station being queried
     *
     * @return true if the station has VHT capabilities,
     *         false otherwise
     */
    bool GetVhtSupported(const WifiRemoteStation* station) const;
    /**
     * Return whether the given station is HE capable.
     *
     * @param station the station being queried
     *
     * @return true if the station has HE capabilities,
     *         false otherwise
     */
    bool GetHeSupported(const WifiRemoteStation* station) const;
    /**
     * Return whether the given station is EHT capable.
     *
     * @param station the station being queried
     *
     * @return true if the station has EHT capabilities,
     *         false otherwise
     */
    bool GetEhtSupported(const WifiRemoteStation* station) const;
    /**
     * @param station the station of a non-AP MLD
     * @return whether the non-AP MLD supports EMLSR
     */
    bool GetEmlsrSupported(const WifiRemoteStation* station) const;
    /**
     * @param station the station of a non-AP MLD
     * @return whether EMLSR mode is enabled for the non-AP MLD on this link
     */
    bool GetEmlsrEnabled(const WifiRemoteStation* station) const;
    /**
     * Return the WifiMode supported by the specified station at the specified index.
     *
     * @param station the station being queried
     * @param i the index
     *
     * @return the WifiMode at the given index of the specified station
     */

    WifiMode GetMcsSupported(const WifiRemoteStation* station, uint8_t i) const;
    /**
     * Return the number of MCS supported by the given station.
     *
     * @param station the station being queried
     *
     * @return the number of MCS supported by the given station
     */
    uint8_t GetNMcsSupported(const WifiRemoteStation* station) const;
    /**
     * Return whether non-ERP mode associated with the specified station at the specified index.
     *
     * @param station the station being queried
     * @param i the index
     *
     * @return WifiMode at the given index of the specified station
     */
    WifiMode GetNonErpSupported(const WifiRemoteStation* station, uint8_t i) const;
    /**
     * Return the number of non-ERP modes supported by the given station.
     *
     * @param station the station being queried
     *
     * @return the number of non-ERP modes supported by the given station
     */
    uint32_t GetNNonErpSupported(const WifiRemoteStation* station) const;
    /**
     * Return the address of the station.
     *
     * @param station the station being queried
     *
     * @return the address of the station
     */
    Mac48Address GetAddress(const WifiRemoteStation* station) const;
    /**
     * Return the channel width supported by the station.
     *
     * @param station the station being queried
     *
     * @return the channel width supported by the station
     */
    MHz_u GetChannelWidth(const WifiRemoteStation* station) const;
    /**
     * Return whether the given station supports HT/VHT short guard interval.
     *
     * @param station the station being queried
     *
     * @return true if the station supports HT/VHT short guard interval,
     *         false otherwise
     */
    bool GetShortGuardIntervalSupported(const WifiRemoteStation* station) const;
    /**
     * Return the shortest HE guard interval duration supported by the station.
     *
     * @param station the station being queried
     *
     * @return the shortest HE guard interval duration supported by the station
     */
    Time GetGuardInterval(const WifiRemoteStation* station) const;
    /**
     * Return whether the given station supports A-MPDU.
     *
     * @param station the station being queried
     *
     * @return true if the station supports MPDU aggregation,
     *         false otherwise
     */
    bool GetAggregation(const WifiRemoteStation* station) const;

    /**
     * Return the number of supported streams the station has.
     *
     * @param station the station being queried
     *
     * @return the number of supported streams the station has
     */
    uint8_t GetNumberOfSupportedStreams(const WifiRemoteStation* station) const;
    /**
     * @returns the number of Ness the station has.
     *
     * @param station the station being queried
     *
     * @return the number of Ness the station has
     */
    uint8_t GetNess(const WifiRemoteStation* station) const;

    uint8_t m_linkId;             //!< the ID of the link this object is associated with
    bool m_incrRetryCountUnderBa; //!< whether  to increment the retry count of frames that are
                                  //!< part of a Block Ack agreement

  private:
    /**
     * If the given TXVECTOR is used for a MU transmission, return the STAID of
     * the station with the given address if we are an AP or our own STAID if we
     * are a STA associated with some AP. Otherwise, return SU_STA_ID.
     *
     * @param address the address of the station
     * @param txVector the TXVECTOR used for a MU transmission
     * @return the STA-ID of the station
     */
    uint16_t GetStaId(Mac48Address address, const WifiTxVector& txVector) const;

    /**
     * Increment the retry count (if needed) for the given PSDU, whose transmission failed.
     *
     * @param station the station the PSDU is addressed to
     * @param psdu the given PSDU
     */
    virtual void DoIncrementRetryCountOnTxFailure(WifiRemoteStation* station, Ptr<WifiPsdu> psdu);

    /**
     * Find the MPDUs to drop (possibly based on their frame retry count) in the given PSDU,
     * whose transmission failed.
     *
     * @param station the station the PSDU is addressed to
     * @param psdu the given PSDU
     * @return the MPDUs in the PSDU to drop
     */
    virtual std::list<Ptr<WifiMpdu>> DoGetMpdusToDropOnTxFailure(WifiRemoteStation* station,
                                                                 Ptr<WifiPsdu> psdu);

    /**
     * @param station the station that we need to communicate
     * @param size the size of the frame to send in bytes
     * @param normally indicates whether the normal 802.11 RTS enable mechanism would
     *        request that the RTS is sent or not.
     *
     * @return true if we want to use an RTS/CTS handshake for this frame before sending it,
     *         false otherwise.
     *
     * Note: This method is called before a unicast packet is sent on the medium.
     */
    virtual bool DoNeedRts(WifiRemoteStation* station, uint32_t size, bool normally);
    /**
     * @param station the station that we need to communicate
     * @param packet the packet to send
     * @param normally indicates whether the normal 802.11 data fragmentation mechanism
     *        would request that the data packet is fragmented or not.
     *
     * @return true if this packet should be fragmented,
     *         false otherwise.
     *
     * Note: This method is called before sending a unicast packet.
     */
    virtual bool DoNeedFragmentation(WifiRemoteStation* station,
                                     Ptr<const Packet> packet,
                                     bool normally);
    /**
     * @return a new station data structure
     */
    virtual WifiRemoteStation* DoCreateStation() const = 0;
    /**
     * @param station the station that we need to communicate
     * @param allowedWidth the allowed width to send a packet to the station
     * @return the TXVECTOR to use to send a packet to the station
     *
     * Note: This method is called before sending a unicast packet or a fragment
     *       of a unicast packet to decide which transmission mode to use.
     */
    virtual WifiTxVector DoGetDataTxVector(WifiRemoteStation* station, MHz_u allowedWidth) = 0;
    /**
     * @param station the station that we need to communicate
     *
     * @return the transmission mode to use to send an RTS to the station
     *
     * Note: This method is called before sending an RTS to a station
     *       to decide which transmission mode to use for the RTS.
     */
    virtual WifiTxVector DoGetRtsTxVector(WifiRemoteStation* station) = 0;

    /**
     * This method is a pure virtual method that must be implemented by the sub-class.
     * This allows different types of WifiRemoteStationManager to respond differently,
     *
     * @param station the station that we failed to send RTS
     */
    virtual void DoReportRtsFailed(WifiRemoteStation* station) = 0;
    /**
     * This method is a pure virtual method that must be implemented by the sub-class.
     * This allows different types of WifiRemoteStationManager to respond differently,
     *
     * @param station the station that we failed to send DATA
     */
    virtual void DoReportDataFailed(WifiRemoteStation* station) = 0;
    /**
     * This method is a pure virtual method that must be implemented by the sub-class.
     * This allows different types of WifiRemoteStationManager to respond differently,
     *
     * @param station the station that we successfully sent RTS
     * @param ctsSnr the SNR of the CTS we received
     * @param ctsMode the WifiMode the receiver used to send the CTS
     * @param rtsSnr the SNR of the RTS we sent
     */
    virtual void DoReportRtsOk(WifiRemoteStation* station,
                               double ctsSnr,
                               WifiMode ctsMode,
                               double rtsSnr) = 0;
    /**
     * This method is a pure virtual method that must be implemented by the sub-class.
     * This allows different types of WifiRemoteStationManager to respond differently,
     *
     * @param station the station that we successfully sent RTS
     * @param ackSnr the SNR of the ACK we received
     * @param ackMode the WifiMode the receiver used to send the ACK
     * @param dataSnr the SNR of the DATA we sent
     * @param dataChannelWidth the channel width of the DATA we sent
     * @param dataNss the number of spatial streams used to send the DATA
     */
    virtual void DoReportDataOk(WifiRemoteStation* station,
                                double ackSnr,
                                WifiMode ackMode,
                                double dataSnr,
                                MHz_u dataChannelWidth,
                                uint8_t dataNss) = 0;
    /**
     * This method is a pure virtual method that must be implemented by the sub-class.
     * This allows different types of WifiRemoteStationManager to respond differently,
     *
     * @param station the station that we failed to send RTS
     */
    virtual void DoReportFinalRtsFailed(WifiRemoteStation* station) = 0;
    /**
     * This method is a pure virtual method that must be implemented by the sub-class.
     * This allows different types of WifiRemoteStationManager to respond differently,
     *
     * @param station the station that we failed to send DATA
     */
    virtual void DoReportFinalDataFailed(WifiRemoteStation* station) = 0;
    /**
     * This method is a pure virtual method that must be implemented by the sub-class.
     * This allows different types of WifiRemoteStationManager to respond differently,
     *
     * @param station the station that sent the DATA to us
     * @param rxSnr the SNR of the DATA we received
     * @param txMode the WifiMode the sender used to send the DATA
     */
    virtual void DoReportRxOk(WifiRemoteStation* station, double rxSnr, WifiMode txMode) = 0;
    /**
     * Typically called per A-MPDU, either when a Block ACK was successfully received
     * or when a BlockAckTimeout has elapsed. This method is a virtual method that must
     * be implemented by the sub-class intended to handle A-MPDUs. This allows different
     * types of WifiRemoteStationManager to respond differently.
     *
     * @param station the station that sent the DATA to us
     * @param nSuccessfulMpdus number of successfully transmitted MPDUs.
     *        A value of 0 means that the Block ACK was missed.
     * @param nFailedMpdus number of unsuccessfuly transmitted MPDUs.
     * @param rxSnr received SNR of the block ack frame itself
     * @param dataSnr data SNR reported by remote station
     * @param dataChannelWidth the channel width of the A-MPDU we sent
     * @param dataNss the number of spatial streams used to send the A-MPDU
     */
    virtual void DoReportAmpduTxStatus(WifiRemoteStation* station,
                                       uint16_t nSuccessfulMpdus,
                                       uint16_t nFailedMpdus,
                                       double rxSnr,
                                       double dataSnr,
                                       MHz_u dataChannelWidth,
                                       uint8_t dataNss);

    /**
     * Return the state of the station associated with the given address.
     *
     * @param address the address of the station
     * @return WifiRemoteStationState corresponding to the address
     */
    std::shared_ptr<WifiRemoteStationState> LookupState(Mac48Address address) const;
    /**
     * Return the station associated with the given address.
     *
     * @param address the address of the station
     *
     * @return WifiRemoteStation corresponding to the address
     */
    WifiRemoteStation* Lookup(Mac48Address address) const;

    /**
     * Actually sets the fragmentation threshold, it also checks the validity of
     * the given threshold.
     *
     * @param threshold the fragmentation threshold
     */
    void DoSetFragmentationThreshold(uint32_t threshold);
    /**
     * Return the current fragmentation threshold
     *
     * @return the fragmentation threshold
     */
    uint32_t DoGetFragmentationThreshold() const;
    /**
     * Return the number of fragments needed for the given packet.
     *
     * @param mpdu the packet to be fragmented
     *
     * @return the number of fragments needed
     */
    uint32_t GetNFragments(Ptr<const WifiMpdu> mpdu);

    /**
     * This is a pointer to the WifiPhy associated with this
     * WifiRemoteStationManager that is set on call to
     * WifiRemoteStationManager::SetupPhy(). Through this pointer the
     * station manager can determine PHY characteristics, such as the
     * set of all transmission rates that may be supported (the
     * "DeviceRateSet").
     */
    Ptr<WifiPhy> m_wifiPhy;
    /**
     * This is a pointer to the WifiMac associated with this
     * WifiRemoteStationManager that is set on call to
     * WifiRemoteStationManager::SetupMac(). Through this pointer the
     * station manager can determine MAC characteristics, such as the
     * interframe spaces.
     */
    Ptr<WifiMac> m_wifiMac;

    /**
     * This member is the list of WifiMode objects that comprise the
     * BSSBasicRateSet parameter. This list is constructed through calls
     * to WifiRemoteStationManager::AddBasicMode(), and an API that
     * allows external access to it is available through
     * WifiRemoteStationManager::GetNBasicModes() and
     * WifiRemoteStationManager::GetBasicMode().
     */
    WifiModeList m_bssBasicRateSet; //!< basic rate set
    WifiModeList m_bssBasicMcsSet;  //!< basic MCS set

    StationStates m_states; //!< States of known stations
    Stations m_stations;    //!< Information for each known stations

    uint32_t m_maxSsrc;                //!< Maximum STA short retry count (SSRC)
    uint32_t m_maxSlrc;                //!< Maximum STA long retry count (SLRC)
    uint32_t m_rtsCtsThreshold;        //!< Threshold for RTS/CTS
    Time m_rtsCtsTxDurationThresh;     //!< TX duration threshold for RTS/CTS
    uint32_t m_fragmentationThreshold; //!< Current threshold for fragmentation
    uint8_t m_defaultTxPowerLevel;     //!< Default transmission power level
    WifiMode m_nonUnicastMode;         //!< Transmission mode for non-unicast Data frames
    bool m_useNonErpProtection;        //!< flag if protection for non-ERP stations against ERP
                                       //!< transmissions is enabled
    bool m_useNonHtProtection; //!< flag if protection for non-HT stations against HT transmissions
                               //!< is enabled
    bool m_shortPreambleEnabled;        //!< flag if short PHY preamble is enabled
    bool m_shortSlotTimeEnabled;        //!< flag if short slot time is enabled
    ProtectionMode m_erpProtectionMode; //!< Protection mode for ERP stations when non-ERP stations
                                        //!< are detected
    ProtectionMode
        m_htProtectionMode; //!< Protection mode for HT stations when non-HT stations are detected

    std::array<uint32_t, AC_BE_NQOS> m_ssrc; //!< short retry count per AC
    std::array<uint32_t, AC_BE_NQOS> m_slrc; //!< long retry count per AC

    /**
     * The trace source fired when the transmission of a single RTS has failed
     */
    TracedCallback<Mac48Address> m_macTxRtsFailed;
    /**
     * The trace source fired when the transmission of a single data packet has failed
     */
    TracedCallback<Mac48Address> m_macTxDataFailed;
    /**
     * The trace source fired when the transmission of a RTS has
     * exceeded the maximum number of attempts
     */
    TracedCallback<Mac48Address> m_macTxFinalRtsFailed;
    /**
     * The trace source fired when the transmission of a data packet has
     * exceeded the maximum number of attempts
     */
    TracedCallback<Mac48Address> m_macTxFinalDataFailed;
};

} // namespace ns3

#endif /* WIFI_REMOTE_STATION_MANAGER_H */
