/*
 * Copyright (c) 2026 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

/**
 * @file
 * Example to demonstrate the SequenceNumber class.
 */

#include "ns3/sequence-number.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    // Note: most sequence numbers have pre-defined aliases, like SequenceNumber16.
    // Here we use the template to define a reduced range sequence number.
    // plus, we define our own short name, for simplicity.
    using SequenceNumber4 = SequenceNumber<uint8_t, 4>;

    SequenceNumber4 seqNum1(0u);
    SequenceNumber4 seqNum2 = seqNum1;

    // Error: SequenceNumbers must have the same underlying type and number of bits
    // SequenceNumber<uint16_t, 4> seqNum3 = seqNum1;
    // SequenceNumber<uint8_t, 5> seqNum4 = seqNum1;

    for (size_t i = 0; i < SequenceNumber4::N_SEQUENCE_NUMBERS; i++)
    {
        std::string relation;
        if (seqNum2 == seqNum1)
        {
            relation = " = ";
        }
        else if (seqNum2 < seqNum1)
        {
            relation = " < ";
        }
        else
        {
            relation = " > ";
        }
        std::cout << "4-bit sequence number value " << seqNum2 << relation << seqNum1 << std::endl;
        seqNum2++;
    }

    std::cout << std::endl << "Table of comparisons (numbers are in hexadecimal):" << std::endl;
    std::cout << "   ";
    for (size_t i = 0; i < SequenceNumber4::N_SEQUENCE_NUMBERS; i++)
    {
        std::cout << std::hex << i << " ";
    }
    std::cout << std::endl;

    // We assign a value instead of incrementing the sequence number.
    // Just an alternative way.
    for (size_t i = 0; i < SequenceNumber4::N_SEQUENCE_NUMBERS; i++)
    {
        seqNum1 = i;
        std::cout << std::hex << seqNum1 << " ";

        for (size_t j = 0; j < SequenceNumber4::N_SEQUENCE_NUMBERS; j++)
        {
            seqNum2 = j;
            std::string relation;
            if (seqNum2 == seqNum1)
            {
                relation = " .";
            }
            else if (seqNum2 < seqNum1)
            {
                relation = " -";
            }
            else
            {
                relation = " +";
            }
            std::cout << relation;
        }
        std::cout << std::endl;
    }

    return 0;
}
