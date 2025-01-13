/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef LTE_RADIO_BEARER_INFO_H
#define LTE_RADIO_BEARER_INFO_H

#include "eps-bearer.h"
#include "lte-rrc-sap.h"

#include "ns3/ipv4-address.h"
#include "ns3/object.h"
#include "ns3/pointer.h"

namespace ns3
{

class LteRlc;
class LtePdcp;

/**
 * store information on active radio bearer instance
 *
 */
class LteRadioBearerInfo : public Object
{
  public:
    LteRadioBearerInfo();
    ~LteRadioBearerInfo() override;
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    Ptr<LteRlc> m_rlc;   ///< RLC
    Ptr<LtePdcp> m_pdcp; ///< PDCP
};

/**
 * store information on active signaling radio bearer instance
 *
 */
class LteSignalingRadioBearerInfo : public LteRadioBearerInfo
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    uint8_t m_srbIdentity;                                  ///< SRB identity
    LteRrcSap::LogicalChannelConfig m_logicalChannelConfig; ///< logical channel config
};

/**
 * store information on active data radio bearer instance
 *
 */
class LteDataRadioBearerInfo : public LteRadioBearerInfo
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    EpsBearer m_epsBearer;                                  ///< EPS bearer
    uint8_t m_epsBearerIdentity;                            ///< EPS bearer identity
    uint8_t m_drbIdentity;                                  ///< DRB identity
    LteRrcSap::RlcConfig m_rlcConfig;                       ///< RLC config
    uint8_t m_logicalChannelIdentity;                       ///< logical channel identity
    LteRrcSap::LogicalChannelConfig m_logicalChannelConfig; ///< logical channel config
    uint32_t m_gtpTeid; /**< S1-bearer GTP tunnel endpoint identifier, see 36.423 9.2.1 */
    Ipv4Address m_transportLayerAddress; /**< IP Address of the SGW, see 36.423 9.2.1 */
};

} // namespace ns3

#endif // LTE_RADIO_BEARER_INFO_H
