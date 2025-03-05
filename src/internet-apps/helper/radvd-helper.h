/*
 * Copyright (c) 2013 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef RADVD_HELPER_H
#define RADVD_HELPER_H

#include "ns3/application-helper.h"
#include "ns3/radvd-interface.h"

#include <map>
#include <stdint.h>

namespace ns3
{

/**
 * @ingroup radvd
 * @brief Radvd application helper.
 */
class RadvdHelper : public ApplicationHelper
{
  public:
    /**
     * @brief Constructor.
     */
    RadvdHelper();

    /**
     * @brief Add a new prefix to be announced through an interface.
     * @param interface outgoing interface
     * @param prefix announced IPv6 prefix
     * @param prefixLength announced IPv6 prefix length
     * @param slaac available for autoconfiguration
     */
    void AddAnnouncedPrefix(uint32_t interface,
                            const Ipv6Address& prefix,
                            uint32_t prefixLength,
                            bool slaac = true);

    /**
     * @brief Enable the router as default router for the interface.
     * The effect is to set the Router Lifetime to the default value (30 minutes)
     * @param interface outgoing interface
     */
    void EnableDefaultRouterForInterface(uint32_t interface);

    /**
     * @brief Disable the router as default router for the interface.
     * The effect is to set the Router Lifetime to zero
     * @param interface outgoing interface
     */
    void DisableDefaultRouterForInterface(uint32_t interface);

    /**
     * @brief Get the low-level RadvdInterface specification for an interface.
     * This method is provided to enable fine-grain parameter setup.
     * @param interface outgoing interface
     * @returns the RadvdInterface
     */
    Ptr<RadvdInterface> GetRadvdInterface(uint32_t interface);

    /**
     * @brief Clear the stored Prefixes
     */
    void ClearPrefixes();

  private:
    Ptr<Application> DoInstall(Ptr<Node> node) override;

    /// Container: interface index, RadvdInterface
    typedef std::map<uint32_t, Ptr<RadvdInterface>> RadvdInterfaceMap;
    /// Container Iterator: interface index, RadvdInterface
    typedef std::map<uint32_t, Ptr<RadvdInterface>>::iterator RadvdInterfaceMapI;

    RadvdInterfaceMap m_radvdInterfaces; //!< RadvdInterface(s)
};

} /* namespace ns3 */

#endif /* RADVD_HELPER_H */
