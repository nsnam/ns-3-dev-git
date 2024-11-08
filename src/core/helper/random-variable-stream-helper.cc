/*
 * Copyright (c) 2012 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */
#include "random-variable-stream-helper.h"

#include "ns3/assert.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"

/**
 * @file
 * @ingroup core-helpers
 * @ingroup randomvariable
 * ns3::RandomVariableStreamHelper implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RandomVariableStreamHelper");

int64_t
RandomVariableStreamHelper::AssignStreams(std::string path, int64_t stream)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_ASSERT(stream >= 0);
    Config::MatchContainer mc = Config::LookupMatches(path);

    std::size_t i = 0;
    for (; i < mc.GetN(); ++i)
    {
        PointerValue ptr = mc.Get(i);
        Ptr<RandomVariableStream> rvs = ptr.Get<RandomVariableStream>();
        NS_LOG_DEBUG("RandomVariableStream found: " << rvs << "; setting stream to "
                                                    << (stream + i));
        rvs->SetStream(stream + i);
    }
    return i;
}

} // namespace ns3
