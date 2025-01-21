/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_MAC_H
#define UAN_MAC_H

#include "ns3/address.h"
#include "ns3/mac8-address.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"

namespace ns3
{

class UanPhy;
class UanChannel;
class UanNetDevice;
class UanTransducer;
class UanTxMode;
class Mac8Address;

/**
 * @ingroup uan
 *
 * Virtual base class for all UAN MAC protocols.
 */
class UanMac : public Object
{
  public:
    /** Default constructor */
    UanMac();
    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Get the MAC Address.
     *
     * @return MAC Address.
     */
    virtual Address GetAddress();

    /**
     * Set the address.
     *
     * @param addr Mac8Address for this MAC.
     */
    virtual void SetAddress(Mac8Address addr);

    /**
     * Enqueue packet to be transmitted.
     *
     * @param pkt Packet to be transmitted.
     * @param dest Destination address.
     * @param protocolNumber The type of the packet.
     * @return True if packet was successfully enqueued.
     */
    virtual bool Enqueue(Ptr<Packet> pkt, uint16_t protocolNumber, const Address& dest) = 0;
    /**
     * Set the callback to forward packets up to higher layers.
     *
     * @param cb The callback.
     * \pname{packet} The packet.
     * \pname{address} The source address.
     */
    virtual void SetForwardUpCb(Callback<void, Ptr<Packet>, uint16_t, const Mac8Address&> cb) = 0;

    /**
     * Attach PHY layer to this MAC.
     *
     * Some MACs may be designed to work with multiple PHY
     * layers.  Others may only work with one.
     *
     * @param phy Phy layer to attach to this MAC.
     */
    virtual void AttachPhy(Ptr<UanPhy> phy) = 0;

    /**
     * Get the broadcast address.
     *
     * @return The broadcast address.
     */
    virtual Address GetBroadcast() const;

    /** Clears all pointer references. */
    virtual void Clear() = 0;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream First stream index to use.
     * @return The number of stream indices assigned by this model.
     */
    virtual int64_t AssignStreams(int64_t stream) = 0;

    /**
     *  TracedCallback signature for packet reception/enqueue/dequeue events.
     *
     * @param [in] packet The Packet.
     * @param [in] mode The UanTxMode.
     */
    typedef void (*PacketModeTracedCallback)(Ptr<const Packet> packet, UanTxMode mode);

    /**
     * Get the Tx mode index (Modulation type).
     * @return the Tx mode index
     */
    uint32_t GetTxModeIndex() const;

    /**
     * Set the Tx mode index (Modulation type).
     * @param txModeIndex the Tx mode index
     */
    void SetTxModeIndex(uint32_t txModeIndex);

  private:
    /** Modulation type */
    uint32_t m_txModeIndex;
    /** The MAC address. */
    Mac8Address m_address;
};

} // namespace ns3

#endif /* UAN_MAC_H */
