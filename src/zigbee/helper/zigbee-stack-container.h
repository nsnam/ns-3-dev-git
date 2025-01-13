/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_STACK_CONTAINER_H
#define ZIGBEE_STACK_CONTAINER_H

#include "ns3/zigbee-stack.h"

#include <stdint.h>
#include <vector>

namespace ns3
{
namespace zigbee
{

/**
 * Holds a vector of ns3::ZigbeeStack pointers
 *
 * Typically ZigbeeStacks are installed on top of a pre-existing NetDevice
 * (an LrWpanNetDevice) which on itself has already being aggregated to a node.
 * A ZigbeeHelper Install method takes a LrWpanNetDeviceContainer.
 * For each of the LrWpanNetDevice in the LrWpanNetDeviceContainer
 * the helper will instantiate a ZigbeeStack, connect the necessary hooks between
 * the MAC (LrWpanMac) and the NWK (ZigbeeNwk) and install it to the node.
 * For each of these ZigbeeStacks, the helper also adds the ZigbeeStack into a Container
 * for later use by the caller. This is that container used to hold the Ptr<ZigbeeStack> which are
 * instantiated by the ZigbeeHelper.
 */
class ZigbeeStackContainer
{
  public:
    typedef std::vector<Ptr<ZigbeeStack>>::const_iterator
        Iterator; //!< The iterator used in this Container

    /**
     * The default constructor, create an empty ZigbeeStackContainer
     */
    ZigbeeStackContainer();
    /**
     * Create a ZigbeeStackContainer with exactly one ZigbeeStack that has previously
     * been instantiated
     *
     * @param stack A ZigbeeStack to add to the container
     */
    ZigbeeStackContainer(Ptr<ZigbeeStack> stack);
    /**
     * Create a ZigbeeStackContainer with exactly one device which has been
     * previously instantiated and assigned a name using the Object name
     * service.  This ZigbeeStack is specified by its assigned name.
     *
     * @param stackName The name of the ZigbeeStack to add to the container
     *
     * Create a ZigbeeStackContainer with exactly one device
     */
    ZigbeeStackContainer(std::string stackName);
    /**
     * Get and iterator which refers to the first ZigbeeStack in the container.
     * @return An iterator referring to the first ZigbeeStack in the container.
     */
    Iterator Begin() const;
    /**
     * Get an iterator which indicates past the last ZigbeeStack in the container.
     * @return An iterator referring to the past the last ZigbeeStack in the container.
     */
    Iterator End() const;
    /**
     * Get the number of stacks present in the stack container.
     * @return The number of stacks in the container.
     */
    uint32_t GetN() const;
    /**
     * Get a stack element from the container.
     * @param i The element number in the container
     * @return The zigbee stack element matching the i index in the container.
     */
    Ptr<ZigbeeStack> Get(uint32_t i) const;
    /**
     * Append the contents of another ZigbeeStackContainer to the end of
     * this container.
     *
     * @param other The ZigbeeStackContainer to append.
     */
    void Add(ZigbeeStackContainer other);
    /**
     * Append a single Ptr<ZigbeeStack> to this container.
     *
     * @param stack The Ptr<ZigbeeStack> to append.
     */
    void Add(Ptr<ZigbeeStack> stack);
    /**
     * Append to this container the single Ptr<ZigbeeStack> referred to
     * via its object name service registered name.
     *
     * @param stackName The name of the ZigbeeStack object to add to the container.
     */
    void Add(std::string stackName);

  private:
    std::vector<Ptr<ZigbeeStack>> m_stacks; //!< ZigbeeStack smart pointers
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_STACK_CONTAINER_H */
