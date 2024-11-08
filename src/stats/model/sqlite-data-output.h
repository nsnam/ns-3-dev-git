/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#ifndef SQLITE_DATA_OUTPUT_H
#define SQLITE_DATA_OUTPUT_H

#include "data-output-interface.h"

#include "ns3/nstime.h"

struct sqlite3_stmt;

namespace ns3
{

class SQLiteOutput;

//------------------------------------------------------------
//--------------------------------------------
/**
 * @ingroup dataoutput
 * @class SqliteDataOutput
 * @brief Outputs data in a format compatible with SQLite
 */
class SqliteDataOutput : public DataOutputInterface
{
  public:
    SqliteDataOutput();
    ~SqliteDataOutput() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    void Output(DataCollector& dc) override;

  private:
    /**
     * @ingroup dataoutput
     *
     * @brief Class to generate OMNeT output
     */
    class SqliteOutputCallback : public DataOutputCallback
    {
      public:
        /**
         * Constructor
         * @param db pointer to the instance this object belongs to
         * @param run experiment descriptor
         */
        SqliteOutputCallback(const Ptr<SQLiteOutput>& db, std::string run);

        /**
         * Destructor
         */
        ~SqliteOutputCallback() override;

        /**
         * @brief Generates data statistics
         * @param key the SQL key to use
         * @param variable the variable name
         * @param statSum the stats to print
         */
        void OutputStatistic(std::string key,
                             std::string variable,
                             const StatisticalSummary* statSum) override;

        /**
         * @brief Generates a single data output
         * @param key the SQL key to use
         * @param variable the variable name
         * @param val the value
         */
        void OutputSingleton(std::string key, std::string variable, int val) override;

        /**
         * @brief Generates a single data output
         * @param key the SQL key to use
         * @param variable the variable name
         * @param val the value
         */
        void OutputSingleton(std::string key, std::string variable, uint32_t val) override;

        /**
         * @brief Generates a single data output
         * @param key the SQL key to use
         * @param variable the variable name
         * @param val the value
         */
        void OutputSingleton(std::string key, std::string variable, double val) override;

        /**
         * @brief Generates a single data output
         * @param key the SQL key to use
         * @param variable the variable name
         * @param val the value
         */
        void OutputSingleton(std::string key, std::string variable, std::string val) override;

        /**
         * @brief Generates a single data output
         * @param key the SQL key to use
         * @param variable the variable name
         * @param val the value
         */
        void OutputSingleton(std::string key, std::string variable, Time val) override;

      private:
        Ptr<SQLiteOutput> m_db; //!< Db
        std::string m_runLabel; //!< Run label

        /// Pointer to a Sqlite3 singleton statement
        sqlite3_stmt* m_insertSingletonStatement;
    };

    Ptr<SQLiteOutput> m_sqliteOut; //!< Database
};

// end namespace ns3
} // namespace ns3

#endif /* SQLITE_DATA_OUTPUT_H */
