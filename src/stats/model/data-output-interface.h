/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#ifndef DATA_OUTPUT_INTERFACE_H
#define DATA_OUTPUT_INTERFACE_H

#include "data-calculator.h"

#include "ns3/nstime.h"
#include "ns3/object.h"

namespace ns3
{

class DataCollector;

//------------------------------------------------------------
//--------------------------------------------
/**
 * @ingroup dataoutput
 *
 * @brief Abstract Data Output Interface class
s */
class DataOutputInterface : public Object
{
  public:
    DataOutputInterface();
    ~DataOutputInterface() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Outputs information from the provided DataCollector
     * @param dc DataCollector object
     */
    virtual void Output(DataCollector& dc) = 0;

    /**
     * Sets the DataOutputInterface prefix to the provided prefix
     * @param prefix prefix as a string
     */
    void SetFilePrefix(const std::string prefix);
    /**
     * Gets the file prefix of the DataOutputInterface
     * @return File prefix as a string
     */
    std::string GetFilePrefix() const;

  protected:
    void DoDispose() override;

    std::string m_filePrefix; //!< File prefix for the DataOutputInterface

    // end class DataOutputInterface
};

/**
 * @ingroup dataoutput
 *
 * @brief Callback class for the DataOutput classes
 *
 */
class DataOutputCallback
{
  public:
    /**
     * Destructor
     */
    virtual ~DataOutputCallback()
    {
    }

    /**
     * Outputs the data from the specified StatisticalSummary
     * @param key Key value of a DataCalculator
     * @param variable Name of the variable for which statistics are being provided
     * @param statSum Pointer to a StatisticalSummary object
     */
    virtual void OutputStatistic(std::string key,
                                 std::string variable,
                                 const StatisticalSummary* statSum) = 0;

    /**
     * Associates the integer value with the variable name for a specific output format
     * @param key Key value of a DataCalculator
     * @param variable Name of the variable for which statistics are being provided
     * @param val Value to be stored
     */
    virtual void OutputSingleton(std::string key, std::string variable, int val) = 0;

    /**
     * Associates the uint32_t value with the variable name for a specific output format
     * @param key Key value of a DataCalculator
     * @param variable Name of the variable for which statistics are being provided
     * @param val Value to be stored
     */
    virtual void OutputSingleton(std::string key, std::string variable, uint32_t val) = 0;

    /**
     * Associates the double value with the variable name for a specific output format
     * @param key Key value of a DataCalculator
     * @param variable Name of the variable for which statistics are being provided
     * @param val Value to be stored
     */
    virtual void OutputSingleton(std::string key, std::string variable, double val) = 0;

    /**
     * Associates the string value with the variable name for a specific output format
     * @param key Key value of a DataCalculator
     * @param variable Name of the variable for which statistics are being provided
     * @param val Value to be stored
     */
    virtual void OutputSingleton(std::string key, std::string variable, std::string val) = 0;

    /**
     * Associates the Time value with the variable name for a specific output format
     * @param key Key value of a DataCalculator
     * @param variable Name of the variable for which statistics are being provided
     * @param val Value to be stored
     */
    virtual void OutputSingleton(std::string key, std::string variable, Time val) = 0;
    // end class DataOutputCallback
};

// end namespace ns3
}; // namespace ns3

#endif /* DATA_OUTPUT_INTERFACE_H */
