/*
 * Copyright (c) 2019 Universita' di Napoli
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "block-ack-type.h"

#include "ns3/fatal-error.h"

namespace ns3
{

BlockAckType::BlockAckType(Variant v)
    : m_variant(v)
{
    switch (m_variant)
    {
    case BASIC:
        m_bitmapLen.push_back(128);
        break;
    case COMPRESSED:
    case EXTENDED_COMPRESSED:
    case GCR:
        m_bitmapLen.push_back(8);
        break;
    case MULTI_TID:
    case MULTI_STA:
        // m_bitmapLen is left empty.
        break;
    default:
        NS_FATAL_ERROR("Unknown block ack type");
    }
}

BlockAckType::BlockAckType()
    : BlockAckType(BASIC)
{
}

BlockAckType::BlockAckType(Variant v, std::vector<uint8_t> l)
    : m_variant(v),
      m_bitmapLen(l)
{
}

BlockAckReqType::BlockAckReqType(Variant v)
    : m_variant(v)
{
    switch (m_variant)
    {
    case BASIC:
    case COMPRESSED:
    case EXTENDED_COMPRESSED:
    case GCR:
        m_nSeqControls = 1;
        break;
    case MULTI_TID:
        m_nSeqControls = 0;
        break;
    default:
        NS_FATAL_ERROR("Unknown block ack request type");
    }
}

BlockAckReqType::BlockAckReqType()
    : BlockAckReqType(BASIC)
{
}

BlockAckReqType::BlockAckReqType(Variant v, uint8_t nSeqControls)
    : m_variant(v),
      m_nSeqControls(nSeqControls)
{
}

std::ostream&
operator<<(std::ostream& os, const BlockAckType& type)
{
    switch (type.m_variant)
    {
    case BlockAckType::BASIC:
        os << "basic-block-ack";
        break;
    case BlockAckType::COMPRESSED:
        os << "compressed-block-ack";
        break;
    case BlockAckType::EXTENDED_COMPRESSED:
        os << "extended-compressed-block-ack";
        break;
    case BlockAckType::MULTI_TID:
        os << "multi-tid-block-ack[" << type.m_bitmapLen.size() << "]";
        break;
    case BlockAckType::GCR:
        os << "gcr-block-ack";
        break;
    case BlockAckType::MULTI_STA:
        os << "multi-sta-block-ack[" << type.m_bitmapLen.size() << "]";
        break;
    default:
        NS_FATAL_ERROR("Unknown block ack type");
    }
    return os;
}

std::ostream&
operator<<(std::ostream& os, const BlockAckReqType& type)
{
    switch (type.m_variant)
    {
    case BlockAckReqType::BASIC:
        os << "basic-block-ack-req";
        break;
    case BlockAckReqType::COMPRESSED:
        os << "compressed-block-ack-req";
        break;
    case BlockAckReqType::EXTENDED_COMPRESSED:
        os << "extended-compressed-block-ack-req";
        break;
    case BlockAckReqType::MULTI_TID:
        os << "multi-tid-block-ack-req[" << type.m_nSeqControls << "]";
        break;
    case BlockAckReqType::GCR:
        os << "gcr-block-ack-req";
        break;
    default:
        NS_FATAL_ERROR("Unknown block ack request type");
    }
    return os;
}

} // namespace ns3
