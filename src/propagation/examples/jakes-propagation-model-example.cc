/*
 * Copyright (c) 2012 Telum (www.telum.ru)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@telum.ru>
 */
#include "ns3/core-module.h"
#include "ns3/jakes-propagation-loss-model.h"
#include "ns3/mobility-module.h"

#include <cmath>
#include <vector>

using namespace ns3;

/**
 * @ingroup propagation
 * @brief Constructs a JakesPropagationlossModel and print the loss value as a function of time into
 * std::cout. Distribution and correlation statistics is compared with a theoretical ones using R
 * package (http://www.r-project.org/). Scripts are presented within comments.
 */
class JakesPropagationExample
{
  public:
    JakesPropagationExample();
    ~JakesPropagationExample();

  private:
    Ptr<PropagationLossModel> m_loss;    //!< loss
    Ptr<MobilityModel> m_firstMobility;  //!< first Mobility
    Ptr<MobilityModel> m_secondMobility; //!< second Mobility
    Time m_step;                         //!< step
    EventId m_nextEvent;                 //!< next event
    /**
     * Next function
     * */
    void Next();
};

JakesPropagationExample::JakesPropagationExample()
    : m_step(Seconds(0.0002)) // 1/5000 part of the second
{
    m_loss = CreateObject<JakesPropagationLossModel>();
    m_firstMobility = CreateObject<ConstantPositionMobilityModel>();
    m_secondMobility = CreateObject<ConstantPositionMobilityModel>();
    m_firstMobility->SetPosition(Vector(0, 0, 0));
    m_secondMobility->SetPosition(Vector(10, 0, 0));
    m_nextEvent = Simulator::Schedule(m_step, &JakesPropagationExample::Next, this);
}

JakesPropagationExample::~JakesPropagationExample()
{
}

void
JakesPropagationExample::Next()
{
    m_nextEvent = Simulator::Schedule(m_step, &JakesPropagationExample::Next, this);
    std::cout << Simulator::Now().As(Time::MS) << " "
              << m_loss->CalcRxPower(0, m_firstMobility, m_secondMobility) << std::endl;
}

int
main(int argc, char* argv[])
{
    Config::SetDefault("ns3::JakesProcess::NumberOfOscillators", UintegerValue(100));
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);
    JakesPropagationExample example;
    Simulator::Stop(Seconds(1000));
    Simulator::Run();
    Simulator::Destroy();
    /*
     * R script for plotting a distribution:
     data<-read.table ("data")
     rayleigh<-(rnorm(1e6)^2+rnorm(1e6)^2)/2
     qqplot(10*log10(rayleigh), data$V2, main="QQ-plot for improved Jakes model", xlab="Reference
     Rayleigh distribution [power, dB]", ylab="Sum-of-sinusoids distribution [power, dB]",
     xlim=c(-45, 10), ylim=c(-45, 10)) lines (c(-50, 50), c(-50, 50)) abline (v=-50:50*2,
     h=-50:50*2, col="light grey")
     */

    /*
     * R script to plot autocorrelation function:
     # Read amplitude distribution:
     data<-10^(read.table ("data")$V2/20)
     x<-1:2000/10
     acf (data, lag.max=200, main="Autocorrelation function of the improved Jakes model", xlab="Time
     x200 microseconds ", ylab="Autocorrelation") # If we have a delta T = 1/5000 part of the second
     and doppler freq = 80 Hz lines (x, besselJ(x*80*2*pi/5000, 0)^2) abline (h=0:10/10, col="light
     grey")
     */
    return 0;
}
