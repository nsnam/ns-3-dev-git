/*
 * Copyright (c) 2008 Timo Bingmann
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
 * Author: Timo Bingmann <timo.bingmann@student.kit.edu>
 */
#include "ns3/command-line.h"
#include "ns3/double.h"
#include "ns3/gnuplot.h"
#include "ns3/integer.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/string.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <map>

/**
 * \file
 * \ingroup core-examples
 * \ingroup randomvariable
 * Example program illustrating use of RandomVariableStream
 */

using namespace ns3;

namespace
{

/**
 * Round a double number to the given precision.
 * For example, `dround(0.234, 0.1) = 0.2`
 * and `dround(0.257, 0.1) = 0.3`
 * \param [in] number The number to round.
 * \param [in] precision The least significant digit to keep in the rounding.
 * \returns \pname{number} rounded to \pname{precision}.
 */
double
dround(double number, double precision)
{
    number /= precision;
    if (number >= 0)
    {
        number = std::floor(number + 0.5);
    }
    else
    {
        number = std::ceil(number - 0.5);
    }
    number *= precision;
    return number;
}

/**
 * Generate a histogram from a RandomVariableStream.
 * \param [in] rndvar The RandomVariableStream to sample.
 * \param [in] probes The number of samples.
 * \param [in] precision The precision to round samples to.
 * \param [in] title The title for the histogram.
 * \param [in] impulses Set the plot style to IMPULSES.
 * \return The histogram as a GnuPlot data set.
 */
GnuplotDataset
Histogram(Ptr<RandomVariableStream> rndvar,
          unsigned int probes,
          double precision,
          const std::string& title,
          bool impulses = false)
{
    typedef std::map<double, unsigned int> histogram_maptype;
    histogram_maptype histogram;

    for (unsigned int i = 0; i < probes; ++i)
    {
        double val = dround(rndvar->GetValue(), precision);

        ++histogram[val];
    }

    Gnuplot2dDataset data;
    data.SetTitle(title);

    if (impulses)
    {
        data.SetStyle(Gnuplot2dDataset::IMPULSES);
    }

    for (auto hi = histogram.begin(); hi != histogram.end(); ++hi)
    {
        data.Add(hi->first, (double)hi->second / (double)probes / precision);
    }

    return data;
}

} // unnamed namespace

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    unsigned int probes = 1000000;
    double precision = 0.01;

    const std::string plotFile{"main-random-variables"};
    GnuplotCollection gnuplots(plotFile + ".pdf");
    gnuplots.SetTerminal("pdf enhanced");

    {
        std::cout << "UniformRandomVariable......." << std::flush;
        Gnuplot plot;
        plot.SetTitle("UniformRandomVariable");
        plot.AppendExtra("set yrange [0:]");

        auto x = CreateObject<UniformRandomVariable>();
        x->SetAttribute("Min", DoubleValue(0.0));
        x->SetAttribute("Max", DoubleValue(1.0));

        plot.AddDataset(Histogram(x, probes, precision, "UniformRandomVariable [0.0 .. 1.0)"));
        plot.AddDataset(Gnuplot2dFunction("1.0", "0 <= x && x <= 1 ? 1.0 : 0"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "ExponentialRandomVariable..." << std::flush;
        Gnuplot plot;
        plot.SetTitle("ExponentialRandomVariable");
        plot.AppendExtra("set xrange [0:4]");
        plot.AppendExtra("ExpDist(x,l) = 1/l * exp(-1/l * x)");

        auto x1 = CreateObject<ExponentialRandomVariable>();
        x1->SetAttribute("Mean", DoubleValue(0.5));

        plot.AddDataset(Histogram(x1, probes, precision, "ExponentialRandomVariable m=0.5"));

        plot.AddDataset(Gnuplot2dFunction("ExponentialDistribution mean 0.5", "ExpDist(x, 0.5)"));

        auto x2 = CreateObject<ExponentialRandomVariable>();
        x2->SetAttribute("Mean", DoubleValue(1.0));

        plot.AddDataset(Histogram(x2, probes, precision, "ExponentialRandomVariable m=1"));

        plot.AddDataset(Gnuplot2dFunction("ExponentialDistribution mean 1.0", "ExpDist(x, 1.0)"));

        auto x3 = CreateObject<ExponentialRandomVariable>();
        x3->SetAttribute("Mean", DoubleValue(1.5));

        plot.AddDataset(Histogram(x3, probes, precision, "ExponentialRandomVariable m=1.5"));

        plot.AddDataset(Gnuplot2dFunction("ExponentialDistribution mean 1.5", "ExpDist(x, 1.5)"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "ParetoRandomVariable........" << std::flush;
        Gnuplot plot;
        plot.SetTitle("ParetoRandomVariable");
        plot.AppendExtra("set xrange [0:4]");

        auto x1 = CreateObject<ParetoRandomVariable>();
        x1->SetAttribute("Scale", DoubleValue(1.0));
        x1->SetAttribute("Shape", DoubleValue(1.5));

        plot.AddDataset(
            Histogram(x1, probes, precision, "ParetoRandomVariable scale=1.0 shape=1.5"));

        auto x2 = CreateObject<ParetoRandomVariable>();
        x2->SetAttribute("Scale", DoubleValue(1.0));
        x2->SetAttribute("Shape", DoubleValue(2.0));

        plot.AddDataset(
            Histogram(x2, probes, precision, "ParetoRandomVariable scale=1.0 shape=2.0"));

        auto x3 = CreateObject<ParetoRandomVariable>();
        x3->SetAttribute("Scale", DoubleValue(1.0));
        x3->SetAttribute("Shape", DoubleValue(2.5));

        plot.AddDataset(
            Histogram(x3, probes, precision, "ParetoRandomVariable scale=1.0 shape=2.5"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "WeibullRandomVariable......." << std::flush;
        Gnuplot plot;
        plot.SetTitle("WeibullRandomVariable");
        plot.AppendExtra("set xrange [0:4]");

        auto x1 = CreateObject<WeibullRandomVariable>();
        x1->SetAttribute("Scale", DoubleValue(1.0));
        x1->SetAttribute("Shape", DoubleValue(1.0));

        plot.AddDataset(
            Histogram(x1, probes, precision, "WeibullRandomVariable scale=1.0 shape=1.0"));

        auto x2 = CreateObject<WeibullRandomVariable>();
        x2->SetAttribute("Scale", DoubleValue(1.0));
        x2->SetAttribute("Shape", DoubleValue(2.0));

        plot.AddDataset(
            Histogram(x2, probes, precision, "WeibullRandomVariable scale=1.0 shape=2.0"));

        auto x3 = CreateObject<WeibullRandomVariable>();
        x3->SetAttribute("Scale", DoubleValue(1.0));
        x3->SetAttribute("Shape", DoubleValue(3.0));

        plot.AddDataset(
            Histogram(x3, probes, precision, "WeibullRandomVariable scale=1.0 shape=3.0"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "NormalRandomVariable........" << std::flush;
        Gnuplot plot;
        plot.SetTitle("NormalRandomVariable");
        plot.AppendExtra("set xrange [-4:4]");
        plot.AppendExtra(
            "NormalDist(x,m,s) = 1 / (s * sqrt(2*pi)) * exp(-1.0 / 2.0 * ((x-m) / s)**2)");

        auto x1 = CreateObject<NormalRandomVariable>();
        x1->SetAttribute("Mean", DoubleValue(0.0));
        x1->SetAttribute("Variance", DoubleValue(1.0));

        plot.AddDataset(Histogram(x1, probes, precision, "NormalRandomVariable m=0.0 v=1.0"));

        plot.AddDataset(Gnuplot2dFunction("NormalDist {/Symbol m}=0.0 {/Symbol s}=1.0",
                                          "NormalDist(x,0.0,1.0)"));

        auto x2 = CreateObject<NormalRandomVariable>();
        x2->SetAttribute("Mean", DoubleValue(0.0));
        x2->SetAttribute("Variance", DoubleValue(2.0));

        plot.AddDataset(Histogram(x2, probes, precision, "NormalRandomVariable m=0.0 v=2.0"));

        plot.AddDataset(Gnuplot2dFunction("NormalDist {/Symbol m}=0.0 {/Symbol s}=sqrt(2.0)",
                                          "NormalDist(x,0.0,sqrt(2.0))"));

        auto x3 = CreateObject<NormalRandomVariable>();
        x3->SetAttribute("Mean", DoubleValue(0.0));
        x3->SetAttribute("Variance", DoubleValue(3.0));

        plot.AddDataset(Histogram(x3, probes, precision, "NormalRandomVariable m=0.0 v=3.0"));

        plot.AddDataset(Gnuplot2dFunction("NormalDist {/Symbol m}=0.0 {/Symbol s}=sqrt(3.0)",
                                          "NormalDist(x,0.0,sqrt(3.0))"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "EmpiricalVariable..........." << std::flush;
        Gnuplot plot;
        plot.SetTitle("EmpiricalRandomVariable");
        plot.AppendExtra("set xrange [*:*]");

        auto x = CreateObject<EmpiricalRandomVariable>();
        x->CDF(0.0, 0.0 / 15.0);
        x->CDF(0.2, 1.0 / 15.0);
        x->CDF(0.4, 3.0 / 15.0);
        x->CDF(0.6, 6.0 / 15.0);
        x->CDF(0.8, 10.0 / 15.0);
        x->CDF(1.0, 15.0 / 15.0);

        plot.AddDataset(
            Histogram(x, probes, precision, "EmpiricalRandomVariable (Sampling)", true));

        x->SetInterpolate(true);
        plot.AppendExtra("set y2range [0:*]");
        auto d2 = Histogram(x, probes, precision, "EmpiricalRandomVariable (Interpolate)");
        d2.SetExtra(" axis x1y2");
        plot.AddDataset(d2);

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "DeterministicVariable......." << std::flush;
        Gnuplot plot;
        plot.SetTitle("DeterministicRandomVariable");
        plot.AppendExtra("set xrange [*:*]");

        auto x1 = CreateObject<DeterministicRandomVariable>();
        double values[] = {0.0, 0.2, 0.2, 0.4, 0.2, 0.6, 0.8, 0.8, 1.0};
        x1->SetValueArray(values, sizeof(values) / sizeof(values[0]));

        plot.AddDataset(Histogram(x1, probes, precision, "DeterministicRandomVariable", true));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "LogNormalRandomVariable....." << std::flush;
        Gnuplot plot;
        plot.SetTitle("LogNormalRandomVariable");
        plot.AppendExtra("set xrange [0:4]");

        plot.AppendExtra("LogNormalDist(x,m,s) = 1.0/x * NormalDist(log(x), m, s)");

        auto x1 = CreateObject<LogNormalRandomVariable>();
        x1->SetAttribute("Mu", DoubleValue(0.0));
        x1->SetAttribute("Sigma", DoubleValue(1.0));

        plot.AddDataset(Histogram(x1, probes, precision, "LogNormalRandomVariable m=0.0 s=1.0"));

        plot.AddDataset(
            Gnuplot2dFunction("LogNormalDist(x, 0.0, 1.0)", "LogNormalDist(x, 0.0, 1.0)"));

        auto x2 = CreateObject<LogNormalRandomVariable>();
        x2->SetAttribute("Mu", DoubleValue(0.0));
        x2->SetAttribute("Sigma", DoubleValue(0.5));

        plot.AddDataset(Histogram(x2, probes, precision, "LogNormalRandomVariable m=0.0 s=0.5"));

        auto x3 = CreateObject<LogNormalRandomVariable>();
        x3->SetAttribute("Mu", DoubleValue(0.0));
        x3->SetAttribute("Sigma", DoubleValue(0.25));

        plot.AddDataset(Histogram(x3, probes, precision, "LogNormalRandomVariable m=0.0 s=0.25"));

        plot.AddDataset(
            Gnuplot2dFunction("LogNormalDist(x, 0.0, 0.25)", "LogNormalDist(x, 0.0, 0.25)"));

        auto x4 = CreateObject<LogNormalRandomVariable>();
        x4->SetAttribute("Mu", DoubleValue(0.0));
        x4->SetAttribute("Sigma", DoubleValue(0.125));

        plot.AddDataset(Histogram(x4, probes, precision, "LogNormalRandomVariable m=0.0 s=0.125"));

        auto x5 = CreateObject<LogNormalRandomVariable>();
        x5->SetAttribute("Mu", DoubleValue(0.0));
        x5->SetAttribute("Sigma", DoubleValue(2.0));

        plot.AddDataset(Histogram(x5, probes, precision, "LogNormalRandomVariable m=0.0 s=2.0"));

        plot.AddDataset(
            Gnuplot2dFunction("LogNormalDist(x, 0.0, 2.0)", "LogNormalDist(x, 0.0, 2.0)"));

        auto x6 = CreateObject<LogNormalRandomVariable>();
        x6->SetAttribute("Mu", DoubleValue(0.0));
        x6->SetAttribute("Sigma", DoubleValue(2.5));

        plot.AddDataset(Histogram(x6, probes, precision, "LogNormalRandomVariable m=0.0 s=2.5"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "TriangularRandomVariable...." << std::flush;
        Gnuplot plot;
        plot.SetTitle("TriangularRandomVariable");
        plot.AppendExtra("set xrange [*:*]");

        auto x1 = CreateObject<TriangularRandomVariable>();
        x1->SetAttribute("Min", DoubleValue(0.0));
        x1->SetAttribute("Max", DoubleValue(1.0));
        x1->SetAttribute("Mean", DoubleValue(0.5));

        plot.AddDataset(
            Histogram(x1, probes, precision, "TriangularRandomVariable [0.0 .. 1.0) m=0.5"));

        auto x2 = CreateObject<TriangularRandomVariable>();
        x2->SetAttribute("Min", DoubleValue(0.0));
        x2->SetAttribute("Max", DoubleValue(1.0));
        x2->SetAttribute("Mean", DoubleValue(0.4));

        plot.AddDataset(
            Histogram(x2, probes, precision, "TriangularRandomVariable [0.0 .. 1.0) m=0.4"));

        auto x3 = CreateObject<TriangularRandomVariable>();
        x3->SetAttribute("Min", DoubleValue(0.0));
        x3->SetAttribute("Max", DoubleValue(1.0));
        x3->SetAttribute("Mean", DoubleValue(0.65));

        plot.AddDataset(
            Histogram(x3, probes, precision, "TriangularRandomVariable [0.0 .. 1.0) m=0.65"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "GammaRandomVariable........." << std::flush;
        Gnuplot plot;
        plot.SetTitle("GammaRandomVariable");
        plot.AppendExtra("set xrange [0:10]");
        plot.AppendExtra("set yrange [0:1]");
        plot.AppendExtra("GammaDist(x,a,b) = x**(a-1) * 1/b**a * exp(-x/b) / gamma(a)");

        plot.AppendExtra(
            "set label 1 '{/Symbol g}(x,{/Symbol a},{/Symbol b}) = x^{/Symbol a-1} e^{-x {/Symbol "
            "b}^{-1}} ( {/Symbol b}^{/Symbol a} {/Symbol G}({/Symbol a}) )^{-1}' at 0.7, 0.9");

        auto x1 = CreateObject<GammaRandomVariable>();
        x1->SetAttribute("Alpha", DoubleValue(1.0));
        x1->SetAttribute("Beta", DoubleValue(1.0));

        plot.AddDataset(Histogram(x1, probes, precision, "GammaRandomVariable a=1.0 b=1.0"));

        plot.AddDataset(Gnuplot2dFunction("{/Symbol g}(x, 1.0, 1.0)", "GammaDist(x, 1.0, 1.0)"));

        auto x2 = CreateObject<GammaRandomVariable>();
        x2->SetAttribute("Alpha", DoubleValue(1.5));
        x2->SetAttribute("Beta", DoubleValue(1.0));

        plot.AddDataset(Histogram(x2, probes, precision, "GammaRandomVariable a=1.5 b=1.0"));

        plot.AddDataset(Gnuplot2dFunction("{/Symbol g}(x, 1.5, 1.0)", "GammaDist(x, 1.5, 1.0)"));

        auto x3 = CreateObject<GammaRandomVariable>();
        x3->SetAttribute("Alpha", DoubleValue(2.0));
        x3->SetAttribute("Beta", DoubleValue(1.0));

        plot.AddDataset(Histogram(x3, probes, precision, "GammaRandomVariable a=2.0 b=1.0"));

        plot.AddDataset(Gnuplot2dFunction("{/Symbol g}(x, 2.0, 1.0)", "GammaDist(x, 2.0, 1.0)"));

        auto x4 = CreateObject<GammaRandomVariable>();
        x4->SetAttribute("Alpha", DoubleValue(4.0));
        x4->SetAttribute("Beta", DoubleValue(1.0));

        plot.AddDataset(Histogram(x4, probes, precision, "GammaRandomVariable a=4.0 b=1.0"));

        plot.AddDataset(Gnuplot2dFunction("{/Symbol g}(x, 4.0, 1.0)", "GammaDist(x, 4.0, 1.0)"));

        auto x5 = CreateObject<GammaRandomVariable>();
        x5->SetAttribute("Alpha", DoubleValue(2.0));
        x5->SetAttribute("Beta", DoubleValue(2.0));

        plot.AddDataset(Histogram(x5, probes, precision, "GammaRandomVariable a=2.0 b=2.0"));

        plot.AddDataset(Gnuplot2dFunction("{/Symbol g}(x, 2.0, 2.0)", "GammaDist(x, 2.0, 2.0)"));

        auto x6 = CreateObject<GammaRandomVariable>();
        x6->SetAttribute("Alpha", DoubleValue(2.5));
        x6->SetAttribute("Beta", DoubleValue(3.0));

        plot.AddDataset(Histogram(x6, probes, precision, "GammaRandomVariable a=2.5 b=3.0"));

        plot.AddDataset(Gnuplot2dFunction("{/Symbol g}(x, 2.5, 3.0)", "GammaDist(x, 2.5, 3.0)"));

        auto x7 = CreateObject<GammaRandomVariable>();
        x7->SetAttribute("Alpha", DoubleValue(2.5));
        x7->SetAttribute("Beta", DoubleValue(4.5));

        plot.AddDataset(Histogram(x7, probes, precision, "GammaRandomVariable a=2.5 b=4.5"));

        plot.AddDataset(Gnuplot2dFunction("{/Symbol g}(x, 2.5, 4.5)", "GammaDist(x, 2.5, 4.5)"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "ErlangRandomVariable........" << std::flush;
        Gnuplot plot;
        plot.SetTitle("ErlangRandomVariable");
        plot.AppendExtra("set xrange [0:10]");
        plot.AppendExtra("ErlangDist(x,k,l) = x**(k-1) * 1/l**k * exp(-x/l) / (k-1)!");

        plot.AppendExtra("set label 1 'Erlang(x,k,{/Symbol l}) = x^{k-1} e^{-x {/Symbol l}^{-1}} ( "
                         "{/Symbol l}^k (k-1)! )^{-1}' at 0.7, 0.9");

        auto x1 = CreateObject<ErlangRandomVariable>();
        x1->SetAttribute("K", IntegerValue(1));
        x1->SetAttribute("Lambda", DoubleValue(1.0));

        plot.AddDataset(
            Histogram(x1, probes, precision, "ErlangRandomVariable k=1 {/Symbol l}=1.0"));

        plot.AddDataset(Gnuplot2dFunction("Erlang(x, 1, 1.0)", "ErlangDist(x, 1, 1.0)"));

        auto x2 = CreateObject<ErlangRandomVariable>();
        x2->SetAttribute("K", IntegerValue(2));
        x2->SetAttribute("Lambda", DoubleValue(1.0));

        plot.AddDataset(
            Histogram(x2, probes, precision, "ErlangRandomVariable k=2 {/Symbol l}=1.0"));

        plot.AddDataset(Gnuplot2dFunction("Erlang(x, 2, 1.0)", "ErlangDist(x, 2, 1.0)"));

        auto x3 = CreateObject<ErlangRandomVariable>();
        x3->SetAttribute("K", IntegerValue(3));
        x3->SetAttribute("Lambda", DoubleValue(1.0));

        plot.AddDataset(
            Histogram(x3, probes, precision, "ErlangRandomVariable k=3 {/Symbol l}=1.0"));

        plot.AddDataset(Gnuplot2dFunction("Erlang(x, 3, 1.0)", "ErlangDist(x, 3, 1.0)"));

        auto x4 = CreateObject<ErlangRandomVariable>();
        x4->SetAttribute("K", IntegerValue(5));
        x4->SetAttribute("Lambda", DoubleValue(1.0));

        plot.AddDataset(
            Histogram(x4, probes, precision, "ErlangRandomVariable k=5 {/Symbol l}=1.0"));

        plot.AddDataset(Gnuplot2dFunction("Erlang(x, 5, 1.0)", "ErlangDist(x, 5, 1.0)"));

        auto x5 = CreateObject<ErlangRandomVariable>();
        x5->SetAttribute("K", IntegerValue(2));
        x5->SetAttribute("Lambda", DoubleValue(2.0));

        plot.AddDataset(
            Histogram(x5, probes, precision, "ErlangRandomVariable k=2 {/Symbol l}=2.0"));

        plot.AddDataset(Gnuplot2dFunction("Erlang(x, 2, 2.0)", "ErlangDist(x, 2, 2.0)"));

        auto x6 = CreateObject<ErlangRandomVariable>();
        x6->SetAttribute("K", IntegerValue(2));
        x6->SetAttribute("Lambda", DoubleValue(3.0));

        plot.AddDataset(
            Histogram(x6, probes, precision, "ErlangRandomVariable k=2 {/Symbol l}=3.0"));

        plot.AddDataset(Gnuplot2dFunction("Erlang(x, 2, 3.0)", "ErlangDist(x, 2, 3.0)"));

        auto x7 = CreateObject<ErlangRandomVariable>();
        x7->SetAttribute("K", IntegerValue(2));
        x7->SetAttribute("Lambda", DoubleValue(5.0));

        plot.AddDataset(
            Histogram(x7, probes, precision, "ErlangRandomVariable k=2 {/Symbol l}=5.0"));

        plot.AddDataset(Gnuplot2dFunction("Erlang(x, 2, 5.0)", "ErlangDist(x, 2, 5.0)"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "BinomialRandomVariable......." << std::flush;
        Gnuplot plot;
        plot.SetTitle("BinomialRandomVariable");
        plot.AppendExtra("set yrange [0:10]");

        auto x = CreateObject<BinomialRandomVariable>();
        x->SetAttribute("Trials", IntegerValue(10));
        x->SetAttribute("Probability", DoubleValue(0.5));

        plot.AddDataset(Histogram(x, probes, precision, "BinomialRandomVariable n=10 p=0.5"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::cout << "BernoulliRandomVariable......." << std::flush;
        Gnuplot plot;
        plot.SetTitle("BernoulliRandomVariable");
        plot.AppendExtra("set yrange [0:1]");

        auto x = CreateObject<BernoulliRandomVariable>();
        x->SetAttribute("Probability", DoubleValue(0.5));

        plot.AddDataset(Histogram(x, probes, precision, "BernoulliRandomVariable p=0.5"));

        gnuplots.AddPlot(plot);
        std::cout << "done" << std::endl;
    }

    {
        std::string gnuFile = plotFile + ".plt";
        std::cout << "Writing Gnuplot file: " << gnuFile << "..." << std::flush;
        std::ofstream gnuStream(gnuFile);
        gnuplots.GenerateOutput(gnuStream);
        gnuStream.close();

        std::cout << "done\nGenerating " << plotFile << ".pdf..." << std::flush;
        std::string shellcmd = "gnuplot " + gnuFile;
        int returnValue = std::system(shellcmd.c_str());
        if (returnValue)
        {
            std::cout << std::endl
                      << "Error: shell command failed with " << returnValue << "; no pdf generated."
                      << std::endl;
        }
        else
        {
            std::cout << "done" << std::endl;
        }
    }

    return 0;
}
