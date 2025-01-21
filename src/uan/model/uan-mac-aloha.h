/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_MAC_ALOHA_H
#define UAN_MAC_ALOHA_H

#include "uan-mac.h"

#include "ns3/mac8-address.h"

namespace ns3
{

class UanPhy;
class UanTxMode;

/**
 * @ingroup uan
 *
 * ALOHA MAC Protocol, the simplest MAC protocol for wireless networks.
 *
 * Packets enqueued are immediately transmitted.  This MAC attaches
 * a UanHeaderCommon to outgoing packets for address information.
 * (The type field is not used)
 */
class UanMacAloha : public UanMac
{
  public:
    /** Default constructor */
    UanMacAloha();
    /** Dummy destructor, see DoDispose. */
    ~UanMacAloha() override;
    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    // Inherited methods
    bool Enqueue(Ptr<Packet> pkt, uint16_t protocolNumber, const Address& dest) override;
    void SetForwardUpCb(Callback<void, Ptr<Packet>, uint16_t, const Mac8Address&> cb) override;
    void AttachPhy(Ptr<UanPhy> phy) override;
    void Clear() override;
    int64_t AssignStreams(int64_t stream) override;

  private:
    /** PHY layer attached to this MAC. */
    Ptr<UanPhy> m_phy;
    /** Forwarding up callback. */
    Callback<void, Ptr<Packet>, uint16_t, const Mac8Address&> m_forUpCb;
    /** Flag when we've been cleared. */
    bool m_cleared;

    /**
     * Receive packet from lower layer (passed to PHY as callback).
     *
     * @param pkt Packet being received.
     * @param sinr SINR of received packet.
     * @param txMode Mode of received packet.
     */
    void RxPacketGood(Ptr<Packet> pkt, double sinr, UanTxMode txMode);

    /**
     * Packet received at lower layer in error.
     *
     * @param pkt Packet received in error.
     * @param sinr SINR of received packet.
     */
    void RxPacketError(Ptr<Packet> pkt, double sinr);

  protected:
    void DoDispose() override;
};

} // namespace ns3

#endif /* UAN_MAC_ALOHA_H */
