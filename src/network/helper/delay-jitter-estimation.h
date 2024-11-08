/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef DELAY_JITTER_ESTIMATION_H
#define DELAY_JITTER_ESTIMATION_H

#include "ns3/nstime.h"
#include "ns3/packet.h"

namespace ns3
{

/**
 * @ingroup stats
 *
 * @brief Quick and dirty delay and jitter estimation,
 * implementing the jitter algorithm originally from
 * \RFC{1889} (RTP), and unchanged in \RFC{3550}
 *
 * This implementation uses the integer variant of the algorithm
 * given in RFC 1889 Appendix A.8 ,p. 71, and repeated in
 * RFC 3550 Appendix A.8, p. 94.
 */
class DelayJitterEstimation
{
  public:
    DelayJitterEstimation();

    /**
     * This method should be invoked once on each packet to
     * record within the packet the tx time which is used upon
     * packet reception to calculate the delay and jitter. The
     * tx time is stored in the packet as an ns3::Tag which means
     * that it does not use any network resources and is not
     * taken into account in transmission delay calculations.
     *
     * @param packet the packet to send over a wire
     */
    static void PrepareTx(Ptr<const Packet> packet);

    /**
     * Invoke this method to update the delay and jitter calculations
     * After a call to this method, \ref GetLastDelay and \ref GetLastJitter
     * will return an updated delay and jitter.
     *
     * @param packet the packet received
     */
    void RecordRx(Ptr<const Packet> packet);

    /**
     * Get the Last Delay object.
     *
     * @return the updated delay.
     */
    Time GetLastDelay() const;

    /**
     * The jitter is calculated using the \RFC{1889} (RTP) jitter
     * definition.
     *
     * @return the updated jitter.
     */
    uint64_t GetLastJitter() const;

  private:
    Time m_jitter{0};  //!< Jitter estimation
    Time m_transit{0}; //!< Relative transit time for the previous packet
};

} // namespace ns3

#endif /* DELAY_JITTER_ESTIMATION_H */
