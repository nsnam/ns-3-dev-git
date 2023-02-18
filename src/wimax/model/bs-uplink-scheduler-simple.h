/*
 * Copyright (c) 2007,2008 INRIA
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
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#ifndef UPLINK_SCHEDULER_SIMPLE_H
#define UPLINK_SCHEDULER_SIMPLE_H

#include "bs-uplink-scheduler.h"
#include "service-flow.h"
#include "ul-mac-messages.h"
#include "wimax-phy.h"

#include "ns3/nstime.h"

#include <stdint.h>

namespace ns3
{

class BaseStationNetDevice;
class SSRecord;
class ServiceFlow;

/**
 * \ingroup wimax
 *
 * Uplink Scheduler - simplified
 */
class UplinkSchedulerSimple : public UplinkScheduler
{
  public:
    UplinkSchedulerSimple();
    /**
     * Constructor
     *
     * \param bs base station device
     */
    UplinkSchedulerSimple(Ptr<BaseStationNetDevice> bs);
    ~UplinkSchedulerSimple() override;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    std::list<OfdmUlMapIe> GetUplinkAllocations() const override;

    /**
     * \brief Determines if channel descriptors sent in the current frame are
     * required to be updated
     *
     * \param updateDcd currently unused (assume true)
     * \param updateUcd currently unused (assume true)
     * \param [out] sendDcd DCD must be updated
     * \param [out] sendUcd UCD must be updated
     *
     */
    void GetChannelDescriptorsToUpdate(bool& updateDcd,
                                       bool& updateUcd,
                                       bool& sendDcd,
                                       bool& sendUcd) override;
    uint32_t CalculateAllocationStartTime() override;
    void AddUplinkAllocation(OfdmUlMapIe& ulMapIe,
                             const uint32_t& allocationSize,
                             uint32_t& symbolsToAllocation,
                             uint32_t& availableSymbols) override;
    void Schedule() override;
    /**
     * Service Unsolicited Grants function
     * \param ssRecord Subscriber station record
     * \param schedulingType the scheduling type
     * \param ulMapIe the UL map IE
     * \param modulationType the modulation type
     * \param symbolsToAllocation the symbols to allocation
     * \param availableSymbols the available symbols
     */
    void ServiceUnsolicitedGrants(const SSRecord* ssRecord,
                                  enum ServiceFlow::SchedulingType schedulingType,
                                  OfdmUlMapIe& ulMapIe,
                                  const WimaxPhy::ModulationType modulationType,
                                  uint32_t& symbolsToAllocation,
                                  uint32_t& availableSymbols) override;
    /**
     * Service Bandwidth Requests function
     * \param ssRecord Subscriber station record
     * \param schedulingType the scheduling type
     * \param ulMapIe the UL map IE
     * \param modulationType the modulation type
     * \param symbolsToAllocation the symbols to allocation
     * \param availableSymbols the available symbols
     */
    void ServiceBandwidthRequests(const SSRecord* ssRecord,
                                  enum ServiceFlow::SchedulingType schedulingType,
                                  OfdmUlMapIe& ulMapIe,
                                  const WimaxPhy::ModulationType modulationType,
                                  uint32_t& symbolsToAllocation,
                                  uint32_t& availableSymbols) override;
    /**
     * Service bandwidth requests function
     * \param serviceFlow the service flow
     * \param schedulingType the scheduling type
     * \param ulMapIe the UL map IE
     * \param modulationType the modulation type
     * \param symbolsToAllocation the symbols to allocation
     * \param availableSymbols the available symbols
     * \returns true if successful
     */
    bool ServiceBandwidthRequests(ServiceFlow* serviceFlow,
                                  enum ServiceFlow::SchedulingType schedulingType,
                                  OfdmUlMapIe& ulMapIe,
                                  const WimaxPhy::ModulationType modulationType,
                                  uint32_t& symbolsToAllocation,
                                  uint32_t& availableSymbols) override;
    /**
     * Allocate Initial Ranging Interval function
     *
     * \param symbolsToAllocation symbols to allocation for UpLink Subframe
     * \param availableSymbols available symbols for rtPS flows
     */
    void AllocateInitialRangingInterval(uint32_t& symbolsToAllocation,
                                        uint32_t& availableSymbols) override;
    /**
     * Setup service flow function
     * \param ssRecord Subscriber station record
     * \param serviceFlow the service flow
     */
    void SetupServiceFlow(SSRecord* ssRecord, ServiceFlow* serviceFlow) override;

    /**
     * Process bandwidth request function
     * \param bwRequestHdr the bandwidth request header
     */
    void ProcessBandwidthRequest(const BandwidthRequestHeader& bwRequestHdr) override;

    /// Init once function
    void InitOnce() override;

    /**
     * Set requested bandwidth function
     * \param sfr the service flow record
     */
    void OnSetRequestedBandwidth(ServiceFlowRecord* sfr) override;

  private:
    std::list<OfdmUlMapIe> m_uplinkAllocations; ///< uplink allocations
};

} // namespace ns3

#endif /* UPLINK_SCHEDULER_SIMPLE_H */
