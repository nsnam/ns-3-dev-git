/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef RADIOTAP_HEADER_H
#define RADIOTAP_HEADER_H

#include "ns3/header.h"

#include <array>
#include <optional>
#include <vector>

namespace ns3
{

/**
 * @brief Radiotap header implementation
 *
 * Radiotap is a de facto standard for 802.11 frame injection and reception.
 * The radiotap header format is a mechanism to supply additional information
 * about frames, from the driver to userspace applications such as libpcap, and
 * from a userspace application to the driver for transmission.
 */
class RadiotapHeader : public Header
{
  public:
    RadiotapHeader();
    /**
     * @brief Get the type ID.
     * @returns the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    /**
     * This method is used by Packet::AddHeader to store the header into the byte
     * buffer of a packet.  This method returns the number of bytes which are
     * needed to store the header data during a Serialize.
     *
     * @returns The expected size of the header.
     */
    uint32_t GetSerializedSize() const override;

    /**
     * This method is used by Packet::AddHeader to store the header into the byte
     * buffer of a packet.  The data written is expected to match bit-for-bit the
     * representation of this header in a real network.
     *
     * @param start An iterator which points to where the header should
     *              be written.
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * This method is used by Packet::RemoveHeader to re-create a header from the
     * byte buffer of a packet.  The data read is expected to match bit-for-bit
     * the representation of this header in real networks.
     *
     * @param start An iterator which points to where the header should be read.
     * @returns The number of bytes read.
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * This method is used by Packet::Print to print the content of the header as
     * ascii data to a C++ output stream.  Although the header is free to format
     * its output as it wishes, it is recommended to follow a few rules to integrate
     * with the packet pretty printer: start with flags, small field
     * values located between a pair of parens. Values should be separated
     * by whitespace. Follow the parens with the important fields,
     * separated by whitespace.
     *
     * eg: (field1 val1 field2 val2 field3 val3) field4 val4 field5 val5
     *
     * @param os The output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * @brief Set the Time Synchronization Function Timer (TSFT) value.  Valid for
     * received frames only.
     *
     * @param tsft Value in microseconds of the MAC's 64-bit 802.11 Time
     *             Synchronization Function timer when the first bit of the MPDU
     *             arrived at the MAC.
     */
    void SetTsft(uint64_t tsft);

    /**
     * @brief Frame flags.
     */
    enum FrameFlag : uint8_t
    {
        FRAME_FLAG_NONE = 0x00,           /**< No flags set */
        FRAME_FLAG_CFP = 0x01,            /**< Frame sent/received during CFP */
        FRAME_FLAG_SHORT_PREAMBLE = 0x02, /**< Frame sent/received with short preamble */
        FRAME_FLAG_WEP = 0x04,            /**< Frame sent/received with WEP encryption */
        FRAME_FLAG_FRAGMENTED = 0x08,     /**< Frame sent/received with fragmentation */
        FRAME_FLAG_FCS_INCLUDED = 0x10,   /**< Frame includes FCS */
        FRAME_FLAG_DATA_PADDING =
            0x20, /**< Frame has padding between 802.11 header and payload (to 32-bit boundary) */
        FRAME_FLAG_BAD_FCS = 0x40,    /**< Frame failed FCS check */
        FRAME_FLAG_SHORT_GUARD = 0x80 /**< Frame used short guard interval (HT) */
    };

    /**
     * @brief Set the frame flags of the transmitted or received frame.
     * @param flags flags to set.
     */
    void SetFrameFlags(uint8_t flags);

    /**
     * @brief Set the transmit/receive channel frequency in units of megahertz
     * @param rate the transmit/receive channel frequency in units of megahertz.
     */
    void SetRate(uint8_t rate);

    /**
     * @brief Channel flags.
     */
    enum ChannelFlags : uint16_t
    {
        CHANNEL_FLAG_NONE = 0x0000,          /**< No flags set */
        CHANNEL_FLAG_TURBO = 0x0010,         /**< Turbo Channel */
        CHANNEL_FLAG_CCK = 0x0020,           /**< CCK channel */
        CHANNEL_FLAG_OFDM = 0x0040,          /**< OFDM channel */
        CHANNEL_FLAG_SPECTRUM_2GHZ = 0x0080, /**< 2 GHz spectrum channel */
        CHANNEL_FLAG_SPECTRUM_5GHZ = 0x0100, /**< 5 GHz spectrum channel */
        CHANNEL_FLAG_PASSIVE = 0x0200,       /**< Only passive scan allowed */
        CHANNEL_FLAG_DYNAMIC = 0x0400,       /**< Dynamic CCK-OFDM channel */
        CHANNEL_FLAG_GFSK = 0x0800           /**< GFSK channel (FHSS PHY) */
    };

    /**
     * structure that contains the subfields of the Channel field.
     */
    struct ChannelFields
    {
        uint16_t frequency{0};             //!< Tx/Rx frequency in MHz
        uint16_t flags{CHANNEL_FLAG_NONE}; //!< flags field (@see ChannelFlags)
    };

    /**
     * @brief Set the subfields of the Channel field
     *
     * @param channelFields The subfields of the Channel field.
     */
    void SetChannelFields(const ChannelFields& channelFields);

    /**
     * @brief Set the RF signal power at the antenna as a decibel difference
     * from an arbitrary, fixed reference.
     *
     * @param signal The RF signal power at the antenna as a decibel difference
     *               from an arbitrary, fixed reference;
     */
    void SetAntennaSignalPower(double signal);

    /**
     * @brief Set the RF noise power at the antenna as a decibel difference
     * from an arbitrary, fixed reference.
     *
     * @param noise The RF noise power at the antenna as a decibel difference
     *              from an arbitrary, fixed reference.
     */
    void SetAntennaNoisePower(double noise);

    /**
     * @brief MCS known bits.
     */
    enum McsKnown : uint8_t
    {
        MCS_KNOWN_NONE = 0x00,           /**< No flags set */
        MCS_KNOWN_BANDWIDTH = 0x01,      /**< Bandwidth */
        MCS_KNOWN_INDEX = 0x02,          /**< MCS index known */
        MCS_KNOWN_GUARD_INTERVAL = 0x04, /**< Guard interval */
        MCS_KNOWN_HT_FORMAT = 0x08,      /**< HT format */
        MCS_KNOWN_FEC_TYPE = 0x10,       /**< FEC type */
        MCS_KNOWN_STBC = 0x20,           /**< STBC known */
        MCS_KNOWN_NESS = 0x40,           /**< Ness known (Number of extension spatial streams) */
        MCS_KNOWN_NESS_BIT_1 =
            0x80, /**< Ness data - bit 1 (MSB) of Number of extension spatial streams */
    };

    /**
     * @brief MCS flags.
     */
    enum McsFlags : uint8_t
    {
        MCS_FLAGS_NONE =
            0x00, /**< Default: 20 MHz, long guard interval, mixed HT format and BCC FEC type */
        MCS_FLAGS_BANDWIDTH_40 = 0x01,   /**< 40 MHz */
        MCS_FLAGS_BANDWIDTH_20L = 0x02,  /**< 20L (20 MHz in lower half of 40 MHz channel) */
        MCS_FLAGS_BANDWIDTH_20U = 0x03,  /**< 20U (20 MHz in upper half of 40 MHz channel) */
        MCS_FLAGS_GUARD_INTERVAL = 0x04, /**< Short guard interval */
        MCS_FLAGS_HT_GREENFIELD = 0x08,  /**< Greenfield HT format */
        MCS_FLAGS_FEC_TYPE = 0x10,       /**< LDPC FEC type */
        MCS_FLAGS_STBC_STREAMS = 0x60,   /**< STBC enabled */
        MCS_FLAGS_NESS_BIT_0 =
            0x80, /**< Ness - bit 0 (LSB) of Number of extension spatial streams */
    };

    /**
     * structure that contains the subfields of the MCS field.
     */
    struct McsFields
    {
        uint8_t known{MCS_KNOWN_NONE}; //!< known flags
        uint8_t flags{MCS_FLAGS_NONE}; //!< flags field
        uint8_t mcs{0};                //!< MCS index value
    };

    /**
     * @brief Set the subfields of the MCS field
     *
     * @param mcsFields The subfields of the MCS field.
     */
    void SetMcsFields(const McsFields& mcsFields);

    /**
     * @brief A-MPDU status flags.
     */
    enum AmpduFlags : uint8_t
    {
        A_MPDU_STATUS_NONE = 0x00,               /**< No flags set */
        A_MPDU_STATUS_REPORT_ZERO_LENGTH = 0x01, /**< Driver reports 0-length subframes */
        A_MPDU_STATUS_IS_ZERO_LENGTH =
            0x02, /**< Frame is 0-length subframe (valid only if 0x0001 is set) */
        A_MPDU_STATUS_LAST_KNOWN =
            0x04, /**< Last subframe is known (should be set for all subframes in an A-MPDU) */
        A_MPDU_STATUS_LAST = 0x08,                /**< This frame is the last subframe */
        A_MPDU_STATUS_DELIMITER_CRC_ERROR = 0x10, /**< Delimiter CRC error */
        A_MPDU_STATUS_DELIMITER_CRC_KNOWN =
            0x20 /**< Delimiter CRC value known: the delimiter CRC value field is valid */
    };

    /**
     * structure that contains the subfields of the A-MPDU status field.
     */
    struct AmpduStatusFields
    {
        uint32_t referenceNumber{
            0}; //!< A-MPDU reference number to identify all subframes belonging to the same A-MPDU
        uint16_t flags{A_MPDU_STATUS_NONE}; //!< flags field
        uint8_t crc{1};                     //!< CRC field
        uint8_t reserved{0};                //!< Reserved field
    };

    /**
     * @brief Set the subfields of the A-MPDU status field
     *
     * @param ampduStatusFields The subfields of the A-MPDU status field.
     */
    void SetAmpduStatus(const AmpduStatusFields& ampduStatusFields);

    /**
     * @brief VHT known bits.
     */
    enum VhtKnown : uint16_t
    {
        VHT_KNOWN_NONE = 0x0000, /**< No flags set */
        VHT_KNOWN_STBC = 0x0001, /**< Space-time block coding (1 if all spatial streams of all users
                                    have STBC, 0 otherwise). */
        VHT_KNOWN_TXOP_PS_NOT_ALLOWED = 0x0002,          /**< TXOP_PS_NOT_ALLOWED known */
        VHT_KNOWN_GUARD_INTERVAL = 0x0004,               /**< Guard interval */
        VHT_KNOWN_SHORT_GI_NSYM_DISAMBIGUATION = 0x0008, /**< Short GI NSYM disambiguation known */
        VHT_KNOWN_LDPC_EXTRA_OFDM_SYMBOL = 0x0010,       /**< LDPC extra OFDM symbol known */
        VHT_KNOWN_BEAMFORMED = 0x0020,  /**< Beamformed known/applicable (this flag should be set to
                                           zero for MU PPDUs). */
        VHT_KNOWN_BANDWIDTH = 0x0040,   /**< Bandwidth known */
        VHT_KNOWN_GROUP_ID = 0x0080,    /**< Group ID known */
        VHT_KNOWN_PARTIAL_AID = 0x0100, /**< Partial AID known/applicable */
    };

    /**
     * @brief VHT flags.
     */
    enum VhtFlags : uint8_t
    {
        VHT_FLAGS_NONE = 0x00, /**< No flags set */
        VHT_FLAGS_STBC =
            0x01, /**< Set if all spatial streams of all users have space-time block coding */
        VHT_FLAGS_TXOP_PS_NOT_ALLOWED =
            0x02, /**< Set if STAs may not doze during TXOP (valid only for AP transmitters). */
        VHT_FLAGS_GUARD_INTERVAL = 0x04, /**< Short guard interval */
        VHT_FLAGS_SHORT_GI_NSYM_DISAMBIGUATION =
            0x08, /**< Set if NSYM mod 10 = 9 (valid only if short GI is used).*/
        VHT_FLAGS_LDPC_EXTRA_OFDM_SYMBOL =
            0x10, /**< Set if one or more users are using LDPC and the encoding process resulted in
                     extra OFDM symbol(s) */
        VHT_FLAGS_BEAMFORMED = 0x20, /**< Set if beamforming is used (valid for SU PPDUs only). */
    };

    /**
     * structure that contains the subfields of the VHT field.
     */
    struct VhtFields
    {
        uint16_t known{VHT_KNOWN_NONE};  //!< known flags field
        uint8_t flags{VHT_FLAGS_NONE};   //!< flags field
        uint8_t bandwidth{0};            //!< bandwidth field
        std::array<uint8_t, 4> mcsNss{}; //!< mcs_nss field
        uint8_t coding{0};               //!< coding field
        uint8_t groupId{0};              //!< group_id field
        uint16_t partialAid{0};          //!< partial_aid field
    };

    /**
     * @brief Set the subfields of the VHT field
     *
     * @param vhtFields The subfields of the VHT field.
     */
    void SetVhtFields(const VhtFields& vhtFields);

    /**
     * @brief bits of the HE data fields.
     */
    enum HeData : uint16_t
    {
        /* Data 1 */
        HE_DATA1_FORMAT_EXT_SU = 0x0001,      /**< HE EXT SU PPDU format */
        HE_DATA1_FORMAT_MU = 0x0002,          /**< HE MU PPDU format */
        HE_DATA1_FORMAT_TRIG = 0x0003,        /**< HE TRIG PPDU format */
        HE_DATA1_BSS_COLOR_KNOWN = 0x0004,    /**< BSS Color known */
        HE_DATA1_BEAM_CHANGE_KNOWN = 0x0008,  /**< Beam Change known */
        HE_DATA1_UL_DL_KNOWN = 0x0010,        /**< UL/DL known */
        HE_DATA1_DATA_MCS_KNOWN = 0x0020,     /**< data MCS known */
        HE_DATA1_DATA_DCM_KNOWN = 0x0040,     /**< data DCM known */
        HE_DATA1_CODING_KNOWN = 0x0080,       /**< Coding known */
        HE_DATA1_LDPC_XSYMSEG_KNOWN = 0x0100, /**< LDPC extra symbol segment known */
        HE_DATA1_STBC_KNOWN = 0x0200,         /**< STBC known */
        HE_DATA1_SPTL_REUSE_KNOWN =
            0x0400, /**< Spatial Reuse known (Spatial Reuse 1 for HE TRIG PPDU format) */
        HE_DATA1_SPTL_REUSE2_KNOWN = 0x0800, /**< Spatial Reuse 2 known (HE TRIG PPDU format),
                                                STA-ID known (HE MU PPDU format) */
        HE_DATA1_SPTL_REUSE3_KNOWN = 0x1000, /**< Spatial Reuse 3 known (HE TRIG PPDU format) */
        HE_DATA1_SPTL_REUSE4_KNOWN = 0x2000, /**< Spatial Reuse 4 known (HE TRIG PPDU format) */
        HE_DATA1_BW_RU_ALLOC_KNOWN = 0x4000, /**< data BW/RU allocation known */
        HE_DATA1_DOPPLER_KNOWN = 0x8000,     /**< Doppler known */
        /* Data 2 */
        HE_DATA2_PRISEC_80_KNOWN = 0x0001,    /**< pri/sec 80 MHz known */
        HE_DATA2_GI_KNOWN = 0x0002,           /**< GI known */
        HE_DATA2_NUM_LTF_SYMS_KNOWN = 0x0004, /**< number of LTF symbols known */
        HE_DATA2_PRE_FEC_PAD_KNOWN = 0x0008,  /**< Pre-FEC Padding Factor known */
        HE_DATA2_TXBF_KNOWN = 0x0010,         /**< TxBF known */
        HE_DATA2_PE_DISAMBIG_KNOWN = 0x0020,  /**< PE Disambiguity known */
        HE_DATA2_TXOP_KNOWN = 0x0040,         /**< TXOP known */
        HE_DATA2_MIDAMBLE_KNOWN = 0x0080,     /**< midamble periodicity known */
        HE_DATA2_RU_OFFSET = 0x3f00,          /**< RU allocation offset */
        HE_DATA2_RU_OFFSET_KNOWN = 0x4000,    /**< RU allocation offset known */
        HE_DATA2_PRISEC_80_SEC = 0x8000,      /**< pri/sec 80 MHz */
        /* Data 3 */
        HE_DATA3_BSS_COLOR = 0x003f,
        HE_DATA3_BEAM_CHANGE = 0x0040,
        HE_DATA3_UL_DL = 0x0080,
        HE_DATA3_DATA_MCS = 0x0f00,
        HE_DATA3_DATA_DCM = 0x1000,
        HE_DATA3_CODING = 0x2000,
        HE_DATA3_LDPC_XSYMSEG = 0x4000,
        HE_DATA3_STBC = 0x8000,
        /* Data 4 */
        HE_DATA4_SU_MU_SPTL_REUSE = 0x000f,
        HE_DATA4_MU_STA_ID = 0x7ff0,
        HE_DATA4_TB_SPTL_REUSE1 = 0x000f,
        HE_DATA4_TB_SPTL_REUSE2 = 0x00f0,
        HE_DATA4_TB_SPTL_REUSE3 = 0x0f00,
        HE_DATA4_TB_SPTL_REUSE4 = 0xf000,
        /* Data 5 */
        HE_DATA5_DATA_BW_RU_ALLOC_40MHZ = 0x0001,  /**< 40 MHz data Bandwidth */
        HE_DATA5_DATA_BW_RU_ALLOC_80MHZ = 0x0002,  /**< 80 MHz data Bandwidth */
        HE_DATA5_DATA_BW_RU_ALLOC_160MHZ = 0x0003, /**< 160 MHz data Bandwidth */
        HE_DATA5_DATA_BW_RU_ALLOC_26T = 0x0004,    /**< 26-tone RU allocation */
        HE_DATA5_DATA_BW_RU_ALLOC_52T = 0x0005,    /**< 52-tone RU allocation */
        HE_DATA5_DATA_BW_RU_ALLOC_106T = 0x0006,   /**< 106-tone RU allocation */
        HE_DATA5_DATA_BW_RU_ALLOC_242T = 0x0007,   /**< 242-tone RU allocation */
        HE_DATA5_DATA_BW_RU_ALLOC_484T = 0x0008,   /**< 484-tone RU allocation */
        HE_DATA5_DATA_BW_RU_ALLOC_996T = 0x0009,   /**< 996-tone RU allocation */
        HE_DATA5_DATA_BW_RU_ALLOC_2x996T = 0x000a, /**< 2x996-tone RU allocation */
        HE_DATA5_GI_1_6 = 0x0010,                  /**< 1.6us GI */
        HE_DATA5_GI_3_2 = 0x0020,                  /**< 3.2us GI */
        HE_DATA5_LTF_SYM_SIZE = 0x00c0,            /**< LTF symbol size */
        HE_DATA5_NUM_LTF_SYMS = 0x0700,            /**< number of LTF symbols */
        HE_DATA5_PRE_FEC_PAD = 0x3000,             /**< Pre-FEC Padding Factor */
        HE_DATA5_TXBF = 0x4000,                    /**< TxBF */
        HE_DATA5_PE_DISAMBIG = 0x8000,             /**< PE Disambiguity */
    };

    /**
     * structure that contains the subfields of the HE field.
     */
    struct HeFields
    {
        uint16_t data1{0}; //!< data1 field
        uint16_t data2{0}; //!< data2 field
        uint16_t data3{0}; //!< data3 field
        uint16_t data4{0}; //!< data4 field
        uint16_t data5{0}; //!< data5 field
        uint16_t data6{0}; //!< data6 field
    };

    /**
     * @brief Set the subfields of the HE field
     *
     * @param heFields The subfields of the HE field.
     */
    void SetHeFields(const HeFields& heFields);

    /**
     * @brief HE MU flags1.
     */
    enum HeMuFlags1 : uint16_t
    {
        HE_MU_FLAGS1_SIGB_MCS = 0x000f,                //!< SIG-B MCS (from SIG-A)
        HE_MU_FLAGS1_SIGB_MCS_KNOWN = 0x0010,          //!< SIG-B MCS known
        HE_MU_FLAGS1_SIGB_DCM = 0x0020,                //!< SIG-B DCM (from SIG-A)
        HE_MU_FLAGS1_SIGB_DCM_KNOWN = 0x0040,          //!< SIG-B DCM known
        HE_MU_FLAGS1_CH2_CENTER_26T_RU_KNOWN = 0x0080, //!< (Channel 2) Center 26-tone RU bit known
        HE_MU_FLAGS1_CH1_RUS_KNOWN = 0x0100, //!< Channel 1 RUs known (which depends on BW)
        HE_MU_FLAGS1_CH2_RUS_KNOWN = 0x0200, //!< Channel 2 RUs known (which depends on BW)
        HE_MU_FLAGS1_CH1_CENTER_26T_RU_KNOWN = 0x1000, //!< (Channel 1) Center 26-tone RU bit known
        HE_MU_FLAGS1_CH1_CENTER_26T_RU = 0x2000,       //!< (Channel 1) Center 26-tone RU value
        HE_MU_FLAGS1_SIGB_COMPRESSION_KNOWN = 0x4000,  //!< SIG-B Compression known
        HE_MU_FLAGS1_NUM_SIGB_SYMBOLS_KNOWN = 0x8000, //!< # of HE-SIG-B Symbols/MU-MIMO Users known
    };

    /**
     * @brief HE MU flags2.
     */
    enum HeMuFlags2 : uint16_t
    {
        HE_MU_FLAGS2_BW_FROM_SIGA = 0x0003, /**< Bandwidth from Bandwidth field in HE-SIG-A */
        HE_MU_FLAGS2_BW_FROM_SIGA_KNOWN =
            0x0004, /**< Bandwidth from Bandwidth field in HE-SIG-A known */
        HE_MU_FLAGS2_SIGB_COMPRESSION_FROM_SIGA = 0x0008, /**< SIG-B compression from SIG-A */
        HE_MU_FLAGS2_NUM_SIGB_SYMBOLS_FROM_SIGA =
            0x00f0, /**< # of HE-SIG-B Symbols - 1 or # of MU-MIMO Users - 1 from SIG-A */
        HE_MU_FLAGS2_PREAMBLE_PUNCTURING_FROM_SIGA_BW_FIELD =
            0x0300, /**< Preamble puncturing from Bandwidth field in HE-SIG-A */
        HE_MU_FLAGS2_PREAMBLE_PUNCTURING_FROM_SIGA_BW_FIELD_KNOWN =
            0x0400, /**< Preamble puncturing from Bandwidth field in HE-SIG-A known */
        HE_MU_FLAGS2_CH2_CENTER_26T_RU = 0x0800, /**< (Channel 2) Center 26-tone RU value */
    };

    /**
     * structure that contains the subfields of the HE-MU field.
     */
    struct HeMuFields
    {
        uint16_t flags1{0};                  //!< flags1 field
        uint16_t flags2{0};                  //!< flags2 field
        std::array<uint8_t, 4> ruChannel1{}; //!< RU_channel1 field
        std::array<uint8_t, 4> ruChannel2{}; //!< RU_channel2 field
    };

    /**
     * @brief Set the subfields of the HE-MU field
     *
     * @param heMuFields The subfields of the HE-MU field.
     */
    void SetHeMuFields(const HeMuFields& heMuFields);

    /**
     * @brief HE MU per_user_known.
     */
    enum HeMuPerUserKnown : uint8_t
    {
        HE_MU_PER_USER_POSITION_KNOWN = 0x01,              //!< User field position known
        HE_MU_PER_USER_STA_ID_KNOWN = 0x02,                //!< STA-ID known
        HE_MU_PER_USER_NSTS_KNOWN = 0x04,                  //!< NSTS known
        HE_MU_PER_USER_TX_BF_KNOWN = 0x08,                 //!< Tx Beamforming known
        HE_MU_PER_USER_SPATIAL_CONFIGURATION_KNOWN = 0x10, //!< Spatial Configuration known
        HE_MU_PER_USER_MCS_KNOWN = 0x20,                   //!< MCS known
        HE_MU_PER_USER_DCM_KNOWN = 0x40,                   //!< DCM known
        HE_MU_PER_USER_CODING_KNOWN = 0x80,                //!< Coding known
    };

    /**
     * structure that contains the subfields of the HE-MU-other-user field.
     */
    struct HeMuOtherUserFields
    {
        uint16_t perUser1{0};       //!< per_user_1 field
        uint16_t perUser2{0};       //!< per_user_2 field
        uint8_t perUserPosition{0}; //!< per_user_position field
        uint8_t perUserKnown{0};    //!< per_user_known field
    };

    /**
     * @brief Set the subfields of the HE-MU-other-user field
     *
     * @param heMuOtherUserFields The subfields of the HE-MU-other-user field.
     */
    void SetHeMuOtherUserFields(const HeMuOtherUserFields& heMuOtherUserFields);

    /**
     * structure that contains the subfields of the TLV fields.
     */
    struct TlvFields
    {
        uint16_t type{0};   //!< type field.
        uint16_t length{0}; //!< length field.
    };

    /**
     * structure that contains the subfields of the U-SIG field.
     */
    struct UsigFields
    {
        uint32_t common{0}; //!< common field.
        uint32_t value{0};  //!< value field.
        uint32_t mask{0};   //!< mask field.
    };

    /**
     * @brief U-SIG common subfield.
     */
    enum UsigCommon : uint32_t
    {
        USIG_COMMON_PHY_VER_KNOWN = 0x00000001,
        USIG_COMMON_BW_KNOWN = 0x00000002,
        USIG_COMMON_UL_DL_KNOWN = 0x00000004,
        USIG_COMMON_BSS_COLOR_KNOWN = 0x00000008,
        USIG_COMMON_TXOP_KNOWN = 0x00000010,
        USIG_COMMON_BAD_USIG_CRC = 0x00000020,
        USIG_COMMON_VALIDATE_BITS_CHECKED = 0x00000040,
        USIG_COMMON_VALIDATE_BITS_OK = 0x00000080,
        USIG_COMMON_PHY_VER = 0x00007000,
        USIG_COMMON_BW = 0x00038000,
        USIG_COMMON_UL_DL = 0x00040000,
        USIG_COMMON_BSS_COLOR = 0x01f80000,
        USIG_COMMON_TXOP = 0xfe000000,
    };

    /**
     * @brief Possible BW values in U-SIG common subfield.
     */
    enum UsigCommonBw : uint8_t
    {
        USIG_COMMON_BW_20MHZ = 0,
        USIG_COMMON_BW_40MHZ = 1,
        USIG_COMMON_BW_80MHZ = 2,
        USIG_COMMON_BW_160MHZ = 3,
        USIG_COMMON_BW_320MHZ_1 = 4,
        USIG_COMMON_BW_320MHZ_2 = 5,
    };

    /**
     * @brief EHT MU PPDU U-SIG contents.
     */
    enum UsigMu : uint32_t
    {
        /* MU-USIG-1 */
        USIG1_MU_B20_B24_DISREGARD = 0x0000001f,
        USIG1_MU_B25_VALIDATE = 0x00000020,
        /* MU-USIG-2 */
        USIG2_MU_B0_B1_PPDU_TYPE = 0x000000c0,
        USIG2_MU_B2_VALIDATE = 0x00000100,
        USIG2_MU_B3_B7_PUNCTURED_INFO = 0x00003e00,
        USIG2_MU_B8_VALIDATE = 0x00004000,
        USIG2_MU_B9_B10_SIG_MCS = 0x00018000,
        USIG2_MU_B11_B15_EHT_SYMBOLS = 0x003e0000,
        USIG2_MU_B16_B19_CRC = 0x03c00000,
        USIG2_MU_B20_B25_TAIL = 0xfc000000,
    };

    /**
     * @brief EHT TB PPDU U-SIG contents.
     */
    enum UsigTb : uint32_t
    {
        /* TB-USIG-1 */
        USIG1_TB_B20_B25_DISREGARD = 0x0000001f,
        /* TB-USIG-2 */
        USIG2_TB_B0_B1_PPDU_TYPE = 0x000000c0,
        USIG2_TB_B2_VALIDATE = 0x00000100,
        USIG2_TB_B3_B6_SPATIAL_REUSE_1 = 0x00001e00,
        USIG2_TB_B7_B10_SPATIAL_REUSE_2 = 0x0001e000,
        USIG2_TB_B11_B15_DISREGARD = 0x003e0000,
        USIG2_TB_B16_B19_CRC = 0x03c00000,
        USIG2_TB_B20_B25_TAIL = 0xfc000000,
    };

    /**
     * @brief Set the subfields of the U-SIG field
     *
     * @param usigFields The subfields of the U-SIG field.
     */
    void SetUsigFields(const UsigFields& usigFields);

    /**
     * structure that contains the subfields of the EHT field.
     */
    struct EhtFields
    {
        uint32_t known{0};                //!< known field.
        std::array<uint32_t, 9> data{};   //!< data fields.
        std::vector<uint32_t> userInfo{}; //!< user info fields.
    };

    /**
     * @brief EHT known subfield.
     */
    enum EhtKnown : uint32_t
    {
        EHT_KNOWN_SPATIAL_REUSE = 0x00000002,
        EHT_KNOWN_GI = 0x00000004,
        EHT_KNOWN_EHT_LTF = 0x00000010,
        EHT_KNOWN_LDPC_EXTRA_SYM_OM = 0x00000020,
        EHT_KNOWN_PRE_PADD_FACOR_OM = 0x00000040,
        EHT_KNOWN_PE_DISAMBIGUITY_OM = 0x00000080,
        EHT_KNOWN_DISREGARD_O = 0x00000100,
        EHT_KNOWN_DISREGARD_S = 0x00000200,
        EHT_KNOWN_CRC1 = 0x00002000,
        EHT_KNOWN_TAIL1 = 0x00004000,
        EHT_KNOWN_CRC2_O = 0x00008000,
        EHT_KNOWN_TAIL2_O = 0x00010000,
        EHT_KNOWN_NSS_S = 0x00020000,
        EHT_KNOWN_BEAMFORMED_S = 0x00040000,
        EHT_KNOWN_NR_NON_OFDMA_USERS_M = 0x00080000,
        EHT_KNOWN_ENCODING_BLOCK_CRC_M = 0x00100000,
        EHT_KNOWN_ENCODING_BLOCK_TAIL_M = 0x00200000,
        EHT_KNOWN_RU_MRU_SIZE_OM = 0x00400000,
        EHT_KNOWN_RU_MRU_INDEX_OM = 0x00800000,
        EHT_KNOWN_RU_ALLOC_TB_OM = 0x01000000,
        EHT_KNOWN_PRIMARY_80 = 0x02000000,
    };

    /**
     * @brief EHT data subfield.
     */
    enum EhtData : uint32_t
    {
        /* Data 0 */
        EHT_DATA0_SPATIAL_REUSE = 0x00000078,
        EHT_DATA0_GI = 0x00000180,
        EHT_DATA0_LTF = 0x00000600,
        EHT_DATA0_EHT_LTF = 0x00003800,
        EHT_DATA0_LDPC_EXTRA_SYM_OM = 0x00004000,
        EHT_DATA0_PRE_PADD_FACOR_OM = 0x00018000,
        EHT_DATA0_PE_DISAMBIGUITY_OM = 0x00020000,
        EHT_DATA0_DISREGARD_S = 0x000c0000,
        EHT_DATA0_DISREGARD_O = 0x003c0000,
        EHT_DATA0_CRC1_O = 0x03c00000,
        EHT_DATA0_TAIL1_O = 0xfc000000,
        /* Data 1 */
        EHT_DATA1_RU_MRU_SIZE = 0x0000001f,
        EHT_DATA1_RU_MRU_INDEX = 0x00001fe0,
        EHT_DATA1_RU_ALLOC_CC_1_1_1 = 0x003fe000,
        EHT_DATA1_RU_ALLOC_CC_1_1_1_KNOWN = 0x00400000,
        EHT_DATA1_PRIMARY_80 = 0xc0000000,
        /* Data 2 */
        EHT_DATA2_RU_ALLOC_CC_2_1_1 = 0x000001ff,
        EHT_DATA2_RU_ALLOC_CC_2_1_1_KNOWN = 0x00000200,
        EHT_DATA2_RU_ALLOC_CC_1_1_2 = 0x0007fc00,
        EHT_DATA2_RU_ALLOC_CC_1_1_2_KNOWN = 0x00080000,
        EHT_DATA2_RU_ALLOC_CC_2_1_2 = 0x1ff00000,
        EHT_DATA2_RU_ALLOC_CC_2_1_2_KNOWN = 0x20000000,
        /* Data 3 */
        EHT_DATA3_RU_ALLOC_CC_1_2_1 = 0x000001ff,
        EHT_DATA3_RU_ALLOC_CC_1_2_1_KNOWN = 0x00000200,
        EHT_DATA3_RU_ALLOC_CC_2_2_1 = 0x0007fc00,
        EHT_DATA3_RU_ALLOC_CC_2_2_1_KNOWN = 0x00080000,
        EHT_DATA3_RU_ALLOC_CC_1_2_2 = 0x1ff00000,
        EHT_DATA3_RU_ALLOC_CC_1_2_2_KNOWN = 0x20000000,
        /* Data 4 */
        EHT_DATA4_RU_ALLOC_CC_2_2_2 = 0x000001ff,
        EHT_DATA4_RU_ALLOC_CC_2_2_2_KNOWN = 0x00000200,
        EHT_DATA4_RU_ALLOC_CC_1_2_3 = 0x0007fc00,
        EHT_DATA4_RU_ALLOC_CC_1_2_3_KNOWN = 0x00080000,
        EHT_DATA4_RU_ALLOC_CC_2_2_3 = 0x1ff00000,
        EHT_DATA4_RU_ALLOC_CC_2_2_3_KNOWN = 0x20000000,
        /* Data 5 */
        EHT_DATA5_RU_ALLOC_CC_1_2_4 = 0x000001ff,
        EHT_DATA5_RU_ALLOC_CC_1_2_4_KNOWN = 0x00000200,
        EHT_DATA5_RU_ALLOC_CC_2_2_4 = 0x0007fc00,
        EHT_DATA5_RU_ALLOC_CC_2_2_4_KNOWN = 0x00080000,
        EHT_DATA5_RU_ALLOC_CC_1_2_5 = 0x1ff00000,
        EHT_DATA5_RU_ALLOC_CC_1_2_5_KNOWN = 0x20000000,
        /* Data 6 */
        EHT_DATA6_RU_ALLOC_CC_2_2_5 = 0x000001ff,
        EHT_DATA6_RU_ALLOC_CC_2_2_5_KNOWN = 0x00000200,
        EHT_DATA6_RU_ALLOC_CC_1_2_6 = 0x0007fc00,
        EHT_DATA6_RU_ALLOC_CC_1_2_6_KNOWN = 0x00080000,
        EHT_DATA6_RU_ALLOC_CC_2_2_6 = 0x1ff00000,
        EHT_DATA6_RU_ALLOC_CC_2_2_6_KNOWN = 0x20000000,
        /* Data 7 */
        EHT_DATA7_CRC2_O = 0x0000000f,
        EHT_DATA7_TAIL_2_O = 0x000003f0,
        EHT_DATA7_NSS_S = 0x0000f000,
        EHT_DATA7_BEAMFORMED_S = 0x00010000,
        EHT_DATA7_NUM_OF_NON_OFDMA_USERS = 0x000e0000,
        EHT_DATA7_USER_ENCODING_BLOCK_CRC = 0x00f00000,
        EHT_DATA7_USER_ENCODING_BLOCK_TAIL = 0x3f000000,
        /* Data 8 */
        EHT_DATA8_RU_ALLOC_TB_FMT_PS_160 = 0x00000001,
        EHT_DATA8_RU_ALLOC_TB_FMT_B0 = 0x00000002,
        EHT_DATA8_RU_ALLOC_TB_FMT_B7_B1 = 0x000001fc,
    };

    /**
     * @brief Possible GI values in EHT data subfield.
     */
    enum EhtData0Gi : uint8_t
    {
        EHT_DATA0_GI_800_NS = 0,
        EHT_DATA0_GI_1600_NS = 1,
        EHT_DATA0_GI_3200_NS = 2,
    };

    /**
     * @brief Possible Primary 80 MHz Channel Position values in EHT data subfield.
     */
    enum EhtData1Primary80 : uint8_t
    {
        EHT_DATA1_PRIMARY_80_LOWEST = 0,
        EHT_DATA1_PRIMARY_80_HIGHEST = 3,
    };

    /**
     * @brief Possible RU/MRU Size values in EHT data subfield.
     */
    enum EhtData1RuSize : uint8_t
    {
        EHT_DATA1_RU_MRU_SIZE_26 = 0,
        EHT_DATA1_RU_MRU_SIZE_52 = 1,
        EHT_DATA1_RU_MRU_SIZE_106 = 2,
        EHT_DATA1_RU_MRU_SIZE_242 = 3,
        EHT_DATA1_RU_MRU_SIZE_484 = 4,
        EHT_DATA1_RU_MRU_SIZE_996 = 5,
        EHT_DATA1_RU_MRU_SIZE_2x996 = 6,
        EHT_DATA1_RU_MRU_SIZE_4x996 = 7,
        EHT_DATA1_RU_MRU_SIZE_52_26 = 8,
        EHT_DATA1_RU_MRU_SIZE_106_26 = 9,
        EHT_DATA1_RU_MRU_SIZE_484_242 = 10,
        EHT_DATA1_RU_MRU_SIZE_996_484 = 11,
        EHT_DATA1_RU_MRU_SIZE_996_484_242 = 12,
        EHT_DATA1_RU_MRU_SIZE_2x996_484 = 13,
        EHT_DATA1_RU_MRU_SIZE_3x996 = 14,
        EHT_DATA1_RU_MRU_SIZE_3x996_484 = 15,
    };

    /**
     * @brief EHT user_info subfield.
     */
    enum EhtUserInfo : uint32_t
    {
        EHT_USER_INFO_STA_ID_KNOWN = 0x00000001,
        EHT_USER_INFO_MCS_KNOWN = 0x00000002,
        EHT_USER_INFO_CODING_KNOWN = 0x00000004,
        EHT_USER_INFO_NSS_KNOWN_O = 0x00000010,
        EHT_USER_INFO_BEAMFORMING_KNOWN_O = 0x00000020,
        EHT_USER_INFO_SPATIAL_CONFIG_KNOWN_M = 0x00000040,
        EHT_USER_INFO_DATA_FOR_USER = 0x00000080,
        EHT_USER_INFO_STA_ID = 0x0007ff00,
        EHT_USER_INFO_CODING = 0x00080000,
        EHT_USER_INFO_MCS = 0x00f00000,
        EHT_USER_INFO_NSS_O = 0x0f000000,
        EHT_USER_INFO_BEAMFORMING_O = 0x20000000,
        EHT_USER_INFO_SPATIAL_CONFIG_M = 0x3f000000,
    };

    /**
     * @brief Set the subfields of the EHT-SIG field
     *
     * @param ehtFields The subfields of the EHT-SIG field.
     */
    void SetEhtFields(const EhtFields& ehtFields);

  private:
    /**
     * Serialize the Channel radiotap header.
     *
     * @param start An iterator which points to where the header should be written.
     */
    void SerializeChannel(Buffer::Iterator& start) const;

    /**
     * Deserialize the Channel radiotap header.
     *
     * @param start An iterator which points to where the header should be read.
     * @param bytesRead the number of bytes already read.

     * @returns The number of bytes read.
     */
    uint32_t DeserializeChannel(Buffer::Iterator start, uint32_t bytesRead);

    /**
     * Add Channel subfield/value pairs to the output stream.
     *
     * @param os The output stream
     */
    void PrintChannel(std::ostream& os) const;

    /**
     * Serialize the MCS radiotap header.
     *
     * @param start An iterator which points to where the header should be written.
     */
    void SerializeMcs(Buffer::Iterator& start) const;

    /**
     * Deserialize the MCS radiotap header.
     *
     * @param start An iterator which points to where the header should be read.
     * @param bytesRead the number of bytes already read.

     * @returns The number of bytes read.
     */
    uint32_t DeserializeMcs(Buffer::Iterator start, uint32_t bytesRead);

    /**
     * Add MCS subfield/value pairs to the output stream.
     *
     * @param os The output stream
     */
    void PrintMcs(std::ostream& os) const;

    /**
     * Serialize the A-MPDU Status radiotap header.
     *
     * @param start An iterator which points to where the header should be written.
     */
    void SerializeAmpduStatus(Buffer::Iterator& start) const;

    /**
     * Deserialize the A-MPDU Status radiotap header.
     *
     * @param start An iterator which points to where the header should be read.
     * @param bytesRead the number of bytes already read.

     * @returns The number of bytes read.
     */
    uint32_t DeserializeAmpduStatus(Buffer::Iterator start, uint32_t bytesRead);

    /**
     * Add A-MPDU Status subfield/value pairs to the output stream.
     *
     * @param os The output stream
     */
    void PrintAmpduStatus(std::ostream& os) const;

    /**
     * Serialize the VHT radiotap header.
     *
     * @param start An iterator which points to where the header should be written.
     */
    void SerializeVht(Buffer::Iterator& start) const;

    /**
     * Deserialize the VHT radiotap header.
     *
     * @param start An iterator which points to where the header should be read.
     * @param bytesRead the number of bytes already read.

     * @returns The number of bytes read.
     */
    uint32_t DeserializeVht(Buffer::Iterator start, uint32_t bytesRead);

    /**
     * Add VHT subfield/value pairs to the output stream.
     *
     * @param os The output stream
     */
    void PrintVht(std::ostream& os) const;

    /**
     * Serialize the HE radiotap header.
     *
     * @param start An iterator which points to where the header should be written.
     */
    void SerializeHe(Buffer::Iterator& start) const;

    /**
     * Deserialize the HE radiotap header.
     *
     * @param start An iterator which points to where the header should be read.
     * @param bytesRead the number of bytes already read.

     * @returns The number of bytes read.
     */
    uint32_t DeserializeHe(Buffer::Iterator start, uint32_t bytesRead);

    /**
     * Add HE subfield/value pairs to the output stream.
     *
     * @param os The output stream
     */
    void PrintHe(std::ostream& os) const;

    /**
     * Serialize the HE-MU radiotap header.
     *
     * @param start An iterator which points to where the header should be written.
     */
    void SerializeHeMu(Buffer::Iterator& start) const;

    /**
     * Deserialize the HE-MU radiotap header.
     *
     * @param start An iterator which points to where the header should be read.
     * @param bytesRead the number of bytes already read.

     * @returns The number of bytes read.
     */
    uint32_t DeserializeHeMu(Buffer::Iterator start, uint32_t bytesRead);

    /**
     * Add HE-MU subfield/value pairs to the output stream.
     *
     * @param os The output stream
     */
    void PrintHeMu(std::ostream& os) const;

    /**
     * Serialize the HE-MU-other-user radiotap header.
     *
     * @param start An iterator which points to where the header should be written.
     */
    void SerializeHeMuOtherUser(Buffer::Iterator& start) const;

    /**
     * Deserialize the HE-MU-other-user radiotap header.
     *
     * @param start An iterator which points to where the header should be read.
     * @param bytesRead the number of bytes already read.

     * @returns The number of bytes read.
     */
    uint32_t DeserializeHeMuOtherUser(Buffer::Iterator start, uint32_t bytesRead);

    /**
     * Add HE-MU-other-user subfield/value pairs to the output stream.
     *
     * @param os The output stream
     */
    void PrintHeMuOtherUser(std::ostream& os) const;

    /**
     * Serialize the U-SIG radiotap header.
     *
     * @param start An iterator which points to where the header should be written.
     */
    void SerializeUsig(Buffer::Iterator& start) const;

    /**
     * Deserialize the U-SIG radiotap header.
     *
     * @param start An iterator which points to where the header should be read.
     * @param bytesRead the number of bytes already read.

     * @returns The number of bytes read.
     */
    uint32_t DeserializeUsig(Buffer::Iterator start, uint32_t bytesRead);

    /**
     * Add U-SIG subfield/value pairs to the output stream.
     *
     * @param os The output stream
     */
    void PrintUsig(std::ostream& os) const;

    /**
     * Serialize the EHT radiotap header.
     *
     * @param start An iterator which points to where the header should be written.
     */
    void SerializeEht(Buffer::Iterator& start) const;

    /**
     * Deserialize the EHT radiotap header.
     *
     * @param start An iterator which points to where the header should be read.
     * @param bytesRead the number of bytes already read.

     * @returns The number of bytes read.
     */
    uint32_t DeserializeEht(Buffer::Iterator start, uint32_t bytesRead);

    /**
     * Add EHT subfield/value pairs to the output stream.
     *
     * @param os The output stream
     */
    void PrintEht(std::ostream& os) const;

    /**
     * @brief Radiotap flags.
     */
    enum RadiotapFlags : uint32_t
    {
        RADIOTAP_TSFT = 0x00000001,
        RADIOTAP_FLAGS = 0x00000002,
        RADIOTAP_RATE = 0x00000004,
        RADIOTAP_CHANNEL = 0x00000008,
        RADIOTAP_FHSS = 0x00000010,
        RADIOTAP_DBM_ANTSIGNAL = 0x00000020,
        RADIOTAP_DBM_ANTNOISE = 0x00000040,
        RADIOTAP_LOCK_QUALITY = 0x00000080,
        RADIOTAP_TX_ATTENUATION = 0x00000100,
        RADIOTAP_DB_TX_ATTENUATION = 0x00000200,
        RADIOTAP_DBM_TX_POWER = 0x00000400,
        RADIOTAP_ANTENNA = 0x00000800,
        RADIOTAP_DB_ANTSIGNAL = 0x00001000,
        RADIOTAP_DB_ANTNOISE = 0x00002000,
        RADIOTAP_RX_FLAGS = 0x00004000,
        RADIOTAP_MCS = 0x00080000,
        RADIOTAP_AMPDU_STATUS = 0x00100000,
        RADIOTAP_VHT = 0x00200000,
        RADIOTAP_HE = 0x00800000,
        RADIOTAP_HE_MU = 0x01000000,
        RADIOTAP_HE_MU_OTHER_USER = 0x02000000,
        RADIOTAP_ZERO_LEN_PSDU = 0x04000000,
        RADIOTAP_LSIG = 0x08000000,
        RADIOTAP_TLV = 0x10000000,
        RADIOTAP_EXT = 0x80000000
    };

    /**
     * @brief Radiotap extended flags.
     */
    enum RadiotapExtFlags : uint32_t
    {
        RADIOTAP_S1G = 0x00000001,
        RADIOTAP_USIG = 0x00000002,
        RADIOTAP_EHT_SIG = 0x00000004
    };

    uint16_t m_length{8};                   //!< entire length of radiotap data + header
    uint32_t m_present{0};                  //!< bits describing which fields follow header
    std::optional<uint32_t> m_presentExt{}; //!< optional extended present bitmask

    uint64_t m_tsft{0}; //!< Time Synchronization Function Timer (when the first bit of the MPDU
                        //!< arrived at the MAC)

    uint8_t m_flags{FRAME_FLAG_NONE}; //!< Properties of transmitted and received frames.

    uint8_t m_rate{0}; //!< TX/RX data rate in units of 500 kbps

    uint8_t m_channelPad{0};         //!< Channel padding.
    ChannelFields m_channelFields{}; //!< Channel fields.

    int8_t m_antennaSignal{
        0}; //!< RF signal power at the antenna, dB difference from an arbitrary, fixed reference.
    int8_t m_antennaNoise{
        0}; //!< RF noise power at the antenna, dB difference from an arbitrary, fixed reference.

    McsFields m_mcsFields{}; //!< MCS fields.

    uint8_t m_ampduStatusPad{0}; //!< A-MPDU Status Flags, padding before A-MPDU Status Field.
    AmpduStatusFields m_ampduStatusFields{}; //!< A-MPDU Status fields.

    uint8_t m_vhtPad{0};     //!< VHT padding.
    VhtFields m_vhtFields{}; //!< VHT fields.

    uint8_t m_hePad{0};    //!< HE padding.
    HeFields m_heFields{}; //!< HE fields.

    uint8_t m_heMuPad{0};      //!< HE MU padding.
    HeMuFields m_heMuFields{}; //!< HE MU fields.

    uint8_t m_heMuOtherUserPad{0};               //!< HE MU other user padding.
    HeMuOtherUserFields m_heMuOtherUserFields{}; //!< HE MU other user fields.

    uint8_t m_usigTlvPad{0};   //!< U-SIG TLV padding.
    TlvFields m_usigTlv{};     //!< U-SIG TLV fields.
    uint8_t m_usigPad{0};      //!< U-SIG padding.
    UsigFields m_usigFields{}; //!< U-SIG fields.

    uint8_t m_ehtTlvPad{0};  //!< EHT TLV padding.
    TlvFields m_ehtTlv{};    //!< EHT TLV fields.
    uint8_t m_ehtPad{0};     //!< EHT padding.
    EhtFields m_ehtFields{}; //!< EHT fields.
};

} // namespace ns3

#endif /*  RADIOTAP_HEADER_H */
