/*
 * Copyright (c) 2011 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef SIXLOWPAN_HELPER_H
#define SIXLOWPAN_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ptr.h"

#include <string>

namespace ns3
{

class Node;
class AttributeValue;
class Time;
class Ipv6InterfaceContainer;
class Icmpv6OptionSixLowPanCapabilityIndication;

/**
 * @ingroup sixlowpan
 *
 * @brief Setup a sixlowpan stack to be used as a shim between IPv6 and a generic NetDevice.
 */
class SixLowPanHelper
{
  public:
    /*
     * Construct a SixlowpanHelper
     */
    SixLowPanHelper();
    /**
     * Set an attribute on each ns3::SixlowpanNetDevice created by
     * SixlowpanHelper::Install.
     *
     * @param n1 [in] The name of the attribute to set.
     * @param v1 [in] The value of the attribute to set.
     */
    void SetDeviceAttribute(std::string n1, const AttributeValue& v1);

    /**
     * @brief Install the SixLoWPAN stack on top of an existing NetDevice.
     *
     * This function requires a set of properly configured NetDevices
     * passed in as the parameter "c". The new NetDevices will have to
     * be used instead of the original ones. In this way these
     * SixLoWPAN devices will behave as shims between the NetDevices
     * passed in and IPv6.
     *
     * Note that only IPv6 (and related protocols, such as ICMPv6) can
     * be transmitted over a 6LoWPAN interface.
     * Any other protocol (e.g., IPv4) will be discarded by 6LoWPAN.
     *
     * Other protocols (e.g., IPv4) could be used on the original NetDevices
     * with some limitations.
     * See the manual for a complete discussion.
     *
     * @note IPv6 stack must be installed \a after SixLoWPAN,
     * using the SixLoWPAN NetDevices. See the example in the
     * examples directory.
     *
     *
     * @param [in] c The NetDevice container.
     * @return A container with the newly created SixLowPanNetDevices.
     */
    NetDeviceContainer Install(NetDeviceContainer c);

    /**
     * @brief Adds a compression Context to a set of NetDevices.
     *
     * This function installs one Compression Context on a set of NetDevices.
     * The context is used only in IPHC compression and decompression.
     *
     * @param [in] c The NetDevice container.
     * @param [in] contextId The context id (must be less than 16).
     * @param [in] context The context prefix.
     * @param [in] validity the context validity time (relative to the actual time).
     */
    void AddContext(NetDeviceContainer c, uint8_t contextId, Ipv6Prefix context, Time validity);

    /**
     * @brief Renew a compression Context in a set of NetDevices.
     *
     * The context will have its lifetime extended and its validity for compression re-enabled.
     *
     * @param [in] c The NetDevice container.
     * @param [in] contextId The context id (must be less than 16).
     * @param [in] validity the context validity time (relative to the actual time).
     */
    void RenewContext(NetDeviceContainer c, uint8_t contextId, Time validity);

    /**
     * @brief Invalidates a compression Context in a set of NetDevices.
     *
     * An invalid context is used only in IPHC decompression and not
     * in IPHC compression.
     *
     * This is necessary to avoid that a context reaching its validity lifetime
     * can not be used for decompression whie packets are traveling the network.
     *
     * @param [in] c The NetDevice container.
     * @param [in] contextId The context id (must be less than 16).
     */
    void InvalidateContext(NetDeviceContainer c, uint8_t contextId);

    /**
     * @brief Remove a compression Context in a set of NetDevices.
     *
     * The context is removed immediately from the contexts in the devices.
     *
     * @param [in] c The NetDevice container.
     * @param [in] contextId The context id (must be less than 16).
     */
    void RemoveContext(NetDeviceContainer c, uint8_t contextId);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model. Return the number of streams (possibly zero) that
     * have been assigned. The Install() method should have previously been
     * called by the user.
     *
     * @param [in] c NetDeviceContainer of the set of net devices for which the
     *          SixLowPanNetDevice should be modified to use a fixed stream.
     * @param [in] stream First stream index to use.
     * @return The number of stream indices assigned by this helper.
     */
    int64_t AssignStreams(NetDeviceContainer c, int64_t stream);

    /**
     * @brief Install the SixLoWPAN-ND stack, associate it with a NetDevice, and set it as a 6LBR.
     *
     * @note IPv6 stack must NOT be installed \a after this function, because it has been already
     * set up.
     *
     * @param [in] c The NetDevice container.
     * @param [in] baseAddr The prefix to be announced by the 6LBR.
     * @return A container of the addresses assigned to the NetDevices.
     */
    Ipv6InterfaceContainer InstallSixLowPanNdBorderRouter(NetDeviceContainer c,
                                                          Ipv6Address baseAddr);

    /**
     * @brief Install the SixLoWPAN-ND stack, associate it with a NetDevice, and set it as a 6LN.
     * @note IPv6 stack must NOT be installed \a after this function, because it has been already
     * set up.
     *
     * @param [in] c The NetDevice container.
     * @return A container of the addresses assigned to the NetDevices.
     */
    Ipv6InterfaceContainer InstallSixLowPanNdNode(NetDeviceContainer c);

    /**
     * @brief Add a new prefix to be advertised by 6LoWPAN-ND.
     * @param [in] nd The NetDevice.
     * @param prefix announced IPv6 prefix
     */
    void SetAdvertisedPrefix(const Ptr<NetDevice> nd, Ipv6Prefix prefix);

    /**
     * @brief Add a new context to be advertised by 6LoWPAN-ND.
     * @param [in] nd The NetDevice.
     * @param context announced IPv6 context
     */
    void AddAdvertisedContext(const Ptr<NetDevice> nd, Ipv6Prefix context);

    /**
     * @brief Remove a context advertised by 6LoWPAN-ND.
     * @param [in] nd The NetDevice.
     * @param context announced IPv6 context
     */
    void RemoveAdvertisedContext(const Ptr<NetDevice> nd, Ipv6Prefix context);

    /**
     * @brief Add a Capability Indication to be advertised by 6LoWPAN-ND.
     * @param [in] nd The NetDevice.
     * @param capability announced Node capability
     */
    void SetCapabilityIndication(const Ptr<NetDevice> nd,
                                 Icmpv6OptionSixLowPanCapabilityIndication capability);

    /**
     * @brief Print the binding table of all nodes at a specific time.
     * @param printTime the time at which the binding table is printed.
     * @param stream the output stream wrapper used for printing.
     * @param unit the time unit to be used in the report.
     */
    static void PrintBindingTableAllAt(Time printTime,
                                       Ptr<OutputStreamWrapper> stream,
                                       Time::Unit unit = Time::S);

    /**
     * @brief Print the binding table of a node.
     * @param node the node for which the binding table is printed.
     * @param stream the output stream wrapper used for printing.
     * @param unit the time unit to be used in the report.
     */
    static void PrintBindingTable(Ptr<Node> node,
                                  Ptr<OutputStreamWrapper> stream,
                                  Time::Unit unit = Time::S);

  private:
    /**
     * @brief Install the SixLoWPAN-ND stack in the node and associates it with a NetDevice.
     *
     * @param [in] c The NetDevice container.
     * @param [in] borderRouter Set the NetDevices to work as 6LBRs.
     */
    void InstallSixLowPanNd(NetDeviceContainer c, bool borderRouter);

    /**
     * @brief Initialize the ROVR for a node.
     * @param [in] node The node to initialize the ROVR for.
     */
    void InitializeRovr(Ptr<Node> node);

    /**
     * @brief Convert a MAC address to a ROVR.
     * @param mac The MAC address to convert.
     * @return A vector of bytes representing the ROVR.
     */
    std::vector<uint8_t> ConvertMacToRovr(const Address& mac);

    /**
     * @brief Print a ROVR as a string.
     * @param rovr The ROVR to print.
     * @return A string representation of the ROVR.
     */
    std::string PrintRovr(const std::vector<uint8_t>& rovr);

    ObjectFactory m_deviceFactory; //!< Object factory.
};

} // namespace ns3

#endif /* SIXLOWPAN_HELPER_H */
