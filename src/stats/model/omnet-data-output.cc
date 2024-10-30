/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#include "omnet-data-output.h"

#include "data-calculator.h"
#include "data-collector.h"

#include "ns3/log.h"
#include "ns3/nstime.h"

#include <cstdlib>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OmnetDataOutput");

//--------------------------------------------------------------
//----------------------------------------------
OmnetDataOutput::OmnetDataOutput()
{
    NS_LOG_FUNCTION(this);

    m_filePrefix = "data";
}

OmnetDataOutput::~OmnetDataOutput()
{
    NS_LOG_FUNCTION(this);
}

/* static */
TypeId
OmnetDataOutput::GetTypeId()
{
    static TypeId tid = TypeId("ns3::OmnetDataOutput")
                            .SetParent<DataOutputInterface>()
                            .SetGroupName("Stats")
                            .AddConstructor<OmnetDataOutput>();
    return tid;
}

void
OmnetDataOutput::DoDispose()
{
    NS_LOG_FUNCTION(this);

    DataOutputInterface::DoDispose();
    // end OmnetDataOutput::DoDispose
}

//----------------------------------------------

inline bool
isNumeric(const std::string& s)
{
    bool decimalPtSeen = false;
    bool exponentSeen = false;
    char last = '\0';

    for (auto it = s.begin(); it != s.end(); it++)
    {
        if ((*it == '.' && decimalPtSeen) || (*it == 'e' && exponentSeen) ||
            (*it == '-' && it != s.begin() && last != 'e'))
        {
            return false;
        }
        else if (*it == '.')
        {
            decimalPtSeen = true;
        }
        else if (*it == 'e')
        {
            exponentSeen = true;
            decimalPtSeen = false;
        }

        last = *it;
    }
    return true;
}

void
OmnetDataOutput::Output(DataCollector& dc)
{
    NS_LOG_FUNCTION(this << &dc);

    std::ofstream scalarFile;
    std::string fn = m_filePrefix + "-" + dc.GetRunLabel() + ".sca";
    scalarFile.open(fn, std::ios_base::out);

    /// @todo add timestamp to the runlevel
    scalarFile << "run " << dc.GetRunLabel() << std::endl;
    scalarFile << "attr experiment \"" << dc.GetExperimentLabel() << "\"" << std::endl;
    scalarFile << "attr strategy \"" << dc.GetStrategyLabel() << "\"" << std::endl;
    scalarFile << "attr measurement \"" << dc.GetInputLabel() << "\"" << std::endl;
    scalarFile << "attr description \"" << dc.GetDescription() << "\"" << std::endl;

    for (auto i = dc.MetadataBegin(); i != dc.MetadataEnd(); i++)
    {
        const auto& blob = (*i);
        scalarFile << "attr \"" << blob.first << "\" \"" << blob.second << "\"" << std::endl;
    }

    scalarFile << std::endl;
    if (isNumeric(dc.GetInputLabel()))
    {
        scalarFile << "scalar . measurement \"" << dc.GetInputLabel() << "\"" << std::endl;
    }
    for (auto i = dc.MetadataBegin(); i != dc.MetadataEnd(); i++)
    {
        const auto& blob = (*i);
        if (isNumeric(blob.second))
        {
            scalarFile << "scalar . \"" << blob.first << "\" \"" << blob.second << "\""
                       << std::endl;
        }
    }
    OmnetOutputCallback callback(&scalarFile);

    for (auto i = dc.DataCalculatorBegin(); i != dc.DataCalculatorEnd(); i++)
    {
        (*i)->Output(callback);
    }

    scalarFile << std::endl << std::endl;
    scalarFile.close();

    // end OmnetDataOutput::Output
}

OmnetDataOutput::OmnetOutputCallback::OmnetOutputCallback(std::ostream* scalar)
    : m_scalar(scalar)
{
    NS_LOG_FUNCTION(this << scalar);
}

void
OmnetDataOutput::OmnetOutputCallback::OutputStatistic(std::string context,
                                                      std::string name,
                                                      const StatisticalSummary* statSum)
{
    NS_LOG_FUNCTION(this << context << name << statSum);

    if (context.empty())
    {
        context = ".";
    }
    if (name.empty())
    {
        name = "\"\"";
    }
    (*m_scalar) << "statistic " << context << " " << name << std::endl;
    if (!std::isnan(statSum->getCount()))
    {
        (*m_scalar) << "field count " << statSum->getCount() << std::endl;
    }
    if (!std::isnan(statSum->getSum()))
    {
        (*m_scalar) << "field sum " << statSum->getSum() << std::endl;
    }
    if (!std::isnan(statSum->getMean()))
    {
        (*m_scalar) << "field mean " << statSum->getMean() << std::endl;
    }
    if (!std::isnan(statSum->getMin()))
    {
        (*m_scalar) << "field min " << statSum->getMin() << std::endl;
    }
    if (!std::isnan(statSum->getMax()))
    {
        (*m_scalar) << "field max " << statSum->getMax() << std::endl;
    }
    if (!std::isnan(statSum->getSqrSum()))
    {
        (*m_scalar) << "field sqrsum " << statSum->getSqrSum() << std::endl;
    }
    if (!std::isnan(statSum->getStddev()))
    {
        (*m_scalar) << "field stddev " << statSum->getStddev() << std::endl;
    }
}

void
OmnetDataOutput::OmnetOutputCallback::OutputSingleton(std::string context,
                                                      std::string name,
                                                      int val)
{
    NS_LOG_FUNCTION(this << context << name << val);

    if (context.empty())
    {
        context = ".";
    }
    if (name.empty())
    {
        name = "\"\"";
    }
    (*m_scalar) << "scalar " << context << " " << name << " " << val << std::endl;
    // end OmnetDataOutput::OmnetOutputCallback::OutputSingleton
}

void
OmnetDataOutput::OmnetOutputCallback::OutputSingleton(std::string context,
                                                      std::string name,
                                                      uint32_t val)
{
    NS_LOG_FUNCTION(this << context << name << val);

    if (context.empty())
    {
        context = ".";
    }
    if (name.empty())
    {
        name = "\"\"";
    }
    (*m_scalar) << "scalar " << context << " " << name << " " << val << std::endl;
    // end OmnetDataOutput::OmnetOutputCallback::OutputSingleton
}

void
OmnetDataOutput::OmnetOutputCallback::OutputSingleton(std::string context,
                                                      std::string name,
                                                      double val)
{
    NS_LOG_FUNCTION(this << context << name << val);

    if (context.empty())
    {
        context = ".";
    }
    if (name.empty())
    {
        name = "\"\"";
    }
    (*m_scalar) << "scalar " << context << " " << name << " " << val << std::endl;
    // end OmnetDataOutput::OmnetOutputCallback::OutputSingleton
}

void
OmnetDataOutput::OmnetOutputCallback::OutputSingleton(std::string context,
                                                      std::string name,
                                                      std::string val)
{
    NS_LOG_FUNCTION(this << context << name << val);

    if (context.empty())
    {
        context = ".";
    }
    if (name.empty())
    {
        name = "\"\"";
    }
    (*m_scalar) << "scalar " << context << " " << name << " " << val << std::endl;
    // end OmnetDataOutput::OmnetOutputCallback::OutputSingleton
}

void
OmnetDataOutput::OmnetOutputCallback::OutputSingleton(std::string context,
                                                      std::string name,
                                                      Time val)
{
    NS_LOG_FUNCTION(this << context << name << val);

    if (context.empty())
    {
        context = ".";
    }
    if (name.empty())
    {
        name = "\"\"";
    }
    (*m_scalar) << "scalar " << context << " " << name << " " << val.GetTimeStep() << std::endl;
    // end OmnetDataOutput::OmnetOutputCallback::OutputSingleton
}
