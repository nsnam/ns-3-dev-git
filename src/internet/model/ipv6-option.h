/*
 * Copyright (c) 2007-2009 Strasbourg University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: David Gross <gdavid.devel@gmail.com>
 */

#ifndef IPV6_OPTION_H
#define IPV6_OPTION_H

#include "ipv6-header.h"
#include "ipv6-interface.h"

#include "ns3/buffer.h"
#include "ns3/ipv6-address.h"
#include "ns3/node.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"

#include <map>

namespace ns3
{

/**
 * @ingroup ipv6HeaderExt
 *
 * @brief IPv6 Option base
 *
 * If you want to implement a new IPv6 option, all you have to do is
 * implement a subclass of this class and add it to an Ipv6OptionDemux.
 */
class Ipv6Option : public Object
{
  public:
    /**
     * @brief Get the type identificator.
     * @return type identificator
     */
    static TypeId GetTypeId();

    /**
     * @brief Destructor.
     */
    ~Ipv6Option() override;

    /**
     * @brief Set the node.
     * @param node the node to set
     */
    void SetNode(Ptr<Node> node);

    /**
     * @brief Get the option number.
     * @return option number
     */
    virtual uint8_t GetOptionNumber() const = 0;

    /**
     * @brief Process method
     *
     * Called from Ipv6L3Protocol::Receive.
     * @param packet the packet
     * @param offset the offset of the extension to process
     * @param ipv6Header the IPv6 header of packet received
     * @param isDropped if the packet must be dropped
     * @return the processed size
     */
    virtual uint8_t Process(Ptr<Packet> packet,
                            uint8_t offset,
                            const Ipv6Header& ipv6Header,
                            bool& isDropped) = 0;

  private:
    /**
     * @brief The node.
     */
    Ptr<Node> m_node;
};

/**
 * @ingroup ipv6HeaderExt
 *
 * @brief IPv6 Option Pad1
 */
class Ipv6OptionPad1 : public Ipv6Option
{
  public:
    /**
     * @brief Pad1 option number.
     */
    static const uint8_t OPT_NUMBER = 0;

    /**
     * @brief Get the type identificator.
     * @return type identificator
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor.
     */
    Ipv6OptionPad1();

    /**
     * @brief Destructor.
     */
    ~Ipv6OptionPad1() override;

    /**
     * @brief Get the option number.
     * @return option number
     */
    uint8_t GetOptionNumber() const override;

    /**
     * @brief Process method
     *
     * Called from Ipv6L3Protocol::Receive.
     * @param packet the packet
     * @param offset the offset of the extension to process
     * @param ipv6Header the IPv6 header of packet received
     * @param isDropped if the packet must be dropped
     * @return the processed size
     */
    uint8_t Process(Ptr<Packet> packet,
                    uint8_t offset,
                    const Ipv6Header& ipv6Header,
                    bool& isDropped) override;
};

/**
 * @ingroup ipv6HeaderExt
 *
 * @brief IPv6 Option Padn
 */
class Ipv6OptionPadn : public Ipv6Option
{
  public:
    /**
     * @brief PadN option number.
     */
    static const uint8_t OPT_NUMBER = 60;

    /**
     * @brief Get the type identificator.
     * @return type identificator
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor.
     */
    Ipv6OptionPadn();

    /**
     * @brief Destructor.
     */
    ~Ipv6OptionPadn() override;

    /**
     * @brief Get the option number.
     * @return option number
     */
    uint8_t GetOptionNumber() const override;

    /**
     * @brief Process method
     *
     * Called from Ipv6L3Protocol::Receive.
     * @param packet the packet
     * @param offset the offset of the extension to process
     * @param ipv6Header the IPv6 header of packet received
     * @param isDropped if the packet must be dropped
     * @return the processed size
     */
    uint8_t Process(Ptr<Packet> packet,
                    uint8_t offset,
                    const Ipv6Header& ipv6Header,
                    bool& isDropped) override;
};

/**
 * @ingroup ipv6HeaderExt
 *
 * @brief IPv6 Option Jumbogram
 */
class Ipv6OptionJumbogram : public Ipv6Option
{
  public:
    /**
     * @brief Jumbogram option number.
     */
    static const uint8_t OPT_NUMBER = 44;

    /**
     * @brief Get the type identificator.
     * @return type identificator
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor.
     */
    Ipv6OptionJumbogram();

    /**
     * @brief Destructor.
     */
    ~Ipv6OptionJumbogram() override;

    /**
     * @brief Get the option number.
     * @return option number
     */
    uint8_t GetOptionNumber() const override;

    /**
     * @brief Process method
     * Called from Ipv6L3Protocol::Receive.
     * @param packet the packet
     * @param offset the offset of the extension to process
     * @param ipv6Header the IPv6 header of packet received
     * @param isDropped if the packet must be dropped
     * @return the processed size
     */
    uint8_t Process(Ptr<Packet> packet,
                    uint8_t offset,
                    const Ipv6Header& ipv6Header,
                    bool& isDropped) override;
};

/**
 * @ingroup ipv6HeaderExt
 *
 * @brief IPv6 Option Router Alert
 */
class Ipv6OptionRouterAlert : public Ipv6Option
{
  public:
    /**
     * @brief Router alert option number.
     */
    static const uint8_t OPT_NUMBER = 43;

    /**
     * @brief Get the type identificator.
     * @return type identificator
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor.
     */
    Ipv6OptionRouterAlert();

    /**
     * @brief Destructor.
     */
    ~Ipv6OptionRouterAlert() override;

    /**
     * @brief Get the option number.
     * @return option number
     */
    uint8_t GetOptionNumber() const override;

    /**
     * @brief Process method
     *
     * Called from Ipv6L3Protocol::Receive.
     * @param packet the packet
     * @param offset the offset of the extension to process
     * @param ipv6Header the IPv6 header of packet received
     * @param isDropped if the packet must be dropped
     * @return the processed size
     */
    uint8_t Process(Ptr<Packet> packet,
                    uint8_t offset,
                    const Ipv6Header& ipv6Header,
                    bool& isDropped) override;
};

} /* namespace ns3 */

#endif /* IPV6_OPTION_H */
