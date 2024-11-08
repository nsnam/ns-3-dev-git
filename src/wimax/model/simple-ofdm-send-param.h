/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */

#ifndef SIMPLE_OFDM_SEND_PARAM_H
#define SIMPLE_OFDM_SEND_PARAM_H

#include "bvec.h"
#include "wimax-channel.h"
#include "wimax-phy.h"

#include "ns3/propagation-loss-model.h"

#include <list>

namespace ns3
{

/**
 * @ingroup wimax
 * SimpleOfdmSendParam class
 */
class SimpleOfdmSendParam
{
  public:
    SimpleOfdmSendParam();
    /**
     * Constructor
     *
     * @param fecBlock FEC block
     * @param burstSize burst size
     * @param isFirstBlock is the first block
     * @param Frequency frequency
     * @param modulationType modulation type
     * @param direction the direction
     * @param rxPowerDbm receive power
     */
    SimpleOfdmSendParam(const Bvec& fecBlock,
                        uint32_t burstSize,
                        bool isFirstBlock,
                        uint64_t Frequency,
                        WimaxPhy::ModulationType modulationType,
                        uint8_t direction,
                        double rxPowerDbm);
    /**
     * Constructor
     *
     * @param burstSize burst size
     * @param isFirstBlock is the first block
     * @param Frequency frequency
     * @param modulationType modulation type
     * @param direction the direction
     * @param rxPowerDbm receive power
     * @param burst packet burst object
     */
    SimpleOfdmSendParam(uint32_t burstSize,
                        bool isFirstBlock,
                        uint64_t Frequency,
                        WimaxPhy::ModulationType modulationType,
                        uint8_t direction,
                        double rxPowerDbm,
                        Ptr<PacketBurst> burst);
    ~SimpleOfdmSendParam();
    /**
     * @brief sent the fec block to send
     * @param fecBlock the fec block to send
     */
    void SetFecBlock(const Bvec& fecBlock);
    /**
     * @brief set the burst size
     * @param burstSize the burst size in bytes
     */
    void SetBurstSize(uint32_t burstSize);
    /**
     * @param isFirstBlock Set to true if this fec block is the first one in the burst, set to false
     * otherwise
     */
    void SetIsFirstBlock(bool isFirstBlock);
    /**
     * @param Frequency set the frequency of the channel in which this fec block will be sent
     */
    void SetFrequency(uint64_t Frequency);
    /**
     * @param modulationType the modulation type used to send this fec block
     */
    void SetModulationType(WimaxPhy::ModulationType modulationType);
    /**
     * @param direction the direction on which this fec block will be sent
     */
    void SetDirection(uint8_t direction);
    /**
     * @param rxPowerDbm the received power
     */
    void SetRxPowerDbm(double rxPowerDbm);
    /**
     * @return the fec block
     */
    Bvec GetFecBlock();
    /**
     * @return the burst size
     */
    uint32_t GetBurstSize() const;
    /**
     * @return true if this fec block is the first one in the burst, false otherwise
     */
    bool GetIsFirstBlock() const;
    /**
     * @return the frequency on which the fec block is sent/received
     */
    uint64_t GetFrequency() const;
    /**
     * @return the modulation type used to send this fec block
     */
    WimaxPhy::ModulationType GetModulationType();
    /**
     * @return the direction on which this fec block was sent. UP or DOWN
     */
    uint8_t GetDirection() const;
    /**
     * @return the Received power
     */
    double GetRxPowerDbm() const;
    /**
     * @return the received burst
     */
    Ptr<PacketBurst> GetBurst();

  private:
    Bvec m_fecBlock;                           ///< FEC block
    uint32_t m_burstSize;                      ///< burst size
    bool m_isFirstBlock;                       ///< is first block
    uint64_t m_frequency;                      ///< frequency
    WimaxPhy::ModulationType m_modulationType; ///< modulation type
    uint8_t m_direction;                       ///< direction
    double m_rxPowerDbm;                       ///< receive power dbM
    Ptr<PacketBurst> m_burst;                  ///< burst
};
} // namespace ns3

#endif /* SIMPLE_OFDM_SEND_PARAM_H */
