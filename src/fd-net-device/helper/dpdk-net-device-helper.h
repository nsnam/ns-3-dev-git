/*
 * Copyright (c) 2020 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Harsh Patel <thadodaharsh10@gmail.com>
 *          Hrishikesh Hiraskar <hrishihiraskar@gmail.com>
 */

#ifndef DPDK_NET_DEVICE_HELPER_H
#define DPDK_NET_DEVICE_HELPER_H

#include "emu-fd-net-device-helper.h"

namespace ns3
{

/**
 * @ingroup fd-net-device
 * @brief build a DpdkNetDevice object attached to a physical network
 * interface
 */
class DpdkNetDeviceHelper : public EmuFdNetDeviceHelper
{
  public:
    /**
     * Construct a DpdkNetDeviceHelper and initialize DPDK EAL
     */
    DpdkNetDeviceHelper();

    ~DpdkNetDeviceHelper() override
    {
    }

    /**
     * @brief Sets list of logical cores to use
     *
     * @param lCoreList Comma separated logical core list (e.g., "0,1")
     */
    void SetLCoreList(std::string lCoreList);

    /**
     * @brief Sets PMD Library to be used for the NIC
     *
     * @param pmdLibrary The PMD Library
     */
    void SetPmdLibrary(std::string pmdLibrary);

    /**
     * @brief Sets DPDK Driver to bind NIC to
     *
     * @param dpdkDriver The DPDK Driver
     */
    void SetDpdkDriver(std::string dpdkDriver);

  protected:
    /**
     * @brief This method creates an ns3::FdNetDevice attached to a physical network
     * interface
     *
     * @param node The node to install the device in
     * @returns A container holding the added net device.
     */
    Ptr<NetDevice> InstallPriv(Ptr<Node> node) const override;

    /**
     * Logical cores to use
     */
    std::string m_lCoreList;

    /**
     * PMD Library to be used for NIC
     */
    std::string m_pmdLibrary;

    /**
     * DPDK Driver to bind NIC to
     */
    std::string m_dpdkDriver;
};

} // namespace ns3

#endif /* DPDK_NET_DEVICE_HELPER_H */
