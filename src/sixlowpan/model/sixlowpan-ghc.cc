/*
 * Copyright (c) 2026 SRM Institute of Science and Technology
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 *
 * 6LoWPAN-GHC: Generic Header Compression - RFC 7400
 *
 * Source: original implementation following RFC 7400 (Bormann, November 2014).
 */

#include "sixlowpan-ghc.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SixLowPanGhc");

// ============================================================================
//  GHC Compression/Decompression Engine
// ============================================================================

void
SixLowPanGhcEngine::InitDictionary(uint8_t* dict,
                                   const Ipv6Address& srcAddr,
                                   const Ipv6Address& dstAddr)
{
    // Bytes 0-15: Source IPv6 address
    srcAddr.GetBytes(dict);

    // Bytes 16-31: Destination IPv6 address
    dstAddr.GetBytes(dict + 16);

    // Bytes 32-47: static dictionary (RFC 7400 Section 3.1)
    static constexpr std::array<uint8_t, 16> staticDict = {0x16,
                                                           0xfe,
                                                           0xfd,
                                                           0x17,
                                                           0xfe,
                                                           0xfd,
                                                           0x00,
                                                           0x01,
                                                           0x00,
                                                           0x00,
                                                           0x00,
                                                           0x00,
                                                           0x00,
                                                           0x01,
                                                           0x00,
                                                           0x00};
    std::copy(staticDict.begin(), staticDict.end(), dict + 32);
}

GhcBytecodeType
SixLowPanGhcEngine::ClassifyBytecode(uint8_t byte)
{
    if ((byte & 0x80) == 0)
    {
        // 0kkkkkkk - Literal copy
        return GhcBytecodeType::LITERAL;
    }
    else if ((byte & 0xF0) == 0x80)
    {
        // 1000nnnn - Zero insertion: emits nnnn+2 zero bytes (RFC 7400 Section 3).
        return GhcBytecodeType::ZERO_INSERT;
    }
    else if (byte == 0x90)
    {
        // 10010000 - Stop code
        return GhcBytecodeType::STOP_CODE;
    }
    else if ((byte & 0xE0) == 0xA0)
    {
        // 101nssss - Extended arguments
        return GhcBytecodeType::EXTENDED_ARGS;
    }
    else if ((byte & 0xC0) == 0xC0)
    {
        // 11nnnkkk - Backreference
        return GhcBytecodeType::BACKREF;
    }
    else
    {
        // 1001nnnn (nnnn > 0) and 011xxxxx - Reserved per RFC 7400 Section 3.
        return GhcBytecodeType::RESERVED;
    }
}

uint32_t
SixLowPanGhcEngine::Decompress(const Ipv6Address& srcAddr,
                               const Ipv6Address& dstAddr,
                               const uint8_t* compressed,
                               uint32_t compressedLen,
                               uint8_t* output,
                               uint32_t outputMaxLen,
                               bool useStopCode)
{
    NS_LOG_FUNCTION(srcAddr << dstAddr << compressedLen << outputMaxLen << useStopCode);

    // Working buffer: dictionary followed by the decompressed output.
    // Sized from the caller's limit so any link MTU is supported.
    const uint32_t bufferSize = DICTIONARY_SIZE + outputMaxLen;
    std::vector<uint8_t> buffer(bufferSize);

    // Initialize dictionary in buffer[0..47]
    InitDictionary(buffer.data(), srcAddr, dstAddr);

    uint32_t outputPos = DICTIONARY_SIZE; // Current write position in buffer
    uint32_t inputPos = 0;                // Current read position in compressed
    uint32_t sa = 0;                      // Start adjust accumulator
    uint32_t na = 0;                      // Number adjust accumulator
    bool stopCodeFound = false;

    while (inputPos < compressedLen && !stopCodeFound)
    {
        uint8_t codeByte = compressed[inputPos++];
        GhcBytecodeType type = ClassifyBytecode(codeByte);

        switch (type)
        {
        case GhcBytecodeType::LITERAL: {
            // 0kkkkkkk: copy k literal bytes from bytecode stream
            uint32_t k = codeByte & 0x7F;

            if (k == 0)
            {
                // No-op
                break;
            }
            if (k > MAX_LITERAL_RUN)
            {
                NS_LOG_WARN("GHC: Literal count k=" << k << " is reserved");
                return 0;
            }
            if (inputPos + k > compressedLen)
            {
                NS_LOG_WARN("GHC: Literal overruns compressed data");
                return 0;
            }
            if (outputPos + k > bufferSize)
            {
                NS_LOG_WARN("GHC: Literal would exceed buffer");
                return 0;
            }

            std::memcpy(buffer.data() + outputPos, compressed + inputPos, k);
            outputPos += k;
            inputPos += k;
            break;
        }

        case GhcBytecodeType::ZERO_INSERT: {
            // 1000nnnn: insert (n+2) zero bytes
            uint32_t n = (codeByte & 0x0F) + 2;

            if (outputPos + n > bufferSize)
            {
                NS_LOG_WARN("GHC: Zero insert would exceed buffer");
                return 0;
            }

            std::memset(buffer.data() + outputPos, 0, n);
            outputPos += n;
            break;
        }

        case GhcBytecodeType::STOP_CODE: {
            // 10010000: terminate decompression (extension headers only)
            if (!useStopCode)
            {
                NS_LOG_WARN("GHC: Unexpected stop code in non-extension context");
                return 0;
            }
            NS_LOG_DEBUG("GHC: Stop code encountered");
            stopCodeFound = true;
            break;
        }

        case GhcBytecodeType::EXTENDED_ARGS: {
            // 101nssss: sa += ssss << 3, na += n << 3
            //   bit layout: 101[n][ssss]
            //   n  = bit 4 (one bit)
            //   ssss = bits 0-3
            uint32_t ssss = codeByte & 0x0F;
            uint32_t nBit = (codeByte >> 4) & 0x01;

            sa += (ssss << 3);
            na += (nBit << 3);

            NS_LOG_DEBUG("GHC: ExtArgs sa=" << sa << " na=" << na);
            break;
        }

        case GhcBytecodeType::BACKREF: {
            // 11nnnkkk: copy n bytes from s bytes back
            //   n = na + (nnn) + 2
            //   s = (kkk) + sa + n
            uint32_t nnn = (codeByte >> 3) & 0x07;
            uint32_t kkk = codeByte & 0x07;

            uint32_t copyLen = na + nnn + 2;
            uint32_t offset = kkk + sa + copyLen;

            // Reset extended arg accumulators
            sa = 0;
            na = 0;

            if (offset > outputPos)
            {
                NS_LOG_WARN("GHC: Backref offset " << offset << " exceeds buffer position "
                                                   << outputPos);
                return 0;
            }
            if (outputPos + copyLen > bufferSize)
            {
                NS_LOG_WARN("GHC: Backref would exceed buffer");
                return 0;
            }

            // Copy byte-by-byte (overlapping copies are valid in LZ77)
            uint32_t srcPos = outputPos - offset;
            for (uint32_t i = 0; i < copyLen; i++)
            {
                buffer[outputPos + i] = buffer[srcPos + i];
            }
            outputPos += copyLen;
            break;
        }

        case GhcBytecodeType::RESERVED:
            NS_LOG_WARN("GHC: Reserved bytecode 0x" << std::hex << +codeByte);
            return 0;
        }
    }

    // Output is everything after the dictionary. The per-bytecode buffer
    // checks above already guarantee decompressedLen <= outputMaxLen.
    uint32_t decompressedLen = outputPos - DICTIONARY_SIZE;

    std::memcpy(output, buffer.data() + DICTIONARY_SIZE, decompressedLen);

    NS_LOG_DEBUG("GHC: Decompressed " << compressedLen << " bytes to " << decompressedLen
                                      << " bytes");

    return decompressedLen;
}

uint32_t
SixLowPanGhcEngine::CountZeros(const uint8_t* input, uint32_t remaining)
{
    uint32_t count = 0;
    while (count < remaining && input[count] == 0)
    {
        count++;
    }
    return count;
}

uint32_t
SixLowPanGhcEngine::CountBackrefExtArgBytes(uint32_t matchLength, uint32_t matchOffset)
{
    // Backref encoding 11nnnkkk: n = na + nnn + 2, s = kkk + sa + n, with
    // nnn and kkk in [0,7]. Work out the extended-args accumulators needed,
    // rounded up to their unit of 8 (see EmitBackref).
    uint32_t nBase = matchLength - 2;
    uint32_t kBase = matchOffset - matchLength;

    uint32_t na = (nBase > 7) ? ((nBase - 7 + 7) / 8) * 8 : 0;
    uint32_t sa = (kBase > 7) ? ((kBase - 7 + 7) / 8) * 8 : 0;

    // Each 101nssss byte carries up to ssss(15)*8 = 120 of sa and 8 of na.
    uint32_t bytes = 0;
    while (sa > 0 || na > 0)
    {
        bytes++;
        sa -= std::min<uint32_t>(sa, 15 * 8);
        na -= std::min<uint32_t>(na, 8);
    }
    return bytes;
}

bool
SixLowPanGhcEngine::FindLongestMatch(const uint8_t* buffer,
                                     uint32_t bufLen,
                                     const uint8_t* input,
                                     uint32_t inputRemaining,
                                     uint32_t& matchOffset,
                                     uint32_t& matchLength)
{
    matchOffset = 0;
    matchLength = 0;

    // Simple exhaustive (brute-force) LZ77 search: for every backward distance
    // we compare against the input and keep the longest match. GHC operates on
    // header-scale data, so the O(window * matchLen) cost is negligible and
    // no hash chains or suffix structures are needed.

    uint32_t maxMatchLen = std::min<uint32_t>(inputRemaining, MAX_BACKREF_LEN);

    // Search the whole buffer: extended-args bytes let a backref address any
    // offset (each one adds up to 120), so the dictionary at the start of the
    // buffer stays reachable even for long payloads. The cost check below
    // rejects matches whose extended-args overhead would exceed the bytes
    // they save.
    for (uint32_t back = 2; back <= bufLen; back++)
    {
        uint32_t searchPos = bufLen - back;
        uint32_t len = 0;

        while (len < maxMatchLen && buffer[searchPos + len] == input[len])
        {
            len++;
        }

        // The encoding requires offset >= length; clamp overlapping matches
        // to what can actually be emitted.
        len = std::min(len, back);

        if (len < 2 || len <= matchLength)
        {
            continue;
        }

        // Emit a backref only when it is no larger than the literal bytes
        // it replaces: 1 opcode byte + extended-args bytes vs len bytes.
        if (1 + CountBackrefExtArgBytes(len, back) > len)
        {
            continue;
        }

        matchLength = len;
        matchOffset = back;

        if (matchLength == maxMatchLen)
        {
            break; // Can't do better
        }
    }

    return (matchLength >= 2);
}

bool
SixLowPanGhcEngine::EmitBackref(uint8_t* output,
                                uint32_t& outPos,
                                uint32_t outputMaxLen,
                                uint32_t matchLength,
                                uint32_t matchOffset)
{
    NS_ASSERT(matchLength >= 2);
    // GHC bytecode 11nnnkkk requires offset >= length;
    // caller (Compress) clamps overlapping matches before calling us.

    // Backref encoding: 11nnnkkk
    //   n = na + nnn + 2  =>  nnn = n - na - 2
    //   s = kkk + sa + n  =>  kkk = s - sa - n  (where s = matchOffset)
    //
    // We need: nnn in [0,7], kkk in [0,7]
    // If they don't fit, we need Extended Args (101nssss) to add to sa/na.

    uint32_t n = matchLength; // Total copy length
    uint32_t s = matchOffset; // Total offset

    // Determine how much sa and na we need
    // nnn = n - na - 2, must be in [0,7]  =>  na = n - 2 - nnn
    // kkk = s - sa - n, must be in [0,7]  =>  sa = s - n - kkk

    // First try without extended args
    uint32_t nBase = n - 2; // Need nnn = nBase - na
    uint32_t kBase = s - n; // Need kkk = kBase - sa

    uint32_t na = 0;
    uint32_t sa = 0;

    // Calculate required na (to make nnn fit in [0,7])
    if (nBase > 7)
    {
        na = nBase - 7; // Minimum na to make nnn <= 7
        // na must be a multiple of 8 (since extended args add in units of 8)
        na = ((na + 7) / 8) * 8;
    }

    // Calculate required sa (to make kkk fit in [0,7])
    if (kBase > 7)
    {
        sa = kBase - 7;
        sa = ((sa + 7) / 8) * 8;
    }

    // Verify encoding is feasible
    uint32_t nnn = nBase - na;
    uint32_t kkk = kBase - sa;

    if (nnn > 7 || kkk > 7)
    {
        NS_LOG_WARN("GHC: Cannot encode backref len=" << n << " offset=" << s);
        return false;
    }

    // Check output space: extended-args bytes + 1 (backref byte)
    uint32_t extArgBytes = CountBackrefExtArgBytes(matchLength, matchOffset);
    if (outPos + extArgBytes + 1 > outputMaxLen)
    {
        return false;
    }

    // Emit extended args
    uint32_t remainingSa = sa;
    uint32_t remainingNa = na;

    while (remainingSa > 0 || remainingNa > 0)
    {
        // Extended arg: 101[n][ssss]
        //   adds ssss*8 to sa, n*8 to na
        uint32_t ssss = std::min<uint32_t>(remainingSa / 8, 15);
        uint32_t nBit = (remainingNa >= 8) ? 1 : 0;

        uint8_t extByte = 0xA0 | (nBit << 4) | (ssss & 0x0F);
        output[outPos++] = extByte;

        remainingSa -= ssss * 8;
        if (nBit)
        {
            remainingNa -= 8;
        }
    }

    // Emit backref: 11nnnkkk
    uint8_t backrefByte = 0xC0 | ((nnn & 0x07) << 3) | (kkk & 0x07);
    output[outPos++] = backrefByte;

    return true;
}

uint32_t
SixLowPanGhcEngine::Compress(const Ipv6Address& srcAddr,
                             const Ipv6Address& dstAddr,
                             const uint8_t* input,
                             uint32_t inputLen,
                             uint8_t* output,
                             uint32_t outputMaxLen,
                             bool emitStopCode)
{
    NS_LOG_FUNCTION(srcAddr << dstAddr << inputLen << outputMaxLen << emitStopCode);

    if (inputLen == 0)
    {
        return 0;
    }

    // Working buffer: dictionary followed by the input bytes consumed so far
    // (bufPos advances in lockstep with inPos, so this size is exact).
    std::vector<uint8_t> buffer(DICTIONARY_SIZE + inputLen);
    InitDictionary(buffer.data(), srcAddr, dstAddr);

    uint32_t bufPos = DICTIONARY_SIZE; // Next write position in buffer
    uint32_t inPos = 0;                // Current position in input
    uint32_t outPos = 0;               // Current position in output

    // Accumulate literal bytes to emit as a group
    std::vector<uint8_t> literalBuf;

    // Lambda to flush accumulated literals
    auto flushLiterals = [&]() -> bool {
        while (!literalBuf.empty())
        {
            uint32_t chunk = std::min<uint32_t>(literalBuf.size(), MAX_LITERAL_RUN);

            // Need 1 byte (count) + chunk bytes (data)
            if (outPos + 1 + chunk > outputMaxLen)
            {
                return false;
            }

            output[outPos++] = static_cast<uint8_t>(chunk); // 0kkkkkkk where k < 96
            std::memcpy(output + outPos, literalBuf.data(), chunk);
            outPos += chunk;

            literalBuf.erase(literalBuf.begin(), literalBuf.begin() + chunk);
        }
        return true;
    };

    while (inPos < inputLen)
    {
        // Evaluate both ZERO_INSERT and BACKREF at this position and pick the
        // one that consumes more input bytes. A backref that covers the zero
        // run PLUS following non-zero bytes (e.g., matching the "00 00 00 00
        // 01" pattern in the static dictionary) is strictly better than a
        // zero-insert that stops at the zero prefix, because it consumes the
        // trailing bytes in the same opcode budget instead of forcing them
        // into a separate literal run. This is the case flagged by Tommaso
        // Pecorella on MR !2802 for Fig.13 (suboptimal "82 03 01" vs optimal
        // "de 02").
        uint32_t zeros = CountZeros(input + inPos, inputLen - inPos);

        uint32_t matchOffset = 0;
        uint32_t matchLength = 0;
        bool haveMatch = FindLongestMatch(buffer.data(),
                                          bufPos,
                                          input + inPos,
                                          inputLen - inPos,
                                          matchOffset,
                                          matchLength);

        // Prefer BACKREF if it strictly covers more input than ZERO_INSERT
        // (or if no zero run is available). Ties go to ZERO_INSERT since its
        // encoding is simpler (no offset math, no extended args).
        const bool preferBackref = haveMatch && (zeros < 2 || matchLength > zeros);

        if (preferBackref)
        {
            if (!flushLiterals())
            {
                return 0;
            }

            uint32_t savedOutPos = outPos;
            if (EmitBackref(output, outPos, outputMaxLen, matchLength, matchOffset))
            {
                // Advance buffer with matched data
                std::memcpy(buffer.data() + bufPos, input + inPos, matchLength);
                bufPos += matchLength;
                inPos += matchLength;
                continue;
            }
            // Backref encoding failed (e.g., offset too large even with
            // extended args); rewind and fall back to zero-insert or literal.
            outPos = savedOutPos;
        }

        if (zeros >= 2)
        {
            if (!flushLiterals())
            {
                return 0;
            }

            // Emit zero insertion instructions
            while (zeros >= 2)
            {
                uint32_t emit = std::min<uint32_t>(zeros, MAX_ZERO_RUN);
                if (outPos + 1 > outputMaxLen)
                {
                    return 0;
                }

                // 1000nnnn where n = emit - 2
                output[outPos++] = 0x80 | ((emit - 2) & 0x0F);

                // Also advance the buffer (for future backreferences)
                std::memset(buffer.data() + bufPos, 0, emit);
                bufPos += emit;
                inPos += emit;
                zeros -= emit;
            }
            continue;
        }

        // Strategy 3: Accumulate as literal
        literalBuf.push_back(input[inPos]);
        buffer[bufPos++] = input[inPos];
        inPos++;
    }

    // Flush remaining literals
    if (!flushLiterals())
    {
        return 0;
    }

    // Emit stop code if requested (for extension headers)
    if (emitStopCode)
    {
        if (outPos + 1 > outputMaxLen)
        {
            return 0;
        }
        output[outPos++] = 0x90; // Stop code
    }

    // Only return compressed data if it's actually smaller
    uint32_t overhead = emitStopCode ? 1 : 0;
    if (outPos >= inputLen + overhead)
    {
        NS_LOG_DEBUG("GHC: No compression benefit (" << outPos << " >= " << inputLen << ")");
        return 0; // No compression benefit
    }

    NS_LOG_DEBUG("GHC: Compressed " << inputLen << " bytes to " << outPos << " bytes (ratio "
                                    << static_cast<float>(inputLen) / outPos << "x)");

    return outPos;
}

// ============================================================================
//  SixLowPanGhcExtension Implementation
// ============================================================================

NS_OBJECT_ENSURE_REGISTERED(SixLowPanGhcExtension);

SixLowPanGhcExtension::SixLowPanGhcExtension()
    : m_nhcByte(GHC_EXT_BASE),
      m_nextHeader(0),
      m_blobLength(0)
{
    std::memset(m_blob, 0, sizeof(m_blob));
}

TypeId
SixLowPanGhcExtension::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SixLowPanGhcExtension")
                            .SetParent<Header>()
                            .SetGroupName("SixLowPan")
                            .AddConstructor<SixLowPanGhcExtension>();
    return tid;
}

TypeId
SixLowPanGhcExtension::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SixLowPanGhcExtension::Print(std::ostream& os) const
{
    os << "GHC Ext Header: EID=" << GetEid() << " NH=" << GetNh() << " blob[" << +m_blobLength
       << "]";
}

uint32_t
SixLowPanGhcExtension::GetSerializedSize() const
{
    // 1 byte NHC + optional 1 byte Next Header + blob length
    uint32_t size = 1; // NHC byte
    if (!GetNh())
    {
        size += 1; // Next header inline
    }
    size += m_blobLength;
    return size;
}

void
SixLowPanGhcExtension::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_nhcByte);
    if (!GetNh())
    {
        start.WriteU8(m_nextHeader);
    }
    start.Write(m_blob, m_blobLength);
}

uint32_t
SixLowPanGhcExtension::Deserialize(Buffer::Iterator start)
{
    m_nhcByte = start.ReadU8();
    if (!GetNh())
    {
        m_nextHeader = start.ReadU8();
    }

    // For GHC extension headers, we need to scan for Stop Code to find blob end.
    // The deserializer must read bytecodes until Stop Code is found.
    m_blobLength = 0;
    while (start.GetRemainingSize() > 0 && m_blobLength < 255)
    {
        uint8_t byte = start.ReadU8();
        m_blob[m_blobLength++] = byte;

        if (byte == 0x90) // Stop code
        {
            break;
        }

        // If it's a literal (0kkkkkkk), skip k bytes
        if ((byte & 0x80) == 0)
        {
            uint32_t k = byte & 0x7F;
            for (uint32_t i = 0; i < k && start.GetRemainingSize() > 0 && m_blobLength < 255; i++)
            {
                m_blob[m_blobLength++] = start.ReadU8();
            }
        }
    }

    return GetSerializedSize();
}

SixLowPanDispatch::NhcDispatch_e
SixLowPanGhcExtension::GetNhcDispatchType() const
{
    return SixLowPanDispatch::LOWPAN_GHC_EXT;
}

void
SixLowPanGhcExtension::SetEid(Eid_e eid)
{
    m_nhcByte = GHC_EXT_BASE | ((static_cast<uint8_t>(eid) & 0x07) << 1) | (m_nhcByte & NH_MASK);
}

SixLowPanGhcExtension::Eid_e
SixLowPanGhcExtension::GetEid() const
{
    return static_cast<Eid_e>((m_nhcByte & EID_MASK) >> 1);
}

void
SixLowPanGhcExtension::SetNextHeader(uint8_t nextHeader)
{
    m_nextHeader = nextHeader;
}

uint8_t
SixLowPanGhcExtension::GetNextHeader() const
{
    return m_nextHeader;
}

void
SixLowPanGhcExtension::SetNh(bool nhField)
{
    if (nhField)
    {
        m_nhcByte |= NH_MASK;
    }
    else
    {
        m_nhcByte &= ~NH_MASK;
    }
}

bool
SixLowPanGhcExtension::GetNh() const
{
    return (m_nhcByte & NH_MASK) != 0;
}

void
SixLowPanGhcExtension::SetBlob(const uint8_t* blob, uint32_t size)
{
    NS_ASSERT(size <= MAX_BLOB_SIZE);
    m_blobLength = size;
    std::memcpy(m_blob, blob, size);
}

uint32_t
SixLowPanGhcExtension::CopyBlob(uint8_t* blob, uint32_t size) const
{
    uint32_t copyLen = std::min<uint32_t>(m_blobLength, size);
    std::memcpy(blob, m_blob, copyLen);
    return copyLen;
}

uint32_t
SixLowPanGhcExtension::GetBlobLength() const
{
    return m_blobLength;
}

std::ostream&
operator<<(std::ostream& os, const SixLowPanGhcExtension& header)
{
    header.Print(os);
    return os;
}

// ============================================================================
//  SixLowPanGhcUdp Implementation
// ============================================================================

NS_OBJECT_ENSURE_REGISTERED(SixLowPanGhcUdp);

SixLowPanGhcUdp::SixLowPanGhcUdp()
    : m_baseFormat(GHC_UDP_BASE),
      m_checksum(0),
      m_srcPort(0),
      m_dstPort(0)
{
}

TypeId
SixLowPanGhcUdp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SixLowPanGhcUdp")
                            .SetParent<Header>()
                            .SetGroupName("SixLowPan")
                            .AddConstructor<SixLowPanGhcUdp>();
    return tid;
}

TypeId
SixLowPanGhcUdp::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SixLowPanGhcUdp::Print(std::ostream& os) const
{
    os << "GHC UDP: src=" << m_srcPort << " dst=" << m_dstPort << " C=" << GetC()
       << " P=" << GetPorts();
}

uint32_t
SixLowPanGhcUdp::GetSerializedSize() const
{
    uint32_t size = 1; // NHC byte

    switch (GetPorts())
    {
    case PORTS_INLINE:
        size += 4; // 2 + 2
        break;
    case PORTS_ALL_SRC_LAST_DST:
    case PORTS_LAST_SRC_ALL_DST:
        size += 3; // 2 + 1 or 1 + 2
        break;
    case PORTS_LAST_SRC_LAST_DST:
        size += 1; // 4-bit + 4-bit packed
        break;
    }

    if (!GetC())
    {
        size += 2; // Checksum inline
    }

    return size;
}

void
SixLowPanGhcUdp::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_baseFormat);

    switch (GetPorts())
    {
    case PORTS_INLINE:
        start.WriteHtonU16(m_srcPort);
        start.WriteHtonU16(m_dstPort);
        break;
    case PORTS_ALL_SRC_LAST_DST:
        start.WriteHtonU16(m_srcPort);
        start.WriteU8(m_dstPort & 0xFF);
        break;
    case PORTS_LAST_SRC_ALL_DST:
        start.WriteU8(m_srcPort & 0xFF);
        start.WriteHtonU16(m_dstPort);
        break;
    case PORTS_LAST_SRC_LAST_DST:
        start.WriteU8(((m_srcPort & 0x0F) << 4) | (m_dstPort & 0x0F));
        break;
    }

    if (!GetC())
    {
        start.WriteHtonU16(m_checksum);
    }
}

uint32_t
SixLowPanGhcUdp::Deserialize(Buffer::Iterator start)
{
    m_baseFormat = start.ReadU8();

    switch (GetPorts())
    {
    case PORTS_INLINE:
        m_srcPort = start.ReadNtohU16();
        m_dstPort = start.ReadNtohU16();
        break;
    case PORTS_ALL_SRC_LAST_DST:
        m_srcPort = start.ReadNtohU16();
        m_dstPort = 0xF000 | start.ReadU8();
        break;
    case PORTS_LAST_SRC_ALL_DST:
        m_srcPort = 0xF000 | start.ReadU8();
        m_dstPort = start.ReadNtohU16();
        break;
    case PORTS_LAST_SRC_LAST_DST: {
        uint8_t packed = start.ReadU8();
        m_srcPort = 0xF0B0 | ((packed >> 4) & 0x0F);
        m_dstPort = 0xF0B0 | (packed & 0x0F);
        break;
    }
    }

    if (!GetC())
    {
        m_checksum = start.ReadNtohU16();
    }

    return GetSerializedSize();
}

SixLowPanDispatch::NhcDispatch_e
SixLowPanGhcUdp::GetNhcDispatchType() const
{
    return SixLowPanDispatch::LOWPAN_GHC_UDP;
}

void
SixLowPanGhcUdp::SetPorts(Ports_e port)
{
    m_baseFormat = GHC_UDP_BASE | (m_baseFormat & C_MASK) | (static_cast<uint8_t>(port) & P_MASK);
}

SixLowPanGhcUdp::Ports_e
SixLowPanGhcUdp::GetPorts() const
{
    return static_cast<Ports_e>(m_baseFormat & P_MASK);
}

void
SixLowPanGhcUdp::SetSrcPort(uint16_t port)
{
    m_srcPort = port;
}

uint16_t
SixLowPanGhcUdp::GetSrcPort() const
{
    return m_srcPort;
}

void
SixLowPanGhcUdp::SetDstPort(uint16_t port)
{
    m_dstPort = port;
}

uint16_t
SixLowPanGhcUdp::GetDstPort() const
{
    return m_dstPort;
}

void
SixLowPanGhcUdp::SetC(bool cField)
{
    if (cField)
    {
        m_baseFormat |= C_MASK;
    }
    else
    {
        m_baseFormat &= ~C_MASK;
    }
}

bool
SixLowPanGhcUdp::GetC() const
{
    return (m_baseFormat & C_MASK) != 0;
}

void
SixLowPanGhcUdp::SetChecksum(uint16_t checksum)
{
    m_checksum = checksum;
}

uint16_t
SixLowPanGhcUdp::GetChecksum() const
{
    return m_checksum;
}

std::ostream&
operator<<(std::ostream& os, const SixLowPanGhcUdp& header)
{
    header.Print(os);
    return os;
}

// ============================================================================
//  SixLowPanGhcIcmpv6 Implementation
// ============================================================================

NS_OBJECT_ENSURE_REGISTERED(SixLowPanGhcIcmpv6);

SixLowPanGhcIcmpv6::SixLowPanGhcIcmpv6()
    : m_blobLength(0)
{
    std::memset(m_blob, 0, sizeof(m_blob));
}

TypeId
SixLowPanGhcIcmpv6::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SixLowPanGhcIcmpv6")
                            .SetParent<Header>()
                            .SetGroupName("SixLowPan")
                            .AddConstructor<SixLowPanGhcIcmpv6>();
    return tid;
}

TypeId
SixLowPanGhcIcmpv6::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SixLowPanGhcIcmpv6::Print(std::ostream& os) const
{
    os << "GHC ICMPv6: blob[" << +m_blobLength << "]";
}

uint32_t
SixLowPanGhcIcmpv6::GetSerializedSize() const
{
    return 1 + m_blobLength; // NHC byte + compressed ICMPv6 body
}

void
SixLowPanGhcIcmpv6::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(GHC_ICMPV6_NHC);
    start.Write(m_blob, m_blobLength);
}

uint32_t
SixLowPanGhcIcmpv6::Deserialize(Buffer::Iterator start)
{
    NS_FATAL_ERROR("The two-iterator Deserialize must be used for this variable-sized header");
    return 0;
}

uint32_t
SixLowPanGhcIcmpv6::Deserialize(Buffer::Iterator start, Buffer::Iterator end)
{
    uint8_t nhc = start.ReadU8();
    NS_ASSERT(nhc == GHC_ICMPV6_NHC);

    // The wire format has no length field: the bytecode blob runs to the
    // end of the packet (RFC 7400 Section 3), which the caller passes as
    // the end iterator.
    uint32_t blobLen = start.GetDistanceFrom(end);
    NS_ASSERT(blobLen <= MAX_BLOB_SIZE);
    m_blobLength = static_cast<uint8_t>(blobLen);
    start.Read(m_blob, m_blobLength);

    return GetSerializedSize();
}

SixLowPanDispatch::NhcDispatch_e
SixLowPanGhcIcmpv6::GetNhcDispatchType() const
{
    return SixLowPanDispatch::LOWPAN_GHC_ICMPV6;
}

void
SixLowPanGhcIcmpv6::SetBlob(const uint8_t* blob, uint32_t size)
{
    NS_ASSERT(size <= MAX_BLOB_SIZE);
    m_blobLength = size;
    std::memcpy(m_blob, blob, size);
}

uint32_t
SixLowPanGhcIcmpv6::CopyBlob(uint8_t* blob, uint32_t size) const
{
    uint32_t copyLen = std::min<uint32_t>(m_blobLength, size);
    std::memcpy(blob, m_blob, copyLen);
    return copyLen;
}

uint32_t
SixLowPanGhcIcmpv6::GetBlobLength() const
{
    return m_blobLength;
}

std::ostream&
operator<<(std::ostream& os, const SixLowPanGhcIcmpv6& header)
{
    header.Print(os);
    return os;
}

// ============================================================================
//  SixLowPan6Cio Implementation
// ============================================================================

NS_OBJECT_ENSURE_REGISTERED(SixLowPan6Cio);

SixLowPan6Cio::SixLowPan6Cio()
    : m_flags(0)
{
}

TypeId
SixLowPan6Cio::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SixLowPan6Cio")
                            .SetParent<Header>()
                            .SetGroupName("SixLowPan")
                            .AddConstructor<SixLowPan6Cio>();
    return tid;
}

TypeId
SixLowPan6Cio::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SixLowPan6Cio::Print(std::ostream& os) const
{
    os << "6CIO: GHC=" << GetGhcCapable() << " flags=0x" << std::hex << m_flags << std::dec;
}

uint32_t
SixLowPan6Cio::GetSerializedSize() const
{
    // Type(1) + Length(1) + Flags(4 bytes = 32 bits)
    // Length field = 1 (in 8-octet units) => total 8 bytes
    // But we use minimum 6 bytes (Type + Len + 4 flag bytes)
    return 8; // Padded to 8-octet boundary per ND option rules
}

void
SixLowPan6Cio::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(SIXLOWPAN_6CIO_TYPE);
    start.WriteU8(SIXLOWPAN_6CIO_LEN);
    start.WriteHtonU32(m_flags);
    start.WriteU16(0); // Padding to 8-byte boundary
}

uint32_t
SixLowPan6Cio::Deserialize(Buffer::Iterator start)
{
    uint8_t type = start.ReadU8();
    NS_ASSERT(type == SIXLOWPAN_6CIO_TYPE);
    uint8_t len = start.ReadU8();
    m_flags = start.ReadNtohU32();

    // Skip remaining bytes based on length field
    uint32_t remaining = (len * 8) - 6; // Total - (type + len + 4 flag bytes)
    for (uint32_t i = 0; i < remaining; i++)
    {
        start.ReadU8();
    }

    return len * 8;
}

void
SixLowPan6Cio::SetGhcCapable(bool ghcCapable)
{
    if (ghcCapable)
    {
        m_flags |= GHC_FLAG_BIT;
    }
    else
    {
        m_flags &= ~GHC_FLAG_BIT;
    }
}

bool
SixLowPan6Cio::GetGhcCapable() const
{
    return (m_flags & GHC_FLAG_BIT) != 0;
}

std::ostream&
operator<<(std::ostream& os, const SixLowPan6Cio& header)
{
    header.Print(os);
    return os;
}

} // namespace ns3
