/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */

#include "simple-ofdm-send-param.h"

#include "simple-ofdm-wimax-channel.h"
#include "simple-ofdm-wimax-phy.h"

namespace ns3
{
simpleOfdmSendParam::simpleOfdmSendParam()
{
    // m_fecBlock = 0;
    m_burstSize = 0;
    m_isFirstBlock = 0;
    m_frequency = 0;
    m_modulationType = WimaxPhy::MODULATION_TYPE_QPSK_12;
    m_direction = 0;
    m_rxPowerDbm = 0;
}

simpleOfdmSendParam::simpleOfdmSendParam(const bvec& fecBlock,
                                         uint32_t burstSize,
                                         bool isFirstBlock,
                                         uint64_t Frequency,
                                         WimaxPhy::ModulationType modulationType,
                                         uint8_t direction,
                                         double rxPowerDbm)
{
    m_fecBlock = fecBlock;
    m_burstSize = burstSize;
    m_isFirstBlock = isFirstBlock;
    m_frequency = Frequency;
    m_modulationType = modulationType;
    m_direction = direction;
    m_rxPowerDbm = rxPowerDbm;
}

simpleOfdmSendParam::simpleOfdmSendParam(uint32_t burstSize,
                                         bool isFirstBlock,
                                         uint64_t Frequency,
                                         WimaxPhy::ModulationType modulationType,
                                         uint8_t direction,
                                         double rxPowerDbm,
                                         Ptr<PacketBurst> burst)
{
    m_burstSize = burstSize;
    m_isFirstBlock = isFirstBlock;
    m_frequency = Frequency;
    m_modulationType = modulationType;
    m_direction = direction;
    m_rxPowerDbm = rxPowerDbm;
    m_burst = burst;
}

simpleOfdmSendParam::~simpleOfdmSendParam()
{
}

void
simpleOfdmSendParam::SetFecBlock(const bvec& fecBlock)
{
    m_fecBlock = fecBlock;
}

void
simpleOfdmSendParam::SetBurstSize(uint32_t burstSize)
{
    m_burstSize = burstSize;
}

void
simpleOfdmSendParam::SetIsFirstBlock(bool isFirstBlock)
{
    m_isFirstBlock = isFirstBlock;
}

void
simpleOfdmSendParam::SetFrequency(uint64_t Frequency)
{
    m_frequency = Frequency;
}

void
simpleOfdmSendParam::SetModulationType(WimaxPhy::ModulationType modulationType)
{
    m_modulationType = modulationType;
}

void
simpleOfdmSendParam::SetDirection(uint8_t direction)
{
    m_direction = direction;
}

void
simpleOfdmSendParam::SetRxPowerDbm(double rxPowerDbm)
{
    m_rxPowerDbm = rxPowerDbm;
}

bvec
simpleOfdmSendParam::GetFecBlock()
{
    return m_fecBlock;
}

uint32_t
simpleOfdmSendParam::GetBurstSize()
{
    return m_burstSize;
}

bool
simpleOfdmSendParam::GetIsFirstBlock()
{
    return m_isFirstBlock;
}

uint64_t
simpleOfdmSendParam::GetFrequency()
{
    return m_frequency;
}

WimaxPhy::ModulationType
simpleOfdmSendParam::GetModulationType()
{
    return m_modulationType;
}

uint8_t
simpleOfdmSendParam::GetDirection()
{
    return m_direction;
}

double
simpleOfdmSendParam::GetRxPowerDbm()
{
    return m_rxPowerDbm;
}

Ptr<PacketBurst>
simpleOfdmSendParam::GetBurst()
{
    return m_burst;
}

} // namespace ns3
