/*
 * Copyright (c) University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef CREATOR_UTILS_H
#define CREATOR_UTILS_H

#include <cstring>
#include <errno.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace ns3
{

extern bool gVerbose;

#define LOG(msg)                                                                                   \
    if (gVerbose)                                                                                  \
    {                                                                                              \
        std::cout << __FUNCTION__ << "(): " << msg << std::endl;                                   \
    }

#define ABORT(msg, printErrno)                                                                     \
    std::cout << __FILE__ << ": fatal error at line " << __LINE__ << ": " << __FUNCTION__          \
              << "(): " << msg << std::endl;                                                       \
    if (printErrno)                                                                                \
    {                                                                                              \
        std::cout << "    errno = " << errno << " (" << strerror(errno) << ")" << std::endl;       \
    }                                                                                              \
    exit(-1);

#define ABORT_IF(cond, msg, printErrno)                                                            \
    if (cond)                                                                                      \
    {                                                                                              \
        ABORT(msg, printErrno);                                                                    \
    }

/**
 * @ingroup fd-net-device
 * @brief Send the file descriptor back to the code that invoked the creation.
 *
 * @param path The socket address information from the Unix socket we use
 * to send the created socket back to.
 * @param fd The file descriptor we're going to send.
 * @param magic_number A verification number to verify the caller is talking to the
 * right process.
 */
void SendSocket(const char* path, int fd, const int magic_number);

} // namespace ns3

#endif /* CREATOR_UTILS_DEVICE_H */
