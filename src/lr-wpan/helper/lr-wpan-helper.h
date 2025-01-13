/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Gary Pei <guangyu.pei@boeing.com>
 *  Tom Henderson <thomas.r.henderson@boeing.com>
 */
#ifndef LR_WPAN_HELPER_H
#define LR_WPAN_HELPER_H

#include "ns3/lr-wpan-mac.h"
#include "ns3/lr-wpan-phy.h"
#include "ns3/node-container.h"
#include "ns3/trace-helper.h"

namespace ns3
{

class SpectrumChannel;
class MobilityModel;

/**
 * @ingroup lr-wpan
 *
 * @brief helps to manage and create IEEE 802.15.4 NetDevice objects
 *
 * This class can help to create IEEE 802.15.4 NetDevice objects
 * and to configure their attributes during creation.  It also contains
 * additional helper functions used by client code.
 *
 * Only one channel is created, and all devices attached to it.  If
 * multiple channels are needed, multiple helper objects must be used,
 * or else the channel object must be replaced.
 */

class LrWpanHelper : public PcapHelperForDevice, public AsciiTraceHelperForDevice
{
  public:
    /**
     * @brief Create a LrWpan helper in an empty state.
     */
    LrWpanHelper();

    /**
     * @brief Create a LrWpan helper in an empty state.
     * @param useMultiModelSpectrumChannel use a MultiModelSpectrumChannel if true, a
     * SingleModelSpectrumChannel otherwise
     */
    LrWpanHelper(bool useMultiModelSpectrumChannel);

    ~LrWpanHelper() override;

    // Delete copy constructor and assignment operator to avoid misuse
    LrWpanHelper(const LrWpanHelper&) = delete;
    LrWpanHelper& operator=(const LrWpanHelper&) = delete;

    /**
     * @brief Get the channel associated to this helper
     * @returns the channel
     */
    Ptr<SpectrumChannel> GetChannel();

    /**
     * @brief Set the channel associated to this helper
     * @param channel the channel
     */
    void SetChannel(Ptr<SpectrumChannel> channel);

    /**
     * @brief Set the channel associated to this helper
     * @param channelName the channel name
     */
    void SetChannel(std::string channelName);

    /**
     * @tparam Ts \deduced Argument types
     * @param name the name of the model to set
     * @param [in] args Name and AttributeValue pairs to set.
     *
     * Add a propagation loss model to the set of currently-configured loss models.
     */
    template <typename... Ts>
    void AddPropagationLossModel(std::string name, Ts&&... args);

    /**
     * @tparam Ts \deduced Argument types
     * @param name the name of the model to set
     * @param [in] args Name and AttributeValue pairs to set.
     *
     * Configure a propagation delay for this channel.
     */
    template <typename... Ts>
    void SetPropagationDelayModel(std::string name, Ts&&... args);

    /**
     * @brief Add mobility model to a physical device
     * @param phy the physical device
     * @param m the mobility model
     */
    void AddMobility(Ptr<lrwpan::LrWpanPhy> phy, Ptr<MobilityModel> m);

    /**
     * @brief Install a LrWpanNetDevice and the associated structures (e.g., channel) in the nodes.
     *
     * If the channel is not already initialized, it will be created as either a
     * SingleModelSpectrumChannel or a MultiModelSpectrumChannel, depending on the
     * useMultiModelSpectrumChannel flag. Additionally, a ConstantSpeedPropagationDelayModel will be
     * set as the default delay model if no delay model is specified, and a
     * LogDistancePropagationLossModel will be added to the channel if no propagation loss models
     * are defined.
     *
     * If the channel is already initialized but lacks either a PropagationDelayModel or a
     * PropagationLossModel, an error will be raised.
     * @param c a set of nodes
     * @returns A container holding the added net devices.
     */
    NetDeviceContainer Install(NodeContainer c);

    /**
     * @brief Creates an PAN with associated nodes and assigned addresses(16 and 64)
     *        from the nodes in the node container.
     *        The first node in the container becomes the PAN coordinator.
     *
     * @param c a The node container with the nodes that will form the PAN.
     * @param panId The PAN identifier.
     */
    void CreateAssociatedPan(NetDeviceContainer c, uint16_t panId);

    /**
     * @brief Set the extended 64 bit addresses (EUI-64) for a group of
     *        LrWpanNetDevices
     *
     * @param c The NetDevice container.
     *
     */
    void SetExtendedAddresses(NetDeviceContainer c);

    /**
     * Helper to enable all LrWpan log components with one statement
     */
    void EnableLogComponents();

    /**
     * @brief Transform the LrWpanPhyEnumeration enumeration into a printable string.
     * @param e the LrWpanPhyEnumeration
     * @return a string
     */
    static std::string LrWpanPhyEnumerationPrinter(lrwpan::PhyEnumeration e);

    /**
     * @brief Transform the LrWpanMacState enumeration into a printable string.
     * @param e the LrWpanMacState
     * @return a string
     */
    static std::string LrWpanMacStatePrinter(lrwpan::MacState e);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model. Return the number of streams that have been
     * assigned. The Install() method should have previously been
     * called by the user.
     *
     * @param c NetDeviceContainer of the set of net devices for which the
     *          CsmaNetDevice should be modified to use a fixed stream
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this helper
     */
    int64_t AssignStreams(NetDeviceContainer c, int64_t stream);

  private:
    /**
     * @brief Enable pcap output on the indicated net device.
     *
     * NetDevice-specific implementation mechanism for hooking the trace and
     * writing to the trace file.
     *
     * @param prefix Filename prefix to use for pcap files.
     * @param nd Net device for which you want to enable tracing.
     * @param promiscuous If true capture all possible packets available at the device.
     * @param explicitFilename Treat the prefix as an explicit filename if true
     */
    void EnablePcapInternal(std::string prefix,
                            Ptr<NetDevice> nd,
                            bool promiscuous,
                            bool explicitFilename) override;

    /**
     * @brief Enable ascii trace output on the indicated net device.
     *
     * NetDevice-specific implementation mechanism for hooking the trace and
     * writing to the trace file.
     *
     * @param stream The output stream object to use when logging ascii traces.
     * @param prefix Filename prefix to use for ascii trace files.
     * @param nd Net device for which you want to enable tracing.
     * @param explicitFilename Treat the prefix as an explicit filename if true
     */
    void EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
                             std::string prefix,
                             Ptr<NetDevice> nd,
                             bool explicitFilename) override;

  private:
    Ptr<SpectrumChannel> m_channel;      //!< channel to be used for the devices
    bool m_useMultiModelSpectrumChannel; //!< indicates whether a MultiModelSpectrumChannel is used
    std::vector<ObjectFactory> m_propagationLoss; ///< vector of propagation loss models
    ObjectFactory m_propagationDelay;             ///< propagation delay model
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
LrWpanHelper::AddPropagationLossModel(std::string name, Ts&&... args)
{
    m_propagationLoss.push_back(ObjectFactory(name, std::forward<Ts>(args)...));
}

template <typename... Ts>
void
LrWpanHelper::SetPropagationDelayModel(std::string name, Ts&&... args)
{
    m_propagationDelay = ObjectFactory(name, std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* LR_WPAN_HELPER_H */
