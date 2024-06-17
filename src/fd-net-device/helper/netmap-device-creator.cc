/*
 * Copyright (c) University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "creator-utils.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define EMU_MAGIC 65867

using namespace ns3;

int
main(int argc, char* argv[])
{
    int c;
    char* path = nullptr;

    opterr = 0;

    while ((c = getopt(argc, argv, "vp:")) != -1)
    {
        switch (c)
        {
        case 'v':
            gVerbose = true;
            break;
        case 'p':
            path = optarg;
            break;
        }
    }

    //
    // This program is spawned by an emu net device running in a simulation.  It
    // wants to create a raw socket as described below.  We are going to do the
    // work here since we're running suid root.  Once we create the raw socket,
    // we have to send it back to the emu net device.  We do that over a Unix
    // (local interprocess) socket.  The emu net device created a socket to
    // listen for our response on, and it is expected to have encoded the address
    // information as a string and to have passed that string as an argument to
    // us.  We see it here as the "path" string.  We can't do anything useful
    // unless we have that string.
    //
    ABORT_IF(!path, "path is a required argument", 0);
    LOG("Provided path is \"" << path << "\"");
    //
    // The whole reason for all of the hoops we went through to call out to this
    // program will pay off here.  We created this program to run as suid root
    // in order to keep the main simulation program from having to be run with
    // root privileges.  We need root privileges to be able to open a raw socket
    // though.  So all of these hoops are to allow us to execute the following
    // single line of code:
    //
    LOG("Creating netmap fd");
    int sock = open("/dev/netmap", O_RDWR);
    ABORT_IF(sock == -1, "CreateSocket(): Unable to open netmap fd", 1);

    //
    // Send the socket back to the emu net device so it can go about its business
    //
    SendSocket(path, sock, EMU_MAGIC);

    return 0;
}
