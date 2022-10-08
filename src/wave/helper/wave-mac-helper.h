/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 * Copyright (c) 2013 Dalian University of Technology
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 * Author: Junling Bu <linlinjavaer@gmail.com>
 */
#ifndef WAVE_MAC_HELPER_H
#define WAVE_MAC_HELPER_H
#include "ns3/wifi-mac-helper.h"

namespace ns3
{

/**
 * \ingroup wave
 * \brief Nqos Wave Mac Helper class
 */
class NqosWaveMacHelper : public WifiMacHelper
{
  public:
    /**
     * Create a NqosWaveMacHelper to make life easier for people who want to
     * work with non-QOS Wave MAC layers.
     */
    NqosWaveMacHelper();

    /**
     * Destroy a NqosWaveMacHelper.
     */
    ~NqosWaveMacHelper() override;
    /**
     * Create a mac helper in a default working state.
     * i.e., this is an ocb mac by default.
     * \returns NqosWaveMacHelper
     */
    static NqosWaveMacHelper Default();
    /**
     * \tparam Ts \deduced Argument types
     * \param type the type of ns3::WifiMac to create.
     * \param [in] args Name and AttributeValue pairs to set.
     *
     * All the attributes specified in this method should exist
     * in the requested mac.
     *
     * note: Here we require users set type with OcbWifiMac or its
     * subclass, otherwise it will become error
     */
    template <typename... Ts>
    void SetType(std::string type, Ts&&... args);
};

/**
 * \ingroup wave
 * \brief Qos Wave Mac Helper class
 */
class QosWaveMacHelper : public WifiMacHelper
{
  public:
    /**
     * Create a QosWaveMacHelper that is used to make life easier when working
     * with Wifi 802.11p devices using a QOS MAC layer.
     */
    QosWaveMacHelper();

    /**
     * Destroy a QosWaveMacHelper
     */
    ~QosWaveMacHelper() override;

    /**
     * Create a mac helper in a default working state.
     * \return A mac helper
     */
    static QosWaveMacHelper Default();

    /**
     * \tparam Ts \deduced Argument types
     * \param type the type of ns3::WifiMac to create.
     * \param [in] args Name and AttributeValue pairs to set.
     *
     * All the attributes specified in this method should exist
     * in the requested mac.
     *
     * note: Here we require users set type with OcbWifiMac or its
     * subclass, otherwise it will become error
     */
    template <typename... Ts>
    void SetType(std::string type, Ts&&... args);
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
NqosWaveMacHelper::SetType(std::string type, Ts&&... args)
{
    if (type != "ns3::OcbWifiMac")
    {
        NS_FATAL_ERROR("QosWaveMacHelper shall set OcbWifiMac");
    }
    WifiMacHelper::SetType("ns3::OcbWifiMac", std::forward<Ts>(args)...);
}

template <typename... Ts>
void
QosWaveMacHelper::SetType(std::string type, Ts&&... args)
{
    if (type != "ns3::OcbWifiMac")
    {
        NS_FATAL_ERROR("QosWaveMacHelper shall set OcbWifiMac");
    }
    WifiMacHelper::SetType("ns3::OcbWifiMac", std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* WAVE_MAC_HELPER_H */
