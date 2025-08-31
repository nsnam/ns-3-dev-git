/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_OPERATION_H
#define UHR_OPERATION_H

#include "ns3/wifi-information-element.h"
#include "ns3/wifi-opt-field.h"

#include <vector>

namespace ns3
{

/// Size in bytes of UHR Operation Parameters, IEEE 802.11bn D1.0 Figure 9-aa1
constexpr uint16_t WIFI_UHR_OP_PARAMS_SIZE_B = 2;
/// Size in bytes of Basic UHR-MCS and NSS Set, IEEE 802.11bn D1.0 Figure 9-aa1
constexpr uint16_t WIFI_UHR_BASIC_MCS_NSS_SET_SIZE_B = 4;
/// Size in bytes of DPS Operation Parameters when present, IEEE 802.11bn D1.0 Figure 9-aa1
constexpr uint16_t WIFI_UHR_DPS_PARAMS_SIZE_B = 4;
/// Size in bytes of NPCA Operation Parameters when present, IEEE 802.11bn D1.0 Figure 9-aa1
constexpr uint16_t WIFI_UHR_NPCA_PARAMS_SIZE_B = 4;
/// Size in bytes of P-EDCA Operation Parameters when present, IEEE 802.11bn D1.0 Figure 9-aa1
constexpr uint16_t WIFI_UHR_PEDCA_PARAMS_SIZE_B = 3;
/// Size in bytes of DBE Operation Parameters when present, IEEE 802.11bn D1.0 Figure 9-aa1
constexpr uint16_t WIFI_UHR_DBE_PARAMS_SIZE_B = 3;
/// Max MCS index, copied from IEEE 802.11be D2.0 Figure 9-1002ai
constexpr uint8_t WIFI_UHR_MAX_MCS_INDEX = 13;
/// Max NSS configurable, copied from 802.11be D2.0 Table 9-401m
constexpr uint8_t WIFI_UHR_MAX_NSS_CONFIGURABLE = 8;
/// Default max Tx/Rx NSS
constexpr uint8_t WIFI_DEFAULT_UHR_MAX_NSS = 1;
/// Size in bytes of NPCA Disabled Subchannel Bitmap, IEEE 802.11bn D1.0 Figure 9-aa3
constexpr uint16_t WIFI_NPCA_DISABLED_SUBCH_BM_SIZE_B = 2;

/**
 * @brief UHR Operation Information Element
 * @ingroup wifi
 *
 * This class serializes and deserializes
 * the UHR Operation Information Element
 */
class UhrOperation : public WifiInformationElement
{
  public:
    /**
     * UHR Operation Parameters subfield
     * IEEE 802.11bn D1.0 Figure 9-aa2
     */
    struct UhrOpParams
    {
        bool dpsEnabled{
            false}; ///< DPS Enabled (do not set, it is set by the OptFieldWithPresenceInd)
        bool npcaEnabled{
            false}; ///< NPCA Enabled (do not set, it is set by the OptFieldWithPresenceInd)
        bool dbeEnabled{
            false}; ///< DBE Enabled (do not set, it is set by the OptFieldWithPresenceInd)
        bool pEdcaEnabled{
            false}; ///< P-EDCA Enabled (do not set, it is set by the OptFieldWithPresenceInd)

        /**
         * Serialize the UHR Operation Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the UHR Operation Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the UHR Operation Parameters subfield
         *
         * @return the serialized size in bytes of the UHR Operation Parameters subfield
         */
        uint16_t GetSerializedSize() const;
    };

    /**
     * Basic UHR-MCS and NSS Set subfield
     * TODO: not defined IEEE 802.11bn D1.0, currently copied from IEEE 802.11bn D2.0 Figure
     * 9-1002ai
     */
    struct UhrBasicMcsNssSet
    {
        std::vector<uint8_t> maxRxNss{}; ///< Max Rx NSS per MCS
        std::vector<uint8_t> maxTxNss{}; ///< Max Tx NSS per MCS

        /**
         * Serialize the Basic EHT-MCS and NSS Set subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the Basic EHT-MCS and NSS Set subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the Basic EHT-MCS and NSS Set subfield
         *
         * @return the serialized size in bytes of the Basic EHT-MCS and NSS Set subfield
         */
        uint16_t GetSerializedSize() const;
    };

    /**
     * UHR Operation Information subfield
     * TODO: not defined yet in IEEE 802.11bn D1.0
     */
    struct UhrOpInfo
    {
        /**
         * Serialize the UHR Operation Information subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the UHR Operation Information subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the UHR Operation Information subfield
         *
         * @return the serialized size in bytes of the UHR Operation Information subfield
         */
        uint16_t GetSerializedSize() const;
    };

    /**
     * DPS Operation Parameters subfield
     * TODO: not defined yet in IEEE 802.11bn D1.0
     */
    struct DpsOpParams
    {
        /**
         * Serialize the DPS Operation Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the DPS Operation Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the DPS Operation Parameters subfield
         *
         * @return the serialized size in bytes of the DPS Operation Parameters subfield
         */
        uint16_t GetSerializedSize() const;
    };

    /**
     * NPCA Operation Parameters subfield
     * IEEE 802.11bn D1.0 Figure 9-aa3
     */
    struct NpcaOpParams
    {
        uint8_t primaryChan : 4 {0};             ///< NPCA Primary Channel
        uint8_t minDurationThresh : 4 {0};       ///< NPCA Minimum Duration Threshold
        uint8_t switchDelay : 6 {0};             ///< NPCA Switch Delay
        uint8_t switchBackDelay : 6 {0};         ///< NPCA Switch Back Delay
        uint8_t initialQsrc : 2 {0};             ///< Initial NPCA QSRC
        uint8_t moplen : 1 {0};                  ///< MOPLEN NPCA
        uint8_t disSubchanBmPresent : 1 {0};     ///< NPCA Disabled Subchannel Bitmap Present
        std::optional<uint16_t> disabledSubchBm; ///< NPCA Disabled Subchannel Bitmap

        /**
         * Serialize the NPCA Operation Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the NPCA Operation Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the NPCA Operation Parameters subfield
         *
         * @return the serialized size in bytes of the NPCA Operation Parameters subfield
         */
        uint16_t GetSerializedSize() const;
    };

    /**
     * P-EDCA Operation Parameters subfield
     * IEEE 802.11bn D1.0 Figure 9-aa4
     */
    struct PEdcaOpParams
    {
        uint8_t eCWmin : 4 {0};        ///< P-EDCA ECWmin
        uint8_t eCWmax : 4 {0};        ///< P-EDCA ECWmax
        uint8_t aifsn : 4 {0};         ///< P-EDCA AIFSN
        uint8_t cwDs : 2 {0};          ///< CW DS
        uint8_t psrcThreshold : 3 {0}; ///< P-EDCA PSRC Threshold
        uint8_t qsrcThreshold : 3 {0}; ///< P-EDCA QSRC Threshold

        /**
         * Serialize the P-EDCA Operation Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the P-EDCA Operation Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the P-EDCA Operation Parameters subfield
         *
         * @return the serialized size in bytes of the P-EDCA Operation Parameters subfield
         */
        uint16_t GetSerializedSize() const;
    };

    /**
     * DBE Operation Parameters subfield
     * IEEE 802.11bn D1.0 Figure 9-aa5
     */
    struct DbeOpParams
    {
        uint8_t dBeBandwidth : 3 {0};            ///< DPE Bandwidth
        uint16_t dbeDisabledSubchannelBitmap{0}; ///< DBE Disabled Subchannel Bitmap

        /**
         * Serialize the DBE Operation Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the DBE Operation Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the DBE Operation Parameters subfield
         *
         * @return the serialized size in bytes of the DBE Operation Parameters subfield
         */
        uint16_t GetSerializedSize() const;
    };

    UhrOperation();
    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    void Print(std::ostream& os) const override;

    /**
     * Set the max Rx NSS for input MCS index range
     * @param maxNss the maximum supported Rx NSS for MCS group
     * @param mcsStart MCS index start
     * @param mcsEnd MCS index end
     */
    void SetMaxRxNss(uint8_t maxNss, uint8_t mcsStart, uint8_t mcsEnd);
    /**
     * Set the max Tx NSS for input MCS index range
     * @param maxNss the maximum supported Rx NSS for MCS group
     * @param mcsStart MCS index start
     * @param mcsEnd MCS index end
     */
    void SetMaxTxNss(uint8_t maxNss, uint8_t mcsStart, uint8_t mcsEnd);

    UhrOpParams m_params;          ///< UHR Operation Parameters
    UhrBasicMcsNssSet m_mcsNssSet; ///< Basic UHR-MCS and NSS set
    // TODO: use OptFieldWithPresenceInd instead of std::optional for m_opInfo once presence bit is
    // defined in a next 802.11bn draft
    std::optional<UhrOpInfo> m_opInfo;                    ///< UHR Operation Information
    OptFieldWithPresenceInd<DpsOpParams> m_dpsParams;     ///< DPS Operation Parameters
    OptFieldWithPresenceInd<NpcaOpParams> m_npcaParams;   ///< NPCA Operation Parameters
    OptFieldWithPresenceInd<PEdcaOpParams> m_pEdcaParams; ///< P-EDCA Operation Parameters
    OptFieldWithPresenceInd<DbeOpParams> m_dbeParams;     ///< DBE Operation Parameters

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* UHR_OPERATION_H */
