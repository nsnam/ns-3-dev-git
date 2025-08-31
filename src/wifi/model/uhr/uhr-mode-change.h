/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_MODE_CHANGE_H
#define UHR_MODE_CHANGE_H

#include "uhr-mode.h"

#include "ns3/wifi-export.h"
#include "ns3/wifi-information-element.h"

#include <optional>
#include <variant>
#include <vector>

namespace ns3
{

/// Size in bytes of Mode Specific Parameters field for DPS, IEEE 802.11bn D1.2 Figure 9-207u
constexpr uint16_t WIFI_UHR_MODE_CHANGE_DPS_PARAMS_SIZE_B = 4;
/// Size in bytes of Mode Specific Parameters field for NPCA, IEEE 802.11bn D1.0 Figure 9-aa58
constexpr uint16_t WIFI_UHR_MODE_CHANGE_NPCA_PARAMS_SIZE_B = 2;
/// Size in bytes of Mode Specific Parameters field for DSO, IEEE 802.11bn D1.2 Figure 9-aa59
constexpr uint16_t WIFI_UHR_MODE_CHANGE_DSO_PARAMS_SIZE_B = 2;
/// Size in bytes of Mode Specific Parameters field for EMLSR, IEEE 802.11bn D1.0 Figure 9-aa60
constexpr uint16_t WIFI_UHR_MODE_CHANGE_EMLSR_PARAMS_SIZE_B = 4;

/**
 * @brief UHR Mode Change Element
 * @ingroup wifi
 *
 * This class serializes and deserializes
 * the UHR Mode Change Element
 */
class WIFI_EXPORT UhrModeChange : public WifiInformationElement
{
  public:
    /**
     * DPS Mode Specific Parameters
     * IEEE 802.11bn D2.0 Figure 9-207u
     */
    struct DpsParams
    {
        uint8_t paddingDelay : 6 {0};         ///< DPS Padding Delay
        uint8_t transitionDelay : 6 {0};      ///< DPS Transition Delay
        uint8_t icfRequired : 1 {0};          ///< ICF Required
        uint8_t parameterizedMode : 1 {0};    ///< Parameterized Mode
        uint8_t lcModeBw : 3 {0};             ///< LC Mode Bandwidth
        uint8_t lcModeNss : 4 {0};            ///< LC Mode NSS
        uint8_t lcModeMcs : 4 {0};            ///< LC Mode MCS
        uint8_t mobileApDpsStaticHcm : 1 {0}; ///< Mobile AP DPS Static HCM

        /**
         * Equality operator.
         *
         * @return whether this object is equal to the given one
         */
        bool operator==(const DpsParams&) const = default;

        /**
         * Serialize the DPS Mode Specific Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the DPS Mode Specific Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the DPS Mode Specific Parameters subfield
         *
         * @return the serialized size in bytes of the DPS Mode Specific Parameters subfield
         */
        uint16_t GetSerializedSize() const;

        /**
         * Print the DPS Mode Specific Parameters subfield
         *
         * @param os the output stream to print to
         */
        void Print(std::ostream& os) const;
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
         * Equality operator.
         *
         * @return whether this object is equal to the given one
         */
        bool operator==(const NpcaParams&) const = default;

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

        /**
         * Print the NPCA Mode Specific Parameters subfield
         *
         * @param os the output stream to print to
         */
        void Print(std::ostream& os) const;
    };

    /**
     * DSO Mode Specific Parameters
     * IEEE 802.11bn D1.2 Figure 9-aa59
     */
    struct DsoParams
    {
        uint8_t paddingDelay : 6 {0};          ///< DSO Padding Delay
        uint8_t switchBackDelay : 6 {0};       ///< DSO Switch Back Delay
        uint8_t preferred80MHzSubband : 2 {3}; ///< Preferred 80 MHz DSO Subband

        /**
         * Equality operator.
         *
         * @return whether this object is equal to the given one
         */
        bool operator==(const DsoParams&) const = default;

        /**
         * Serialize the DSO Mode Specific Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the DSO Mode Specific Parameters subfield
         *
         * @param start iterator pointing to where the subfield should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the DSO Mode Specific Parameters subfield
         *
         * @return the serialized size in bytes of the DSO Mode Specific Parameters subfield
         */
        uint16_t GetSerializedSize() const;

        /**
         * Print the DSO Mode Specific Parameters subfield
         *
         * @param os the output stream to print to
         */
        void Print(std::ostream& os) const;
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
         * Equality operator.
         *
         * @return whether this object is equal to the given one
         */
        bool operator==(const EmlsrParams&) const = default;

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

        /**
         * Print the EMLSR Mode Specific Parameters subfield
         *
         * @param os the output stream to print to
         */
        void Print(std::ostream& os) const;
    };

    /// Mode Specific Parameters variant
    using ModeSpecificParams = std::variant<DpsParams, NpcaParams, DsoParams, EmlsrParams>;

    /**
     * Mode Tuple subfield
     * IEEE 802.11bn D1.0 Figure 9-aa57
     */
    struct ModeTuple
    {
        uint8_t modeId : 6 {UhrMode::MAX_MODE_ID}; ///< Mode ID
        uint8_t modeEnable : 1 {0};                ///< Mode Enable
        uint8_t modeUpdate : 1 {0};                ///< Mode Update
        std::optional<uint8_t> modeLength;         ///< Mode Length
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

    /**
     * @param delay the delay expressed in UHR mode Specific Parameters
     * @return the encoded value for the UHR mode Specific Parameters subfield
     */
    static uint8_t EncodeDelay(const Time& delay);

    /**
     * @param value the value for the UHR mode Specific Parameters subfield
     * @return the corresponding delay
     */
    static Time DecodeDelay(uint8_t value);

    std::vector<ModeTuple> m_modeTuples; ///< Mode Tuple fields

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* UHR_MODE_CHANGE_H */
