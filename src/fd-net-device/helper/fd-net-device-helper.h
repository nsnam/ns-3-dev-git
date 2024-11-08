/*
 * Copyright (c) 2012 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Alina Quereilhac <alina.quereilhac@inria.fr>
 *
 */

#ifndef FD_NET_DEVICE_HELPER_H
#define FD_NET_DEVICE_HELPER_H

#include "ns3/attribute.h"
#include "ns3/fd-net-device.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/trace-helper.h"

#include <string>

namespace ns3
{

/**
 * @ingroup fd-net-device
 * @brief build a set of FdNetDevice objects
 * Normally we eschew multiple inheritance, however, the classes
 * PcapUserHelperForDevice and AsciiTraceUserHelperForDevice are
 * treated as "mixins".  A mixin is a self-contained class that
 * encapsulates a general attribute or a set of functionality that
 * may be of interest to many other classes.
 */
class FdNetDeviceHelper : public PcapHelperForDevice, public AsciiTraceHelperForDevice
{
  public:
    /**
     * Construct a FdNetDeviceHelper.
     */
    FdNetDeviceHelper();

    ~FdNetDeviceHelper() override
    {
    }

    /**
     * Set the TypeId of the Objects to be created by this helper.
     *
     * @param [in] type The TypeId of the object to instantiate.
     */
    void SetTypeId(std::string type);

    /**
     * @param n1 the name of the attribute to set
     * @param v1 the value of the attribute to set
     *
     * Set these attributes on each ns3::FdNetDevice created
     * by FdNetDeviceHelper::Install
     */
    void SetAttribute(std::string n1, const AttributeValue& v1);

    /**
     * This method creates a FdNetDevice and associates it to a node
     *
     * @param node The node to install the device in
     * @returns A container holding the added net device.
     */
    virtual NetDeviceContainer Install(Ptr<Node> node) const;

    /**
     * This method creates a FdNetDevice and associates it to a node
     *
     * @param name The name of the node to install the device in
     * @returns A container holding the added net device.
     */
    virtual NetDeviceContainer Install(std::string name) const;

    /**
     * This method creates a FdNetDevice and associates it to a node.
     * For each Ptr<node> in the provided container: it creates an ns3::FdNetDevice
     * (with the attributes  configured by FdNetDeviceHelper::SetDeviceAttribute);
     * adds the device to the node; and attaches the channel to the device.
     *
     * @param c The NodeContainer holding the nodes to be changed.
     * @returns A container holding the added net devices.
     */
    virtual NetDeviceContainer Install(const NodeContainer& c) const;

  protected:
    /**
     * This method creates an ns3::FdNetDevice and associates it to a node
     *
     * @param node The node to install the device in
     * @returns A container holding the added net device.
     */
    virtual Ptr<NetDevice> InstallPriv(Ptr<Node> node) const;

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

    ObjectFactory m_deviceFactory; //!< factory for the NetDevices
};

} // namespace ns3

#endif /* FD_NET_DEVICE_HELPER_H */
