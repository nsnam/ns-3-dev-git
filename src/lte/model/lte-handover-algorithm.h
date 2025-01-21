/*
 * Copyright (c) 2013 Budiarto Herman
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#ifndef LTE_HANDOVER_ALGORITHM_H
#define LTE_HANDOVER_ALGORITHM_H

#include "lte-rrc-sap.h"

#include "ns3/object.h"

namespace ns3
{

class LteHandoverManagementSapUser;
class LteHandoverManagementSapProvider;

/**
 * @brief The abstract base class of a handover algorithm that operates using
 *        the Handover Management SAP interface.
 *
 * Handover algorithm receives measurement reports from an eNodeB RRC instance
 * and tells the eNodeB RRC instance when to do a handover.
 *
 * This class is an abstract class intended to be inherited by subclasses that
 * implement its virtual methods. By inheriting from this abstract class, the
 * subclasses gain the benefits of being compatible with the LteEnbNetDevice
 * class, being accessible using namespace-based access through ns-3 Config
 * subsystem, and being installed and configured by LteHelper class (see
 * LteHelper::SetHandoverAlgorithmType and
 * LteHelper::SetHandoverAlgorithmAttribute methods).
 *
 * The communication with the eNodeB RRC instance is done through the *Handover
 * Management SAP* interface. The handover algorithm instance corresponds to the
 * "provider" part of this interface, while the eNodeB RRC instance takes the
 * role of the "user" part. The following code skeleton establishes the
 * connection between both instances:
 *
 *     Ptr<LteEnbRrc> u = ...;
 *     Ptr<LteHandoverAlgorithm> p = ...;
 *     u->SetLteHandoverManagementSapProvider (p->GetLteHandoverManagementSapProvider ());
 *     p->SetLteHandoverManagementSapUser (u->GetLteHandoverManagementSapUser ());
 *
 * However, user rarely needs to use the above code, since it has already been
 * taken care by LteHelper::InstallEnbDevice.
 *
 * \sa LteHandoverManagementSapProvider, LteHandoverManagementSapUser
 */
class LteHandoverAlgorithm : public Object
{
  public:
    LteHandoverAlgorithm();
    ~LteHandoverAlgorithm() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Set the "user" part of the Handover Management SAP interface that
     *        this handover algorithm instance will interact with.
     * @param s a reference to the "user" part of the interface, typically a
     *          member of an LteEnbRrc instance
     */
    virtual void SetLteHandoverManagementSapUser(LteHandoverManagementSapUser* s) = 0;

    /**
     * @brief Export the "provider" part of the Handover Management SAP interface.
     * @return the reference to the "provider" part of the interface, typically to
     *         be kept by an LteEnbRrc instance
     */
    virtual LteHandoverManagementSapProvider* GetLteHandoverManagementSapProvider() = 0;

  protected:
    // inherited from Object
    void DoDispose() override;

    // HANDOVER MANAGEMENT SAP PROVIDER IMPLEMENTATION

    /**
     * @brief Implementation of LteHandoverManagementSapProvider::ReportUeMeas.
     * @param rnti Radio Network Temporary Identity, an integer identifying the UE
     *             where the report originates from
     * @param measResults a single report of one measurement identity
     */
    virtual void DoReportUeMeas(uint16_t rnti, LteRrcSap::MeasResults measResults) = 0;
};

} // end of namespace ns3

#endif /* LTE_HANDOVER_ALGORITHM_H */
