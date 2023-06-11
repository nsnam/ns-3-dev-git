/*
 * Copyright (c) 2016
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_MAC_HELPER_H
#define WIFI_MAC_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/qos-utils.h"
#include "ns3/wifi-standards.h"

#include <map>

namespace ns3
{

class WifiMac;
class WifiNetDevice;

/**
 * @brief create MAC layers for a ns3::WifiNetDevice.
 *
 * This class can create MACs of type ns3::ApWifiMac, ns3::StaWifiMac and ns3::AdhocWifiMac.
 * Its purpose is to allow a WifiHelper to configure and install WifiMac objects on a collection
 * of nodes. The WifiMac objects themselves are mainly composed of TxMiddle, RxMiddle,
 * ChannelAccessManager, FrameExchangeManager, WifiRemoteStationManager, MpduAggregator and
 * MsduAggregartor objects, so this helper offers the opportunity to configure attribute values away
 * from their default values, on a per-NodeContainer basis. By default, it creates an Adhoc MAC
 * layer without QoS. Typically, it is used to set type and attribute values, then hand this object
 * over to the WifiHelper that finishes the job of installing.
 *
 * This class may be further subclassed (WaveMacHelper is an example of this).
 *
 */
class WifiMacHelper
{
  public:
    /**
     * Create a WifiMacHelper to make life easier for people who want to
     * work with Wifi MAC layers.
     */
    WifiMacHelper();
    /**
     * Destroy a WifiMacHelper.
     */
    virtual ~WifiMacHelper();

    /**
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param type the type of ns3::WifiMac to create.
     * @param args A sequence of name-value pairs of the attributes to set.
     *
     * All the attributes specified in this method should exist
     * in the requested MAC.
     */
    template <typename... Args>
    void SetType(std::string type, Args&&... args);

    /**
     * Helper function used to create and set the Txop object.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetDcf(Args&&... args);

    /**
     * Helper function used to create and set the QosTxop object corresponding to the given AC.
     *
     * @param aci the AC index
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetEdca(AcIndex aci, Args&&... args);

    /**
     * Helper function used to set the Channel Access Manager object.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetChannelAccessManager(Args&&... args);

    /**
     * Helper function used to set the Frame Exchange Manager object.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetFrameExchangeManager(Args&&... args);

    /**
     * Helper function used to set the Association Manager.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param type the type of Association Manager
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetAssocManager(std::string type, Args&&... args);

    /**
     * Helper function used to set the MAC queue scheduler.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param type the type of MAC queue scheduler
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetMacQueueScheduler(std::string type, Args&&... args);

    /**
     * Helper function used to set the Protection Manager.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param type the type of Protection Manager
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetProtectionManager(std::string type, Args&&... args);

    /**
     * Helper function used to set the Acknowledgment Manager.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param type the type of Acknowledgment Manager
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetAckManager(std::string type, Args&&... args);

    /**
     * Helper function used to set the Multi User Scheduler that can be aggregated
     * to an HE AP's MAC.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param type the type of Multi User Scheduler
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetMultiUserScheduler(std::string type, Args&&... args);

    /**
     * Helper function used to set the EMLSR Manager that can be installed on an EHT non-AP MLD.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param type the type of EMLSR Manager
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetEmlsrManager(std::string type, Args&&... args);

    /**
     * Helper function used to set the AP EMLSR Manager that can be installed on an EHT AP MLD.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param type the type of AP EMLSR Manager
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetApEmlsrManager(std::string type, Args&&... args);

    /**
     * Helper function used to set the GCR Manager that can be installed on a QoS AP.
     *
     * @tparam Args \deduced Template type parameter pack for the sequence of name-value pairs.
     * @param type the type of GCR Manager
     * @param args A sequence of name-value pairs of the attributes to set.
     */
    template <typename... Args>
    void SetGcrManager(std::string type, Args&&... args);

    /**
     * @param device the device within which the MAC object will reside
     * @param standard the standard to configure during installation
     * @returns a new MAC object.
     *
     * This allows the ns3::WifiHelper class to create MAC objects from ns3::WifiHelper::Install.
     */
    virtual Ptr<WifiMac> Create(Ptr<WifiNetDevice> device, WifiStandard standard) const;

  protected:
    ObjectFactory m_mac;                                     ///< MAC object factory
    ObjectFactory m_dcf;                                     ///< Txop (DCF) object factory
    std::map<AcIndex, ObjectFactory, std::greater<>> m_edca; ///< QosTxop (EDCA) object factories
    ObjectFactory m_channelAccessManager; ///< Channel Access Manager object factory
    ObjectFactory m_frameExchangeManager; ///< Frame Exchange Manager object factory
    ObjectFactory m_assocManager;         ///< Association Manager
    ObjectFactory m_queueScheduler;       ///< MAC queue scheduler
    ObjectFactory m_protectionManager;    ///< Factory to create a protection manager
    ObjectFactory m_ackManager;           ///< Factory to create an acknowledgment manager
    ObjectFactory m_muScheduler;          ///< Multi-user Scheduler object factory
    ObjectFactory m_emlsrManager;         ///< EMLSR Manager object factory
    ObjectFactory m_apEmlsrManager;       ///< AP EMLSR Manager object factory
    ObjectFactory m_gcrManager;           ///< GCR Manager object factory
};

} // namespace ns3

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

namespace ns3
{

template <typename... Args>
void
WifiMacHelper::SetType(std::string type, Args&&... args)
{
    m_mac.SetTypeId(type);
    m_mac.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetDcf(Args&&... args)
{
    m_dcf.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetEdca(AcIndex aci, Args&&... args)
{
    auto it = m_edca.find(aci);
    NS_ASSERT_MSG(it != m_edca.cend(), "No object factory for " << aci);
    it->second.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetChannelAccessManager(Args&&... args)
{
    m_channelAccessManager.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetFrameExchangeManager(Args&&... args)
{
    m_frameExchangeManager.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetAssocManager(std::string type, Args&&... args)
{
    m_assocManager.SetTypeId(type);
    m_assocManager.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetMacQueueScheduler(std::string type, Args&&... args)
{
    m_queueScheduler.SetTypeId(type);
    m_queueScheduler.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetProtectionManager(std::string type, Args&&... args)
{
    m_protectionManager.SetTypeId(type);
    m_protectionManager.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetAckManager(std::string type, Args&&... args)
{
    m_ackManager.SetTypeId(type);
    m_ackManager.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetMultiUserScheduler(std::string type, Args&&... args)
{
    m_muScheduler.SetTypeId(type);
    m_muScheduler.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetEmlsrManager(std::string type, Args&&... args)
{
    m_emlsrManager.SetTypeId(type);
    m_emlsrManager.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetApEmlsrManager(std::string type, Args&&... args)
{
    m_apEmlsrManager.SetTypeId(type);
    m_apEmlsrManager.Set(args...);
}

template <typename... Args>
void
WifiMacHelper::SetGcrManager(std::string type, Args&&... args)
{
    m_gcrManager.SetTypeId(type);
    m_gcrManager.Set(args...);
}

} // namespace ns3

#endif /* WIFI_MAC_HELPER_H */
