/*
 * Copyright (c) 2007-2009 Strasbourg University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: David Gross <gdavid.devel@gmail.com>
 */

#ifndef IPV6_OPTION_DEMUX_H
#define IPV6_OPTION_DEMUX_H

#include "ns3/object.h"
#include "ns3/ptr.h"

#include <list>

namespace ns3
{

class Ipv6Option;
class Node;

/**
 * @ingroup ipv6HeaderExt
 *
 * @brief IPv6 Option Demux.
 */
class Ipv6OptionDemux : public Object
{
  public:
    /**
     * @brief The interface ID.
     * @return type ID
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor.
     */
    Ipv6OptionDemux();

    /**
     * @brief Destructor.
     */
    ~Ipv6OptionDemux() override;

    /**
     * @brief Set the node.
     * @param node the node to set
     */
    void SetNode(Ptr<Node> node);

    /**
     * @brief Insert a new IPv6 Option.
     * @param option the option to insert
     */
    void Insert(Ptr<Ipv6Option> option);

    /**
     * @brief Get the option corresponding to optionNumber.
     * @param optionNumber the option number of the option to retrieve
     * @return a matching IPv6 option
     */
    Ptr<Ipv6Option> GetOption(int optionNumber);

    /**
     * @brief Remove an option from this demux.
     * @param option pointer on the option to remove
     */
    void Remove(Ptr<Ipv6Option> option);

  protected:
    /**
     * @brief Dispose this object.
     */
    void DoDispose() override;

  private:
    /**
     * @brief Container of the IPv6 Options types.
     */
    typedef std::list<Ptr<Ipv6Option>> Ipv6OptionList_t;

    /**
     * @brief List of IPv6 Options supported.
     */
    Ipv6OptionList_t m_options;

    /**
     * @brief The node.
     */
    Ptr<Node> m_node;
};

} /* namespace ns3 */

#endif /* IPV6_OPTION_DEMUX_H */
