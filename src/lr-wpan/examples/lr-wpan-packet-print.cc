/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Tom Henderson <thomas.r.henderson@boeing.com>
 */
#include "ns3/core-module.h"
#include "ns3/lr-wpan-module.h"

#include <iostream>

using namespace ns3;
using namespace ns3::lrwpan;

int
main(int argc, char* argv[])
{
    Packet::EnablePrinting();
    Packet::EnableChecking();
    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_BEACON, 0); // sequence number set to 0
    macHdr.SetSrcAddrMode(2);                                      // short addr
    macHdr.SetDstAddrMode(0);                                      // no addr
    // ... other setters

    uint16_t srcPanId = 100;
    Mac16Address srcWpanAddr("00:11");

    macHdr.SetSrcAddrFields(srcPanId, srcWpanAddr);

    LrWpanMacTrailer macTrailer;

    Ptr<Packet> p = Create<Packet>(20); // 20 bytes of dummy data

    p->AddHeader(macHdr);
    p->AddTrailer(macTrailer);

    p->Print(std::cout);
    std::cout << std::endl;
    return 0;
}
