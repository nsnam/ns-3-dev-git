/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_CAPABILITIES_H
#define UHR_CAPABILITIES_H

#include "ns3/wifi-export.h"
#include "ns3/wifi-information-element.h"

#include <array>
#include <optional>

namespace ns3
{

/// Size of the UHR MAC Capabilities Information field (Figure 9-aa9 of 802.11bn/D2.0)
constexpr uint16_t WIFI_UHR_MAC_CAPABILITIES_SIZE_B = 6;

/// Size of the UHR PHY Capabilities Information field (Figure 9-aa10 of 802.11bn/D2.0)
constexpr uint16_t WIFI_UHR_PHY_CAPABILITIES_SIZE_B = 5;

/// Size of the fixed part of the DBE Capability Parameters field (Figure 9-aa11 of 802.11bn/D2.0)
constexpr uint16_t WIFI_UHR_DBE_CAPABILITY_PARAMETERS_HEADER_SIZE_B = 3;

/// Size of a DBE Capabilities (BW = 160/320 MHz) field (Figures 9-aa12/9-aa13 of 802.11bn/D2.0)
constexpr uint16_t WIFI_UHR_DBE_BANDWIDTH_CAPABILITIES_SIZE_B = 4;

/**
 * @ingroup wifi
 *
 * The IEEE 802.11bn UHR Capabilities
 */
class WIFI_EXPORT UhrCapabilities : public WifiInformationElement
{
  public:
    /**
     * UHR MAC Capabilities Information field (Figure 9-aa9 of 802.11bn/D2.0)
     */
    struct UhrMacCapabilities
    {
        uint8_t dpsSupport : 1 {0};                 //!< DPS Support
        uint8_t dpsAssistingSupport : 1 {0};        //!< DPS Assisting Support
        uint8_t dpsApStaticHcmSupport : 1 {0};      //!< DPS Static HCM Support
        uint8_t npcaSupport : 1 {0};                //!< NPCA Support
        uint8_t enhancedBsrSupport : 1 {0};         //!< Enhanced BSR Support
        uint8_t additionalMappedTidSupport : 1 {0}; //!< Additional Mapped TID Support
        uint8_t eotspSupport : 1 {0};               //!< EOTSP support
        uint8_t dsoSupport : 1 {0};                 //!< DSO support
        uint8_t pEdcaSupport : 1 {0};               //!< P-EDCA support
        uint8_t dbeSupport : 1 {0};                 //!< DBE support
        uint8_t ulLliSupport : 1 {0};               //!< UL LLI support
        uint8_t p2pLliSupport : 1 {0};              //!< Peer-to-Peer LLI support
        uint8_t puoSupport : 1 {0};                 //!< PUO support
        uint8_t apPuoSupport : 1 {0};               //!< AP PUO support
        uint8_t duoSupport : 1 {0};                 //!< DUO support
        //! OM Control UL MU Data Disable RX Support
        uint8_t omCtrlUlMuDataDisableRxSupport : 1 {0};
        uint8_t aomSupport : 1 {0};             //!< AOM support
        uint8_t ifcsLocationSupport : 1 {0};    //!< IFCS Location support
        uint8_t uhrTrsSupport : 1 {0};          //!< UHR TRS support
        uint8_t txspgSupport : 1 {0};           //!< TXSPG support
        uint8_t txspgTxopReturnSupport : 1 {0}; //!< TXOP Return Support In TXSPG
        //! UHR Operating Mode And Parameters Update Timeout
        uint8_t uhrOpModeAndParamsUpdateTimeout : 4 {0};
        //! Parameter Update Adv Notification Interval
        uint8_t paramUpdateAdvNotificationInterval : 5 {0};
        uint8_t updateIndicationTimIntervals : 5 {0};   //!< Update Indication In TIM Interval
        uint8_t boundedEss : 1 {0};                     //!< Bounded ESS
        uint8_t btmAssurance : 1 {0};                   //!< BTM Assurance
        uint8_t coBfSupport : 1 {0};                    //!< Co-BF Support
        uint8_t coSrSupport : 1 {0};                    //!< Co-SR Support
        uint8_t mapcEnhancedMeasurementSupport : 1 {0}; //!< MAPC Enhanced Measurement Support
        uint8_t hgRtwtSupport : 1 {0};                  //!< HG RTWT Support
        uint8_t extendedChannelUsageSupport : 1 {0};    //!< Extended Channel Usage Support

        /**
         * Set the UHR Operating Mode And Parameters (OMP) Update Timeout subfield of the UHR MAC
         * Capabilities field.
         *
         * @param timeout the Transition Timeout (Table 9-bb3 of 802.11bn/D2.0)
         */
        void SetOmpUpdateTimeout(const Time& timeout);

        /**
         * Get the UHR Operating Mode And Parameters (OMP) Update Timeout subfield subfield of the
         * UHR MAC Capabilities field.
         *
         * @return the Transition Timeout
         */
        Time GetOmpUpdateTimeout() const;

        /**
         * Get the size of the serialized UHR MAC capabilities subfield
         *
         * @return the size of the serialized UHR MAC capabilities subfield
         */
        uint16_t GetSize() const;

        /**
         * Serialize the UHR MAC capabilities subfield
         *
         * @param start iterator pointing to where the UHR MAC capabilities subfield should be
         * written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the UHR MAC capabilities subfield
         *
         * @param start iterator pointing to where the UHR MAC capabilities subfield should be read
         * from
         * @return the number of bytes read
         */
        uint16_t Deserialize(Buffer::Iterator start);
    };

    /**
     * UHR PHY Capabilities Information field (Figure 9-aa10 of 802.11bn/D2.0)
     */
    struct UhrPhyCapabilities
    {
        uint8_t maxNssRxForSoundingNdpLe80Mhz : 1 {0}; //!< Max Nss Rx for sounding NDP (<= 80 MHz)
        //! Max Nss,total Rx for DL MU-MIMO (<= 80 MHz)
        uint8_t maxNssTotalRxForDlMuMimoLe80Mhz : 1 {0};
        uint8_t maxNssRxForSoundingNdp160Mhz : 1 {0}; //!< Max Nss Rx for sounding NDP (= 160 MHz)
        //! Max Nss,total Rx for DL MU-MIMO (= 160 MHz)
        uint8_t maxNssTotalRxForDlMuMimo160Mhz : 1 {0};
        uint8_t maxNssRxForSoundingNdp320Mhz : 1 {0}; //!< Max Nss Rx for sounding NDP (= 320 MHz)
        //! Max Nss,total Rx for DL MU-MIMO (= 320 MHz)
        uint8_t maxNssTotalRxForDlMuMimo320Mhz : 1 {0};
        uint8_t elrTxSupport : 1 {0};                //!< ELR Tx Support
        uint8_t elrRxSupport : 1 {0};                //!< ELR Rx Support
        uint8_t partialBwDlMuMimoSupport : 1 {0};    //!< Partial BW DL MU-MIMO Support
        uint8_t partialBwUlMuMimoSupport : 1 {0};    //!< Partial BW UL MU-MIMO Support
        uint8_t mcs15Support : 1 {0};                //!< MCS 15 Support
        uint8_t ldpc2xTxSupport : 1 {0};             //!< 2xLDPC Tx Support
        uint8_t ldpc2xRxSupport : 1 {0};             //!< 2xLDPC Rx Support
        uint8_t ueqmTxSupportAndMaxNssTx : 2 {0};    //!< UEQM Tx Support and Max Nss Tx
        uint8_t ueqmRxSupportAndMaxNssRx : 2 {0};    //!< UEQM Rx Support and Max Nss Rx
        uint8_t coBfJointSoundingSupport : 1 {0};    //!< Co-BF Joint Sounding Support
        uint8_t imTxSupport : 1 {0};                 //!< IM Tx Support
        uint8_t imRxSupport : 1 {0};                 //!< IM Rx Support
        uint8_t coSrMode1Support : 1 {0};            //!< Co-SR Mode 1 Support
        uint8_t coSrMode2Support : 1 {0};            //!< Co-SR Mode 2 Support
        uint8_t druDbw20InPbw20MhzSupport : 1 {0};   //!< DRU DBW=20 in PBW=20 MHz Support
        uint8_t druDbw40InPbw40MhzSupport : 1 {0};   //!< DRU DBW=40 in PBW=40 MHz Support
        uint8_t druDbw80InPbw80MhzSupport : 1 {0};   //!< DRU DBW=80 in PBW=80 MHz Support
        uint8_t druDbw80InPbw160MhzSupport : 1 {0};  //!< DRU DBW=80 in PBW=160 MHz Support
        uint8_t druDbw80InPbw320MhzSupport : 1 {0};  //!< DRU DBW=80 in PBW=320 MHz Support
        uint8_t druDbw20InPbwGe80MhzSupport : 1 {0}; //!< DRU DBW=20 in PBW>=80 MHz Support
        uint8_t druDbw40InPbwGe80MhzSupport : 1 {0}; //!< DRU DBW=40 in PBW>=80 MHz Support
        uint8_t druDbw60InPbwGe80MhzSupport : 1 {0}; //!< DRU DBW=60 in PBW>=80 MHz Support
        uint8_t druRruHybridModeSupport : 1 {0};     //!< DRU+RRU Hybrid mode Support

        /**
         * Get the size of the serialized UHR PHY capabilities subfield
         *
         * @return the size of the serialized UHR PHY capabilities subfield
         */
        uint16_t GetSize() const;

        /**
         * Serialize the UHR PHY capabilities subfield
         *
         * @param start iterator pointing to where the UHR PHY capabilities subfield should be
         * written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the UHR PHY capabilities subfield
         *
         * @param start iterator pointing to where the UHR PHY capabilities subfield should be read
         * from
         * @return the number of bytes read
         */
        uint16_t Deserialize(Buffer::Iterator start);
    };

    /**
     * DBE Capabilities (BW = 160/320 MHz) field (Figures 9-aa12/9-aa13 of 802.11bn/D2.0)
     */
    struct DbeBandwidthCapabilities
    {
        std::array<uint8_t, 3> ehtMcsMap{};  //!< EHT-MCS Map (24 bits)
        uint8_t nSoundingDimensions : 3 {0}; //!< Number Of Sounding Dimensions
        uint8_t nonOfdmaUlMuMimo : 1 {0};    //!< Non-OFDMA UL MU-MIMO
        uint8_t muBeamformer : 1 {0};        //!< MU Beamformer
        uint8_t beamformeeSs : 3 {0};        //!< Beamformee SS

        /**
         * Get the size of the serialized DBE Capabilities field
         *
         * @return the size of the serialized DBE Capabilities field
         */
        uint16_t GetSize() const;

        /**
         * Serialize the DBE Capabilities field
         *
         * @param start iterator pointing to where the field should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the DBE Capabilities field
         *
         * @param start iterator pointing to where the field should be read from
         * @return the number of bytes read
         */
        uint16_t Deserialize(Buffer::Iterator start);
    };

    /**
     * DBE Capability Parameters field (Figure 9-aa11 of 802.11bn/D2.0)
     */
    struct DbeCapabilityParameters
    {
        uint8_t maxSupportedDbeBandwidth : 3 {0};     //!< Maximum Supported DBE Bandwidth
        uint8_t dbeCapabilities160MhzPresent : 1 {0}; //!< DBE Capabilities (BW = 160 MHz) Present
        uint8_t dbeCapabilities320MhzPresent : 1 {0}; //!< DBE Capabilities (BW = 320 MHz) Present
        uint8_t maxDbeBandwidthSwitchTimePeriod{0};   //!< Maximum DBE Bandwidth Switch Time Period
        uint8_t dbeModeChangeInterval{0};             //!< DBE Mode Change Interval
        std::optional<DbeBandwidthCapabilities>
            dbeCapabilities160Mhz; //!< DBE Capabilities (BW = 160 MHz)
        std::optional<DbeBandwidthCapabilities>
            dbeCapabilities320Mhz; //!< DBE Capabilities (BW = 320 MHz)

        /**
         * Get the size of the serialized DBE Capability Parameters field
         *
         * @return the size of the serialized DBE Capability Parameters field
         */
        uint16_t GetSize() const;

        /**
         * Serialize the DBE Capability Parameters field
         *
         * @param start iterator pointing to where the field should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the DBE Capability Parameters field
         *
         * @param start iterator pointing to where the field should be read from
         * @return the number of bytes read
         */
        uint16_t Deserialize(Buffer::Iterator start);
    };

    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    void Print(std::ostream& os) const override;

    UhrMacCapabilities m_macCapabilities; //!< UHR MAC Capabilities Info subfield
    UhrPhyCapabilities m_phyCapabilities; //!< UHR PHY Capabilities Info subfield
    std::optional<DbeCapabilityParameters>
        m_dbeCapabilityParams; //!< DBE Capability Parameters (AP only, if DBE Support = 1)

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* UHR_CAPABILITIES_H */
