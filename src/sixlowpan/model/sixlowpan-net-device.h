/*
 * Copyright (c) 2013 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *         Michele Muccio <michelemuccio@virgilio.it>
 */

#ifndef SIXLOWPAN_NET_DEVICE_H
#define SIXLOWPAN_NET_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/traced-callback.h"

#include <map>
#include <stdint.h>
#include <string>
#include <tuple>

namespace ns3
{

class Node;
class UniformRandomVariable;
class EventId;

/**
 * @defgroup sixlowpan 6LoWPAN
 * @brief Performs 6LoWPAN compression of IPv6 packets as specified by \RFC{4944} and \RFC{6282}
 *
 * This module acts as a shim between IPv6 and a generic NetDevice.
 *
 * The module implements \RFC{4944} and \RFC{6282}, with the following exceptions:
 * <ul>
 * <li> MESH and LOWPAN_BC0 dispatch types are not supported </li>
 * <li> HC2 encoding is not supported </li>
 * <li> IPHC's SAC and DAC are not supported </li>
 *</ul>
 */

/**
 * @ingroup sixlowpan
 * @ingroup tests
 * @defgroup sixlowpan-tests 6LoWPAN module tests
 */

/**
 * @ingroup sixlowpan
 *
 * @brief Shim performing 6LoWPAN compression, decompression and fragmentation.
 *
 * This class implements the shim between IPv6 and a generic NetDevice,
 * performing packet compression, decompression and fragmentation in a transparent way.
 * To this end, the class pretend to be a normal NetDevice, masquerading some functions
 * of the underlying NetDevice.
 */
class SixLowPanNetDevice : public NetDevice
{
  public:
    /**
     * Enumeration of the dropping reasons in SixLoWPAN.
     */
    enum DropReason
    {
        DROP_FRAGMENT_TIMEOUT = 1,           //!< Fragment timeout exceeded
        DROP_FRAGMENT_BUFFER_FULL,           //!< Fragment buffer size exceeded
        DROP_UNKNOWN_EXTENSION,              //!< Unsupported compression kind
        DROP_DISALLOWED_COMPRESSION,         //!< HC1 while in IPHC mode or vice-versa
        DROP_SATETFUL_DECOMPRESSION_PROBLEM, //!< Decompression failed due to missing or expired
                                             //!< context
    };

    /**
     * @brief The protocol number for 6LoWPAN (0xA0ED) - see \RFC{7973}.
     */
    static constexpr uint16_t PROT_NUMBER{0xA0ED};

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Constructor for the SixLowPanNetDevice.
     */
    SixLowPanNetDevice();

    // Delete copy constructor and assignment operator to avoid misuse
    SixLowPanNetDevice(const SixLowPanNetDevice&) = delete;
    SixLowPanNetDevice& operator=(const SixLowPanNetDevice&) = delete;

    // inherited from NetDevice base class
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    void SetAddress(Address address) override;
    Address GetAddress() const override;
    bool SetMtu(const uint16_t mtu) override;

    /**
     * @brief Returns the link-layer MTU for this interface.
     * If the link-layer MTU is smaller than IPv6's minimum MTU (\RFC{4944}),
     * 1280 will be returned.
     *
     * @return The link-level MTU in bytes for this interface.
     */
    uint16_t GetMtu() const override;
    bool IsLinkUp() const override;
    void AddLinkChangeCallback(Callback<void> callback) override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    bool IsPointToPoint() const override;
    bool IsBridge() const override;
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;
    void SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;
    Address GetMulticast(Ipv6Address addr) const override;

    /**
     * @brief Returns a smart pointer to the underlying NetDevice.
     *
     * @return A smart pointer to the underlying NetDevice.
     */
    Ptr<NetDevice> GetNetDevice() const;

    /**
     * @brief Setup SixLowPan to be a proxy for the specified NetDevice.
     * All the packets incoming and outgoing from the NetDevice will be
     * processed by SixLowPanNetDevice.
     *
     * @param [in] device A smart pointer to the NetDevice to be proxied.
     */
    void SetNetDevice(Ptr<NetDevice> device);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param [in] stream First stream index to use.
     * @return the number of stream indices assigned by this model.
     */
    int64_t AssignStreams(int64_t stream);

    /**
     * TracedCallback signature for packet send/receive events.
     *
     * @param [in] packet The packet.
     * @param [in] sixNetDevice The SixLowPanNetDevice.
     * @param [in] ifindex The ifindex of the device.
     * @deprecated The non-const \c Ptr<SixLowPanNetDevice> argument
     * is deprecated and will be changed to \c Ptr<const SixLowPanNetDevice>
     * in a future release.
     */
    // NS_DEPRECATED() - tag for future removal
    typedef void (*RxTxTracedCallback)(Ptr<const Packet> packet,
                                       Ptr<SixLowPanNetDevice> sixNetDevice,
                                       uint32_t ifindex);

    /**
     * TracedCallback signature for packet drop events
     *
     * @param [in] reason The reason for the drop.
     * @param [in] packet The packet.
     * @param [in] sixNetDevice The SixLowPanNetDevice.
     * @param [in] ifindex The ifindex of the device.
     * @deprecated The non-const \c Ptr<SixLowPanNetDevice> argument
     * is deprecated and will be changed to \c Ptr<const SixLowPanNetDevice>
     * in a future release.
     */
    // NS_DEPRECATED() - tag for future removal
    typedef void (*DropTracedCallback)(DropReason reason,
                                       Ptr<const Packet> packet,
                                       Ptr<SixLowPanNetDevice> sixNetDevice,
                                       uint32_t ifindex);

    /**
     * Add, remove, or update a context used in IPHC stateful compression.
     *
     * A context with a zero validLifetime will be immediately removed.
     *
     * @param [in] contextId context id (most be between 0 and 15 included).
     * @param [in] contextPrefix context prefix to be used in compression/decompression.
     * @param [in] compressionAllowed compression and decompression allowed (true), decompression
     * only (false).
     * @param [in] validLifetime validity time (relative to the actual time).
     *
     */
    void AddContext(uint8_t contextId,
                    Ipv6Prefix contextPrefix,
                    bool compressionAllowed,
                    Time validLifetime);

    /**
     * Get a context used in IPHC stateful compression.
     *
     * @param [in] contextId context id (most be between 0 and 15 included).
     * @param [out] contextPrefix context prefix to be used in compression/decompression.
     * @param [out] compressionAllowed compression and decompression allowed (true), decompression
     * only (false).
     * @param [out] validLifetime validity time (relative to the actual time).
     *
     * @return false if the context has not been found.
     *
     */
    bool GetContext(uint8_t contextId,
                    Ipv6Prefix& contextPrefix,
                    bool& compressionAllowed,
                    Time& validLifetime);

    /**
     * Renew a context used in IPHC stateful compression.
     *
     * The context will have its lifetime extended and its validity for compression re-enabled.
     *
     * @param [in] contextId context id (most be between 0 and 15 included).
     * @param [in] validLifetime validity time (relative to the actual time).
     */
    void RenewContext(uint8_t contextId, Time validLifetime);

    /**
     * Invalidate a context used in IPHC stateful compression.
     *
     * An invalid context will not be used for compression but it will be used for decompression.
     *
     * @param [in] contextId context id (most be between 0 and 15 included).
     */
    void InvalidateContext(uint8_t contextId);

    /**
     * Remove a context used in IPHC stateful compression.
     *
     * @param [in] contextId context id (most be between 0 and 15 included).
     */
    void RemoveContext(uint8_t contextId);

  protected:
    void DoDispose() override;

  private:
    /**
     * @brief Receives all the packets from a NetDevice for further processing.
     * @param [in] device The NetDevice the packet ws received from.
     * @param [in] packet The received packet.
     * @param [in] protocol The protocol (if known).
     * @param [in] source The source address.
     * @param [in] destination The destination address.
     * @param [in] packetType The packet kind (e.g., HOST, BROADCAST, etc.).
     */
    void ReceiveFromDevice(Ptr<NetDevice> device,
                           Ptr<const Packet> packet,
                           uint16_t protocol,
                           const Address& source,
                           const Address& destination,
                           PacketType packetType);

    /**
     * @param [in] packet Packet sent from above down to Network Device.
     * @param [in] source Source mac address (only used if doSendFrom is true, i.e., "MAC
     * spoofing").
     * @param [in] dest Mac address of the destination (already resolved).
     * @param [in] protocolNumber Identifies the type of payload contained in this packet. Used to
     * call the right L3Protocol when the packet is received.
     * @param [in] doSendFrom Perform a SendFrom instead of a Send.
     *
     *  Called from higher layer to send packet into Network Device
     *  with the specified source and destination Addresses.
     *
     * @return Whether the Send operation succeeded.
     */
    bool DoSend(Ptr<Packet> packet,
                const Address& source,
                const Address& dest,
                uint16_t protocolNumber,
                bool doSendFrom);

    /**
     * The callback used to notify higher layers that a packet has been received.
     */
    NetDevice::ReceiveCallback m_rxCallback;

    /**
     * The callback used to notify higher layers that a packet has been received in promiscuous
     * mode.
     */
    NetDevice::PromiscReceiveCallback m_promiscRxCallback;

    /**
     * @brief Callback to trace TX (transmission) packets.
     *
     * Data passed:
     * \li Packet received (including 6LoWPAN header)
     * \li Ptr to SixLowPanNetDevice
     * \li interface index
     * @deprecated The non-const \c Ptr<SixLowPanNetDevice> argument
     * is deprecated and will be changed to \c Ptr<const SixLowPanNetDevice>
     * in a future release.
     */
    // NS_DEPRECATED() - tag for future removal
    TracedCallback<Ptr<const Packet>, Ptr<SixLowPanNetDevice>, uint32_t> m_txTrace;

    /**
     * @brief Callback to trace RX (reception) packets.
     *
     * Data passed:
     * \li Packet received (including 6LoWPAN header)
     * \li Ptr to SixLowPanNetDevice
     * \li interface index
     * @deprecated The non-const \c Ptr<SixLowPanNetDevice> argument
     * is deprecated and will be changed to \c Ptr<const SixLowPanNetDevice>
     * in a future release.
     */
    // NS_DEPRECATED() - tag for future removal
    TracedCallback<Ptr<const Packet>, Ptr<SixLowPanNetDevice>, uint32_t> m_rxTrace;

    /**
     * @brief Callback to trace drop packets.
     *
     * Data passed:
     * \li DropReason
     * \li Packet dropped (including 6LoWPAN header)
     * \li Ptr to SixLowPanNetDevice
     * \li interface index
     * @deprecated The non-const \c Ptr<SixLowPanNetDevice> argument
     * is deprecated and will be changed to \c Ptr<const SixLowPanNetDevice>
     * in a future release.
     */
    // NS_DEPRECATED() - tag for future removal
    TracedCallback<DropReason, Ptr<const Packet>, Ptr<SixLowPanNetDevice>, uint32_t> m_dropTrace;

    /**
     * @brief Compress the headers according to HC1 compression.
     * @param [in] packet The packet to be compressed.
     * @param [in] src The MAC source address.
     * @param [in] dst The MAC destination address.
     * @return The size of the removed headers.
     */
    uint32_t CompressLowPanHc1(Ptr<Packet> packet, const Address& src, const Address& dst);

    /**
     * @brief Decompress the headers according to HC1 compression.
     * @param [in] packet the packet to be compressed.
     * @param [in] src the MAC source address.
     * @param [in] dst the MAC destination address.
     */
    void DecompressLowPanHc1(Ptr<Packet> packet, const Address& src, const Address& dst);

    /**
     * @brief Compress the headers according to IPHC compression.
     * @param [in] packet The packet to be compressed.
     * @param [in] src The MAC source address.
     * @param [in] dst The MAC destination address.
     * @return The size of the removed headers.
     */
    uint32_t CompressLowPanIphc(Ptr<Packet> packet, const Address& src, const Address& dst);

    /**
     * @brief Checks if the next header can be compressed using NHC.
     * @param [in] headerType The header kind to be compressed.
     * @return True if the header can be compressed.
     */
    bool CanCompressLowPanNhc(uint8_t headerType);

    /**
     * @brief Decompress the headers according to IPHC compression.
     * @param [in] packet The packet to be compressed.
     * @param [in] src The MAC source address.
     * @param [in] dst The MAC destination address.
     * @return true if the packet can not be decompressed due to wrong context information.
     */
    bool DecompressLowPanIphc(Ptr<Packet> packet, const Address& src, const Address& dst);

    /**
     * @brief Compress the headers according to NHC compression.
     * @param [in] packet The packet to be compressed.
     * @param [in] headerType The header type.
     * @param [in] src The MAC source address.
     * @param [in] dst The MAC destination address.
     * @return The size of the removed headers.
     */
    uint32_t CompressLowPanNhc(Ptr<Packet> packet,
                               uint8_t headerType,
                               const Address& src,
                               const Address& dst);

    /**
     * @brief Decompress the headers according to NHC compression.
     * @param [in] packet The packet to be compressed.
     * @param [in] src The MAC source address.
     * @param [in] dst The MAC destination address.
     * @param [in] srcAddress The IPv6 source address.
     * @param [in] dstAddress The IPv6 destination address.
     * @return A std::pair containing the decompressed header type and a flag - true if the packet
     * can not be decompressed due to wrong context information.
     */
    std::pair<uint8_t, bool> DecompressLowPanNhc(Ptr<Packet> packet,
                                                 const Address& src,
                                                 const Address& dst,
                                                 Ipv6Address srcAddress,
                                                 Ipv6Address dstAddress);

    /**
     * @brief Compress the headers according to NHC compression.
     * @param [in] packet The packet to be compressed.
     * @param [in] omitChecksum Omit UDP checksum (if true).
     * @return The size of the removed headers.
     */
    uint32_t CompressLowPanUdpNhc(Ptr<Packet> packet, bool omitChecksum);

    /**
     * @brief Decompress the headers according to NHC compression.
     * @param [in] packet The packet to be compressed.
     * @param [in] saddr The IPv6 source address.
     * @param [in] daddr The IPv6 destination address.
     */
    void DecompressLowPanUdpNhc(Ptr<Packet> packet, Ipv6Address saddr, Ipv6Address daddr);

    /**
     * Fragment identifier type: src/dst address src/dst port.
     */
    typedef std::pair<std::pair<Address, Address>, std::pair<uint16_t, uint16_t>> FragmentKey_t;

    /// Container for fragment timeouts.
    typedef std::list<std::tuple<Time, FragmentKey_t, uint32_t>> FragmentsTimeoutsList_t;
    /// Container Iterator for fragment timeouts.
    typedef std::list<std::tuple<Time, FragmentKey_t, uint32_t>>::iterator FragmentsTimeoutsListI_t;

    /**
     * @brief Set a new timeout "event" for a fragmented packet
     * @param key the fragment identification
     * @param iif input interface of the packet
     * @return an iterator to the inserted "event"
     */
    FragmentsTimeoutsListI_t SetTimeout(FragmentKey_t key, uint32_t iif);

    /**
     * @brief Handles a fragmented packet timeout
     */
    void HandleTimeout();

    FragmentsTimeoutsList_t m_timeoutEventList; //!< Timeout "events" container

    EventId m_timeoutEvent; //!< Event for the next scheduled timeout

    /**
     * @brief A Set of Fragments.
     */
    class Fragments : public SimpleRefCount<Fragments>
    {
      public:
        /**
         * @brief Constructor.
         */
        Fragments();

        /**
         * @brief Destructor.
         */
        ~Fragments();

        /**
         * @brief Add a fragment to the pool.
         * @param [in] fragment the fragment.
         * @param [in] fragmentOffset the offset of the fragment.
         */
        void AddFragment(Ptr<Packet> fragment, uint16_t fragmentOffset);

        /**
         * @brief Add the first packet fragment. The first fragment is needed to
         * allow the post-defragmentation decompression.
         * @param [in] fragment The fragment.
         */
        void AddFirstFragment(Ptr<Packet> fragment);

        /**
         * @brief If all fragments have been added.
         * @returns True if the packet is entire.
         */
        bool IsEntire() const;

        /**
         * @brief Get the entire packet.
         * @return The entire packet.
         */
        Ptr<Packet> GetPacket() const;

        /**
         * @brief Set the packet-to-be-defragmented size.
         * @param [in] packetSize The packet size (bytes).
         */
        void SetPacketSize(uint32_t packetSize);

        /**
         * @brief Get a list of the current stored fragments.
         * @returns The current stored fragments.
         */
        std::list<Ptr<Packet>> GetFragments() const;

        /**
         * @brief Set the Timeout iterator.
         * @param iter The iterator.
         */
        void SetTimeoutIter(FragmentsTimeoutsListI_t iter);

        /**
         * @brief Get the Timeout iterator.
         * @returns The iterator.
         */
        FragmentsTimeoutsListI_t GetTimeoutIter();

      private:
        /**
         * @brief The size of the reconstructed packet (bytes).
         */
        uint32_t m_packetSize;

        /**
         * @brief The current fragments.
         */
        std::list<std::pair<Ptr<Packet>, uint16_t>> m_fragments;

        /**
         * @brief The very first fragment.
         */
        Ptr<Packet> m_firstFragment;

        /**
         * @brief Timeout iterator to "event" handler
         */
        FragmentsTimeoutsListI_t m_timeoutIter;
    };

    /**
     * @brief Performs a packet fragmentation.
     * @param [in] packet the packet to be fragmented (with headers already compressed with
     * 6LoWPAN).
     * @param [in] origPacketSize the size of the IP packet before the 6LoWPAN header compression,
     * including the IP/L4 headers.
     * @param [in] origHdrSize the size of the IP header before the 6LoWPAN header compression.
     * @param [in] extraHdrSize the sum of the sizes of BC0 header and MESH header if mesh routing
     * is used or 0.
     * @param [out] listFragments A reference to the list of the resulting packets, all with the
     * proper headers in place.
     */
    void DoFragmentation(Ptr<Packet> packet,
                         uint32_t origPacketSize,
                         uint32_t origHdrSize,
                         uint32_t extraHdrSize,
                         std::list<Ptr<Packet>>& listFragments);

    /**
     * @brief Process a packet fragment.
     * @param [in] packet The packet.
     * @param [in] src The source MAC address.
     * @param [in] dst The destination MAC address.
     * @param [in] isFirst True if it is the first fragment, false otherwise.
     * @return True is the fragment completed the packet.
     */
    bool ProcessFragment(Ptr<Packet>& packet, const Address& src, const Address& dst, bool isFirst);

    /**
     * @brief Process the timeout for packet fragments.
     * @param [in] key A key representing the packet fragments.
     * @param [in] iif Input Interface.
     */
    void HandleFragmentsTimeout(FragmentKey_t key, uint32_t iif);

    /**
     * @brief Drops the oldest fragment set.
     */
    void DropOldestFragmentSet();

    /**
     * Get a Mac16 from its Mac48 pseudo-MAC
     * @param addr the PseudoMac address
     * @return the Mac16Address
     */
    Address Get16MacFrom48Mac(Address addr);

    /**
     * Container for fragment key -> fragments.
     */
    typedef std::map<FragmentKey_t, Ptr<Fragments>> MapFragments_t;
    /**
     * Container Iterator for fragment key -> fragments.
     */
    typedef std::map<FragmentKey_t, Ptr<Fragments>>::iterator MapFragmentsI_t;

    MapFragments_t m_fragments;       //!< Fragments hold to be rebuilt.
    Time m_fragmentExpirationTimeout; //!< Time limit for fragment rebuilding.

    /**
     * @brief How many packets can be rebuilt at the same time.
     * Some real implementation do limit this. Zero means no limit.
     */
    uint16_t m_fragmentReassemblyListSize;

    bool m_useIphc; //!< Use IPHC or HC1.

    bool m_meshUnder;            //!< Use a mesh-under routing.
    uint8_t m_bc0Serial;         //!< Serial number used in BC0 header.
    uint8_t m_meshUnderHopsLeft; //!< Start value for mesh-under hops left.
    uint16_t m_meshCacheLength;  //!< length of the cache for each source.
    Ptr<RandomVariableStream>
        m_meshUnderJitter; //!< Random variable for the mesh-under packet retransmission.
    std::map<Address /* OriginatorAddress */, std::list<uint8_t /* SequenceNumber */>>
        m_seenPkts; //!< Seen packets, memorized by OriginatorAddress, SequenceNumber.

    Ptr<Node> m_node;           //!< Smart pointer to the Node.
    Ptr<NetDevice> m_netDevice; //!< Smart pointer to the underlying NetDevice.
    uint32_t m_ifIndex;         //!< Interface index.

    bool m_omitUdpChecksum; //!< Omit UDP checksum in NC1 encoding.

    uint32_t m_compressionThreshold; //!< Minimum L2 payload size.

    Ptr<UniformRandomVariable> m_rng; //!< Rng for the fragments tag.

    /**
     * Structure holding the information for a context (used in compression and decompression)
     */
    struct ContextEntry
    {
        Ipv6Prefix contextPrefix; //!< context prefix to be used in compression/decompression
        bool compressionAllowed;  //!< compression and decompression allowed (true), decompression
                                  //!< only (false)
        Time validLifetime;       //!< validity period
    };

    std::map<uint8_t, ContextEntry>
        m_contextTable; //!< Table of the contexts used in compression/decompression

    /**
     * @brief Finds if the given unicast address matches a context for compression
     *
     * \param[in] address the address to check
     * \param[out] contextId the context found
     * @return true if a valid context has been found
     */
    bool FindUnicastCompressionContext(Ipv6Address address, uint8_t& contextId);

    /**
     * @brief Finds if the given multicast address matches a context for compression
     *
     * \param[in] address the address to check
     * \param[out] contextId the context found
     * @return true if a valid context has been found
     */
    bool FindMulticastCompressionContext(Ipv6Address address, uint8_t& contextId);

    /**
     * @brief Clean an address from its prefix.
     *
     * This function is used to find the relevant bits to be sent in stateful IPHC compression.
     * Only the prefix length is used - the address prefix is assumed to be matching the prefix.
     *
     * @param address the address to be cleaned
     * @param prefix the prefix to remove
     * @return An address with the prefix zeroed.
     */
    Ipv6Address CleanPrefix(Ipv6Address address, Ipv6Prefix prefix);
};

} // namespace ns3

#endif /* SIXLOWPAN_NET_DEVICE_H */
