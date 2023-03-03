/*
 * Copyright (c) 2013 Lawrence Livermore National Laboratory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/core-module.h"

#include <iomanip>
#include <iostream>
#include <string>

/**
 * \file
 * \ingroup core-examples
 * \ingroup commandline
 * Example program illustrating use of ns3::CommandLine.
 */

using namespace ns3;

namespace
{

/**
 * Global variable to illustrate command line arguments handled by a
 * Callback function.
 */
std::string g_cbArg = "cbArg default";

/**
 * Function to illustrate command line arguments handled by a
 * Callback function.
 *
 * \param [in] val New value for \pname{g_cbArg}.
 * \returns \c true.
 */
bool
SetCbArg(const std::string& val)
{
    g_cbArg = val;
    return true;
}

} // unnamed namespace

/**
 * Print a row containing the name, the default
 * and the final values of an argument.
 *
 * \param [in] label The argument label.
 * \param [in] defaultValue The default value of the argument.
 * \param [in] finalValue The final value of the argument.
 *
 */
#define DefaultFinal(label, defaultValue, finalValue)                                              \
    std::left << std::setw(20) << label + std::string(":") << std::setw(20) << defaultValue        \
              << finalValue << "\n"

int
main(int argc, char* argv[])
{
    // Plain old data options
    int intArg = 1;
    bool boolArg = false;
    std::string strArg = "strArg default";

    // Attribute path option
    const std::string attrClass = "ns3::RandomVariableStream";
    const std::string attrName = "Antithetic";
    const std::string attrPath = attrClass + "::" + attrName;

    // char* buffer option
    constexpr int CHARBUF_SIZE = 10;
    char charbuf[CHARBUF_SIZE] = "charstar";

    // Non-option arguments
    int nonOpt1 = 1;
    int nonOpt2 = 1;

    // Cache the initial values.  Normally one wouldn't do this,
    // but we want to demonstrate that CommandLine has changed them.
    const int intDef = intArg;
    const bool boolDef = boolArg;
    const std::string strDef = strArg;
    const std::string cbDef = g_cbArg;
    // Look up default value for attribute
    const TypeId tid = TypeId::LookupByName(attrClass);
    std::string attrDef;
    {
        struct TypeId::AttributeInformation info;
        tid.LookupAttributeByName(attrName, &info);
        attrDef = info.originalInitialValue->SerializeToString(info.checker);
    }
    const std::string charbufDef{charbuf};
    const int nonOpt1Def = nonOpt1;
    const int nonOpt2Def = nonOpt2;

    CommandLine cmd(__FILE__);
    cmd.Usage("CommandLine example program.\n"
              "\n"
              "This little program demonstrates how to use CommandLine.");
    cmd.AddValue("intArg", "an int argument", intArg);
    cmd.AddValue("boolArg", "a bool argument", boolArg);
    cmd.AddValue("strArg", "a string argument", strArg);
    cmd.AddValue("anti", attrPath);
    cmd.AddValue("cbArg", "a string via callback", MakeCallback(SetCbArg));
    cmd.AddValue("charbuf", "a char* buffer", charbuf, CHARBUF_SIZE);
    cmd.AddNonOption("nonOpt1", "first non-option", nonOpt1);
    cmd.AddNonOption("nonOpt2", "second non-option", nonOpt2);
    cmd.Parse(argc, argv);

    // Show what happened
    std::cout << std::endl;
    std::cout << cmd.GetName() << ":" << std::endl;

    // Print the source version used to build this example
    std::cout << "Program Version: ";
    cmd.PrintVersion(std::cout);
    std::cout << std::endl;

    std::cout << "Argument            Initial Value       Final Value\n"
              << std::left << std::boolalpha;

    std::cout << DefaultFinal("intArg", intDef, intArg) //
              << DefaultFinal("boolArg",
                              (boolDef ? "true" : "false"),
                              (boolArg ? "true" : "false")) //
              << DefaultFinal("strArg", "\"" + strDef + "\"", "\"" + strArg + "\"");

    // Look up new default value for attribute
    std::string antiArg;
    {
        struct TypeId::AttributeInformation info;
        tid.LookupAttributeByName(attrName, &info);
        antiArg = info.initialValue->SerializeToString(info.checker);
    }

    std::cout << DefaultFinal("anti", "\"" + attrDef + "\"", "\"" + antiArg + "\"")
              << DefaultFinal("cbArg", cbDef, g_cbArg)
              << DefaultFinal("charbuf",
                              "\"" + charbufDef + "\"",
                              "\"" + std::string(charbuf) + "\"")
              << DefaultFinal("nonOpt1", nonOpt1Def, nonOpt1)
              << DefaultFinal("nonOpt2", nonOpt2Def, nonOpt2) << std::endl;

    std::cout << std::setw(40)
              << "Number of extra non-option arguments:" << cmd.GetNExtraNonOptions() << std::endl;

    for (std::size_t i = 0; i < cmd.GetNExtraNonOptions(); ++i)
    {
        std::cout << DefaultFinal("extra non-option " + std::to_string(i),
                                  "",
                                  "\"" + cmd.GetExtraNonOption(i) + "\"");
    }
    std::cout << std::endl;

#undef DefaultFinal

    return 0;
}
