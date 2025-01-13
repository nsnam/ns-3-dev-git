/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef SPECTRUM_ERROR_MODEL_H
#define SPECTRUM_ERROR_MODEL_H

#include "spectrum-value.h"

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"

namespace ns3
{

/**
 * @ingroup spectrum
 *
 */
class SpectrumErrorModel : public Object
{
  public:
    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();
    ~SpectrumErrorModel() override;

    /**
     * Start a packet reception
     * @param p the packet
     */
    virtual void StartRx(Ptr<const Packet> p) = 0;

    /**
     * Evaluates a chunk
     * @param sinr the SpectrumValue experienced by the Chunk
     * @param duration the Chunk length
     */
    virtual void EvaluateChunk(const SpectrumValue& sinr, Time duration) = 0;

    /**
     * Checks if the packet being received is correct
     * @returns true if the packet is correct.
     */
    virtual bool IsRxCorrect() = 0;
};

/**
 * @ingroup spectrum
 *
 * This class implements the error model described in this paper:
 * N. Baldo and M. Miozzo, "Spectrum-aware Channel and PHY layer modeling
 * for ns3", in Proceedings of International Workshop on Network
 * Simulation Tools (NSTOOLS 2009), 19 October 2009, Pisa (Italy).
 */
class ShannonSpectrumErrorModel : public SpectrumErrorModel
{
  protected:
    void DoDispose() override;

  public:
    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();
    // inherited from SpectrumErrorModel
    void StartRx(Ptr<const Packet> p) override;
    void EvaluateChunk(const SpectrumValue& sinr, Time duration) override;
    bool IsRxCorrect() override;

  private:
    uint32_t m_bytes;            //!< Length of the packet being received
    uint32_t m_deliverableBytes; //!< Bytes that can be received according to the Shnanon's formula
};

} // namespace ns3

#endif /* SPECTRUM_ERROR_MODEL_H */
