/*
 * Copyright (c) 2013 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mitch Watrous (watrous@u.washington.edu)
 */

#include "ns3/core-module.h"
#include "ns3/stats-module.h"

using namespace ns3;

namespace
{

/**
 * This function creates a file with 2 columns of values and separated
 * by commas.
 */
void
CreateCommaSeparatedFile()
{
    std::string fileName = "file-aggregator-comma-separated.txt";
    std::string datasetContext = "Dataset/Context/String";

    // Create an aggregator.
    Ptr<FileAggregator> aggregator =
        CreateObject<FileAggregator>(fileName, FileAggregator::COMMA_SEPARATED);

    // aggregator must be turned on
    aggregator->Enable();

    double time;
    double value;

    // Create the 2-D dataset.
    for (time = -5.0; time <= +5.0; time += 1.0)
    {
        // Calculate the 2-D curve
        //
        //                   2
        //     value  =  time   .
        //
        value = time * time;

        // Add this point to the plot.
        aggregator->Write2d(datasetContext, time, value);
    }

    // Disable logging of data for the aggregator.
    aggregator->Disable();
}

/**
 * This function creates a file with 2 columns of values and separated
 * by commas.
 */
void
CreateSpaceSeparatedFile()
{
    std::string fileName = "file-aggregator-space-separated.txt";
    std::string datasetContext = "Dataset/Context/String";

    // Create an aggregator.  Note that the default type is space
    // separated.
    Ptr<FileAggregator> aggregator = CreateObject<FileAggregator>(fileName);

    // aggregator must be turned on
    aggregator->Enable();

    double time;
    double value;

    // Create the 2-D dataset.
    for (time = -5.0; time <= +5.0; time += 1.0)
    {
        // Calculate the 2-D curve
        //
        //                   2
        //     value  =  time   .
        //
        value = time * time;

        // Add this point to the plot.
        aggregator->Write2d(datasetContext, time, value);
    }

    // Disable logging of data for the aggregator.
    aggregator->Disable();
}

/**
 * This function creates a file with formatted values.
 */
void
CreateFormattedFile()
{
    std::string fileName = "file-aggregator-formatted-values.txt";
    std::string datasetContext = "Dataset/Context/String";

    // Create an aggregator that will have formatted values.
    Ptr<FileAggregator> aggregator =
        CreateObject<FileAggregator>(fileName, FileAggregator::FORMATTED);

    // Set the format for the values.
    aggregator->Set2dFormat("Time = %.3e\tValue = %.0f");

    // aggregator must be turned on
    aggregator->Enable();

    double time;
    double value;

    // Create the 2-D dataset.
    for (time = -5.0; time < 5.5; time += 1.0)
    {
        // Calculate the 2-D curve
        //
        //                   2
        //     value  =  time   .
        //
        value = time * time;

        // Add this point to the plot.
        aggregator->Write2d(datasetContext, time, value);
    }

    // Disable logging of data for the aggregator.
    aggregator->Disable();
}

} // unnamed namespace

int
main(int argc, char* argv[])
{
    CreateCommaSeparatedFile();
    CreateSpaceSeparatedFile();
    CreateFormattedFile();

    return 0;
}
