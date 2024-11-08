/*
 *  Copyright (c) 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *         Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 *
 */

#ifndef WIMAX_CS_PARAMETERS_H
#define WIMAX_CS_PARAMETERS_H

#include "ipcs-classifier-record.h"
#include "wimax-tlv.h"

namespace ns3
{

/**
 * @ingroup wimax
 * @brief CsParameters class
 */
class CsParameters
{
  public:
    /// Action enumeration
    enum Action
    {
        ADD = 0,
        REPLACE = 1,
        DELETE = 2
    };

    CsParameters();
    ~CsParameters();
    /**
     * @brief creates a convergence sub-layer parameters from a tlv
     * @param tlv the TLV
     */
    CsParameters(Tlv tlv);
    /**
     * @brief creates a convergence sub-layer parameters from an ipcs classifier record
     * @param classifierDscAction the DCS action type
     * @param classifier the IPCS classifier record
     */
    CsParameters(Action classifierDscAction, IpcsClassifierRecord classifier);
    /**
     * @brief sets the dynamic service classifier action to ADD, Change or delete. Only ADD is
     * supported
     * @param action the action enumeration
     */
    void SetClassifierDscAction(Action action);
    /**
     * @brief sets the packet classifier rules
     * @param packetClassifierRule the IPCS classifier record
     */
    void SetPacketClassifierRule(IpcsClassifierRecord packetClassifierRule);
    /**
     * @return the  dynamic service classifier action
     */
    Action GetClassifierDscAction() const;
    /**
     * @return the  the packet classifier rules
     */
    IpcsClassifierRecord GetPacketClassifierRule() const;
    /**
     * @brief creates a tlv from the classifier record
     * @return the created tlv
     */
    Tlv ToTlv() const;

  private:
    Action m_classifierDscAction;                ///< classifier DSC action
    IpcsClassifierRecord m_packetClassifierRule; ///< packet classifier rule
};

} // namespace ns3
#endif /* WIMAX_CS_PARAMETERS_H */
