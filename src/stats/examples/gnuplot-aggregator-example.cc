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
 * This function creates a 2-Dimensional plot.
 */
void
Create2dPlot()
{
    std::string fileNameWithoutExtension = "gnuplot-aggregator";
    std::string plotTitle = "Gnuplot Aggregator Plot";
    std::string plotXAxisHeading = "Time (seconds)";
    std::string plotYAxisHeading = "Double Values";
    std::string plotDatasetLabel = "Data Values";
    std::string datasetContext = "Dataset/Context/String";

    // Create an aggregator.
    Ptr<GnuplotAggregator> aggregator = CreateObject<GnuplotAggregator>(fileNameWithoutExtension);

    // Set the aggregator's properties.
    aggregator->SetTerminal("png");
    aggregator->SetTitle(plotTitle);
    aggregator->SetLegend(plotXAxisHeading, plotYAxisHeading);

    // Add a data set to the aggregator.
    aggregator->Add2dDataset(datasetContext, plotDatasetLabel);

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

} // unnamed namespace

int
main(int argc, char* argv[])
{
    Create2dPlot();

    return 0;
}
