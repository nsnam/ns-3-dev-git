/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include <ns3/command-line.h>
#include <ns3/fatal-error.h>
#include <ns3/log.h>

#include <cstdlib>
#include <fstream>
#include <list>
#include <string>
#include <thread>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("parallel-scheduler");

int
main(int argc, char* argv[])
{
    std::string inputFileName{""};
    auto numParallelJobs{std::thread::hardware_concurrency()};

    CommandLine cmd(__FILE__);
    cmd.AddValue("inputFileName",
                 "location of the file containing the programs to execute in parallel and the "
                 "parameters for each run",
                 inputFileName);
    cmd.AddValue("numParallelJobs",
                 "the number of jobs to execute in parallel (by default it corresponds to the "
                 "number of hardware thread contexts)",
                 numParallelJobs);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(inputFileName.empty(), "Input file not specified");
    std::ifstream inputFile;
    inputFile.open(inputFileName, std::ios::in);
    if (inputFile.fail())
    {
        NS_FATAL_ERROR("File " << inputFileName << " not found");
    }
    std::list<std::string> commandsToRun{};
    while (!inputFile.eof())
    {
        std::string command;
        getline(inputFile, command);
        if (command.empty())
        {
            continue;
        }
        commandsToRun.push_back(command);
    }
    inputFile.close();

    const bool useParallel{!std::system("command -v parallel > /dev/null 2>&1")};
    const std::string parallelOpts{"--will-cite"};
    auto sysCmd{useParallel ? ("parallel " + parallelOpts + " -j" +
                               std::to_string(numParallelJobs) + " ::: ")
                            : ""};
    std::size_t commandIndex{0};
    for (const auto& commandToRun : commandsToRun)
    {
        const auto command = "./ns3 run --no-build \"" + commandToRun + "\"";
        NS_LOG_INFO((useParallel ? "parallel" : "sequential")
                    << " scheduling of command: " << command);
        if (useParallel)
        {
            sysCmd += "'" + command + "' ";
        }
        else
        {
            const auto isLastCmd = (commandIndex == (commandsToRun.size() - 1));
            sysCmd += command + (isLastCmd ? "" : " && ");
        }
        ++commandIndex;
    }

    // TODO: find an alternative to std::system since there is no guarantee on its output value.
    // For all supported cases, std::system returns the exit code of the invoked program.
    if (std::system(sysCmd.c_str()))
    {
        exit(1);
    }

    return 0;
}
