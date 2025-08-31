/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_MODE_CHANGE_H
#define UHR_MODE_CHANGE_H

#include "ns3/wifi-information-element.h"

#include <optional>
#include <variant>
#include <vector>

namespace ns3
{

/// Size in bytes of common part for Mode Tuple, IEEE 802.11bn D1.0 Figure 9-aa57
constexpr uint16_t WIFI_UHR_MODE_TUPLE_COMMON_SIZE_B = 1;
/// Size in bytes of common part for Mode Tuple when specific parameters are present, IEEE 802.11bn
/// D1.0 Figure 9-aa57
constexpr uint16_t WIFI_UHR_MODE_TUPLE_WITH_PARAMS_SIZE_B = 2;
/// Size in bytes of Mode Specific Parameters field for NPCA, IEEE 802.11bn D1.0 Figure 9-aa58
constexpr uint16_t WIFI_UHR_MODE_CHANGE_NPCA_PARAMS_SIZE_B = 2;
/// Size in bytes of Mode Specific Parameters field for EMLSR, IEEE 802.11bn D1.0 Figure 9-aa60
constexpr uint16_t WIFI_UHR_MODE_CHANGE_EMLSR_PARAMS_SIZE_B = 4;

/**
 * @brief UHR Mode Change Element
 * @ingroup wifi
 *
 * This class serializes and deserializes
 * the UHR Mode Change Element
 */
class UhrModeChange : public WifiInformationElement
{
  public:
    /**
     * Values for the Mode ID field
     * IEEE 802.11bn D1.0 Table 9-417g
     */
    enum class UhrModes : uint8_t
    {
        DPS = 0,
        NPCA = 1,
        DUO = 2,
        DSO = 3,
        P_EDCA = 4,
        ELR_RECEPTION = 5,
        AOM = 6,
        LLI = 7,
        CO_BF = 8,
        CO_SR = 9,
        EMLSR = 10,
        DBE = 11,
        MAX_MODE_ID = 12
    };

    /**
     * NPCA Mode Specific Parameters
     * IEEE 802.11bn D1.0 Figure 9-aa58
     */
    struct NpcaParams
    {
        uint8_t switchingDelay : 6 {0};  ///< NPCA Switching Delay
        uint8_t switchBackDelay : 6 {0}; ///< NPCA Switch Back Delay

        /**
         * Serialize the NPCA Mode Specific Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the NPCA Mode Specific Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the NPCA Mode Specific Parameters subfield
         *
         * @return the serialized size in bytes of the NPCA Mode Specific Parameters subfield
         */
        uint16_t GetSerializedSize() const;
    };

    /**
     * EMLSR Mode Specific Parameters
     * IEEE 802.11bn D1.0 Figure 9-aa60
     */
    struct EmlsrParams
    {
        uint16_t linkBm{0};              ///< EMLSR Link Bitmap
        uint8_t paddingDelay : 6 {0};    ///< EMLSR Padding Delay
        uint8_t transitionDelay : 6 {0}; ///< EMLSR Transition Delay
        uint8_t inDevCoexAct : 1 {0};    ///< In-Device Coexistence Activities

        /**
         * Serialize the EMLSR Mode Specific Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the EMLSR Mode Specific Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the EMLSR Mode Specific Parameters subfield
         *
         * @return the serialized size in bytes of the EMLSR Mode Specific Parameters subfield
         */
        uint16_t GetSerializedSize() const;
    };

    /// Mode Specific Parameters variant
    using ModeSpecificParams = std::variant<NpcaParams, EmlsrParams>;

    /**
     * Mode Tuple subfield
     * IEEE 802.11bn D1.0 Figure 9-aa57
     */
    struct ModeTuple
    {
        uint8_t modeId : 6 {static_cast<uint8_t>(UhrModes::MAX_MODE_ID)}; ///< Mode ID
        uint8_t modeEnable : 1 {0};                                       ///< Mode Enable
        uint8_t modeUpdate : 1 {0};                                       ///< Mode Update
        std::optional<uint8_t> modeLength;                                ///< Mode Length
        // TODO: Mode Specific Control not defined since it is currently reserved for all modes in
        // latest draft
        std::optional<ModeSpecificParams> modeParams; ///< Mode Specific Parameters

        /**
         * Serialize the Mode Tuple subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the Mode Tuple subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the Mode Tuple subfield
         *
         * @return the serialized size in bytes of the Mode Tuple subfield
         */
        uint16_t GetSerializedSize() const;
    };

    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    void Print(std::ostream& os) const override;

    std::vector<ModeTuple> m_modeTuples; ///< Mode Tuple fields

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* UHR_MODE_CHANGE_H */
