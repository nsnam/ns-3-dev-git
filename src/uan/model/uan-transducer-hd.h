/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_TRANSDUCER_HD_H
#define UAN_TRANSDUCER_HD_H

#include "uan-transducer.h"

#include "ns3/simulator.h"

namespace ns3
{

/**
 * @ingroup uan
 *
 * Half duplex implementation of transducer object
 *
 * This class will only allow attached Phy's to receive packets
 * if not in TX mode.
 */
class UanTransducerHd : public UanTransducer
{
  public:
    /** Constructor */
    UanTransducerHd();
    /** Dummy destructor, see DoDispose */
    ~UanTransducerHd() override;

    /**
     * Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    // inherited methods
    State GetState() const override;
    bool IsRx() const override;
    bool IsTx() const override;
    const ArrivalList& GetArrivalList() const override;
    double ApplyRxGainDb(double rxPowerDb, UanTxMode mode) override;
    void SetRxGainDb(double gainDb) override;
    double GetRxGainDb() override;
    void Receive(Ptr<Packet> packet, double rxPowerDb, UanTxMode txMode, UanPdp pdp) override;
    void Transmit(Ptr<UanPhy> src, Ptr<Packet> packet, double txPowerDb, UanTxMode txMode) override;
    void SetChannel(Ptr<UanChannel> chan) override;
    Ptr<UanChannel> GetChannel() const override;
    void AddPhy(Ptr<UanPhy>) override;
    const UanPhyList& GetPhyList() const override;
    void Clear() override;

  private:
    State m_state;             //!< Transducer state.
    ArrivalList m_arrivalList; //!< List of arriving packets which overlap in time.
    UanPhyList m_phyList;      //!< List of physical layers attached above this transducer.
    Ptr<UanChannel> m_channel; //!< The attached channel.
    EventId m_endTxEvent;      //!< Event scheduled for end of transmission.
    Time m_endTxTime;          //!< Time at which transmission will be completed.
    bool m_cleared;            //!< Flab when we've been cleared.
    double m_rxGainDb;         //!< Receive gain in dB.

    /**
     * Remove an entry from the arrival list.
     *
     * @param arrival The packet arrival to remove.
     */
    void RemoveArrival(UanPacketArrival arrival);
    /** Handle end of transmission event. */
    void EndTx();

  protected:
    void DoDispose() override;
};

} // namespace ns3

#endif /* UAN_TRANSDUCER_HD_H */
