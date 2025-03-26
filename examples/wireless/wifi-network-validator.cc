/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/command-line.h"
#include "ns3/data-rate.h"
#include "ns3/log.h"
#include "ns3/si-units.h"

#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("wifi-network-validator");

int
main(int argc, char* argv[])
{
    std::string resultsFileName{""};
    DataRate minExpectedThroughput;
    DataRate maxExpectedThroughput;
    auto tolerance{0.0};
    auto printResultBanner{true};
    auto printLastOnly{false};

    CommandLine cmd(__FILE__);
    cmd.AddValue("resultsFile",
                 "file containing the list of files containing results",
                 resultsFileName);
    cmd.AddValue("minExpectedThroughput",
                 "if set, simulation fails if the lowest throughput is below this value",
                 minExpectedThroughput);
    cmd.AddValue("maxExpectedThroughput",
                 "if set, simulation fails if the highest throughput is above this value",
                 maxExpectedThroughput);
    cmd.AddValue("tolerance",
                 "accepted tolerance for difference between expected bounds and actual results",
                 tolerance);
    cmd.AddValue("printResultBanner", "Flag whether to print the result banner", printResultBanner);
    cmd.AddValue("printLastOnly",
                 "Flag whether only the last result shall be printed",
                 printLastOnly);
    cmd.Parse(argc, argv);

    if (printResultBanner)
    {
        std::cout << "MCS value"
                  << "\t\t"
                  << "Channel width"
                  << "\t\t"
                  << "GI"
                  << "\t\t\t"
                  << "Throughput" << '\n';
    }

    std::ifstream validationFile;
    validationFile.open(resultsFileName, std::ios::in);
    if (validationFile.fail())
    {
        NS_FATAL_ERROR("File " << resultsFileName << " not found");
    }

    uint8_t minMcs{std::numeric_limits<uint8_t>::max()};
    uint8_t maxMcs{std::numeric_limits<uint8_t>::min()};
    MHz_t minWidth{std::numeric_limits<uint16_t>::max()};
    MHz_t maxWidth{std::numeric_limits<uint16_t>::min()};
    auto minGi{Time::Max()};
    auto maxGi{Time::Min()}; // nanoseconds
    std::vector<
        std::tuple<uint8_t /* MCS */, MHz_t /* width */, Time /* GI */, std::string /* filename */>>
        runs{};
    while (!validationFile.eof())
    {
        std::string line;
        getline(validationFile, line);
        if (!line.empty())
        {
            std::size_t pos{0};
            const std::string delimiter{" "};
            std::vector<std::string> words{};
            do
            {
                pos = line.find(delimiter);
                words.push_back(line.substr(0, pos));
                line.erase(0, pos + delimiter.size());
            } while (pos != std::string::npos);
            NS_ABORT_MSG_IF(words.size() != 4, "Incorrect validation file");
            const uint8_t mcs = std::stod(words.at(0));
            minMcs = std::min(mcs, minMcs);
            maxMcs = std::max(mcs, maxMcs);
            const MHz_t width{words.at(1)};
            minWidth = std::min(width, minWidth);
            maxWidth = std::max(width, maxWidth);
            const Time gi{words.at(2)};
            minGi = std::min(gi, minGi);
            maxGi = std::max(gi, maxGi);
            const auto filename = words.at(3);
            runs.emplace_back(mcs, width, gi, filename);
        }
    }
    validationFile.close();

    std::vector<std::tuple<uint8_t, MHz_t, Time, DataRate>> results{};
    for (const auto& [mcs, width, gi, resultFileName] : runs)
    {
        std::ifstream resultFile;
        resultFile.open(resultFileName, std::ios::in);
        if (resultFile.fail())
        {
            NS_FATAL_ERROR("File " << resultFileName << " not found");
        }
        double throughput{0.0};
        while (!resultFile.eof())
        {
            std::string line;
            getline(resultFile, line);
            if (!line.empty())
            {
                throughput = std::stod(line);
            }
        }
        results.emplace_back(mcs, width, gi, throughput * 1e6);
    }

    std::optional<uint8_t> prevMcs;
    std::optional<DataRate> prevThroughput;
    std::map<std::pair<MHz_t, Time>, DataRate> prevThroughputs;
    std::size_t resultIndex{0};
    for (const auto& [mcs, width, gi, throughput] : results)
    {
        if (!printLastOnly || (printLastOnly && (++resultIndex == results.size())))
        {
            std::cout << +mcs << "\t\t\t" << width << "\t\t\t" << gi.GetNanoSeconds() << " ns\t\t\t"
                      << throughput.GetBitRate() * 1e-6 << " Mbit/s" << std::endl;
        }
        if (mcs != prevMcs)
        {
            prevThroughput = 0.0;
            prevMcs = mcs;
        }
        // test first element
        if (mcs == minMcs && width == minWidth && gi == maxGi)
        {
            if (throughput * (1.0 + tolerance) < minExpectedThroughput)
            {
                NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                exit(1);
            }
        }

        // test last element
        if (mcs == maxMcs && width == maxWidth && gi == minGi)
        {
            if (maxExpectedThroughput > 0.0 &&
                throughput > maxExpectedThroughput * (1.0 + tolerance))
            {
                NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                exit(1);
            }
        }

        // test previous throughput is smaller (for the same mcs)
        if (throughput * (1.0 + tolerance) > *prevThroughput)
        {
            prevThroughput = throughput;
        }
        else if (throughput > 0.0)
        {
            NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
            exit(1);
        }

        // test previous throughput is smaller (for the same channel width and GI)
        const auto widthGiPair = std::make_pair(width, gi);
        auto [it, inserted] = prevThroughputs.try_emplace(widthGiPair, DataRate());
        if (throughput * (1.0 + tolerance) > it->second)
        {
            it->second = throughput;
        }
        else if (throughput > 0.0)
        {
            NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
            exit(1);
        }
    }

    return 0;
}
