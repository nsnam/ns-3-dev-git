/*
 * Copyright (c) 2015 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "queue-disc-container.h"

namespace ns3
{

QueueDiscContainer::QueueDiscContainer()
{
}

QueueDiscContainer::QueueDiscContainer(Ptr<QueueDisc> qDisc)
{
    m_queueDiscs.push_back(qDisc);
}

QueueDiscContainer::ConstIterator
QueueDiscContainer::Begin() const
{
    return m_queueDiscs.begin();
}

QueueDiscContainer::ConstIterator
QueueDiscContainer::End() const
{
    return m_queueDiscs.end();
}

std::size_t
QueueDiscContainer::GetN() const
{
    return m_queueDiscs.size();
}

Ptr<QueueDisc>
QueueDiscContainer::Get(std::size_t i) const
{
    return m_queueDiscs[i];
}

void
QueueDiscContainer::Add(QueueDiscContainer other)
{
    for (auto i = other.Begin(); i != other.End(); i++)
    {
        m_queueDiscs.push_back(*i);
    }
}

void
QueueDiscContainer::Add(Ptr<QueueDisc> qDisc)
{
    m_queueDiscs.push_back(qDisc);
}

} // namespace ns3
