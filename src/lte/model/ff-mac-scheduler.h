/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 *         Marco Miozzo <marco.miozzo@cttc.es>
 */

#ifndef FF_MAC_SCHEDULER_H
#define FF_MAC_SCHEDULER_H

#include "ff-mac-common.h"

#include <ns3/object.h>

namespace ns3
{

class FfMacCschedSapUser;
class FfMacSchedSapUser;
class FfMacCschedSapProvider;
class FfMacSchedSapProvider;
class LteFfrSapProvider;
class LteFfrSapUser;

/**
 * \ingroup lte
 * \defgroup ff-api FF MAC Schedulers
 */

/// DL HARQ process status vector
using DlHarqProcessesStatus_t = std::vector<uint8_t>;

/// DL HARQ process timer vector
using DlHarqProcessesTimer_t = std::vector<uint8_t>;

/// DL HARQ process DCI buffer vector
using DlHarqProcessesDciBuffer_t = std::vector<DlDciListElement_s>;

/// Vector of the LCs and layers per UE
using RlcPduList_t = std::vector<std::vector<RlcPduListElement_s>>;

/// Vector of the 8 HARQ processes per UE
using DlHarqRlcPduListBuffer_t = std::vector<RlcPduList_t>;

/// UL HARQ process DCI buffer vector
using UlHarqProcessesDciBuffer_t = std::vector<UlDciListElement_s>;

/// UL HARQ process status vector
using UlHarqProcessesStatus_t = std::vector<uint8_t>;

/// Value for SINR outside the range defined by FF-API, used to indicate that there is
/// no CQI for this element
constexpr double NO_SINR = -5000;

/// Number of HARQ processes
constexpr uint32_t HARQ_PROC_NUM = 8;

/// HARQ DL timeout
constexpr uint32_t HARQ_DL_TIMEOUT = 11;

/**
 * \ingroup ff-api
 *
 * This abstract base class identifies the interface by means of which
 * the helper object can plug on the MAC a scheduler implementation based on the
 * FF MAC Sched API.
 *
 *
 */
class FfMacScheduler : public Object
{
  public:
    /**
     * The type of UL CQI to be filtered (ALL means accept all the CQI,
     * where a new CQI of any type overwrite the old one, even of another type)
     *
     */
    enum UlCqiFilter_t
    {
        SRS_UL_CQI,
        PUSCH_UL_CQI
    };

    /**
     * constructor
     *
     */
    FfMacScheduler();
    /**
     * destructor
     *
     */
    ~FfMacScheduler() override;

    // inherited from Object
    void DoDispose() override;
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * set the user part of the FfMacCschedSap that this Scheduler will
     * interact with. Normally this part of the SAP is exported by the MAC.
     *
     * \param s
     */
    virtual void SetFfMacCschedSapUser(FfMacCschedSapUser* s) = 0;

    /**
     *
     * set the user part of the FfMacSchedSap that this Scheduler will
     * interact with. Normally this part of the SAP is exported by the MAC.
     *
     * \param s
     */
    virtual void SetFfMacSchedSapUser(FfMacSchedSapUser* s) = 0;

    /**
     *
     * \return the Provider part of the FfMacCschedSap provided by the Scheduler
     */
    virtual FfMacCschedSapProvider* GetFfMacCschedSapProvider() = 0;

    /**
     *
     * \return the Provider part of the FfMacSchedSap provided by the Scheduler
     */
    virtual FfMacSchedSapProvider* GetFfMacSchedSapProvider() = 0;

    // FFR SAPs
    /**
     *
     * Set the Provider part of the LteFfrSap that this Scheduler will
     * interact with
     *
     * \param s
     */
    virtual void SetLteFfrSapProvider(LteFfrSapProvider* s) = 0;

    /**
     *
     * \return the User part of the LteFfrSap provided by the FfrAlgorithm
     */
    virtual LteFfrSapUser* GetLteFfrSapUser() = 0;

  protected:
    UlCqiFilter_t m_ulCqiFilter; ///< UL CQI filter
};

} // namespace ns3

#endif /* FF_MAC_SCHEDULER_H */
