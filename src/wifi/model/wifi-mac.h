/*
 * Copyright (c) 2008 INRIA
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

#ifndef WIFI_MAC_H
#define WIFI_MAC_H

#include "qos-utils.h"
#include "ssid.h"
#include "wifi-remote-station-manager.h"
#include "wifi-standards.h"

#include <list>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

namespace ns3
{

class Txop;
class WifiNetDevice;
class QosTxop;
class WifiPsdu;
class MacRxMiddle;
class MacTxMiddle;
class WifiMacQueue;
class WifiMpdu;
class HtConfiguration;
class VhtConfiguration;
class HeConfiguration;
class EhtConfiguration;
class FrameExchangeManager;
class ChannelAccessManager;
class ExtendedCapabilities;
class WifiMacQueueScheduler;
class OriginatorBlockAckAgreement;
class RecipientBlockAckAgreement;

/**
 * Enumeration for type of station
 */
enum TypeOfStation
{
    STA,
    AP,
    ADHOC_STA,
    MESH,
    OCB
};

/**
 * \ingroup wifi
 * \enum WifiMacDropReason
 * \brief The reason why an MPDU was dropped
 */
enum WifiMacDropReason : uint8_t
{
    WIFI_MAC_DROP_FAILED_ENQUEUE = 0,
    WIFI_MAC_DROP_EXPIRED_LIFETIME,
    WIFI_MAC_DROP_REACHED_RETRY_LIMIT,
    WIFI_MAC_DROP_QOS_OLD_PACKET
};

typedef std::unordered_map<uint16_t /* staId */, Ptr<WifiPsdu> /* PSDU */> WifiPsduMap;

/**
 * \brief base class for all MAC-level wifi objects.
 * \ingroup wifi
 *
 * This class encapsulates all the low-level MAC functionality
 * DCA, EDCA, etc) and all the high-level MAC functionality
 * (association/disassociation state machines).
 *
 */
class WifiMac : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    WifiMac();
    ~WifiMac() override;

    // Delete copy constructor and assignment operator to avoid misuse
    WifiMac(const WifiMac&) = delete;
    WifiMac& operator=(const WifiMac&) = delete;

    /**
     * Sets the device this PHY is associated with.
     *
     * \param device the device this PHY is associated with
     */
    void SetDevice(const Ptr<WifiNetDevice> device);
    /**
     * Return the device this PHY is associated with
     *
     * \return the device this PHY is associated with
     */
    Ptr<WifiNetDevice> GetDevice() const;

    /**
     * Get the Frame Exchange Manager associated with the given link
     *
     * \param linkId the ID of the given link
     * \return the Frame Exchange Manager
     */
    Ptr<FrameExchangeManager> GetFrameExchangeManager(uint8_t linkId = SINGLE_LINK_OP_ID) const;

    /**
     * Get the Channel Access Manager associated with the given link
     *
     * \param linkId the ID of the given link
     * \return the Channel Access Manager
     */
    Ptr<ChannelAccessManager> GetChannelAccessManager(uint8_t linkId = SINGLE_LINK_OP_ID) const;

    /**
     * Get the number of links (can be greater than 1 for 11be devices only).
     *
     * \return the number of links used by this MAC
     */
    uint8_t GetNLinks() const;
    /**
     * Get the ID of the link having the given MAC address, if any.
     *
     * \param address the given MAC address
     * \return the ID of the link having the given MAC address, if any
     */
    virtual std::optional<uint8_t> GetLinkIdByAddress(const Mac48Address& address) const;

    /**
     * \param remoteAddr the (MLD or link) address of a remote device
     * \return the MLD address of the remote device having the given (MLD or link) address, if
     *         the remote device is an MLD.
     */
    std::optional<Mac48Address> GetMldAddress(const Mac48Address& remoteAddr) const;

    /**
     * Get the local MAC address used to communicate with a remote STA. Specifically:
     * - If the given remote address is the address of a STA affiliated with a remote MLD
     * and operating on a setup link, the address of the local STA operating on such a link
     * is returned.
     * - If the given remote address is the MLD address of a remote MLD (with which some link
     * has been setup), the MLD address of this device is returned.
     * - If this is a single link device, the unique MAC address of this device is returned.
     * - Otherwise, return the MAC address of the affiliated STA (which must exists) that
     * can be used to communicate with the remote device.
     *
     * \param remoteAddr the MAC address of the remote device
     * \return the local MAC address used to communicate with the remote device
     */
    Mac48Address GetLocalAddress(const Mac48Address& remoteAddr) const;

    /**
     * Accessor for the Txop object
     *
     * \return a smart pointer to Txop
     */
    Ptr<Txop> GetTxop() const;
    /**
     * Accessor for a specified EDCA object
     *
     * \param ac the Access Category
     * \return a smart pointer to a QosTxop
     */
    Ptr<QosTxop> GetQosTxop(AcIndex ac) const;
    /**
     * Accessor for a specified EDCA object
     *
     * \param tid the Traffic ID
     * \return a smart pointer to a QosTxop
     */
    Ptr<QosTxop> GetQosTxop(uint8_t tid) const;
    /**
     * Get the wifi MAC queue of the (Qos)Txop associated with the given AC,
     * if such (Qos)Txop is installed, or a null pointer, otherwise.
     *
     * \param ac the given Access Category
     * \return the wifi MAC queue of the (Qos)Txop associated with the given AC,
     *         if such (Qos)Txop is installed, or a null pointer, otherwise
     */
    virtual Ptr<WifiMacQueue> GetTxopQueue(AcIndex ac) const;

    /**
     * Set the wifi MAC queue scheduler
     *
     * \param scheduler the wifi MAC queue scheduler
     */
    virtual void SetMacQueueScheduler(Ptr<WifiMacQueueScheduler> scheduler);
    /**
     * Get the wifi MAC queue scheduler
     *
     * \return the wifi MAC queue scheduler
     */
    Ptr<WifiMacQueueScheduler> GetMacQueueScheduler() const;

    /**
     * This method is invoked by a subclass to specify what type of
     * station it is implementing. This is something that the channel
     * access functions need to know.
     *
     * \param type the type of station.
     */
    void SetTypeOfStation(TypeOfStation type);
    /**
     * Return the type of station.
     *
     * \return the type of station.
     */
    TypeOfStation GetTypeOfStation() const;

    /**
     * \param ssid the current SSID of this MAC layer.
     */
    void SetSsid(Ssid ssid);
    /**
     * \brief Sets the interface in promiscuous mode.
     *
     * Enables promiscuous mode on the interface. Note that any further
     * filtering on the incoming frame path may affect the overall
     * behavior.
     */
    void SetPromisc();
    /**
     * Enable or disable CTS-to-self feature.
     *
     * \param enable true if CTS-to-self is to be supported,
     *               false otherwise
     */
    void SetCtsToSelfSupported(bool enable);

    /**
     * \return the MAC address associated to this MAC layer.
     */
    Mac48Address GetAddress() const;
    /**
     * \return the SSID which this MAC layer is going to try to stay in.
     */
    Ssid GetSsid() const;
    /**
     * \param address the current address of this MAC layer.
     */
    virtual void SetAddress(Mac48Address address);
    /**
     * \return the BSSID of the network the given link belongs to.
     * \param linkId the ID of the given link
     */
    Mac48Address GetBssid(uint8_t linkId) const;
    /**
     * \param bssid the BSSID of the network that the given link belongs to.
     * \param linkId the ID of the given link
     */
    void SetBssid(Mac48Address bssid, uint8_t linkId);

    /**
     * Return true if packets can be forwarded to the given destination,
     * false otherwise.
     *
     * \param to the address to which the packet should be sent
     * \return whether packets can be forwarded to the given destination
     */
    virtual bool CanForwardPacketsTo(Mac48Address to) const = 0;
    /**
     * \param packet the packet to send.
     * \param to the address to which the packet should be sent.
     * \param from the address from which the packet should be sent.
     *
     * The packet should be enqueued in a TX queue, and should be
     * dequeued as soon as the DCF function determines that
     * access it granted to this MAC. The extra parameter "from" allows
     * this device to operate in a bridged mode, forwarding received
     * frames without altering the source address.
     */
    virtual void Enqueue(Ptr<Packet> packet, Mac48Address to, Mac48Address from);
    /**
     * \param packet the packet to send.
     * \param to the address to which the packet should be sent.
     *
     * The packet should be enqueued in a TX queue, and should be
     * dequeued as soon as the DCF function determines that
     * access it granted to this MAC.
     */
    virtual void Enqueue(Ptr<Packet> packet, Mac48Address to) = 0;
    /**
     * \return if this MAC supports sending from arbitrary address.
     *
     * The interface may or may not support sending from arbitrary address.
     * This function returns true if sending from arbitrary address is supported,
     * false otherwise.
     */
    virtual bool SupportsSendFrom() const;

    /**
     * \param phys the physical layers attached to this MAC.
     */
    virtual void SetWifiPhys(const std::vector<Ptr<WifiPhy>>& phys);
    /**
     * \param linkId the index (starting at 0) of the PHY object to retrieve
     * \return the physical layer attached to this MAC
     */
    Ptr<WifiPhy> GetWifiPhy(uint8_t linkId = SINGLE_LINK_OP_ID) const;
    /**
     * Remove currently attached WifiPhy objects from this MAC.
     */
    void ResetWifiPhys();

    /**
     * \param stationManager the station manager attached to this MAC.
     */
    void SetWifiRemoteStationManager(Ptr<WifiRemoteStationManager> stationManager);
    /**
     * \param stationManagers the station managers attached to this MAC.
     */
    void SetWifiRemoteStationManagers(
        const std::vector<Ptr<WifiRemoteStationManager>>& stationManagers);
    /**
     * \param linkId the ID (starting at 0) of the link of the RemoteStationManager object
     * to retrieve
     * \return the remote station manager operating on the given link
     */
    Ptr<WifiRemoteStationManager> GetWifiRemoteStationManager(uint8_t linkId = 0) const;

    /**
     * This type defines the callback of a higher layer that a
     * WifiMac(-derived) object invokes to pass a packet up the stack.
     *
     * \param packet the packet that has been received.
     * \param from the MAC address of the device that sent the packet.
     * \param to the MAC address of the device that the packet is destined for.
     */
    typedef Callback<void, Ptr<const Packet>, Mac48Address, Mac48Address> ForwardUpCallback;

    /**
     * \param upCallback the callback to invoke when a packet must be
     *        forwarded up the stack.
     */
    void SetForwardUpCallback(ForwardUpCallback upCallback);
    /**
     * \param linkUp the callback to invoke when the link becomes up.
     */
    virtual void SetLinkUpCallback(Callback<void> linkUp);
    /**
     * \param linkDown the callback to invoke when the link becomes down.
     */
    void SetLinkDownCallback(Callback<void> linkDown);
    /* Next functions are not pure virtual so non QoS WifiMacs are not
     * forced to implement them.
     */

    /**
     * Notify that channel on the given link has been switched.
     *
     * \param linkId the ID of the given link
     */
    virtual void NotifyChannelSwitching(uint8_t linkId);

    /**
     * \param packet the packet being enqueued
     *
     * Public method used to fire a MacTx trace. Implemented for encapsulation purposes.
     * Note this trace indicates that the packet was accepted by the device only.
     * The packet may be dropped later (e.g. if the queue is full).
     */
    void NotifyTx(Ptr<const Packet> packet);
    /**
     * \param packet the packet being dropped
     *
     * Public method used to fire a MacTxDrop trace.
     * This trace indicates that the packet was dropped before it was queued for
     * transmission (e.g. when a STA is not associated with an AP).
     */
    void NotifyTxDrop(Ptr<const Packet> packet);
    /**
     * \param packet the packet we received
     *
     * Public method used to fire a MacRx trace. Implemented for encapsulation purposes.
     */
    void NotifyRx(Ptr<const Packet> packet);
    /**
     * \param packet the packet we received promiscuously
     *
     * Public method used to fire a MacPromiscRx trace. Implemented for encapsulation purposes.
     */
    void NotifyPromiscRx(Ptr<const Packet> packet);
    /**
     * \param packet the packet we received but is not destined for us
     *
     * Public method used to fire a MacRxDrop trace. Implemented for encapsulation purposes.
     */
    void NotifyRxDrop(Ptr<const Packet> packet);

    /**
     * \param standard the wifi standard to be configured
     *
     * This method completes the configuration process for a requested PHY standard
     * by creating the Frame Exchange Manager and the Channel Access Manager and
     * configuring the PHY dependent parameters.
     * This method can only be called after a configured PHY has been set.
     */
    virtual void ConfigureStandard(WifiStandard standard);

    /**
     * \return pointer to HtConfiguration if it exists
     */
    Ptr<HtConfiguration> GetHtConfiguration() const;
    /**
     * \return pointer to VhtConfiguration if it exists
     */
    Ptr<VhtConfiguration> GetVhtConfiguration() const;
    /**
     * \return pointer to HeConfiguration if it exists
     */
    Ptr<HeConfiguration> GetHeConfiguration() const;
    /**
     * \return pointer to EhtConfiguration if it exists
     */
    Ptr<EhtConfiguration> GetEhtConfiguration() const;

    /**
     * Return the extended capabilities of the device.
     *
     * \return the extended capabilities that we support
     */
    ExtendedCapabilities GetExtendedCapabilities() const;
    /**
     * Return the HT capabilities of the device for the given link.
     *
     * \param linkId the ID of the given link
     * \return the HT capabilities that we support
     */
    HtCapabilities GetHtCapabilities(uint8_t linkId) const;
    /**
     * Return the VHT capabilities of the device for the given link.
     *
     * \param linkId the ID of the given link
     * \return the VHT capabilities that we support
     */
    VhtCapabilities GetVhtCapabilities(uint8_t linkId) const;
    /**
     * Return the HE capabilities of the device for the given link.
     *
     * \param linkId the ID of the given link
     * \return the HE capabilities that we support
     */
    HeCapabilities GetHeCapabilities(uint8_t linkId) const;
    /**
     * Return the EHT capabilities of the device for the given link.
     *
     * \param linkId the ID of the given link
     * \return the EHT capabilities that we support
     */
    EhtCapabilities GetEhtCapabilities(uint8_t linkId) const;

    /**
     * Return whether the device supports QoS.
     *
     * \return true if QoS is supported, false otherwise
     */
    bool GetQosSupported() const;
    /**
     * Return whether the device supports ERP on the given link.
     *
     * \param linkId the ID of the given link
     * \return true if ERP is supported, false otherwise
     */
    bool GetErpSupported(uint8_t linkId) const;
    /**
     * Return whether the device supports DSSS on the given link.
     *
     * \param linkId the ID of the given link
     * \return true if DSSS is supported, false otherwise
     */
    bool GetDsssSupported(uint8_t linkId) const;
    /**
     * Return whether the device supports HT.
     *
     * \return true if HT is supported, false otherwise
     */
    bool GetHtSupported() const;
    /**
     * Return whether the device supports VHT on the given link.
     *
     * \param linkId the ID of the given link.
     * \return true if VHT is supported, false otherwise
     */
    bool GetVhtSupported(uint8_t linkId) const;
    /**
     * Return whether the device supports HE.
     *
     * \return true if HE is supported, false otherwise
     */
    bool GetHeSupported() const;
    /**
     * Return whether the device supports EHT.
     *
     * \return true if EHT is supported, false otherwise
     */
    bool GetEhtSupported() const;

    /**
     * \param address the (link or MLD) address of a remote station
     * \return true if the remote station with the given address supports HT
     */
    bool GetHtSupported(const Mac48Address& address) const;
    /**
     * \param address the (link or MLD) address of a remote station
     * \return true if the remote station with the given address supports VHT
     */
    bool GetVhtSupported(const Mac48Address& address) const;
    /**
     * \param address the (link or MLD) address of a remote station
     * \return true if the remote station with the given address supports HE
     */
    bool GetHeSupported(const Mac48Address& address) const;
    /**
     * \param address the (link or MLD) address of a remote station
     * \return true if the remote station with the given address supports EHT
     */
    bool GetEhtSupported(const Mac48Address& address) const;

    /**
     * Return the maximum A-MPDU size of the given Access Category.
     *
     * \param ac Access Category index
     * \return the maximum A-MPDU size
     */
    uint32_t GetMaxAmpduSize(AcIndex ac) const;
    /**
     * Return the maximum A-MSDU size of the given Access Category.
     *
     * \param ac Access Category index
     * \return the maximum A-MSDU size
     */
    uint16_t GetMaxAmsduSize(AcIndex ac) const;

    /// optional const reference to OriginatorBlockAckAgreement
    using OriginatorAgreementOptConstRef =
        std::optional<std::reference_wrapper<const OriginatorBlockAckAgreement>>;
    /// optional const reference to RecipientBlockAckAgreement
    using RecipientAgreementOptConstRef =
        std::optional<std::reference_wrapper<const RecipientBlockAckAgreement>>;

    /**
     * \param recipient (link or device) MAC address of the recipient
     * \param tid traffic ID.
     *
     * \return the originator block ack agreement, if one has been established
     *
     * Checks if an originator block ack agreement is established with station addressed by
     * <i>recipient</i> for TID <i>tid</i>.
     */
    OriginatorAgreementOptConstRef GetBaAgreementEstablishedAsOriginator(Mac48Address recipient,
                                                                         uint8_t tid) const;
    /**
     * \param originator (link or device) MAC address of the originator
     * \param tid traffic ID.
     *
     * \return the recipient block ack agreement, if one has been established
     *
     * Checks if a recipient block ack agreement is established with station addressed by
     * <i>originator</i> for TID <i>tid</i>.
     */
    RecipientAgreementOptConstRef GetBaAgreementEstablishedAsRecipient(Mac48Address originator,
                                                                       uint8_t tid) const;

    /**
     * \param recipient MAC address
     * \param tid traffic ID
     *
     * \return the type of Block Acks sent by the recipient
     *
     * This function returns the type of Block Acks sent by the recipient.
     */
    BlockAckType GetBaTypeAsOriginator(const Mac48Address& recipient, uint8_t tid) const;
    /**
     * \param recipient MAC address of recipient
     * \param tid traffic ID
     *
     * \return the type of Block Ack Requests sent to the recipient
     *
     * This function returns the type of Block Ack Requests sent to the recipient.
     */
    BlockAckReqType GetBarTypeAsOriginator(const Mac48Address& recipient, uint8_t tid) const;
    /**
     * \param originator MAC address of originator
     * \param tid traffic ID
     *
     * \return the type of Block Acks sent to the originator
     *
     * This function returns the type of Block Acks sent to the originator.
     */
    BlockAckType GetBaTypeAsRecipient(Mac48Address originator, uint8_t tid) const;
    /**
     * \param originator MAC address of originator
     * \param tid traffic ID
     *
     * \return the type of Block Ack Requests sent by the originator
     *
     * This function returns the type of Block Ack Requests sent by the originator.
     */
    BlockAckReqType GetBarTypeAsRecipient(Mac48Address originator, uint8_t tid) const;

  protected:
    void DoInitialize() override;
    void DoDispose() override;

    /**
     * \param cwMin the minimum contention window size
     * \param cwMax the maximum contention window size
     *
     * This method is called to set the minimum and the maximum
     * contention window size.
     */
    virtual void ConfigureContentionWindow(uint32_t cwMin, uint32_t cwMax);

    /**
     * Enable or disable QoS support for the device. Construct a Txop object
     * or QosTxop objects accordingly. This method can only be called before
     * initialization.
     *
     * \param enable whether QoS is supported
     */
    void SetQosSupported(bool enable);

    /**
     * Enable or disable short slot time feature.
     *
     * \param enable true if short slot time is to be supported,
     *               false otherwise
     */
    void SetShortSlotTimeSupported(bool enable);
    /**
     * \return whether the device supports short slot time capability.
     */
    bool GetShortSlotTimeSupported() const;

    /**
     * Accessor for the AC_VO channel access function
     *
     * \return a smart pointer to QosTxop
     */
    Ptr<QosTxop> GetVOQueue() const;
    /**
     * Accessor for the AC_VI channel access function
     *
     * \return a smart pointer to QosTxop
     */
    Ptr<QosTxop> GetVIQueue() const;
    /**
     * Accessor for the AC_BE channel access function
     *
     * \return a smart pointer to QosTxop
     */
    Ptr<QosTxop> GetBEQueue() const;
    /**
     * Accessor for the AC_BK channel access function
     *
     * \return a smart pointer to QosTxop
     */
    Ptr<QosTxop> GetBKQueue() const;

    /**
     * This method acts as the MacRxMiddle receive callback and is
     * invoked to notify us that a frame has been received on the given link.
     * The implementation is intended to capture logic that is going to be
     * common to all (or most) derived classes. Specifically, handling
     * of Block Ack management frames is dealt with here.
     *
     * This method will need, however, to be overridden by derived
     * classes so that they can perform their data handling before
     * invoking the base version.
     *
     * The given link may be undefined in some cases (e.g., in case of
     * QoS Data frames received in the context of a Block Ack agreement --
     * because the BlockAckManager does not have to record the link each
     * buffered MPDU has been received on); in such a cases, the value
     * of <i>linkId</i> should be WIFI_LINKID_UNDEFINED.
     *
     * \param mpdu the MPDU that has been received.
     * \param linkId the ID of the given link
     */
    virtual void Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId);
    /**
     * Forward the packet up to the device.
     *
     * \param packet the packet that we are forwarding up to the device
     * \param from the address of the source
     * \param to the address of the destination
     */
    void ForwardUp(Ptr<const Packet> packet, Mac48Address from, Mac48Address to);

    /**
     * This method can be called to de-aggregate an A-MSDU and forward
     * the constituent packets up the stack.
     *
     * \param mpdu the MPDU containing the A-MSDU.
     */
    virtual void DeaggregateAmsduAndForward(Ptr<const WifiMpdu> mpdu);

    /**
     * Structure holding information specific to a single link. Here, the meaning of
     * "link" is that of the 11be amendment which introduced multi-link devices. For
     * previous amendments, only one link can be created. Therefore, "link" has not
     * to be confused with the general concept of link for a NetDevice (used by the
     * m_linkUp and m_linkDown callbacks).
     */
    struct LinkEntity
    {
        /// Destructor (a virtual method is needed to make this struct polymorphic)
        virtual ~LinkEntity();

        uint8_t id;                                     //!< Link ID (starting at 0)
        Ptr<WifiPhy> phy;                               //!< Wifi PHY object
        Ptr<ChannelAccessManager> channelAccessManager; //!< channel access manager object
        Ptr<FrameExchangeManager> feManager;            //!< Frame Exchange Manager object
        Ptr<WifiRemoteStationManager> stationManager;   //!< Remote station manager (rate control,
                                                        //!< RTS/CTS/fragmentation thresholds etc.)
        bool erpSupported{false};  //!< set to \c true iff this WifiMac is to model 802.11g
        bool dsssSupported{false}; //!< set to \c true iff this WifiMac is to model 802.11b
    };

    /**
     * Get a reference to the link associated with the given ID.
     *
     * \param linkId the given link ID
     * \return a reference to the link associated with the given ID
     */
    LinkEntity& GetLink(uint8_t linkId) const;

    Ptr<MacRxMiddle> m_rxMiddle; //!< RX middle (defragmentation etc.)
    Ptr<MacTxMiddle> m_txMiddle; //!< TX middle (aggregation etc.)
    Ptr<Txop> m_txop;            //!< TXOP used for transmission of frames to non-QoS peers.
    Ptr<WifiMacQueueScheduler> m_scheduler; //!< wifi MAC queue scheduler

    Callback<void> m_linkUp;   //!< Callback when a link is up
    Callback<void> m_linkDown; //!< Callback when a link is down

  private:
    /**
     * \param dcf the DCF to be configured
     * \param cwmin the minimum contention window for the DCF
     * \param cwmax the maximum contention window for the DCF
     * \param isDsss vector of flags to indicate whether PHY is DSSS or HR/DSSS for every link
     * \param ac the access category for the DCF
     *
     * Configure the DCF with appropriate values depending on the given access category.
     */
    void ConfigureDcf(Ptr<Txop> dcf,
                      uint32_t cwmin,
                      uint32_t cwmax,
                      std::list<bool> isDsss,
                      AcIndex ac);

    /**
     * Configure PHY dependent parameters such as CWmin and CWmax on the given link.
     *
     * \param linkId the ID of the given link
     */
    void ConfigurePhyDependentParameters(uint8_t linkId);

    /**
     * This method is a private utility invoked to configure the channel
     * access function for the specified Access Category.
     *
     * \param ac the Access Category of the queue to initialise.
     */
    void SetupEdcaQueue(AcIndex ac);

    /**
     * Create a Frame Exchange Manager depending on the supported version
     * of the standard.
     *
     * \param standard the supported version of the standard
     * \return the created Frame Exchange Manager
     */
    Ptr<FrameExchangeManager> SetupFrameExchangeManager(WifiStandard standard);

    /**
     * Create a LinkEntity object.
     *
     * \return a unique pointer to the created LinkEntity object
     */
    virtual std::unique_ptr<LinkEntity> CreateLinkEntity() const;

    /**
     * This method is called if this device is an MLD to determine the MAC address of
     * the affiliated STA used to communicate with the single link device having the
     * given MAC address. This method is overridden because its implementation depends
     * on the type of station.
     *
     * \param remoteAddr the MAC address of the remote single link device
     * \return the MAC address of the affiliated STA used to communicate with the remote device
     */
    virtual Mac48Address DoGetLocalAddress(const Mac48Address& remoteAddr) const;

    /**
     * Enable or disable ERP support for the given link.
     *
     * \param enable whether ERP is supported
     * \param linkId the ID of the given link
     */
    void SetErpSupported(bool enable, uint8_t linkId);
    /**
     * Enable or disable DSSS support for the given link.
     *
     * \param enable whether DSSS is supported
     * \param linkId the ID of the given link
     */
    void SetDsssSupported(bool enable, uint8_t linkId);

    /**
     * Set the block ack threshold for AC_VO.
     *
     * \param threshold the block ack threshold for AC_VO.
     */
    void SetVoBlockAckThreshold(uint8_t threshold);
    /**
     * Set the block ack threshold for AC_VI.
     *
     * \param threshold the block ack threshold for AC_VI.
     */
    void SetViBlockAckThreshold(uint8_t threshold);
    /**
     * Set the block ack threshold for AC_BE.
     *
     * \param threshold the block ack threshold for AC_BE.
     */
    void SetBeBlockAckThreshold(uint8_t threshold);
    /**
     * Set the block ack threshold for AC_BK.
     *
     * \param threshold the block ack threshold for AC_BK.
     */
    void SetBkBlockAckThreshold(uint8_t threshold);

    /**
     * Set VO block ack inactivity timeout.
     *
     * \param timeout the VO block ack inactivity timeout.
     */
    void SetVoBlockAckInactivityTimeout(uint16_t timeout);
    /**
     * Set VI block ack inactivity timeout.
     *
     * \param timeout the VI block ack inactivity timeout.
     */
    void SetViBlockAckInactivityTimeout(uint16_t timeout);
    /**
     * Set BE block ack inactivity timeout.
     *
     * \param timeout the BE block ack inactivity timeout.
     */
    void SetBeBlockAckInactivityTimeout(uint16_t timeout);
    /**
     * Set BK block ack inactivity timeout.
     *
     * \param timeout the BK block ack inactivity timeout.
     */
    void SetBkBlockAckInactivityTimeout(uint16_t timeout);

    /**
     * This Boolean is set \c true iff this WifiMac is to model
     * 802.11e/WMM style Quality of Service. It is exposed through the
     * attribute system.
     *
     * At the moment, this flag is the sole selection between QoS and
     * non-QoS operation for the STA (whether IBSS, AP, or
     * non-AP). Ultimately, we will want a QoS-enabled STA to be able to
     * fall back to non-QoS operation with a non-QoS peer. This'll
     * require further intelligence - i.e., per-association QoS
     * state. Having a big switch seems like a good intermediate stage,
     * however.
     */
    bool m_qosSupported;

    bool m_shortSlotTimeSupported; ///< flag whether short slot time is supported
    bool m_ctsToSelfSupported;     ///< flag indicating whether CTS-To-Self is supported

    TypeOfStation m_typeOfStation; //!< the type of station

    Ptr<WifiNetDevice> m_device;                      //!< Pointer to the device
    std::vector<std::unique_ptr<LinkEntity>> m_links; //!< vector of Link objects

    Mac48Address m_address; //!< MAC address of this station
    Ssid m_ssid;            //!< Service Set ID (SSID)

    /** This type defines a mapping between an Access Category index,
    and a pointer to the corresponding channel access function.
    Access Categories are sorted in decreasing order of priority. */
    typedef std::map<AcIndex, Ptr<QosTxop>, std::greater<AcIndex>> EdcaQueues;

    /** This is a map from Access Category index to the corresponding
    channel access function */
    EdcaQueues m_edca;

    uint16_t m_voMaxAmsduSize; ///< maximum A-MSDU size for AC_VO (in bytes)
    uint16_t m_viMaxAmsduSize; ///< maximum A-MSDU size for AC_VI (in bytes)
    uint16_t m_beMaxAmsduSize; ///< maximum A-MSDU size for AC_BE (in bytes)
    uint16_t m_bkMaxAmsduSize; ///< maximum A-MSDU size for AC_BK (in bytes)

    uint32_t m_voMaxAmpduSize; ///< maximum A-MPDU size for AC_VO (in bytes)
    uint32_t m_viMaxAmpduSize; ///< maximum A-MPDU size for AC_VI (in bytes)
    uint32_t m_beMaxAmpduSize; ///< maximum A-MPDU size for AC_BE (in bytes)
    uint32_t m_bkMaxAmpduSize; ///< maximum A-MPDU size for AC_BK (in bytes)

    ForwardUpCallback m_forwardUp; //!< Callback to forward packet up the stack

    /**
     * The trace source fired when packets come into the "top" of the device
     * at the L3/L2 transition, before being queued for transmission.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxTrace;
    /**
     * The trace source fired when packets coming into the "top" of the device
     * are dropped at the MAC layer before being queued for transmission.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxDropTrace;
    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a promiscuous trace.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macPromiscRxTrace;
    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a non- promiscuous trace.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macRxTrace;
    /**
     * The trace source fired when packets coming into the "top" of the device
     * are dropped at the MAC layer during reception.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macRxDropTrace;

    TracedCallback<const WifiMacHeader&> m_txOkCallback;  ///< transmit OK callback
    TracedCallback<const WifiMacHeader&> m_txErrCallback; ///< transmit error callback

    /**
     * TracedCallback signature for MPDU drop events.
     *
     * \param reason the reason why the MPDU was dropped (\see WifiMacDropReason)
     * \param mpdu the dropped MPDU
     */
    typedef void (*DroppedMpduCallback)(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu);

    /// TracedCallback for MPDU drop events typedef
    typedef TracedCallback<WifiMacDropReason, Ptr<const WifiMpdu>> DroppedMpduTracedCallback;

    /**
     * This trace indicates that an MPDU was dropped for the given reason.
     */
    DroppedMpduTracedCallback m_droppedMpduCallback;

    /// TracedCallback for acked/nacked MPDUs typedef
    typedef TracedCallback<Ptr<const WifiMpdu>> MpduTracedCallback;

    MpduTracedCallback m_ackedMpduCallback;  ///< ack'ed MPDU callback
    MpduTracedCallback m_nackedMpduCallback; ///< nack'ed MPDU callback

    /**
     * TracedCallback signature for MPDU response timeout events.
     *
     * \param reason the reason why the timer was started
     * \param mpdu the MPDU whose response was not received before the timeout
     * \param txVector the TXVECTOR used to transmit the MPDU
     */
    typedef void (*MpduResponseTimeoutCallback)(uint8_t reason,
                                                Ptr<const WifiMpdu> mpdu,
                                                const WifiTxVector& txVector);

    /// TracedCallback for MPDU response timeout events typedef
    typedef TracedCallback<uint8_t, Ptr<const WifiMpdu>, const WifiTxVector&>
        MpduResponseTimeoutTracedCallback;

    /**
     * MPDU response timeout traced callback.
     * This trace source is fed by a WifiTxTimer object.
     */
    MpduResponseTimeoutTracedCallback m_mpduResponseTimeoutCallback;

    /**
     * TracedCallback signature for PSDU response timeout events.
     *
     * \param reason the reason why the timer was started
     * \param psdu the PSDU whose response was not received before the timeout
     * \param txVector the TXVECTOR used to transmit the PSDU
     */
    typedef void (*PsduResponseTimeoutCallback)(uint8_t reason,
                                                Ptr<const WifiPsdu> psdu,
                                                const WifiTxVector& txVector);

    /// TracedCallback for PSDU response timeout events typedef
    typedef TracedCallback<uint8_t, Ptr<const WifiPsdu>, const WifiTxVector&>
        PsduResponseTimeoutTracedCallback;

    /**
     * PSDU response timeout traced callback.
     * This trace source is fed by a WifiTxTimer object.
     */
    PsduResponseTimeoutTracedCallback m_psduResponseTimeoutCallback;

    /**
     * TracedCallback signature for PSDU map response timeout events.
     *
     * \param reason the reason why the timer was started
     * \param psduMap the PSDU map for which not all responses were received before the timeout
     * \param missingStations the MAC addresses of the stations that did not respond
     * \param nTotalStations the total number of stations that had to respond
     */
    typedef void (*PsduMapResponseTimeoutCallback)(uint8_t reason,
                                                   WifiPsduMap* psduMap,
                                                   const std::set<Mac48Address>* missingStations,
                                                   std::size_t nTotalStations);

    /// TracedCallback for PSDU map response timeout events typedef
    typedef TracedCallback<uint8_t, WifiPsduMap*, const std::set<Mac48Address>*, std::size_t>
        PsduMapResponseTimeoutTracedCallback;

    /**
     * PSDU map response timeout traced callback.
     * This trace source is fed by a WifiTxTimer object.
     */
    PsduMapResponseTimeoutTracedCallback m_psduMapResponseTimeoutCallback;
};

} // namespace ns3

#endif /* WIFI_MAC_H */
