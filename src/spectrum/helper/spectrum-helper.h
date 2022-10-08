/*
 * Copyright (c) 2010 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef SPECTRUM_HELPER_H
#define SPECTRUM_HELPER_H

#include <ns3/attribute.h>
#include <ns3/net-device-container.h>
#include <ns3/node-container.h>
#include <ns3/object-factory.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/spectrum-propagation-loss-model.h>

#include <string>

namespace ns3
{

class SpectrumPhy;
class SpectrumChannel;
class Node;
class NetDevice;

/**
 * \ingroup spectrum
 * \brief Setup a SpectrumChannel
 */
class SpectrumChannelHelper
{
  public:
    /**
     * \brief Setup a default SpectrumChannel. The Default mode is:
     * Channel: "ns3::SingleModelSpectrumChannel",
     * PropagationDelay: "ns3::ConstantSpeedPropagationDelayModel", and
     * SpectrumPropagationLoss: "ns3::FriisSpectrumPropagationLossModel".
     *
     * \returns a Default-configured SpectrumChannelHelper
     */
    static SpectrumChannelHelper Default();

    /**
     * \tparam Ts \deduced Argument types
     * \param type the type of the SpectrumChannel to use
     * \param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetChannel(std::string type, Ts&&... args);
    /**
     * \tparam Ts \deduced Argument types
     * \param name the name of the model to set
     * \param [in] args Name and AttributeValue pairs to set.
     *
     * Add a new single-frequency propagation loss model to this channel helper.
     */
    template <typename... Ts>
    void AddPropagationLoss(std::string name, Ts&&... args);

    /**
     * Add a new single-frequency propagation loss model instance to this channel helper.
     *
     * \param m a pointer to the instance of the propagation loss model
     */
    void AddPropagationLoss(Ptr<PropagationLossModel> m);

    /**
     * \tparam Ts \deduced Argument types
     * \param name the name of the model to set
     * \param [in] args Name and AttributeValue pairs to set.
     *
     * Add a new frequency-dependent propagation loss model to this channel helper.
     */
    template <typename... Ts>
    void AddSpectrumPropagationLoss(std::string name, Ts&&... args);

    /**
     * Add a new frequency-dependent propagation loss model instance to this channel helper.
     *
     * \param m a pointer to the instance of the propagation loss model
     */
    void AddSpectrumPropagationLoss(Ptr<SpectrumPropagationLossModel> m);

    /**
     * \tparam Ts \deduced Argument types
     * \param name the name of the model to set
     * \param [in] args Name and AttributeValue pairs to set.
     *
     * Configure a propagation delay for this channel.
     */
    template <typename... Ts>
    void SetPropagationDelay(std::string name, Ts&&... args);

    /**
     * \returns a new channel
     *
     * Create a channel based on the configuration parameters set previously.
     */
    Ptr<SpectrumChannel> Create() const;

  private:
    Ptr<SpectrumPropagationLossModel>
        m_spectrumPropagationLossModel;               //!< Spectrum propagation loss model
    Ptr<PropagationLossModel> m_propagationLossModel; //!< Propagation loss model
    ObjectFactory m_propagationDelay;                 //!< Propagation delay
    ObjectFactory m_channel;                          //!< Channel
};

/**
 * \ingroup spectrum
 *
 * Create and configure several SpectrumPhy instances and connect them to a channel.
 */
class SpectrumPhyHelper
{
  public:
    /**
     * \tparam Ts \deduced Argument types
     * \param name the type of SpectrumPhy to use
     * \param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetPhy(std::string name, Ts&&... args);

    /**
     * set the channel that will be used by SpectrumPhy instances created by this helper
     *
     * @param channel
     */
    void SetChannel(Ptr<SpectrumChannel> channel);

    /**
     * set the channel that will be used by SpectrumPhy instances created by this helper
     *
     * @param channelName
     */
    void SetChannel(std::string channelName);

    /**
     * \param name the name of the attribute to set
     * \param v the value of the attribute
     *
     * Set an attribute of the SpectrumPhy instances to be created
     */
    void SetPhyAttribute(std::string name, const AttributeValue& v);

    /**
     *
     * @param node
     * @param device
     *
     * @return a  newly created SpectrumPhy instance
     */
    Ptr<SpectrumPhy> Create(Ptr<Node> node, Ptr<NetDevice> device) const;

  private:
    ObjectFactory m_phy;            //!< Object factory for the phy objects
    Ptr<SpectrumChannel> m_channel; //!< Channel
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
SpectrumChannelHelper::SetChannel(std::string type, Ts&&... args)
{
    m_channel.SetTypeId(type);
    m_channel.Set(std::forward<Ts>(args)...);
}

template <typename... Ts>
void
SpectrumChannelHelper::AddPropagationLoss(std::string name, Ts&&... args)
{
    ObjectFactory factory(name, std::forward<Ts>(args)...);
    Ptr<PropagationLossModel> m = factory.Create<PropagationLossModel>();
    AddPropagationLoss(m);
}

template <typename... Ts>
void
SpectrumChannelHelper::AddSpectrumPropagationLoss(std::string name, Ts&&... args)
{
    ObjectFactory factory(name, std::forward<Ts>(args)...);
    Ptr<SpectrumPropagationLossModel> m = factory.Create<SpectrumPropagationLossModel>();
    AddSpectrumPropagationLoss(m);
}

template <typename... Ts>
void
SpectrumChannelHelper::SetPropagationDelay(std::string name, Ts&&... args)
{
    m_propagationDelay = ObjectFactory(name, std::forward<Ts>(args)...);
}

template <typename... Ts>
void
SpectrumPhyHelper::SetPhy(std::string name, Ts&&... args)
{
    m_phy.SetTypeId(name);
    m_phy.Set(std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* SPECTRUM_HELPER_H */
