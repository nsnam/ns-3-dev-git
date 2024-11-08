/*
 * Copyright (c) 2012 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mitch Watrous (watrous@u.washington.edu)
 *
 * This file is based on pcap-test.h by Craig Dowell (craigdo@ee.washington.edu)
 */

#ifndef ASCII_TEST_H
#define ASCII_TEST_H

#include "ascii-file.h"
#include "test.h"

#include <stdint.h>

/**
 * @brief Test that a pair of new/reference ascii files are equal
 *
 * @param gotFilename The name of the new file to read in including
 * its path
 * @param expectedFilename The name of the reference file to read in
 * including its path
 */
#define NS_ASCII_TEST_EXPECT_EQ(gotFilename, expectedFilename)                                     \
    do                                                                                             \
    {                                                                                              \
        uint64_t line(0);                                                                          \
        bool diff = AsciiFile::Diff(gotFilename, expectedFilename, line);                          \
        NS_TEST_EXPECT_MSG_EQ(diff,                                                                \
                              false,                                                               \
                              "ASCII traces " << gotFilename << " and " << expectedFilename        \
                                              << " differ starting from line " << line);           \
    } while (false)

#endif /* ASCII_TEST_H */
