/*
 * Copyright (c) 2026 SRM Institute of Science and Technology
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 *
 * 6LoWPAN-GHC: Generic Header Compression for IPv6 over
 * Low-Power Wireless Personal Area Networks (6LoWPANs) - RFC 7400
 *
 * Source: original implementation following RFC 7400 (Bormann, November 2014).
 */

#ifndef SIXLOWPAN_GHC_H
#define SIXLOWPAN_GHC_H

#include "sixlowpan-header.h"

#include "ns3/buffer.h"
#include "ns3/header.h"
#include "ns3/ipv6-address.h"

#include <cstdint>
#include <vector>

namespace ns3
{

/**
 * @ingroup sixlowpan
 * @brief GHC bytecode instruction types per RFC 7400 Section 3.
 *
 * The decompressor processes a stream of bytecode instructions:
 *   - Literal:      0kkkkkkk  - Copy k literal bytes from stream
 *   - ZeroInsert:   1000nnnn  - Insert (n+2) zero bytes
 *   - StopCode:     10010000  - Terminate extension header decompression
 *   - ExtendedArgs: 101nssss  - Extend next backreference range
 *   - Backref:      11nnnkkk  - Copy n bytes from s bytes back
 */
enum class GhcBytecodeType : uint8_t
{
    LITERAL = 0,   //!< 0kkkkkkk: copy k literal bytes
    ZERO_INSERT,   //!< 1000nnnn: insert (n+2) zero bytes
    STOP_CODE,     //!< 10010000: terminate decompression
    EXTENDED_ARGS, //!< 101nssss: extend backreference
    BACKREF,       //!< 11nnnkkk: backreference copy
    RESERVED       //!< Reserved/invalid pattern
};

// ============================================================================
//  GHC Compression/Decompression Engine
// ============================================================================

/**
 * @ingroup sixlowpan
 * @brief Core GHC compression and decompression engine per RFC 7400.
 *
 * This class implements the LZ77-style bytecode compression defined in
 * RFC 7400 Section 3. The engine operates on raw byte buffers and is
 * used by the GHC header classes for actual data transformation.
 *
 * The decompression buffer is pre-initialized with a 48-byte dictionary:
 *   Bytes  0-15: Source IPv6 address from the packet
 *   Bytes 16-31: Destination IPv6 address from the packet
 *   Bytes 32-47: Static dictionary (16 bytes from RFC 7400 Section 3.1)
 *
 * The static dictionary contains:
 *   0x16 0xfe 0xfd 0x17 0xfe 0xfd 0x00 0x01
 *   0x00 0x00 0x00 0x00 0x00 0x01 0x00 0x00
 */
class SixLowPanGhcEngine
{
  public:
    /**
     * @brief Predefined dictionary size (48 bytes).
     *
     * 16 bytes source address + 16 bytes destination address + 16 bytes static.
     */
    static constexpr uint32_t DICTIONARY_SIZE = 48;

    /**
     * @brief Maximum bytes copied by one 0kkkkkkk literal bytecode.
     *
     * k is in [1, 95]; values 96-127 are reserved (RFC 7400 Section 3).
     */
    static constexpr uint32_t MAX_LITERAL_RUN = 95;

    /**
     * @brief Maximum zeros inserted by one 1000nnnn bytecode: n+2 with n <= 15.
     */
    static constexpr uint32_t MAX_ZERO_RUN = 17;

    /**
     * @brief Maximum copy length of one 11nnnkkk backreference.
     *
     * n = nnn (max 7) + na (max 8 from one extended-args byte) + 2 = 17.
     * Longer repeats become consecutive backreferences.
     */
    static constexpr uint32_t MAX_BACKREF_LEN = 17;

    /**
     * @brief Decompress a GHC-compressed byte stream.
     *
     * @param [in] srcAddr   Source IPv6 address for dictionary initialization.
     * @param [in] dstAddr   Destination IPv6 address for dictionary initialization.
     * @param [in] compressed   Pointer to compressed bytecode data.
     * @param [in] compressedLen Length of compressed data.
     * @param [out] output   Buffer to receive decompressed output.
     * @param [in] outputMaxLen Maximum output buffer size.
     * @param [in] useStopCode  If true, decompression terminates on Stop Code
     *                          (used for extension headers where Length is elided).
     * @return Number of decompressed bytes, or 0 on error.
     */
    static uint32_t Decompress(const Ipv6Address& srcAddr,
                               const Ipv6Address& dstAddr,
                               const uint8_t* compressed,
                               uint32_t compressedLen,
                               uint8_t* output,
                               uint32_t outputMaxLen,
                               bool useStopCode = false);

    /**
     * @brief Compress a byte stream using the GHC LZ77 algorithm.
     *
     * The compressor scans the dictionary and previously output data for
     * the longest match, then emits backreference bytecodes. This is a
     * full LZ77 compressor implementation.
     *
     * @param [in] srcAddr     Source IPv6 address for dictionary initialization.
     * @param [in] dstAddr     Destination IPv6 address for dictionary initialization.
     * @param [in] input       Pointer to uncompressed data.
     * @param [in] inputLen    Length of uncompressed data.
     * @param [out] output     Buffer to receive compressed bytecodes.
     * @param [in] outputMaxLen Maximum output buffer size.
     * @param [in] emitStopCode If true, append Stop Code at end (for extension headers).
     * @return Number of compressed bytes, or 0 on error/no compression benefit.
     */
    static uint32_t Compress(const Ipv6Address& srcAddr,
                             const Ipv6Address& dstAddr,
                             const uint8_t* input,
                             uint32_t inputLen,
                             uint8_t* output,
                             uint32_t outputMaxLen,
                             bool emitStopCode = false);

    /**
     * @brief Classify a GHC bytecode instruction.
     *
     * @param [in] byte The bytecode byte to classify.
     * @return The bytecode instruction type.
     */
    static GhcBytecodeType ClassifyBytecode(uint8_t byte);

  private:
    /**
     * @brief Initialize the 48-byte decompression dictionary.
     *
     * @param [out] dict     Buffer to receive dictionary (must be >= 48 bytes).
     * @param [in] srcAddr   Source IPv6 address.
     * @param [in] dstAddr   Destination IPv6 address.
     */
    static void InitDictionary(uint8_t* dict,
                               const Ipv6Address& srcAddr,
                               const Ipv6Address& dstAddr);

    /**
     * @brief Count the extended-args bytes a backreference encoding needs.
     *
     * Each 101nssss extended-args byte carries up to 15*8 = 120 of the
     * start-adjust and 8 of the number-adjust accumulator (RFC 7400
     * Section 3.2).
     *
     * @param [in] matchLength  Number of bytes to copy (>= 2).
     * @param [in] matchOffset  Offset from current position (>= matchLength).
     * @return Number of 101nssss bytes needed before the 11nnnkkk bytecode.
     */
    static uint32_t CountBackrefExtArgBytes(uint32_t matchLength, uint32_t matchOffset);

    /**
     * @brief Find the longest match in the buffer for LZ77 compression.
     *
     * Searches backward from the current position for the longest
     * substring match, respecting GHC backreference encoding limits.
     *
     * @param [in] buffer    Full buffer (dictionary + output so far).
     * @param [in] bufLen    Current buffer length.
     * @param [in] input     Input data at current position.
     * @param [in] inputRemaining Remaining input bytes.
     * @param [out] matchOffset  Offset of best match (bytes back from bufLen).
     * @param [out] matchLength  Length of best match.
     * @return true if a useful match was found (length >= 2).
     */
    static bool FindLongestMatch(const uint8_t* buffer,
                                 uint32_t bufLen,
                                 const uint8_t* input,
                                 uint32_t inputRemaining,
                                 uint32_t& matchOffset,
                                 uint32_t& matchLength);

    /**
     * @brief Emit a backreference bytecode (with optional extended args).
     *
     * @param [out] output      Output bytecode buffer.
     * @param [in,out] outPos   Current output position (advanced on write).
     * @param [in] outputMaxLen Maximum output buffer size.
     * @param [in] matchLength  Number of bytes to copy (>= 2).
     * @param [in] matchOffset  Offset from current position (>= matchLength).
     * @return true on success, false if output buffer would overflow.
     */
    static bool EmitBackref(uint8_t* output,
                            uint32_t& outPos,
                            uint32_t outputMaxLen,
                            uint32_t matchLength,
                            uint32_t matchOffset);

    /**
     * @brief Count consecutive zero bytes in input.
     *
     * @param [in] input Pointer to current input position.
     * @param [in] remaining Remaining bytes in input.
     * @return Number of consecutive zero bytes (0 to remaining).
     */
    static uint32_t CountZeros(const uint8_t* input, uint32_t remaining);
};

// ============================================================================
//  GHC NHC Header Classes
// ============================================================================

/**
 * @ingroup sixlowpan
 * @brief GHC NHC Extension Header - see RFC 7400 Section 5.
 *
 * Replaces the RFC 6282 extension header NHC byte pattern (1110EID[NH])
 * with the GHC pattern (1011EID[NH]).
 *
 * @verbatim
 *     0   1   2   3   4   5   6   7
 *   +---+---+---+---+---+---+---+---+
 *   | 1 | 0 | 1 | 1 |    EID    |NH |
 *   +---+---+---+---+---+---+---+---+
 * @endverbatim
 *
 * The compressed payload following this header contains GHC bytecodes.
 * Decompression terminates upon encountering the Stop Code (0x90).
 * The Length field of the original extension header is elided.
 */
class SixLowPanGhcExtension : public Header
{
  public:
    /**
     * @brief Extension Header ID (same encoding as RFC 6282).
     */
    enum Eid_e
    {
        EID_HOPBYHOP_OPTIONS_H = 0,
        EID_ROUTING_H = 1,
        EID_FRAGMENTATION_H = 2,
        EID_DESTINATION_OPTIONS_H = 3,
        EID_MOBILITY_H = 4,
        EID_IPv6_H = 7
    };

    SixLowPanGhcExtension();

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Get the NhcDispatch type.
     * @return The NhcDispatch type (LOWPAN_GHC_EXT).
     */
    SixLowPanDispatch::NhcDispatch_e GetNhcDispatchType() const;

    /**
     * @brief Set the Extension Header Type.
     * @param [in] eid The Extension Header Type.
     */
    void SetEid(Eid_e eid);

    /**
     * @brief Get the Extension Header Type.
     * @return The Extension Header Type.
     */
    Eid_e GetEid() const;

    /**
     * @brief Set the Next Header field value.
     * @param [in] nextHeader The Next Header value.
     */
    void SetNextHeader(uint8_t nextHeader);

    /**
     * @brief Get the Next Header field value.
     * @return The Next Header value.
     */
    uint8_t GetNextHeader() const;

    /**
     * @brief Set the NH field (next header compressed flag).
     * @param [in] nhField True if next header is compressed via NHC/GHC.
     */
    void SetNh(bool nhField);

    /**
     * @brief Get the NH field.
     * @return True if next header is compressed.
     */
    bool GetNh() const;

    /**
     * @brief Maximum blob size: the blob length field is 8 bits.
     */
    static constexpr uint32_t MAX_BLOB_SIZE = 255;

    /**
     * @brief Set the GHC-compressed bytecode blob.
     * @param [in] blob  Buffer holding compressed bytecodes.
     * @param [in] size  Blob length.
     */
    void SetBlob(const uint8_t* blob, uint32_t size);

    /**
     * @brief Copy the GHC-compressed bytecode blob.
     * @param [out] blob  Buffer to copy blob into.
     * @param [in] size   Buffer size.
     * @return Bytes copied.
     */
    uint32_t CopyBlob(uint8_t* blob, uint32_t size) const;

    /**
     * @brief Get the blob length.
     * @return Length of the compressed bytecode blob.
     */
    uint32_t GetBlobLength() const;

  private:
    uint8_t m_nhcByte;                 //!< NHC header byte: 1011EID[NH]
    uint8_t m_nextHeader;              //!< Next Header value (if NH=0)
    uint8_t m_blobLength;              //!< Length of GHC compressed data
    uint8_t m_blob[MAX_BLOB_SIZE + 1]; //!< GHC compressed bytecodes

    /**
     * @brief Base NHC byte pattern for GHC extension: 10110000 = 0xB0.
     */
    static constexpr uint8_t GHC_EXT_BASE = 0xB0;

    /**
     * @brief Mask for EID field extraction (bits 1-3).
     */
    static constexpr uint8_t EID_MASK = 0x0E;

    /**
     * @brief Mask for NH flag extraction (bit 0).
     */
    static constexpr uint8_t NH_MASK = 0x01;
};

/**
 * @brief Stream insertion operator.
 * @param [in,out] os The reference to the output stream.
 * @param [in] header The SixLowPanGhcExtension header.
 * @return The reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const SixLowPanGhcExtension& header);

// ============================================================================

/**
 * @ingroup sixlowpan
 * @brief GHC UDP NHC Header - see RFC 7400 Section 5.
 *
 * Replaces the RFC 6282 UDP NHC byte pattern (11110CPP)
 * with the GHC pattern (11010CPP). Port compression uses
 * the same encoding as RFC 6282. GHC bytecodes compress
 * the UDP payload following the header.
 *
 * @verbatim
 *     0   1   2   3   4   5   6   7
 *   +---+---+---+---+---+---+---+---+
 *   | 1 | 1 | 0 | 1 | 0 | C |   P   |
 *   +---+---+---+---+---+---+---+---+
 * @endverbatim
 */
class SixLowPanGhcUdp : public Header
{
  public:
    /**
     * @brief Port compression modes (same as RFC 6282 UDP NHC).
     */
    enum Ports_e
    {
        PORTS_INLINE = 0,
        PORTS_ALL_SRC_LAST_DST = 1,
        PORTS_LAST_SRC_ALL_DST = 2,
        PORTS_LAST_SRC_LAST_DST = 3
    };

    SixLowPanGhcUdp();

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Get the NhcDispatch type.
     * @return The NhcDispatch type (LOWPAN_GHC_UDP).
     */
    SixLowPanDispatch::NhcDispatch_e GetNhcDispatchType() const;

    /**
     * @brief Set the port compression mode.
     * @param [in] port The port compression mode.
     */
    void SetPorts(Ports_e port);

    /**
     * @brief Get the port compression mode.
     * @return The port compression mode.
     */
    Ports_e GetPorts() const;

    /**
     * @brief Set the source port.
     * @param [in] port The source port.
     */
    void SetSrcPort(uint16_t port);

    /**
     * @brief Get the source port.
     * @return The source port.
     */
    uint16_t GetSrcPort() const;

    /**
     * @brief Set the destination port.
     * @param [in] port The destination port.
     */
    void SetDstPort(uint16_t port);

    /**
     * @brief Get the destination port.
     * @return The destination port.
     */
    uint16_t GetDstPort() const;

    /**
     * @brief Set the checksum elision flag (C bit).
     * @param [in] cField True if checksum is elided.
     */
    void SetC(bool cField);

    /**
     * @brief Get the checksum elision flag (C bit).
     * @return True if checksum is elided.
     */
    bool GetC() const;

    /**
     * @brief Set the UDP checksum.
     * @param [in] checksum The UDP checksum value.
     */
    void SetChecksum(uint16_t checksum);

    /**
     * @brief Get the UDP checksum.
     * @return The UDP checksum value.
     */
    uint16_t GetChecksum() const;

  private:
    uint8_t m_baseFormat; //!< NHC byte: 11010CPP
    uint16_t m_checksum;  //!< UDP checksum
    uint16_t m_srcPort;   //!< Source port
    uint16_t m_dstPort;   //!< Destination port

    static constexpr uint8_t GHC_UDP_BASE = 0xD0; //!< 11010000
    static constexpr uint8_t C_MASK = 0x04;       //!< Checksum flag mask
    static constexpr uint8_t P_MASK = 0x03;       //!< Port compression mask
};

/**
 * @brief Stream insertion operator.
 * @param [in,out] os The reference to the output stream.
 * @param [in] header The SixLowPanGhcUdp header.
 * @return The reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const SixLowPanGhcUdp& header);

// ============================================================================

/**
 * @ingroup sixlowpan
 * @brief GHC ICMPv6 NHC Header - see RFC 7400 Section 5.
 *
 * Fixed NHC byte value 0xDF (11011111). This is the key GHC addition
 * that fills the gap left by RFC 6282 which defines no ICMPv6 NHC.
 * The entire ICMPv6 message body is compressed using GHC bytecodes.
 *
 * @verbatim
 *     0   1   2   3   4   5   6   7
 *   +---+---+---+---+---+---+---+---+
 *   | 1 | 1 | 0 | 1 | 1 | 1 | 1 | 1 |
 *   +---+---+---+---+---+---+---+---+
 * @endverbatim
 *
 * This header enables compression of:
 *   - RPL control messages (DIO, DAO, DIS, DAO-ACK)
 *   - Neighbor Discovery messages (NS, NA, RS, RA)
 *   - Echo Request/Reply
 *   - Any ICMPv6 message
 */
class SixLowPanGhcIcmpv6 : public Header
{
  public:
    SixLowPanGhcIcmpv6();

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Deserialize the header from the given buffer region.
     *
     * The GHC ICMPv6 wire format has no length field: the bytecode blob
     * extends to the end of the packet (RFC 7400 Section 3). This
     * variable-size variant must be used, through
     * Packet::RemoveHeader(header, size).
     *
     * @param [in] start Start of the header.
     * @param [in] end   End of the blob (end of the packet).
     * @return Bytes consumed.
     */
    uint32_t Deserialize(Buffer::Iterator start, Buffer::Iterator end) override;

    /**
     * @brief Get the NhcDispatch type.
     * @return The NhcDispatch type (LOWPAN_GHC_ICMPV6).
     */
    SixLowPanDispatch::NhcDispatch_e GetNhcDispatchType() const;

    /**
     * @brief Maximum blob size: the blob length field is 8 bits.
     */
    static constexpr uint32_t MAX_BLOB_SIZE = 255;

    /**
     * @brief Set the GHC-compressed bytecode blob for the ICMPv6 body.
     * @param [in] blob  Buffer holding compressed bytecodes.
     * @param [in] size  Blob length.
     */
    void SetBlob(const uint8_t* blob, uint32_t size);

    /**
     * @brief Copy the GHC-compressed bytecode blob.
     * @param [out] blob Buffer to copy blob into.
     * @param [in] size  Buffer size.
     * @return Bytes copied.
     */
    uint32_t CopyBlob(uint8_t* blob, uint32_t size) const;

    /**
     * @brief Get the blob length.
     * @return Compressed bytecode length.
     */
    uint32_t GetBlobLength() const;

  private:
    static constexpr uint8_t GHC_ICMPV6_NHC = 0xDF; //!< Fixed: 11011111
    uint8_t m_blobLength;                           //!< Length of GHC compressed ICMPv6 body
    uint8_t m_blob[MAX_BLOB_SIZE + 1];              //!< GHC compressed bytecodes
};

/**
 * @brief Stream insertion operator.
 * @param [in,out] os The reference to the output stream.
 * @param [in] header The SixLowPanGhcIcmpv6 header.
 * @return The reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const SixLowPanGhcIcmpv6& header);

// ============================================================================
//  6CIO - 6LoWPAN Capability Indication Option
// ============================================================================

/**
 * @ingroup sixlowpan
 * @brief 6LoWPAN Capability Indication Option (6CIO) - RFC 7400 Section 6.
 *
 * Carried in Neighbor Discovery messages (RS, RA, NS, NA) to advertise
 * GHC capability. The G bit (bit 15 of the flags field) indicates
 * that the node supports GHC compression.
 *
 * @verbatim
 *   +--------+--------+--------+--------+--------+--------+
 *   | Type=36| Length |    Flags (variable, 16+ bits)      |
 *   +--------+--------+--------+--------+--------+--------+
 *
 *   Flags[15] = G bit: GHC Capable (1 = supports GHC)
 * @endverbatim
 */
class SixLowPan6Cio : public Header
{
  public:
    SixLowPan6Cio();

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Set the GHC capability flag.
     * @param [in] ghcCapable True if this node supports GHC.
     */
    void SetGhcCapable(bool ghcCapable);

    /**
     * @brief Get the GHC capability flag.
     * @return True if GHC is supported.
     */
    bool GetGhcCapable() const;

  private:
    static constexpr uint8_t SIXLOWPAN_6CIO_TYPE = 36;   //!< ND Option Type
    static constexpr uint8_t SIXLOWPAN_6CIO_LEN = 1;     //!< Length in 8-octet units
    static constexpr uint32_t GHC_FLAG_BIT = (1u << 15); //!< G bit position

    uint32_t m_flags; //!< Flags field (variable, up to 48 bits per RFC)
};

/**
 * @brief Stream insertion operator.
 * @param [in,out] os The reference to the output stream.
 * @param [in] header The SixLowPan6Cio header.
 * @return The reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const SixLowPan6Cio& header);

} // namespace ns3

#endif /* SIXLOWPAN_GHC_H */
