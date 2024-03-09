/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef TGAX_VIRTUAL_DESKTOP_H
#define TGAX_VIRTUAL_DESKTOP_H

#include "source-application.h"

#include "ns3/address.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"

#include <utility>
#include <vector>

namespace ns3
{

class ExponentialRandomVariable;
class UniformRandomVariable;
class BernoulliRandomVariable;
class NormalRandomVariable;

/**
 * @ingroup applications
 * @brief Generate Virtual Desktop Infrastructure (VDI) traffic.
 *
 * This VDI traffic generator follows requirements from IEEE 802.11-14/0571r12 - 11ax Evaluation
 * Methodology (Appendix 2 – Traffic model descriptions: Virtual Desktop Infrastructure Traffic
 * Model).
 *
 * In this model, desktop application packet arrival interval obeys an exponential distribution and
 * packet size obeys a normal distribution.
 */
class TgaxVirtualDesktop : public SourceApplication
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TgaxVirtualDesktop();
    ~TgaxVirtualDesktop() override;

    /// Model presets enumeration
    enum class ModelPresets : uint8_t
    {
        CUSTOM = 0, //< Custom traffic model (by default, load parameters of DL model presets)
        DOWNLINK,   //< DL model presets
        UPLINK      //< UL model presets
    };

    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoInitialize() override;

  private:
    void DoStartApplication() override;
    void DoConnectionSucceeded(Ptr<Socket> socket) override;
    void CancelEvents() override;

    /**
     * @brief Set the parameters of the normal random variable used to generate the VDI packet
     * sizes.
     *
     * @param params a vector of pairs (mean, standard deviation) for each mode of the normal
     * distribution used to generate the VDI packet sizes
     */
    void SetParametersPacketSize(const std::vector<std::pair<double, double>>& params);

    /**
     * @brief Get the duration to use to schedule the TX of the next VDI packet.
     *
     * @return Get the duration to use to schedule the TX of the next VDI packet
     */
    Time GetInterArrival() const;

    /**
     * @brief Get the size in bytes of the next VDI packet to send.
     *
     * @return Get the size in bytes of the next VDI packet to send
     */
    uint32_t GetPacketSize() const;

    /**
     * Schedule the next TX
     */
    void ScheduleNext();

    /**
     * Transmit the next VDI packet
     */
    void SendPacket();

    /**
     * Handle a Data Sent event
     *
     * @param socket the connected socket
     * @param size the amount of transmitted bytes
     */
    void TxDone(Ptr<Socket> socket, uint32_t size);

    /**
     * Handle a Send event
     *
     * @param socket the connected socket
     * @param available the amount of available bytes for transmission
     */
    void TxAvailable(Ptr<Socket> socket, uint32_t available);

    bool m_initialPacket{true}; //!< True if the next packet to send is the initial packet

    ModelPresets
        m_modelPresets; //!< Model presets to use to configure the VDI traffic model parameters
    Ptr<UniformRandomVariable> m_initialArrivalUniform; //!< Uniform random variable to generate
                                                        //!< initial packet arrival in nanoseconds
    Ptr<ExponentialRandomVariable>
        m_interArrivalExponential; //!< Exponential random variable to generate packet arrival times
                                   //!< in nanoseconds
    Ptr<BernoulliRandomVariable> m_dlModeSelection; //!< Uniform random variable to select mode for
                                                    //!< downlink bimodal distribution
    std::vector<Ptr<NormalRandomVariable>>
        m_pktSizeDistributions; //!< Single or multi modal normal random variables to generate
                                //!< packet sizes in bytes

    EventId m_txEvent;          //!< Event id of pending TX event
    Ptr<Packet> m_unsentPacket; //!< Unsent packet cached for future attempt
};

} // namespace ns3

#endif /* TGAX_VIRTUAL_DESKTOP_H */
