/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#ifndef DATA_CALCULATOR_H
#define DATA_CALCULATOR_H

#include "ns3/deprecated.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/simulator.h"

namespace ns3
{
NS_DEPRECATED_3_44("Use std::nan(\"\") instead")
extern const double NaN; //!< Stored representation of NaN

/**
 * @brief true if x is NaN
 * @param x
 * @return whether x is NaN
 */
NS_DEPRECATED_3_44("Use std::isnan() instead")

inline bool
isNaN(double x)
{
    return std::isnan(x);
}

class DataOutputCallback;

/**
 * @ingroup stats
 * @class StatisticalSummary
 * @brief Abstract class for calculating statistical data
 *
 */
class StatisticalSummary
{
  public:
    /**
     * Destructor
     */
    virtual ~StatisticalSummary()
    {
    }

    /**
     * Returns the number of observations.
     * @return Number of observations
     */
    virtual long getCount() const = 0;

    /**
     * @return Sum of values
     * @see getWeightedSum()
     */
    virtual double getSum() const = 0;

    /**
     * @return Sum of squared values
     * @see getWeightedSqrSum()
     */
    virtual double getSqrSum() const = 0;

    /**
     * Returns the minimum of the values.
     * @return Minimum of values
     */
    virtual double getMin() const = 0;

    /**
     * Returns the maximum of the values.
     * @return Maximum of values
     */
    virtual double getMax() const = 0;

    /**
     * Returns the mean of the (weighted) observations.
     * @return Mean of (weighted) observations
     */
    virtual double getMean() const = 0;

    /**
     * Returns the standard deviation of the (weighted) observations.
     * @return Standard deviation of (weighted) observations
     */
    virtual double getStddev() const = 0;

    /**
     * Returns the variance of the (weighted) observations.
     * @return Variance of (weighted) observations
     */
    virtual double getVariance() const = 0;
};

//------------------------------------------------------------
//--------------------------------------------
/**
 * @ingroup stats
 * @class DataCalculator
 * @brief Calculates data during a simulation
 *
 */
class DataCalculator : public Object
{
  public:
    DataCalculator();
    ~DataCalculator() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Returns whether the DataCalculator is enabled
     * @return true if DataCalculator is enabled
     */
    bool GetEnabled() const;
    /**
     * Enables DataCalculator when simulation starts
     */
    void Enable();
    /**
     * Disables DataCalculator when simulation stops
     */
    void Disable();
    /**
     * Sets the DataCalculator key to the provided key
     * @param key Key value as a string
     */
    void SetKey(const std::string key);
    /**
     * Gets the DataCalculator key
     * @return Key value as a string
     */
    std::string GetKey() const;

    /**
     * Sets the DataCalculator context to the provided context
     * @param context Context value as a string
     */
    void SetContext(const std::string context);
    /**
     * Gets the DataCalculator context
     * @return Context value as a string
     */
    std::string GetContext() const;

    /**
     * Starts DataCalculator at a given time in the simulation
     * @param startTime
     */
    virtual void Start(const Time& startTime);
    /**
     * Stops DataCalculator at a given time in the simulation
     * @param stopTime
     */
    virtual void Stop(const Time& stopTime);

    /**
     * Outputs data based on the provided callback
     * @param callback
     */
    virtual void Output(DataOutputCallback& callback) const = 0;

  protected:
    bool m_enabled; //!< Descendant classes *must* check & respect m_enabled!

    std::string m_key;     //!< Key value
    std::string m_context; //!< Context value

    void DoDispose() override;

  private:
    EventId m_startEvent; //!< Start event
    EventId m_stopEvent;  //!< Stop event

    // end class DataCalculator
};

// end namespace ns3
}; // namespace ns3

#endif /* DATA_CALCULATOR_H */
