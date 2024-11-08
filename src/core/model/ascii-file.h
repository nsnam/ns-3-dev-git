/*
 * Copyright (c) 2012 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mitch Watrous (watrous@u.washington.edu)
 *
 * This file is based on pcap-file.h by Craig Dowell (craigdo@ee.washington.edu)
 */

#ifndef ASCII_FILE_H
#define ASCII_FILE_H

#include <fstream>
#include <stdint.h>
#include <string>

namespace ns3
{

/**
 * @brief A class representing an ascii file.
 *
 * This class represents an ascii file
 */
class AsciiFile
{
  public:
    AsciiFile();
    ~AsciiFile();

    /**
     * @return true if the 'fail' bit is set in the underlying iostream, false otherwise.
     */
    bool Fail() const;
    /**
     * @return true if the 'eof' bit is set in the underlying iostream, false otherwise.
     */
    bool Eof() const;

    /**
     * Create a new ascii file or open an existing ascii file.
     *
     * @param filename String containing the name of the file.
     * @param mode the access mode for the file.
     */
    void Open(const std::string& filename, std::ios::openmode mode);

    /**
     * Close the underlying file.
     */
    void Close();

    /**
     * @brief Read next line from file
     *
     * @param line    [out] line from file
     *
     */
    void Read(std::string& line);

    /**
     * @brief Compare two ASCII files line-by-line
     *
     * @return true if files are different, false otherwise
     *
     * @param  f1         First ASCII file name
     * @param  f2         Second ASCII file name
     * @param  lineNumber   [out] Line number of first different line.
     */
    static bool Diff(const std::string& f1, const std::string& f2, uint64_t& lineNumber);

  private:
    std::string m_filename; //!< output file name
    std::fstream m_file;    //!< output file
};

} // namespace ns3

#endif /* ASCII_FILE_H */
