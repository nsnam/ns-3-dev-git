/*
 * Copyright (c) 2009 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@cutebugs.net>
 */

#ifndef RAW_TEXT_CONFIG_H
#define RAW_TEXT_CONFIG_H

#include "file-config.h"

#include <fstream>
#include <string>

namespace ns3
{

/**
 * @ingroup configstore
 * @brief A class to enable saving of configuration store in a raw text file
 *
 */
class RawTextConfigSave : public FileConfig
{
  public:
    RawTextConfigSave();           //!< default constructor
    ~RawTextConfigSave() override; //!< destructor
    // Inherited
    void SetFilename(std::string filename) override;
    void Default() override;
    void Global() override;
    void Attributes() override;

  private:
    /// Config store output stream
    std::ofstream* m_os;
};

/**
 * @ingroup configstore
 * @brief A class to enable loading of configuration store from a raw text file
 *
 */
class RawTextConfigLoad : public FileConfig
{
  public:
    RawTextConfigLoad();           //!< default constructor
    ~RawTextConfigLoad() override; //!< destructor
    // Inherited
    void SetFilename(std::string filename) override;
    void Default() override;
    void Global() override;
    void Attributes() override;

  private:
    /**
     * Parse (potentially multi-) line configs into type, name, and values.
     * This will return \c false for blank lines, comments (lines beginning with '#'),
     * and incomplete entries. An entry is considered complete when a type and name
     * have been found and the value contains two delimiting quotation marks '"'.
     * @param line the config file line
     * @param type the config type {default, global, value}
     * @param name the config attribute name
     * @param value the value to set
     * @returns true if all of type, name, and value parsed; false otherwise
     *
     */
    virtual bool ParseLine(const std::string& line,
                           std::string& type,
                           std::string& name,
                           std::string& value);

    /**
     * Strip out attribute value
     * @param value the input string
     * @returns the updated string
     */
    std::string Strip(std::string value);
    /// Config store input stream
    std::ifstream* m_is;
};

} // namespace ns3

#endif /* RAW_TEXT_CONFIG_H */
