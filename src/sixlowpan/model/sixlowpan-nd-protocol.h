/*
 * Copyright (c) 2020 Università di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Alessio Bonadio <alessio.bonadio@gmail.com>
 *         Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *         Adnan Rashid <adnanrashidpk@gmail.com>
 *         Boh Jie Qi <jieqiboh5836@gmail.com>
 */

#ifndef SIXLOWPAN_ND_PROTOCOL_H
#define SIXLOWPAN_ND_PROTOCOL_H

#include "sixlowpan-header.h"
#include "sixlowpan-nd-binding-table.h"
#include "sixlowpan-nd-header.h"

#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/ip-l4-protocol.h"
#include "ns3/ipv6-address.h"
#include "ns3/lollipop-counter.h"

#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace ns3
{

class SixLowPanNdPrefix;
class SixLowPanNdContext;
class SixLowPanNetDevice;
class SixLowPanNdNsEaroPacketTest;
class SixLowPanNdNaEaroPacketTest;
class SixLowPanNdRaPacketTest;
class SixLowPanNdRsPacketTest;
class SixLowPanNdRovrTest;

/**
 * @ingroup sixlowpan
 * @brief An optimization of the ND protocol for 6LoWPANs.
 */
class SixLowPanNdProtocol : public Icmpv6L4Protocol
{
    friend class SixLowPanNdNsEaroPacketTest;
    friend class SixLowPanNdNaEaroPacketTest;
    friend class SixLowPanNdRaPacketTest;
    friend class SixLowPanNdRsPacketTest;
    friend class SixLowPanNdNsNaTest;
    friend class SixLowPanNdRovrTest;

  public:
    /**
     * 6LoWPAN-ND EARO registration status codes
     */
    enum RegStatus
    {
        SUCCESS = 0x0,       //!< Success
        DUPLICATE_ADDRESS,   //!< Duplicate Address
        NEIGHBOR_CACHE_FULL, //!< Neighbor Cache Full
        MOVED,               //!< Registration failed because it is not the most recent
        REMOVED,             //!< Binding state was removed.
        VALIDATION_REQUEST, //!< Registering Node is challenged for owning the Registered Address or
                            //!< for being an acceptable proxy for the registration.
        DUPLICATE_SOURCE_ADDRESS, //!< Address used as the source of the NS(EARO) conflicts with an
                                  //!< existing registration.
        INVALID_SOURCE_ADDRESS, //!< Address used as the source of the NS(EARO) is not a Link-Local
                                //!< Address.
        REGISTERED_ADDRESS_TOPOLOGICALLY_INCORRECT, //!< Address being registered is not usable on
                                                    //!< this link.
        SIXLBR_REGISTRY_SATURATED,                  //!< 6LBR Registry is saturated.
        VALIDATION_FAILED //!< The proof of ownership of the Registered Address is not correct.
    };

    /**
     * @brief 6LBR constant: minimum delay (in seconds) before a context change may be
     * advertised after a previous change. RFC 6775, Table 2.
     */
    static constexpr uint16_t MIN_CONTEXT_CHANGE_DELAY{300};

    /**
     * @brief 6LR constant: maximum number of initial RA transmissions. RFC 6775, Table 1.
     * Currently unused; reserved for future 6LR role implementation.
     */
    static constexpr uint8_t MAX_RTR_ADVERTISEMENTS{3};

    /**
     * @brief 6LR constant: minimum delay (in seconds) between consecutive RAs. RFC 6775, Table 1.
     * Currently unused; reserved for future 6LR role implementation.
     */
    static constexpr uint8_t MIN_DELAY_BETWEEN_RAS{10};

    /**
     * @brief 6LR constant: maximum delay (in seconds) before responding to an RS. RFC 6775,
     * Table 1. Currently unused; reserved for future 6LR role implementation.
     */
    static constexpr uint8_t MAX_RA_DELAY_TIME{2};

    /**
     * @brief Router constant: hop limit used for multihop DAR/DAC messages. RFC 6775, Table 1.
     */
    static constexpr uint8_t MULTIHOP_HOPLIMIT{64};

    /**
     * @brief Constructor.
     */
    SixLowPanNdProtocol();

    /**
     * @brief Destructor.
     */
    ~SixLowPanNdProtocol() override;

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    void DoInitialize() override;
    void NotifyNewAggregate() override;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

    enum IpL4Protocol::RxStatus Receive(Ptr<Packet> p,
                                        const Ipv6Header& header,
                                        Ptr<Ipv6Interface> interface) override;

    /**
     * @brief Create and register a binding table for the given device and interface.
     * @param device the net device
     * @param interface the IPv6 interface
     */
    void CreateBindingTable(Ptr<NetDevice> device, Ptr<Ipv6Interface> interface);

    bool Lookup(Ptr<Packet> p,
                const Ipv6Header& ipHeader,
                Ipv6Address dst,
                Ptr<NetDevice> device,
                Ptr<NdiscCache> cache,
                Address* hardwareDestination) override;

    void FunctionDadTimeout(Ipv6Interface* interface, Ipv6Address addr) override;

    /**
     * Sets the ROVR for the node.
     *
     * @param rovr ROVR to set for this node
     */
    void SetRovr(const std::vector<uint8_t> rovr);

    /**
     * @brief Send a NS for 6LoWPAN ND (+ EARO, SLLAO).
     * @param addrToRegister source IPv6 address
     * @param dst destination IPv6 address
     * @param dstMac destination MAC address
     * @param time registration lifetime (EARO)
     * @param rovr ROVR (EARO)
     * @param sixDevice SixLowPan NetDevice
     */
    void SendSixLowPanNsWithEaro(Ipv6Address addrToRegister,
                                 Ipv6Address dst,
                                 Address dstMac,
                                 uint16_t time,
                                 const std::vector<uint8_t>& rovr,
                                 Ptr<NetDevice> sixDevice);

    /**
     * @brief Send a NA for 6LoWPAN ND (+ EARO).
     * @param src source IPv6 address
     * @param dst destination IPv6 address
     * @param target target IPv6 address
     * @param time registration lifetime (EARO)
     * @param rovr ROVR (EARO)
     * @param sixDevice SixLowPan NetDevice
     * @param status status (EARO)
     */
    void SendSixLowPanNaWithEaro(Ipv6Address src,
                                 Ipv6Address dst,
                                 Ipv6Address target,
                                 uint16_t time,
                                 const std::vector<uint8_t>& rovr,
                                 Ptr<NetDevice> sixDevice,
                                 uint8_t status);
    /**
     * @brief Send a Multicast RS (+ 6CIO) (RFC6775 5.3)
     * @param src source IPv6 address
     * @param hardwareAddress the hardware address of the node
     */
    void SendSixLowPanMulticastRS(Ipv6Address src, Address hardwareAddress);

    /**
     * @brief Send a RA for 6LoWPAN ND (+ PIO, 6CO, 6CIO, ABRO, SLLAO).
     * @param src source IPv6 address
     * @param dst destination IPv6 address
     * @param interface the interface from which the packet will be sent
     */
    void SendSixLowPanRA(Ipv6Address src, Ipv6Address dst, Ptr<Ipv6Interface> interface);

    /**
     * @brief Set an interface to be used as a 6LBR
     * @param device device to be used for announcement
     */
    void SetInterfaceAs6lbr(Ptr<SixLowPanNetDevice> device);

    /**
     * @brief Set a prefix to be announced on an interface (6LBR)
     * @param device device to be used for announcement
     * @param prefix announced prefix
     */
    void SetAdvertisedPrefix(Ptr<SixLowPanNetDevice> device, Ipv6Prefix prefix);

    /**
     * @brief Add a context to be advertised on an interface (6LBR)
     * @param device device to be used for advertisement
     * @param context advertised context
     */
    void AddAdvertisedContext(Ptr<SixLowPanNetDevice> device, Ipv6Prefix context);

    /**
     * @brief Remove a context to be advertised on an interface (6LBR)
     * @param device device to be used for advertisement
     * @param context advertised context
     */
    void RemoveAdvertisedContext(Ptr<SixLowPanNetDevice> device, Ipv6Prefix context);

    /**
     * @brief Checks if an interface is set as 6LBR
     * @param device the interface to check
     * @return true if the interface is configured as a 6LBR
     */
    bool IsBorderRouterOnInterface(Ptr<SixLowPanNetDevice> device) const;

    /**
     * @brief Find the binding table corresponding to the IPv6 interface.
     * @param interface the IPv6 interface
     * @return the SixLowPanNdBindingTable associated with the interface
     */
    Ptr<SixLowPanNdBindingTable> FindBindingTable(Ptr<Ipv6Interface> interface);

  protected:
    /**
     * @brief Dispose this object.
     */
    void DoDispose() override;

  private:
    /**
     * @brief RA advertised from routers for 6LoWPAN ND.
     */
    class SixLowPanRaEntry : public SimpleRefCount<SixLowPanRaEntry>
    {
      public:
        SixLowPanRaEntry();

        /**
         * Creates a RA entry from the RA header contents
         * @param raHeader the RA header
         * @param abroHdr the ABRO header
         * @param contextList the list of Contexts
         * @param prefixList the list of PIOs
         */
        SixLowPanRaEntry(Icmpv6RA raHeader,
                         Icmpv6OptionSixLowPanAuthoritativeBorderRouter abroHdr,
                         std::list<Icmpv6OptionSixLowPanContext> contextList,
                         std::list<Icmpv6OptionPrefixInformation> prefixList);

        ~SixLowPanRaEntry();

        /**
         * @brief Get the prefixes advertised for this interface.
         * @return a list of IPv6 prefixes
         */
        std::list<Ptr<SixLowPanNdPrefix>> GetPrefixes() const;

        /**
         * @brief Add a prefix to advertise on interface.
         * @param prefix prefix to advertise
         */
        void AddPrefix(Ptr<SixLowPanNdPrefix> prefix);

        /**
         * @brief Remove a prefix from the ones advertised on interface.
         * @param prefix prefix to remove from advertisements
         */
        void RemovePrefix(Ptr<SixLowPanNdPrefix> prefix);

        /**
         * @brief Get list of 6LoWPAN contexts advertised for this interface.
         * @return list of 6LoWPAN contexts
         */
        std::map<uint8_t, Ptr<SixLowPanNdContext>> GetContexts() const;

        /**
         * @brief Add a 6LoWPAN context to advertise on interface.
         * @param context 6LoWPAN context to advertise
         */
        void AddContext(Ptr<SixLowPanNdContext> context);

        /**
         * @brief Remove a 6LoWPAN context.
         * @param context 6LoWPAN context to remove
         */
        void RemoveContext(Ptr<SixLowPanNdContext> context);

        /**
         * Builds an Icmpv6RA from the stored data.
         * @return the Icmpv6RA.
         */
        Icmpv6RA BuildRouterAdvertisementHeader() const;

        /**
         * Builds a container of Icmpv6OptionPrefixInformation from the stored data.
         * @return the Icmpv6OptionPrefixInformation container.
         */
        std::list<Icmpv6OptionPrefixInformation> BuildPrefixInformationOptions();

        /**
         * @brief Is managed flag enabled ?
         * @return managed flag
         */
        bool IsManagedFlag() const;

        /**
         * @brief Set managed flag
         * @param managedFlag value
         */
        void SetManagedFlag(bool managedFlag);

        /**
         * @brief Is "other config" flag enabled ?
         * @return other config flag
         */
        bool IsOtherConfigFlag() const;

        /**
         * @brief Is "home agent" flag enabled ?
         * @return "home agent" flag
         */
        bool IsHomeAgentFlag() const;

        /**
         * @brief Set "home agent" flag.
         * @param homeAgentFlag value
         */
        void SetHomeAgentFlag(bool homeAgentFlag);

        /**
         * @brief Set "other config" flag
         * @param otherConfigFlag value
         */
        void SetOtherConfigFlag(bool otherConfigFlag);

        /**
         * @brief Get reachable time.
         * @return reachable time
         */
        uint32_t GetReachableTime() const;

        /**
         * @brief Set reachable time.
         * @param time reachable time
         */
        void SetReachableTime(uint32_t time);

        /**
         * @brief Get router lifetime.
         * @return router lifetime
         */
        uint32_t GetRouterLifeTime() const;

        /**
         * @brief Set router lifetime.
         * @param time router lifetime
         */
        void SetRouterLifeTime(uint32_t time);

        /**
         * @brief Get retransmission timer.
         * @return retransmission timer
         */
        uint32_t GetRetransTimer() const;

        /**
         * @brief Set retransmission timer.
         * @param timer retransmission timer
         */
        void SetRetransTimer(uint32_t timer);

        /**
         * @brief Get current hop limit.
         * @return current hop limit for the link
         */
        uint8_t GetCurHopLimit() const;

        /**
         * @brief Set current hop limit.
         * @param curHopLimit current hop limit for the link
         */
        void SetCurHopLimit(uint8_t curHopLimit);

        /**
         * @brief Get version value (ABRO).
         * @return the version value
         */
        uint32_t GetAbroVersion() const;

        /**
         * @brief Set version value (ABRO).
         * @param version the version value
         */
        void SetAbroVersion(uint32_t version);

        /**
         * @brief Get valid lifetime value (ABRO).
         * @return the valid lifetime (units of 60 seconds)
         */
        uint16_t GetAbroValidLifeTime() const;

        /**
         * @brief Set valid lifetime value (ABRO).
         * @param time the valid lifetime (units of 60 seconds)
         */
        void SetAbroValidLifeTime(uint16_t time);

        /**
         * @brief Get Border Router address (ABRO).
         * @return the Border Router address
         */
        Ipv6Address GetAbroBorderRouterAddress() const;

        /**
         * @brief Set Border Router address (ABRO).
         * @param border the Border Router address
         */
        void SetAbroBorderRouterAddress(Ipv6Address border);

        /**
         * @brief Parse an ABRO and records the appropriate params.
         * @param abro the Authoritative Border Router Option header
         * @return true if the parsing was correct.
         */
        bool ParseAbro(Icmpv6OptionSixLowPanAuthoritativeBorderRouter abro);

        /**
         * @brief Build an ABRO header.
         * @return the Authoritative Border Router Option header
         */
        Icmpv6OptionSixLowPanAuthoritativeBorderRouter MakeAbro();

      private:
        /**
         * @brief Advertised Prefixes.
         */
        std::list<Ptr<SixLowPanNdPrefix>> m_prefixes;

        /**
         * @brief List of 6LoWPAN contexts advertised.
         */
        std::map<uint8_t, Ptr<SixLowPanNdContext>> m_contexts;

        /**
         * @brief Managed flag. If true, the host uses the stateful protocol for address
         * autoconfiguration.
         */
        bool m_managedFlag;

        /**
         * @brief Other configuration flag. If true, the host uses the stateful protocol for other
         * (non-address) information.
         */
        bool m_otherConfigFlag;

        /**
         * @brief Flag to add HA (home agent) flag in RA.
         */
        bool m_homeAgentFlag;

        /**
         * @brief Reachable time in milliseconds.
         */
        uint32_t m_reachableTime;

        /**
         * @brief Retransmission timer in milliseconds.
         */
        uint32_t m_retransTimer;

        /**
         * @brief Current hop limit (TTL).
         */
        uint8_t m_curHopLimit;

        /**
         * @brief Router life time in seconds.
         */
        uint32_t m_routerLifeTime;

        /**
         * @brief Version value for ABRO.
         */
        uint32_t m_abroVersion;

        /**
         * @brief Valid lifetime value for ABRO (units of 60 seconds).
         */
        uint16_t m_abroValidLifeTime;

        /**
         * @brief Border Router address for ABRO.
         */
        Ipv6Address m_abroBorderRouter;
    };

    /**
     *  Role of the node: 6LN, 6LR, 6LBR
     *
     *  A Node starts either as a 6LN or as a 6LBR.
     *  A 6LN can become a 6LR.
     *
     */
    enum SixLowPanNodeStatus_e
    {
        SixLowPanNodeOnly,    //!< a 6LN that can not become a 6LR
        SixLowPanNode,        //!< a 6LN that can (and want to) become a 6LR
        SixLowPanBorderRouter //!< a 6LBR
    };

    /**
     * @brief The amount of time (units of 60 seconds) that the router should retain the NCE for the
     * node.
     */
    uint16_t m_regTime;

    /**
     * @brief How many seconds before registration expiry to begin re-registration.
     */
    uint16_t m_advance;

    /**
     * @brief Status of the node.
     */
    SixLowPanNodeStatus_e m_nodeRole;

    Time m_routerLifeTime;       //!< Default Router Lifetime.
    Time m_pioPreferredLifeTime; //!< Default Prefix Information Preferred Lifetime.
    Time m_pioValidLifeTime;     //!< Default Prefix Information Valid Lifetime.
    Time m_contextValidLifeTime; //!< Default Context Valid Lifetime.
    Time m_abroValidLifeTime;    //!< Default ABRO Valid Lifetime.

    /**
     * @brief Random jitter before sending address registrations.
     */
    Ptr<RandomVariableStream> m_addressRegistrationJitter;

    /**
     * @brief NS handler for 6LoWPAN ND.
     * @param packet the packet.
     * @param src source address.
     * @param dst destination address.
     * @param interface the interface from which the packet is coming.
     */
    void HandleSixLowPanNS(Ptr<Packet> packet,
                           const Ipv6Address& src,
                           const Ipv6Address& dst,
                           Ptr<Ipv6Interface> interface);

    /**
     * @brief NA handler for 6LoWPAN ND.
     * @param packet the packet
     * @param src source address
     * @param dst destination address
     * @param interface the interface from which the packet is coming
     */
    void HandleSixLowPanNA(Ptr<Packet> packet,
                           const Ipv6Address& src,
                           const Ipv6Address& dst,
                           Ptr<Ipv6Interface> interface);

    /**
     * @brief RS handler for 6LoWPAN ND.
     * @param packet the packet
     * @param src source address
     * @param dst destination address
     * @param interface the interface from which the packet is coming
     */
    void HandleSixLowPanRS(Ptr<Packet> packet,
                           const Ipv6Address& src,
                           const Ipv6Address& dst,
                           Ptr<Ipv6Interface> interface);

    /**
     * @brief RA handler for 6LoWPAN ND.
     * @param packet the packet
     * @param src source address
     * @param dst destination address
     * @param interface the interface from which the packet is coming
     */
    void HandleSixLowPanRA(Ptr<Packet> packet,
                           const Ipv6Address& src,
                           const Ipv6Address& dst,
                           Ptr<Ipv6Interface> interface);

    /**
     * Address registration procedure
     */
    void AddressRegistration();

    /**
     * Address registration success or failure.
     *
     * @param registrar node that performs the registration. Registered Node or a proxy.
     */
    void AddressRegistrationSuccess(Ipv6Address registrar);

    /**
     * Address registration timeout handler
     */
    void AddressRegistrationTimeout();

    uint8_t m_addressRegistrationCounter = 0; //!< Number of retries of an address registration.

    std::vector<uint8_t> m_rovr; //!< Node ROVR

    EventId m_addressRegistrationTimeoutEvent; //!< Address Registration timeout event.

    EventId m_addressRegistrationEvent; //!< Address Registration event.

    std::set<Ipv6Address>
        m_raCache; //!< Set of 6LBR addresses from which a RA has already been processed.

    std::map<Ptr<SixLowPanNetDevice>, Ptr<SixLowPanRaEntry>>
        m_raEntries; //!< Router Advertisement entries (if the node is a 6LBR).

    TracedCallback<Ipv6Address, bool, uint8_t>
        m_addressRegistrationResultTrace; //!< Traces address registration result (address,
                                          //!< success/failure, status code)

    typedef Callback<void, Ipv6Address, bool, uint8_t>
        AddressRegistrationCallback; //!< Trace sink signature for address registration result

    TracedCallback<Ipv6Address> m_multicastRsTrace; //!< Trace fired whenever a multicast RS is sent

    typedef Callback<void, Ipv6Address>
        MulticastRsCallback; //!< Trace sink signature for multicast RS sends

    TracedCallback<Ptr<Packet>> m_naRxTrace; //!< Trace fired whenever an NA packet is received

    typedef Callback<void, Ptr<Packet>> NaRxCallback; //!< Trace sink signature for NA reception

    /**
     * Struct holding data about a pending RA being processed.
     */
    struct SixLowPanPendingRa
    {
        Ipv6Address source; //!< Origin of the RA / Registering Node (will be a 6LBR)
        Icmpv6OptionLinkLayerAddress llaHdr; //!< Contains MAC address of the RA sender (6LBR)
        Ptr<Ipv6Interface> interface;        //!< Interface that received the RA
        std::list<std::pair<Ipv6Address, Icmpv6OptionPrefixInformation>>
            addressesToBeRegistered; //!< Addresses pending registration with their PIOs.
    };

    std::list<SixLowPanPendingRa> m_pendingRas; //!< RA awaiting processing (address registration).

    /**
     * Struct holding data about registered addresses.
     */
    struct SixLowPanRegisteredAddress
    {
        Ipv6Address registrar;               //!< Registering node (link-local addr / gaddr of 6LBR)
        Time registrationTimeout;            //!< Registration expiration time
        Ipv6Address registeredAddr;          //!< Registered address
        Icmpv6OptionLinkLayerAddress llaHdr; //!< Contains MAC address of the RA sender (6LBR)
        Ptr<Ipv6Interface> interface;        //!< Interface used for the registration
        Icmpv6OptionPrefixInformation
            pioHdr; //!< Prefix Information Option for the address being registered
    };

    std::list<SixLowPanRegisteredAddress>
        m_registeredAddresses; //!< Addresses that have been registered.

    /**
     * Struct holding data for the address currently being registered.
     */
    struct AddressPendingRegistration
    {
        bool isValid; //!< The data are valid (for timeouts and retransmissions)
        Ipv6Address addressPendingRegistration; //!< Address being Registered
        Ipv6Address registrar;                  //!< Registering node address (always link-local)
        bool newRegistration; //!< new registration (true) or re-registration (false)
        Icmpv6OptionLinkLayerAddress
            llaHdr; //!< Link-Layer address option from the RA (can be 6LR or 6LBR).
        Ptr<Ipv6Interface> interface; //!< Interface on which the RA was received
        Icmpv6OptionPrefixInformation
            pioHdr; //!< Prefix Information Option for the address being registered
    };

    AddressPendingRegistration m_addrPendingReg; //!< Address currently being Registered

    Time m_maxRtrSolicitationInterval; //!< Maximum RS Retransmission interval

    using BindingTableList =
        std::list<Ptr<SixLowPanNdBindingTable>>; //!< container of BindingTables

    BindingTableList m_bindingTableList; //!< Binding Table for 6LoWPAN ND

    /**
     * @brief Construct NS (EARO) packet.
     * @param src source address
     * @param dst destination address
     * @param nsHdr
     * @param slla
     * @param tlla
     * @param earo
     * @return NS (EARO) Packet
     */
    static Ptr<Packet> MakeNsEaroPacket(Ipv6Address src,
                                        Ipv6Address dst,
                                        Icmpv6NS& nsHdr,
                                        Icmpv6OptionLinkLayerAddress& slla,
                                        Icmpv6OptionLinkLayerAddress& tlla,
                                        Icmpv6OptionSixLowPanExtendedAddressRegistration& earo);

    /**
     * @brief Construct NA (EARO) packet.
     * @param src source address
     * @param dst destination address
     * @param naHdr
     * @param earo
     * @return NA (EARO) Packet
     */
    static Ptr<Packet> MakeNaEaroPacket(Ipv6Address src,
                                        Ipv6Address dst,
                                        Icmpv6NA& naHdr,
                                        Icmpv6OptionSixLowPanExtendedAddressRegistration& earo);

    /**
     * @brief Constructs a RA packet (raEntry contains info for raHdr, pios, abro and contexts)
     * @param src source address
     * @param dst destination address
     * @param slla Source Link-Layer Address Option
     * @param cio Capability Indication Option
     * @param raEntry RA entry containing router advertisement information
     * @return RA Packet
     */
    static Ptr<Packet> MakeRaPacket(Ipv6Address src,
                                    Ipv6Address dst,
                                    Icmpv6OptionLinkLayerAddress& slla,
                                    Icmpv6OptionSixLowPanCapabilityIndication& cio,
                                    Ptr<SixLowPanRaEntry> raEntry);

    /**
     * @brief Parses NS packet and populates params, returning true if packet is a valid NS/NS(EARO)
     * packet
     * @param p Packet to be parsed
     * @param nsHdr populated with packet nsHdr
     * @param slla populated with packet SLLAO if present
     * @param tlla populated with packet TLLAO if present
     * @param earo populated with packet EARO if present
     * @param hasEaro true if NS packet contains an EARO option
     * @return True if packet is valid, false otherwise
     */
    static bool ParseAndValidateNsEaroPacket(Ptr<Packet> p,
                                             Icmpv6NS& nsHdr,
                                             Icmpv6OptionLinkLayerAddress& slla,
                                             Icmpv6OptionLinkLayerAddress& tlla,
                                             Icmpv6OptionSixLowPanExtendedAddressRegistration& earo,
                                             bool& hasEaro);

    /**
     * @brief Parses NA packet and populates params, returning true if packet is valid
     * @param p Packet to be parsed
     * @param naHdr populated with packet NA header
     * @param tlla populated with packet Target Link-Layer Address Option if present
     * @param earo populated with packet EARO if present
     * @param hasEaro true if NA packet contains an EARO option
     * @return True if packet is valid, false otherwise
     */
    static bool ParseAndValidateNaEaroPacket(Ptr<Packet> p,
                                             Icmpv6NA& naHdr,
                                             Icmpv6OptionLinkLayerAddress& tlla,
                                             Icmpv6OptionSixLowPanExtendedAddressRegistration& earo,
                                             bool& hasEaro);

    /**
     * @brief Parses RS packet and populates params, returning true if packet is valid
     * @param p Packet to be parsed
     * @param rsHdr populated with packet RS header
     * @param slla populated with packet Source Link-Layer Address Option if present
     * @param cio populated with packet Capability Indication Option if present
     * @return True if packet is valid, false otherwise
     */
    static bool ParseAndValidateRsPacket(Ptr<Packet> p,
                                         Icmpv6RS& rsHdr,
                                         Icmpv6OptionLinkLayerAddress& slla,
                                         Icmpv6OptionSixLowPanCapabilityIndication& cio);

    /**
     * @brief Parses RA packet and populates params, returning true if packet is valid
     * @param p Packet to be parsed
     * @param raHdr populated with packet RA header
     * @param pios populated with Prefix Information Options from the packet
     * @param abro populated with Authoritative Border Router Option from the packet
     * @param slla populated with Source Link-Layer Address Option if present
     * @param cio populated with Capability Indication Option if present
     * @param contexts populated with 6LoWPAN Context Options from the packet
     * @return True if packet is valid, false otherwise
     */
    static bool ParseAndValidateRaPacket(Ptr<Packet> p,
                                         Icmpv6RA& raHdr,
                                         std::list<Icmpv6OptionPrefixInformation>& pios,
                                         Icmpv6OptionSixLowPanAuthoritativeBorderRouter& abro,
                                         Icmpv6OptionLinkLayerAddress& slla,
                                         Icmpv6OptionSixLowPanCapabilityIndication& cio,
                                         std::list<Icmpv6OptionSixLowPanContext>& contexts);
};

} /* namespace ns3 */

#endif
