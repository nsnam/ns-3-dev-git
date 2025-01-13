/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef LTE_RLC_SEQUENCE_NUMBER_H
#define LTE_RLC_SEQUENCE_NUMBER_H

#include "ns3/assert.h"

#include <iostream>
#include <limits>
#include <stdint.h>

namespace ns3
{

/// SequenceNumber10 class
class SequenceNumber10
{
  public:
    SequenceNumber10()
        : m_value(0),
          m_modulusBase(0)
    {
    }

    /**
     * Constructor
     *
     * @param value the value
     */
    explicit SequenceNumber10(uint16_t value)
        : m_value(value % 1024),
          m_modulusBase(0)
    {
    }

    /**
     * Constructor
     *
     * @param value the value
     */
    SequenceNumber10(const SequenceNumber10& value)
        : m_value(value.m_value),
          m_modulusBase(value.m_modulusBase)
    {
    }

    /**
     * Assignment operator
     *
     * @param value the value
     * @returns SequenceNumber10
     */
    SequenceNumber10& operator=(uint16_t value)
    {
        m_value = value % 1024;
        return *this;
    }

    /**
     * @brief Extracts the numeric value of the sequence number
     * @returns the sequence number value
     */
    uint16_t GetValue() const
    {
        return m_value;
    }

    /**
     * @brief Set modulus base
     * @param modulusBase the modulus
     */
    void SetModulusBase(SequenceNumber10 modulusBase)
    {
        m_modulusBase = modulusBase.m_value;
    }

    /**
     * @brief Set modulus base
     * @param modulusBase the modulus
     */
    void SetModulusBase(uint16_t modulusBase)
    {
        m_modulusBase = modulusBase;
    }

    /**
     * postfix ++ operator
     * @returns SequenceNumber10
     */
    SequenceNumber10 operator++(int)
    {
        SequenceNumber10 retval(m_value);
        m_value = ((uint32_t)m_value + 1) % 1024;
        retval.SetModulusBase(m_modulusBase);
        return retval;
    }

    /**
     * addition operator
     * @param delta the amount to add
     * @returns SequenceNumber10
     */
    SequenceNumber10 operator+(uint16_t delta) const
    {
        SequenceNumber10 ret((m_value + delta) % 1024);
        ret.SetModulusBase(m_modulusBase);
        return ret;
    }

    /**
     * subtraction operator
     * @param delta the amount to subtract
     * @returns SequenceNumber10
     */
    SequenceNumber10 operator-(uint16_t delta) const
    {
        SequenceNumber10 ret((m_value - delta) % 1024);
        ret.SetModulusBase(m_modulusBase);
        return ret;
    }

    /**
     * subtraction operator
     * @param other the amount to subtract
     * @returns SequenceNumber10
     */
    uint16_t operator-(const SequenceNumber10& other) const
    {
        uint16_t diff = m_value - other.m_value;
        return diff;
    }

    /**
     * greater than operator
     * @param other the object to compare
     * @returns true if greater than
     */
    bool operator>(const SequenceNumber10& other) const
    {
        NS_ASSERT(m_modulusBase == other.m_modulusBase);
        uint16_t v1 = (m_value - m_modulusBase) % 1024;
        uint16_t v2 = (other.m_value - other.m_modulusBase) % 1024;
        return v1 > v2;
    }

    /**
     * equality operator
     * @param other the object to compare
     * @returns true if equal
     */
    bool operator==(const SequenceNumber10& other) const
    {
        return (m_value == other.m_value);
    }

    /**
     * inequality operator
     * @param other the object to compare
     * @returns true if not equal
     */
    bool operator!=(const SequenceNumber10& other) const
    {
        return (m_value != other.m_value);
    }

    /**
     * less than or equal operator
     * @param other the object to compare
     * @returns true if less than or equal
     */
    bool operator<=(const SequenceNumber10& other) const
    {
        return (!this->operator>(other));
    }

    /**
     * greater than or equal operator
     * @param other the object to compare
     * @returns true if greater than or equal
     */
    bool operator>=(const SequenceNumber10& other) const
    {
        return (this->operator>(other) || this->operator==(other));
    }

    /**
     * less than operator
     * @param other the object to compare
     * @returns true if less than
     */
    bool operator<(const SequenceNumber10& other) const
    {
        return !this->operator>(other) && m_value != other.m_value;
    }

    friend std::ostream& operator<<(std::ostream& os, const SequenceNumber10& val);

  private:
    uint16_t m_value;       ///< the value
    uint16_t m_modulusBase; ///< the modulus base
};

} // namespace ns3

#endif // LTE_RLC_SEQUENCE_NUMBER_H
