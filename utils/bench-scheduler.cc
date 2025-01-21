/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/core-module.h"

#include <cmath> // sqrt
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <vector>

using namespace ns3;

/** Flag to write debugging output. */
bool g_debug = false;

/** Name of this program. */
std::string g_me;
/** Log to std::cout */
#define LOG(x) std::cout << x << std::endl
/** Log with program name prefix. */
#define LOGME(x) LOG(g_me << x)
/** Log debugging output. */
#define DEB(x)                                                                                     \
    if (g_debug)                                                                                   \
    {                                                                                              \
        LOGME(x);                                                                                  \
    }

/** Output field width for numeric data. */
int g_fwidth = 6;

/**
 *  Benchmark instance which can do a single run.
 *
 *  The run is controlled by the event population size and
 *  total number of events, which are set at construction.
 *
 *  The event distribution in time is set by SetRandomStream()
 */
class Bench
{
  public:
    /**
     * Constructor
     * @param [in] population The number of events to keep in the scheduler.
     * @param [in] total The total number of events to execute.
     */
    Bench(const uint64_t population, const uint64_t total)
        : m_population(population),
          m_total(total),
          m_count(0)
    {
    }

    /**
     * Set the event delay interval random stream.
     *
     * @param [in] stream The random variable stream to be used to generate
     *              delays for future events.
     */
    void SetRandomStream(Ptr<RandomVariableStream> stream)
    {
        m_rand = stream;
    }

    /**
     * Set the number of events to populate the scheduler with.
     * Each event executed schedules a new event, maintaining the population.
     * @param [in] population The number of events to keep in the scheduler.
     */
    void SetPopulation(const uint64_t population)
    {
        m_population = population;
    }

    /**
     * Set the total number of events to execute.
     * @param [in] total The total number of events to execute.
     */
    void SetTotal(const uint64_t total)
    {
        m_total = total;
    }

    /** The output. */
    struct Result
    {
        double init;     /**< Time (s) for initialization. */
        double simu;     /**< Time (s) for simulation. */
        uint64_t pop;    /**< Event population. */
        uint64_t events; /**< Number of events executed. */
    };

    /**
     *  Run the benchmark as configured.
     *
     * @returns The Result.
     */
    Result Run();

  private:
    /**
     *  Event function. This checks for completion (total number of events
     *  executed) and schedules a new event if not complete.
     */
    void Cb();

    Ptr<RandomVariableStream> m_rand; /**< Stream for event delays. */
    uint64_t m_population;            /**< Event population size. */
    uint64_t m_total;                 /**< Total number of events to execute. */
    uint64_t m_count;                 /**< Count of events executed so far. */
};

Bench::Result
Bench::Run()
{
    SystemWallClockMs timer;
    double init;
    double simu;

    DEB("initializing");
    m_count = 0;

    timer.Start();
    for (uint64_t i = 0; i < m_population; ++i)
    {
        Time at = NanoSeconds(m_rand->GetValue());
        Simulator::Schedule(at, &Bench::Cb, this);
    }
    init = timer.End() / 1000.0;
    DEB("initialization took " << init << "s");

    DEB("running");
    timer.Start();
    Simulator::Run();
    simu = timer.End() / 1000.0;
    DEB("run took " << simu << "s");

    Simulator::Destroy();

    return Result{init, simu, m_population, m_count};
}

void
Bench::Cb()
{
    if (m_count >= m_total)
    {
        Simulator::Stop();
        return;
    }
    DEB("event at " << Simulator::Now().GetSeconds() << "s");

    Time after = NanoSeconds(m_rand->GetValue());
    Simulator::Schedule(after, &Bench::Cb, this);
    ++m_count;
}

/** Benchmark which performs an ensemble of runs. */
class BenchSuite
{
  public:
    /**
     * Perform the runs for a single scheduler type.
     *
     * This will create and set the scheduler, then execute a priming run
     * followed by the number of data runs requested.
     *
     * Output will be in the form of a table showing performance for each run.
     *
     * @param [in] factory Factory pre-configured to create the desired Scheduler.
     * @param [in] pop The event population size.
     * @param [in] total The total number of events to execute.
     * @param [in] runs The number of replications.
     * @param [in] eventStream The random stream of event delays.
     * @param [in] calRev For the CalendarScheduler, whether the Reverse attribute was set.
     */
    BenchSuite(ObjectFactory& factory,
               uint64_t pop,
               uint64_t total,
               uint64_t runs,
               Ptr<RandomVariableStream> eventStream,
               bool calRev);

    /** Write the results to \c LOG() */
    void Log() const;

  private:
    /** Print the table header. */
    void Header() const;

    /** Statistics from a single phase, init or run. */
    struct PhaseResult
    {
        double time;   /**< Phase run time time (s). */
        double rate;   /**< Phase event rate (events/s). */
        double period; /**< Phase period (s/event). */
    };

    /** Results from initialization and execution of a single run. */
    struct Result
    {
        PhaseResult init; /**< Initialization phase results. */
        PhaseResult run;  /**< Run (simulation) phase results. */
        /**
         * Construct from the individual run result.
         *
         * @param [in] r The result from a single run.
         * @returns The run result.
         */
        static Result Bench(Bench::Result r);

        /**
         * Log this result.
         *
         * @tparam T The type of the label.
         * @param label The label for the line.
         */
        template <typename T>
        void Log(T label) const;
    }; // struct Result

    std::string m_scheduler;       /**< Descriptive string for the scheduler. */
    std::vector<Result> m_results; /**< Store for the run results. */

}; // BenchSuite

/* static */
BenchSuite::Result
BenchSuite::Result::Bench(Bench::Result r)
{
    return Result{{r.init, r.pop / r.init, r.init / r.pop},
                  {r.simu, r.events / r.simu, r.simu / r.events}};
}

template <typename T>
void
BenchSuite::Result::Log(T label) const
{
    // Need std::left for string labels

    LOG(std::left << std::setw(g_fwidth) << label << std::setw(g_fwidth) << init.time
                  << std::setw(g_fwidth) << init.rate << std::setw(g_fwidth) << init.period
                  << std::setw(g_fwidth) << run.time << std::setw(g_fwidth) << run.rate
                  << std::setw(g_fwidth) << run.period);
}

BenchSuite::BenchSuite(ObjectFactory& factory,
                       uint64_t pop,
                       uint64_t total,
                       uint64_t runs,
                       Ptr<RandomVariableStream> eventStream,
                       bool calRev)
{
    Simulator::SetScheduler(factory);

    m_scheduler = factory.GetTypeId().GetName();
    if (m_scheduler == "ns3::CalendarScheduler")
    {
        m_scheduler += ": insertion order: " + std::string(calRev ? "reverse" : "normal");
    }
    if (m_scheduler == "ns3::MapScheduler")
    {
        m_scheduler += " (default)";
    }

    Bench bench(pop, total);
    bench.SetRandomStream(eventStream);
    bench.SetPopulation(pop);
    bench.SetTotal(total);

    m_results.reserve(runs);
    Header();

    // Prime
    DEB("priming");
    auto prime = bench.Run();
    Result::Bench(prime).Log("prime");

    // Perform the actual runs
    for (uint64_t i = 0; i < runs; i++)
    {
        auto run = bench.Run();
        m_results.push_back(Result::Bench(run));
        m_results.back().Log(i);
    }

    Simulator::Destroy();
}

void
BenchSuite::Header() const
{
    // table header
    LOG("");
    LOG(m_scheduler);
    LOG(std::left << std::setw(g_fwidth) << "Run #" << std::left << std::setw(3 * g_fwidth)
                  << "Initialization:" << std::left << "Simulation:");
    LOG(std::left << std::setw(g_fwidth) << "" << std::left << std::setw(g_fwidth) << "Time (s)"
                  << std::left << std::setw(g_fwidth) << "Rate (ev/s)" << std::left
                  << std::setw(g_fwidth) << "Per (s/ev)" << std::left << std::setw(g_fwidth)
                  << "Time (s)" << std::left << std::setw(g_fwidth) << "Rate (ev/s)" << std::left
                  << "Per (s/ev)");
    LOG(std::setfill('-') << std::right << std::setw(g_fwidth) << " " << std::right
                          << std::setw(g_fwidth) << " " << std::right << std::setw(g_fwidth) << " "
                          << std::right << std::setw(g_fwidth) << " " << std::right
                          << std::setw(g_fwidth) << " " << std::right << std::setw(g_fwidth) << " "
                          << std::right << std::setw(g_fwidth) << " " << std::setfill(' '));
}

void
BenchSuite::Log() const
{
    if (m_results.size() < 2)
    {
        LOG("");
        return;
    }

    // Average the results

    // See Welford's online algorithm for these expressions,
    // which avoid subtracting large numbers.
    // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford's_online_algorithm

    uint64_t n{0};                // number of samples
    Result average{m_results[0]}; // average
    Result moment2{{0, 0, 0},     // 2nd moment, to calculate stdev
                   {0, 0, 0}};

    for (; n < m_results.size(); ++n)
    {
        double deltaPre;
        double deltaPost;
        const auto& run = m_results[n];
        uint64_t count = n + 1;

#define ACCUMULATE(phase, field)                                                                   \
    deltaPre = run.phase.field - average.phase.field;                                              \
    average.phase.field += deltaPre / count;                                                       \
    deltaPost = run.phase.field - average.phase.field;                                             \
    moment2.phase.field += deltaPre * deltaPost

        ACCUMULATE(init, time);
        ACCUMULATE(init, rate);
        ACCUMULATE(init, period);
        ACCUMULATE(run, time);
        ACCUMULATE(run, rate);
        ACCUMULATE(run, period);

#undef ACCUMULATE
    }

    auto stdev = Result{
        {std::sqrt(moment2.init.time / n),
         std::sqrt(moment2.init.rate / n),
         std::sqrt(moment2.init.period / n)},
        {std::sqrt(moment2.run.time / n),
         std::sqrt(moment2.run.rate / n),
         std::sqrt(moment2.run.period / n)},
    };

    average.Log("average");
    stdev.Log("stdev");

    LOG("");
}

/**
 *  Create a RandomVariableStream to generate next event delays.
 *
 *  If the \p filename parameter is empty a default exponential time
 *  distribution will be used, with mean delay of 100 ns.
 *
 *  If the \p filename is `-` standard input will be used.
 *
 *  @param [in] filename The delay interval source file name.
 *  @returns The RandomVariableStream.
 */
Ptr<RandomVariableStream>
GetRandomStream(std::string filename)
{
    Ptr<RandomVariableStream> stream = nullptr;

    if (filename.empty())
    {
        LOG("  Event time distribution:      default exponential");
        auto erv = CreateObject<ExponentialRandomVariable>();
        erv->SetAttribute("Mean", DoubleValue(100));
        stream = erv;
    }
    else
    {
        std::istream* input;

        if (filename == "-")
        {
            LOG("  Event time distribution:      from stdin");
            input = &std::cin;
        }
        else
        {
            LOG("  Event time distribution:      from " << filename);
            input = new std::ifstream(filename);
        }

        double value;
        std::vector<double> nsValues;

        while (!input->eof())
        {
            if (*input >> value)
            {
                auto ns = (uint64_t)(value * 1000000000);
                nsValues.push_back(ns);
            }
            else
            {
                input->clear();
                std::string line;
                *input >> line;
            }
        }
        LOG("    Found " << nsValues.size() << " entries");
        auto drv = CreateObject<DeterministicRandomVariable>();
        drv->SetValueArray(&nsValues[0], nsValues.size());
        stream = drv;
    }

    return stream;
}

int
main(int argc, char* argv[])
{
    bool allSched = false;
    bool schedCal = false;
    bool schedHeap = false;
    bool schedList = false;
    bool schedMap = false; // default scheduler
    bool schedPQ = false;

    uint64_t pop = 100000;
    uint64_t total = 1000000;
    uint64_t runs = 1;
    std::string filename = "";
    bool calRev = false;

    CommandLine cmd(__FILE__);
    cmd.Usage("Benchmark the simulator scheduler.\n"
              "\n"
              "Event intervals are taken from one of:\n"
              "  an exponential distribution, with mean 100 ns,\n"
              "  an ascii file, given by the --file=\"<filename>\" argument,\n"
              "  or standard input, by the argument --file=\"-\"\n"
              "In the case of either --file form, the input is expected\n"
              "to be ascii, giving the relative event times in ns.\n"
              "\n"
              "If no scheduler is specified the MapScheduler will be run.");
    cmd.AddValue("all", "use all schedulers", allSched);
    cmd.AddValue("cal", "use CalendarScheduler", schedCal);
    cmd.AddValue("calrev", "reverse ordering in the CalendarScheduler", calRev);
    cmd.AddValue("heap", "use HeapScheduler", schedHeap);
    cmd.AddValue("list", "use ListScheduler", schedList);
    cmd.AddValue("map", "use MapScheduler (default)", schedMap);
    cmd.AddValue("pri", "use PriorityQueue", schedPQ);
    cmd.AddValue("debug", "enable debugging output", g_debug);
    cmd.AddValue("pop", "event population size", pop);
    cmd.AddValue("total", "total number of events to run", total);
    cmd.AddValue("runs", "number of runs", runs);
    cmd.AddValue("file", "file of relative event times", filename);
    cmd.AddValue("prec", "printed output precision", g_fwidth);
    cmd.Parse(argc, argv);

    g_me = cmd.GetName() + ": ";
    g_fwidth += 6; // 5 extra chars in '2.000002e+07 ': . e+0 _

    LOG(std::setprecision(g_fwidth - 6)); // prints blank line
    LOGME(" Benchmark the simulator scheduler");
    LOG("  Event population size:        " << pop);
    LOG("  Total events per run:         " << total);
    LOG("  Number of runs per scheduler: " << runs);
    DEB("debugging is ON");

    if (allSched)
    {
        schedCal = schedHeap = schedList = schedMap = schedPQ = true;
    }
    // Set the default case if nothing else is set
    if (!(schedCal || schedHeap || schedList || schedMap || schedPQ))
    {
        schedMap = true;
    }

    auto eventStream = GetRandomStream(filename);

    ObjectFactory factory("ns3::MapScheduler");
    if (schedCal)
    {
        factory.SetTypeId("ns3::CalendarScheduler");
        factory.Set("Reverse", BooleanValue(calRev));
        BenchSuite(factory, pop, total, runs, eventStream, calRev).Log();
        if (allSched)
        {
            factory.Set("Reverse", BooleanValue(!calRev));
            BenchSuite(factory, pop, total, runs, eventStream, !calRev).Log();
        }
    }
    if (schedHeap)
    {
        factory.SetTypeId("ns3::HeapScheduler");
        BenchSuite(factory, pop, total, runs, eventStream, calRev).Log();
    }
    if (schedList)
    {
        factory.SetTypeId("ns3::ListScheduler");
        auto listTotal = total;
        if (allSched)
        {
            LOG("Running List scheduler with 1/10 total events");
            listTotal /= 10;
        }
        BenchSuite(factory, pop, listTotal, runs, eventStream, calRev).Log();
    }
    if (schedMap)
    {
        factory.SetTypeId("ns3::MapScheduler");
        BenchSuite(factory, pop, total, runs, eventStream, calRev).Log();
    }
    if (schedPQ)
    {
        factory.SetTypeId("ns3::PriorityQueueScheduler");
        BenchSuite(factory, pop, total, runs, eventStream, calRev).Log();
    }

    return 0;
}
