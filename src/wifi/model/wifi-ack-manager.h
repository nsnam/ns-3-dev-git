/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_ACK_MANAGER_H
#define WIFI_ACK_MANAGER_H

#include "wifi-acknowledgment.h"

#include "ns3/object.h"

#include <memory>

namespace ns3
{

class WifiTxParameters;
class WifiMpdu;
class WifiPsdu;
class WifiMac;
class WifiRemoteStationManager;

/**
 * @ingroup wifi
 *
 * WifiAckManager is an abstract base class. Each subclass defines a logic
 * to select the acknowledgment method for a given frame.
 */
class WifiAckManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    WifiAckManager();
    ~WifiAckManager() override;

    /**
     * Set the MAC which is using this Acknowledgment Manager
     *
     * @param mac a pointer to the MAC
     */
    void SetWifiMac(Ptr<WifiMac> mac);
    /**
     * Set the ID of the link this Acknowledgment Manager is associated with.
     *
     * @param linkId the ID of the link this Acknowledgment Manager is associated with
     */
    void SetLinkId(uint8_t linkId);

    /**
     * Set the QoS Ack policy for the given MPDU, which must be a QoS data frame.
     *
     * @param item the MPDU
     * @param acknowledgment the WifiAcknowledgment object storing the QoS Ack policy to set
     */
    static void SetQosAckPolicy(Ptr<WifiMpdu> item, const WifiAcknowledgment* acknowledgment);

    /**
     * Set the QoS Ack policy for the given PSDU, which must include at least a QoS data frame.
     *
     * @param psdu the PSDU
     * @param acknowledgment the WifiAcknowledgment object storing the QoS Ack policy to set
     */
    static void SetQosAckPolicy(Ptr<WifiPsdu> psdu, const WifiAcknowledgment* acknowledgment);

    /**
     * Determine the acknowledgment method to use if the given MPDU is added to the current
     * frame. Return a null pointer if the acknowledgment method is unchanged or the new
     * acknowledgment method otherwise.
     *
     * @param mpdu the MPDU to be added to the current frame
     * @param txParams the current TX parameters for the current frame
     * @return a null pointer if the acknowledgment method is unchanged or the new
     *         acknowledgment method otherwise
     */
    virtual std::unique_ptr<WifiAcknowledgment> TryAddMpdu(Ptr<const WifiMpdu> mpdu,
                                                           const WifiTxParameters& txParams) = 0;

    /**
     * Determine the acknowledgment method to use if the given MSDU is aggregated to the current
     * frame. Return a null pointer if the acknowledgment method is unchanged or the new
     * acknowledgment method otherwise.
     *
     * @param msdu the MSDU to be aggregated to the current frame
     * @param txParams the current TX parameters for the current frame
     * @return a null pointer if the acknowledgment method is unchanged or the new
     *         acknowledgment method otherwise
     */
    virtual std::unique_ptr<WifiAcknowledgment> TryAggregateMsdu(
        Ptr<const WifiMpdu> msdu,
        const WifiTxParameters& txParams) = 0;

  protected:
    void DoDispose() override;

    /**
     * @return the remote station manager operating on our link
     */
    Ptr<WifiRemoteStationManager> GetWifiRemoteStationManager() const;

    Ptr<WifiMac> m_mac; //!< MAC which is using this Acknowledgment Manager
    uint8_t m_linkId;   //!< ID of the link this Acknowledgment Manager is operating on
};

} // namespace ns3

#endif /* WIFI_ACK_MANAGER_H */
