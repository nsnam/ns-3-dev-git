/*
 * Copyright (c) 2008,2009 IITP RAS
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
 * Author: Kirill Andreev <andreev@iitp.ru>
 *         Pavel Boyko <boyko@iitp.ru>
 */

#ifndef MESH_HELPER_H
#define MESH_HELPER_H

#include "mesh-stack-installer.h"

#include "ns3/object-factory.h"
#include "ns3/qos-utils.h"
#include "ns3/wifi-standards.h"

namespace ns3
{

class NetDeviceContainer;
class WifiPhyHelper;
class WifiNetDevice;
class NodeContainer;

/**
 * \ingroup dot11s
 *
 * \brief Helper to create IEEE 802.11s mesh networks
 */
class MeshHelper
{
  public:
    /**
     * Construct a MeshHelper used to make life easier when creating 802.11s networks.
     */
    MeshHelper();

    /**
     * Destroy a MeshHelper.
     */
    ~MeshHelper();

    /**
     * \brief Set the helper to the default values for the MAC type,  remote
     * station manager and channel policy.
     * \returns the default MeshHelper
     */
    static MeshHelper Default();

    /**
     * \brief Set the Mac Attributes.
     *
     * All the attributes specified in this method should exist
     * in the requested mac.
     *
     * \tparam Ts \deduced Argument types
     * \param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetMacType(Ts&&... args);
    /**
     * \brief Set the remote station manager type and Attributes.
     *
     * All the attributes specified in this method should exist
     * in the requested station manager.
     *
     * \tparam Ts \deduced Argument types
     * \param type the type of remote station manager to use.
     * \param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetRemoteStationManager(std::string type, Ts&&... args);
    /**
     * Set standard
     * \param standard the wifi phy standard
     */
    void SetStandard(WifiStandard standard);

    /// \todo SetMeshId
    // void SetMeshId (std::string s);
    /**
     *  \brief Spread/not spread frequency channels of MP interfaces.
     *
     *  If set to true different non-overlapping 20MHz frequency
     *  channels will be assigned to different mesh point interfaces.
     */
    enum ChannelPolicy
    {
        SPREAD_CHANNELS,
        ZERO_CHANNEL
    };

    /**
     * \brief set the channel policy
     * \param policy the channel policy
     */
    void SetSpreadInterfaceChannels(ChannelPolicy policy);
    /**
     * \brief Set a number of interfaces in a mesh network
     * \param nInterfaces is the number of interfaces
     */
    void SetNumberOfInterfaces(uint32_t nInterfaces);

    /**
     * \brief Install 802.11s mesh device & protocols on given node list
     *
     * \param phyHelper           Wifi PHY helper
     * \param c               List of nodes to install
     *
     * \return list of created mesh point devices, see MeshPointDevice
     */
    NetDeviceContainer Install(const WifiPhyHelper& phyHelper, NodeContainer c) const;
    /**
     * \brief Set the MeshStack type to use.
     *
     * \tparam Ts \deduced Argument types
     * \param type the type of ns3::MeshStack.
     * \param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetStackInstaller(std::string type, Ts&&... args);

    /**
     * \brief Print statistics.
     *
     * \param device the net device
     * \param os the output stream
     */
    void Report(const ns3::Ptr<ns3::NetDevice>& device, std::ostream& os);

    /**
     * \brief Reset statistics.
     * \param device the net device
     */
    void ResetStats(const ns3::Ptr<ns3::NetDevice>& device);
    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.  The Install() method of this helper
     * should have previously been called by the user.
     *
     * \param stream first stream index to use
     * \param c NetDeviceContainer of the set of devices for which the mesh devices
     *          should be modified to use a fixed stream
     * \return the number of stream indices assigned by this helper
     */
    int64_t AssignStreams(NetDeviceContainer c, int64_t stream);

    /**
     * Helper to enable all MeshPointDevice log components with one statement
     */
    static void EnableLogComponents();

  private:
    /**
     * \param phyHelper
     * \param node
     * \param channelId
     * \returns a WifiNetDevice with ready-to-use interface
     */
    Ptr<WifiNetDevice> CreateInterface(const WifiPhyHelper& phyHelper,
                                       Ptr<Node> node,
                                       uint16_t channelId) const;
    uint32_t m_nInterfaces;              ///< number of interfaces
    ChannelPolicy m_spreadChannelPolicy; ///< spread channel policy
    Ptr<MeshStack> m_stack;              ///< stack
    ObjectFactory m_stackFactory;        ///< stack factory

    // Interface factory
    ObjectFactory m_mac;                  ///< the MAC
    ObjectFactory m_stationManager;       ///< the station manager
    ObjectFactory m_ackPolicySelector[4]; ///< ack policy selector for all ACs
    WifiStandard m_standard;              ///< standard
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
MeshHelper::SetMacType(Ts&&... args)
{
    m_mac.SetTypeId("ns3::MeshWifiInterfaceMac");
    m_mac.Set(std::forward<Ts>(args)...);
}

template <typename... Ts>
void
MeshHelper::SetRemoteStationManager(std::string type, Ts&&... args)
{
    m_stationManager = ObjectFactory(type, std::forward<Ts>(args)...);
}

template <typename... Ts>
void
MeshHelper::SetStackInstaller(std::string type, Ts&&... args)
{
    m_stackFactory.SetTypeId(type);
    m_stackFactory.Set(std::forward<Ts>(args)...);
    m_stack = m_stackFactory.Create<MeshStack>();
    if (!m_stack)
    {
        NS_FATAL_ERROR("Stack has not been created: " << type);
    }
}

} // namespace ns3

#endif /* MESH_HELPER_H */
