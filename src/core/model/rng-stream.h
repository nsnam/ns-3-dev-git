//
//  Copyright (C) 2001  Pierre L'Ecuyer (lecuyer@iro.umontreal.ca)
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Modified for ns-3 by: Rajib Bhattacharjea<raj.b@gatech.edu>

#ifndef RNGSTREAM_H
#define RNGSTREAM_H
#include <stdint.h>
#include <string>

/**
 * @file
 * @ingroup rngimpl
 * ns3::RngStream declaration.
 */

namespace ns3
{

/**
 * @ingroup randomvariable
 * @defgroup rngimpl RNG Implementation
 */

/**
 * @ingroup rngimpl
 *
 * @brief Combined Multiple-Recursive Generator MRG32k3a
 *
 * This class is the combined multiple-recursive random number
 * generator called MRG32k3a.  The ns3::RandomVariableBase class
 * holds a static instance of this class.  The details of this
 * class are explained in:
 * http://www.iro.umontreal.ca/~lecuyer/myftp/papers/streams00.pdf
 */
class RngStream
{
  public:
    /**
     * Construct from explicit seed, stream and substream values.
     *
     * @param [in] seed The starting seed.
     * @param [in] stream The stream number.
     * @param [in] substream The sub-stream number.
     */
    RngStream(uint32_t seed, uint64_t stream, uint64_t substream);
    /**
     * Copy constructor.
     *
     * @param [in] r The RngStream to copy.
     */
    RngStream(const RngStream& r);
    /**
     * Generate the next random number for this stream.
     * Uniformly distributed between 0 and 1.
     *
     * @returns The next random.
     */
    double RandU01();

  private:
    /**
     * Advance \pname{state} of the RNG by leaps and bounds.
     *
     * @param [in] nth The stream or substream index.
     * @param [in] by The log2 base of \pname{nth}.
     * @param [in] state The state vector to advance.
     */
    void AdvanceNthBy(uint64_t nth, int by, double state[6]);

    /** The RNG state vector. */
    double m_currentState[6];
};

} // namespace ns3

#endif
