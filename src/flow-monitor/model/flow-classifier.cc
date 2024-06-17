//
// Copyright (c) 2009 INESC Porto
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt> <gjcarneiro@gmail.com>
//

#include "flow-classifier.h"

namespace ns3
{

FlowClassifier::FlowClassifier()
    : m_lastNewFlowId(0)
{
}

FlowClassifier::~FlowClassifier()
{
}

FlowId
FlowClassifier::GetNewFlowId()
{
    return ++m_lastNewFlowId;
}

} // namespace ns3
