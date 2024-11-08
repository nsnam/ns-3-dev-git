/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>

namespace ns3
{

/**
 * @brief Convert a byte buffer to a string containing a hex representation
 * of the buffer.  Make the string pretty by adding a colon (':') between
 * the hex.
 *
 * @param buffer The input buffer to be converted.
 * @param len The length of the input buffer.
 * @returns A string containing a hex representation of the data in buffer.
 */
std::string
BufferToString(uint8_t* buffer, uint32_t len)
{
    std::ostringstream oss;
    //
    // Tell the stream to make hex characters, zero-filled
    //
    oss.setf(std::ios::hex, std::ios::basefield);
    oss.fill('0');

    //
    // Loop through the buffer, separating the two-digit-wide hex bytes
    // with a colon.
    //
    for (uint32_t i = 0; i < len; i++)
    {
        oss << ":" << std::setw(2) << (uint32_t)buffer[i];
    }
    return oss.str();
}

/**
 * @brief Convert string encoded by the inverse function (TapBufferToString)
 * back into a byte buffer.
 *
 * @param s The input string.
 * @param buffer The buffer to initialize with the converted bits.
 * @param len The length of the data that is valid in the buffer.
 * @returns True indicates a successful conversion.
 */
bool
StringToBuffer(std::string s, uint8_t* buffer, uint32_t* len)
{
    //
    // If the string was made by our inverse function, the string length must
    // be a multiple of three characters in length.  Use this fact to do a
    // quick reasonableness test.
    //
    if ((s.length() % 3) != 0)
    {
        return false;
    }

    std::istringstream iss;
    iss.str(s);

    uint8_t n = 0;

    while (iss.good())
    {
        //
        // The first character in the "triplet" we're working on is always the
        // the ':' separator.  Read that into a char and make sure we're skipping
        // what we think we're skipping.
        //
        char c;
        iss.read(&c, 1);
        if (c != ':')
        {
            return false;
        }

        //
        // And then read in the real bits and convert them.
        //
        uint32_t tmp;
        iss >> std::hex >> tmp;
        buffer[n] = tmp;
        n++;
    }

    *len = n;
    return true;
}

} // namespace ns3
