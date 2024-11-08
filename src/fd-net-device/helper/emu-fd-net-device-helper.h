/*
 * Copyright (c) 2012 INRIA, 2012 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#ifndef EMU_FD_NET_DEVICE_HELPER_H
#define EMU_FD_NET_DEVICE_HELPER_H

#include "fd-net-device-helper.h"

#include "ns3/attribute.h"
#include "ns3/fd-net-device.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include <string>

namespace ns3
{

/**
 * @ingroup fd-net-device
 * @brief build a set of FdNetDevice objects attached to a physical network
 * interface
 *
 */
class EmuFdNetDeviceHelper : public FdNetDeviceHelper
{
  public:
    /**
     * Construct a EmuFdNetDeviceHelper.
     */
    EmuFdNetDeviceHelper();

    ~EmuFdNetDeviceHelper() override
    {
    }

    /**
     * Get the device name of this device.
     *
     * @returns The device name of this device.
     */
    std::string GetDeviceName();

    /**
     * Set the device name of this device.
     *
     * @param deviceName The device name of this device.
     */
    void SetDeviceName(std::string deviceName);

    /**
     * @brief Request host qdisc bypass
     * @param hostQdiscBypass to enable host qdisc bypass
     */
    void HostQdiscBypass(bool hostQdiscBypass);

  protected:
    /**
     * This method creates an ns3::FdNetDevice attached to a physical network
     * interface
     *
     * @param node The node to install the device in
     * @returns A container holding the added net device.
     */
    Ptr<NetDevice> InstallPriv(Ptr<Node> node) const override;

    /**
     * Sets a file descriptor on the FileDescriptorNetDevice.
     * @param device the device to install the file descriptor in
     */
    virtual void SetFileDescriptor(Ptr<FdNetDevice> device) const;

    /**
     * Call out to a separate process running as suid root in order to get a raw
     * socket.  We do this to avoid having the entire simulation running as root.
     * @return the rawSocket number
     */
    virtual int CreateFileDescriptor() const;

    /**
     * The Unix/Linux name of the underlying device (e.g., eth0)
     */
    std::string m_deviceName;
    bool m_hostQdiscBypass; //!< True if request host qdisc bypass
};

} // namespace ns3

#endif /* EMU_FD_NET_DEVICE_HELPER_H */
