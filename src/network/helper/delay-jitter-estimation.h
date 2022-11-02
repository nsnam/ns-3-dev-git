/*
 * Copyright (c) 2007 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef DELAY_JITTER_ESTIMATION_H
#define DELAY_JITTER_ESTIMATION_H

#include "ns3/nstime.h"
#include "ns3/packet.h"

namespace ns3
{

/**
 * \ingroup stats
 *
 * \brief Quick and dirty delay and jitter estimation,
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
     * \param packet the packet to send over a wire
     */
    static void PrepareTx(Ptr<const Packet> packet);

    /**
     * Invoke this method to update the delay and jitter calculations
     * After a call to this method, \ref GetLastDelay and \ref GetLastJitter
     * will return an updated delay and jitter.
     *
     * \param packet the packet received
     */
    void RecordRx(Ptr<const Packet> packet);

    /**
     * Get the Last Delay object.
     *
     * \return the updated delay.
     */
    Time GetLastDelay() const;

    /**
     * The jitter is calculated using the \RFC{1889} (RTP) jitter
     * definition.
     *
     * \return the updated jitter.
     */
    uint64_t GetLastJitter() const;

  private:
    Time m_jitter{0};  //!< Jitter estimation
    Time m_transit{0}; //!< Relative transit time for the previous packet
};

} // namespace ns3

#endif /* DELAY_JITTER_ESTIMATION_H */
