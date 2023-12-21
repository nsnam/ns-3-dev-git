/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#ifndef UNIFORM_RANDOM_BIT_GENERATOR_H
#define UNIFORM_RANDOM_BIT_GENERATOR_H

#include "random-variable-stream.h"

#include <limits>

namespace ns3
{

/**
 * Wraps a UniformRandomVariable into a class that meets the requirements of a
 * UniformRandomBitGenerator as specified by the C++11 standard, thus allowing
 * the usage of ns-3 style UniformRandomVariables as generators in the functions
 * of the standard random library.
 */
class UniformRandomBitGenerator
{
  public:
    UniformRandomBitGenerator()
        : m_rv(CreateObject<UniformRandomVariable>())
    {
    }

    /**
     * \return the ns-3 style uniform random variable
     */
    Ptr<UniformRandomVariable> GetRv() const
    {
        return m_rv;
    }

    /// Typedef needed to meet requirements. Result type must be an unsigned integer type.
    using result_type = uint32_t;

    /**
     * \return the smallest value that operator() may return
     */
    static constexpr result_type min()
    {
        return 0;
    }

    /**
     * \return the largest value that operator() may return
     */
    static constexpr result_type max()
    {
        return std::numeric_limits<result_type>::max();
    }

    /**
     * \return a value in the closed interval [min(), max()]
     */
    result_type operator()()
    {
        return m_rv->GetInteger(min(), max());
    }

  private:
    Ptr<UniformRandomVariable> m_rv; //!< ns-3 style uniform random variable
};

} // namespace ns3

#endif /* UNIFORM_RANDOM_BIT_GENERATOR_H */
