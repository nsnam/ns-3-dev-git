/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#ifndef COEX_HELPER_H
#define COEX_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include <string>

namespace ns3
{

/**
 * @ingroup coex
 * @brief install Coex arbitrators on nodes
 *
 */
class CoexHelper
{
  public:
    /**
     * Set the type of Coex Arbitrator(s) that CoexHelper::Install will create and
     * install on given node(s).
     *
     * @tparam Ts \deduced Argument types
     * @param arbitratorTypeId the TypeID of the Coex Arbitrator
     * @param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetArbitrator(std::string arbitratorTypeId, Ts&&... args);

    /**
     * This method creates and installs a CoexArbitrator with the attributes configured via the
     * SetArbitrator method on the given node. The CoexArbitrator should be installed on a node
     * before all the device Coex Managers.
     *
     * @param node the node on which to install a CoexArbitrator
     */
    void Install(Ptr<Node> node) const;

    /**
     * This method creates and installs a CoexArbitrator with the attributes configured via the
     * SetArbitrator method on all the nodes of the given container. The CoexArbitrator should be
     * installed on a node before all the device Coex Managers.
     *
     * @param c The NodeContainer holding the nodes on which to install a CoexArbitrator
     */
    void Install(const NodeContainer& c) const;

    /**
     * Helper to enable all coex log components with one statement
     * @param logLevel (optional) log level setting
     */
    static void EnableLogComponents(LogLevel logLevel = LOG_LEVEL_ALL);

  private:
    ObjectFactory m_factory; //!< factory for the CoexArbitrator
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
CoexHelper::SetArbitrator(std::string arbitratorTypeId, Ts&&... args)
{
    m_factory.SetTypeId(arbitratorTypeId);
    m_factory.Set(std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* COEX_HELPER_H */
