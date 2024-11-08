/*
 * Copyright (c) 2008 Timo Bingmann
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Timo Bingmann <timo.bingmann@student.kit.edu>
 */

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/double.h"
#include "ns3/gnuplot.h"
#include "ns3/jakes-propagation-loss-model.h"
#include "ns3/pointer.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

#include <map>

using namespace ns3;

/**
 * Round a double number to the given precision. e.g. dround(0.234, 0.1) = 0.2
 * and dround(0.257, 0.1) = 0.3
 *
 * @param number The number to round.
 * @param precision The precision.
 * @return the rounded number
 */
static double
dround(double number, double precision)
{
    number /= precision;
    if (number >= 0)
    {
        number = floor(number + 0.5);
    }
    else
    {
        number = ceil(number - 0.5);
    }
    number *= precision;
    return number;
}

/**
 * Test the model by sampling over a distance.
 *
 * @param model The model to test.
 * @param targetDistance The target distance.
 * @param step The step.
 * @return a Gnuplot object to be plotted.
 */
static Gnuplot
TestDeterministic(Ptr<PropagationLossModel> model, double targetDistance, double step)
{
    Ptr<ConstantPositionMobilityModel> a = CreateObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> b = CreateObject<ConstantPositionMobilityModel>();

    Gnuplot plot;

    plot.AppendExtra("set xlabel 'Distance'");
    plot.AppendExtra("set ylabel 'rxPower (dBm)'");
    plot.AppendExtra("set key top right");

    double txPowerDbm = +20; // dBm

    Gnuplot2dDataset dataset;

    dataset.SetStyle(Gnuplot2dDataset::LINES);

    {
        a->SetPosition(Vector(0.0, 0.0, 0.0));

        for (double distance = 0.0; distance < targetDistance; distance += step)
        {
            b->SetPosition(Vector(distance, 0.0, 0.0));

            // CalcRxPower() returns dBm.
            double rxPowerDbm = model->CalcRxPower(txPowerDbm, a, b);

            dataset.Add(distance, rxPowerDbm);

            Simulator::Stop(Seconds(1));
            Simulator::Run();
        }
    }

    std::ostringstream os;
    os << "txPower " << txPowerDbm << "dBm";
    dataset.SetTitle(os.str());

    plot.AddDataset(dataset);

    plot.AddDataset(Gnuplot2dFunction("-94 dBm CSThreshold", "-94.0"));

    return plot;
}

/**
 * Test the model by sampling over a distance.
 *
 * @param model The model to test.
 * @param targetDistance The target distance.
 * @param step The step.
 * @param samples Number of samples.
 * @return a Gnuplot object to be plotted.
 */
static Gnuplot
TestProbabilistic(Ptr<PropagationLossModel> model,
                  double targetDistance,
                  double step,
                  unsigned int samples)
{
    Ptr<ConstantPositionMobilityModel> a = CreateObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> b = CreateObject<ConstantPositionMobilityModel>();

    Gnuplot plot;

    plot.AppendExtra("set xlabel 'Distance'");
    plot.AppendExtra("set ylabel 'rxPower (dBm)'");
    plot.AppendExtra("set zlabel 'Probability' offset 0,+10");
    plot.AppendExtra("set view 50, 120, 1.0, 1.0");
    plot.AppendExtra("set key top right");

    plot.AppendExtra("set ticslevel 0");
    plot.AppendExtra("set xtics offset -0.5,0");
    plot.AppendExtra("set ytics offset 0,-0.5");
    plot.AppendExtra("set xrange [100:]");

    double txPowerDbm = +20; // dBm

    Gnuplot3dDataset dataset;

    dataset.SetStyle("with linespoints");
    dataset.SetExtra("pointtype 3 pointsize 0.5");

    typedef std::map<double, unsigned int> rxPowerMapType;

    // Take given number of samples from CalcRxPower() and show probability
    // density for discrete distances.
    {
        a->SetPosition(Vector(0.0, 0.0, 0.0));

        for (double distance = 100.0; distance < targetDistance; distance += step)
        {
            b->SetPosition(Vector(distance, 0.0, 0.0));

            rxPowerMapType rxPowerMap;

            for (unsigned int samp = 0; samp < samples; ++samp)
            {
                // CalcRxPower() returns dBm.
                double rxPowerDbm = model->CalcRxPower(txPowerDbm, a, b);
                rxPowerDbm = dround(rxPowerDbm, 1.0);

                rxPowerMap[rxPowerDbm]++;

                Simulator::Stop(Seconds(0.01));
                Simulator::Run();
            }

            for (auto i = rxPowerMap.begin(); i != rxPowerMap.end(); ++i)
            {
                dataset.Add(distance, i->first, (double)i->second / (double)samples);
            }
            dataset.AddEmptyLine();
        }
    }

    std::ostringstream os;
    os << "txPower " << txPowerDbm << "dBm";
    dataset.SetTitle(os.str());

    plot.AddDataset(dataset);

    return plot;
}

/**
 * Test the model by sampling over time.
 *
 * @param model The model to test.
 * @param timeStep The time step.
 * @param timeTotal The total time.
 * @param distance The distance.
 * @return a Gnuplot object to be plotted.
 */
static Gnuplot
TestDeterministicByTime(Ptr<PropagationLossModel> model,
                        Time timeStep,
                        Time timeTotal,
                        double distance)
{
    Ptr<ConstantPositionMobilityModel> a = CreateObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> b = CreateObject<ConstantPositionMobilityModel>();

    Gnuplot plot;

    plot.AppendExtra("set xlabel 'Time (s)'");
    plot.AppendExtra("set ylabel 'rxPower (dBm)'");
    plot.AppendExtra("set key center right");

    double txPowerDbm = +20; // dBm

    Gnuplot2dDataset dataset;

    dataset.SetStyle(Gnuplot2dDataset::LINES);

    {
        a->SetPosition(Vector(0.0, 0.0, 0.0));
        b->SetPosition(Vector(distance, 0.0, 0.0));

        Time start = Simulator::Now();
        while (Simulator::Now() < start + timeTotal)
        {
            // CalcRxPower() returns dBm.
            double rxPowerDbm = model->CalcRxPower(txPowerDbm, a, b);

            Time elapsed = Simulator::Now() - start;
            dataset.Add(elapsed.GetSeconds(), rxPowerDbm);

            Simulator::Stop(timeStep);
            Simulator::Run();
        }
    }

    std::ostringstream os;
    os << "txPower " << txPowerDbm << "dBm";
    dataset.SetTitle(os.str());

    plot.AddDataset(dataset);

    plot.AddDataset(Gnuplot2dFunction("-94 dBm CSThreshold", "-94.0"));

    return plot;
}

int
main(int argc, char* argv[])
{
    bool test = false;
    CommandLine cmd(__FILE__);
    cmd.AddValue("test", "Run as a test, sample the models only once", test);
    cmd.Parse(argc, argv);

    double testDeterministicDistance = 2500.0;
    double testProbabilisticDistance = 2500.0;
    unsigned int testProbabilisticSamples = 100000;
    Time testJakesTimeOneMsRes = Seconds(1);
    Time testJakesTimeZeroDotOneMsRes = Seconds(0.1);

    if (test)
    {
        testDeterministicDistance = 10;
        testProbabilisticDistance = 200;
        testProbabilisticSamples = 1;
        testJakesTimeOneMsRes = Seconds(0.001);
        testJakesTimeZeroDotOneMsRes = Seconds(0.0001);
    }

    GnuplotCollection gnuplots("main-propagation-loss.pdf");

    {
        Ptr<FriisPropagationLossModel> friis = CreateObject<FriisPropagationLossModel>();

        Gnuplot plot = TestDeterministic(friis, testDeterministicDistance, 10.0);
        plot.SetTitle("ns3::FriisPropagationLossModel (Default Parameters)");
        gnuplots.AddPlot(plot);
    }

    {
        Ptr<LogDistancePropagationLossModel> log = CreateObject<LogDistancePropagationLossModel>();
        log->SetAttribute("Exponent", DoubleValue(2.5));

        Gnuplot plot = TestDeterministic(log, testDeterministicDistance, 10.0);
        plot.SetTitle("ns3::LogDistancePropagationLossModel (Exponent = 2.5)");
        gnuplots.AddPlot(plot);
    }

    {
        Ptr<RandomPropagationLossModel> random = CreateObject<RandomPropagationLossModel>();
        Ptr<ExponentialRandomVariable> expVar =
            CreateObjectWithAttributes<ExponentialRandomVariable>("Mean", DoubleValue(50.0));
        random->SetAttribute("Variable", PointerValue(expVar));

        Gnuplot plot = TestDeterministic(random, testDeterministicDistance, 10.0);
        plot.SetTitle("ns3::RandomPropagationLossModel with Exponential Distribution");
        gnuplots.AddPlot(plot);
    }

    {
        Ptr<JakesPropagationLossModel> jakes = CreateObject<JakesPropagationLossModel>();

        // doppler frequency shift for 5.15 GHz at 100 km/h
        Config::SetDefault("ns3::JakesProcess::DopplerFrequencyHz", DoubleValue(477.9));

        Gnuplot plot = TestDeterministicByTime(jakes, Seconds(0.001), testJakesTimeOneMsRes, 100.0);
        plot.SetTitle(
            "ns3::JakesPropagationLossModel (with 477.9 Hz shift and 1 millisec resolution)");
        gnuplots.AddPlot(plot);
        // Usually objects are aggregated either to a Node or a Channel, and this aggregation
        // ensures a proper call to Dispose. Here we must call it manually, since the
        // PropagationLossModel is not aggregated to anything.
        jakes->Dispose();
    }

    {
        Ptr<JakesPropagationLossModel> jakes = CreateObject<JakesPropagationLossModel>();

        // doppler frequency shift for 5.15 GHz at 100 km/h
        Config::SetDefault("ns3::JakesProcess::DopplerFrequencyHz", DoubleValue(477.9));

        Gnuplot plot =
            TestDeterministicByTime(jakes, Seconds(0.0001), testJakesTimeZeroDotOneMsRes, 100.0);
        plot.SetTitle(
            "ns3::JakesPropagationLossModel (with 477.9 Hz shift and 0.1 millisec resolution)");
        gnuplots.AddPlot(plot);
        // Usually objects are aggregated either to a Node or a Channel, and this aggregation
        // ensures a proper call to Dispose. Here we must call it manually, since the
        // PropagationLossModel is not aggregated to anything.
        jakes->Dispose();
    }

    {
        Ptr<ThreeLogDistancePropagationLossModel> log3 =
            CreateObject<ThreeLogDistancePropagationLossModel>();

        Gnuplot plot = TestDeterministic(log3, testDeterministicDistance, 10.0);
        plot.SetTitle("ns3::ThreeLogDistancePropagationLossModel (Defaults)");
        gnuplots.AddPlot(plot);
    }

    {
        Ptr<ThreeLogDistancePropagationLossModel> log3 =
            CreateObject<ThreeLogDistancePropagationLossModel>();
        // more prominent example values:
        log3->SetAttribute("Exponent0", DoubleValue(1.0));
        log3->SetAttribute("Exponent1", DoubleValue(3.0));
        log3->SetAttribute("Exponent2", DoubleValue(10.0));

        Gnuplot plot = TestDeterministic(log3, testDeterministicDistance, 10.0);
        plot.SetTitle("ns3::ThreeLogDistancePropagationLossModel (Exponents 1.0, 3.0 and 10.0)");
        gnuplots.AddPlot(plot);
    }

    {
        Ptr<NakagamiPropagationLossModel> nak = CreateObject<NakagamiPropagationLossModel>();

        Gnuplot plot =
            TestProbabilistic(nak, testProbabilisticDistance, 100.0, testProbabilisticSamples);
        plot.SetTitle("ns3::NakagamiPropagationLossModel (Default Parameters)");
        gnuplots.AddPlot(plot);
    }

    {
        Ptr<ThreeLogDistancePropagationLossModel> log3 =
            CreateObject<ThreeLogDistancePropagationLossModel>();

        Ptr<NakagamiPropagationLossModel> nak = CreateObject<NakagamiPropagationLossModel>();
        log3->SetNext(nak);

        Gnuplot plot =
            TestProbabilistic(log3, testProbabilisticDistance, 100.0, testProbabilisticSamples);
        plot.SetTitle("ns3::ThreeLogDistancePropagationLossModel and "
                      "ns3::NakagamiPropagationLossModel (Default Parameters)");
        gnuplots.AddPlot(plot);
    }

    gnuplots.GenerateOutput(std::cout);

    // produce clean valgrind
    Simulator::Destroy();
    return 0;
}
