/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#ifndef OMNET_DATA_OUTPUT_H
#define OMNET_DATA_OUTPUT_H

#include "data-output-interface.h"

#include "ns3/nstime.h"

namespace ns3
{

//------------------------------------------------------------
//--------------------------------------------
/**
 * @ingroup dataoutput
 * @class OmnetDataOutput
 * @brief Outputs data in a format compatible with OMNeT library and framework
 *
 */
class OmnetDataOutput : public DataOutputInterface
{
  public:
    OmnetDataOutput();
    ~OmnetDataOutput() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    void Output(DataCollector& dc) override;

  protected:
    void DoDispose() override;

  private:
    /**
     * @ingroup dataoutput
     *
     * @brief Class to generate OMNeT output
     */
    class OmnetOutputCallback : public DataOutputCallback
    {
      public:
        /**
         * Constructor
         * @param scalar the output stream
         */
        OmnetOutputCallback(std::ostream* scalar);

        /**
         * @brief Generates data statistics
         * @param context the output context
         * @param name the output name
         * @param statSum the stats to print
         */
        void OutputStatistic(std::string context,
                             std::string name,
                             const StatisticalSummary* statSum) override;

        /**
         * @brief Generates a single data output
         * @param context the output context
         * @param name the output name
         * @param val the value
         */
        void OutputSingleton(std::string context, std::string name, int val) override;

        /**
         * @brief Generates a single data output
         * @param context the output context
         * @param name the output name
         * @param val the value
         */
        void OutputSingleton(std::string context, std::string name, uint32_t val) override;

        /**
         * @brief Generates a single data output
         * @param context the output context
         * @param name the output name
         * @param val the value
         */
        void OutputSingleton(std::string context, std::string name, double val) override;

        /**
         * @brief Generates a single data output
         * @param context the output context
         * @param name the output name
         * @param val the value
         */
        void OutputSingleton(std::string context, std::string name, std::string val) override;

        /**
         * @brief Generates a single data output
         * @param context the output context
         * @param name the output name
         * @param val the value
         */
        void OutputSingleton(std::string context, std::string name, Time val) override;

      private:
        std::ostream* m_scalar; //!< output stream
                                // end class OmnetOutputCallback
    };

    // end class OmnetDataOutput
};

// end namespace ns3
}; // namespace ns3

#endif /* OMNET_DATA_OUTPUT_H */
