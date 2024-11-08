/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NS3_CHANNEL_H
#define NS3_CHANNEL_H

#include "ns3/object.h"
#include "ns3/ptr.h"

#include <stdint.h>
#include <string>

namespace ns3
{

class NetDevice;

/**
 * @ingroup network
 * @defgroup channel Channel
 */
/**
 * @ingroup channel
 * @brief Abstract Channel Base Class.
 *
 * A channel is a logical path over which information flows.  The path can
 * be as simple as a short piece of wire, or as complicated as space-time.
 *
 * Subclasses must use Simulator::ScheduleWithContext to correctly update
 * event contexts when scheduling an event from one node to another one.
 */
class Channel : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    Channel();
    ~Channel() override;

    /**
     * @returns the unique id of this channel
     *
     * This unique id happens to be also the index of the Channel into
     * the ChannelList.
     */
    uint32_t GetId() const;

    /**
     * @returns the number of NetDevices connected to this Channel.
     *
     * This method must be implemented by subclasses.
     */
    virtual std::size_t GetNDevices() const = 0;
    /**
     * @param i index of NetDevice to retrieve
     * @returns one of the NetDevices connected to this channel.
     *
     * This method must be implemented by subclasses.
     */
    virtual Ptr<NetDevice> GetDevice(std::size_t i) const = 0;

  private:
    uint32_t m_id; //!< Channel id for this channel
};

} // namespace ns3

#endif /* NS3_CHANNEL_H */
