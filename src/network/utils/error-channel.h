/*
 * Copyright (c) 2013 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */
#ifndef ERROR_CHANNEL_H
#define ERROR_CHANNEL_H

#include "error-model.h"
#include "mac48-address.h"
#include "simple-channel.h"

#include "ns3/channel.h"
#include "ns3/nstime.h"

#include <vector>

namespace ns3
{

class SimpleNetDevice;
class Packet;

/**
 * @ingroup channel
 * @brief A Error channel, introducing deterministic delays on even/odd packets. Used for testing
 */
class ErrorChannel : public SimpleChannel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    ErrorChannel();

    // inherited from ns3::SimpleChannel
    void Send(Ptr<Packet> p,
              uint16_t protocol,
              Mac48Address to,
              Mac48Address from,
              Ptr<SimpleNetDevice> sender) override;

    void Add(Ptr<SimpleNetDevice> device) override;

    // inherited from ns3::Channel
    std::size_t GetNDevices() const override;
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

    /**
     * @brief Set the delay for the odd packets (even ones are not delayed)
     * @param delay Delay for the odd packets.
     */
    void SetJumpingTime(Time delay);

    /**
     * @brief Set if the odd packets are delayed (even ones are not delayed ever)
     * @param mode true if the odd packets should be delayed.
     */
    void SetJumpingMode(bool mode);

    /**
     * @brief Set the delay for the odd duplicate packets (even ones are not duplicated)
     * @param delay Delay for the odd packets.
     */
    void SetDuplicateTime(Time delay);

    /**
     * @brief Set if the odd packets are duplicated (even ones are not duplicated ever)
     * @param mode true if the odd packets should be duplicated.
     */
    void SetDuplicateMode(bool mode);

  private:
    std::vector<Ptr<SimpleNetDevice>> m_devices; //!< devices connected by the channel
    Time m_jumpingTime;                          //!< Delay time in Jumping mode.
    uint8_t m_jumpingState;                      //!< Counter for even/odd packets in Jumping mode.
    bool m_jumping;                              //!< Flag for Jumping mode.
    Time m_duplicateTime;                        //!< Duplicate time in Duplicate mode.
    bool m_duplicate;                            //!< Flag for Duplicate mode.
    uint8_t m_duplicateState; //!< Counter for even/odd packets in Duplicate mode.
};

} // namespace ns3

#endif /* ERROR_CHANNEL_H */
