/*
 * Copyright (c) 2008 Drexel University
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
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#ifndef BASIC_DATA_CALCULATORS_H
#define BASIC_DATA_CALCULATORS_H

#include "data-calculator.h"
#include "data-output-interface.h"

#include "ns3/type-name.h"

namespace ns3
{

/**
 * \ingroup stats
 * \class MinMaxAvgTotalCalculator
 * \brief Template class MinMaxAvgTotalCalculator
 *
 */
//------------------------------------------------------------
//--------------------------------------------
template <typename T = uint32_t>
class MinMaxAvgTotalCalculator : public DataCalculator, public StatisticalSummary
{
  public:
    MinMaxAvgTotalCalculator();
    ~MinMaxAvgTotalCalculator() override;

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Updates all variables of MinMaxAvgTotalCalculator
     * \param i value of type T to use for updating the calculator
     */
    void Update(const T i);
    /**
     * Reinitializes all variables of MinMaxAvgTotalCalculator
     */
    void Reset();

    /**
     * Outputs the data based on the provided callback
     * \param callback
     */
    void Output(DataOutputCallback& callback) const override;

    /**
     * Returns the count
     * \return Count
     */
    long getCount() const override
    {
        return m_count;
    }

    /**
     * Returns the sum
     * \return Total
     */
    double getSum() const override
    {
        return m_total;
    }

    /**
     * Returns the minimum value
     * \return Min
     */
    double getMin() const override
    {
        return m_min;
    }

    /**
     * Returns the maximum value
     * \return Max
     */
    double getMax() const override
    {
        return m_max;
    }

    /**
     * Returns the mean value
     * \return Mean
     */
    double getMean() const override
    {
        return m_meanCurr;
    }

    /**
     * Returns the standard deviation
     * \return Standard deviation
     */
    double getStddev() const override
    {
        return std::sqrt(m_varianceCurr);
    }

    /**
     * Returns the current variance
     * \return Variance
     */
    double getVariance() const override
    {
        return m_varianceCurr;
    }

    /**
     * Returns the sum of squares
     * \return Sum of squares
     */
    double getSqrSum() const override
    {
        return m_squareTotal;
    }

  protected:
    /**
     * Dispose of this Object.
     */
    void DoDispose() override;

    uint32_t m_count; //!< Count value of MinMaxAvgTotalCalculator

    T m_total;       //!< Total value of MinMaxAvgTotalCalculator
    T m_squareTotal; //!< Sum of squares value of MinMaxAvgTotalCalculator
    T m_min;         //!< Minimum value of MinMaxAvgTotalCalculator
    T m_max;         //!< Maximum value of MinMaxAvgTotalCalculator

    double m_meanCurr;     //!< Current mean of MinMaxAvgTotalCalculator
    double m_sCurr;        //!< Current s of MinMaxAvgTotalCalculator
    double m_varianceCurr; //!< Current variance of MinMaxAvgTotalCalculator

    double m_meanPrev; //!< Previous mean of MinMaxAvgTotalCalculator
    double m_sPrev;    //!< Previous s of MinMaxAvgTotalCalculator

    // end MinMaxAvgTotalCalculator
};

//----------------------------------------------
template <typename T>
MinMaxAvgTotalCalculator<T>::MinMaxAvgTotalCalculator()
{
    m_count = 0;

    m_total = 0;
    m_squareTotal = 0;

    m_meanCurr = NaN;
    m_sCurr = NaN;
    m_varianceCurr = NaN;

    m_meanPrev = NaN;
    m_sPrev = NaN;
}

template <typename T>
MinMaxAvgTotalCalculator<T>::~MinMaxAvgTotalCalculator()
{
}

template <typename T>
void
MinMaxAvgTotalCalculator<T>::DoDispose()
{
    DataCalculator::DoDispose();
    // MinMaxAvgTotalCalculator::DoDispose
}

/* static */
template <typename T>
TypeId
MinMaxAvgTotalCalculator<T>::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MinMaxAvgTotalCalculator<" + TypeNameGet<T>() + ">")
                            .SetParent<Object>()
                            .SetGroupName("Stats")
                            .AddConstructor<MinMaxAvgTotalCalculator<T>>();
    return tid;
}

template <typename T>
void
MinMaxAvgTotalCalculator<T>::Update(const T i)
{
    if (m_enabled)
    {
        m_count++;

        m_total += i;
        m_squareTotal += i * i;

        if (m_count == 1)
        {
            m_min = i;
            m_max = i;
        }
        else
        {
            m_min = (i < m_min) ? i : m_min;
            m_max = (i > m_max) ? i : m_max;
        }

        // Calculate the variance based on equations (15) and (16) on
        // page 216 of "The Art of Computer Programming, Volume 2",
        // Second Edition. Donald E. Knuth.  Addison-Wesley
        // Publishing Company, 1973.
        //
        // The relationships between the variance, standard deviation,
        // and s are as follows
        //
        //                      s
        //     variance  =  -----------
        //                   count - 1
        //
        //                                -------------
        //                               /
        //     standard_deviation  =    /  variance
        //                            \/
        //
        if (m_count == 1)
        {
            // Set the very first values.
            m_meanCurr = i;
            m_sCurr = 0;
            m_varianceCurr = m_sCurr;
        }
        else
        {
            // Save the previous values.
            m_meanPrev = m_meanCurr;
            m_sPrev = m_sCurr;

            // Update the current values.
            m_meanCurr = m_meanPrev + (i - m_meanPrev) / m_count;
            m_sCurr = m_sPrev + (i - m_meanPrev) * (i - m_meanCurr);
            m_varianceCurr = m_sCurr / (m_count - 1);
        }
    }
    // end MinMaxAvgTotalCalculator::Update
}

template <typename T>
void
MinMaxAvgTotalCalculator<T>::Reset()
{
    m_count = 0;

    m_total = 0;
    m_squareTotal = 0;

    m_meanCurr = NaN;
    m_sCurr = NaN;
    m_varianceCurr = NaN;

    m_meanPrev = NaN;
    m_sPrev = NaN;
    // end MinMaxAvgTotalCalculator::Reset
}

template <typename T>
void
MinMaxAvgTotalCalculator<T>::Output(DataOutputCallback& callback) const
{
    callback.OutputStatistic(m_context, m_key, this);
}

/**
 * \ingroup stats
 * \class CounterCalculator
 * \brief Template class CounterCalculator
 *
 */
//------------------------------------------------------------
//--------------------------------------------
template <typename T = uint32_t>
class CounterCalculator : public DataCalculator
{
  public:
    CounterCalculator();
    ~CounterCalculator() override;

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Increments count by 1
     */
    void Update();
    /**
     * Increments count by i
     * \param i value of type T to increment count
     */
    void Update(const T i);

    /**
     * Returns the count of the CounterCalculator
     * \return Count as a value of type T
     */
    T GetCount() const;

    /**
     * Outputs the data based on the provided callback
     * \param callback
     */
    void Output(DataOutputCallback& callback) const override;

  protected:
    /**
     * Dispose of this Object.
     */
    void DoDispose() override;

    T m_count; //!< Count value of CounterCalculator

    // end CounterCalculator
};

//--------------------------------------------
template <typename T>
CounterCalculator<T>::CounterCalculator()
    : m_count(0)
{
}

template <typename T>
CounterCalculator<T>::~CounterCalculator()
{
}

/* static */
template <typename T>
TypeId
CounterCalculator<T>::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CounterCalculator<" + TypeNameGet<T>() + ">")
                            .SetParent<Object>()
                            .SetGroupName("Stats")
                            .AddConstructor<CounterCalculator<T>>();
    return tid;
}

template <typename T>
void
CounterCalculator<T>::DoDispose()
{
    DataCalculator::DoDispose();
    // CounterCalculator::DoDispose
}

template <typename T>
void
CounterCalculator<T>::Update()
{
    if (m_enabled)
    {
        m_count++;
    }
    // end CounterCalculator::Update
}

template <typename T>
void
CounterCalculator<T>::Update(const T i)
{
    if (m_enabled)
    {
        m_count += i;
    }
    // end CounterCalculator::Update
}

template <typename T>
T
CounterCalculator<T>::GetCount() const
{
    return m_count;
    // end CounterCalculator::GetCount
}

template <typename T>
void
CounterCalculator<T>::Output(DataOutputCallback& callback) const
{
    callback.OutputSingleton(m_context, m_key, m_count);
    // end CounterCalculator::Output
}

// The following explicit template instantiation declaration prevents modules
// including this header file from implicitly instantiating CounterCalculator<uint32_t>.
// This would cause some examples on Windows to crash at runtime with the
// following error message: "Trying to allocate twice the same UID:
// ns3::CounterCalculator<uint32_t>"
extern template class CounterCalculator<uint32_t>;

// end namespace ns3
}; // namespace ns3

#endif /* BASIC_DATA_CALCULATORS_H */
