/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef LTE_UE_CMAC_SAP_H
#define LTE_UE_CMAC_SAP_H

#include "ns3/packet.h"

namespace ns3
{

class LteMacSapUser;

/**
 * Service Access Point (SAP) offered by the UE MAC to the UE RRC
 *
 * This is the MAC SAP Provider, i.e., the part of the SAP that contains the MAC methods called by
 * the RRC
 */
class LteUeCmacSapProvider
{
  public:
    virtual ~LteUeCmacSapProvider();

    /// RachConfig structure
    struct RachConfig
    {
        uint8_t numberOfRaPreambles;  ///< number of RA preambles
        uint8_t preambleTransMax;     ///< preamble transmit maximum
        uint8_t raResponseWindowSize; ///< RA response window size
        uint8_t connEstFailCount;     ///< the counter value for T300 timer expiration
    };

    /**
     * Configure RACH function
     *
     * @param rc the RACH config
     */
    virtual void ConfigureRach(RachConfig rc) = 0;

    /**
     * tell the MAC to start a contention-based random access procedure,
     * e.g., to perform RRC connection establishment
     *
     */
    virtual void StartContentionBasedRandomAccessProcedure() = 0;

    /**
     * tell the MAC to start a non-contention-based random access
     * procedure, e.g., as a consequence of handover
     *
     * @param rnti
     * @param rapId Random Access Preamble Identifier
     * @param prachMask
     */
    virtual void StartNonContentionBasedRandomAccessProcedure(uint16_t rnti,
                                                              uint8_t rapId,
                                                              uint8_t prachMask) = 0;

    /// LogicalChannelConfig structure
    struct LogicalChannelConfig
    {
        uint8_t priority;                ///< priority
        uint16_t prioritizedBitRateKbps; ///< prioritize bit rate Kbps
        uint16_t bucketSizeDurationMs;   ///< bucket size duration ms
        uint8_t logicalChannelGroup;     ///< logical channel group
    };

    /**
     * add a new Logical Channel (LC)
     *
     * @param lcId the ID of the LC
     * @param lcConfig the LC configuration provided by the RRC
     * @param msu the corresponding LteMacSapUser
     */
    virtual void AddLc(uint8_t lcId, LogicalChannelConfig lcConfig, LteMacSapUser* msu) = 0;

    /**
     * remove an existing LC
     *
     * @param lcId
     */
    virtual void RemoveLc(uint8_t lcId) = 0;

    /**
     * reset the MAC
     *
     */
    virtual void Reset() = 0;

    /**
     *
     * @param rnti the cell-specific UE identifier
     */
    virtual void SetRnti(uint16_t rnti) = 0;

    /**
     * @brief Notify MAC about the successful RRC connection
     * establishment.
     */
    virtual void NotifyConnectionSuccessful() = 0;

    /**
     * @brief A method call by UE RRC to communicate the IMSI to the UE MAC
     * @param imsi the IMSI of the UE
     */
    virtual void SetImsi(uint64_t imsi) = 0;
};

/**
 * Service Access Point (SAP) offered by the UE MAC to the UE RRC
 *
 * This is the MAC SAP User, i.e., the part of the SAP that contains the RRC methods called by the
 * MAC
 */
class LteUeCmacSapUser
{
  public:
    virtual ~LteUeCmacSapUser();

    /**
     *
     *
     * @param rnti the T-C-RNTI, which will eventually become the C-RNTI after contention resolution
     */
    virtual void SetTemporaryCellRnti(uint16_t rnti) = 0;

    /**
     * Notify the RRC that the MAC Random Access procedure completed successfully
     *
     */
    virtual void NotifyRandomAccessSuccessful() = 0;

    /**
     * Notify the RRC that the MAC Random Access procedure failed
     *
     */
    virtual void NotifyRandomAccessFailed() = 0;
};

} // namespace ns3

#endif // LTE_UE_CMAC_SAP_H
