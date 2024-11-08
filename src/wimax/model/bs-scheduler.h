/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

/* BS outbound scheduler as per in Section 6.3.5.1 */

#ifndef BS_SCHEDULER_H
#define BS_SCHEDULER_H

#include "dl-mac-messages.h"
#include "service-flow.h"
#include "wimax-phy.h"

#include "ns3/packet-burst.h"
#include "ns3/packet.h"

#include <list>

namespace ns3
{

class BaseStationNetDevice;
class GenericMacHeader;
class WimaxConnection;
class Cid;

/**
 * @ingroup wimax
 *
 * BaseStation Scheduler
 */
class BSScheduler : public Object
{
  public:
    BSScheduler();
    /**
     * Constructor
     *
     * @param bs base station device
     */
    BSScheduler(Ptr<BaseStationNetDevice> bs);
    ~BSScheduler() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief This function returns all the downlink bursts scheduled for the next
     * downlink sub-frame
     * @returns  all the downlink bursts scheduled for the next downlink sub-frame
     */
    virtual std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst>>>* GetDownlinkBursts() const = 0;
    /**
     * @brief This function adds a downlink burst to the list of downlink bursts
     * scheduled for the next downlink sub-frame
     * @param connection a pointer to connection in which the burst will be sent
     * @param diuc downlink iuc
     * @param modulationType the modulation type of the burst
     * @param burst the downlink burst to add to the downlink sub frame
     */
    virtual void AddDownlinkBurst(Ptr<const WimaxConnection> connection,
                                  uint8_t diuc,
                                  WimaxPhy::ModulationType modulationType,
                                  Ptr<PacketBurst> burst) = 0;

    /**
     * @brief the scheduling function for the downlink subframe.
     */
    virtual void Schedule() = 0;
    /**
     * @brief Selects a connection from the list of connections having packets to be sent .
     * @param connection will point to a connection that have packets to be sent
     * @returns false if no connection has packets to be sent, true otherwise
     */
    virtual bool SelectConnection(Ptr<WimaxConnection>& connection) = 0;

    /**
     * @brief Creates a downlink UGS burst
     * @param serviceFlow the service flow of the burst
     * @param modulationType the modulation type to be used for the burst
     * @param availableSymbols maximum number of OFDM symbols to be used by the burst
     * @returns a Burst (list of packets)
     */
    virtual Ptr<PacketBurst> CreateUgsBurst(ServiceFlow* serviceFlow,
                                            WimaxPhy::ModulationType modulationType,
                                            uint32_t availableSymbols) = 0;

    /**
     * @brief Get the base station.
     * @returns the base station net device
     */
    virtual Ptr<BaseStationNetDevice> GetBs();
    /**
     * @brief Set the base station.
     * @param bs the base station net device
     */
    virtual void SetBs(Ptr<BaseStationNetDevice> bs);

    /**
     * @brief Check if the packet fragmentation is possible for transport connection.
     * @param connection the downlink connection
     * @param availableSymbols maximum number of OFDM symbols to be used by the burst
     * @param modulationType the modulation type to be used for the burst
     * @returns false if packet fragmentation is not possible, true otherwise
     */
    bool CheckForFragmentation(Ptr<WimaxConnection> connection,
                               int availableSymbols,
                               WimaxPhy::ModulationType modulationType);

  private:
    Ptr<BaseStationNetDevice> m_bs;                                         ///< base station
    std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst>>>* m_downlinkBursts; ///< down link bursts
};

} // namespace ns3

#endif /* BS_SCHEDULER_H */
