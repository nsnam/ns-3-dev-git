/*
 * Copyright (c) 2009 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@cutebugs.net>
 */

#ifndef FILE_CONFIG_H
#define FILE_CONFIG_H

#include <string>

namespace ns3
{

/**
 * @ingroup configstore
 * @brief base class for ConfigStore classes using files
 *
 */
class FileConfig
{
  public:
    virtual ~FileConfig();
    /**
     * Set the file name
     * @param filename the filename
     */
    virtual void SetFilename(std::string filename) = 0;
    /**
     * Load or save the default values
     */
    virtual void Default() = 0;
    /**
     * Load or save the global values
     */
    virtual void Global() = 0;
    /**
     * Load or save the attributes values
     */
    virtual void Attributes() = 0;
};

/**
 * @ingroup configstore
 * @brief A dummy class (does nothing)
 */
class NoneFileConfig : public FileConfig
{
  public:
    NoneFileConfig();
    ~NoneFileConfig() override;
    void SetFilename(std::string filename) override;
    void Default() override;
    void Global() override;
    void Attributes() override;
};

} // namespace ns3

#endif /* FILE_CONFIG_H */
