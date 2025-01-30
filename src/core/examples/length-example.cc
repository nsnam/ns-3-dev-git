/*
 * Copyright (c) 2019 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathew Bielejeski <bielejeski1@llnl.gov>
 */

#include "ns3/core-module.h"
#include "ns3/length.h"

#include <iostream>

/**
 * @defgroup length-examples Demonstrates usage of the ns3::Length class
 * @ingroup core-examples
 * @ingroup length
 */

/**
 * @file
 * @ingroup length-examples
 * Demonstrates usage of the ns3::Length class
 */

using namespace ns3;

/**
 * @ingroup length-examples
 * @brief Demonstrates the use of ns3::Length constructors.
 */
void
Constructors()
{
    double input = 5;
    Length::Quantity quantity(input, Length::Unit::Meter);

    std::cout << "\nConstructors:\n"
              << "Length (" << input << ", Unit::Meter) = " << Length(input, Length::Unit::Meter)
              << "\n"
              << "Length (" << input << ", \"m\") = " << Length(input, "m") << "\n"
              << "Length (" << input << ", \"meter\") = " << Length(input, "meter") << "\n"
              << "Length (Quantity(" << input << ", Unit::Meter)) = " << Length(quantity) << "\n"
              << "Length (\"5m\") = " << Length("5m") << "\n"
              << "Length (\"5 m\") = " << Length("5 m") << "\n"
              << "Length (\"5meter\") = " << Length("5meter") << "\n"
              << "Length (\"5 meter\") = " << Length("5 meter") << "\n"
              << "Length (\"5meters\") = " << Length("5meters") << "\n"
              << "Length (\"5 meters\") = " << Length("5 meters") << std::endl;
}

/**
 * @ingroup length-examples
 * @brief Demonstrates the use of ns3::Length conversions.
 */
void
Conversions()
{
    // construct length using value and unit
    Length moonDistance(3.84402e8, Length::Unit::Meter);

    // Demonstrate conversion to various units
    std::cout << "\n"
              << "Conversions:\n"
              << "Distance to moon = " << moonDistance << "\n"
              << "In Feet: " << moonDistance.As(Length::Unit::Foot) << "\n"
              << "In Miles: " << moonDistance.As(Length::Unit::Mile) << "\n"
              << "In Kilometers: " << moonDistance.As(Length::Unit::Kilometer) << std::endl;
}

/**
 * @ingroup length-examples
 * @brief Demonstrates the use of ns3::Length arithmetic operators.
 */
void
ArithmeticOperators()
{
    double scale = 10;

    // construct lengths using helper functions
    Length oneMeter = Meters(1);
    Length twoMeter = Meters(2);

    std::cout << "\n"
              << "Arithmetic Operations:\n"
              << "Addition: " << oneMeter << " + " << twoMeter << " = " << (oneMeter + twoMeter)
              << "\n"
              << "Subtraction: " << twoMeter << " - " << oneMeter << " = " << (twoMeter - oneMeter)
              << "\n"
              << "Multiplication By Scalar: " << oneMeter << " * " << scale << " = "
              << (oneMeter * scale) << "\n"
              << "Division: " << oneMeter << " / " << twoMeter << " = " << (oneMeter / twoMeter)
              << "\n"
              << "Division By Scalar: " << oneMeter << " / " << scale << " = " << (oneMeter / scale)
              << std::endl;
}

/**
 * @ingroup length-examples
 * @brief Demonstrates the use of ns3::Length equality operators.
 */
void
EqualityOperators()
{
    Length oneMeter = Meters(1);
    Length twoMeter = Meters(2);
    Length threeMeter = Meters(3);

    // NOLINTBEGIN(misc-redundant-expression)
    std::cout << "\n"
              << "Comparison Operations:\n"
              << std::boolalpha << "Equality: " << oneMeter << " == " << oneMeter << " is "
              << (oneMeter == oneMeter) << "\n"
              << "Equality: " << oneMeter << " == " << twoMeter << " is " << (oneMeter == twoMeter)
              << "\n"
              << "Inequality: " << oneMeter << " != " << oneMeter << " is "
              << (oneMeter != oneMeter) << "\n"
              << "Inequality: " << oneMeter << " != " << twoMeter << " is "
              << (oneMeter != twoMeter) << "\n"
              << "Lesser: " << oneMeter << " < " << oneMeter << " is " << (oneMeter < oneMeter)
              << "\n"
              << "Lesser: " << oneMeter << " < " << twoMeter << " is " << (oneMeter < twoMeter)
              << "\n"
              << "Lesser: " << threeMeter << " < " << oneMeter << " is " << (threeMeter < oneMeter)
              << "\n"
              << "Greater: " << oneMeter << " > " << oneMeter << " is " << (oneMeter > oneMeter)
              << "\n"
              << "Greater: " << oneMeter << " > " << twoMeter << " is " << (oneMeter > twoMeter)
              << "\n"
              << "Greater: " << threeMeter << " > " << oneMeter << " is " << (threeMeter > oneMeter)
              << std::endl;
    // NOLINTEND(misc-redundant-expression)
}

/**
 * @ingroup length-examples
 * @brief Demonstrates the use of ns3::Length multiplications and divisions.
 */
void
DivAndMod()
{
    // construct length using helper function
    Length totalLen = Feet(20);
    Length pieceLen = Feet(3);
    Length remainder;

    int64_t count = Div(totalLen, pieceLen, &remainder);

    std::cout << "\nHow many times can a " << totalLen.As(Length::Unit::Foot) << " length "
              << "be split into " << pieceLen.As(Length::Unit::Foot) << " sized pieces? " << count
              << "\nremainder: " << remainder.As(Length::Unit::Foot) << std::endl;

    std::cout << "\nHow much remains after splitting a " << totalLen.As(Length::Unit::Foot)
              << " length into " << pieceLen.As(Length::Unit::Foot) << " sized pieces? "
              << Mod(totalLen, pieceLen).As(Length::Unit::Foot) << std::endl;
}

int
main(int argc, char** argv)
{
    Constructors();
    Conversions();
    ArithmeticOperators();
    EqualityOperators();
    DivAndMod();

    return 0;
}
