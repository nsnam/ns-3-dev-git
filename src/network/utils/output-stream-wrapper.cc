/*
 * Copyright (c) 2010 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "output-stream-wrapper.h"

#include "ns3/abort.h"
#include "ns3/fatal-impl.h"
#include "ns3/log.h"

#include <fstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OutputStreamWrapper");

OutputStreamWrapper::OutputStreamWrapper(std::string filename, std::ios::openmode filemode)
    : m_destroyable(true)
{
    NS_LOG_FUNCTION(this << filename << filemode);
    auto os = new std::ofstream();
    os->open(filename, filemode);
    m_ostream = os;
    FatalImpl::RegisterStream(m_ostream);
    NS_ABORT_MSG_UNLESS(os->is_open(),
                        "AsciiTraceHelper::CreateFileStream(): Unable to Open "
                            << filename << " for mode " << filemode);
}

OutputStreamWrapper::OutputStreamWrapper(std::ostream* os)
    : m_ostream(os),
      m_destroyable(false)
{
    NS_LOG_FUNCTION(this << os);
    FatalImpl::RegisterStream(m_ostream);
    NS_ABORT_MSG_UNLESS(m_ostream->good(), "Output stream is not valid for writing.");
}

OutputStreamWrapper::~OutputStreamWrapper()
{
    NS_LOG_FUNCTION(this);
    FatalImpl::UnregisterStream(m_ostream);
    if (m_destroyable)
    {
        delete m_ostream;
    }
    m_ostream = nullptr;
}

std::ostream*
OutputStreamWrapper::GetStream()
{
    NS_LOG_FUNCTION(this);
    return m_ostream;
}

} // namespace ns3
